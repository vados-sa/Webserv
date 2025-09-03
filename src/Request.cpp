#include "Request.hpp"
#include <sstream>
#include <vector>

Request::Request() : isCgi_(false) {};

Request::Request(const std::string &raw) : isCgi_(false) {
    this->parseRequest(raw);
};

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

    setReqPath(cleanPath);
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
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

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

    std::map<std::string, std::string>::iterator it = headers_.find("transfer-encoding");
    if (!raw.empty() && it != headers_.end() && it->second == "chunked") {
        return parseChunkedBody(raw); // use the helper we wrote earlier
    }

    // 🔹 2. Otherwise, fall back to Content-Length
    it = headers_.find("content-length");
    if (it == headers_.end())
        return false;

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

bool Request::parseChunkedBody(std::string &raw)
{
    body_.clear();
    std::string::size_type pos = 0;

    while (true)
    {
        // 1. Find end of the chunk size line
        std::string::size_type endline = raw.find("\r\n", pos);
        if (endline == std::string::npos)
            return false; // not enough data yet

        // 2. Parse chunk size (hexadecimal)
        std::string sizeStr = raw.substr(pos, endline - pos);
        std::istringstream iss(sizeStr);
        std::size_t chunkSize = 0;
        iss >> std::hex >> chunkSize;

        if (iss.fail())
            return false; // bad chunk size

        pos = endline + 2; // move past "\r\n"

        // 3. If chunk size is 0 → end of body
        if (chunkSize == 0)
        {
            // skip trailing CRLF after last 0\r\n
            std::string::size_type trailerEnd = raw.find("\r\n", pos);
            if (trailerEnd == std::string::npos)
                return false;             // wait for end
            raw.erase(0, trailerEnd + 2); // consume used data
            return true;
        }

        // 4. Make sure we have the whole chunk data
        if (raw.size() < pos + chunkSize + 2)
            return false; // wait for more

        // 5. Append chunk to body
        body_.append(raw, pos, chunkSize);

        pos += chunkSize;

        // 6. Chunks are followed by CRLF
        if (raw.substr(pos, 2) != "\r\n")
            return false; // malformed
        pos += 2;
    }
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

std::string normalizePath(const std::string &rawPath) {
    std::istringstream iss(rawPath);
    std::vector<std::string> parts;
    std::string token;

    while (std::getline(iss, token, '/')) {
        if (token.empty() || token == ".")
            continue ;
        else if (token == "..") {
            if (!parts.empty())
                parts.pop_back();
        }
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

