#include "ServerSocket.hpp"

ServerSocket::ServerSocket() : fd(-1), port(0), isSetup(false) {
	std::memset(&address, 0, sizeof(address));
	//setupServerSocket(); // I can't call it here, because I wouldn't be able to control possible errors
}

ServerSocket::~ServerSocket() {
	/* if (fd != -1) {
		close(fd);
	} */ // it was causing the server socket to close prematurely
}

bool ServerSocket::setupServerSocket(int port) {
	//signal(SIGINT, ServerSocket::signalHandler);
	
	this->port = port;
	if (!createSocket()) return false;
	if (!configureSocket()) return false;
	if (!bindSocket(port)) return false;
	if (!listenMode()) return false;

	isSetup = true;
	return true;
}

bool ServerSocket::createSocket() {
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket failed");
		return false;
	}

	return true;
};

bool ServerSocket::configureSocket() {
	// non-blocking
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
		perror("fcntl failed");
		return false;
	}

	// reuse of address
	int option = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
		perror("setsockopt failed");
		return false;
	}

	return true;
};

bool ServerSocket::bindSocket(int port) {
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (bind(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("bind failed");
		return false;
	}

	return true;
};

bool ServerSocket::listenMode() {
	if (listen(fd, SOMAXCONN) < 0) {
		perror("listen failed");
		return false;
	}

	std::cout << "✅ Server listening on http://localhost:" << port << "\n";

	return true;
};