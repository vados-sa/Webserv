#include <iostream>
#include <string>
#include <cstring>      // for memset
#include <sys/socket.h> // for socket functions
#include <netinet/in.h> // for sockaddr_in
#include <unistd.h>     // for close()

int main()
{
	// 1 - Create the Serve Socket: file descriptor that can do network stuff
	int server_fd = socket(AF_INET, SOCK_STREAM, 0); // use IPv4 addresses, use TCP, choose default protocol TCP
    if (server_fd == -1) {
        perror("socket failed");
        return 1;
    }

	// 2 - Set address info (IPv4, any IP, port 8080)
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address)); // good practice to zero out structs
	
	int port = 8080;
	int addrlen = sizeof(address);
	
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY; // IP address - on any network interface
	address.sin_port = htons(port); // wants to receive messages on port 8080 (network byte order)
	
	// 3 - Bind to a port so the OS know where to route connections
	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
	    perror("bind failed");
	    return 1;
	}

	// 4 - Start Listening for Connections
	if (listen(server_fd, SOMAXCONN) < 0) { // listen() marks the socket 'server_fd' as ready to accept connections;
	    perror("listen failed");			// SOMMAXCONN is backlog - how many clients can wait in line, in this case, system's max allowed
	    return 1;
	}
	std::cout << "✅ Server listening on port " << port << std::endl;

	// 5 - Accept a Client
	sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&client_len); // second socket to talk to that specific client
	if (client_fd < 0) {
	    perror("accept failed");
	    return 1;
	}
	std::cout << "🔌 Client connected!" << std::endl;

	// 6 - Read the client's Request
	char buffer[4096];
	int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0); // Use recv() to read data sent by the client — in this case, an HTTP GET request from curl or telnet.
	if (bytes_read < 0) {
	    perror("recv failed");
	    return 1;
	}
	buffer[bytes_read] = '\0'; // null-terminate
	std::cout << "📥 Received request:\n" << buffer << std::endl;

	// 7 - Send a hardcoded HTTP response
	std::string body = "<html><body><h1>Hello from Webserv</h1></body></html>";
	std::string response =
	    "HTTP/1.1 200 OK\r\n"
	    "Content-Type: text/html\r\n"
	    "Content-Length: " + std::to_string(body.length()) + "\r\n" // must match the body
	    "Connection: close\r\n" //  tells the browser to close the socket after
	    "\r\n" +
	    body; // HTTP requires headers followed by a blank line and then the body

	send(client_fd, response.c_str(), response.length(), 0);
	std::cout << "📤 Response sent!\n";

	// 8 - Close Sockets and Exit
	close(client_fd);
	close(server_fd);
	std::cout << "👋 Connection closed. Server exiting.\n"; // Always clean up sockets. Every file descriptor is a system resource.

	return 0;
}
