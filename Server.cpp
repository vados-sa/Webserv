#include "Server.hpp"

Server::Server() : server_fd(-1), port(0), isSetup(false) {
	std::memset(&address, 0, sizeof(address));
}

Server::~Server() {
	
	if (server_fd != -1) {
		close(server_fd);
	}
}

bool Server::setupServer(int port) {
	this->port = port;
	if (!createSocket()) return false;
	if (!configureSocket()) return false;
	if (!bindSocket(port)) return false;
	if (!listenMode()) return false;
	
	isSetup = true;
	return true;
}

bool Server::createSocket() {
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		perror("socket failed");
		return false;
	}

	return true;
};

bool Server::configureSocket() {
	// non-blocking
	if (fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0) {
		perror("fcntl failed");
		return false;
	}

	// reuse of address
	int option = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
		perror("setsockopt failed");
		return false;
	}

	return true;
};

bool Server::bindSocket(int port) {
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("bind failed");
		return false;
	}

	return true;
};

bool Server::listenMode() {
	if (listen(server_fd, SOMAXCONN) < 0) {
		perror("listen failed");
		return false;
	}

	return true;
};

void Server::run() {
	
}
