#pragma once

#include <string>
#include <netinet/in.h>

class ServerSocket {
	private:
		int fd;
		struct sockaddr_in address;
		int port;
		std::string host;
		bool isSetup;
		
		bool createSocket();
		bool configureSocket();
		bool bindSocket();
		bool listenMode();

	public:
		ServerSocket();
		~ServerSocket();

		bool setupServerSocket(int port, std::string host);

		int getFd() const {return fd;}
		std::string getHost() const {return host;}
		int getPort() const {return port;}
};