#pragma once

#include <string>
#include <sys/stat.h>
#include <vector>

namespace util {
    std::string normalizePath(const std::string &rawPath);
    bool isValidPathChar(char c);
    bool isValidPath(const std::string &path);
    std::string intToString(int value);
    bool fileExists(const std::string &path, struct ::stat &st);
    std::string wrapHtml(const std::string &title, const std::string &body);
    std::string generateAutoIndexHtml(const std::string &uri, const std::vector<std::string> &entries);
}