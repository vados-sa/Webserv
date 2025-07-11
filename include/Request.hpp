#pragma once

#include <string>

class Request
{
private:
    //one dictionary with the request line (Method, Path and HTTP version)
    //another dictionary for headers
    //string for body

public:
    Request();
    Request(std::string &req);
    Request(const Request &obj);
    Request &operator=(const Request &obj);
    ~Request();

    //---methods
    // static Request parse(const std::string &raw);
    //..
};

