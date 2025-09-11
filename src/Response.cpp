#include "Response.hpp"
#include "Request.hpp"
#include "LocationConfig.hpp"
#include "CgiHandler.hpp"
#include "Config.hpp"
#include "HttpException.hpp"
#include "Utils.hpp"

#include <dirent.h>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <cstring>
#include <fstream>

static const std::pair<const char *, const char *> mimeArray[] = {
    std::pair<const char *, const char *>(".html", "text/html"),
    std::pair<const char *, const char *>(".htm", "text/html"),
    std::pair<const char *, const char *>(".css", "text/css"),
    std::pair<const char *, const char *>(".js", "application/javascript"),
    std::pair<const char *, const char *>(".png", "image/png"),
    std::pair<const char *, const char *>(".jpg", "image/jpeg"),
    std::pair<const char *, const char *>(".jpeg", "image/jpeg"),
    std::pair<const char *, const char *>(".gif", "image/gif"),
    std::pair<const char *, const char *>(".txt", "text/plain"),
    std::pair<const char *, const char *>(".pdf", "application/pdf"),
    std::pair<const char *, const char *>(".json", "application/json"),
    std::pair<const char *, const char *>(".svg", "image/svg+xml")};

Response::Response() : fullPath_(".") {}

Response::Response(std::map<int, std::string> error_pages)
    : statusCode_(200), statusMessage_("OK"), fullPath_("."), error_pages_config(error_pages) {}

Response::Response(std::map<int, std::string> error_pages, int code, const std::string &message, bool error)
    : statusCode_(200), statusMessage_("OK"), fullPath_("."), error_pages_config(error_pages)
{
    setVersion("HTTP/1.1");
    setPage(code, message, error);
    setHeader("Connection", "close");
}

void Response::handleGet(const LocationConfig &loc) {

    struct stat file_stat;
    if (!util::fileExists(fullPath_, file_stat))
        return;

    if (S_ISDIR(file_stat.st_mode))
    {
        std::vector<std::string> index_files = loc.getIndexFiles();
        for (std::vector<std::string>::const_iterator it = index_files.begin(); it != index_files.end(); ++it)
        {
            std::string candidate = fullPath_ + "/" + *it;
            struct stat s;
            if (::stat(candidate.c_str(), &s) == 0 && S_ISREG(s.st_mode))
            {
                fullPath_ = candidate;
                return handleGet(loc);
            }
        }
        if (loc.getAutoindex())
            return (generateAutoIndex());
        else
            throw HttpException(403, "Directory listing denied.", true);
    }

    if (!S_ISREG(file_stat.st_mode))
        throw HttpException(403, "Requested resource is not a file", true);
    if (!(file_stat.st_mode & S_IROTH))
        throw HttpException(403, "Permission denied", true);

    readFileIntoBody(fullPath_);
    //setHeader(HEADER_CONTENT_LENGTH, util::intToString(body_.size()));
    setHeader(HEADER_CONTENT_TYPE, getContentType(fullPath_));
    return;

}

