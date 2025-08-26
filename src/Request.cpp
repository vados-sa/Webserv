#include "Request.hpp"

Request::Request() : method_(""), reqPath_(""), fullpath_(""), is_cgi(false) {
};

Request::Request(const std::string &raw) : method_(""), reqPath_(""), fullpath_(""), is_cgi(false) {
    this->parseRequest(raw);
};

//Request::Request(const Request &obj) : method_(obj.method_), reqPath_(obj.reqPath_) {}

void Request::parseRequest(const std::string &raw) {
    std::string temp = raw;

    if (!this->parseRequestLine(temp)) {
        throw std::runtime_error("Failed to parse request line");
    }

    if (!this->parseHeaders(temp)) {
        throw std::runtime_error("Failed to parse request headers");
    }

    if (!this->parseBody(temp)) {
        throw std::runtime_error("Failed to parse request body");
    }
}

bool Request::parseRequestLine(std::string &raw)
{
    std::istringstream iss(raw);
    std::string method, path, version;

    if (!(iss >> method >> path >> version))
    {
        return false;
    }

    setMethod(method);
    std::string query;
    size_t qpos = path.find('?');
    if (qpos != std::string::npos)
    {
        query = path.substr(qpos + 1);
        path = path.substr(0, qpos);
    }

    std::string cleanPath = normalizePath(path);
    if (cleanPath.empty())
        return false;

    setreqPath(cleanPath);
    setQueryString(query);
    setVersion(version);

    size_t pos = raw.find("\r\n");
    if (pos == std::string::npos)
        return false;
    raw.erase(0, pos + 2);

    return true;
}

bool Request::parseHeaders(std::string &raw)
{
    std::string::size_type headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return (false);

    std::string headersBlock = raw.substr(0, headerEnd);
    raw.erase(0, headerEnd + 4);

    std::istringstream iss(headersBlock);
    std::string line;

    while (std::getline(iss, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line.empty())
            continue;

        std::string::size_type pos = line.find(":");
        if (pos == std::string::npos)
            return (false);

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        key.erase(0, key.find_first_not_of(" \t\r\n"));
        key.erase(key.find_last_not_of(" \t\r\n") + 1);
        value.erase(0, value.find_first_not_of(" \t\r\n"));
        value.erase(value.find_last_not_of(" \t\r\n") + 1);

        if (key.empty())
            return (false);

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        headers_[key] = value;
    }

    return (true);
}

bool Request::parseBody(std::string &raw)
{
    if (raw.empty() && method_ != "POST")
        return (true);

    std::map<std::string, std::string>::iterator it = headers_.find("content-length");
    if (it == headers_.end())
        return (false);

    const std::string &lengthStr = it->second;
    long length = -1;
    std::istringstream(lengthStr) >> length;

    if (length < 0)
        return (false);

    if (raw.empty() && method_ == "POST" && length != 0)
        return (false);

    if (raw.size() < static_cast<size_t>(length))
        return (false);

    body_= raw.substr(0, length);
    raw.erase(0, length);
    return (true);
}

std::ostream &operator<<(std::ostream &out, const Request &obj) {
    out << "Method: " << obj.getMethod() << std::endl
        << "Request path: " << obj.getreqPath() << std::endl
        << "Full path: " << obj.getfullPath() << std::endl
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

std::string normalizePath(const std::string &rawPath) {
    std::istringstream iss(rawPath);
    std::vector<std::string> parts;
    std::string token;

    while (std::getline(iss, token, '/')) {
        if (token.empty() || token == ".")
            continue ;
        else if (token == "..")
            parts.pop_back();
        else
            parts.push_back(token);
    }

    std::string normalized = "/";
    for (std::vector<std::string>::iterator it = parts.begin(); it != parts.end(); ++it)
    {
        normalized += *it;
        if (it + 1 != parts.end())
            normalized += "/";
    }
    return (normalized);
}