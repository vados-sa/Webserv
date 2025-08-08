#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <csignal>

#include "ServerConfig.hpp"

class ServerSocket {
	private:
		int fd;
		struct sockaddr_in address;
		int port;
		bool isSetup;
		
		bool createSocket();
		bool configureSocket();
		bool bindSocket(int port);
		bool listenMode();

	public:
		ServerSocket();
		~ServerSocket(); // ver se ta certo msm

		bool setupServerSocket(int port);
};