#include "Client.hpp"

Client::Client(int fd) : client_fd(fd), current_state(CONNECTED), state_start_time(time(NULL)) {
	// initialize other attributes
};

Client::~Client() {
	//close(client_fd);
};

void Client::appendRequestData(char* buffer, int bytes) {
	request_buffer.append(buffer, bytes); // Safe - uses exact byte count
}

bool Client::isRequestComplete() const {
	return request_buffer.find("\r\n\r\n") != std::string::npos; // npos = no position / not found
	// work for basic cases like GET, use Content-Lenght later after HTTP parser
}

bool Client::isTimedOut(int timeout_seconds) const {
	return (time(NULL) - state_start_time) >= timeout_seconds;
}

void Client::setState(State new_state) { 
	current_state = new_state;
	state_start_time = time(NULL);  // Reset timer on every state change
}
