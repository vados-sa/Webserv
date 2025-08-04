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
    void addLocation(LocationConfig &locConfig) { locations.push_back(locConfig); };

    int getPort() { return listen_port; };
    const std::string &getHost() { return host; };
    const std::unordered_map<int, std::string> &getErrorPagesConfig() { return error_pages_config; }
    const std::string &getErrorPage (int code) const;
    int getMaxBodySize() { return client_max_body_size; };
    //get locations? talvez entender o uso antes

    void setHost(std::string set) { host = set; };
    void setPort(int set) { listen_port = set; };
    void setErrorPagesConfig(std::unordered_map<int, std::string> set) { error_pages_config = set; };
    void setMaxBodySize(int set) { client_max_body_size = set; };
    //set location? entender o uso
};

std::ostream &operator<<(std::ostream &os, ServerConfig &obj);
