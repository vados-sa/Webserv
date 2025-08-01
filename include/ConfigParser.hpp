#pragma once

#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

class Config;

class ConfigParser {
private:
    std::vector<std::string> collectBlock(std::ifstream &file, std::string line);
    ServerConfig parseServerBlock(std::vector<std::string> lines);
    LocationConfig parseLocationBlock(std::vector<std::string> lines);

    std::string parseHost(std::string line);
    int parsePort(std::string line);
    std::string parseRoot(std::string line);
    std::vector<std::string> parseIndex(std::string line);
public:
    Config parseConfigFile(const std::string &filename);
};

std::string trimComment(std::string line);
std::string trimWhitespace(std::string line);
std::string trimLine(std::string line);

