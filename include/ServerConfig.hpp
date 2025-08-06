#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>

#include "LocationConfig.hpp"

class ServerConfig
{
private:
    int listen_port;
    std::string host;
    std::unordered_map<int, std::string> error_pages_config;
    int client_max_body_size;
    std::vector<LocationConfig> locations;

public:
    ServerConfig();

    int getPort() const { return listen_port; }
    const std::string &getHost() const { return host; }
    const std::unordered_map<int, std::string> &getErrorPagesConfig() const { return error_pages_config; }
    const std::string &getErrorPage (int code) const;
    int getMaxBodySize() const { return client_max_body_size; }
    const std::vector<LocationConfig> &getLocations() const { return locations; }

    void setHost(std::string set) { host = set; };
    void setPort(int set) { listen_port = set; };
    void setErrorPagesConfig(std::pair<int, std::string> set) { error_pages_config[set.first] = set.second; };
    void setMaxBodySize(int set) { client_max_body_size = set; };
    void addLocation(LocationConfig &locConfig) { locations.push_back(locConfig); };
};

std::ostream &operator<<(std::ostream &os, ServerConfig &obj);

