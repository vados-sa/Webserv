#include "Config.hpp"

int main(int ac, char *av[])
{
	try {
		if (ac != 2) {
			throw std::runtime_error("Usage: ./webserv <config_file>");
		}
		// IMPLEMENTAR - Garantir que a extensão do config file é .config
		Config config(av[1]); //dps error check ou sei la! fiz aqui pra testar o parse
		std::cout << config << std::endl;

		if (!config.setupServer()) {
			throw std::runtime_error("Failed to set server up.");
		};

		if (!config.run()) {
			throw std::runtime_error("Server failed to run.");
		};
	}
	catch (const std::exception &e)
	{
		std::cerr << "\033[31mException: " << e.what() << "\033[0m" << std::endl;
		return 1;
	}

	return 0;
}
