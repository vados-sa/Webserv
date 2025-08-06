#include "ConfigParser.hpp"
#include "Config.hpp"

Config ConfigParser::parseConfigFile(const std::string &filename) {
    Config config;

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Couldn't open config file" << std::endl;
        return (Config()); //wrong, we should exit
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(trimLine(line));
    }

    std::vector<std::string> tokens;
    for (size_t i = 0; i < lines.size(); i++) {
        line = trimLine(lines[i]);
        tokens = tokenize(line);
        if (!tokens.empty()) {
            if (tokens[0] == "server") {
                if (tokens[1] != "{") {
                    std::cerr << "Error: Expected '{' after 'server'\n";
                    return (Config()); // wrong, we should exit
                }
                std::vector<std::string> serverLines = collectBlock(lines, i);
                ServerConfig server = parseServerBlock(serverLines);
                config.addServer(server);
            }
        }
    }

    return (config);
}

std::vector<std::string> ConfigParser::collectBlock(std::vector<std::string> lines, size_t i) {
    std::vector<std::string> blockLines;
    int braceCount = 0;

    for (; i < lines.size(); i++) {
        if (lines[i].find('{') != std::string::npos) {
            braceCount ++;
        }
        if (lines[i].find('}') != std::string::npos)
        {
            braceCount--;
        }
        if (braceCount == 0)
            break;

        blockLines.push_back(lines[i]);
    }

    return (blockLines);
}

ServerConfig ConfigParser::parseServerBlock(std::vector<std::string> lines) {
    ServerConfig servConfig;
    std::vector<std::string> tokens;

    for (size_t i = 1; i < lines.size(); i++) {
        std::string line = trimLine(lines[i]);
        tokens = tokenize(line);
        if (!tokens.empty() && tokens[0] == "host")
            servConfig.setHost(parseHost(tokens));
        else if (!tokens.empty() && tokens[0] == "listen")
            servConfig.setPort(parsePort(tokens));
        else if (!tokens.empty() && tokens[0] == "error_page")
            servConfig.setErrorPagesConfig(parseErrorPageLine(tokens));
        else if (!tokens.empty() && tokens[0] == "client_max_body_size")
            servConfig.setMaxBodySize(parseMaxBodySize(tokens));
        else if (!tokens.empty() && tokens[0] == "location")
        {
            std::vector<std::string> locationLines = collectBlock(lines, i);
            LocationConfig loc = parseLocationBlock(locationLines);
            servConfig.addLocation(loc);
        }
    }
    return (servConfig);
}

std::string ConfigParser::parseHost(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2) {
        std::cerr << "Error: Missing host value in configuration line.\n";
        return ("");
    }
    //is this ok or do we throw exceptions?
    return (tokens[1]);
}

int ConfigParser::parsePort(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2) {
        std::cerr << "Error: Missing port value in configuration line.\n";
        return (-1); //acho que n ta bom
    }

    //check if tokens[1] is numeric?
    //tem um range de ports acho
    int portInt = std::atoi(tokens[1].c_str());
    if (portInt == 0 && tokens[1] != "0") {
        std::cerr << "Error: Invalid error code '" << tokens[1] << "'\n";
        return (-1); //n sei se ta bom
    }
    return (portInt);
}

std::pair<int, std::string> ConfigParser::parseErrorPageLine(const std::vector<std::string> &tokens)
{
    std::pair<int, std::string> entry;
    if (tokens.size() < 3) {
        std::cerr << "Error: Unable to parse error_page\n";
        return (entry); // acho que n ta bom
    }

    int key = std::atoi(tokens[1].c_str());
    if (key == 0 && tokens[1] != "0") {
        std::cerr << "Error: Invalid error code '" << tokens[1] << "'\n";
        return (entry);
    }

    entry = std::make_pair(key, tokens[2]);
    return (entry);
}

