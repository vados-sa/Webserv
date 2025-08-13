#include "ConfigParser.hpp"
#include "Config.hpp"

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
        lines.push_back(trimLine(line));
    }

    std::vector<std::string> tokens;
    for (size_t i = 0; i < lines.size(); i++) {
        line = trimLine(lines[i]);
        tokens = tokenize(line);
        if (!tokens.empty()) {
            if (tokens[0] == "server") {
                if (tokens[1] != "{") {
                    errorMessage << fileName << ":" << i + 1 << "Expected '{' after 'server'";
                    throw std::runtime_error(errorMessage.str());
                }
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
        std::string line = trimLine(lines[i]);
        lineNum ++;
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
            lineNum --;
            std::vector<std::string> locationLines = collectBlock(lines, i);
            LocationConfig loc = parseLocationBlock(locationLines);
            servConfig.addLocation(loc);
            i += locationLines.size();
            lineNum ++;
        }
        else if (!tokens.empty() && tokens[0] != "}")
        {
            errorMessage << fileName << ":" << lineNum << "   \"" << tokens[0] << "\" directive is not allowed here\n";
            throw std::runtime_error(errorMessage.str());
        }
    }
    return (servConfig);
}

std::string ConfigParser::parseHost(const std::vector<std::string> &tokens)
{
    std::vector<std::string> octets;
    if (tokens.size() < 2) {
        errorMessage << fileName << ":" << lineNum << "  Missing host value in configuration line.\n";
        throw std::runtime_error(errorMessage.str());
    }

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
    if (octets.size() != 4) {
        errorMessage << fileName << ":" << lineNum << "  Invalid host IP address in configuration file\n";
        throw std::runtime_error(errorMessage.str());
    }
    for (int i = 0; i < 4; i++) {
        //checks if octets empty, which would catch an error if ip address is missing an octet
        if (octets[i].empty()) {
            errorMessage << fileName << ":" << lineNum << "  Host address incomplete in configuration file\n";
            throw std::runtime_error(errorMessage.str());
        }

        //rejects leading 0 ("0" is ok, but "00" ou "01" is not!)
        if (octets[i][0] == '0' && octets[i].length() > 1) {
            errorMessage << fileName << ":" << lineNum << "  Host address contains leading zeros in configuration file\n";
            throw std::runtime_error(errorMessage.str());
        }


        //checks if the octet is purely digits. whitespace isnt a problem here, octets[i] is already a token
        for (std::string::const_iterator it = octets[i].begin(); it != octets[i].end(); ++it) {
            if (!isdigit(*it)) {
                errorMessage << fileName << ":" << lineNum << "  Host address must include digits only\n";
                throw std::runtime_error(errorMessage.str());
            }
        }

        //converts them to int in order to check the range
        int num = std::atoi(octets[i].c_str());
        if ((num == 0 && octets[i] != "0") || num < 0 || num > 255) {
            errorMessage << fileName << ":" << lineNum << "  Invalid host IP address in configuration file\n";
            throw std::runtime_error(errorMessage.str());
        }
    }
    return (tokens[1]);
}

int ConfigParser::parsePort(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2) {
        errorMessage << fileName + ":" << lineNum << "  Missing port value in configuration line.";
		throw std::runtime_error(errorMessage.str());
    }

    if (tokens.size() > 2) {
        errorMessage << fileName + ":" << lineNum << "  Port value contains unexpected spaces or extra tokens";
        throw std::runtime_error(errorMessage.str());
    }

    for (std::string::const_iterator it = tokens[1].begin(); it != tokens[1].end(); ++it)
    {
        if (!isdigit(*it)) {
            errorMessage << fileName + ":" << lineNum << "  Port must include digits only";
            throw std::runtime_error(errorMessage.str());
        }
    }

    int portInt = std::atoi(tokens[1].c_str());
    if (portInt == 0 && tokens[1] != "0") {
        errorMessage << fileName + ":" << lineNum << "  Invalid port number '" << tokens[1] + "'";
        throw std::runtime_error(errorMessage.str());
    }

    if (portInt < 1 || portInt > 65535) {
        errorMessage << fileName << ":" << lineNum << "  Port number '" << tokens[1] << "' out of range";
        throw std::runtime_error(errorMessage.str());
    }
    
    return (portInt);
}

