#pragma once

#include "ServerConfig.hpp"
#include <vector>
using std::vector;

class Config
{
private:
    vector<ServerConfig> servers;
public:
    Config();
    ~Config();

    void addServer(ServerConfig &server);
};


