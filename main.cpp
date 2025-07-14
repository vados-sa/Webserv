#include "Server.hpp"

int main(int ac, char *av[])
{
	if (ac != 2) {
		std::cerr << "Wrong number of arguments!\n";
		return 1;
	}

	Server server;
	if (!server.setupServer(8080)) {
		std::cerr << "Failed to setup server.\n";
		return 1; // Server Destructor is called
	};

	server.run();

	return 0;
}