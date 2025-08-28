#include "Response.hpp"
#include "Request.hpp"
#include "LocationConfig.hpp"
#include "CgiHandler.hpp"
#include "Config.hpp"
#include <dirent.h>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <cstring>
#include <fstream>

static const char *HEADER_CONTENT_TYPE = "content-type";
static const char *HEADER_CONTENT_LENGTH = "content-length";
static const char *HEADER_CONNECTION = "connection";
static const char *MIME_HTML = "text/html";

Response::Response() : fullPath_(".") {}

Response::Response(std::map<int, std::string> error_pages) :
    fullPath_("."), error_pages_config(error_pages) {}

Response::Response(std::map<int, std::string> error_pages, int code, const std::string &message, bool error)
    : fullPath_("."), error_pages_config(error_pages)
{
    setPage(code, message, error);
}

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
                generateAutoIndex(loc);
                return;
            } else
                return (setPage(403, "Directory listing denied.", true));
        }

        if (!S_ISREG(file_stat.st_mode))
            return (setPage(403, "Requested resource is not a file", true));
        if (!(file_stat.st_mode & S_IROTH))
            return (setPage(403, "You do not have permission to read this file", true));

        readFileIntoBody(fullPath_);
        setHeader(HEADER_CONTENT_LENGTH, int_to_string(body_.size()));
        setHeader(HEADER_CONTENT_TYPE, getContentType(fullPath_));
        statusCode_ = 200;
        return;
    } else
    {
        if (errno == ENOENT)
            return (setPage(404, "File does not exist", true));
        else if (errno == EACCES)
            return (setPage(403, "Access denied.", true));
        else
            return (setPage(500, "Internal server error while accessing file.", true));
    }
}

