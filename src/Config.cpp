#include "Config.hpp"
#include "ConfigParser.hpp"
#include "HttpException.hpp"

Config::Config(const std::string &filepath) : client_count(0)
{
    loadFromFile(filepath);
}

Config &Config::operator=(const Config &other)
{
    if (this != &other)
    {
        this->servers = other.servers;
    }
    return *this;
}

void Config::addServer(ServerConfig &server)
{
    servers.push_back(server);
}

void Config::loadFromFile(const std::string &filepath)
{
    ConfigParser parser;

    *this = parser.parseConfigFile(filepath);
}

bool Config::setupServer()
{
    std::string errorMsg;
    if (validateBindings(errorMsg) == false)
    {
        logs(ERROR, errorMsg);
        return false;
    }

    for (size_t i = 0; i < servers.size(); i++)
    {
        ServerSocket socketObj;
        if (!socketObj.setupServerSocket(servers[i].getPort(), servers[i].getHost()))
            return false;
        else {
            std::ostringstream oss;
            oss << "Server listening on " << (socketObj.getHost().empty() ? "0.0.0.0" : socketObj.getHost())
            << ":" << socketObj.getPort(); 
            std::string msg = oss.str();
            logs(INFO, msg);
        }
        serverSockets.push_back(socketObj);
    }
    return true;
}

bool Config::validateBindings(std::string &errorMsg) const
{
    std::map<int, PortState> ports;

    for (size_t i = 0; i < servers.size(); i++)
    {
        const ServerConfig &s = servers[i];
        const int port = s.getPort();
        const std::string &host = s.getHost();

        PortState &ps = ports[port];

        if (host == "0.0.0.0")
        {
            if (ps.anyTaken)
            {
                std::ostringstream oss;
                oss << "Duplicate binding: 0.0.0.0:" << port
                    << " already used by server#" << ps.anyServerIdx
                    << " (also requested by server#" << i << ")";
                errorMsg = oss.str();
                return false;
            }
            if (!ps.ipToServerIdx.empty())
            {
                std::map<std::string, int>::const_iterator it = ps.ipToServerIdx.begin();
                std::ostringstream oss;
                oss << "Conflict: 0.0.0.0:" << port
                    << " requested by server #" << i
                    << " but " << it->first << ":" << port
                    << " is already used by server #" << it->second;
                errorMsg = oss.str();
                return false;
            }
            ps.anyTaken = true;
            ps.anyServerIdx = static_cast<int>(i);
        }
        else
        {
            if (ps.anyTaken)
            {
                std::ostringstream oss;
                oss << "Conflict: " << host << ":" << port
                    << " requested by server #" << i
                    << " but 0.0.0.0:" << port
                    << " is already used by server #" << ps.anyServerIdx;
                errorMsg = oss.str();
                return false;
            }
            std::map<std::string, int>::const_iterator it = ps.ipToServerIdx.find(host);
            if (it != ps.ipToServerIdx.end())
            {
                std::ostringstream oss;
                oss << "Duplicate binding: " << host << ":" << port
                    << " already used by server #" << it->second
                    << " (also requested by server #" << i << ")";
                errorMsg = oss.str();
                return false;
            }
            ps.ipToServerIdx[host] = static_cast<int>(i);
        }
    }

    return true;
}

bool Config::run()
{
    const int server_count = serverSockets.size();
    setupPollfdSet(server_count);
    return pollLoop(server_count);
}

void Config::setupPollfdSet(int server_count)
{
    for (int i = 0; i < server_count; i++)
    {
        pollfd server_pollfd = {serverSockets[i].getFd(), POLLIN, 0};
        poll_fds.push_back(server_pollfd);
    }
}

