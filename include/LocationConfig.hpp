#pragma once

#include <string>
using std::string;

#include <vector>
using std::vector;

class LocationConfig
{
private:
    string root;
    vector<string> index_files;
public:
    LocationConfig();

};


