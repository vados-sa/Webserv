#pragma once

#include <string>
#include <vector>

class LocationConfig
{
private:
    std::string root;
    std::vector<std::string> index_files;
public:
    LocationConfig();

    void setRoot(std::string set) { root = set; };
    void setIndex(std::vector<std::string> set) { index_files = set; };
};


