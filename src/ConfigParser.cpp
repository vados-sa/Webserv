#include "ConfigParser.hpp"

Config ConfigParser::parseConfigFile(std::string &filename) {
    Config config;

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Couldn't open config file" << std::endl;
        return (Config());
    }

    std::string line;
    while(std::getline(file, line)) {
        line = trimWhitespace(line);
        if (line == "server {") { //fazer uma funcao pra refinar melhor isso dps
            std::vector<std::string> serverLines; // = collectBlock(file, line);
            ServerConfig server = parseServerBlock(serverLines);
            config.addServer(server);
        }
    }
}

std::vector<std::string> collectBlock(std::ifstream file, std::string line) {
    std::vector<std::string> serverLines;

    serverLines.push_back(line);
    while(line[0] != '}') { //partindo da logica que o closing } do server nunca tem identacao??
        std::getline(file, line);
        serverLines.push_back(line);
    }

    return (serverLines);
}

ServerConfig ConfigParser::parseServerBlock(std::vector<std::string> lines) {
    ServerConfig servConfig;

    for (size_t i = 0; i < lines.size(); i++) {
        lines[i] = trimLine(lines[i]);
        if (lines[i].find("host")) {
            servConfig.setHost(parseHost(lines[i]));
        }
        else if (lines[i].find("listen")) {
            servConfig.setPort(parsePort(lines[i]));
        }
        // else if (lines[i].find("location")) {
        //     //catch the block and parse it

        //     servConfig.addLocation("a");
        // }
    }

    return servConfig;
}

std::string parseHost(std::string line) {
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

int parsePort(std::string line)
{
    if (!line.compare(0, 6, "listen"))
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
    if (line[line.length() - 1] == ';')
        line.pop_back();
    line = trimWhitespace(line);
    return (line);
}
