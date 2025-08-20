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
    bool has_return;
    int return_status;
    std::string return_target;
    std::string upload_dir;
    bool autoindex;
    bool allow_upload;
    std::string cgi_extension;

public:
    LocationConfig();

    void setPath(std::string set) { path = set; };
    void setRoot(std::string set) { root = set; };
    void setIndex(std::vector<std::string> set) { index_files = set; };
    void setAllowedMethods(std::vector<std::string> set) { allowed_methods = set; };
    void setRedirection(std::pair<int, std::string> set) {
        has_return = true; return_status = set.first; return_target = set.second;};
    void setUploadDir(std::string set) { upload_dir = set; };
    void setAutoindex(bool set) { autoindex = set; };
    void setAllowUpload(bool set) { allow_upload = set; };
	void setCgiExtension(std::string set) { cgi_extension = set; };

    std::string getPath() const { return path; }
    std::string getRoot() const { return root; }
    const std::vector<std::string> &getIndexFiles() const { return index_files; }
    const std::vector<std::string> &getAllowedMethods() const { return allowed_methods; };
    const int &getReturnStatus() const { return return_status; };
    const std::string &getReturnTarget() const { return return_target; };
    const std::string &getUploadDir() const { return upload_dir; };
    const bool &getAutoindex() const { return autoindex; };
    const bool &getAllowUpload() const { return allow_upload; };
	const std::string &getCgiExtension() const { return cgi_extension; };
	bool isCgiRequest(std::string &uri);
};

std::ostream &operator<<(std::ostream &os, const std::vector<LocationConfig> &obj);

std::ostream &operator<<(std::ostream &os, const std::vector<std::string>& obj);