std::pair<int, std::string> ConfigParser::parseErrorPageLine(const std::vector<std::string> &tokens)
{
    std::pair<int, std::string> entry;
    if (tokens.size() < 3) {
        errorMessage << fileName << ":" << lineNum << "  Unable to parse error_page, missing value";
        throw std::runtime_error(errorMessage.str());
    }

    if (tokens.size() > 3)
    {
        errorMessage << fileName << ":" << lineNum << "  error_page contains unexpected spaces or extra tokens";
        throw std::runtime_error(errorMessage.str());
    }

    for (std::string::const_iterator it = tokens[1].begin(); it != tokens[1].end(); ++it)
    {
        if (!isdigit(*it)) {
            errorMessage << fileName << ":" << lineNum << "  Error code must include digits only";
            throw std::runtime_error(errorMessage.str());
        }
    }

    int key = std::atoi(tokens[1].c_str());
    if (key == 0 && tokens[1] != "0")
		throw std::runtime_error(std::string("Invalid error code '") + tokens[1] + "'");

    if (key < 300 || key > 599)
        throw std::runtime_error(std::string("Invalid error code '") + tokens[1] + "': not a valid HTTP error code");

    entry = std::make_pair(key, tokens[2]);
    return (entry);
}

size_t ConfigParser::parseMaxBodySize(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2)
    {
        errorMessage << fileName << ":" << lineNum << "  Error: Missing argument for client_max_body_size";
        throw std::runtime_error(errorMessage.str());
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
				errorMessage << fileName << ":" << lineNum << "  Unknown size unit in client_max_body_size";
                throw std::runtime_error(errorMessage.str());
        }

        // Use resize() instead of pop_back()
        value.resize(value.size() - 1);
    }

    int number = std::atoi(value.c_str());
    if (number <= 0)
    {
        errorMessage << fileName << ":" << lineNum << "  Invalid numeric value for client_max_body_size";
        throw std::runtime_error(errorMessage.str());
    }

    return static_cast<size_t>(number) * multiplier;
}

LocationConfig ConfigParser::parseLocationBlock(std::vector<std::string> lines)
{
    LocationConfig locConfig;
    std::vector<std::string> tokens;

    for (size_t i = 0; i < lines.size(); i++)
    {
        lineNum ++;
        lines[i] = trimLine(lines[i]);
        tokens = tokenize(lines[i]);
        if (!tokens.empty() && tokens[0] == "location")
            locConfig.setPath(parsePath(tokens));
        else if (!tokens.empty() && tokens[0] == "root")
            locConfig.setRoot(parseRoot(tokens));
        else if (!tokens.empty() && tokens[0] == "index")
            locConfig.setIndex(parseIndex(tokens));
        else if (!tokens.empty() && tokens[0] == "allowed_methods")
            locConfig.setAllowedMethods(parseAllowedMethods(tokens));
        else if (!tokens.empty() && tokens[0] == "upload_path")
            locConfig.setUploadDir(parseUploadDir(tokens));
        else if (!tokens.empty() && tokens[0] == "autoindex")
            locConfig.setAutoindex(parseAutoindex(tokens));
        else if (!tokens.empty() && tokens[0] == "allow_upload")
            locConfig.setAllowUpload(parseAllowUpload(tokens));
        else if (tokens[0] == "cgi_extension")
        	locConfig.setCgiExtension(parseCgiExtension(tokens));
        else if (!tokens.empty() && tokens[0] != "}") {
            errorMessage << fileName << ":" << lineNum << "   \"" << tokens[0] << "\" directive is not allowed here\n";
            throw std::runtime_error(errorMessage.str());
        }
    }
    return (locConfig);
}

std::string ConfigParser::parsePath(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2) {
        errorMessage << fileName << ":" << lineNum << "  Missing path value in configuration line.";
        throw std::runtime_error(errorMessage.str());
    }

    if (tokens[0] == "location") {
        if (tokens[1][0] != '/') {
            errorMessage << fileName << ":" << lineNum << "  Path missing starting slash";
            throw std::runtime_error(errorMessage.str());
        }

        if (tokens.size() > 3 && !tokens[2].empty()) {
            errorMessage << fileName << ":" << lineNum << "  Path contains unexpected spaces or extra tokens";
            throw std::runtime_error(errorMessage.str());
        }

        if (!isValidPath(tokens[1])) {
            errorMessage << fileName << ":" << lineNum << "  Path contains illegal characters";
            throw std::runtime_error(errorMessage.str());
        }
        return (tokens[1]);
    }

    errorMessage << fileName << ":" << lineNum << "  Incorrect syntax for location path.";
    throw std::runtime_error(errorMessage.str());
}

std::string ConfigParser::parseRoot(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2) {
		errorMessage << fileName << ":" << lineNum << "  Missing root value in configuration line.";
        throw std::runtime_error(errorMessage.str());
    }

    if (tokens.size() > 2) {
        errorMessage << fileName << ":" << lineNum << "  Root path contains unexpected spaces or extra tokens";
        throw std::runtime_error(errorMessage.str());
    }

    if (tokens[1][0] != '/'){
        errorMessage << fileName << ":" << lineNum << "  Root path must start with '/'";
        throw std::runtime_error(errorMessage.str());
    }

    if (!isValidPath(tokens[1])){
        errorMessage << fileName << ":" << lineNum << "  Root path contains illegal characters";
        throw std::runtime_error(errorMessage.str());
    }

    return (tokens[1]);
}

