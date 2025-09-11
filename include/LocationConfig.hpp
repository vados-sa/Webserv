#pragma once

#include <string>
#include <vector>
#include <iostream>

class LocationConfig
{
private:
    std::string uri;
    std::string root;
    std::vector<std::string> index_files;
    std::vector<std::string> allowed_methods;
    bool has_return;
    int return_status;
    std::string return_target;
    std::string upload_dir;
    bool autoindex;
    std::string cgi_extension;
    int client_max_body_size;

public:
    LocationConfig();

    //methods
    bool isCgiRequest(std::string &uri);
    bool isMethodAllowed(const std::string &method) const;

    //getters
    std::string getUri() const { return uri; }
    std::string getRoot() const { return root; }
    const std::vector<std::string> &getIndexFiles() const { return index_files; }
    const std::vector<std::string> &getAllowedMethods() const { return allowed_methods; };
    const int &getReturnStatus() const { return return_status; };
    const std::string &getReturnTarget() const { return return_target; };
    const std::string &getUploadDir() const { return upload_dir; };
    const bool &getAutoindex() const { return autoindex; };
    const std::string &getCgiExtension() const { return cgi_extension; };
    int getMaxBodySize() const { return client_max_body_size; }

    //setters
    void setUri(std::string set) { uri = set; };
    void setRoot(std::string set) { root = set; };
    void setIndex(std::vector<std::string> set) { index_files = set; };
    void setAllowedMethods(std::vector<std::string> set) { allowed_methods = set; };
    void setRedirection(std::pair<int, std::string> set) { has_return = true; return_status = set.first; return_target = set.second;};
    void setUploadDir(std::string set) { upload_dir = set; };
    void setAutoindex(bool set) { autoindex = set; };
	void setCgiExtension(std::string set) { cgi_extension = set; };
    void setMaxBodySize(int set) { client_max_body_size = set; };
};

std::ostream &operator<<(std::ostream &os, const std::vector<LocationConfig> &obj);
std::ostream &operator<<(std::ostream &os, const std::vector<std::string>& obj);

