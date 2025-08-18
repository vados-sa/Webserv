#include "Request.hpp"

Request::Request() : method_(""), path_("") {
};

Request::Request(const Request &obj) : method_(obj.method_), path_(obj.path_) {}

Request Request::parseRequest(const std::string &raw)
{
    Request helperReqObj;
    std::string temp = raw;

    if (!helperReqObj.parseRequestLine(temp)) {
        throw std::runtime_error("Failed to parse request line");
    }
    if (!helperReqObj.parseHeaders(temp)) {
        throw std::runtime_error("Failed to parse request headers");
    }
    if (!helperReqObj.parseBody(temp)) {
        throw std::runtime_error("Failed to parse request body");
    }

    return (helperReqObj);
}

bool Request::parseRequestLine(std::string &raw)
{
    std::istringstream iss(raw);

    std::string method, path, version;

    if (!(iss >> method >> path >> version)) {
        return (false);
    }

    if (method != "GET" && method != "POST" && method != "DELETE")
        return (false);

    setMethod(method);

    std::string cleanPath = normalizePath(path);
    if (cleanPath.empty())
        return (false);

    setPath(cleanPath);
    setVersion(version);

    size_t pos = raw.find("\r\n");
    if (pos == std::string::npos)
        return (false);
    raw.erase(0, pos + 2);

    return (true);
}

bool Request::parseHeaders(std::string &raw)
{
    std::map<std::string, std::string> headers;

    std::istringstream iss (raw);
    std::string line;
    size_t consumed = 0;

    while (std::getline(iss, line))
    {
        consumed += line.size() + 1;

        if (!line.empty() && line[line.length() - 1] == '\r')
            line.erase(line.length() - 1);

        if (line.empty())
            break;

        std::string::size_type pos = line.find(":");
        if (pos == std::string::npos)
            return (false); // mal formed request; every header MUST have ':'

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        key.erase(0, key.find_first_not_of(" \t\r\n"));
        key.erase(key.find_last_not_of(" \t\r\n") + 1);
        value.erase(0, value.find_first_not_of(" \t\r\n"));
        value.erase(value.find_last_not_of(" \t\r\n") + 1);

        if (key.empty())
            return (false); //a header cant have an empty key but may have an empty value

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        headers[key] = value;
    }

    std::string::size_type headerEnd = raw.find("\r\n\r\n");
    if (headerEnd != std::string::npos)
        raw.erase(0, headerEnd + 4);
    else
        return (false);

    headers_ = headers;
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
        return false; // invalid Content-Length

    if (raw.empty() && method_ == "POST" && length != 0)
        return false; // POST with missing body

    if (raw.size() < static_cast<size_t>(length))
        return false; // incomplete body

    body_= raw.substr(0, length);
    raw.erase(0, length);
    return (true);
}

std::ostream &operator<<(std::ostream &out, const Request &obj) {
    out << "Method: " << obj.getMethod() << std::endl
        << "Path: " << obj.getPath() << std::endl
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

std::string &normalizePath(const std::string &rawPath) {
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