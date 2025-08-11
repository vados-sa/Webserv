#pragma once

#include "ServerConfig.hpp"
#include "ServerSocket.hpp"
#include "Client.hpp"
#include "Request.hpp"
#include "Response.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include <vector>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

// check necessity of includes

const int MAX_CLIENT = 100;

class Config
{
    private:
        std::vector<ServerConfig> servers;
        void loadFromFile(const std::string &filepath);
        std::vector<ServerSocket> serverSockets;

        int client_count;
        std::vector<Client> clients;
		std::vector<pollfd> poll_fds;

        void handleNewConnection(int server_fd);
		void handleClientRequest(size_t index, int client_idx);
		bool sendResponse(size_t index, int client_idx);
		void cleanup();
    
    public:
        Config(const std::string &filepath);
        Config() {};
        Config(const Config &obj) : servers(obj.servers) {};
        Config &operator=(const Config &other);
        ~Config() {};

        void addServer(ServerConfig &server);
        const std::vector<ServerConfig> &getServers() const { return servers; };
        bool setupServer();
        bool run();
};

std::ostream &operator<<(std::ostream &os, const Config &obj);
