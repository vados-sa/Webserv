#include "Server.hpp"
#include "Config.hpp"

int main(int ac, char *av[])
{
	if (ac != 2) {
		std::cerr << "Usage: " << av[0] << " <config_file>\n";
		return 1;
	}

    Config config(av[1]); //dps error check ou sei la! fiz aqui pra testar o parse
    std::cout << config << std::endl;

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