size_t ConfigParser::parseMaxBodySize(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2)
    {
        std::cerr << "Error: Missing argument for client_max_body_size\n";
        return 0;
    }

    std::string value = tokens[1];
    size_t multiplier = 1;
    char unit = value.back();

    if (!std::isdigit(unit)) {
        switch (unit) {
            case 'k':
            case 'K':
                multiplier = 1024;
                break;
            case 'm':
            case 'M':
                multiplier = 1024 * 1024;
                break;
            case 'g':
            case 'G':
                multiplier = 1024 * 1024 * 1024;
                break;
            default:
                std::cerr << "Error: Unknown size unit in client_max_body_size\n";
                return 0;
        }
        value.pop_back();
    }

    int number = std::atoi(value.c_str());
    if (number <= 0)
    {
        std::cerr << "Error: Invalid numeric value for client_max_body_size\n";
        return 0;
    }

    return static_cast<size_t>(number) * multiplier;
}

LocationConfig ConfigParser::parseLocationBlock(std::vector<std::string> lines)
{
    LocationConfig locConfig;
    std::vector<std::string> tokens;

    for (size_t i = 0; i < lines.size(); i++)
    {
        lines[i] = trimLine(lines[i]);
        tokens = tokenize(lines[i]);

        if (tokens[0] == "root")
            locConfig.setRoot(parseRoot(tokens));
        else if (tokens[0] =="listen")
            locConfig.setIndex(parseIndex(tokens));
        else if (tokens[0] == "allowed_methods")
            locConfig.setAllowedMethods(parseAllowedMethods(tokens));
        else if (tokens[0] == "upload_path")
            locConfig.setUploadDir(parseUploadDir(tokens));
        else if (tokens[0] == "autoindex")
            locConfig.setAutoindex(parseAutoindex(tokens));
        //decidir como store a informacao do redirect
        //e a informacao do cgi

    }
    return (locConfig);
}

std::string ConfigParser::parseRoot(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2) {
        std::cerr << "Error: Missing root value in configuration line.\n";
        return ("");
    }
    // is this ok or do we throw exceptions?
    return (tokens[1]);
}

std::vector<std::string> ConfigParser::parseIndex(const std::vector<std::string> &tokens)
{
    std::vector<std::string> ret;

    if (tokens.size() < 2) {
        std::cerr << "Error: Missing index value in configuration line.\n";
        return (ret);
    }

    for (size_t i = 1; i < tokens.size() && !tokens[i].empty(); i++) {
        ret.push_back(tokens[i]);
    }
    return (ret);
}

std::vector<std::string> ConfigParser::parseAllowedMethods(const std::vector<std::string> &tokens)
{
    std::vector<std::string> ret;

    if (tokens.size() < 2)
    {
        std::cerr << "Error: Missing allowed_methods value in configuration line.\n";
        return (ret);
    }

    for (size_t i = 1; i < tokens.size() && !tokens[i].empty(); i++)
    {
        ret.push_back(tokens[i]);
    }
    return (ret);
}

std::string ConfigParser::parseUploadDir(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2)
    {
        std::cerr << "Error: Missing upload_path value in configuration line.\n";
        return ("");
    }
    //also what is allow_upload?
    // is this ok or do we throw exceptions?
    return (tokens[1]);
}

bool ConfigParser::parseAutoindex(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2)
    {
        std::cerr << "Error: Missing autoindex value in configuration line.\n";
        return ("");
    }
    if (tokens[1] == "on")
        return (true);
    return (false);
}

std::string trimComment(std::string line) {
    if (line.empty())
        return (line);
    size_t pos = line.find("#");
    if (pos != std::string::npos) {
        line = line.substr(0, pos);
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
    if (line.back() == ';')
        line.pop_back();
    return (line);
}

std::vector<std::string> tokenize(const std::string line) {
    std::istringstream iss(line);
    std::string word;
    std::vector<std::string> tokens;

    while (iss >> word) {
        tokens.push_back(word);
    }
    return (tokens);
}