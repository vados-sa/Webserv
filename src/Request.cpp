#include "Request.hpp"
#include "Utils.hpp"
#include "HttpException.hpp"
#include <sstream>
#include <vector>

Request::Request() : isCgi_(false) {};

// Request::Request(const std::string &raw, int maxBodySize) : maxBodySize_(maxBodySize), isCgi_(false) {
//     this->parseRequest(raw);
// };

Request::Request(const std::string &raw) : isCgi_(false) {
    this->parseRequest(raw);
};

void Request::parseRequest(const std::string &raw) {
    std::string temp = raw;

    parseRequestLine(temp);
    parseHeaders(temp);
    parseBody(temp);
}

bool Request::parseRequestLine(std::string &raw)
{
    std::istringstream iss(raw);
    std::string method, path, version;

    if (!(iss >> method >> path >> version))
        throw HttpException(400, "Malformed request line", true);

    setMethod(method);
    if (method != "GET" && method != "POST" && method != "DELETE")
        throw HttpException(405, "Method Not Allowed", true);

    std::string query;
    size_t qpos = path.find('?');
    if (qpos != std::string::npos) {
        query = path.substr(qpos + 1);
        path = path.substr(0, qpos);
    }

    std::string cleanPath = util::normalizePath(path); /// turns "a/b/../c/./d" and turn it into a clean, normalized path "/a/c/d"
    if (cleanPath.empty())
        cleanPath = "/";
    if (!util::isValidPath(cleanPath))
        throw HttpException(400, "Bad Request", true);

    setReqPath(cleanPath);
    setQueryString(query);
    if (version != "HTTP/1.0" && version != "HTTP/1.1")
        throw HttpException(400, "Bad Request: unsupported HTTP version", true);
    setVersion(version);

    size_t pos = raw.find("\r\n");
    if (pos == std::string::npos)
        throw HttpException(400, "Malformed request line ending", true);
    raw.erase(0, pos + 2);

    return (true);
}

bool Request::parseHeaders(std::string &raw)
{
    std::string::size_type headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        throw HttpException(400, "Malformed headers: missing CRLFCRLF", true);

    std::string headersBlock = raw.substr(0, headerEnd);
    raw.erase(0, headerEnd + 4);

    std::istringstream iss(headersBlock);
    std::string line;

    while (std::getline(iss, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        if (line.empty())
            continue;

        std::string::size_type pos = line.find(":");
        if (pos == std::string::npos)
            throw HttpException(400, "Malformed header line", true);

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        key.erase(0, key.find_first_not_of(" \t\r\n"));
        key.erase(key.find_last_not_of(" \t\r\n") + 1);
        value.erase(0, value.find_first_not_of(" \t\r\n"));
        value.erase(value.find_last_not_of(" \t\r\n") + 1);

        if (key.empty())
            throw HttpException(400, "Bad Request", true);

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        headers_[key] = value;
    }

    if (headers_.count("transfer-encoding")) {
        if (headers_["transfer-encoding"] != "chunked")
            throw HttpException(400, "Unsupported Transfer-Encoding", true);
    }

    return (true);
}

bool Request::parseBody(std::string &raw)
{
    if (raw.empty())
        return (true);

    std::map<std::string, std::string>::iterator it = headers_.find("transfer-encoding");
    if (!raw.empty() && it != headers_.end() && it->second == "chunked") {
        util::parseChunkedBody(raw);
        return (true);
    }

    it = headers_.find("content-length");
    if (it == headers_.end())
        throw HttpException(411, "Length Required", true);

    const std::string &lengthStr = it->second;
    long length = -1;
    std::istringstream(lengthStr) >> length;

    if (length < 0)
        throw HttpException(400, "Bad Request", true);

    if (raw.size() < static_cast<size_t>(length))
        throw HttpException(400, "Bad Request", true);

    body_= raw.substr(0, length);
    raw.erase(0, length);
    return (true);
}

std::ostream &operator<<(std::ostream &out, const Request &obj) {
    out << "Method: " << obj.getMethod() << std::endl
        << "Request path: " << obj.getReqPath() << std::endl
        << "Full path: " << obj.getFullPath() << std::endl
        << "Version: " << obj.getVersion() << std::endl
        << " ----- " << std::endl
        << "Headers: " << std::endl
        << obj.getHeaders() << std::endl
        << " ----- " << std::endl
        << "Body: " << obj.getBody() << std::endl;
    return (out);
}

std::ostream &operator<<(std::ostream &os, const std::map<std::string, std::string> &map)
{
    for (std::map<std::string, std::string>::const_iterator it = map.begin(); it != map.end(); ++it)
        os << it->first << ": " << it->second << '\n';
    return os;
}


