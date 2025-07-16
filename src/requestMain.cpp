#include "../include/webserv.hpp"
#include "RequestTests.hpp"

int main(void) {

    testParseGET();
    testParsePOSTFilledHeaders();


    // ---- ex. of post request, one empty value
    // req =
    //     "POST /upload HTTP/1.1\r\n"
    //     "Host: files.example\r\n"
    //     "User-Agent:\r\n"
    //     "Content-Type: text/plain\r\n"
    //     "Content-Length: 11\r\n"
    //     "\r\n"
    //     "Hello world";

    // req =
    //     "DELETE /resource/123 HTTP/1.1\r\n"
    //     "Host: example.com\r\n"
    //     "User-Agent: curl/8.7.1\r\n"
    //     "Accept: */*\r\n"
    //     "\r\n";

    // std::cout << Request::parse(req) << std::endl;
}

