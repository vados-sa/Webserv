#include <string>
#include <fstream>
#include <sys/stat.h>

#include "HttpMessage.hpp"
#include "Request.hpp"

#define ROOT "./www"

class Response : public HttpMessage
{
private:
    std::string statusCode_;
    std::string statusMessage_;
    std::string fullPath_;

public:
    Response();

    //methods
    void handleGet(const Request &reqObj);
    void handlePost(const Request &reqObj);
    // void handleDelete(const Request &reqObj);
    std::string writeResponseString();

    //getter
    std::string getCode() const { return statusCode_; };
    std::string getStatusMessage() const { return statusMessage_; };
    std::string getFullPath() const { return fullPath_; };

    //setter
    void setFullPath(const std::string &reqPath);
    void setCode(const std::string code);
    void setError(const std::string &code, const std::string &message);
};

std::string buildResponse(const Request &obj);
std::string generateErrorPage(const std::string &code, const std::string &message);
std::ostream &operator<<(std::ostream &out, const Response &obj);