void Response:: readFileIntoBody(const std::string &fileName) {
    std::ifstream file(fileName.c_str(), std::ios::in | std::ios::binary);
    if (!file) {
        std::ifstream test(fileName.c_str());
        if (!test)
            throw HttpException(404, "Not Found", true);
        else
            throw HttpException(500, "Server error: unable to open file.", true);
        return;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    body_ = ss.str();
}

void Response::generateAutoIndex(void) {
    std::string uri = reqPath_;
    std::string path = fullPath_;

    DIR *dir = opendir(path.c_str());
    if (!dir)
        throw HttpException(403, "Forbidden", true);

    std::vector<std::string> entries;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name(entry->d_name);
        if (name == "." || name == "..")
            continue;

        struct ::stat st;
        std::string fullpath = path + "/" + name;
        if (::stat(fullpath.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
            name += "/";

        entries.push_back(name);
    }
    closedir(dir);
    body_ = util::generateAutoIndexHtml(uri, entries);
}

static std::map<std::string, std::string> initMime()
{
    std::map<std::string, std::string> m;
    size_t len = sizeof(mimeArray) / sizeof(mimeArray[0]);
    for (size_t i = 0; i < len; ++i)
        m[mimeArray[i].first] = mimeArray[i].second;
    return m;
}

std::string Response::getContentType(const std::string &path)
{
    static std::map<std::string, std::string> mime = initMime();

    size_t dot = path.rfind('.');
    if (dot != std::string::npos) {
        std::string ext = path.substr(dot);
        std::map<std::string, std::string>::const_iterator it = mime.find(ext);
        if (it != mime.end())
            return (it->second);
    }
    return ("application/octet-stream");
}

void Response::handlePost(const Request &reqObj, LocationConfig loc)
{
    if (loc.getUploadDir().empty())
        throw std::runtime_error("Config error: No upload_path specified in location " + loc.getUri());

    std::string reqPath = reqObj.getReqPath();
    if (reqPath != "/upload" && reqPath.find("/upload/") != 0)
        throw HttpException(405, "Method Not Allowed", true);

    if (!reqObj.findHeader(HEADER_CONTENT_TYPE))
        throw HttpException(400, "Missing Content-Type header", true);
    else {
        std::string boundary = util::extractBoundary(*reqObj.findHeader(HEADER_CONTENT_TYPE));
        if (boundary.empty())
            throw HttpException (400, "Bad Request", true);
        struct util::MultipartPart mp_struct = util::parseMultipartBody(reqObj.getBody(), boundary);

        std::string uploadFullPath = "." + loc.getUploadDir();
        if (!util::createUploadDir(uploadFullPath))
            throw HttpException(500, "Failed to create upload directory", true);
        std::string safeFilename = util::sanitizeFileName(mp_struct.filename);
        std::string filePath = uploadFullPath + "/" + safeFilename;

        if (!util::saveFile(filePath, mp_struct.content))
            throw HttpException(500, "Failed to save uploaded file", true);
        setPage(201, "File uploaded successfully", false);
    }
}

void Response::uploadFile(const std::string &uploadFullPath)
{
    std::ofstream file((uploadFullPath + "/" + filename_).c_str(), std::ios::binary);
    if (file.is_open()) {
        file.write(body_.c_str(), body_.size());
        file.close();
        return (setPage(201, "File created", false));
    }
    else
        throw HttpException(500, "Server error: could not open file for writing.", true);
}

void Response::handleDelete(const Request &reqObj) {
    std::string prefix = "/upload/"; //talvez criar um vetor e encher com as locations que podem POST

    if (reqObj.getReqPath().compare(0, prefix.size(), prefix) != 0)
        throw HttpException(404, "Wrong path. Expected \"/upload/", true);

    filename_ = "." + reqObj.getFullPath();
    struct stat fileStat;


    if (stat(filename_.c_str(), &fileStat) != 0)
        throw HttpException(404, "File not found: \"" + filename_ + "\"", true);


    if (!S_ISREG(fileStat.st_mode))
        throw HttpException(404, "\"" + filename_ + "\" is not a regular file", true);

    if (remove(filename_.c_str()) != 0)
        throw HttpException(500, "Failed to delete file: \"" + filename_ + "\"", true);

    logs(INFO, "\"" + filename_ + "\" deleted successfully");
    setPage(204, "No content. File \"" + filename_ + "\" deleted successfully.", false);
}

std::string Response::writeResponseString() const
{
    std::ostringstream res;
    std::map<std::string, std::string> headers = getHeaders();
    res << "HTTP/1.1 " << statusCode_ << " " << statusMessage_ << "\r\n";
    for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); ++it)
        res << it->first << ": " << it->second << "\r\n";
    res << "\r\n";
    res << body_;

    return (res.str());
}

void Response::setCode(const int code)
{
    statusCode_ = code;

    static std::map<int, std::string> codeToMessage;
    if (codeToMessage.empty())
    {
        codeToMessage[200] = "OK";
        codeToMessage[201] = "Created";
        codeToMessage[204] = "No Content";
        codeToMessage[301] = "Moved Permanently";
        codeToMessage[302] = "Found";
        codeToMessage[303] = "See Other";
        codeToMessage[307] = "Temporary Redirect";
        codeToMessage[308] = "Permanent Redirect";
        codeToMessage[400] = "Bad Request";
        codeToMessage[403] = "Forbidden";
        codeToMessage[404] = "Not Found";
        codeToMessage[408] = "Request Timeout";
        codeToMessage[405] = "Method Not Allowed";
        codeToMessage[411] = "Length Required";
        codeToMessage[414] = "URI too long";
        codeToMessage[500] = "Internal Server Error";
    }

    statusMessage_ = codeToMessage[code];
    if (statusMessage_.empty())
        statusMessage_ = "Unknown Status";

}

void Response::setPage(const int code, const std::string &message, bool error)
{
    setCode(code);

    std::string errorPagePath = error_pages_config[code];
    if (errorPagePath.empty())
        body_ = generateDefaultPage(code, message, error);
    else
        readFileIntoBody("." + errorPagePath);

    setHeader(HEADER_CONTENT_LENGTH, util::intToString(body_.size()));
    setHeader(HEADER_CONTENT_TYPE, MIME_HTML);
}

