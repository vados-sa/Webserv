#include "Response.hpp"
#include "LocationConfig.hpp"
#include "Config.hpp"
#include <dirent.h>
#include <string>
#include <iostream>
#include <sstream>

Response::Response() : statusCode_(""), statusMessage_(""), fullPath_("."), filename_("upload/") {}

template <typename T>
std::string int_to_string(T value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

void Response::handleGet(const Request &reqObj, const LocationConfig &loc) {

    (void) reqObj;
    struct stat file_stat;

    std::vector<std::string> index_files = loc.getIndexFiles();

    if (stat(fullPath_.c_str(), &file_stat) == 0) {
        if (S_ISDIR(file_stat.st_mode)) {
            for (size_t i = 0; i < index_files.size(); ++i) {
                std::string candidate = fullPath_ + "/" + index_files[i];
                if (stat(candidate.c_str(), &file_stat) == 0 && S_ISREG(file_stat.st_mode))
                {
                    fullPath_ = candidate;
                    return handleGet(reqObj, loc);
                }
            }
            if (loc.getAutoindex()) {
                generateAutoIndex(*this, loc);
                return;
            } else
                return (setPage("403", "Directory listing denied.", true));
        }

        if (!S_ISREG(file_stat.st_mode))
            return (setPage("403", "Requested resource is not a file", true));
    }

    if (stat(fullPath_.c_str(), &file_stat) == 0) {
        if (!S_ISREG(file_stat.st_mode))
            return (setPage("403", "Requested resource is not a file", true));
        if (!(file_stat.st_mode & S_IROTH))
            return (setPage("403", "You do not have permission to read this file", true));
        std::ifstream file(fullPath_.c_str(), std::ios::in | std::ios::binary);
        if (!file)
            return (setPage("500", "Server error: unable to open file.", true));

        std::ostringstream ss;
        ss << file.rdbuf();
        body_ = ss.str();

        setHeader("Content-Length", int_to_string(body_.size()));
        setHeader("Content-Type", getContentType(fullPath_));
        statusCode_ = "200";
        return;
    }
    else
    {
        if (errno == ENOENT)
            return (setPage("404", "File does not exist", true));
        else if (errno == EACCES)
            return (setPage("403", "Access denied.", true));
        else
            return (setPage("500", "Internal server error while accessing file.", true));
    }
}

std::string Response::generateAutoIndex(Response &res, LocationConfig loc) {
    std::string uri = loc.getUri();
    std::string path = res.getFullPath();

    DIR *dir = opendir(path.c_str());
    if (!dir){
        res.setPage("403", "Forbidden", true);
        return ("");
    }

    std::ostringstream html;

    html << "<!DOCTYPE html>\n"
            << "<html>\n<head>\n"
            << "<title>Index of " << uri << "</title>\n"
            << "</head>\n<body>\n"
            << "<h1>Index of " << uri << "</h1>\n"
            << "<ul>\n";

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name(entry->d_name);
        if (name == "." || name == "..")
            continue;
        struct stat st;
        std::string fullpath = path + "/" + name;
        if (stat(fullpath.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
            name += "/";
        html << "<li><a href=\"" << uri;
        if (uri[uri.size() - 1] != '/')
            html << "/";
        html << name << "\">" << name << "</a></li>\n";
    }

    html << "</ul>\n</body>\n</html>\n";
    res.setPage("200", "OK", false);
    closedir(dir);
    res.setBody(html.str());
    return (html.str());
}

std::string Response::getContentType(std::string path)
{
    size_t dot = path.rfind('.');
    if (dot != std::string::npos)
    {
        std::string ext = path.substr(dot);

        if (ext == ".html" || ext == ".htm")
            return "text/html";
        if (ext == ".css")
            return "text/css";
        if (ext == ".js")
            return "application/javascript";
        if (ext == ".png")
            return "image/png";
        if (ext == ".jpg" || ext == ".jpeg")
            return "image/jpeg";
        if (ext == ".gif")
            return "image/gif";
        if (ext == ".txt")
            return "text/plain";
        if (ext == ".pdf")
            return "application/pdf";
        if (ext == ".json")
            return "application/json";
        if (ext == ".svg")
            return "image/svg+xml";
    }
    return "application/octet-stream";
}

void Response::handlePost(const Request &reqObj)
{
    filename_ = "www/upload/";
    if (reqObj.getreqPath() != "/upload/") {
        setBody(generatePage("404", "Wrong path. Expected \"/upload/", true));
        return;
    }
    if (reqObj.getBody().empty()) {
        return (setPage("400", "No body detected on request. Body necessary", true));
    }
    std::cout << getHeaders() << std::endl;
    if (!reqObj.findHeader("Content-Type"))
    {
        setPage("400", "Missing Content-Type header", true);
        return;
    }
    parseContentType(reqObj);
    mkdir("www/upload", 0755);
    std::ofstream file(filename_.c_str(), std::ios::binary);
    if (file.is_open()) {
        file.write(body_.c_str(), body_.size());
        file.close();
        return (setPage("201", "File created", false));
    } else
        return (setPage("500", "Server error: could not open file for writing.", true));
}

void Response::handleDelete(const Request &reqObj) {
    if (reqObj.getreqPath().substr(0, 8) != "/upload/")
    {
        setPage("404", "Wrong path. Expected \"/upload/", true);
        return;
    }
    filename_ = "www/upload/";
    filename_ = filename_.append(reqObj.getreqPath().substr(8));

    struct stat fileStat;

    if (stat(filename_.c_str(), &fileStat) != 0) {
        setPage("404", "\"" + filename_ + "\" is not a regular file", true);
        return;
    };

    if(!S_ISREG(fileStat.st_mode)) {
        setPage("404", "File not found :\"" + filename_ + "\"", true);
        return;
    }

    if (remove(filename_.c_str()) != 0) {
        setPage("500", "Failed to delete file: \"" + filename_ + "\"", true);
        return;
    }
    setPage("200", "File \"" + filename_ + "\" deleted successfully.", true);
}

void Response::parseContentType(const Request &obj)
{

    // ------ GET BOUNDARY -----

    std::string rawValue = *obj.findHeader("Content-Type");
    size_t pos = rawValue.find("boundary=");
    std::string boundary = "--";
    if (pos != std::string::npos)
        boundary.append(rawValue.substr(pos + 9));    //should i check if it comes wrapped in quotes?

    // ----- EXTRACT BOUNDARY FROM BODY

    std::string rawBody = obj.getBody();
    size_t start = rawBody.find(boundary);
    if (start != std::string::npos)
        rawBody = rawBody.substr(start + boundary.length() + 2);

    boundary.append("--");
    size_t end = rawBody.rfind(boundary);
    if (end != std::string::npos)
        rawBody = rawBody.substr(0, end - 2);

    // ----- EXTRACT HEADERS FROM THE BODY
    std::string headers;
    std::string fileContent;
    size_t headerEnd = rawBody.find("\r\n\r\n");
    if (headerEnd != std::string::npos)
    {
        headers = rawBody.substr(0, headerEnd);
        fileContent = rawBody.substr(headerEnd + 4);
    }

    // ---- EXTRACT FILENAME
    //string filename = "www/upload/";
    pos = headers.find("filename=\"");
    if (pos != std::string::npos) {
        size_t start = pos + 10;
        size_t end = headers.find("\"", start);
        filename_ = filename_.append(headers.substr(start, end - start));
        std::cout << filename_ << std::endl; // pode apagar dps
    }

    // ---- EXTRACT CONTENT-TYPE
    pos = headers.find("Content-Type: ");
    if (pos != std::string::npos) {
        start = pos + 14;
        end = headers.find("\r\n", start);
        contentType_ = headers.substr(start, end - start);
        setHeader("Content-Type", contentType_);
    }
    body_ = fileContent;
}

std::string Response::writeResponseString()
{
    std::ostringstream res;
    res << version_ << " " << statusCode_ << " " << statusMessage_ << "\r\n"
        << getHeaders() << "\r\n"
        << body_;
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

void Response::setPage(const std::string &code, const std::string &message, bool error)
{
    setCode(code);
    body_ = generatePage(code, message, error);
    setHeader("Content-Length", int_to_string(body_.size()));
    setHeader("Content-Type", "text/html");
}

std::string Response::generateDefaultPage(const int code, const std::string &message, bool error)
{
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html><head><title>" << code << " " << message << "</title></head>"
         << "<body style='font-family:sans-serif;text-align:center;margin-top:100px;'>";
    if (error)
        html << "<h1>Error ";
    else
        html << "<h1>Status ";
    html << code << "</h1><p>" << message << "</p></body></html>";
    return html.str();
}

void Response::setFullPath(const std::string &reqPath)
{
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

std::string Response::buildResponse(const Request &reqObj, const LocationConfig &locConfig)
{
    Response res;

    res.setVersion(reqObj.getVersion());
    res.setFullPath(locConfig.getRoot() + reqObj.getreqPath());

    // Check if the request is a CGI request
    //std::string reqPath = reqObj.getPath();
    // if (locConfig.isCgiRequest(reqPath)) {
    //     res.handleCgi(reqObj, locConfig);
    if (reqObj.getIsCgi()) {
        res.handleCgi(reqObj, locConfig);
    } else if (!reqObj.getMethod().compare("GET")) {
        res.handleGet(reqObj, locConfig);
    } else if (!reqObj.getMethod().compare("POST")) {
        res.handlePost(reqObj);
    } else if (!reqObj.getMethod().compare("DELETE")) {
        res.handleDelete(reqObj);
    } else {
        res.setPage("405", "Method not allowed", true);
        res.setHeader("Allow", "GET, POST, DELETE");
    }

    if (reqObj.findHeader("Connection"))
        res.setHeader("Connection", *reqObj.findHeader("Connection"));

    std::string reqStr = res.writeResponseString();
    return reqStr;
}

void Response::handleCgi(const Request &reqObj, const LocationConfig &locConfig)
{
    std::string cgiScriptPath = "." + locConfig.getRoot() + reqObj.getreqPath();

    // Set up environment variables for the CGI script
    setenv("REQUEST_METHOD", reqObj.getMethod().c_str(), 1);
    setenv("SCRIPT_FILENAME", cgiScriptPath.c_str(), 1);
    setenv("QUERY_STRING", reqObj.getQueryString().c_str(), 1);

    // Execute the CGI script and capture its output
    FILE *pipe = popen(cgiScriptPath.c_str(), "r");
    if (!pipe) {
        setPage("500", "Failed to execute CGI script.", true);
        return;
    }

    std::ostringstream output;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output << buffer;
    }
    pclose(pipe);

    // Set the response body and headers
    body_ = output.str();
    setHeader("Content-Length", int_to_string(body_.size()));
    setHeader("Content-Type", "text/html");
    setCode("200");
}