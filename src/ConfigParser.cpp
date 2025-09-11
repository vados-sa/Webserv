#include "ConfigParser.hpp"
#include "Config.hpp"
#include "Utils.hpp"

inline void throwConfigError(const std::string &file, int line, const std::string &msg)
{
    throw std::runtime_error(file + ":" + util::intToString(line) + "  " + msg);
}

inline bool isDirective(const std::vector<std::string> &tokens, const std::string &name)
{
    return !tokens.empty() && tokens[0] == name;
}

Config ConfigParser::parseConfigFile(const std::string &filename) {
    Config config;
    fileName = filename;
    lineNum = 0;

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
        lines.push_back(cleanLine(line));
    }

    std::vector<std::string> tokens;
    for (size_t i = 0; i < lines.size(); i++) {
        line = cleanLine(lines[i]);
        tokens = tokenize(line);
        if (!tokens.empty()) {
            if (tokens[0] == "server") {
                if (tokens[1] != "{")
                    throwConfigError(fileName, i + 1, "Expected '{' after 'server'");
                lineNum = i + 1;
                std::vector<std::string> serverLines = collectBlock(lines, i);
                ServerConfig server = parseServerBlock(serverLines);
                config.addServer(server);
                i += serverLines.size() + 1;
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

    blockLines.push_back(lines[i]);
    return (blockLines);
}

ServerConfig ConfigParser::parseServerBlock(std::vector<std::string> lines) {
    ServerConfig servConfig;

    std::vector<std::string> tokens;

    for (size_t i = 1; i < lines.size(); i++) {
        std::string line = cleanLine(lines[i]);
        lineNum ++;
        tokens = tokenize(line);
        if (isDirective(tokens, "host") && servConfig.getHost().empty())
            servConfig.setHost(parseHost(tokens));
        else if (isDirective(tokens, "listen") && servConfig.getPort() == -1)
            servConfig.setPort(parsePort(tokens));
        else if (isDirective(tokens, "error_page"))
            servConfig.setErrorPagesConfig(parseErrorPageLine(tokens));
        else if (isDirective(tokens, "client_max_body_size") && servConfig.getMaxBodySize() == -1)
            servConfig.setMaxBodySize(parseMaxBodySize(tokens));
        else if (isDirective(tokens, "location"))
        {
            lineNum --;
            std::vector<std::string> locationLines = collectBlock(lines, i);
            LocationConfig loc = parseLocationBlock(locationLines);
            servConfig.addLocation(loc);
            i += locationLines.size();
            lineNum ++;
            if (loc.getMaxBodySize() == -1)
                loc.setMaxBodySize(servConfig.getMaxBodySize());
        }
        else if (!tokens.empty() && tokens[0] != "}")
            throwConfigError(fileName, lineNum, "   \"" + tokens[0] + "\" directive is not allowed here\n");
    }

    if (servConfig.getHost().empty())
        servConfig.setHost("0.0.0.0");
    if (servConfig.getPort() == -1)
        throwConfigError(fileName, lineNum, "Missing port value in configuration file.\n");

    return (servConfig);
}

std::string ConfigParser::parseHost(const std::vector<std::string> &tokens)
{
    std::vector<std::string> octets;
    if (tokens.size() < 2)
        throwConfigError(fileName, lineNum, "  Missing host value in configuration file.\n");

    std::string host = tokens[1];

    for (int i = 0; i < 4; i++) {
        size_t pos = host.find('.');
        if (pos == std::string::npos)
            break;
        octets.push_back(host.substr(0, pos));
        host = host.substr(pos + 1);
    }
    octets.push_back(host);
    if (octets.size() != 4)
        throwConfigError(fileName, lineNum, "  Invalid host IP address in configuration file\n");

    for (int i = 0; i < 4; i++) {
        if (octets[i].empty())
            throwConfigError(fileName, lineNum, "  Host address incomplete in configuration file\n");

        if (octets[i][0] == '0' && octets[i].length() > 1)
            throwConfigError(fileName, lineNum, "  Host address contains leading zeros in configuration file\n");

        for (std::string::const_iterator it = octets[i].begin(); it != octets[i].end(); ++it) {
            if (!isdigit(*it))
                throwConfigError(fileName, lineNum, "  Host address must include digits only\n");
        }

        int num = std::atoi(octets[i].c_str());
        if ((num == 0 && octets[i] != "0") || num < 0 || num > 255)
            throwConfigError(fileName, lineNum, "  Invalid host IP address in configuration file\n");
    }
    return (tokens[1]);
}

int ConfigParser::parsePort(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2)
        throwConfigError(fileName, lineNum, "  Missing port value in configuration file.");

    if (tokens.size() > 2)
        throwConfigError(fileName, lineNum, "  Port value contains unexpected spaces or extra tokens");

    for (std::string::const_iterator it = tokens[1].begin(); it != tokens[1].end(); ++it)
    {
        if (!isdigit(*it)) {
            throwConfigError(fileName, lineNum, "  Port must include digits only");
        }
    }

    int portInt = std::atoi(tokens[1].c_str());
    if (portInt == 0 && tokens[1] != "0")
        throwConfigError(fileName, lineNum, "  Invalid port number '" + tokens[1] + "'");

    if (portInt < 1 || portInt > 65535)
        throwConfigError(fileName, lineNum, "  Port number '" + tokens[1] + "' out of range");

    return (portInt);
}

std::pair<int, std::string> ConfigParser::parseErrorPageLine(const std::vector<std::string> &tokens)
{
    std::pair<int, std::string> entry;
    if (tokens.size() < 3)
        throwConfigError(fileName, lineNum, "  Unable to parse error_page, missing value");

    if (tokens.size() > 3)
        throwConfigError(fileName, lineNum, "  error_page contains unexpected spaces or extra tokens");

    for (std::string::const_iterator it = tokens[1].begin(); it != tokens[1].end(); ++it)
    {
        if (!isdigit(*it))
            throwConfigError(fileName, lineNum, "  Error code must include digits only");
    }

    int key = std::atoi(tokens[1].c_str());
    if (key == 0 && tokens[1] != "0")
        throwConfigError(fileName, lineNum, "Invalid error code '" + tokens[1] + "'");

    if (key < 300 || key > 599)
        throwConfigError(fileName, lineNum,"Invalid error code '" + tokens[1] + "': not a valid HTTP error code");

    entry = std::make_pair(key, tokens[2]);
    return (entry);
}

size_t ConfigParser::parseMaxBodySize(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2)
        throwConfigError(fileName, lineNum, "  Error: Missing argument for client_max_body_size");

    std::string value = tokens[1];
    size_t multiplier = 1;

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
				throwConfigError(fileName, lineNum, "  Unknown size unit in client_max_body_size");
        }

        value.resize(value.size() - 1);
    }

    int number = std::atoi(value.c_str());
    if (number <= 0)
        throwConfigError(fileName, lineNum, "  Invalid numeric value for client_max_body_size");

    return (static_cast<size_t>(number) * multiplier);
}

