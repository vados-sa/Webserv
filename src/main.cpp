#include "Server.hpp"

int main(int ac, char *av[])
{
	if (ac != 2) {
		std::cerr << "Usage: " << av[0] << " <config_file>\n";
		return 1;
	}

	(void)av; // parse config file

	Server server;
	Server::instance = &server;
	if (!server.setupServer(8080)) {
		std::cerr << "Failed to setup server.\n";
		return 1; // Server Destructor is called
	};

	if (!server.run()) {
		std::cerr << "Server failed.\n";
		return 1;
	};

	// transform all if statements in try/catch blocks
	return 0;
}