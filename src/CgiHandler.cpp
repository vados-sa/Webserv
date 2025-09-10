#include "Response.hpp"
#include "LocationConfig.hpp"
#include "CgiHandler.hpp"
#include "Utils.hpp"
#include "HttpException.hpp"

#include <cstdlib>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstring>
#include <cerrno>
#include <fcntl.h>

CgiHandler::CgiHandler(const Request &reqObj, const LocationConfig &locConfig)
    : cgiScriptPath("." + locConfig.getRoot() + reqObj.getReqPath()),
      interpreterPath(getInterpreterPath(locConfig.getCgiExtension())),
      error(NO_ERROR),
      status_cgi(false)
{
    std::string msg;

	if (interpreterPath.empty()) {
		error = EXECUTION_FAILED;
        throw HttpException(500, "CGI interpreter not found for extension: " + locConfig.getCgiExtension(), true);
    }

	struct stat buffer;
    util::fileExists(cgiScriptPath, buffer);
    logs(INFO, msg = "CGI Request: " + reqObj.getMethod() + " " + cgiScriptPath);

    std::string transferEncoding = reqObj.findHeader("Transfer-Encoding") ? *reqObj.findHeader("Transfer-Encoding") : "";
	if (transferEncoding == "chunked") {
		std::string rawStream = reqObj.getBody();
        body = util::parseChunkedBody(rawStream);
    } else {
		body = reqObj.getBody();
	}

	std::string contentType = reqObj.findHeader("Content-Type") ? *reqObj.findHeader("Content-Type") : "";
    if (reqObj.getMethod() == "POST" && contentType.find("multipart/form-data") != std::string::npos) {

        struct util::MultipartPart mp = util::parseMultipartBody(contentType, body);
        util::saveFile(locConfig.getUploadDir(), mp.content);
    }

	env["REQUEST_METHOD"] = reqObj.getMethod();
	env["SCRIPT_FILENAME"] = cgiScriptPath;
	env["SERVER_PROTOCOL"] = reqObj.getVersion();
	env["CONTENT_LENGTH"] = body.empty() ? "0" : util::intToString(body.size());

	std::map<std::string, std::string> headers = reqObj.getHeaders();
    for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); ++it) {
        std::string key = it->first;
		for (std::size_t i = 0; i < key.size(); ++i) {
            key[i] = std::toupper(key[i]);
        }
        std::replace(key.begin(), key.end(), '-', '_');
		if (key == "CONTENT_TYPE") {
            env["CONTENT_TYPE"] = it->second;
        } else {
            env["HTTP_" + key] = it->second;
        }
    }
	if (reqObj.getMethod() == "GET") {
		env["QUERY_STRING"] = reqObj.getQueryString().empty() ? "" : reqObj.getQueryString();
	}
}

// Non-blocking CGI execution - starts the CGI process and returns immediately
bool CgiHandler::startCgi(Client &client) {
    int inPipe[2], outPipe[2], errPipe[2];

    // Create pipes
    if (pipe(inPipe) == -1 || pipe(outPipe) == -1 || pipe(errPipe) == -1) {
        error = PIPE_FAILED;
        logs(ERROR, "Failed to create pipes");
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        error = FORK_FAILED;
        logs(ERROR, "Fork failed");
        close(inPipe[0]); close(inPipe[1]);
        close(outPipe[0]); close(outPipe[1]);
        close(errPipe[0]); close(errPipe[1]);
        return false;
    }

    if (pid == 0) { // Child process
        dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);
        dup2(errPipe[1], STDERR_FILENO);

        close(inPipe[1]);
        close(outPipe[0]);
        close(errPipe[0]);

        // Prepare environment variables
        std::vector<char *> envp;
        for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); ++it) {
            std::string entry = it->first + "=" + it->second;
            envp.push_back(strdup(entry.c_str()));
        }
        envp.push_back(NULL);

        // Change to script's directory for relative path file access
        std::string scriptDir = cgiScriptPath.substr(0, cgiScriptPath.find_last_of('/'));
        if (chdir(scriptDir.c_str()) != 0) {
            perror("chdir failed");
            _exit(EXIT_FAILURE);
        }

        // Prepare arguments
        std::string scriptName = cgiScriptPath.substr(cgiScriptPath.find_last_of('/') + 1);
        char *argv_fixed[] = {const_cast<char *>(interpreterPath.c_str()), const_cast<char *>(scriptName.c_str()), NULL};

        // Execute the CGI script
        if (execve(interpreterPath.c_str(), argv_fixed, &envp[0]) == -1) {
            std::ostringstream oss;
            oss << "CGI execve failed: " << strerror(errno);
            std::string msg = oss.str();
            logs(ERROR, msg);
            _exit(EXIT_FAILURE);
        }
    } else { // Parent process
        close(inPipe[0]);
        close(outPipe[1]);
        close(errPipe[1]);

        // Set pipes to non-blocking mode
        if (fcntl(inPipe[1], F_SETFL, O_NONBLOCK) < 0 ||
            fcntl(outPipe[0], F_SETFL, O_NONBLOCK) < 0 ||
            fcntl(errPipe[0], F_SETFL, O_NONBLOCK) < 0) {
            logs(ERROR, "Failed to set CGI pipes to non-blocking mode");
            close(inPipe[1]);
            close(outPipe[0]);
            close(errPipe[0]);
            kill(pid, SIGKILL);
            return false;
        }

        // Store CGI context in client
        Client::CgiContext &cgi = client.getCgiContext();
        cgi.pid = pid;
        cgi.stdin_fd = inPipe[1];
        cgi.stdout_fd = outPipe[0];
        cgi.stderr_fd = errPipe[0];
        cgi.input_buffer = body;
        cgi.input_sent = 0;
        cgi.start_time = time(NULL);
        cgi.stdin_closed = false;
        cgi.stdout_closed = false;
        cgi.stderr_closed = false;

        status_cgi = true; // Successfully started
        return true;
    }
    return false;
}

// Finish CGI execution - process final output and return result
std::string CgiHandler::finishCgi(const Client &client, int exit_status) {
    const Client::CgiContext &cgi = client.getCgiContext();

    if (WIFEXITED(exit_status) && WEXITSTATUS(exit_status) == 0) {
        status_cgi = true;
        logs(INFO, "CGI script executed successfully");
        return cgi.output_buffer;
    } else {
        error = CGI_SCRIPT_FAILED;
        logs(ERROR, "CGI script failed");
        return "";
    }
}

std::string getInterpreterPath(const std::string &cgiExtension) {
    std::string interpreter;
    std::string msg;
    if (cgiExtension == ".py") {
        interpreter = "python3";
    } else if (cgiExtension == ".php") {
        interpreter = "php"; // STILL TO BE TESTED
    } else {
        msg = "Unsupported CGI extension: " + cgiExtension;
        logs(ERROR, msg);
        return "";
    }

    std::string command = "which " + interpreter + " 2>/dev/null";
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) {
        msg = "Failed to execute `which` command for: " + interpreter;
        logs(ERROR, msg);
        return "";
    }

    char buffer[128];
    std::ostringstream result;
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result << buffer;
    }
    pclose(pipe);

    std::string path = result.str();
    if (!path.empty() && path[path.size() - 1] == '\n') {
        path.erase(path.size() - 1);
    }
    if (path.empty()) {
        msg = "Interpreter not found for: " + interpreter;
        logs(ERROR, msg);
    }
    return path;
}