LocationConfig ConfigParser::parseLocationBlock(std::vector<std::string> lines)
{
    LocationConfig locConfig;
    std::vector<std::string> tokens;

    for (size_t i = 0; i < lines.size(); i++)
    {
        lineNum ++;
        lines[i] = cleanLine(lines[i]);
        tokens = tokenize(lines[i]);
        if (isDirective(tokens, "location"))
            locConfig.setUri(parsePath(tokens));
        else if (isDirective(tokens, "root"))
            locConfig.setRoot(parseRoot(tokens));
        else if (isDirective(tokens, "index"))
            locConfig.setIndex(parseIndex(tokens));
        else if (isDirective(tokens, "allowed_methods"))
            locConfig.setAllowedMethods(parseAllowedMethods(tokens));
        else if (isDirective(tokens, "return"))
            locConfig.setRedirection(parseRedirection(tokens));
        else if (isDirective(tokens, "upload_path"))
            locConfig.setUploadDir(parseUploadDir(tokens));
        else if (isDirective(tokens, "autoindex"))
            locConfig.setAutoindex(parseAutoindex(tokens));
        else if (isDirective(tokens, "cgi_extension"))
        	locConfig.setCgiExtension(parseCgiExtension(tokens));
        else if (isDirective(tokens, "client_max_body_size") && locConfig.getMaxBodySize() == -1)
            locConfig.setMaxBodySize(parseMaxBodySize(tokens));
        else if (!tokens.empty() && tokens[0] != "}") {
            throwConfigError(fileName, lineNum, "   \"" + tokens[0] + "\" directive is not allowed here\n");
        }
    }
    return (locConfig);
}

std::string ConfigParser::parsePath(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2) {
        throwConfigError(fileName, lineNum, "  Missing path value in configuration file.");
    }

    if (tokens[0] == "location") {
        // if (tokens[1][0] != '/') ---- NAO APAGAR AINDA
        //     throwConfigError(fileName, lineNum, "  Path missing starting slash");

        // if (tokens.size() > 3 && !tokens[2].empty())
        //     throwConfigError(fileName, lineNum, "  Path contains unexpected spaces or extra tokens");

        if (!util::isValidPath(tokens[1]))
            throwConfigError(fileName, lineNum, "  Path contains illegal characters");
        return (util::normalizePath(tokens[1]));
    }

    throwConfigError(fileName, lineNum, "  Incorrect syntax for location path.");
    return ("");
}

std::string ConfigParser::parseRoot(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2) {
		throwConfigError(fileName, lineNum, "  Missing root value in configuration file.");
    }

    if (tokens.size() > 2) {
        throwConfigError(fileName, lineNum, "  Root path contains unexpected spaces or extra tokens");
    }

    if (tokens[1][0] != '/'){
        throwConfigError(fileName, lineNum, "  Root path must start with '/'");
    }

    if (!util::isValidPath(tokens[1])){
        throwConfigError(fileName, lineNum, "  Root path contains illegal characters");
    }

    return (tokens[1]);
}

