#pragma once

#include <string>

namespace util {
    std::string normalizePath(const std::string &rawPath);
    bool isValidPathChar(char c);
    bool isValidPath(const std::string &path);
    std::string intToString(int value);
}