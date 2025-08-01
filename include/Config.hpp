#pragma once

#include "ServerConfig.hpp"
#include <vector>
using std::vector;

class Config
{
private:
    vector<ServerConfig> servers;
    void loadFromFile(const std::string &filepath);
public:
    Config(const std::string &filepath);
    Config() {};
    Config(const Config &obj) : servers(obj.servers) {};
    Config &operator=(const Config &other);
    ~Config() {};

    void addServer(ServerConfig &server);
    const std::vector<ServerConfig> &getServers() const { return servers; };
};

std::ostream &operator<<(std::ostream &os, const Config &obj);
