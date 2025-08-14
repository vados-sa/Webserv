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
    for (size_t i = 0; i < servers.size(); i++) {
        ServerSocket socketObj;
        if (!socketObj.setupServerSocket(servers[i].getPort()))
            return false;
        serverSockets.push_back(socketObj);
    }
    return true;
}

bool Config::run() {
	const int server_count = serverSockets.size();
	setupPollfdSet(server_count);
	return pollLoop(server_count);
}

void Config::setupPollfdSet(int server_count) {
    for (int i = 0; i < server_count; i++) {
	    pollfd server_pollfd = {serverSockets[i].getFd(), POLLIN, 0};
	    poll_fds.push_back(server_pollfd);
    }
}

bool Config::pollLoop(int server_count) {
	while (true) {
		int ready = poll(poll_fds.data(), poll_fds.size(), 5000);
		if (ready < 0) {
			cleanup();
			if (errno == EINTR) {
				std::cout << "📡 Signal received, gracefully shutting down..." << std::endl;
				return true ;
			}
			perror("poll failed");
			return false;
		}
		
		for (int i = poll_fds.size() - 1; i >= 0; --i) {
			if ((i < server_count)) {
                if (poll_fds[i].revents & POLLIN) {
                    handleNewConnection(poll_fds[i].fd);
                }
                continue ;
			} else {
				const int client_idx = i - server_count;
				if (clients[client_idx].isTimedOut(60))
                    handleIdleClient(client_idx, i);
				else if (poll_fds[i].revents & POLLIN)
                    handleClientRequest(i, client_idx);
				else if (poll_fds[i].revents & POLLOUT)
                    handleResponse(client_idx, i);
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

void Config::handleIdleClient(int client_idx, int pollfd_idx) {
	std::cout << "⏰ 505: Gateway Timeout\n Client fd("
			  << poll_fds[pollfd_idx].fd << ")/port(" << clients[client_idx].getPort()
			  << ")" << "\n" << std::endl;
	close(poll_fds[pollfd_idx].fd);
	poll_fds.erase(poll_fds.begin() + pollfd_idx);
	clients.erase(clients.begin() + client_idx);
}

// ver como fzr melhor
void Config::handleClientRequest(size_t index, int client_idx) {
	Client& client = clients[client_idx];
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
		clients.erase(clients.begin() + (client_idx));
    } else {
		client.appendRequestData(buffer, bytes);

		if(client.isRequestComplete()) {
			client.setState(Client::WAITING_RESPONSE);
			std::string request = client.getRequest();
			std::cout << "📥 Complete request received:\n" << request << std::endl;

			Request reqObj = Request::parseRequest(request);
			LocationConfig locConfig = findLocationConfig(reqObj.getPath());
			std::string response = buildResponse(reqObj, locConfig);
			client.setKeepAlive(reqObj.getHeaders());
			poll_fds[index].events = POLLOUT;
			client.setResponse(response);
		}
	}
}

void Config::handleResponse(int client_idx, int pollfd_idx) {
	if (sendResponse(pollfd_idx, client_idx) == true) {
		if (clients[client_idx].getKeepAlive() == false) {
			std::cout << "❌ Client disconnected:\nfd - " << poll_fds[pollfd_idx].fd 
			<< "\nport - " << clients[client_idx].getPort() << "\n" << std::endl;

			close(poll_fds[pollfd_idx].fd);
			poll_fds.erase(poll_fds.begin() + pollfd_idx);
			clients.erase(clients.begin() + client_idx);
		} else
            poll_fds[pollfd_idx].events = POLLIN;
    }	
}

bool Config::sendResponse(size_t index, int client_idx) {
	Client& client = clients[client_idx];
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

LocationConfig Config::findLocationConfig(const std::string &path) const {
    for (std::vector<ServerConfig>::const_iterator serverIt = servers.begin(); serverIt != servers.end(); ++serverIt) {
        const std::vector<LocationConfig> &locations = serverIt->getLocations();
        for (std::vector<LocationConfig>::const_iterator locationIt = locations.begin(); locationIt != locations.end(); ++locationIt) {
            if (path.find(locationIt->getPath()) == 0) { // Match the beginning of the path
                return *locationIt;
            }
        }
    }
    throw std::runtime_error("No matching location configuration found for path: " + path);
}

/* bool Config::run() {
    const int server_count = serverSockets.size();
    for (int i = 0; i < server_count; i++) {
	    pollfd server_pollfd = {serverSockets[i].getFd(), POLLIN, 0};
	    poll_fds.push_back(server_pollfd);
    }

	while (true) {
		int ready = poll(poll_fds.data(), poll_fds.size(), 5000);
		if (ready < 0) {
			cleanup();
			if (errno == EINTR) {
                std::cout << "📡 Signal received, gracefully shutting down..." << std::endl;
                break ;
            }
            perror("poll failed");
            return false;
        }

		for (int i = poll_fds.size() - 1; i >= 0; --i) {

            if ((i < server_count)) {
                if (poll_fds[i].revents & POLLIN) {
                    handleNewConnection(poll_fds[i].fd);
                }
                continue ;
			} else {
                const int client_idx = i - server_count;

                if (clients[client_idx].isTimedOut(60)) {
                    std::cout << "⏰ 505: Gateway Timeout\n Client fd("
                    << poll_fds[i].fd << ")/port(" << clients[client_idx].getPort()
                    << ")" << "\n" << std::endl;
    
                    close(poll_fds[i].fd);
                    poll_fds.erase(poll_fds.begin() + i);
                    clients.erase(clients.begin() + client_idx);

                } else if (poll_fds[i].revents & POLLIN) {
                    handleClientRequest(i, client_idx);

                } else if (poll_fds[i].revents & POLLOUT) {
                    if (sendResponse(i, client_idx) == true) {
                        if (clients[client_idx].getKeepAlive() == false) {
							
							std::cout << "❌ Client disconnected:\nfd - " << poll_fds[i].fd 
							<< "\nport - " << clients[client_idx].getPort() << "\n" << std::endl;

                            close(poll_fds[i].fd);
                            poll_fds.erase(poll_fds.begin() + i);
                            clients.erase(clients.begin() + client_idx);

                        } else
                            poll_fds[i].events = POLLIN;
                    }
                }
            }
		}
	}
	return true;
} */