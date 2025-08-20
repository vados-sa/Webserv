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
    std::string reqPath_;
    std::string fullpath_;
    std::string query_string;
    bool is_cgi;

    bool parseRequestLine(std::string &raw);
    bool parseHeaders(std::string &raw);
    bool parseBody(std::string &raw);
public:
    Request();
    Request(const Request &obj);

    //---methods
    static Request parseRequest(const std::string &raw);
    //..

    //---getters
    std::string getMethod() const { return method_; }
    std::string getreqPath() const { return reqPath_; }
    std::string getfullPath() const { return fullpath_; }
    std::string getVersion() const { return version_; }
    std::string getQueryString() const { return query_string; }
    bool getIsCgi() const { return is_cgi; }

    //---setters
    void setMethod(const std::string &methodToSet) { method_ = methodToSet; }
    void setreqPath(const std::string &pathToSet) { reqPath_ = pathToSet; }
    void setfullPath(const std::string &pathToSet) { fullpath_ = pathToSet; }
    void setQueryString(const std::string &set) { query_string = set;}
    void setIsCgi(const bool &set) { is_cgi = set; }
	//std::string parseQueryString() const;
};

std::ostream &operator<<(std::ostream &out, const Request &obj);
std::ostream &operator<<(std::ostream &out, const std::map<std::string, std::string> &map);
std::string normalizePath(const std::string &rawPath);
