#pragma once

#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include <string>
#include <utility>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

class Config;

class ConfigParser {
private:
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
    std::string parseUploadDir(const std::vector<std::string> &tokens);
    bool parseAutoindex(const std::vector<std::string> &tokens);
    bool parseAllowUpload(const std::vector<std::string> &tokens);

public:
    Config  parseConfigFile(const std::string &filename);
};

bool isValidPathChar(char c);
bool isValidPath(const std::string &path);
std::string trimComment(std::string line);
std::string trimWhitespace(std::string line);
std::string trimLine(std::string line);
std::vector<std::string> tokenize(const std::string line);