bool Config::pollLoop(int server_count)
{
    while (true)
    {
        int ready = poll(poll_fds.data(), poll_fds.size(), 5000);
        if (ready < 0)
        {
            cleanup();
            if (errno == EINTR) {
                logs(INFO, "Signal received, shutting down the server...\n");
                return true;
            }
            perror("poll failed");
            return (false);
        }

        for (int i = poll_fds.size() - 1; i >= 0; --i)
        {
            const short revent = poll_fds[i].revents;

            if (revent & (POLLERR | POLLHUP | POLLNVAL)) {
                std::ostringstream oss;
                if (i < server_count) {
                    oss << "Critical poll event on server socket fd=" << poll_fds[i].fd ;
                    std::string msg = oss.str();
                    logs(ERROR, msg);
                    cleanup();
                    return false;
                }
                int client_idx = i - server_count;
                if (revent & POLLERR) oss << "POLLERR";
                if (revent & POLLHUP) oss << "POLLHUP";
                if (revent & POLLNVAL) oss << "POLLNVAL";
                oss << " event on client fd=" << poll_fds[i].fd << " port=" << servers[clients[client_idx].getServerIndex()].getPort();
                logs(INFO, oss.str());
                close(poll_fds[i].fd);
                poll_fds.erase(poll_fds.begin() + i);
                clients.erase(clients.begin() + client_idx);
                client_count--;
                continue;
            }

            if (i < server_count)
            {
                if (revent & POLLIN)
                    handleNewConnection(poll_fds[i].fd, i);
                continue;
            }

            const int client_idx = i - server_count;
            try {
                if (clients[client_idx].isTimedOut(60) && clients[client_idx].getState() != Client::IDLE)
                    handleIdleClient(client_idx, i);
                else if (poll_fds[i].revents & POLLIN)
                    handleClientRequest(i, client_idx);
                else if (poll_fds[i].revents & POLLOUT)
                    handleResponse(client_idx, i);
            } catch (const HttpException &e) {
                ServerConfig srv = this->servers[clients[client_idx].getServerIndex()];
                std::map<int, std::string> error_pages = srv.getErrorPagesConfig();
                Response res(error_pages, e.getStatusCode(), e.what(), e.getError());
                std::string res_string = res.writeResponseString(); //some checks maybe?
                clients[client_idx].setResponseBuffer(res_string);
                handleResponse(client_idx, i);
                //poll_fds[pollfd_idx].events = POLLOUT;
            }
        }
    }

    return true;
}

void Config::handleNewConnection(int server_fd, int server_idx)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

    if (client_fd < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            perror("accept failed");
        }
        return;
    }

    if (client_count >= MAX_CLIENT)
    {
        std::string msg = "Refusing client, max reached";
        logs(ERROR, msg);
        close(client_fd);
        return;
    }

    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        perror("fcntl failed for client");
        close(client_fd);
        return;
    }

    clients.push_back(Client(client_fd, server_idx));
    pollfd client_pollfd = {client_fd, POLLIN, 0};
    poll_fds.push_back(client_pollfd);

    client_count++;

    int port = ntohs(client_addr.sin_port);
    clients.back().setPort(port);
    std::ostringstream oss;
    oss << "Accepted client fd=" << client_fd << " port=" << servers[server_idx].getPort();
    std::string msg = oss.str();
    logs(INFO, msg);
}

void Config::handleIdleClient(int client_idx, int pollfd_idx)
{
    Client &client = clients[client_idx];
    ServerConfig srv = servers[client.getServerIndex()];

    std::ostringstream oss;
    oss << "Request Timeout\nClient fd(" << poll_fds[pollfd_idx].fd
        << ")/port(" << clients[client_idx].getPort() << ")";
    std::string errorMessage = oss.str();

    poll_fds[pollfd_idx].events = POLLOUT;
    client.setState(Client::IDLE);

    throw HttpException(408, errorMessage, true);
}

