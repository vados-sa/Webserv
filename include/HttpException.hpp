#pragma once

#include <exception>
#include <string>

class HttpException : public std::exception {
    private:
    int statusCode;
    std::string message;
    bool error;

    public:
    HttpException(int code, const std::string &msg, const bool error) : statusCode(code), message(msg), error(error) {};

    virtual ~HttpException() throw() {}

    virtual const char *what() const throw() { return message.c_str(); }
    int getStatusCode() const { return statusCode; }
    bool getError() const { return error; }
};