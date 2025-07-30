#include <string>
#include <fstream>
#include <sys/stat.h>
#include "HttpMessage.hpp"
#include "Request.hpp"

using std::string;
using std::cout;
using std::endl;

#define ROOT "./www"

class Response : public HttpMessage
{
private:
    string statusCode_;
    string statusMessage_;
    string fullPath_;
    string filename_;
    string contentType_;

    // methods
    void parseContentType(const Request &obj);

public:
    Response();

    // methods
    void handleGet(const Request &reqObj);
    void handlePost(const Request &reqObj);
    void handleDelete(const Request &reqObj);
    string writeResponseString();
    string getContentType(string path);

    // getter
    string getCode() const { return statusCode_; };
    string getStatusMessage() const { return statusMessage_; };
    string getFullPath() const { return fullPath_; };

    //setter
    void setFullPath(const string &reqPath);
    void setCode(const string code);
    void setPage(const string &code, const string &message, bool error);
};

string buildResponse(const Request &obj);
string generatePage(const string &code, const string &message, bool error);
std::ostream &operator<<(std::ostream &out, const Response &obj);