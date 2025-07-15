#include "../include/Request.hpp"

Request::Request() {
    _method = "";
    _path = "";
    _version = "";
    _body = "";
};

Request Request::parse(const std::string &raw)
{
    std::string methodToSet;
    std::string pathToSet;
    std::string versionToSet;
    std::string bodyToSet;

    // proceed with parsing
    //...
    // end parsing

    //maybe writing a fluent builder looks better?
    Request *reqObj = new Request();
    reqObj->setMethod(methodToSet);
    reqObj->setPath(pathToSet);
    reqObj->setVersion(versionToSet);
    reqObj->setBody(bodyToSet);
}

