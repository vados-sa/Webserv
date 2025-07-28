#include "Response.hpp"

Response::Response() : statusCode_(""), statusMessage_(""), fullPath_("./www"), filename_("upload/") {}

void Response::handleGet(const Request &reqObj) {

    (void) reqObj;
    struct stat file_stat;
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

        setHeader("Content-Length", std::to_string(body_.size()));
        setHeader("Content-Type", "text/html");
        statusCode_ = "200";
        return;
    }
    else {
        if (errno == ENOENT)
            return (setPage("404", "File does not exist", true));
        else if (errno == EACCES)
            return (setPage("403", "Access denied.", true));
        else
            return (setPage("500", "Internal server error while accessing file.", true));
    }
}

void Response::handlePost(const Request &reqObj)
{
    if (reqObj.getPath() != "/upload/") {
        setBody(generatePage("404", "Wrong path. Expected \"/upload/", true));
        return;
    }
    if (reqObj.getBody().empty()) {
        return (setPage("400", "No body detected on request. Body necessary", true));
    }
    parseContentType(reqObj);
    mkdir("upload", 0755);
    std::ofstream file(filename_.c_str(), std::ios::binary);
    if (file.is_open()) {
        file.write(body_.c_str(), body_.size());
        file.close();
        return (setPage("201", "File created", false));
    } else
        return (setPage("500", "Server error: could not open file for writing.", true));
}

void Response::handleDelete(const Request &reqObj) {
    if (reqObj.getPath().substr(0, 8) != "/upload/")
    {
        setPage("404", "Wrong path. Expected \"/upload/", true);
        return;
    }

    filename_ = filename_.append(reqObj.getPath().substr(8));
    struct stat fileStat;

    if (stat(filename_.c_str(), &fileStat) != 0) {
        setPage("404", "\"" + filename_ + "\" is not a regular file", true);
        return;
    };

    if(!S_ISREG(fileStat.st_mode)) {
        setPage("400", "File not found :\"" + filename_ + "\"", true);
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

    string rawValue = *obj.findHeader("Content-Type");
    size_t pos = rawValue.find("boundary=");
    string boundary = "--";
    if (pos != string::npos)
        boundary.append(rawValue.substr(pos + 9));    //should i check if it comes wrapped in quotes?

    // ----- EXTRACT BOUNDARY FROM BODY

    string rawBody = obj.getBody();
    size_t start = rawBody.find(boundary);
    if (start != string::npos)
        rawBody = rawBody.substr(start + boundary.length() + 2);

    boundary.append("--");
    size_t end = rawBody.rfind(boundary);
    if (end != string::npos)
        rawBody = rawBody.substr(0, end - 2);

    // ----- EXTRACT HEADERS FROM THE BODY
    string headers;
    string fileContent;
    size_t headerEnd = rawBody.find("\r\n\r\n");
    if (headerEnd != string::npos) {
        headers = rawBody.substr(0, headerEnd);
        fileContent = rawBody.substr(headerEnd + 4);
    }

    // ---- EXTRACT FILENAME
    pos = headers.find("filename=\"");
    if (pos != std::string::npos) {
        size_t start = pos + 10;
        size_t end = headers.find("\"", start);
        filename_ = filename_.append(headers.substr(start, end - start));
        cout << filename_ << endl;
    }

    // ---- EXTRACT CONTENT-TYPE
    pos = headers.find("Content-Type: ");
    if (pos != std::string::npos) {
        start = pos + 14;
        end = headers.find("\r\n", start);
        contentType_ = headers.substr(start, end - start);
    }
    body_ = fileContent;
}

string Response::writeResponseString() {
    std::ostringstream res;
    res << version_ << " " << statusCode_ << " " << statusMessage_ << "\r\n"
        << getHeaders() << "\r\n"
        << body_ << "\r\n";
    return (res.str());
}

void Response::setCode(const string code)
{
    statusCode_ = code;

    static std::map<string, string> codeToMessage;
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

    std::map<string, string>::iterator it = codeToMessage.find(code);
    if (it != codeToMessage.end())
        statusMessage_ = it->second;
    else
        statusMessage_ = "Unknown Status";
}

void Response::setPage(const string &code, const string &message, bool error) {
    setCode(code);
    body_ = generatePage(code, message, error);
    setHeader("Content-Length", std::to_string(body_.size()));
    setHeader("Content-Type", "text/html");
}

string generatePage(const string &code, const string &message, bool error)
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

void Response::setFullPath(const string &reqPath) {
    fullPath_.append(reqPath);
}

std::ostream &operator<<(std::ostream &out, const Response &obj)
{
    out << "Version: " << obj.getVersion() << endl
        << "Code: " << obj.getCode() << endl
        << "Status Message: " << obj.getStatusMessage() << endl
        << " ----- " << endl
        << "Headers: " << endl
        << obj.getHeaders() << endl
        << " ----- " << endl
        << "Body: " << obj.getBody() << endl;
    return (out);
}

string buildResponse(const Request &reqObj)
{
    Response res;

    res.setVersion(reqObj.getVersion());
    res.setFullPath(reqObj.getPath());
    if (!reqObj.getMethod().compare("GET"))
        res.handleGet(reqObj);
    if (!reqObj.getMethod().compare("POST"))
        res.handlePost(reqObj);
    if (!reqObj.getMethod().compare("DELETE"))
        res.handleDelete(reqObj);
    if (reqObj.findHeader("Connection"))
        res.setHeader("Connection", *reqObj.findHeader("Connection"));
    string reqStr = res.writeResponseString();
    return (reqStr);
}

