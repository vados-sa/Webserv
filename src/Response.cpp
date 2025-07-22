#include "Response.hpp"

Response::Response() : fullPath_("./www"), statusCode_(""), statusMessage_("") {}

void Response::handleGet(const Request &reqObj) {

    (void) reqObj;
    struct stat file_stat;
    if (stat(fullPath_.c_str(), &file_stat) == 0) {
        if (!S_ISREG(file_stat.st_mode))
            return (setError("403", "Requested resource is not a file"));
        if (!(file_stat.st_mode & S_IROTH))
            return (setError("403", "You do not have permission to read this file"));
        std::ifstream file(fullPath_.c_str(), std::ios::in | std::ios::binary);
        if (!file)
            return (setError("500", "Server error: unable to open file."));

        std::ostringstream ss;
        ss << file.rdbuf();
        body_ = ss.str();

        setHeader("Content_Length", std::to_string(body_.size()));
        setHeader("Content-Type", "text/html");
        statusCode_ = "200";
        return;
    }
    else {
        if (errno == ENOENT)
            return (setError("404", "File does not exist"));
        else if (errno == EACCES)
            return (setError("403", "Access denied."));
        else
            return (setError("500", "Internal server error while accessing file."));
    }
}

void Response::handlePost(const Request &reqObj)
{
    if (reqObj.getPath() != "/upload/") {
        setBody(generateErrorPage("404", "Wrong path. Expected \"/upload/"));
        return;
    }
    if (reqObj.getBody().empty()) {
        return (setError("400", "No body detected on request. Body necessary"));
    }
    const std::string *filename = reqObj.findHeader("Filename");
    if (!filename)
        return (setError("400", "Missing Filename header"));

    std::ofstream file(filename->c_str());
    if (file.is_open()) {
        file << reqObj.getBody();
        file.close();
        return (setError("201", "File created")); //is this correct?
    } else
        return (setError("500", "Server error: could not open file for writing."));
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

void Response::setError(const std::string &code, const std::string &message) {
    setCode(code);
    body_ = generateErrorPage(code, message);
    setHeader("Content_Length", std::to_string(body_.size()));
    setHeader("Content-Type", "text/html");
}

std::string generateErrorPage(const std::string &code, const std::string &message)
{
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html><head><title>" << code << " " << message << "</title></head>"
         << "<body style='font-family:sans-serif;text-align:center;margin-top:100px;'>"
         << "<h1>Error " << code << "</h1><p>" << message << "</p></body></html>";
    return html.str();
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

