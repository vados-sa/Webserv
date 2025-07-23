#pragma once

#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include "HttpMessage.hpp"

class Request : public HttpMessage
{
private:
    std::string method_;
    std::string path_;

public:
    Request();
    Request(const Request &obj);

    //---methods
    static Request parseRequest(const std::string &raw);
    static int parseRequestLine(std::string &raw, Request *obj);
    static int parseHeaders(std::string &raw, Request *obj);
    static int parseBody(std::string &raw, Request *obj);
    //..

    //---getters
    std::string getMethod() const { return method_; };
    std::string getPath() const { return path_; };
    std::string getVersion() const { return version_; };

    //---setters
    void setMethod(const std::string &methodToSet) { method_ = methodToSet; };
    void setPath(const std::string &pathToSet) { path_ = pathToSet; };
};

std::ostream &operator<<(std::ostream &out, const Request &obj);
std::ostream &operator<<(std::ostream &out, const std::map<std::string, std::string> &map);
int strToInt(std::string &str);