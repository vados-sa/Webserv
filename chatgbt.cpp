#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <vector>
#include <fcntl.h>

#define PORT 8080
#define MAX_CLIENTS 100

std::string build_http_response() {
    std::string body = "<html><body><h1>Hello from Webserv</h1></body></html>";
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(body.length()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" +
        body;
    return response;
}

int main() {
    // SERVER SETUP
    // create a socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return 1;
    }

    // Set server_fd to non-blocking
    if (fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0) {
        perror("fcntl failed");
        close(server_fd);
        return 1;
    }

    // allows reuse of the address
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        return 1;
    }

    // prepare the server address
    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // bind the socket with the address
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        return 1;
    }

    // put the socket in listening mode
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        close(server_fd);
        return 1;
    }
    std::cout << "✅ Server listening on http://localhost:" << PORT << "\n";

    // POLL MONITORING
    // setup
    std::vector<pollfd> poll_fds;
    pollfd server_pollfd = { server_fd, // ( int fd; ) File descriptor to monitor
                             POLLIN, // ( short events; ) Events we want to watch for (POLLIN, POLLOUT, etc.)
                             0 // ( short revents; ) Events that actually occurred (filled by poll())
                            };
    poll_fds.push_back(server_pollfd); // server_pollfd is at index 0

    char buffer[4096]; // change approach

    // event loop
    while (true) {
        // poll() system call
        int ready = poll(poll_fds.data(), poll_fds.size(), -1);
        if (ready < 0) {
            perror("poll failed");
            break;
        }

        // the event processing loop
        for (size_t i = 0; i < poll_fds.size(); ++i) { 
            // Check each monitored file descriptor. 
            // Only process FDs that have revents set.

            // New connection
            if (poll_fds[i].fd == server_fd /* i == 0 */ && poll_fds[i].revents & POLLIN) { // Order matters: Server socket (index 0) is checked first
                // accepts new connections
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len); // client_fd is a NEW socket for this specific client
                if (client_fd >= 0) {
                    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) { // Make client non-blocking: So recv()/send() won't hang the server
                        perror("fcntl failed");
                        close(client_fd);
                        continue ;
                    }
                    pollfd client_pollfd = { client_fd, POLLIN, 0 }; // tell poll() to monitor this new client
                    poll_fds.push_back(client_pollfd); // add to vector
                    std::cout << "🔌 New client connected: FD " << client_fd << "\n";
                }
            }
            // Client request
            else if (poll_fds[i].revents & POLLIN) {
                int client_fd = poll_fds[i].fd;
                int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                if (bytes < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // Not an error - no data available right now
                        continue ; // Keep connection alive, try again later
                    } else {
                        perror("recv failed"); // Real error - close connection
                        close(client_fd);
                        poll_fds.erase(poll_fds.begin() + i);
                        --i;
                    }
                } else if (bytes == 0) {
                    std::cout << "❌ Client disconnected: FD " << client_fd << "\n";
                    close(client_fd); // *
                    poll_fds.erase(poll_fds.begin() + i);
                    --i;
                } else {
                    buffer[bytes] = '\0';
                    std::cout << "📥 Received request on FD " << client_fd << ":\n" << buffer << "\n";

                    std::string response = build_http_response();
                    send(client_fd, response.c_str(), response.size(), 0);
                    // * same code
                    close(client_fd); // Not keeping connection alive
                    poll_fds.erase(poll_fds.begin() + i);
                    --i;
                }
                /* current "close immediately" approach is fine for the skeleton, but the real webserv 
                    will need sophisticated connection management! */
            }
        }
    }

    close(server_fd);
    return 0;
}
