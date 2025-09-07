#include "Logger.hpp"

static std::string timestamp() {
    std::time_t now = std::time(0);
    std::tm *ltm = std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(ltm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void logs(LogLevel level, const std::string &msg) {
    const char* tag = "";
    const char* color = "";
    const char* reset = "\033[0m";
    switch (level) {
        case INFO:   tag = "INFO"; color = "\033[32m"; break; // green
        case ACCESS: tag = "ACCESS"; color = "\033[36m"; break; // cyan
        case ERROR:  tag = "ERROR"; color = "\033[31m"; break; // red
    }

    std::ostream& out = (level == ERROR) ? std::cerr : std::cout;
    out << "[" << color << tag << reset << "]" << " " << timestamp() << " " << color << msg << reset << std::endl;
}