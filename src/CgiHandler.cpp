#include "Response.hpp"
#include "LocationConfig.hpp"
#include "CgiHandler.hpp"
#include "Utils.hpp"

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

CgiHandler::CgiHandler(const Request &reqObj, const LocationConfig &locConfig) : status_cgi(false) {

	cgiScriptPath = "." + locConfig.getRoot() + reqObj.getReqPath();
	interpreterPath = getInterpreterPath(locConfig.getCgiExtension());
	error = NO_ERROR;
    std::string msg;
	if (interpreterPath.empty()) {
		error = EXECUTION_FAILED;
        msg = "Interpreter not found for CGI extension: " + locConfig.getCgiExtension();
        logs(ERROR, msg);
		return;
	}

	struct stat buffer;
	if (stat(cgiScriptPath.c_str(), &buffer) != 0)
	{
		error = SCRIPT_NOT_FOUND;
        msg = "CGI script not found: " + cgiScriptPath;
        logs(ERROR, msg);
		return;
	}

    msg = "CGI Request: " + reqObj.getMethod() + " " + cgiScriptPath;
    logs(INFO, msg);

	std::string transferEncoding = reqObj.findHeader("Transfer-Encoding") ? *reqObj.findHeader("Transfer-Encoding") : "";
	if (transferEncoding == "chunked") {
		std::istringstream rawStream(reqObj.getBody());
		body = decodeChunkedBody(rawStream);
	} else {
		body = reqObj.getBody();
	}

	std::string contentType = reqObj.findHeader("Content-Type") ? *reqObj.findHeader("Content-Type") : "";
    if (reqObj.getMethod() == "POST" && contentType.find("multipart/form-data") != std::string::npos) {
        std::string uploadDir = locConfig.getUploadDir();
        handleFileUpload(body, uploadDir);
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

// void CgiHandler::handleFileUpload(const std::string &body, const std::string &uploadDir) {
//     std::string contentType = env["CONTENT_TYPE"];
//     std::string boundary;
//     if (contentType.find("boundary=") != std::string::npos) {
//         boundary = "--" + contentType.substr(contentType.find("boundary=") + 9);
//     } else {
//         std::cerr << "Boundary not found in Content-Type header" << std::endl;
//         return;
//     }

//     std::string::size_type start = 0, stop;
//     while ((stop = body.find(boundary, start)) != std::string::npos) {
//         std::string part = body.substr(start, stop - start);
//         start = stop + boundary.length();
//         if (part.empty() || part == "--") {
//             continue;
//         }
//         std::size_t headerStop = part.find("\r\n\r\n");
//         if (headerStop == std::string::npos) {
//             std::cerr << "Wrong format of multipart/form-data" << std::endl;
//             continue;
//         }
//         std::string headers = part.substr(0, headerStop);
//         std::string fileContent = part.substr(headerStop + 4); // 4 because of "\r\n\r\n"

//         std::string fileName;
//         std::size_t fileNamePos = headers.find("filename=\"");
//         if (fileNamePos != std::string::npos) {
//             fileNamePos += 10;
//             std::size_t endPos = headers.find("\"", fileNamePos);
//             if (endPos != std::string::npos) {
//                 fileName = headers.substr(fileNamePos, endPos - fileNamePos);
//             } else {
//                 std::cerr << "Wrong format of Content-Disposition header" << std::endl;
//                 continue;
//             }
//         } else {
//             std::cerr << "Filename not found in Content-Disposition header" << std::endl;
//             continue;
//         }

//         std::string filePath = uploadDir + "/" + fileName;
//         std::ofstream outFile(filePath.c_str(), std::ios::binary); // Use .c_str() for C++98 compatibility
//         if (!outFile) {
//             std::cerr << "Failed to open file: " + filePath << std::endl;
//             continue;
//         }
//         outFile.write(fileContent.c_str(), fileContent.size());
//         outFile.close();

//         std::cout << "Success to upload file: " + filePath << std::endl;
//     }
// }

void CgiHandler::handleFileUpload(const std::string &body, const std::string &uploadDir) {
    // Extract the boundary
    std::string boundary = extractBoundary(env["CONTENT_TYPE"]);
    if (boundary.empty()) {
        logs(ERROR, "Boundary not found in Content-Type header");
        return;
    }

    // Process each part of the body
    std::string::size_type start = 0, stop;
    while ((stop = body.find(boundary, start)) != std::string::npos) {
        std::string part = body.substr(start, stop - start);
        start = stop + boundary.length();

        // Skip empty parts or the final boundary marker
        if (part.empty() || part == "--") {
            continue;
        }

        // Parse the part and save the file
        if (!processPart(part, uploadDir)) {
            logs(ERROR, "Failed to process part");
        }
    }
}

// Helper function to extract the boundary from the Content-Type header
std::string CgiHandler::extractBoundary(const std::string &contentType) {
    std::size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos != std::string::npos) {
        return "--" + contentType.substr(boundaryPos + 9); // "boundary=" is 9 characters
    }
    return "";
}

// Helper function to process a single part of the multipart body
bool CgiHandler::processPart(const std::string &part, const std::string &uploadDir) {
    // Extract headers and content
    std::size_t headerStop = part.find("\r\n\r\n");
    if (headerStop == std::string::npos) {
        logs(ERROR, "Malformed multipart/form-data part");
        return false;
    }

    std::string headers = part.substr(0, headerStop);
    std::string fileContent = part.substr(headerStop + 4); // Skip "\r\n\r\n"

    // Extract the filename
    std::string fileName = extractFileName(headers);
    if (fileName.empty()) {
        logs(ERROR, "Filename not found in Content-Disposition header");
        return false;
    }

    // Sanitize the filename
    fileName = sanitizeFileName(fileName);

    // Save the file
    std::string filePath = uploadDir + "/" + fileName;
    return saveFile(filePath, fileContent);
}

// Helper function to extract the filename from the headers
std::string CgiHandler::extractFileName(const std::string &headers) {
    std::size_t fileNamePos = headers.find("filename=\"");
    if (fileNamePos != std::string::npos) {
        fileNamePos += 10; // Move past "filename=\""
        std::size_t endPos = headers.find("\"", fileNamePos);
        if (endPos != std::string::npos) {
            return headers.substr(fileNamePos, endPos - fileNamePos);
        }
    }
    return "";
}

// Helper function to sanitize the filename
std::string CgiHandler::sanitizeFileName(const std::string &fileName) {
    std::string sanitized = fileName;
    // Replace any invalid characters (e.g., "../") to prevent directory traversal
    std::replace(sanitized.begin(), sanitized.end(), '/', '_');
    std::replace(sanitized.begin(), sanitized.end(), '\\', '_');
    return sanitized;
}

// Helper function to save the file to disk
bool CgiHandler::saveFile(const std::string &filePath, const std::string &fileContent) {
    std::ofstream outFile(filePath.c_str(), std::ios::binary); // Use .c_str() for C++98 compatibility
    std::string msg;
    if (!outFile) {
        msg = "File uploaded successfully: " + filePath;
        logs(ERROR, msg);
        return false;
    }
    outFile.write(fileContent.c_str(), fileContent.size());
    outFile.close();
    msg = "File uploaded successfully: " + filePath;
    logs(INFO, msg);
    return true;
}


std::string CgiHandler::run() {
    int inPipe[2], outPipe[2], errPipe[2];

    // Create pipes
    if (pipe(inPipe) == -1 || pipe(outPipe) == -1 || pipe(errPipe) == -1) {
        error = PIPE_FAILED;
        logs(ERROR, "Failed to create pipes");
        return "";
    }

    pid_t pid = fork();
    if (pid < 0) {
        error = FORK_FAILED;
        logs(ERROR, "Fork failed");
        return "";
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
        // After chdir, we need just the script filename, not the full path
        std::string scriptName = cgiScriptPath.substr(cgiScriptPath.find_last_of('/') + 1);
        char *argv_fixed[] = {const_cast<char *>(interpreterPath.c_str()), const_cast<char *>(scriptName.c_str()), NULL};

        // Execute the CGI script
        if (execve(interpreterPath.c_str(), argv_fixed, &envp[0]) == -1) {
			error = EXECVE_FAILED;
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

        // Write the request body to the CGI script's stdin
        if (!body.empty()) {
            write(inPipe[1], body.c_str(), body.size());
        }
        close(inPipe[1]);

        // Timeout mechanism
        int timeout_status;
        time_t startTime = time(NULL);
        while (waitpid(pid, &timeout_status, WNOHANG) == 0) {
            if (time(NULL) - startTime > 5) {
                kill(pid, SIGKILL);
                waitpid(pid, &timeout_status, 0);
                error = TIMEOUT;
                close(outPipe[0]);
                close(errPipe[0]);
                logs(ERROR, "CGI script timed out");
                return "";
            }
            usleep(100000);
        }

        // Read stdout and stderr
        std::stringstream cgiOutput;
        char buffer[4096];
        ssize_t bytesRead;

        while ((bytesRead = read(outPipe[0], buffer, sizeof(buffer))) > 0) {
            cgiOutput.write(buffer, bytesRead);
        }
        close(outPipe[0]);

        std::stringstream cgiError;
        while ((bytesRead = read(errPipe[0], buffer, sizeof(buffer))) > 0) {
            cgiError.write(buffer, bytesRead);
        }
        close(errPipe[0]);

        // Check the exit status of the child process
        if (WIFEXITED(timeout_status) && WEXITSTATUS(timeout_status) == 0) {
            status_cgi = true;
            logs(INFO, "CGI script executed successfully");
            return cgiOutput.str();
        } else {
            error = CGI_SCRIPT_FAILED;
            logs(ERROR, "CGI script failed");
            return "";
        }
    }
    return "";
}


std::string decodeChunkedBody(std::istringstream &rawStream) {
    std::string decodedBody;
    std::string line;

    while (std::getline(rawStream, line)) {
        // Remove trailing \r (C++98-compatible way)
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }

        // Parse the chunk size (C++98-compatible way using std::stringstream)
        std::stringstream chunkSizeStream(line);
        size_t chunkSize;
        chunkSizeStream >> std::hex >> chunkSize;

        if (chunkSize == 0) {
            break; // End of chunks
        }

        // Read the chunk data
        char *buffer = new char[chunkSize];
        rawStream.read(buffer, chunkSize);
        decodedBody.append(buffer, chunkSize);
        delete[] buffer;

        // Skip the trailing \r\n after each chunk
        rawStream.ignore(2);
    }

    return decodedBody;
}

//check this version of decodeChunkedBody
// std::string decodeChunkedBody(std::istream &rawStream) {
//     std::string decodedBody;
//     std::string line;

//     while (std::getline(rawStream, line)) {
//         // Remove trailing \r (C++98-compatible way)
//         if (!line.empty() && line[line.size() - 1] == '\r') {
//             line.erase(line.size() - 1);
//         }

//         // Parse the chunk size (C++98-compatible way using std::stringstream)
//         std::istringstream chunkSizeStream(line);
//         size_t chunkSize;
//         chunkSizeStream >> std::hex >> chunkSize;

//         if (chunkSize == 0) {
//             break; // End of chunks
//         }

// 		std::string chunkData(chunkSize, '\0');
// 		rawStream.read(&chunkData[0], chunkSize);
// 		decodedBody += chunkData;

// 		char cr, lf;
// 		rawStream.get(cr);
// 		rawStream.get(lf);
// 		if (cr != '\r' || lf != '\n') {
// 			std::cerr << "Malformed chunked encoding" << std::endl;
// 			break;
// 		}
//     }

//     return decodedBody;
// }

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
