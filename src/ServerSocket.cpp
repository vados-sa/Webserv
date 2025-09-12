#include "ServerSocket.hpp"

#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include <cstring>
#include <cstdio> 


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
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
		perror("fcntl failed");
		return false;
	}

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
		struct addrinfo addrHints;
		std::memset(&addrHints, 0, sizeof(addrHints));
		addrHints.ai_family = AF_INET;
		addrHints.ai_socktype = SOCK_STREAM;

		struct addrinfo* addrResult = 0;
		int getAddrStatus = getaddrinfo(host.c_str(), 0, &addrHints, &addrResult);
		if (getAddrStatus != 0) {
			perror("getaddrinfo failed");
			return false;
		}

		const struct sockaddr_in* resolveAddr = reinterpret_cast<const struct sockaddr_in*>(addrResult->ai_addr);
		address = *resolveAddr;
		address.sin_port = htons(port);

		freeaddrinfo(addrResult);
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