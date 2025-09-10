
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

    std::string wrapHtml(const std::string &title, const std::string &innerHtml)
    {
        // Inline minimal CSS matching your test.html look (no external deps; C++98-safe)
        const std::string css =
            "body{margin:0;min-height:100vh;display:flex;align-items:center;justify-content:center;"
            "padding:30px 0;box-sizing:border-box;"
            "background:url('/img/images/background.png') center/cover no-repeat fixed;"
            "background-color:#41479b;"
            "font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,'Helvetica Neue',Arial,"
            "'Noto Sans','Liberation Sans',sans-serif;color:#0f4910}"
            ".content{margin:0 auto;width:85%;max-width:960px;background:#F2F1EF;border-radius:10px;"
            "box-shadow:2px 2px 9px rgba(0,0,0,0.3);padding:24px 28px}"
            "h1,h2,h3{font-weight:600;margin:0 0 12px;text-align:center}"
            "hr{border:0;border-top:1px solid #AAB03C;margin:20px auto}"
            "p{line-height:1.5;font-size:15px;text-align:justify}"
            ".center{text-align:center;margin:12px 0;}";

        std::string html;
        html.reserve(1024 + innerHtml.size());
        html += "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\">";
        html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
        html += "<title>";
        html += title;
        html += "</title><style>";
        html += css;
        html += "</style></head><body><div class=\"content\">";
        html += innerHtml;
        html += "</div></body></html>";
        return html;
        //return html.str();
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