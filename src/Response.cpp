#include "Response.hpp"

Response::Response() : fullPath_("./www") {}

void Response::handleGet(const Request &reqObj) {

    (void) reqObj;
    struct stat file_stat;
    if (stat(fullPath_.c_str(), &file_stat) == 0) {
        if (!S_ISREG(file_stat.st_mode)) //if it isnt a regular file
            exit(0);
        if (!(file_stat.st_mode & S_IROTH))
            exit(0);
        std::ifstream file(fullPath_.c_str(), std::ios::in | std::ios::binary); //try to read
        if (!file)
            exit(500);

        std::ostringstream ss;
        ss << file.rdbuf();
        body_ = ss.str();

        setHeader("Content_Length", std::to_string(body_.size()));
        setHeader("Content-Type", "changethislater");
        setCode("200");
        return ;
    }
    else {
        if (errno == ENOENT)
            setCode("404");
        else if (errno == EACCES)
            setCode("403");
        else
            setCode("500");
    }
}

void Response::handlePost(const Request &reqObj)
{
    if (reqObj.getPath() != "/upload/") {
        setCode("404");
        setBody("Wrong path. Expected \"/upload/");
        setHeader("Content-Type", "text/plain"); //hardcoded for now but write something to deal with error later on
        setHeader("Content-Length", "31");
        return;
    }
    if (reqObj.getBody().empty()) {
        setCode("400");
        setBody("No body detected on request. Body necessary");
        setHeader("Content-Type", "text/plain");
        setHeader("Content-Length", "44");
        return;
    }
    const std::string *filename = reqObj.findHeader("Filename");
    if (!filename) {
        setCode("400");
        setBody("Missing Filename header");
        setHeader("Content-Type", "text/plain");
        setHeader("Content-Length", "24");
        return;
    }

    std::ofstream file(filename->c_str());
    if (file.is_open()) {
        file << reqObj.getBody();
        file.close();
        setCode("201");
        setBody("File created");
        setHeader("Content-Type", "text/plain");
        setHeader("Content-Length", "13");
    } else {
        setCode("500");
        setBody("Server error: could not open file for writing.");
        setHeader("Content-Type", "text/plain");
        setHeader("Content-Length", "47");
        return;
    }

    /*Set an appropriate response
        201 Created if the file is newly created
        200 OK if the file was overwritten or updated
        403 Forbidden or 500 Internal Server Error on failure
    */
}

std::string Response::writeResponseString() {
    std::ostringstream res;
    res << version_ << " " << statusCode_ << " " << statusMessage_ << "\r\n"
        << getHeaders() << "\r\n"
        << body_ << "\r\n";
    return (res.str());
}

void Response::setCode(const std::string code)
{
    statusCode_ = code;

    static std::map<std::string, std::string> codeToMessage;
    if (codeToMessage.empty())
    {
        codeToMessage["200"] = "OK";
        codeToMessage["201"] = "Created";
        codeToMessage["204"] = "No Content";
        codeToMessage["400"] = "Bad Request";
        codeToMessage["403"] = "Forbidden";
        codeToMessage["404"] = "Not Found";
        codeToMessage["405"] = "Method Not Allowed";
        codeToMessage["500"] = "Internal Server Error";
    }

    std::map<std::string, std::string>::iterator it = codeToMessage.find(code);
    if (it != codeToMessage.end())
        statusMessage_ = it->second;
    else
        statusMessage_ = "Unknown Status";
}

void Response::setFullPath(const std::string &reqPath) {
    fullPath_.append(reqPath);
}

std::ostream &operator<<(std::ostream &out, const Response &obj)
{
    out << "Version: " << obj.getVersion() << std::endl
        << "Code: " << obj.getCode() << std::endl
        << "Status Message: " << obj.getStatusMessage() << std::endl
        << " ----- " << std::endl
        << "Headers: " << std::endl
        << obj.getHeaders() << std::endl
        << " ----- " << std::endl
        << "Body: " << obj.getBody() << std::endl;
    return (out);
}

std::string buildResponse(const Request &reqObj)
{
    Response res;

    res.setVersion(reqObj.getVersion());
    res.setFullPath(reqObj.getPath());
    if (!reqObj.getMethod().compare("GET"))
        res.handleGet(reqObj);
    if (!reqObj.getMethod().compare("POST"))
        res.handlePost(reqObj);
    // if (!reqObj.getMethod().compare("DELETE"))
    //     res.handleDelete(reqObj);

    std::string reqStr = res.writeResponseString();
    return (reqStr);
}