#include "Request.hpp"

Request::Request() : method_(""), path_("") {
};

Request::Request(const Request &obj) : method_(obj.method_), path_(obj.path_) {}

Request Request::parseRequest(const std::string &raw)
{
    Request helperReqObj;
    std::string temp = raw;

    if (!parseRequestLine(temp, &helperReqObj)) {
        std::cerr << "Failed to parse request line" << std::endl; // we can throw exceptions later, rn i dont want the program to quit
        return (helperReqObj);
    }
    if (!parseHeaders(temp, &helperReqObj)){
        std::cerr << "Failed to parse headers" << std::endl;
        return (helperReqObj);
    }
    if (!parseBody(temp, &helperReqObj)){
        std::cerr << "Failed to parse body" << std::endl;
        return (helperReqObj);
    }

    return (helperReqObj);
}

int Request::parseRequestLine(std::string &raw, Request *obj) {

    // ----- PARSE METHOD -----
    int len = raw.find(" ");
    std::string possibleMethod = raw.substr(0, len);
    raw = raw.substr(len + 1);
    if (possibleMethod != "GET" && possibleMethod != "POST" && possibleMethod != "DELETE") //im sure theres a more graceful way to do this
        return (0);
    obj->method_ = possibleMethod;

    // ----- PARSE PATH ------
    len = raw.find(" ");
    std::string possiblePath = raw.substr(0, len);
    raw = raw.substr(len + 1);
    if (possiblePath[0] != '/') //didnt check if there are more things to validate.
        return (0);
    //maybe at this point i could check if path exists? otherwise we can already return an HTTP error..
    obj->path_ = possiblePath;
    if (obj->path_ == "/")
        obj->path_.append("index.html");

    // ----- PARSE VERSION ---
    len = raw.find("\r\n");
    std::string possibleVersion = raw.substr(0, len);
    raw = raw.substr(len + 2);
    if (possibleVersion.substr(0, 4) != "HTTP") // didnt check if there are more things that i could validate here.
        return (0);
    obj->version_ = possibleVersion;

    return (1);
}

int Request::parseHeaders(std::string &raw, Request *obj)
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
    obj->headers_ = headers;
    return (1);
}

int Request::parseBody(std::string &raw, Request *obj) {
    if (raw.empty())
        return (1); //for now its ok body to be empty, but maybe we should allow it to be empty if its POST.
    std::map<std::string, std::string>::iterator it;
    it = obj->headers_.find("Content-Length");
    if (it == obj->headers_.end())
        return (0);
    std::string lengthStr = obj->headers_["Content-Length"];
    int lenght;
    std::istringstream(lengthStr) >> lenght; //i wonder if we can use this to convert from string to int
    obj->body_= raw;
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

