#include <string>
#include <sys/stat.h>
#include "Request.hpp"
#include <fstream>
#include "HttpMessage.hpp"

#define ROOT "./www"

class Response : public HttpMessage
{
private:
    std::string statusCode_;
    std::string statusMessage_;
    std::string fullPath_;

public:
    Response();
    // ~Response();

    //methods
    void handleGet(const Request &reqObj);
    void handlePost(const Request &reqObj);
    // void handleDelete(const Request &reqObj);

    //getter
    std::string getCode() const { return statusCode_; };
    std::string getStatusMessage() const { return statusMessage_; };
    std::string getFullPath() const { return fullPath_; };

    //setter
    void setFullPath(const std::string &reqPath);
    void setCode(const std::string code);
};


Response buildResponse(const Request &obj);

std::ostream &operator<<(std::ostream &out, const Response &obj);