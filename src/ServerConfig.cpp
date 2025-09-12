#include "ServerConfig.hpp"

ServerConfig::ServerConfig() : listen_port(-1), host(""), error_pages_config(),
                                client_max_body_size(1048576), locations()
{};

const std::string &ServerConfig::getErrorPage(int code) const {
    std::map<int, std::string>::const_iterator it = error_pages_config.find(code);

    if (it != error_pages_config.end())
        return it->second;

    static const std::string empty = "";
    return (empty);
};

std::ostream &operator<<(std::ostream &os, const ServerConfig &obj) {
    os << "==== SERVER CONFIGURATION OBJECT ====";
    os << "\n- Listen port: " << obj.getPort();
    os << "\n- Host: " << obj.getHost();
    os << "\n- Error Pages: " << obj.getErrorPagesConfig();
    os << "\n- Client Max Body Size: " << obj.getMaxBodySize();
    os << "\n- Locations: " << obj.getLocations() << std::endl;
    return os;
};

std::ostream &operator<<(std::ostream &os, const std::map<int, std::string>&obj) {
    std::map<int, std::string>::const_iterator it;

    for (it = obj.begin(); it != obj.end(); ++it) {
        os << "Error [" << it->first << "] = " << it->second << "\n";
    }
    return (os);
};

