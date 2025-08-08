#pragma once

#include <string>
#include <vector>
#include <iostream>

class LocationConfig
{
private:
    std::string path;
    std::string root;
    std::vector<std::string> index_files;
    std::vector<std::string> allowed_methods;
    std::string upload_dir;
    bool autoindex;
    bool allow_upload;
    //something about redirection, but im not sure how to store it
    //also something about cgi string

public:
    LocationConfig();

    void setPath(std::string set) { path = set; };
    void setRoot(std::string set) { root = set; };
    void setIndex(std::vector<std::string> set) { index_files = set; };
    void setAllowedMethods(std::vector<std::string> set) { allowed_methods = set; };
    void setUploadDir(std::string set) { upload_dir = set; };
    void setAutoindex(bool set) { autoindex = set; };

    std::string getPath() const { return path; }
    std::string getRoot() const { return root; }
    const std::vector<std::string> &getIndexFiles() const { return index_files; }
    const std::vector<std::string> &getAllowedMethods() const { return allowed_methods; };
    const std::string &getUploadDir() const { return upload_dir; };
    const bool &getAutoindex() const { return autoindex; };
};

std::ostream &operator<<(std::ostream &os, const std::vector<LocationConfig> &obj);

std::ostream &operator<<(std::ostream &os, const std::vector<std::string>& obj);

