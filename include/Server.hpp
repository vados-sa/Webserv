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


#include "Client.hpp"
#include "Request.hpp"
#include "Response.hpp"

class Server {
	private:
		int server_fd;
		struct sockaddr_in address;
		int port;
		bool isSetup;
		std::vector<Client> clients;
		std::vector<pollfd> poll_fds;
		
		bool createSocket();
		bool configureSocket();
		bool bindSocket(int port);
		bool listenMode();
		
		void handleNewConnection();
		void handleClientRequest(size_t index);
		bool sendResponse(size_t index);
		void cleanup();
		static void signalHandler(int signum); // static to be compatible with signal()

	public:
		Server();
		~Server();

		static Server* instance;

		bool setupServer(int port); 
		/* eventually: 
		bool setupServer(const Config& config);  // Multiple ports from config 
		*/
		bool run();
};
