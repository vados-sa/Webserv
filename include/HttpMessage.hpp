#pragma once

#include <map>
#include <string>

class HttpMessage
{
protected:
    std::map<std::string, std::string> headers_;
    std::string body_;
    std::string version_;

public:
    HttpMessage();

    const std::string *findHeader(const std::string &key) const;

    std::map<std::string, std::string> getHeaders() const { return headers_; };
    std::string getBody() const { return body_; };
    std::string getVersion() const { return version_; };

    void setHeader(std::string key, std::string value);
    void setBody(const std::string &bodyToSet) { body_ = bodyToSet; };
    void setVersion(const std::string &versionToSet) { version_ = versionToSet; };
};

