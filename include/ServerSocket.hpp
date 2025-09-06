#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <csignal>
#include <cstdio> 

#include "ServerConfig.hpp"

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