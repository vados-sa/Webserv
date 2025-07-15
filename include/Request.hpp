#pragma once

#include <string>

class Request
{
private:
    std::string _method;
    std::string _path;
    std::string _version;
    //another dictionary for headers? just a big string? idk lets see how we're going to use it
    std::string _body;

public:
    Request();
    Request(const Request &obj); //do we need to write a copy constructor?
    Request &operator=(const Request &obj); //same
    ~Request(); //same

    //---methods
    static Request parse(const std::string &raw);
    //..

    //---getters
    std::string &getMethod() { return _method; };
    std::string &getPath() { return _path; };
    std::string &getVersion() { return _version; };
    std::string &getBody() { return _body; };

    //---setters
    void setMethod(const std::string &methodToSet) { _method = methodToSet; };
    void setPath(const std::string &pathToSet) { _path = pathToSet; };
    void setVersion(const std::string &versionToSet) { _version = versionToSet; };
    void setBody(const std::string &bodyToSet) { _body = bodyToSet; };
};

