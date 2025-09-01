#pragma once

#include <map>
#include "Request.hpp"
#include "LocationConfig.hpp"

class CgiHandler {
	public:
		enum CgiErrorType {
			SCRIPT_NOT_FOUND,
			EXECUTION_FAILED,
			EXECVE_FAILED,
			PIPE_FAILED,
			FORK_FAILED,
			TIMEOUT,
			CGI_SCRIPT_FAILED,
			NO_ERROR
		};
	private:
		std::string cgiScriptPath;
		std::string interpreterPath;
		std::map<std::string, std::string> env;
		std::string body;
		CgiErrorType  error;
		bool status_cgi;
		
		public:
		CgiHandler(const Request &req, const LocationConfig &locConfig);
		void handleFileUpload(const std::string &body, const std::string &uploadDir);
		bool getStatus() const { return status_cgi; };
		CgiErrorType getError() const { return error; };
		std::string run();
		std::string extractBoundary(const std::string &contentType);
		bool processPart(const std::string &part, const std::string &uploadDir);
		std::string extractFileName(const std::string &headers);
		std::string sanitizeFileName(const std::string &fileName);
		bool saveFile(const std::string &filePath, const std::string &fileContent);

};

std::string getInterpreterPath(const std::string &cgiExtension);
std::string decodeChunkedBody(std::istringstream &rawStream);