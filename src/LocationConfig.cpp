#include "LocationConfig.hpp"

LocationConfig::LocationConfig() : autoindex(false)
{
    allowed_methods.push_back("GET");
    allowed_methods.push_back("POST");
    allowed_methods.push_back("DELETE");
}

std::ostream &operator<<(std::ostream &os, const std::vector<LocationConfig>& obj) {
    for (size_t i = 0; i < obj.size(); i++) {
        os << "Location " << i << ":\n";
        os << " -- Root: " << obj[i].getRoot() << "\n";
        os << " -- Path: " << obj[i].getPath() << "\n";
        os << " -- Index: " << obj[i].getIndexFiles() << "\n";
        os << " -- Allowed methods: " << obj[i].getAllowedMethods() << "\n";
        os << " -- Upload dir: " << obj[i].getUploadDir() << "\n";
        os << " -- Autoindex: " << obj[i].getAutoindex() << "\n";
    }
    return (os);
}

std::ostream &operator<<(std::ostream &os, const std::vector<std::string>& obj) {
    os << "[";
    for (size_t i = 0; i < obj.size(); i++) {
        os << obj[i];
        if (i != obj.size() - 1)
            os << ", ";
    }
    os << "]/n";
    return (os);
}
