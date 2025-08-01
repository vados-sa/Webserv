#pragma once

#include <string>
#include <vector>
#include <iostream>

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

    int getPort() { return listen_port; };
    const std::string &getHost() { return host; };

    void setHost(std::string set) { host = set; };
    void setPort(int set) { listen_port = set; };
};

std::ostream &operator<<(std::ostream &os, ServerConfig &obj);
