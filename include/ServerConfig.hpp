#pragma once

#include <string>
using std::string;

#include <vector>
using std::vector;

#include "LocationConfig.hpp"

class ServerConfig
{
private:
    int listen_port;
    string host;
    vector<LocationConfig *> locations;
public:
    ServerConfig();
};


