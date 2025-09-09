
#include "Utils.hpp"
#include "HttpException.hpp"

#include <string>
#include <sys/stat.h>
#include <sstream>
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
}