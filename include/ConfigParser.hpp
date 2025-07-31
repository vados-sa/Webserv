#pragma once

#include "Config.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>

class ConfigParser {
private:
    std::vector<std::string> collectBlock(std::ifstream file, std::string line);
    ServerConfig parseServerBlock(std::vector<std::string> lines);
    std::string parseHost(std::string line);
    const int parsePort(std::string line);

    LocationConfig parseLocationBlock(std::istream &input);
public:
    Config parseConfigFile(std::string &filename);
};

std::string trimComment(std::string line);
std::string trimWhitespace(std::string line);
std::string trimLine(std::string line);

