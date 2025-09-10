
#include "Utils.hpp"
#include "HttpException.hpp"
#include "Logger.hpp"

#include <string>
#include <sys/stat.h>
#include <sstream>
#include <fstream>
#include <vector>
#include <cerrno>

namespace util {

    std::string normalizePath(const std::string &rawPath)
    {
        std::istringstream iss(rawPath);
        std::vector<std::string> parts;
        std::string token;

        while (std::getline(iss, token, '/'))
        {
            if (token.empty() || token == ".")
                continue;
            else if (token == "..")
            {
                if (!parts.empty())
                    parts.pop_back();
            }
            else
                parts.push_back(token);
        }

        std::string normalized = "/";
        for (std::vector<std::string>::iterator it = parts.begin(); it != parts.end(); ++it)
        {
            normalized += *it;
            if (it + 1 != parts.end())
                normalized += "/";
        }
        return (normalized);
    }

    std::string sanitizeFileName(const std::string &fileName)
    {
        std::string sanitized = fileName;
        std::replace(sanitized.begin(), sanitized.end(), '/', '_');
        std::replace(sanitized.begin(), sanitized.end(), '\\', '_');
        return sanitized;
    }

    bool isValidPathChar(char c)
    {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '/' || c == '-' || c == '_' || c == '.' || c == '~')
            return (true);
        return (false);
    }

    bool isValidPath(const std::string &path)
    {
        for (std::string::const_iterator it = path.begin(); it != path.end(); ++it)
            if (!isValidPathChar(*it))
                return (false);
        return (true);
    }

    std::string intToString(int value)
    {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }

    bool fileExists(const std::string &path, struct ::stat &st)
    {
        if (::stat(path.c_str(), &st) == 0)
            return true;
        if (errno == ENOENT)
            throw HttpException(404, "File does not exist", true);
        if (errno == EACCES)
            throw HttpException(403, "Access denied", true);
        throw HttpException(500, "Internal server error while accessing file", true);
    }

    bool saveFile(const std::string &filePath, const std::string &fileContent)
    {
        std::ofstream outFile(filePath.c_str(), std::ios::binary);
        std::string msg;
        if (!outFile)
        {
            logs(ERROR, "Failed to open file: " + filePath);
            return false;
        }
        outFile.write(fileContent.c_str(), fileContent.size());
        outFile.close();
        msg = "File uploaded successfully: " + filePath;
        logs(INFO, msg);
        return true;
    }

    bool createUploadDir(const std::string &uploadFullPath)
    {
        if (mkdir(uploadFullPath.c_str(), 0755) == -1)
        {
            if (errno != EEXIST)
            {
                std::ostringstream oss;
                oss << "mkdir failed for " << uploadFullPath << ": " << strerror(errno);
                std::string msg = oss.str();
                logs(ERROR, msg);
                return (false);
            }
            return (true);
        }
        return (true);
    }

    std::string wrapHtml(const std::string &title, const std::string &body)
    {
        std::ostringstream html;
        html << "<!DOCTYPE html>\n<html>\n<head>\n"
             << "<meta charset=\"UTF-8\">\n<title>" << title << "</title>\n"
             << "</head>\n<body>\n"
             << body
             << "\n</body>\n</html>";
        return html.str();
    }

    std::string generateAutoIndexHtml(const std::string &uri, const std::vector<std::string> &entries)
    {
        std::ostringstream body;
        body << "<h1>Index of " << uri << "</h1>\n<ul>\n";

        for (std::vector<std::string>::const_iterator it = entries.begin(); it != entries.end(); ++it)
        {
            body << "<li><a href=\"" << uri;
            if (uri[uri.size() - 1] != '/')
                body << "/";
            body << *it << "\">" << *it << "</a></li>\n";
        }
        body << "</ul>\n";

        return wrapHtml("Index of " + uri, body.str());
    }

    std::string extractBoundary(std::string rawValue) {
        size_t pos = rawValue.find("boundary=");
        std::string boundary = "--";
        if (pos != std::string::npos)
            boundary.append(rawValue.substr(pos + 9));
        return (boundary);
    }

    std::string extractFilename(std::string headers) {
        size_t pos = headers.find("filename=\"");
        if (pos != std::string::npos)
        {
            size_t start = pos + 10;
            size_t end = headers.find("\"", start);
            return (headers.substr(start, end - start));
        }
        return ("");
    }

    std::string extractContentType(std::string headers) {
        size_t pos = headers.find("Content-Type: ");
        if (pos != std::string::npos)
        {
            size_t start = pos + 14;
            size_t end = headers.find("\r\n", start);
            return (headers.substr(start, end - start));
        }
        return ("");
    }

    std::string parseChunkedBody(std::string &raw)
    {
        //body_.clear();
        std::string body;
        std::string::size_type pos = 0;
        //std::size_t totalSize = 0;

        while (true)
        {
            std::string::size_type endline = raw.find("\r\n", pos);
            if (endline == std::string::npos)
                throw HttpException(400, "Malformed chunk size line", true);

            std::string sizeStr = raw.substr(pos, endline - pos);
            std::istringstream iss(sizeStr);
            std::size_t chunkSize = 0;
            iss >> std::hex >> chunkSize;
            if (iss.fail())
                throw HttpException(400, "Invalid chunk size", true);

            pos = endline + 2; // move past "\r\n"

            if (chunkSize == 0)
            {
                std::string::size_type trailerEnd = raw.find("\r\n", pos);
                if (trailerEnd == std::string::npos)
                    throw HttpException(400, "Missing CRLF after last chunk", true);
                raw.erase(0, trailerEnd + 2);
                return (body);
            }

            //totalSize += chunkSize;
            // if ((int)totalSize > maxBodySize)
            //     throw HttpException(413, "Payload Too Large", true);

            if (raw.size() < pos + chunkSize + 2)
                throw HttpException(400, "Incomplete chunk data", true);

            body.append(raw, pos, chunkSize);

            pos += chunkSize;

            if (raw.substr(pos, 2) != "\r\n")
                throw HttpException(400, "Missing CRLF after chunk data", true);
            pos += 2;
        }
    }

    MultipartPart parseMultipartBody(const std::string &rawBody, const std::string &boundary)
    {
        MultipartPart part;

        std::string body = rawBody;
        size_t start = body.find(boundary);
        if (start != std::string::npos)
            body = body.substr(start + boundary.length() + 2);

        std::string closingBoundary = boundary + "--";
        size_t end = body.rfind(closingBoundary);
        if (end != std::string::npos)
            body = body.substr(0, end - 2);

        size_t headerEnd = body.find("\r\n\r\n");
        if (headerEnd == std::string::npos)
            throw HttpException(400, "Malformed multipart part", true);

        std::string headers = body.substr(0, headerEnd);
        part.content = body.substr(headerEnd + 4);

        part.filename = extractFilename(headers);
        if (part.filename.empty())
            throw HttpException(400, "Filename missing in multipart part", true);

        part.contentType = extractContentType(headers);
        if (part.contentType.empty())
            throw HttpException(400, "Content-Type missing in multipart part", true);

        return part;
    }

}