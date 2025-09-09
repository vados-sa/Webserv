#pragma once

#include "ServerConfig.hpp"
#include "ServerSocket.hpp"
#include "Client.hpp"
#include "LocationConfig.hpp"

#include <poll.h>

#include <vector>
#include <string>
#include <iostream>

const int MAX_CLIENT = 1024;

struct PortState {
    bool anyTaken;
    int  anyServerIdx;
    std::map<std::string, int> ipToServerIdx;

    PortState() : anyTaken(false), anyServerIdx(-1) {}
};

class Config
{
    private:
        std::vector<ServerConfig> servers;
        void loadFromFile(const std::string &filepath);
        std::vector<ServerSocket> serverSockets;

        int client_count;
        std::vector<Client> clients;
		std::vector<pollfd> poll_fds;

        bool validateBindings(std::string &errorMsg) const;
        void setupPollfdSet(int server_count);
        bool pollLoop(int server_count);
        void handleNewConnection(int server_fd, int server_idx);
        void handleIdleClient(int client_idx, int pollfd_idx);
		void handleClientRequest(int pollfd_idx, int client_idx);
        void handleResponse(int client_idx, int pollfd_idx);
		void cleanup();
        
        public:
        Config(const std::string &filepath);
        Config() {};
        Config(const Config &obj) : servers(obj.servers) {};
        Config &operator=(const Config &other);
        ~Config() {};
        
        const std::vector<ServerConfig> &getServers() const { return servers; };
        void addServer(ServerConfig &server);
        bool setupServer();
        bool run();
};

//std::ostream &operator<<(std::ostream &os, const Config &obj); // to print config in main
const LocationConfig *matchLocation(const std::string &path, const ServerConfig &obj);
std::string buildRequestAndResponse(const std::string& raw, const ServerConfig& srv, Request& outReq);
void applyLocationConfig(Request& reqObj, const LocationConfig& loc);