std::vector<std::string> ConfigParser::parseIndex(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2) {
        errorMessage << fileName << ":" << lineNum << "  Missing index value in configuration line.";
        throw std::runtime_error(errorMessage.str());
    }

    std::vector<std::string> ret;

    for (size_t i = 1; i < tokens.size(); i++) {
        if (tokens[i].empty()) {
            errorMessage << fileName << ":" << lineNum << "  Index value cannot be empty";
            throw std::runtime_error(errorMessage.str());
        }

        if (!isValidPath(tokens[i])){
            errorMessage << fileName << ":" << lineNum << "  Index value contains illegal characters: " << tokens[i];
            throw std::runtime_error(errorMessage.str());
        }

        ret.push_back(tokens[i]);
    }
    return (ret);
}

std::vector<std::string> ConfigParser::parseAllowedMethods(const std::vector<std::string> &tokens)
{
    std::vector<std::string> ret;

    if (tokens.size() < 2){
        errorMessage << fileName << ":" << lineNum << "  Missing allowed_methods value in configuration line.";
        throw std::runtime_error(errorMessage.str());
    }

    std::set<std::string> seen;

    for (size_t i = 1; i < tokens.size(); i++)
    {
        const std::string &method = tokens[i];
        if (method.empty()){
            errorMessage << fileName << ":" << lineNum << "  allowed_methods: empty method value is not allowed";
            throw std::runtime_error(errorMessage.str());
        }

        if (method != "GET" && method != "POST" && method != "DELETE"){
            errorMessage << fileName << ":" << lineNum << "  allowed_methods: unsupported HTTP method '" << method << "'";
            throw std::runtime_error(errorMessage.str());
        }
        if (seen.find(method) == seen.end()) {
            ret.push_back(method);
            seen.insert(method);
        }

        ret.push_back(tokens[i]);
    }
    return (ret);
}

std::string ConfigParser::parseUploadDir(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2) {
        errorMessage << fileName << ":" << lineNum << "  Missing upload_path value in configuration line.";
        throw std::runtime_error(errorMessage.str());
    }

    if (tokens[1][0] != '/') {
        errorMessage << fileName << ":" << lineNum << "  Missing leading slash in upload_path value in configuration file.";
        throw std::runtime_error(errorMessage.str());
    }

    if (tokens[1] == "/") {
        errorMessage << fileName << ":" << lineNum << "  Upload path cannot be root '/'";
        throw std::runtime_error(errorMessage.str());
    }

    return (tokens[1]);
}

bool ConfigParser::parseAutoindex(const std::vector<std::string> &tokens)
{
    if (tokens.size() < 2) {
        errorMessage << fileName << ":" << lineNum << "  Missing autoindex value in configuration line.";
        throw std::runtime_error(errorMessage.str());
    }

    if (tokens.size() > 2) {
        errorMessage << fileName << ":" << lineNum << "  Autoindex contains unexpected extra tokens.";
        throw std::runtime_error(errorMessage.str());
    }

    if (tokens[1] == "on")
        return (true);
    if (tokens[1] == "off")
        return (false);

    errorMessage << fileName << ":" << lineNum << "  Incorrect syntax for autoindex directive.";
    throw std::runtime_error(errorMessage.str());
}

bool ConfigParser::parseAllowUpload(const std::vector<std::string> &tokens) {
    if (tokens.size() < 2) {
        errorMessage << fileName << ":" << lineNum << "  Missing allow_upload value in configuration line.";
        throw std::runtime_error(errorMessage.str());

    }

    if (tokens.size() > 2) {
        errorMessage << fileName << ":" << lineNum << "  Allow_upload contains unexpected extra tokens.";
        throw std::runtime_error(errorMessage.str());
    }

    if (tokens[1] == "on")
        return (true);
    if (tokens[1] == "off")
        return (false);

    errorMessage << fileName << ":" << lineNum << "  Incorrect syntax for allow_upload directive." ;
    throw std::runtime_error(errorMessage.str());
}

std::string ConfigParser::parseCgiExtension(const std::vector<std::string> &tokens)
{
	if (tokens.size() != 2) {
		throw std::runtime_error("Error: Exactly one CGI extension must be specified in the configuration line.");
	}

	const std::string &extension = tokens[1];

	if (extension.empty()) {
		throw std::runtime_error("Error: Empty CGI extension is not allowed.");
	}
	if (!isValidPath(extension)) {
		throw std::runtime_error("Error: Invalid CGI extension '" + extension + "'.");
	}
	return extension;
}

bool isValidPathChar(char c)
{
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '/' || c == '-' || c == '_' || c == '.' || c == '~')
        return (true);
    return (false);
}

bool isValidPath(const std::string &path)
{
    for (std::string::const_iterator it = path.begin(); it != path.end(); ++it)
        if (!isValidPathChar(*it))
            return (false);
    return (true);
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
