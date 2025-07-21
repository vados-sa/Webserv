#include "../include/webserv.hpp"
#include "RequestTests.hpp"
#include "Response.hpp"

int main(void) {

    // std::string req =
    //     "GET /simple.html HTTP/1.1\r\n"
    //     "Host: localhost\r\n"
    //     "\r\n";

    // ---- ex. of post request, one empty value
    std::string req =
        "POST /upload/ HTTP/1.1\r\n"
        "Host: files.example\r\n"
        "Filename: uploadtest.txt\r\n"
        "User-Agent:\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Hello world";

    // req =
    //     "DELETE /resource/123 HTTP/1.1\r\n"
    //     "Host: example.com\r\n"
    //     "User-Agent: curl/8.7.1\r\n"
    //     "Accept: */*\r\n"
    //     "\r\n";

    Request reqObj = Request::parseRequest(req);
    std::cout << "THIS IS THE REQUEST: \n ----------------- \n";
    std::cout << reqObj << "END" << std::endl;

    std::cout << "THIS IS THE RESPONSE STRING: \n ----------------- \n";
    std::string responseStr = buildResponse(reqObj);

    std::cout << responseStr << "END" << std::endl;
}

