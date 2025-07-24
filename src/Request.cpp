#include "Request.hpp"

Request::Request() : method_(""), path_("") {
};

Request::Request(const Request &obj) : method_(obj.method_), path_(obj.path_) {}

Request Request::parseRequest(const string &raw)
{
    Request helperReqObj;
    string temp = raw;

    if (!parseRequestLine(temp, &helperReqObj)) {
        std::cerr << "Failed to parse request line" << endl; // we can throw exceptions later, rn i dont want the program to quit
        return (helperReqObj);
    }
    if (!parseHeaders(temp, &helperReqObj)){
        std::cerr << "Failed to parse headers" << endl;
        return (helperReqObj);
    }
    if (!parseBody(temp, &helperReqObj)){
        std::cerr << "Failed to parse body" << endl;
        return (helperReqObj);
    }

    return (helperReqObj);
}

int Request::parseRequestLine(string &raw, Request *obj) {

    // ----- PARSE METHOD -----
    int len = raw.find(" ");
    string possibleMethod = raw.substr(0, len);
    raw = raw.substr(len + 1);
    if (possibleMethod != "GET" && possibleMethod != "POST" && possibleMethod != "DELETE") //im sure theres a more graceful way to do this
        return (0);
    obj->method_ = possibleMethod;

    // ----- PARSE PATH ------
    len = raw.find(" ");
    string possiblePath = raw.substr(0, len);
    raw = raw.substr(len + 1);
    if (possiblePath[0] != '/')
        return (0);
    obj->path_ = possiblePath;
    if (obj->path_ == "/")
        obj->path_.append("index.html");

    // ----- PARSE VERSION ---
    len = raw.find("\r\n");
    string possibleVersion = raw.substr(0, len);
    raw = raw.substr(len + 2);
    if (possibleVersion.substr(0, 4) != "HTTP")
        return (0);
    obj->version_ = possibleVersion;

    return (1);
}

int Request::parseHeaders(string &raw, Request *obj)
{
    map<string, string> headers;
    int len;
    string possibleKey;

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
        string possibleValue = raw.substr(0, len);
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

int Request::parseBody(string &raw, Request *obj) {
    if (raw.empty())
        return (1); //for now its ok body to be empty, but maybe we should allow it to be empty if its POST.
    map<string, string>::iterator it;
    it = obj->headers_.find("Content-Length");
    if (it == obj->headers_.end())
        return (0);
    string lengthStr = obj->headers_["Content-Length"];
    int lenght;
    std::istringstream(lengthStr) >> lenght; //i wonder if we can use this to convert from string to int
    obj->body_= raw;
    return (1);
}


std::ostream &operator<<(std::ostream &out, const Request &obj) {
    out << "Method: " << obj.getMethod() << endl
        << "Path: " << obj.getPath() << endl
        << "Version: " << obj.getVersion() << endl
        << " ----- " << endl
        << "Headers: " << endl
        << obj.getHeaders() << endl
        << " ----- " << endl
        << "Body: " << obj.getBody() << endl;
    return (out);
}

std::ostream &operator<<(std::ostream &os, const map<string, string> &map)
{
    for (std::map<string, string>::const_iterator it = map.begin(); it != map.end(); ++it)
        os << it->first << ": " << it->second << '\n';
    return os;
}

