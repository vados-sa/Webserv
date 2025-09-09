#include "Client.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>

#include <iostream>
#include <sstream>
#include <map>

Client::Client(int fd, int server_index) : client_fd(fd), server_idx(server_index),
                                           current_state(CONNECTED), state_start_time(time(NULL)),
                                           bytes_sent(0), port(-1), keep_alive(true), res(NULL) {};
Client::~Client() {};

void Client::appendRequestData(char* buffer, int bytes) {
	request_buffer.append(buffer, bytes);
}

bool Client::isTimedOut(int timeout_seconds) const {
	return (time(NULL) - state_start_time) >= timeout_seconds;
}

void Client::setState(State new_state) {
	current_state = new_state;
	state_start_time = time(NULL);
}

void Client::setKeepAlive(const Request &req)
{
	const std::string version = req.getVersion();
    std::string connection = "";
    if (req.findHeader("Connection")) {
        connection = *req.findHeader("Connection");
        for (size_t i = 0; i < connection.size(); ++i)
            connection[i] = (char)std::tolower(connection[i]);
    }

    bool want_close;
    if (version == "HTTP/1.1")
		want_close = (connection == "close");
    else
		want_close = (connection != "keep-alive");
	keep_alive = !want_close;
}
