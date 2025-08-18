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

int Request::parseRequestLine(std::string &raw)
{

    // ----- PARSE METHOD -----
    int len = raw.find(" ");
    std::string possibleMethod = raw.substr(0, len);
    raw = raw.substr(len + 1);
    method_ = possibleMethod;

    // ----- PARSE PATH ------
    len = raw.find(" ");
    std::string possiblePath = raw.substr(0, len);
    raw = raw.substr(len + 1);
    if (possiblePath[0] != '/')
        return (0);

    // Split query string
    std::string::size_type qpos = possiblePath.find('?');
    if (qpos != std::string::npos)
    {
        query_string = possiblePath.substr(qpos + 1);
        path_ = possiblePath.substr(0, qpos);
    }
    else
    {
        query_string.clear();
        path_ = possiblePath;
    }

    // Default index.html
    if (path_ == "/")
        path_.append("index.html");

    // // ----- CGI DETECTION -----
    // obj->is_cgi = false;
    // if (!loc.cgi_extension.empty())
    // {
    //     std::string ext = loc.cgi_extension;
    //     if (obj->path_.size() >= ext.size() &&
    //         obj->path_.substr(obj->path_.size() - ext.size()) == ext)
    //     {
    //         obj->is_cgi = true;
    //     }
    // }

    // ----- PARSE VERSION ---
    len = raw.find("\r\n");
    std::string possibleVersion = raw.substr(0, len);
    raw = raw.substr(len + 2);
    if (possibleVersion.substr(0, 4) != "HTTP")
        return (0);
    version_ = possibleVersion;

    return (1);
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