std::string Response::generateDefaultPage(int code, const std::string &message, bool error) const
{
    std::ostringstream content;
    content << "<h1>" << (error ? "Error " : "Status ") << code << "</h1>\n"
            << "<p>" << message << "</p>";
    return util::wrapHtml(util::intToString(code) + " " + message, content.str());
}

void Response::setFullPath(const std::string &reqPath) {
    fullPath_.append(reqPath);
}

// std::ostream &operator<<(std::ostream &out, const Response &obj)
// {
//     out << "Version: " << obj.getVersion() << std::endl
//         << "Code: " << obj.getCode() << std::endl
//         << "Status Message: " << obj.getStatusMessage() << std::endl
//         << " ----- " << std::endl
//         << "Headers: " << std::endl
//         << obj.getHeaders() << std::endl
//         << " ----- " << std::endl
//         << "Body: " << obj.getBody() << std::endl;
//     return (out);
// }

std::string Response::buildResponse(const Request &reqObj, const LocationConfig &locConfig)
{
    this->setVersion(reqObj.getVersion());
    this->setFullPath(reqObj.getFullPath());
    this->setReqPath(reqObj.getReqPath());

    if (reqObj.getReqPath().size() > MAX_URI_LENGTH) {
        this->setPage(414, "URI Too Long", true);
        return (this->writeResponseString());
    }

    if (!locConfig.isMethodAllowed(reqObj.getMethod())) {
        this->setPage(405, "Method not allowed", true);
        std::vector<std::string> allowed = locConfig.getAllowedMethods();
        std::string allowHeader;
        for (size_t i = 0; i < allowed.size(); i++) {
            allowHeader += allowed[i];
            if (i != allowed.size() - 1)
                allowHeader += ", ";
        }
        this->setHeader("Allow", allowHeader);
        return (this->writeResponseString());
    }

    if (!locConfig.getReturnTarget().empty()) {
        handleRedirect(locConfig);
    } else if (!reqObj.getMethod().compare("GET")) {
        handleGet(locConfig);
    } else if (!reqObj.getMethod().compare("POST")) {
        handlePost(reqObj, locConfig);
    } else if (!reqObj.getMethod().compare("DELETE")) {
        handleDelete(reqObj);
    } else
        throw HttpException(501, "Method not implemented", true);

    const std::string version = reqObj.getVersion();
    std::string connection;
    if (reqObj.findHeader(HEADER_CONNECTION)) {
        connection = *reqObj.findHeader(HEADER_CONNECTION);
        for (size_t i = 0; i < connection.size(); ++i)
            connection[i] = (char)std::tolower(connection[i]);
    }

    bool want_close;
    if (version == "HTTP/1.1")
        want_close = (connection == "close"); // default is keep-alive
    else // HTTP/1.0
        want_close = (connection != "keep-alive"); // default is close

    setHeader("Connection", want_close ? "close" : "keep-alive");
    setHeader(HEADER_CONTENT_LENGTH, util::intToString(body_.size()));
    return (writeResponseString());
}

void Response::parseCgiResponse(const std::string &cgiOutput) {
    std::istringstream stream(cgiOutput);
    std::string line;
    bool headersDone = false;
    std::ostringstream body;
    while (std::getline(stream, line)) {
        // Remove trailing \r if present
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        if (!headersDone) {
            if (line.empty()) {
                headersDone = true;
                continue;
            }
            size_t colon = line.find(":");
            if (colon != std::string::npos) {
                std::string key = line.substr(0, colon);
                std::string value = line.substr(colon + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                setHeader(key, value);
            }
        } else {
            body << line << "\n";
        }
    }
    std::string b = body.str();
    if (!b.empty() && b[b.size()-1] == '\n') b.erase(b.size()-1);

    setBody(b);

    if (!findHeader("Content-Length")) {
        setHeader("Content-Length", util::intToString(b.size()));
    }
}

void Response::handleRedirect(const LocationConfig &locConfig)
{
    std::string newLocation = locConfig.getReturnTarget();
    int statusCode = locConfig.getReturnStatus();

    setCode(statusCode);
    setHeader("Location", newLocation);

    body_ =
        "<html><head><title>" + util::intToString(statusCode) + " " + statusMessage_ + "</title></head>"
        "<body style='font-family:sans-serif;text-align:center;margin-top:100px;'>"
        "<h1>" + util::intToString(statusCode) + " " + statusMessage_ + "</h1>"
        "<p>Resource has moved to <a href=\"" +
        newLocation + "\">" + newLocation + "</a>.</p></body></html>";
}
