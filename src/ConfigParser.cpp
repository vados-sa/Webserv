#include "ConfigParser.hpp"
#include "Config.hpp"

Config ConfigParser::parseConfigFile(const std::string &filename) {
    Config config;

	if (filename.size() < 5 || filename.substr(filename.size() - 5) != ".conf") {
		throw std::runtime_error("Invalid file extension, expected .conf");
	}
	std::ifstream file(filename.c_str());
	if (!file.is_open()) {
		throw std::runtime_error("Couldn't open config file");
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
                    throw std::runtime_error("Error: Expected '{' after 'server'");
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
    std::vector<std::string> octets;
    if (tokens.size() < 2)
        throw std::runtime_error("Missing host value in configuration line.\n");

    std::string host = tokens[1];

    //splits address into octets
    for (int i = 0; i < 4; i++) {
        size_t pos = host.find('.');
        if (pos == std::string::npos)
            break;
        octets.push_back(host.substr(0, pos));
        host = host.substr(pos + 1);
    }
    octets.push_back(host);
    if (octets.size() != 4)
        throw std::runtime_error("Invalid host IP address in configuration file\n");

    for (int i = 0; i < 4; i++) {
        //checks if octets empty, which would catch an error if ip address is missing an octet
        if (octets[i].empty())
            throw std::runtime_error("Host address incomplete in configuration file\n");

        //rejects leading 0 ("0" is ok, but "00" ou "01" is not!)
        if (octets[i][0] == '0' && octets[i].length() > 1)
            throw std::runtime_error("Host address contains leading zeros in configuration file\n");

        //checks if the octet is purely digits. whitespace isnt a problem here, octets[i] is already a token
        for (std::string::const_iterator it = octets[i].begin(); it != octets[i].end(); ++it) {
            if (!isdigit(*it))
                throw std::runtime_error("Host address must include digits only\n");
        }

        //converts them to int in order to check the range
        int num = std::atoi(octets[i].c_str());
        if ((num == 0 && octets[i] != "0") || num < 0 || num > 255)
            throw std::runtime_error("Invalid host IP address in configuration file\n");
    }
    return (tokens[1]);
}

int ConfigParser::parsePort(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2) {
		throw std::runtime_error("Error: Missing port value in configuration line.");
    }

    //check if tokens[1] is numeric?
    //tem um range de ports acho
    int portInt = std::atoi(tokens[1].c_str());
    if (portInt == 0 && tokens[1] != "0") {
		throw std::runtime_error(std::string("Error: Invalid error code '") + tokens[1] + "'");
    }
    return (portInt);
}

std::pair<int, std::string> ConfigParser::parseErrorPageLine(const std::vector<std::string> &tokens)
{
    std::pair<int, std::string> entry;
    if (tokens.size() < 3) {
		throw std::runtime_error("Error: Unable to parse error_page");
    }

    int key = std::atoi(tokens[1].c_str());
    if (key == 0 && tokens[1] != "0") {
		throw std::runtime_error(std::string("Error: Invalid error code '") + tokens[1] + "'");
        return (entry);
    }

    entry = std::make_pair(key, tokens[2]);
    return (entry);
}

size_t ConfigParser::parseMaxBodySize(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2)
    {
        throw std::runtime_error("Error: Missing argument for client_max_body_size");
    }

    std::string value = tokens[1];
    size_t multiplier = 1;

    // Use operator[] instead of .back()
    char unit = value[value.size() - 1];

    if (!std::isdigit(unit))
    {
        switch (unit)
        {
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
				throw std::runtime_error("Error: Unknown size unit in client_max_body_size");
        }

        // Use resize() instead of pop_back()
        value.resize(value.size() - 1);
    }

    int number = std::atoi(value.c_str());
    if (number <= 0)
    {
        throw std::runtime_error("Error: Invalid numeric value for client_max_body_size");
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

        if (tokens[0] == "location")
            locConfig.setPath(parsePath(tokens));
        else if (tokens[0] == "root")
            locConfig.setRoot(parseRoot(tokens));
        else if (tokens[0] =="index")
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

std::string ConfigParser::parsePath(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2) {
		throw std::runtime_error("Error: Missing path value in configuration line.");
    }

    if (tokens[0] == "location" && tokens[1][0] == '/') {
        return (tokens[1]);
    }
    return (""); //should we throw an exception here?
}

std::string ConfigParser::parseRoot(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2) {
		throw std::runtime_error("Error: Missing root value in configuration line.");
    }
    return (tokens[1]);
}

std::vector<std::string> ConfigParser::parseIndex(const std::vector<std::string> &tokens)
{
    std::vector<std::string> ret;

    if (tokens.size() < 2) {
        throw std::runtime_error("Error: Missing index value in configuration line.");
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
        throw std::runtime_error("Error: Missing allowed_methods value in configuration line.");
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
        throw std::runtime_error("Error: Missing upload_path value in configuration line.");
    }
    //also what is allow_upload?
    return (tokens[1]);
}

bool ConfigParser::parseAutoindex(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2)
    {
        throw std::runtime_error("Error: Missing autoindex value in configuration line.");
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
        line.resize(line.length() - 1);
    }
    return (line);
}

std::string trimLine(std::string line) {
    if (line.empty())
        return (line);
    line = trimComment(line);
    line = trimWhitespace(line);
   if (!line.empty() && line[line.length() - 1] == ';')
        line.resize(line.length() - 1);
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
