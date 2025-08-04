#include "ServerConfig.hpp"

ServerConfig::ServerConfig() {};

const std::string &ServerConfig::getErrorPage(int code) const {
    std::unordered_map<int, std::string>::const_iterator it = error_pages_config.find(code);

    if (it != error_pages_config.end())
        return it->second;

    static const std::string empty = "";
    return (empty);
};

std::ostream &operator<<(std::ostream &os, ServerConfig &obj) {
    os << "SERVERCONFIG:\nListen port: " << obj.getPort()
        << "\nHost: " << obj.getHost() << std::endl;
    return os;
};