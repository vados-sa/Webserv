#include "Client.hpp"

Client::Client(int fd, int server_index) : client_fd(fd), server_idx(server_index),
                                           current_state(CONNECTED), state_start_time(time(NULL)),
                                           bytes_sent(0), port(-1), keep_alive(true), res(NULL) {};
Client::~Client() {};

void Client::appendRequestData(char* buffer, int bytes) {
	request_buffer.append(buffer, bytes);
}

/*
* taking HTTP/1.1 into account.
	missing handling of:
		- Case variations: content-length, CONTENT-LENGTH
		- Transfer-Encoding: chunked
		400 Bad Request -> I think http parser handles it
		- 411 error: length required */
/* bool Client::isRequestComplete() const {
	size_t headers_end = request_buffer.find("\r\n\r\n");
	if (headers_end != std::string::npos) {
		size_t pos = request_buffer.find("Content-Length:");
		if (pos != std::string::npos) {
			pos += 15;
			size_t end_pos = request_buffer.find("\r\n", pos);
			if (end_pos != std::string::npos) {
				std::string length_str = request_buffer.substr(pos, end_pos - pos);
				std::istringstream iss(length_str);
				size_t content_length;
				// comparar com client_max_body_size e reclamar -> bool body_too_big = true/false
				if (iss >> content_length) {
					size_t body_start = headers_end + 4;  // Start of body
					size_t body_length = request_buffer.length() - body_start;
					if (body_length >= content_length) {
						return true;
					} else {
						return false;
					}
				}
			}
		} else {
			return true;
		}
	}
	return false;
} */

bool Client::isTimedOut(int timeout_seconds) const {
	return (time(NULL) - state_start_time) >= timeout_seconds;
}

void Client::setState(State new_state) {
	current_state = new_state;
	state_start_time = time(NULL);  // Reset timer on every state change
}

void Client::setResponseBuffer(std::string response) {
	response_buffer = response;
}

void Client::setBytesSent(size_t bytes) {
	bytes_sent = bytes;
}

void Client::setPort(int p) {
	port = p;
}

void Client::setKeepAlive(std::map<std::string, std::string> headers_) {
	std::map<std::string, std::string>::iterator it = headers_.find("connection");
	if (it != headers_.end() && it->second == "close") {
		keep_alive = false;
	} else
		keep_alive = true;
}