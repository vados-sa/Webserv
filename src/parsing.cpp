#include "../include/webserv.hpp"
#include "../include/Request.hpp"

int main(void) {
    std::string req;

    req = "GET / HTTP/1.1\nHost : localhost : 8080\n User - Agent : curl / 8.7.1 > Accept : */*";

    std::cout << Request::parse(req) << std::endl;
}
