#include "Server.hpp"

Server* Server::instance = NULL;

Server::Server() : server_fd(-1), port(0), isSetup(false), client_count(0) {
	std::memset(&address, 0, sizeof(address));
}

Server::~Server() {
	
	if (server_fd != -1) {
		close(server_fd);
	}
}

bool Server::setupServer(int port) {
	signal(SIGINT, Server::signalHandler);
	
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
		int ready = poll(poll_fds.data(), poll_fds.size(), 5000);
		if (ready < 0) {
            perror("poll failed");
            cleanup();
            return false;
        }

		for (int i = poll_fds.size() - 1; i >= 0; --i) {
			if (i == 0 && poll_fds[i].revents & POLLIN) {
				handleNewConnection();
			} else if (i > 0) {
				if (clients[i - 1].isTimedOut(10)) { // decide final timeout time !
					std::cout << "⏰ Client timed out: FD " << "\n" << "fd - " 
									<< poll_fds[i].fd << "\n" << "port - " 
									<< clients[i - 1].getPort() << "\n" << std::endl;
					close(poll_fds[i].fd);
					poll_fds.erase(poll_fds.begin() + i);
					clients.erase(clients.begin() + (i - 1));
				} else if (poll_fds[i].revents & POLLIN) {
					handleClientRequest(i);
				} else if (poll_fds[i].revents & POLLOUT) {
					if (sendResponse(i) == true) { // later: swtich connection back to read for long lasting connections
						std::cout << "❌ Client disconnected: " << "\n" << "fd - " 
									<< poll_fds[i].fd << "\n" << "port - " 
									<< clients[i - 1].getPort() << "\n" << std::endl;
						close(poll_fds[i].fd);
						poll_fds.erase(poll_fds.begin() + i);
						clients.erase(clients.begin() + (i - 1));
					}
				}
			}
		}
	}

	return true;
}

void Server::handleNewConnection() {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
	
	client_count++;
	if (client_count > MAX_CLIENT) {
		std::cerr << "Refusing client, max reached\n";
		close(client_fd);
		return ;
	}
	
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

	int port = ntohs(client_addr.sin_port);
	clients.back().setPort(port);
	std::cout << "🔌 New client (fd " << client_fd << ") connected on port: " 
				<< port << "\n" << std::endl;
}

// ver como fzr melhor
void Server::handleClientRequest(size_t index) {
	Client& client = clients[index - 1];
	int client_fd = poll_fds[index].fd;
	char buffer[4096];
	
	if (client.getState() == Client::CONNECTED)
		client.setState(Client::SENDING_REQUEST);
	
	// maybe make it cleaner with a separate funciton
	int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	if (bytes < 0) {
            return ;
    } else if (bytes == 0) {
        std::cout << "❌ Client disconnected: " << "\n" << "fd - " << client_fd << "\n" 
								<< "port - " << client.getPort() << "\n" << std::endl;
		close(client_fd);
		poll_fds.erase(poll_fds.begin() + index);
		clients.erase(clients.begin() + (index - 1));
    } else {
		client.appendRequestData(buffer, bytes);

		if(client.isRequestComplete()) {
			client.setState(Client::WAITING_RESPONSE);
			std::string request = client.getRequest();
			std::cout << "📥 Complete request received:\n" << request << std::endl;

			Request reqObj = Request::parseRequest(request);
			std::string response = buildResponse(reqObj);
			poll_fds[index].events = POLLOUT;
			client.setResponse(response);
		}
	}
}

bool Server::sendResponse(size_t index) {
	Client& client = clients[index - 1];
	int client_fd = poll_fds[index].fd;
	std::string response = client.getResponse();
	size_t already_sent= client.getBytesSent();
	size_t remaining = response.size() - already_sent;

	if (remaining == 0) return true;
	
	size_t bytes_sent = send(client_fd, response.c_str() + already_sent, remaining, 0);
	if (bytes_sent < 0) {
		perror("send failed");
		return false;
	}
	client.setBytesSent(bytes_sent);
	return client.getBytesSent() == response.size();

	return false;
}

void Server::cleanup() {
	for (size_t i = 1; i < poll_fds.size(); ++i) {
		close(poll_fds[i].fd);
	}
	clients.clear();
	poll_fds.clear();
}

void Server::signalHandler(int signum) {
	std::cout << "\nCaught signal " << signum << ", cleaning up..." << std::endl;
	if (Server::instance != NULL)
		Server::instance->cleanup();
	exit(signum);
}