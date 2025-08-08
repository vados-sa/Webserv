#include "Config.hpp"
#include "ConfigParser.hpp"

Config::Config(const std::string &filepath) : client_count(0) {
    loadFromFile(filepath);
}

Config &Config::operator=(const Config &other)
{
    if (this != &other)
    {
        this->servers = other.servers;
    }
    return *this;
}

void Config::addServer(ServerConfig &server) {
    servers.push_back(server);
}

void Config::loadFromFile(const std::string &filepath) {
    ConfigParser parser;

    *this = parser.parseConfigFile(filepath);
}

bool Config::setupServer() {
    // for each ServerConfig object, setup a ServerSocket object
    for (size_t i = 0; i < servers.size(); i++) {
        ServerSocket socketObj;
        if (!socketObj.setupServerSocket(servers[i].getPort()))
            return false;
        serverSockets.push_back(socketObj);
    }
    return true;
}

bool Config::run() {
    // for each serverSocket
    for (size_t i = 0; i < serverSockets.size(); i++) {
	    pollfd server_pollfd = {serverSockets[i].getFd(), POLLIN, 0};
	    poll_fds.push_back(server_pollfd);
    }

	while (true) {
		int ready = poll(poll_fds.data(), poll_fds.size(), 5000);
		if (ready < 0) {
            perror("poll failed");
            cleanup();
            return false;
        }

		for (int i = poll_fds.size() - 1; i >= 0; --i) {
			if (i == 0 && poll_fds[i].revents & POLLIN) {
				handleNewConnection(poll_fds[i].fd);
			} else if (i > 0) {
				if (clients[i - 1].isTimedOut(60)) {
					std::cout << "⏰ 505: Gateway Timeout\n Client fd("
							 << poll_fds[i].fd << ")/port(" << clients[i - 1].getPort()
							 << ")" << "\n" << std::endl;
					close(poll_fds[i].fd);
					poll_fds.erase(poll_fds.begin() + i);
					clients.erase(clients.begin() + (i - 1));
				} else if (poll_fds[i].revents & POLLIN) {
					handleClientRequest(i);
				} else if (poll_fds[i].revents & POLLOUT) {
					if (sendResponse(i) == true) {
						if (clients[i - 1].getKeepAlive() == false){
							std::cout << "❌ Client disconnected: " << "\n" << "fd - "
										<< poll_fds[i].fd << "\n" << "port - "
										<< clients[i - 1].getPort() << "\n" << std::endl;
							close(poll_fds[i].fd);
							poll_fds.erase(poll_fds.begin() + i);
							clients.erase(clients.begin() + (i - 1));
						} else
							poll_fds[i].events = POLLIN;
					}
				}
			}
		}
	}

	return true;
}

void Config::handleNewConnection(int server_fd) {
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

	clients.push_back(Client(client_fd));
	pollfd client_pollfd = {client_fd, POLLIN, 0};
	poll_fds.push_back(client_pollfd);

	int port = ntohs(client_addr.sin_port);
	clients.back().setPort(port);
	std::cout << "🔌 New client (fd " << client_fd << ") connected on port: "
				<< port << "\n" << std::endl;
}

// ver como fzr melhor
void Config::handleClientRequest(size_t index) {
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
			client.setKeepAlive(reqObj.getHeaders());
			poll_fds[index].events = POLLOUT;
			client.setResponse(response);
		}
	}
}

bool Config::sendResponse(size_t index) {
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

void Config::cleanup() {
	for (size_t i = 1; i < poll_fds.size(); ++i) {
		close(poll_fds[i].fd);
	}
	clients.clear();
	poll_fds.clear();
}

std::ostream &operator<<(std::ostream &os, const Config &obj) {
    std::vector<ServerConfig> serversVector = obj.getServers();

    for (size_t i = 0; i < serversVector.size(); i ++) {
        os << serversVector[i] << std::endl;
    }
    return os;
}