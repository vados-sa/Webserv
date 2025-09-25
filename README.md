# Webserv

###### A fully functional HTTP/1.1 web server built from scratch in C++.

This project was completed as a systems programming challenge: to design and implement a real web server without external frameworks, handling everything from socket-level I/O to HTTP parsing, file uploads, and CGI execution.

It demonstrates practical knowledge of network programming, HTTP internals, concurrency, and C++ system-level development.

#### 🎯 Project Goals

- ##### Deeply understand HTTP
    Implemented request parsing and response generation fully in-house, covering methods, headers, chunked transfer encoding, and error handling.

- ##### Learn systems-level networking
    Built non-blocking servers using sockets and multiplexing (select/poll), enabling multiple clients concurrently.

- ##### Safe and efficient C++
    Wrote the entire codebase in C++98 with careful resource management, avoiding leaks and undefined behavior.

- ##### Mirror real-world software
    Designed a configuration file system (inspired by Nginx) supporting locations, methods, upload paths, error pages, CGI routes, and request size limits.

- ##### Dynamic content execution
    Added CGI support for Python scripts, bridging server and application logic.

#### Capabilities

- **Static file serving** (`GET`)

- **File upload** with `POST` (`multipart/form-data and raw bodies`)

- **File deletion** with `DELETE`

- **Chunked transfer encoding** (streaming requests)

- **Custom error pages** (auto-generated fallback if not specified)

- **Python CGI execution** for dynamic responses

- **Directory listing** (when enabled in the configuration file)

- **Redirections** (configurable through directives)

- **Configuration file system** (inspired by Nginx)

    - Default: `config/default.conf` (used if no config file specified)
    - Directives: `listen`, `root`, `allowed_methods`, `client_max_body_size`, `error_page`, `autoindex`, `return`, etc.

#### HTTP Compliance

Webserv implements proper status code handling and message mapping for common scenarios, including:

- **2xx Success:** `200 OK`, `201 Created`, `204 No Content`

- **3xx Redirection:** `301 Moved Permanently`, `302 Found`, `303 See Other`, `307 Temporary Redirect`, `308 Permanent Redirect`

- **4xx Client Errors:** `400 Bad Request`, `403 Forbidden`, `404 Not Found`, `405 Method Not Allowed`, `408 Request Timeout`, `411 Length Required`, `414 URI Too Long`

- **5xx Server Errors:** `500 Internal Server Error`

When no custom error page is configured for a given status, Webserv automatically generates an HTML fallback.

#### Examples

- ##### Serve static content
```bash
curl http://localhost:8080/index.html
```

- ##### Upload a file
```bash
curl -X POST -F "file=@example.txt" http://localhost:8080/upload/
```


- ##### Run Python CGI
```bash
curl http://localhost:8080/cgi-bin/post_test.py
```


- #### Trigger a custom error page
```bash
curl http://localhost:8080/not-found
```

- #### Directory listing (if enabled in config)
```bash
curl http://localhost:8080/autoindex
```

- #### Redirection
```bash
curl -i http://localhost:8080/redirect-me
# → HTTP/1.1 301 Moved Permanently → Location: /new
```
