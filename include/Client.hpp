#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>

#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include <map>

#include "Response.hpp"

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
		int server_idx;
		std::string request_buffer;
		State current_state;
		time_t state_start_time;
		std::string response_buffer;
		size_t bytes_sent;
		int port;
		bool keep_alive;
        Response *res;

    public:
    	Client(int fd, int server_index);
    	~Client();


		int getFd() const { return client_fd;}
		void appendRequestData(char* buffer, int bytes);
		//bool isRequestComplete() const;
		void consumeRequestBytes(size_t n) {request_buffer.erase(0, n);};
		bool isTimedOut(int timeout_seconds) const;

		std::string getRequest() const {return request_buffer;}
    	int getServerIndex() const { return server_idx; }
    	std::string getResponse() const {return response_buffer;}

		size_t getBytesSent() const {return bytes_sent;}
		State getState() const { return Client::current_state; }
		int getPort() const {return port;}
		bool getKeepAlive() const {return keep_alive;}
    Response *getResponseObj() { return res; }

    void setState(State new_state);
		void setResponseBuffer(std::string response);
		void setBytesSent(size_t bytes);
		void setPort(int p);
		void setKeepAlive(bool set) {keep_alive = set;};
		void setKeepAlive(std::map<std::string, std::string> headers_);
        void setResponseObj(Response *r) { res = r; }
};