// Parse the header block (up to and incl. the CRLFCRLF) into a lowercased key map.
// Returns false on malformed headers (e.g., missing colon).
static bool parse_headers_block(const std::string &headers,
                                std::map<std::string, std::string> &out_hmap)
{
    std::istringstream iss(headers);
    std::string line;

    // First line is the request line; we skip storing it here.
    if (!std::getline(iss, line))
        return false;

    while (std::getline(iss, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            break; // blank line before body

        std::string::size_type pos = line.find(':');
        if (pos == std::string::npos)
            return false;

        std::string k = line.substr(0, pos);
        std::string v = line.substr(pos + 1);

        // trim spaces/tabs
        while (!k.empty() && (k[0] == ' ' || k[0] == '\t'))
            k.erase(0, 1);
        while (!k.empty() && (k[k.size() - 1] == ' ' || k[k.size() - 1] == '\t'))
            k.erase(k.size() - 1);
        while (!v.empty() && (v[0] == ' ' || v[0] == '\t'))
            v.erase(0, 1);
        while (!v.empty() && (v[v.size() - 1] == ' ' || v[v.size() - 1] == '\t'))
            v.erase(v.size() - 1);

        // lowercase key
        for (size_t i = 0; i < k.size(); ++i)
            k[i] = (char)std::tolower(k[i]);

        out_hmap[k] = v;
    }
    return true;
}

// If TE: chunked is present, parse chunk framing starting at body_start.
// Returns: 0 (need more), >0 (total bytes consumed for this request), -1 (malformed).
static long parse_chunked_body_consumed(const std::string &buf, size_t body_start)
{
    size_t p = body_start;

    while (true)
    {
        // find "<hex-size>\r\n"
        std::string::size_type crlf = buf.find("\r\n", p);
        if (crlf == std::string::npos)
            return 0; // need more size line

        std::string size_line = buf.substr(p, crlf - p);
        // strip any chunk extensions after ';'
        std::string::size_type sc = size_line.find(';');
        if (sc != std::string::npos)
            size_line.erase(sc);
        // trim
        while (!size_line.empty() && std::isspace((unsigned char)size_line[0]))
            size_line.erase(0, 1);
        while (!size_line.empty() && std::isspace((unsigned char)size_line[size_line.size() - 1]))
            size_line.erase(size_line.size() - 1);

        long chunk_size = -1;
        {
            std::istringstream hexin(size_line);
            hexin >> std::hex >> chunk_size;
            if (!hexin || chunk_size < 0)
                return -1; // invalid size
        }

        p = crlf + 2; // after the size line CRLF

        // need chunk_size bytes + CRLF
        if (buf.size() < p + (size_t)chunk_size + 2)
            return 0; // incomplete
        p += (size_t)chunk_size;
        if (buf.compare(p, 2, "\r\n") != 0)
            return -1; // missing CRLF after data
        p += 2;

        if (chunk_size == 0)
        {
            // // último chunk, fim do corpo
            // if (buf.size() < p + 2)
            //     return 0;         // ainda não recebeu o CRLF final
            return (long)(p + 2); // posição logo após o CRLF do último chunk
        }
    }
}

// Parse Content-Length value safely into 'out_len'.
// Returns false if missing/invalid/negative.
static bool parse_content_length(const std::map<std::string, std::string> &hmap, size_t &out_len)
{
    std::map<std::string, std::string>::const_iterator it = hmap.find("content-length");
    if (it == hmap.end())
    {
        out_len = 0;
        return true;
    } // no body by default

    // Accept only plain decimal; reject negatives or junk.
    long n = -1;
    std::istringstream iss(it->second);
    iss >> n;
    if (!iss || n < 0)
        return false;
    out_len = (size_t)n;
    return true;
}

static long extract_one_http_request(const std::string &buf)
{
    // 1) Need end of headers
    std::string::size_type hdr_end = buf.find("\r\n\r\n");
    if (hdr_end == std::string::npos)
        return 0;
    const size_t body_start = hdr_end + 4;
    const std::string headers = buf.substr(0, body_start);

    // 2) Parse headers
    std::map<std::string, std::string> hmap;
    if (!parse_headers_block(headers, hmap))
        return -1;

    // 3) Transfer-Encoding: chunked ? (case-insensitive check on value)
    {
        std::map<std::string, std::string>::const_iterator it = hmap.find("transfer-encoding");
        if (it != hmap.end())
        {
            // Lowercase a copy of the value for robust "chunked" detection.
            std::string te = it->second;
            for (size_t i = 0; i < te.size(); ++i)
                te[i] = (char)std::tolower(te[i]);
            if (te.find("chunked") != std::string::npos)
                return parse_chunked_body_consumed(buf, body_start);
        }
    }

    // 4) Else: Content-Length?
    size_t need = 0;
    if (!parse_content_length(hmap, need))
        return -1; // malformed CL

    const size_t have = (buf.size() >= body_start) ? (buf.size() - body_start) : 0;
    if (have < need)
        return 0;

    return (long)(body_start + need); // headers + body bytes
}

void Config::handleClientRequest(int pollfd_idx, int client_idx)
{
    Client &client = clients[client_idx];
    const int client_fd = poll_fds[pollfd_idx].fd;

    if (client.getState() == Client::CONNECTED)
        client.setState(Client::SENDING_REQUEST);

    char buffer[4096];
    int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes < 0)
    {
        std::ostringstream oss;
        oss << "recv() failed on client fd=" << client_fd << " port=" << servers[client.getServerIndex()].getPort();
        std::string msg = oss.str();
        logs(ERROR, msg);
        close(client_fd);
        poll_fds.erase(poll_fds.begin() + pollfd_idx);
        clients.erase(clients.begin() + client_idx);
        client_count--;
        return;
    }
    if (bytes == 0)
    {
        std::ostringstream oss;
        oss << "Disconnected client fd=" << client_fd << " port=" << servers[client.getServerIndex()].getPort();
        std::string msg = oss.str();
        logs(INFO, msg);
        close(client_fd);
        poll_fds.erase(poll_fds.begin() + pollfd_idx);
        clients.erase(clients.begin() + (client_idx));
        client_count--;
        return;
    }

    client.appendRequestData(buffer, bytes);
    while (true)
    {
        long consumed = extract_one_http_request(client.getRequest());
        if (consumed == 0)
            break;
        if (consumed < 0) {
            client.setKeepAlive(false);
            poll_fds[pollfd_idx].events = POLLIN | POLLOUT;
            throw HttpException(400, "Bad Request", true);
        }
        std::string raw = client.getRequest().substr(0, (size_t)consumed);
        client.consumeRequestBytes((size_t)consumed);
        Request reqObj(raw);
        //std::cout << "This is the raw request: " << raw << std::endl;
        ServerConfig srv = servers[client.getServerIndex()];
        std::string response = buildRequestAndResponse(raw, srv, reqObj);
        client.setKeepAlive(reqObj);
        poll_fds[pollfd_idx].events = POLLIN | POLLOUT;
        client.setResponseBuffer(response);
        break;
    }
}

