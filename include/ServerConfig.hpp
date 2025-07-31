#pragma once

#include <string>
#include <vector>

#include "LocationConfig.hpp"

class ServerConfig
{
private:
    int listen_port;
    std::string host;
    std::vector<LocationConfig> locations;

public:
    ServerConfig();
    void addLocation(LocationConfig &locConfig);

    const int getPort() { int listen_port; };
    const std::string &getHost() { std::string host; };

    void setHost(std::string set) { host = set; };
    void setPort(int set) { listen_port = set; };
};

