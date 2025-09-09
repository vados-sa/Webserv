#pragma once

#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include <string>
#include <utility>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <set>

class Config;

class ConfigParser {
private:
    std::ostringstream errorMessage;
    std::string fileName;
    size_t lineNum;

    std::vector<std::string> collectBlock(std::vector<std::string> lines, size_t i);
    ServerConfig parseServerBlock(std::vector<std::string> lines);
    LocationConfig parseLocationBlock(std::vector<std::string> lines);

    // Parsers for the SERVER block
    std::string parseHost(const std::vector<std::string> &tokens);
    int parsePort(const std::vector<std::string> &tokens);
    std::pair<int, std::string> parseErrorPageLine(const std::vector<std::string> &tokens);
    size_t parseMaxBodySize(const std::vector<std::string> &tokens);

    // Parsers for the LOCATION block
    std::string parsePath(const std::vector<std::string> &tokens);
    std::string parseRoot(const std::vector<std::string> &tokens);
    std::vector<std::string> parseIndex(const std::vector<std::string> &tokens);
    std::vector<std::string> parseAllowedMethods(const std::vector<std::string> &tokens);
    std::pair<int, std::string> parseRedirection(const std::vector<std::string> &tokens);
    std::string parseUploadDir(const std::vector<std::string> &tokens);
    bool parseAutoindex(const std::vector<std::string> &tokens);
	std::string parseCgiExtension(const std::vector<std::string> &tokens);

public:
    Config  parseConfigFile(const std::string &filename);
};

std::string cleanLine(std::string line);
std::vector<std::string> tokenize(const std::string line);
