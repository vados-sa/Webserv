#include "ServerSocket.hpp"

ServerSocket::ServerSocket() : fd(-1), port(0), isSetup(false) {
	std::memset(&address, 0, sizeof(address));
}

ServerSocket::~ServerSocket() {}

bool ServerSocket::setupServerSocket(int port, std::string host) {
	
	this->port = port;
	this->host = host;
	if (!createSocket()) return false;
	if (!configureSocket()) return false;
	if (!bindSocket()) return false;
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

bool ServerSocket::bindSocket() {
	
	if (host.empty() || host == "0.0.0.0" || host == "*") {
		address.sin_family = AF_INET;
		address.sin_port = htons(port);
		address.sin_addr.s_addr = htonl(INADDR_ANY);
	} else {
		struct addrinfo hints;
		std::memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;

		struct addrinfo* res = 0;
		int rc = getaddrinfo(host.c_str(), 0, &hints, &res);
		if (rc != 0) {
			perror("getaddrinfo failed");
			return false;
		}

		const struct sockaddr_in* got = reinterpret_cast<const struct sockaddr_in*>(res->ai_addr);
		address = *got;
		address.sin_port = htons(port);

		freeaddrinfo(res);
	}

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

	return true;
};