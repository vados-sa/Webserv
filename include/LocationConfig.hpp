#pragma once

#include <string>
#include <vector>

class LocationConfig
{
private:
    std::string root;
    std::vector<std::string> index_files;
    std::vector<std::string> allowed_methods;
    std::string upload_dir;
    bool autoindex;
    //something about redirection, but im not sure how to store it
    //also something about cgi

public:
    LocationConfig();

    void setRoot(std::string set) { root = set; };
    void setIndex(std::vector<std::string> set) { index_files = set; };
    void setAllowedMethods(std::vector<std::string> set) { allowed_methods = set; };
    void setUploadDir(std::string set) { upload_dir = set; };
    void setAutoindex(bool set) { autoindex = set; };

    const std::string &getRoot() { return root; };
    const std::vector<std::string> &getIndexFiles() { return index_files; };
    const std::vector<std::string> &getAllowedMethods() { return allowed_methods; };
    const std::string &getUploadDir() { return upload_dir; };
    const bool &getAutoindex() { return autoindex; };
};


