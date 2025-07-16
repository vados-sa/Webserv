#include "RequestTests.hpp"

void testParseGET()
{
    std::string req =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    Request r = Request::parse(req);
    assert(r.getMethod() == "GET");
    assert(r.getPath() == "/");
    assert(r.getVersion() == "HTTP/1.1");
    assert(r.getHeaders()["Host"] == "localhost");
    std::cout << "testParseGET passed\n";
}

void testParsePOSTFilledHeaders()
{
    std::string req =
        "POST /submit HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 27\r\n"
        "\r\n"
        "username=admin&password=123";

    Request r = Request::parse(req);
    assert(r.getMethod() == "POST");
    assert(r.getPath() == "/submit");
    assert(r.getVersion() == "HTTP/1.1");
    assert(r.getHeaders()["Content-Type"] == "application/x-www-form-urlencoded");
    assert(r.getHeaders()["Content-Length"] == "27");
    assert(r.getBody() == "username=admin&password=123");
    std::cout << "testParsePOSTFilledHeaders passed\n";
}
