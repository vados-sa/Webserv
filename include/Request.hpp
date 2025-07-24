#pragma once

#include <string>
using std::string;

#include <map>
using std::map;

#include <iostream>
using std::cout;
using std::endl;

#include <sstream>
using std::ostream;

#include "HttpMessage.hpp"

class Request : public HttpMessage
{
private:
    string method_;
    string path_;

public:
    Request();
    Request(const Request &obj);

    //---methods
    static Request parseRequest(const string &raw);
    static int parseRequestLine(string &raw, Request *obj);
    static int parseHeaders(string &raw, Request *obj);
    static int parseBody(string &raw, Request *obj);
    //..

    //---getters
    string getMethod() const { return method_; };
    string getPath() const { return path_; };
    string getVersion() const { return version_; };

    //---setters
    void setMethod(const string &methodToSet) { method_ = methodToSet; };
    void setPath(const string &pathToSet) { path_ = pathToSet; };
};

ostream &operator<<(ostream &out, const Request &obj);
ostream &operator<<(ostream &out, const std::map<string, string> &map);
int strToInt(string &str);