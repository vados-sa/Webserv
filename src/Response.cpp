#include "Response.hpp"

Response::Response() : fullPath_("./www") {}

Response buildResponse(const Request &reqObj) {
    Response res;

    res.setFullPath(reqObj.getPath());
    if (!reqObj.getMethod().compare("GET"))
        res.handleGet(reqObj);
    if (!reqObj.getMethod().compare("POST"))
        res.handlePost(reqObj);
    // if (!reqObj.getMethod().compare("DELETE"))
    //     res.handleDelete(reqObj);

    // std::string reqStr = writeResponseString(res);
    return (res);
}

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
    if (reqObj.getPath().compare("/upload/"))
        return(setCode("404")); //maybe return here?
    if (reqObj.getBody().empty())
        return (setCode("404"));
    const std::string *filename = reqObj.findHeader("Filename");
    if (!filename) {
        setCode("400");
        setBody("Missing Filename header");
        return;
    }
    std::string filePath = fullPath_;
    filePath.append(*filename);

    struct stat file_stat;
    if (stat(filePath.c_str(), &file_stat) == 0) {

    }

    //build full path with name
    //open the file for writing
    //write the body in the file
    //code 201 created

    /* Validate the request:
        Does the path make sense?
        Is the body not empty?
        Is the filename valid?

    Ensure the directory exists
        If you're saving to ./www/upload/, create it if it doesn't exist.

    Check write permissions
        Can the server write to the destination?
        If the file exists, are you allowed to overwrite it?

    Write the file
        Save the body content to the resolved full path.

    Set an appropriate response
        201 Created if the file is newly created
        200 OK if the file was overwritten or updated
        403 Forbidden or 500 Internal Server Error on failure
*/
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