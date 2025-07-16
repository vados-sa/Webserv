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

	std::cout << "✅ Server listening on http://localhost:" << port << "\n";

	return true;
};

bool Server::run() {
	pollfd server_pollfd = {server_fd, POLLIN, 0};
	poll_fds.push_back(server_pollfd);

	while (true) {
		int ready = poll(poll_fds.data(), poll_fds.size(), -1); // check timeout
		if (ready < 0) {
            perror("poll failed");
            return false; // or break?
        }

		for (int i = poll_fds.size() - 1; i >= 0; --i) { // backward iteration
			if (i == 0 && poll_fds[i].revents & POLLIN)
				handleNewConnection();
			else if (poll_fds[i].revents & POLLIN)
				handleClientRequest(i);
		}
	}

	return true;
}

void Server::handleNewConnection() {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
	
	if (client_fd < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			perror("accept failed");
		}
		return ;
	}
	
	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
		perror("fcntl failed for client");
		close(client_fd);
		return ;
	}
	
	clients.emplace_back(Client(client_fd));
	pollfd client_pollfd = {client_fd, POLLIN, 0};
	poll_fds.push_back(client_pollfd);

	clients.back().setConnectionState(true);
	std::cout << "New client connected: FD " << client_fd << std::endl;
}

void Server::handleClientRequest(size_t index) {
	int client_fd = poll_fds[index].fd;
	char buffer[4096];

	int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	if (bytes <= 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return ;
        } else if (bytes == 0) {
            std::cout << "❌ Client disconnected: FD " << client_fd << "\n";
        } else {
            perror("recv failed");
        }
		close(client_fd);
		poll_fds.erase(poll_fds.begin() + index);
		clients.erase(clients.begin() + (index - 1));
		return ;
	}

	clients[index - 1].appendRequestData(buffer, bytes);

	if(clients[index - 1].isRequestComplete()) {
		std::string request = clients[index - 1].getRequestData();
		std::cout << "📥 Complete request received:\n" << request << std::endl;
		
		// For now, hardcoded response - depend on Natalia
		std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 13\r\n\r\nHello World!\n";
		send(client_fd, response.c_str(), response.size(), 0);
		
		close(client_fd);
		poll_fds.erase(poll_fds.begin() + index);
		clients.erase(clients.begin() + (index - 1));
	}

}