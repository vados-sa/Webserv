#include "ServerConfig.hpp"

ServerConfig::ServerConfig() : listen_port(-1), host("") {}

std::ostream &operator<<(std::ostream &os, ServerConfig &obj) {
    os << "SERVERCONFIG:\nListen port: " << obj.getPort()
        << "\nHost: " << obj.getHost() << std::endl;
    return os;
}