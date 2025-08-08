#include "Config.hpp"
#include "ConfigParser.hpp"

Config::Config(const std::string &filepath) {
    loadFromFile(filepath);
}

Config &Config::operator=(const Config &other)
{
    if (this != &other)
    {
        this->servers = other.servers;
    }
    return *this;
}

void Config::addServer(ServerConfig &server) {
    servers.push_back(server);
}

void Config::loadFromFile(const std::string &filepath) {
    ConfigParser parser;

    *this = parser.parseConfigFile(filepath);
}

std::ostream &operator<<(std::ostream &os, const Config &obj) {
    std::vector<ServerConfig> serversVector = obj.getServers();

    for (size_t i = 0; i < serversVector.size(); i ++) {
        os << serversVector[i] << std::endl;
    }
    return os;
}