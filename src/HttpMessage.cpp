#include "HttpMessage.hpp"

HttpMessage::HttpMessage() : body_(""), version_(""), headers_() {}


void HttpMessage::setHeader(std::string key, std::string value)
{
    headers_[key] = value;
}

const std::string *HttpMessage::findHeader(const std::string &key) const {
    std::map<std::string, std::string>::const_iterator it = headers_.find(key);
    if (it != headers_.end())
        return (&it->second);
    return (NULL);
}