#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include <iostream>

class Server {
	private:
		int server_fd;
		struct sockaddr_in address;
		int port;
		bool isSetup;

		bool createSocket();
		bool configureSocket();
		bool bindSocket(int port);
		bool listenMode();

	public:
		Server();
		~Server();

		bool setupServer(int port);
		void run();
};