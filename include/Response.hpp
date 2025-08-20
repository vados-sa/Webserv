#pragma once

#include <string>
#include <fstream>
#include <sys/stat.h>
#include <sstream>
#include <dirent.h>
#include "HttpMessage.hpp"
#include "Request.hpp"
#include "LocationConfig.hpp"
#include "Config.hpp"

#define ROOT "./www"

class Config;

class Response : public HttpMessage
{
private:
    std::string statusCode_;
    std::string statusMessage_;
    std::string fullPath_;
    std::string filename_;
    std::string contentType_;

    // methods
    void parseContentType(const Request &obj);

public:
    Response();

    // methods
    void handleGet(const Request &reqObj, const LocationConfig &loc);
    void handlePost(const Request &reqObj);
    void handleDelete(const Request &reqObj);
    std::string writeResponseString();
    std::string getContentType(std::string path);

    // getter
    std::string getCode() const { return statusCode_; };
    std::string getStatusMessage() const { return statusMessage_; };
    std::string getFullPath() const { return fullPath_; };

    //setter
    void setFullPath(const std::string &reqPath);
    void setCode(const std::string code);
    void setPage(const std::string &code, const std::string &message, bool error);
	void handleCgi(const Request &reqObj, const LocationConfig &locConfig);
};

std::string buildResponse(const Request &reqObj, const LocationConfig& Config);
std::string generatePage(const std::string &code, const std::string &message, bool error);
std::string generateAutoIndex(Response &res, LocationConfig loc);
std::ostream &operator<<(std::ostream &out, const Response &obj);