std::vector<std::string> ConfigParser::parseIndex(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2) {
        throwConfigError(fileName, lineNum, "  Missing index value in configuration file.");
    }

    std::vector<std::string> ret;

    for (size_t i = 1; i < tokens.size(); i++) {
        if (tokens[i].empty()) {
            throwConfigError(fileName, lineNum, "  Index value cannot be empty");
        }

        if (!util::isValidPath(tokens[i])){
            throwConfigError(fileName, lineNum, "  Index value contains illegal characters: " + tokens[i]);
        }

        ret.push_back(tokens[i]);
    }
    return (ret);
}

std::vector<std::string> ConfigParser::parseAllowedMethods(const std::vector<std::string> &tokens)
{
    std::vector<std::string> ret;

    if (tokens.size() < 2){
        throwConfigError(fileName, lineNum, "  Missing allowed_methods value in configuration file.");
    }

    std::set<std::string> seen;

    for (size_t i = 1; i < tokens.size(); i++)
    {
        const std::string &method = tokens[i];
        if (method.empty()){
            throwConfigError(fileName, lineNum, "  allowed_methods: empty method value is not allowed");
        }

        if (method != "GET" && method != "POST" && method != "DELETE"){
            throwConfigError(fileName, lineNum, "  allowed_methods: unsupported HTTP method '" + method + "'");
        }
        if (seen.find(method) == seen.end()) {
            ret.push_back(method);
            seen.insert(method);
        }
    }
    return (ret);
}

std::pair<int, std::string> ConfigParser::parseRedirection(const std::vector<std::string> &tokens)
{

    std::pair<int, std::string> entry;
    if (tokens.size() < 3) {
        throwConfigError(fileName, lineNum, "  Unable to parse return, missing value");
    }

    if (tokens[2] == "\"\"" || tokens[2].empty())
        throw std::runtime_error("Redirection target must not be empty");

    if (tokens.size() > 3)
    {
        throwConfigError(fileName, lineNum, "  return contains unexpected spaces or extra tokens");
    }

    for (std::string::const_iterator it = tokens[1].begin(); it != tokens[1].end(); ++it)
    {
        if (!isdigit(*it)) {
            throwConfigError(fileName, lineNum, "  Redirection code must include digits only");
        }
    }

    int key = std::atoi(tokens[1].c_str());
    if (key == 0 && tokens[1] != "0")
		throwConfigError(fileName, lineNum, "Invalid redirection code '" + tokens[1] + "'");

    if ((key < 301 || key > 302) && (key < 307 || key > 308))
        throwConfigError(fileName, lineNum, "Invalid redirection code '" + tokens[1] + "': not a valid HTTP redirection code");

    entry = std::make_pair(key, tokens[2]);
    return (entry);
}

std::string ConfigParser::parseUploadDir(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2) {
        throwConfigError(fileName, lineNum, "  Missing upload_path value in configuration file.");
    }

    if (tokens[1] == "/") {
        throwConfigError(fileName, lineNum, "  Upload path cannot be root '/'");
    }

    return (tokens[1]);
}

bool ConfigParser::parseAutoindex(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2) {
        throwConfigError(fileName, lineNum, "  Missing autoindex value in configuration file.");
    }

    if (tokens.size() > 2) {
        throwConfigError(fileName, lineNum, "  Autoindex contains unexpected extra tokens.");
    }

    if (tokens[1] == "on")
        return (true);
    if (tokens[1] == "off")
        return (false);

    throwConfigError(fileName, lineNum, "  Incorrect syntax for autoindex directive.");
    return (false);
}

std::string ConfigParser::parseCgiExtension(const std::vector<std::string> &tokens)
{
	if (tokens.size() != 2) {
		throwConfigError(fileName, lineNum, "Error: Exactly one CGI extension must be specified in the configuration file.");
	}

	const std::string &extension = tokens[1];

	if (extension.empty()) {
		throwConfigError(fileName, lineNum, "Error: Empty CGI extension is not allowed.");
	}
	if (extension[0] != '.') {
        throwConfigError(fileName, lineNum, "Error: CGI extension must start with a '.' character.");
    }
	if (!util::isValidPath(extension)) {
		throwConfigError(fileName, lineNum, "Error: Invalid CGI extension '" + extension + "'.");
	}
	return extension;
}

std::string cleanLine(std::string line)
{
    if (line.empty())
        return line;
    size_t pos = line.find("#");
    if (pos != std::string::npos)
        line = line.substr(0, pos);

    while (!line.empty() && std::isspace(line[0]))
        line = line.substr(1);
    while (!line.empty() && std::isspace(line[line.size() - 1]))
        line.resize(line.size() - 1);

    if (!line.empty() && line[line.size() - 1] == ';')
        line.resize(line.size() - 1);
    return line;
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
