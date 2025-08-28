#include "Config.hpp"
#include "ConfigParser.hpp"

Config::Config(const std::string &filepath) : client_count(0) {
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

void Config::addServer(ServerConfig &server) {
    servers.push_back(server);
}

void Config::loadFromFile(const std::string &filepath) {
    ConfigParser parser;

    *this = parser.parseConfigFile(filepath);
}

bool Config::setupServer() {
	std::string errorMsg;
	if (validateBindings(errorMsg) == false) {
		std::cerr << errorMsg << std::endl;
		return false;
	}

	for (size_t i = 0; i < servers.size(); i++) {
        ServerSocket socketObj;
        if (!socketObj.setupServerSocket(servers[i].getPort(), servers[i].getHost()))
            return false;
        serverSockets.push_back(socketObj);
    }
    return true;
}

bool Config::validateBindings(std::string &errorMsg) const {
	std::map<int, PortState> ports;

	for (size_t i = 0; i < servers.size(); i++) {
		const ServerConfig &s = servers[i];
		const int port = s.getPort();
		const std::string &host = s.getHost();

		PortState &ps = ports[port];

		if (host == "0.0.0.0") {
			if (ps.anyTaken) {
				std::ostringstream oss;
                oss << "Duplicate binding: 0.0.0.0:" << port
                    << " already used by server#" << ps.anyServerIdx
                    << " (also requested by server#" << i << ")";
                errorMsg = oss.str();
                return false;
			}
			if (!ps.ipToServerIdx.empty()) {
                std::map<std::string,int>::const_iterator it = ps.ipToServerIdx.begin();
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
		else {
			if (ps.anyTaken) {
                std::ostringstream oss;
                oss << "Conflict: " << host << ":" << port
                    << " requested by server #" << i
                    << " but 0.0.0.0:" << port
                    << " is already used by server #" << ps.anyServerIdx;
                errorMsg = oss.str();
                return false;
            }
			std::map<std::string,int>::const_iterator it = ps.ipToServerIdx.find(host);
            if (it != ps.ipToServerIdx.end()) {
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

bool Config::run() {
	const int server_count = serverSockets.size();
	setupPollfdSet(server_count);
	return pollLoop(server_count);
}

void Config::setupPollfdSet(int server_count) {
    for (int i = 0; i < server_count; i++) {
	    pollfd server_pollfd = {serverSockets[i].getFd(), POLLIN, 0};
	    poll_fds.push_back(server_pollfd);
    }
}

bool Config::pollLoop(int server_count) {
	while (true) {
		int ready = poll(poll_fds.data(), poll_fds.size(), 5000);
		if (ready < 0) {
			cleanup();
			if (errno == EINTR) {
				std::cout << "📡 Signal received, gracefully shutting down..." << std::endl;
				return true ;
			}
			perror("poll failed");
			return false;
		}

		for (int i = poll_fds.size() - 1; i >= 0; --i) {

			if ((i < server_count)) {
                if (poll_fds[i].revents & POLLIN) {
                    handleNewConnection(poll_fds[i].fd, i);
                }
                continue ;
			}

			const int client_idx = i - server_count;
			if (clients[client_idx].isTimedOut(60))
            	handleIdleClient(client_idx, i); // handle http error !!
			else if (poll_fds[i].revents & POLLIN)
            	handleClientRequest(i, client_idx);
			else if (poll_fds[i].revents & POLLOUT)
            	handleResponse(client_idx, i);
		}
	}

	return true;
}

void Config::handleNewConnection(int server_fd, int server_idx)
{
  struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

	client_count++;
	if (client_count > MAX_CLIENT) {
		std::cerr << "Refusing client, max reached\n";
		close(client_fd);
		return ;
	}

	if (client_fd < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			perror("accept failed");
		}
		return ;
	}

	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
		perror("fcntl failed for client");
		close(client_fd);
		return ;
	}

    clients.push_back(Client(client_fd, server_idx));
    pollfd client_pollfd = {client_fd, POLLIN, 0};
	poll_fds.push_back(client_pollfd);

	int port = ntohs(client_addr.sin_port);
	clients.back().setPort(port);
	std::cout << "🔌 New client (fd " << client_fd << ") connected on port: "
				<< port << "\n" << std::endl;
}

void Config::handleIdleClient(int client_idx, int pollfd_idx) {

	std::cout << "⏰ 408: Request Timeout\n Client fd("
			  << poll_fds[pollfd_idx].fd << ")/port(" << clients[client_idx].getPort()
			  << ")" << "\n" << std::endl;

	close(poll_fds[pollfd_idx].fd);
	poll_fds.erase(poll_fds.begin() + pollfd_idx);
	clients.erase(clients.begin() + client_idx);
}

<<<<<<< Updated upstream
/* Returns: 0 = not enough data yet
        >0 = number of bytes that form exactly one complete HTTP request
        -1 = malformed (e.g., bad chunk framing) -> you should send 400 */
static long extract_one_http_request(const std::string &buf) {
    // 1) headers?
=======
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

    while (std::getline(iss, line)) {
        if (!line.empty() && line[line.size()-1] == '\r')
			line.erase(line.size()-1);
        if (line.empty())
			break; // blank line before body

        std::string::size_type pos = line.find(':');
        if (pos == std::string::npos)
			return false;

        std::string k = line.substr(0, pos);
        std::string v = line.substr(pos + 1);

        // trim spaces/tabs
        while (!k.empty() && (k[0]==' '||k[0]=='\t'))
			k.erase(0,1);
        while (!k.empty() && (k[k.size()-1]==' '||k[k.size()-1]=='\t'))
			k.erase(k.size()-1);
        while (!v.empty() && (v[0]==' '||v[0]=='\t')) v.erase(0,1);
        while (!v.empty() && (v[v.size()-1]==' '||v[v.size()-1]=='\t'))
			v.erase(v.size()-1);

        // lowercase key
        for (size_t i=0;i<k.size();++i) k[i] = (char)std::tolower(k[i]);

        out_hmap[k] = v;
    }
    return true;
}

// If TE: chunked is present, parse chunk framing starting at body_start.
// Returns: 0 (need more), >0 (total bytes consumed for this request), -1 (malformed).
static long parse_chunked_body_consumed(const std::string &buf, size_t body_start)
{
    size_t p = body_start;

    while (true) {
        // find "<hex-size>\r\n"
        std::string::size_type crlf = buf.find("\r\n", p);
        if (crlf == std::string::npos)
			return 0; // need more size line

        std::string size_line = buf.substr(p, crlf - p);
        // strip any chunk extensions after ';'
        std::string::size_type sc = size_line.find(';');
        if (sc != std::string::npos) size_line.erase(sc);
        // trim
        while (!size_line.empty() && std::isspace((unsigned char)size_line[0]))
			size_line.erase(0,1);
        while (!size_line.empty() && std::isspace((unsigned char)size_line[size_line.size()-1]))
			size_line.erase(size_line.size()-1);

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

        if (chunk_size == 0) {
            std::string::size_type tend = buf.find("\r\n\r\n", p); // Trailers (optional): end with CRLFCRLF
            if (tend == std::string::npos)
				return 0; // need more trailers or final CRLFCRLF
            return (long)(tend + 4); // everything up to end of trailers
        } // else: loop for next chunk

    }
}

// Parse Content-Length value safely into 'out_len'.
// Returns false if missing/invalid/negative.
static bool parse_content_length(const std::map<std::string,std::string> &hmap, size_t &out_len)
{
    std::map<std::string,std::string>::const_iterator it = hmap.find("content-length");
    if (it == hmap.end()) {
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
>>>>>>> Stashed changes
    std::string::size_type hdr_end = buf.find("\r\n\r\n");
    if (hdr_end == std::string::npos) return 0;
    const std::string headers = buf.substr(0, hdr_end + 4);

    // Minimal header parse: case-insensitive key map
    // (Reuse your Request::parseHeaders normalization by copying parts of it if you like)
    std::map<std::string, std::string> hmap;
    {
        std::istringstream iss(headers);
        std::string line;

        // first line is the request line; skip storing it here
        if (!std::getline(iss, line)) return -1;

        while (std::getline(iss, line)) {
            if (!line.empty() && line[line.size()-1] == '\r') line.erase(line.size()-1);
            if (line.empty()) break;
            std::string::size_type pos = line.find(':');
            if (pos == std::string::npos) return -1;
            std::string k = line.substr(0, pos);
            std::string v = line.substr(pos + 1);
            // trim
            while (!k.empty() && (k[0]==' '||k[0]=='\t')) k.erase(0,1);
            while (!k.empty() && (k[k.size()-1]==' '||k[k.size()-1]=='\t')) k.erase(k.size()-1);
            while (!v.empty() && (v[0]==' '||v[0]=='\t')) v.erase(0,1);
            while (!v.empty() && (v[v.size()-1]==' '||v[v.size()-1]=='\t')) v.erase(v.size()-1);
            // lowercase key
            for (size_t i=0;i<k.size();++i) k[i] = (char)std::tolower(k[i]);
            hmap[k] = v;
        }
    }

    const size_t body_start = hdr_end + 4;

    // 2) Transfer-Encoding: chunked?
    {
        std::map<std::string,std::string>::const_iterator it = hmap.find("transfer-encoding");
        if (it != hmap.end()) {
<<<<<<< Updated upstream
            // very simple check: if contains "chunked"
            if (it->second.find("chunked") != std::string::npos) {
                // parse chunks
                size_t p = body_start;
                while (true) {
                    // find size line
                    std::string::size_type crlf = buf.find("\r\n", p);
                    if (crlf == std::string::npos) return 0; // need more bytes
                    // hex size may have extensions like ";foo=bar"
                    std::string size_line = buf.substr(p, crlf - p);
                    std::string::size_type sc = size_line.find(';');
                    if (sc != std::string::npos) size_line.erase(sc);
                    // trim
                    while (!size_line.empty() && isspace(size_line[0])) size_line.erase(0,1);
                    while (!size_line.empty() && isspace(size_line[size_line.size()-1])) size_line.erase(size_line.size()-1);
                    // parse hex
                    long chunk_size = 0;
                    std::istringstream hexin(size_line);
                    hexin >> std::hex >> chunk_size;
                    if (!hexin || chunk_size < 0) return -1; // malformed size
                    p = crlf + 2;
                    // need chunk data + CRLF
                    if (buf.size() < p + (size_t)chunk_size + 2) return 0; // need more
                    p += (size_t)chunk_size;
                    if (buf.compare(p, 2, "\r\n") != 0) return -1; // malformed
                    p += 2;

                    if (chunk_size == 0) {
                        // trailers until CRLFCRLF (or none)
                        std::string::size_type tend = buf.find("\r\n\r\n", p);
                        if (tend == std::string::npos) return 0; // need more
                        return (long)(tend + 4); // consumed bytes for full message
                    }
                }
            }
=======
            // Lowercase a copy of the value for robust "chunked" detection.
            std::string te = it->second;
            for (size_t i = 0; i < te.size(); ++i)
				te[i] = (char)std::tolower(te[i]);
            if (te.find("chunked") != std::string::npos)
                return parse_chunked_body_consumed(buf, body_start);
>>>>>>> Stashed changes
        }
    }

    // 3) Content-Length?
    size_t need = 0;
    {
        std::map<std::string,std::string>::const_iterator it = hmap.find("content-length");
        if (it != hmap.end()) {
            long n = -1;
            std::istringstream iss(it->second);
            iss >> n;
            if (!iss || n < 0) return -1; // malformed CL
            need = (size_t)n;
        } else {
            need = 0; // requests without TE/CL => no body
        }
    }

    const size_t have = buf.size() - body_start;
    if (have < need) return 0; // need more body bytes
    return (long)(body_start + need); // full message consumed
}


void Config::handleClientRequest(int pollfd_idx, int client_idx) {
	Client& client = clients[client_idx];
	const int client_fd = poll_fds[pollfd_idx].fd;

	if (client.getState() == Client::CONNECTED)
	client.setState(Client::SENDING_REQUEST);

	char buffer[4096];
	int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	if (bytes < 0) {
            return ; // check with valgrind if client_fd has to be closed
    }
	if (bytes == 0) {
        std::cout << "❌ Client disconnected: " << "\n" << "fd - " << client_fd << "\n"
								<< "port - " << client.getPort() << "\n" << std::endl;
		close(client_fd);
		poll_fds.erase(poll_fds.begin() + pollfd_idx);
		clients.erase(clients.begin() + (client_idx));
		return ;
    }

	client.appendRequestData(buffer, bytes);
	while (true) {
		long consumed = extract_one_http_request(client.getRequest());
		if (consumed == 0)
			break ;
		if (consumed < 0) {
			Response res(servers[client.getServerIndex()].getErrorPagesConfig());
			Request bad; bad.setVersion("HTTP/1.1");
			res.setPage(400, "Bad Request", true);
			client.setResponse(res.writeResponseString());
			client.setKeepAlive(false);
    		poll_fds[pollfd_idx].events = POLLIN | POLLOUT;
			return ;
		}

		std::string raw = client.getRequest().substr(0, (size_t)consumed);
		client.consumeRequestBytes((size_t)consumed);
		Request reqObj(raw);
		ServerConfig srv = servers[client.getServerIndex()];
        std::string response = buildRequestAndResponse(raw, srv, reqObj);
        client.setKeepAlive(reqObj.getHeaders());
		poll_fds[pollfd_idx].events = POLLIN | POLLOUT;
		client.setResponse(response);

		break ;
	}
}

void Config::handleResponse(int client_idx, int pollfd_idx) {
	if (sendResponse(pollfd_idx, client_idx) == true) {
		if (clients[client_idx].getKeepAlive() == false) {
			std::cout << "❌ Client disconnected:\nfd - " << poll_fds[pollfd_idx].fd
			<< "\nport - " << clients[client_idx].getPort() << "\n" << std::endl;

			close(poll_fds[pollfd_idx].fd);
			poll_fds.erase(poll_fds.begin() + pollfd_idx);
			clients.erase(clients.begin() + client_idx);
		} else
            poll_fds[pollfd_idx].events = POLLIN;
    }
}

bool Config::sendResponse(int pollfd_idx, int client_idx) {
	Client& client = clients[client_idx];
	int client_fd = poll_fds[pollfd_idx].fd;
	std::string response = client.getResponse();
	size_t already_sent= client.getBytesSent();
	size_t remaining = response.size() - already_sent;

	if (remaining == 0) return true;

	size_t bytes_sent = send(client_fd, response.c_str() + already_sent, remaining, 0);
	if (bytes_sent < 0) {
		perror("send failed");
		return false;
	}
	client.setBytesSent(bytes_sent);
	return client.getBytesSent() == response.size();

	return false;
}

void Config::cleanup() {
	for (size_t i = 1; i < poll_fds.size(); ++i) {
		close(poll_fds[i].fd);
	}
	clients.clear();
	poll_fds.clear();
}

std::ostream &operator<<(std::ostream &os, const Config &obj) {
    std::vector<ServerConfig> serversVector = obj.getServers();

    for (size_t i = 0; i < serversVector.size(); i ++) {
        os << serversVector[i] << std::endl;
    }
    return os;
}

/* bool Config::run() {
    const int server_count = serverSockets.size();
    for (int i = 0; i < server_count; i++) {
	    pollfd server_pollfd = {serverSockets[i].getFd(), POLLIN, 0};
	    poll_fds.push_back(server_pollfd);
    }

	while (true) {
		int ready = poll(poll_fds.data(), poll_fds.size(), 5000);
		if (ready < 0) {
			cleanup();
			if (errno == EINTR) {
                std::cout << "📡 Signal received, gracefully shutting down..." << std::endl;
                break ;
            }
            perror("poll failed");
            return false;
        }

		for (int i = poll_fds.size() - 1; i >= 0; --i) {

            if ((i < server_count)) {
                if (poll_fds[i].revents & POLLIN) {
                    handleNewConnection(poll_fds[i].fd);
                }
                continue ;
			} else {
                const int client_idx = i - server_count;

                if (clients[client_idx].isTimedOut(60)) {
                    std::cout << "⏰ 505: Gateway Timeout\n Client fd("
                    << poll_fds[i].fd << ")/port(" << clients[client_idx].getPort()
                    << ")" << "\n" << std::endl;

                    close(poll_fds[i].fd);
                    poll_fds.erase(poll_fds.begin() + i);
                    clients.erase(clients.begin() + client_idx);

                } else if (poll_fds[i].revents & POLLIN) {
                    handleClientRequest(i, client_idx);

                } else if (poll_fds[i].revents & POLLOUT) {
                    if (sendResponse(i, client_idx) == true) {
                        if (clients[client_idx].getKeepAlive() == false) {

							std::cout << "❌ Client disconnected:\nfd - " << poll_fds[i].fd
							<< "\nport - " << clients[client_idx].getPort() << "\n" << std::endl;

                            close(poll_fds[i].fd);
                            poll_fds.erase(poll_fds.begin() + i);
                            clients.erase(clients.begin() + client_idx);

                        } else
                            poll_fds[i].events = POLLIN;
                    }
                }
            }
		}
	}
	return true;
} */

const LocationConfig *matchLocation(const std::string &reqPath, const ServerConfig &srv) {
	const LocationConfig *bestMatch = NULL;
	size_t longest = 0;

	const std::vector<LocationConfig> &locations = srv.getLocations();

	for (size_t i = 0; i < locations.size(); ++i) {
		const std::string &prefix = locations[i].getUri();
		if (reqPath.compare(0, prefix.size(), prefix) == 0) {
			if (prefix.size() > longest) {
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
    reqObj.setFullPath(loc.getRoot() + remainingPath);
}