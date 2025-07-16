#include "Client.hpp"

Client::Client(int fd) : client_fd(fd) {
	// initialize other attributes
};

Client::~Client() {
	//close(client_fd);
};

void Client::appendRequestData(char* buffer, int bytes) {
	request_buffer.append(buffer, bytes); // Safe - uses exact byte count
}

bool Client::isRequestComplete() {
	return request_buffer.find("\r\n\r\n") != std::string::npos; // npos = no position / not found
	// work for basic cases like GET, use Content-Lenght later after HTTP parser
}
