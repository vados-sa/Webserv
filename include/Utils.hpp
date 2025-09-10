#pragma once

#include <string>
#include <sys/stat.h>
#include <vector>

static const char *HEADER_CONTENT_TYPE = "content-type";
static const char *HEADER_CONTENT_LENGTH = "content-length";
static const char *HEADER_CONNECTION = "connection";
static const char *MIME_HTML = "text/html";

namespace util {

    struct MultipartPart
    {
        std::string filename;
        std::string contentType;
        std::string content;
    };

    std::string normalizePath(const std::string &rawPath);
    std::string sanitizeFileName(const std::string &fileName);
    bool isValidPathChar(char c);
    bool isValidPath(const std::string &path);
    std::string intToString(int value);
    bool fileExists(const std::string &path, struct ::stat &st);
    bool saveFile(const std::string &filePath, const std::string &fileContent);
    bool createUploadDir(const std::string &uploadFullPath);
        std::string wrapHtml(const std::string &title, const std::string &body);
    std::string generateAutoIndexHtml(const std::string &uri, const std::vector<std::string> &entries);
    std::string extractBoundary(std::string rawValue);
    std::string extractFilename(std::string headers);
    std::string extractContentType(std::string headers);
    std::string parseChunkedBody(std::string &raw);
    MultipartPart parseMultipartBody(const std::string &rawBody, const std::string &boundary);
}