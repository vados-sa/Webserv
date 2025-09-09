#pragma once

#include <string>
#include <map>
#include <iostream>

#include "HttpMessage.hpp"
#include <algorithm>

class Request : public HttpMessage
{
private:
    std::string method_;
    std::string reqPath_;
    std::string fullPath_;
    std::string queryString_;
    int maxBodySize_;
    bool isCgi_;

    bool parseRequestLine(std::string &raw);
    bool parseHeaders(std::string &raw);
    bool parseBody(std::string &raw);
    void parseRequest(const std::string &raw);
    bool parseChunkedBody(std::string &raw);

public :
    Request();
    explicit Request(const std::string &raw, int maxBodySize);

    //---getters
    const std::string getMethod() const { return method_; }
    const std::string getReqPath() const { return reqPath_; }
    const std::string getFullPath() const { return fullPath_; }
    const std::string getVersion() const { return version_; }
    const std::string getQueryString() const { return queryString_; }
    bool isCgi() const { return isCgi_; }

    //---setters
    void setMethod(const std::string &m) { method_ = m; }
    void setReqPath(const std::string &p) { reqPath_ = p; }
    void setFullPath(const std::string &p) { fullPath_ = p; }
    void setQueryString(const std::string &q) { queryString_ = q; }
    void setIsCgi(bool cgi) { isCgi_ = cgi; }
};

std::ostream &operator<<(std::ostream &out, const Request &obj);
std::ostream &operator<<(std::ostream &out, const std::map<std::string, std::string> &map);
