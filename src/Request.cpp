#include "../include/Request.hpp"

Request::Request() {
    _method = "";
    _path = "";
    _version = "";
    _body = "";
};

Request::Request(const Request &obj) : _method(obj._method), _path(obj._path), _version(obj._version), _body(obj._body) {}

Request *Request::parse(const std::string &raw)
{
    std::string methodToSet;
    std::string pathToSet;
    std::string versionToSet;
    std::string bodyToSet;
    Request helperReqObj;

    if (!parseRequestLine(raw, &helperReqObj))
        ;//deu ruim
    if (!parseHeaders(raw, &helperReqObj))
        ;//deu ruim
    if (!parseBody(raw, &helperReqObj))
        ;//deu ruim

    Request *reqObj = new Request(helperReqObj);
    return (reqObj);
}

int Request::parseRequestLine(const std::string &raw, Request *obj) {
    std::string temp = raw;

    // ----- PARSE METHOD -----
    int len = temp.find(" ");
    std::string possibleMethod = temp.substr(0, len);
    temp = temp.substr(len + 1);
    if (possibleMethod != "GET" && possibleMethod != "POST" && possibleMethod != "DELETE") //im sure theres a more graceful way to do this
        return (0);
    obj->_method = possibleMethod;

    // ----- PARSE PATH ------
    len = temp.find(" ");
    std::string possiblePath = temp.substr(0, len);
    temp = temp.substr(len + 1);
    if (possiblePath[0] != '/') //didnt check if there are more things to validate.
        return (0);
    //maybe at this point i could check if path exists? otherwise we can already return an HTTP error..
    obj->_path = possiblePath;

    // ----- PARSE VERSION ---
    len = temp.find("\n");
    std::string possibleVersion = temp.substr(0, len);
    temp = temp.substr(len + 1);
    if (temp.substr(0, 4) != "HTTP") // didnt check if there are more things that i could validate here.
        return (0);
    obj->_path = possibleVersion;

    return (1);
}

int Request::parseHeaders(const std::string &raw, Request *obj)
{
    std::map<std::string, std::string> headers;
    std::string temp = raw;
    int len;
    std::string possibleKey;

    while (!temp.compare("\r\n"))
    {
        len = temp.find(":");
        if (len < 0)
            return (0);
        possibleKey = temp.substr(0, len);
        temp = temp.substr(len + 2); // considering ": ", im not considering multiple whitespaces yet
        len = temp.find("\r\n");
        if (len < 0)
            return (0);
        std::string possibleValue = temp.substr(0, len);
        // insert a check if possibleValue is empty?
        headers[possibleKey] = possibleValue;
        temp = temp.substr(len + 2);
    }
    obj->_headers = headers;
    return (1);
}
