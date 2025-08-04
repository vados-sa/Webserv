#include "ConfigParser.hpp"
#include "Config.hpp"

Config ConfigParser::parseConfigFile(const std::string &filename) {
    Config config;

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Couldn't open config file" << std::endl;
        return (Config());
    }

    std::string line;
    while(std::getline(file, line)) {
        line = trimWhitespace(line);
        if (line == "server {") {
            std::vector<std::string> serverLines = collectBlock(file, line);
            ServerConfig server = parseServerBlock(serverLines);
            config.addServer(server);
        }
    }
    return (config);
}

std::vector<std::string> ConfigParser::collectBlock(std::ifstream &file, std::string line) {
    std::vector<std::string> blockLines;
    int braceCount = 1;

    blockLines.push_back(line);
    while(std::getline(file, line)) {
        if (line.find('{') != std::string::npos) {
            braceCount++;
        }
        if (line.find('}') != std::string::npos) {
            braceCount--;
        }

        if (braceCount == 0)
            break;

        blockLines.push_back(line);
    }

    return (blockLines);
}

ServerConfig ConfigParser::parseServerBlock(std::vector<std::string> lines) {
    ServerConfig servConfig;

    for (size_t i = 0; i < lines.size(); i++) {
        lines[i] = trimLine(lines[i]);
        if (lines[i].find("host"))
            servConfig.setHost(parseHost(lines[i]));
        else if (lines[i].find("listen"))
            servConfig.setPort(parsePort(lines[i]));
        else if (lines[i].find("error_page"))
            servConfig.parseErrorPageLine(lines[i]);
        else if (lines[i].find("client_max_body_size"))
            servConfig.setMaxBodySize(parseMaxBodySize(lines[i]));

        else if (lines[i].find("location")) {
        //     //catch the block and parse it

        //     servConfig.addLocation("a");
        }
    }

    return servConfig;
}

std::string ConfigParser::parseHost(std::string line)
{
    if (line.compare(0, 4, "host")) {
        size_t pos = line.find_first_of(" \t", 4);
        if (pos == std::string::npos)
            return ""; //no argument?, host is empty

        std::string hostValue = line.substr(pos);
        hostValue = trimWhitespace(hostValue);
        //considering that i already checked and trim the semicolon
        return (hostValue);
    }
    return ("");
}

int ConfigParser::parsePort(std::string line)
{
    if (!line.compare(0, 7, "listen"))
    {
        size_t pos = line.find_first_of(" \t", 6);
        if (pos == std::string::npos)
            return (-1); // no argument?, port is empty

        std::string portValue = line.substr(pos);
        portValue = trimWhitespace(portValue);
        // considering that i already checked and trim the semicolon

        //check if portValue is numeric?

        int portInt = std::atoi(portValue.c_str());
        return (portInt);
    }
    return (-1);
}

void ConfigParser::parseErrorPageLine(std::string line) {
    if (!line.compare(0, 11, "error_page")) {
        size_t pos = line.find_first_of(" \t", 6);
    }
}

LocationConfig ConfigParser::parseLocationBlock(std::vector<std::string> lines)
{
    LocationConfig locConfig;
    for (size_t i = 0; i < lines.size(); i++)
    {
        lines[i] = trimLine(lines[i]);
        if (lines[i].find("root"))
            locConfig.setRoot(parseRoot(lines[i]));
        else if (lines[i].find("listen"))
            locConfig.setIndex(parseIndex(lines[i]));
        else if (lines[i].find("allowed_methods"))
            locConfig.setAllowedMethods(parseAllowedMethods(line[i]));
        else if (lines[i].find("upload_enable"))
            locConfig.setUploadDir(parseUploadDir(line[i]));
        else if (lines[i].find("autoindex"))
            locConfig.setAutoindex(parseAutoindex(line[i]));
        //decidir como store a informacao do redirect
        //e a informacao do cgi

    }
    return (locConfig);
}


std::string ConfigParser::parseRoot(std::string line) {
    if (!line.compare(0, 4, "root")) {
        size_t pos = line.find_first_of(" \t", 4);
        if (pos == std::string::npos)
            return ("");

        line = line.substr(pos);
        return (line);
    }
    return ("");
}

vector<std::string> ConfigParser::parseIndex(std::string line) {
    vector<std::string> ret;
    if (line.compare(0, 5, "index") != 0)
        return (ret);

    line = line.substr(5);
    line = trimWhitespace(line);
    if (!line.empty() && line.back() == ';')
        line.pop_back();

    std::istringstream iss(line);
    std::string token;
    while (iss >> token)
        ret.push_back(token);

    return (ret);
}

std::string trimComment(std::string line) {
    if (line.empty())
        return (line);
    size_t pos = line.find("#");
    if (pos != std::string::npos) {
        line = line.substr(pos + 1);
    }
    return (line);
}

std::string trimWhitespace(std::string line) {
    if (line.empty())
        return (line);
    while (!line.empty() && std::isspace(line[0])) {
        line = line.substr(1);
    }
    while (!line.empty() && std::isspace(line[line.length() - 1])) {
        line.pop_back();
    }
    return (line);
}

std::string trimLine(std::string line) {
    if (line.empty())
        return (line);
    line = trimComment(line);
    line = trimWhitespace(line);
    return (line);
}
