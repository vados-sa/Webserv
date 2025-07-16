#pragma once

#include <string>
#include <map>
#include <iostream>
#include <sstream>

class Request
{
private:
    std::string method_;
    std::string path_;
    std::string version_;
    std::map<std::string, std::string> headers_;
    std::string body_;

public:
    Request();
    Request(const Request &obj);
    Request &operator=(const Request &obj);
    //~Request();

    //---methods
    static Request parse(const std::string &raw);
    static int parseRequestLine(std::string &raw, Request *obj); // ex: GET /index.html HTTP/1.1
    static int parseHeaders(std::string &raw, Request *obj);
    static int parseBody(std::string &raw, Request *obj);
    //..

    //---getters
    std::string getMethod() const { return method_; };
    std::string getPath() const { return path_; };
    std::string getVersion() const { return version_; };
    std::map<std::string, std::string> getHeaders() const { return headers_; };
    std::string getBody() const { return body_; };

    //---setters
    void setMethod(const std::string &methodToSet) { method_ = methodToSet; };
    void setPath(const std::string &pathToSet) { path_ = pathToSet; };
    void setVersion(const std::string &versionToSet) { version_ = versionToSet; };
    void setBody(const std::string &bodyToSet) { body_ = bodyToSet; };
};

std::ostream &operator<<(std::ostream &out, const Request &obj);
std::ostream &operator<<(std::ostream &out, const std::map<std::string, std::string> &map);
int strToInt(std::string &str);