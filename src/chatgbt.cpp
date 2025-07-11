#include "../include/webserv.hpp"

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
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        return 1;
    }

    // Set server_fd to non-blocking
    fcntl(server_fd, F_SETFL, O_NONBLOCK);

    std::cout << "✅ Server listening on http://localhost:" << PORT << "\n";

    std::vector<pollfd> poll_fds;
    pollfd server_pollfd = { server_fd, POLLIN, 0 };
    poll_fds.push_back(server_pollfd); // colocar como primeiro, pra saber sempre index 0

    char buffer[4096];

    while (true) {
        int ready = poll(poll_fds.data(), poll_fds.size(), -1);
        if (ready < 0) {
            perror("poll failed");
            break;
        }

        for (size_t i = 0; i < poll_fds.size(); ++i) {
            // New connection
            if (poll_fds[i].fd == server_fd && poll_fds[i].revents & POLLIN) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                if (client_fd >= 0) {
                    fcntl(client_fd, F_SETFL, O_NONBLOCK);
                    pollfd client_pollfd = { client_fd, POLLIN, 0 };
                    poll_fds.push_back(client_pollfd);
                    std::cout << "🔌 New client connected: FD " << client_fd << "\n";
                }
            }
            // Client request
            else if (poll_fds[i].revents & POLLIN) {
                int client_fd = poll_fds[i].fd;
                int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                if (bytes <= 0) {
                    std::cout << "❌ Client disconnected: FD " << client_fd << "\n";
                    close(client_fd);
                    poll_fds.erase(poll_fds.begin() + i);
                    --i;
                } else {
                    buffer[bytes] = '\0';
                    std::cout << "📥 Received request on FD " << client_fd << ":\n" << buffer << "\n";

                    std::string response = build_http_response();
                    send(client_fd, response.c_str(), response.size(), 0);

                    close(client_fd); // Not keeping connection alive
                    poll_fds.erase(poll_fds.begin() + i);
                    --i;
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
