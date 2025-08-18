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

    setMethod(method);
    setPath(path); //i have to check later if its "/" and get the info from configparser
    setVersion(version);

    size_t pos = raw.find("\r\n");
    if (pos == std::string::npos)
        return (false);
    raw.erase(0, pos + 2);

    return (true)
}

int Request::parseHeaders(std::string &raw)
{
    std::map<std::string, std::string> headers;
    int len;
    std::string possibleKey;

    while (raw.substr(0, 4).compare("\r\n\r\n"))
    {
        len = raw.find(":");
        if (len < 0)
            return (0);
        possibleKey = raw.substr(0, len);
        raw = raw.substr(len + 2); // considering ": ", im not considering multiple whitespaces yet
        len = raw.find("\r\n");
        if (len < 0)
            return (0);
        std::string possibleValue = raw.substr(0, len);
        // insert a check if possibleValue is empty?
        headers[possibleKey] = possibleValue;
        raw = raw.substr(len);
        if (raw.substr(0, 4).compare("\r\n\r\n"))
            raw = raw.substr(2);
    }
    raw = raw.substr(4);
    headers_ = headers;
    return (1);
}

int Request::parseBody(std::string &raw)
{
    if (raw.empty())
        return (1); //for now its ok body to be empty, but maybe we should allow it to be empty if its POST.
    std::map<std::string, std::string>::iterator it;
    it = headers_.find("Content-Length");
    if (it == headers_.end())
        return (0);
    std::string lengthStr = headers_["Content-Length"];
    int lenght;
    std::istringstream(lengthStr) >> lenght; //i wonder if we can use this to convert from string to int
    body_= raw;
    return (1);
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


// std::string Request::getQueryString() const {
//     size_t pos = path_.find('?');
//     if (pos != std::string::npos) {
//         return path_.substr(pos + 1); // Extract everything after '?'
//     }
//     return ""; // No query string found
// }