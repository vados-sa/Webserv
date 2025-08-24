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

#define MAX_URI_LENGTH 8192

class Config;

class Response : public HttpMessage
{
private:
    int statusCode_;
    std::string statusMessage_;
    std::string fullPath_;
    std::string filename_;
    std::string contentType_;
    std::map<int, std::string> error_pages_config;

    // methods
    void parseMultipartBody(const Request &obj);
    void generateAutoIndex(const LocationConfig &loc);
    void handleGet(const Request &reqObj, const LocationConfig &loc);
    void handleDelete(const Request &reqObj);
    std::string getContentType(std::string path);

    // methods -- "POST"
    void handlePost(const Request &reqObj, LocationConfig loc);
    void createUploadDir(const std::string &uploadFullPath);
    void uploadFile(const std::string &uploadFullPath);

    // methods -- general
    std::string writeResponseString();

    // methods --- utils
    void readFileIntoBody(const std::string &fileName);
    std::string generateDefaultPage(const int code, const std::string &message, bool error);

public:
    Response();
    Response(std::map<int, std::string> error_pages_config);

    // methods
    std::string buildResponse(const Request &reqObj, const LocationConfig &Config);

    // getter
    int getCode() const { return statusCode_; };
    std::string getStatusMessage() const { return statusMessage_; };
    std::string getFullPath() const { return fullPath_; };

    //setter
    void setFullPath(const std::string &reqPath);
    void setCode(const int code);
    void setPage(const  int code, const std::string &message, bool error);
	void handleCgi(const Request &reqObj, const LocationConfig &locConfig);
};

std::ostream &operator<<(std::ostream &out, const Response &obj);
