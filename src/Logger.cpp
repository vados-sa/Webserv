#include "Logger.hpp"
#include <iostream>
#include <ctime>

static std::string timestamp() {
    std::time_t now = std::time(0);
    std::tm *ltm = std::localtime(&now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ltm);
    return std::string(buf);
}

void logs(LogLevel level, const std::string &msg) {
    const char* tag = "";
    const char* color = "";
    const char* reset = "\033[0m";
    switch (level) {
        case INFO:   tag = "INFO"; color = "\033[32m"; break; // green
        case ERROR:  tag = "ERROR"; color = "\033[31m"; break; // red
    }

    std::ostream& out = (level == ERROR) ? std::cerr : std::cout;
    out << "[" << color << tag << reset << "]" << " " << timestamp() << " " << color << msg << reset << std::endl;
}