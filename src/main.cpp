#include "Config.hpp"
#include <signal.h>

void signal_handler(int sig)
{
    (void)sig;
}

int main(int ac, char *av[])
{
    signal(SIGINT, signal_handler);

    try
    {
        std::string configfile;
        if (ac != 2)
        {
            configfile = "config/default.conf";
        }
        else
        {
            configfile = av[1];
        }

        Config config(configfile);

        if (!config.setupServer())
        {
            throw std::runtime_error("Failed to set server up.");
        };

        if (!config.run())
        {
            throw std::runtime_error("Server failed to run.");
        };
    }
    catch (const std::exception &e)
    {
        std::string msg = e.what();
        logs(ERROR, msg);
        return 1;
    }
    return 0;
}
