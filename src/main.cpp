#include "Config.hpp"
#include <signal.h>

void signal_handler(int sig) {
	(void)sig;
}

int main(int ac, char *av[])
{
    signal(SIGINT, signal_handler);

    try
    {
        std::string configfile;
        if (ac != 2) {
            configfile = "config/simple.conf";
        } else {
            configfile = av[1];
        }

        Config config(configfile);

        // std::cout << config << std::endl;
        if (!config.setupServer()) {
            throw std::runtime_error("Failed to set server up.");
        };

        if (!config.run()) {
            throw std::runtime_error("Server failed to run.");
        };
    } catch (const std::exception &e) {
        std::cerr << "\033[31m" << e.what() << "\033[0m" << std::endl;
        return 1;
    }
    return 0;
}
