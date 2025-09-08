
#include "Utils.hpp"

#include <sstream>
#include <vector>

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
}