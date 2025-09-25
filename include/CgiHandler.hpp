#pragma once

#include <map>
#include "Request.hpp"
#include "Client.hpp"
#include "LocationConfig.hpp"
#include "Logger.hpp"

class CgiHandler
{
public:
    enum CgiErrorType
    {
        SCRIPT_NOT_FOUND,
        EXECUTION_FAILED,
        EXECVE_FAILED,
        PIPE_FAILED,
        FORK_FAILED,
        TIMEOUT,
        CGI_SCRIPT_FAILED,
        NO_ERROR
    };

private:
    std::string cgiScriptPath;
    std::string interpreterPath;
    std::map<std::string, std::string> env;
    std::string body;
    CgiErrorType error;
    bool status_cgi;

public:
    CgiHandler(const Request &req, const LocationConfig &locConfig);
    bool getStatus() const { return status_cgi; };
    CgiErrorType getError() const { return error; };

    bool startCgi(Client &client);
    std::string finishCgi(const Client &client, int exit_status);
};

std::string getInterpreterPath(const std::string &cgiExtension);
