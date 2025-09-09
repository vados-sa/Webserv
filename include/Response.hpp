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

    int statusCode_;
    std::string statusMessage_;
    std::string fullPath_;
    std::string filename_;
    std::string contentType_;
    std::map<int, std::string> error_pages_config;

    void parseMultipartBody(const Request &obj);
    void generateAutoIndex(const LocationConfig &loc);
    void handleGet(const Request &reqObj, const LocationConfig &loc);
    void handleDelete(const Request &reqObj);
    std::string getContentType(const std::string &path);
    void handleRedirect(const LocationConfig &locConfig);

    void handlePost(const Request &reqObj, LocationConfig loc);
    void createUploadDir(const std::string &uploadFullPath);
    void uploadFile(const std::string &uploadFullPath);

    void readFileIntoBody(const std::string &fileName);
    std::string generateDefaultPage(const int code, const std::string &message, bool error) const;

    void handleCgi(const Request &reqObj, const LocationConfig &locConfig);
    void parseCgiResponse(const std::string &cgiOutput);

public:
    Response();
    Response(std::map<int, std::string> error_pages_config);
    Response(std::map<int, std::string> error_pages_config, int code, const std::string &message, bool error);

    std::string writeResponseString() const;
    std::string buildResponse(const Request &reqObj, const LocationConfig &Config);

    int getCode() const { return statusCode_; }
    const std::string getStatusMessage() const { return statusMessage_; }
    const std::string getFullPath() const { return fullPath_; }

    void setFullPath(const std::string &reqPath);
    void setCode(const int code);
    void setPage(const int code, const std::string &message, bool error);
};

std::ostream &operator<<(std::ostream &out, const Response &obj);
