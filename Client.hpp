#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>

#include <iostream>
#include <string>

class Client {
	private:
		int client_fd;
		std::string request_buffer;
		std::string response_buffer;
		//bool response_ready;
		bool isConnected;

	public:
		Client(int fd);
		~Client();

		int getFd() const { return client_fd;}
		void appendRequestData(char* buffer, int bytes);
		bool isRequestComplete();

		std::string getRequestData() const {return request_buffer;};
		void setConnectionState(bool state) { isConnected = state; }
};