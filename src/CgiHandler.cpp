#include "Response.hpp"
#include "LocationConfig.hpp"
#include "CgiHandler.hpp"
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

CgiHandler::CgiHandler(const Request &reqObj, const LocationConfig &locConfig) : status_cgi(false) {

	cgiScriptPath = "." + locConfig.getRoot() + reqObj.getReqPath();
	interpreterPath = getInterpreterPath(locConfig.getCgiExtension());
	error = NO_ERROR;
	if (interpreterPath.empty()) {
		error = EXECUTION_FAILED;
		std::cerr << "Interpreter not found for CGI extension: " << locConfig.getCgiExtension() << std::endl;
		return;
	}

	struct stat buffer;
	if (stat(cgiScriptPath.c_str(), &buffer) != 0)
	{
		error = SCRIPT_NOT_FOUND;
		std::cerr << "CGI script not found: " << cgiScriptPath << std::endl;
		return;
	}

	std::cout << "CGI Request: " << reqObj.getMethod() << " " << cgiScriptPath << std::endl; //SHOULD IT BE COUT OR CERR?

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
	env["CONTENT_LENGTH"] = body.empty() ? "0" : std::to_string(body.size());

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
	// for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); ++it) {
	// 	setenv(it->first.c_str(), it->second.c_str(), 1);
	// }

}

void CgiHandler::handleFileUpload(const std::string &body, const std::string &uploadDir) {
    std::string contentType = env["CONTENT_TYPE"];
    std::string boundary;
    if (contentType.find("boundary=") != std::string::npos) {
        boundary = "--" + contentType.substr(contentType.find("boundary=") + 9);
    } else {
        std::cerr << "Boundary not found in Content-Type header" << std::endl;
        return;
    }

    std::string::size_type start = 0, stop;
    while ((stop = body.find(boundary, start)) != std::string::npos) {
        std::string part = body.substr(start, stop - start);
        start = stop + boundary.length();
        if (part.empty() || part == "--") {
            continue;
        }
        std::size_t headerStop = part.find("\r\n\r\n");
        if (headerStop == std::string::npos) {
            std::cerr << "Wrong format of multipart/form-data" << std::endl;
            continue;
        }
        std::string headers = part.substr(0, headerStop);
        std::string fileContent = part.substr(headerStop + 4); // 4 because of "\r\n\r\n"

        std::string fileName;
        std::size_t fileNamePos = headers.find("filename=\"");
        if (fileNamePos != std::string::npos) {
            fileNamePos += 10;
            std::size_t endPos = headers.find("\"", fileNamePos);
            if (endPos != std::string::npos) {
                fileName = headers.substr(fileNamePos, endPos - fileNamePos);
            } else {
                std::cerr << "Wrong format of Content-Disposition header" << std::endl;
                continue;
            }
        } else {
            std::cerr << "Filename not found in Content-Disposition header" << std::endl;
            continue;
        }

        std::string filePath = uploadDir + "/" + fileName;
        std::ofstream outFile(filePath.c_str(), std::ios::binary); // Use .c_str() for C++98 compatibility
        if (!outFile) {
            std::cerr << "Failed to open file: " + filePath << std::endl;
            continue;
        }
        outFile.write(fileContent.c_str(), fileContent.size());
        outFile.close();

        std::cout << "Success to upload file: " + filePath << std::endl;
    }
}

//substituicao da funcao handleFileUpload

// void CgiHandler::handleFileUpload(const std::string &body, const std::string &uploadDir) {
//     // Extract the boundary
//     std::string boundary = extractBoundary(env["CONTENT_TYPE"]);
//     if (boundary.empty()) {
//         std::cerr << "Boundary not found in Content-Type header" << std::endl;
//         return;
//     }

//     // Process each part of the body
//     std::string::size_type start = 0, stop;
//     while ((stop = body.find(boundary, start)) != std::string::npos) {
//         std::string part = body.substr(start, stop - start);
//         start = stop + boundary.length();

//         // Skip empty parts or the final boundary marker
//         if (part.empty() || part == "--") {
//             continue;
//         }

//         // Parse the part and save the file
//         if (!processPart(part, uploadDir)) {
//             std::cerr << "Failed to process part" << std::endl;
//         }
//     }
// }

// // Helper function to extract the boundary from the Content-Type header
// std::string CgiHandler::extractBoundary(const std::string &contentType) {
//     std::size_t boundaryPos = contentType.find("boundary=");
//     if (boundaryPos != std::string::npos) {
//         return "--" + contentType.substr(boundaryPos + 9); // "boundary=" is 9 characters
//     }
//     return "";
// }

// // Helper function to process a single part of the multipart body
// bool CgiHandler::processPart(const std::string &part, const std::string &uploadDir) {
//     // Extract headers and content
//     std::size_t headerStop = part.find("\r\n\r\n");
//     if (headerStop == std::string::npos) {
//         std::cerr << "Malformed multipart/form-data part" << std::endl;
//         return false;
//     }

//     std::string headers = part.substr(0, headerStop);
//     std::string fileContent = part.substr(headerStop + 4); // Skip "\r\n\r\n"

//     // Extract the filename
//     std::string fileName = extractFileName(headers);
//     if (fileName.empty()) {
//         std::cerr << "Filename not found in Content-Disposition header" << std::endl;
//         return false;
//     }

//     // Sanitize the filename
//     fileName = sanitizeFileName(fileName);

//     // Save the file
//     std::string filePath = uploadDir + "/" + fileName;
//     return saveFile(filePath, fileContent);
// }

