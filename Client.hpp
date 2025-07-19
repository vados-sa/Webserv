#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>

#include <iostream>
#include <string>
#include <ctime>

class Client {
	public:
		enum State {
			CONNECTED,
			SENDING_REQUEST,
			WAITING_RESPONSE,
			IDLE
		};

	private:
		int client_fd;
		std::string request_buffer;
		std::string response_buffer;
		State current_state;
		time_t state_start_time;
		//time_t last_activity;

	public:
		Client(int fd);
		~Client();


		int getFd() const { return client_fd;}
		void appendRequestData(char* buffer, int bytes);
		bool isRequestComplete() const;
		bool isTimedOut(int timeout_seconds) const;

		std::string getRequestData() const {return request_buffer;};
		State getState() const { return Client::current_state; };
		
		void setState(State new_state);
};