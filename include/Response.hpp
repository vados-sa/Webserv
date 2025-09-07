#pragma once

#include <string>
#include "HttpMessage.hpp"
#include "Logger.hpp"


class Config;
class Request;
class LocationConfig;

class Response : public HttpMessage
{
private:
    static const int MAX_URI_LENGTH = 8192;

    // HTTP status info
    int statusCode_;
    std::string statusMessage_;

    // File info
    std::string fullPath_;
    std::string filename_;
    std::string contentType_;

    // Config info
    std::map<int, std::string> error_pages_config;

    // methods
    void parseMultipartBody(const Request &obj);
    void generateAutoIndex(const LocationConfig &loc);
    void handleGet(const Request &reqObj, const LocationConfig &loc);
    void handleDelete(const Request &reqObj);
    std::string getContentType(const std::string &path);
    void handleRedirect(const LocationConfig &locConfig);

    // methods -- "POST"
    void handlePost(const Request &reqObj, LocationConfig loc);
    void createUploadDir(const std::string &uploadFullPath);
    void uploadFile(const std::string &uploadFullPath);

    // methods -- general

    // methods --- utils
    void readFileIntoBody(const std::string &fileName);
    std::string generateDefaultPage(const int code, const std::string &message, bool error) const;

public:
    Response();
    Response(std::map<int, std::string> error_pages_config);
    Response(std::map<int, std::string> error_pages_config, int code, const std::string &message, bool error);

    // methods
    std::string writeResponseString() const;
    std::string buildResponse(const Request &reqObj, const LocationConfig &Config);

    // getter
    int getCode() const { return statusCode_; }
    std::string getStatusMessage() const { return statusMessage_; }
    std::string getFullPath() const { return fullPath_; }

    //setter
    void setFullPath(const std::string &reqPath);
    void setCode(const int code);
    void setPage(const  int code, const std::string &message, bool error);
	void handleCgi(const Request &reqObj, const LocationConfig &locConfig);
	void parseCgiResponse(const std::string &cgiOutput);
};

std::ostream &operator<<(std::ostream &out, const Response &obj);
std::string intToString(int value);