void Response:: readFileIntoBody(const std::string &fileName) {
    std::ifstream file(fileName.c_str(), std::ios::in | std::ios::binary);
    if (!file) {
        std::ifstream test(fileName.c_str());
        if (!test) {
            setPage(404, "Not Found", true);
        } else {
            setPage(500, "Server error: unable to open file.", true);
        }
        return;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    body_ = ss.str();
}

void Response::generateAutoIndex(const LocationConfig& loc) {
    std::string uri = loc.getUri();
    std::string path = fullPath_;

    DIR *dir = opendir(path.c_str());
    if (!dir){
        setPage(403, "Forbidden", true);
        return;
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
    setPage(200, "OK", false);
    closedir(dir);
    body_ = html.str();
    return;
}

std::string Response::getContentType(const std::string &path)
{
    static std::map<std::string, std::string> mime;
    if (mime.empty())
    {
        mime[".html"] = "text/html";
        mime[".htm"] = "text/html";
        mime[".css"] = "text/css";
        mime[".js"] = "application/javascript";
        mime[".png"] = "image/png";
        mime[".jpg"] = "image/jpeg";
        mime[".jpeg"] = "image/jpeg";
        mime[".gif"] = "image/gif";
        mime[".txt"] = "text/plain";
        mime[".pdf"] = "application/pdf";
        mime[".json"] = "application/json";
        mime[".svg"] = "image/svg+xml";
    }
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
    if (!loc.getAllowUpload())
        return (setPage(403, "Forbidden", true));
    else {
        std::vector<std::string> allowed = loc.getAllowedMethods();
        allowed.push_back("POST");
        loc.setAllowedMethods(allowed);
    }
    if (loc.getAllowUpload() && loc.getUploadDir().empty())
        throw std::runtime_error("Config error: allow_upload is enabled but no upload_path specified in location " + loc.getUri());

    if (reqObj.getReqPath() != "/upload") {
        setBody(generateDefaultPage(404, "Wrong path. Expected \"/upload", true));
        return;
    }
    if (reqObj.getBody().empty()) {
        return (setPage(400, "No body detected on request. Body necessary", true));
    }

    if (!reqObj.findHeader(HEADER_CONTENT_TYPE)) {
        setPage(400, "Missing Content-Type header", true);
        return;
    }

    parseMultipartBody(reqObj);
    std::string uploadFullPath = "./" + loc.getUploadDir();
    createUploadDir(uploadFullPath);
    uploadFile(uploadFullPath);
}

void Response::createUploadDir(const std::string &uploadFullPath) {
    if (mkdir(uploadFullPath.c_str(), 0755) == -1) {
        if (errno != EEXIST)
            std::cerr << "mkdir failed for " << uploadFullPath << ": " << strerror(errno) << std::endl;
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
        return (setPage(500, "Server error: could not open file for writing.", true));
}

void Response::handleDelete(const Request &reqObj) {
    std::string prefix = "/upload/";

    if (reqObj.getReqPath().compare(0, prefix.size(), prefix) != 0) {
        setPage(404, "Wrong path. Expected \"/upload/", true);
        return;
    }

    filename_ = "." + reqObj.getFullPath();
    struct stat fileStat;

    if (stat(filename_.c_str(), &fileStat) != 0) {
        setPage(404, "File not found: \"" + filename_ + "\"", true);
        return;
    }

    if (!S_ISREG(fileStat.st_mode)) {
        setPage(404, "\"" + filename_ + "\" is not a regular file", true);
        return;
    }

    if (remove(filename_.c_str()) != 0) {
        setPage(500, "Failed to delete file: \"" + filename_ + "\"", true);
        return;
    }
    setPage(200, "File \"" + filename_ + "\" deleted successfully.", true);
}

void Response::parseMultipartBody(const Request &obj) {
    // ------ GET BOUNDARY -----

    std::string rawValue = *obj.findHeader(HEADER_CONTENT_TYPE);
    size_t pos = rawValue.find("boundary=");
    std::string boundary = "--";
    if (pos != std::string::npos)
        boundary.append(rawValue.substr(pos + 9));

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
        filename_ = headers.substr(start, end - start);
        std::cout << filename_ << std::endl;
    }

    // ---- EXTRACT CONTENT-TYPE
    pos = headers.find("content-type: ");
    if (pos != std::string::npos) {
        start = pos + 14;
        end = headers.find("\r\n", start);
        contentType_ = headers.substr(start, end - start);
        setHeader(HEADER_CONTENT_TYPE, contentType_);
    }
    body_ = fileContent;
}

std::string Response::writeResponseString() const
{
    std::ostringstream res;
    res << version_ << " " << statusCode_ << " " << statusMessage_ << "\r\n"
        << getHeaders() << "\r\n"
        << body_;
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
        codeToMessage[400] = "Bad Request";
        codeToMessage[403] = "Forbidden";
        codeToMessage[404] = "Not Found";
        codeToMessage[405] = "Method Not Allowed";
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
    setHeader(HEADER_CONTENT_LENGTH, int_to_string(body_.size()));
    setHeader(HEADER_CONTENT_TYPE, MIME_HTML);
}

std::string Response::generateDefaultPage(const int code, const std::string &message, bool error) const
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

std::string Response::buildResponse(const Request &reqObj, const LocationConfig &locConfig)
{
    this->setVersion(reqObj.getVersion());
    this->setFullPath(reqObj.getFullPath());

    if (reqObj.getReqPath().size() > MAX_URI_LENGTH) {
        this->setPage(414, "URI Too Long", true);
        return (this->writeResponseString());
    }

    if (!locConfig.isMethodAllowed(reqObj.getMethod()) && reqObj.getMethod() != "POST") {
        this->setPage(405, "Method not allowed", true);
        this->setHeader("Allow", "GET, POST, DELETE");
    }
    else if (reqObj.isCgi()) {
        this->handleCgi(reqObj, locConfig);
    } else if (!reqObj.getMethod().compare("GET")) {
        this->handleGet(reqObj, locConfig);
    } else if (!reqObj.getMethod().compare("POST")) {
        this->handlePost(reqObj, locConfig);
    } else if (!reqObj.getMethod().compare("DELETE")) {
        this->handleDelete(reqObj);
    } else
        this->setPage(501, "Method not implemented", true);

    if (reqObj.findHeader(HEADER_CONNECTION))
        this->setHeader(HEADER_CONNECTION, *reqObj.findHeader(HEADER_CONNECTION));

    return (this->writeResponseString());
}

void Response::handleCgi(const Request &reqObj, const LocationConfig &locConfig)
{
    std::string cgiScriptPath = "." + locConfig.getRoot() + reqObj.getReqPath();

	std::cout << "Processing CGI Request: " << reqObj.getMethod() << " " << cgiScriptPath << std::endl;
    CgiHandler cgiHandler(reqObj, locConfig);
	// setenv("SERVER_NAME", "localhost", 1); // Replace with actual server name if available
	// setenv("SERVER_PORT", "8080", 1);     // Replace with actual server port if available

	// Check if the CGI script was found
    if (!cgiHandler.getStatus() && cgiHandler.getError() == CgiHandler::SCRIPT_NOT_FOUND) {
        setPage(404, "The requested CGI script was not found: " + reqObj.getReqPath(), true);
        setHeader("Content-Type", "text/html");
        return;
    }
    // Run the CGI script and capture its output
    std::string cgiOutput = cgiHandler.run();
    if (cgiHandler.getStatus()) {
        // CGI execution was successful
        std::cout << "CGI execution successful: " << reqObj.getReqPath() << std::endl;
        setCode(200);
		std::cout << "Raw CGI output:\n" << cgiOutput << "\nEND OF CGI OUTPUT\n";
        parseCgiResponse(cgiOutput);
    } else {
		switch (cgiHandler.getError()) {
			case CgiHandler::PIPE_FAILED:
				setPage(500, "CGI execution failed: pipe creation failed", true);
				setHeader("Content-Type", "text/plain");
				break;
			case CgiHandler::FORK_FAILED:
				setPage(500, "CGI execution failed: fork failed", true);
				setHeader("Content-Type", "text/plain");
				break;
			case CgiHandler::EXECVE_FAILED:
				setPage(500, "CGI execution failed: execve failed", true);
				setHeader("Content-Type", "text/plain");
				break;
			case CgiHandler::TIMEOUT:
				setPage(504, "CGI execution failed: script timed out", true);
				setHeader("Content-Type", "text/plain");
				break;
			case CgiHandler::CGI_SCRIPT_FAILED:
				setPage(500, "CGI execution failed: script error", true);
				setHeader("Content-Type", "text/plain");
				break;
			default:
				setPage(500, "CGI execution failed: unknown error", true);
				setHeader("Content-Type", "text/plain");
		}
    }
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
}
