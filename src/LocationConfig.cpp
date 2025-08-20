#include "LocationConfig.hpp"

LocationConfig::LocationConfig() : has_return(false), autoindex(false), allow_upload(false)
{
    allowed_methods.push_back("GET");
    allowed_methods.push_back("POST");
    allowed_methods.push_back("DELETE");
}

std::ostream &operator<<(std::ostream &os, const std::vector<LocationConfig>& obj) {
    for (size_t i = 0; i < obj.size(); i++) {
        os << "Location " << i << ":\n";
        os << " -- Root: " << obj[i].getRoot() << "\n";
        os << " -- Path: " << obj[i].getUri() << "\n";
        os << " -- Index: " << obj[i].getIndexFiles() << "\n";
        os << " -- Allowed methods: " << obj[i].getAllowedMethods() << "\n";
        os << " -- Redirection: " << obj[i].getReturnStatus() << " " << obj[i].getReturnTarget() << "\n";
        os << " -- Allow upload: " << obj[i].getAllowUpload() << "\n";
        os << " -- Upload dir: " << obj[i].getUploadDir() << "\n";
        os << " -- Autoindex: " << obj[i].getAutoindex() << "\n";
		os << " -- CGI extension: " << obj[i].getCgiExtension() << "\n";
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

bool LocationConfig::isCgiRequest(std::string &uri) {
    size_t pos = uri.rfind('.');
    if (pos != std::string::npos) {
        std::string extension = uri.substr(pos);
        return extension == cgi_extension;
    }
    return false;
}
