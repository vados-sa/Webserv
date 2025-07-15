#include "../include/Request.hpp"

Request::Request() {
    method_ = "";
    path_ = "";
    version_ = "";
    body_ = "";
};

Request::Request(const Request &obj) : method_(obj.method_), path_(obj.path_), version_(obj.version_), body_(obj.body_) {}

Request Request::parse(const std::string &raw)
{
    Request helperReqObj;
    std::string temp = raw;

    if (!parseRequestLine(temp, &helperReqObj))
        ;//deu ruim
    if (!parseHeaders(temp, &helperReqObj))
        ;//deu ruim
    if (!parseBody(temp, &helperReqObj))
        ;//deu ruim

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

    // ----- PARSE VERSION ---
    len = raw.find("\n");
    std::string possibleVersion = raw.substr(0, len);
    raw = raw.substr(len + 1);
    if (raw.substr(0, 4) != "HTTP") // didnt check if there are more things that i could validate here.
        return (0);
    obj->version_ = possibleVersion;

    return (1);
}

int Request::parseHeaders(std::string &raw, Request *obj)
{
    std::map<std::string, std::string> headers;
    int len;
    std::string possibleKey;

    while (!raw.compare("\r\n\r\n"))
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
        raw = raw.substr(len + 2);
    }
    obj->headers_ = headers;
    return (1);
}

int Request::parseBody(std::string &raw, Request *obj) {
    std::map<std::string, std::string>::iterator it;

    it = obj->headers_.find("Content-Length");
    if (it == obj->headers_.end())
        return (0);
    std::string lengthStr = obj->headers_["Content-Length"];
    //convert lengthStr to int
    //...
    return (1);
}

std::ostream &operator<<(std::ostream &out, const Request &obj) {
    out << "Method: " << obj.getMethod() << std::endl
        << "Path: " << obj.getPath() << std::endl
        << "Version: " << obj.getVersion() << std::endl
        << "Body: " << obj.getBody() << std::endl;
    //to include headers!
    return (out);
}