// // Helper function to extract the filename from the headers
// std::string CgiHandler::extractFileName(const std::string &headers) {
//     std::size_t fileNamePos = headers.find("filename=\"");
//     if (fileNamePos != std::string::npos) {
//         fileNamePos += 10; // Move past "filename=\""
//         std::size_t endPos = headers.find("\"", fileNamePos);
//         if (endPos != std::string::npos) {
//             return headers.substr(fileNamePos, endPos - fileNamePos);
//         }
//     }
//     return "";
// }

// // Helper function to sanitize the filename
// std::string CgiHandler::sanitizeFileName(const std::string &fileName) {
//     std::string sanitized = fileName;
//     // Replace any invalid characters (e.g., "../") to prevent directory traversal
//     std::replace(sanitized.begin(), sanitized.end(), '/', '_');
//     std::replace(sanitized.begin(), sanitized.end(), '\\', '_');
//     return sanitized;
// }

// // Helper function to save the file to disk
// bool CgiHandler::saveFile(const std::string &filePath, const std::string &fileContent) {
//     std::ofstream outFile(filePath.c_str(), std::ios::binary); // Use .c_str() for C++98 compatibility
//     if (!outFile) {
//         std::cerr << "Failed to open file: " + filePath << std::endl;
//         return false;
//     }
//     outFile.write(fileContent.c_str(), fileContent.size());
//     outFile.close();
//     std::cout << "File uploaded successfully: " + filePath << std::endl;
//     return true;
// }


std::string CgiHandler::run() {
    int inPipe[2], outPipe[2], errPipe[2];

    // Create pipes
    if (pipe(inPipe) == -1 || pipe(outPipe) == -1 || pipe(errPipe) == -1) {
        error = PIPE_FAILED;
        std::cerr << "Failed to create pipes" << std::endl;
        return "";
    }

    pid_t pid = fork();
    if (pid < 0) {
        error = FORK_FAILED;
        std::cerr << "Fork failed" << std::endl;
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
            char *env_entry = strdup(entry.c_str());
            envp.push_back(env_entry);
        }
        envp.push_back(NULL);

        // Prepare arguments
        char *argv[] = {const_cast<char *>(interpreterPath.c_str()), const_cast<char *>(cgiScriptPath.c_str()), NULL};

        // Execute the CGI script
        if (execve(interpreterPath.c_str(), argv, &envp[0]) == -1) {
			error = EXECVE_FAILED;
            std::cerr << "CGI execve failed: " << strerror(errno) << std::endl;
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
        int timeout = 5;
        pid_t result;
        time_t startTime = time(NULL);

        do {
            result = waitpid(pid, &timeout_status, WNOHANG);
            if (result == 0) { // Child is still running
                if (time(NULL) - startTime >= timeout) {
                    kill(pid, SIGKILL); // Terminate the child process
                    error = TIMEOUT;
                    std::cerr << "CGI script timed out" << std::endl;
                    return "";
                }
                usleep(100000); // Sleep for 100ms before checking again
            }
        } while (result == 0);

		if (WIFEXITED(timeout_status) && WEXITSTATUS(timeout_status) == 0) {
			char buffer[4096];
			ssize_t bytesRead;
			std::stringstream cgiOutput;
			std::stringstream cgiError;
            status_cgi = true;
			std::cout << "CGI script executed successfully" << std::endl;

			while ((bytesRead = read(outPipe[0], buffer, sizeof(buffer))) > 0) {
				cgiOutput.write(buffer, bytesRead);
			}
			close(outPipe[0]);

			while ((bytesRead = read(errPipe[0], buffer, sizeof(buffer))) > 0) {
				cgiError.write(buffer, bytesRead);
			}
			close(errPipe[0]);
            return cgiOutput.str();
        } else {
			std::stringstream cgiError;
			char buffer[4096];
			ssize_t bytesRead;
            error = CGI_SCRIPT_FAILED;
            std::cerr << "CGI script failed: " << cgiError.str() << std::endl;
			while ((bytesRead = read(errPipe[0], buffer, sizeof(buffer))) > 0) {
				cgiError.write(buffer, bytesRead);
			}
			close(errPipe[0]);
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

std::string getInterpreterPath(const std::string &cgiExtension) {
    std::string interpreter;
    if (cgiExtension == ".py") {
        interpreter = "python3";
    } else if (cgiExtension == ".php") {
        interpreter = "php"; // STILL TO BE TESTED
    } else {
        std::cerr << "Unsupported CGI extension: " << cgiExtension << std::endl;
        return "";
    }

    std::string command = "which " + interpreter + " 2>/dev/null";
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to execute `which` command for: " << interpreter << std::endl;
        return "";
    }

    char buffer[128];
    std::ostringstream result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result << buffer;
    }
    pclose(pipe);

    std::string path = result.str();
    if (!path.empty() && path[path.size() - 1] == '\n') {
        path.erase(path.size() - 1);
    }
    if (path.empty()) {
        std::cerr << "Interpreter not found for: " << interpreter << std::endl;
    }
    return path;
}

// void Response::parseCgiResponse(const std::string &cgiOutput) {
// 	std::istringstream stream(cgiOutput);
// 	std::string line;
// 	bool headersDone = false;
// 	std::ostringstream body;
// 	while (std::getline(stream, line)) {
// 		if (!headersDone) {
// 			if (line == "\r" || line == "" || line == "\n") {
// 				headersDone = true;
// 				continue;
// 			}
// 			size_t colon = line.find(":");
// 			if (colon != std::string::npos) {
// 				std::string key = line.substr(0, colon);
// 				std::string value = line.substr(colon + 1);
// 				value.erase(0, value.find_first_not_of(" \t"));
// 				setHeader(key, value);
// 			}
// 		} else {
// 			body << line << "\n";
// 		}
// 	}
// 	std::string b = body.str();
// 	if (!b.empty() && b[b.size()-1] == '\n') b.erase(b.size()-1);
// 	setBody(b);
// }

