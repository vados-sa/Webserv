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
    std::string reqPath_;
    std::string filename_;
    std::string contentType_;
    std::map<int, std::string> error_pages_config;

    void generateAutoIndex(void);
    void handleGet(const LocationConfig &loc);
    void handleDelete(const Request &reqObj);
    std::string getContentType(const std::string &path);
    void handleRedirect(const LocationConfig &locConfig);

    void handlePost(const Request &reqObj, LocationConfig loc);
    void uploadFile(const std::string &uploadFullPath);
    void readFileIntoBody(const std::string &fileName);
    std::string generateDefaultPage(const int code, const std::string &message, bool error) const;


public:
    Response();
    Response(std::map<int, std::string> error_pages_config);
    Response(std::map<int, std::string> error_pages_config, int code, const std::string &message, bool error);

    std::string writeResponseString() const;
    std::string buildResponse(const Request &reqObj, const LocationConfig &Config);

    int getCode() const { return statusCode_; }
    const std::string getStatusMessage() const { return statusMessage_; }
    const std::string getFullPath() const { return fullPath_; }
    const std::string getReqPath() const { return reqPath_; }

    void setFullPath(const std::string &fullPath);
    void setReqPath(const std::string &reqPath) { reqPath_ = reqPath; };
    void setCode(const int code);
    void setPage(const int code, const std::string &message, bool error);

    void parseCgiResponse(const std::string &cgiOutput);
};