void Config::handleResponse(int client_idx, int pollfd_idx)
{
    Client &client = clients[client_idx];
    int client_fd = client.getFd();
    std::string responseStr = client.getResponse();
    size_t alreadySent = client.getBytesSent();
    size_t remaining = responseStr.size() - alreadySent;

    if (remaining > 0)
    {
        ssize_t bytes = send(client_fd, responseStr.c_str() + alreadySent, remaining, 0);
        if (bytes < 0)
        {
            std::ostringstream oss;
            oss << "send() failed on client fd=" << client_fd << " port=" << servers[client.getServerIndex()].getPort();
            std::string msg = oss.str();
            logs(ERROR, msg);
            close(client_fd);
            poll_fds.erase(poll_fds.begin() + pollfd_idx);
            clients.erase(clients.begin() + client_idx);
            client_count--;
            return;
        }
        if (bytes == 0)
            return ;
        client.setBytesSent(alreadySent + bytes);
    }

    // If all bytes sent
    if (client.getBytesSent() >= responseStr.size())
    {
        client.setBytesSent(0);
        alreadySent = 0;
        // delete client.getResponseObj(); // delete heap Response
        //client.setResponseObj(NULL);
        if (!client.getKeepAlive() || (client.getState() == Client::IDLE))
        {
            std::ostringstream oss;
            oss << "Disconnecting client fd=" << client_fd << " port=" << servers[client.getServerIndex()].getPort();
            std::string msg = oss.str();
            logs(INFO, msg);
            close(client_fd);
            poll_fds.erase(poll_fds.begin() + pollfd_idx);
            clients.erase(clients.begin() + client_idx);
            client_count--;
        }
        else
        {
            poll_fds[pollfd_idx].events = POLLIN;
        }
    }
}

void Config::cleanup()
{
    for (size_t i = 0; i < poll_fds.size(); ++i)
    {
        close(poll_fds[i].fd);
    }
    poll_fds.clear();
    serverSockets.clear();
    servers.clear();
    clients.clear();
    client_count = 0;
}

std::ostream &operator<<(std::ostream &os, const Config &obj)
{
    std::vector<ServerConfig> serversVector = obj.getServers();

    for (size_t i = 0; i < serversVector.size(); i++)
    {
        os << serversVector[i] << std::endl;
    }
    return os;
}

const LocationConfig *matchLocation(const std::string &reqPath, const ServerConfig &srv)
{
    const LocationConfig *bestMatch = NULL;
    size_t longest = 0;

    const std::vector<LocationConfig> &locations = srv.getLocations();

    for (size_t i = 0; i < locations.size(); ++i)
    {
        const std::string &prefix = locations[i].getUri();
        if (reqPath.compare(0, prefix.size(), prefix) == 0)
        {
            if (prefix.size() > longest)
            {
                longest = prefix.size();
                bestMatch = &locations[i];
            }
        }
    }
    return (bestMatch);
}

std::string buildRequestAndResponse(const std::string &raw, const ServerConfig &srv, Request &outReq)
{
    Request reqObj(raw);
    const LocationConfig *loc = matchLocation(reqObj.getReqPath(), srv);
    if (loc)
        applyLocationConfig(reqObj, *loc);
    outReq = reqObj;

    Response res(srv.getErrorPagesConfig());
    return (res.buildResponse(reqObj, *loc));
}

void applyLocationConfig(Request &reqObj, const LocationConfig &loc)
{
    std::string requestPath = reqObj.getReqPath();

    const std::string &ext = loc.getCgiExtension();
    if (!ext.empty() && requestPath.size() >= ext.size() &&
        requestPath.compare(requestPath.size() - ext.size(), ext.size(), ext) == 0) {
        reqObj.setIsCgi(true);
    }

    std::string remainingPath = requestPath.substr(loc.getUri().size());
    reqObj.setFullPath(loc.getRoot() + "/" + remainingPath);
}
