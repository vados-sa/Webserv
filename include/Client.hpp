#pragma once

#include "Response.hpp"
#include "Request.hpp"

#include <string>
#include <ctime>

class Client {
	public:
		enum State {
			CONNECTED,
			SENDING_REQUEST,
			WAITING_RESPONSE,
			WAITING_CGI,
			IDLE
		};

		struct CgiContext {
			pid_t pid;
			int stdin_fd;   // write to CGI
			int stdout_fd;  // read from CGI
			int stderr_fd;  // read CGI errors
			std::string input_buffer;   // data to write to CGI
			std::string output_buffer;  // data read from CGI
			std::string error_buffer;   // errors from CGI
			size_t input_sent;          // bytes already sent to CGI
			time_t start_time;
			bool stdin_closed;
			bool stdout_closed;
			bool stderr_closed;
			
			CgiContext() : pid(-1), stdin_fd(-1), stdout_fd(-1), stderr_fd(-1),
							input_sent(0), start_time(0), stdin_closed(false),
							stdout_closed(false), stderr_closed(false) {}
		};

	private:
		int client_fd;
		int server_idx;
		std::string request_buffer;
		State current_state;
		time_t state_start_time;
		std::string response_buffer;
		size_t bytes_sent;
		int port;
		bool keep_alive;
        Response *res;
		CgiContext cgi_context;

    public:
    	Client(int fd, int server_index);
    	~Client();


		void appendRequestData(char* buffer, int bytes);
		void consumeRequestBytes(size_t n) {request_buffer.erase(0, n);};
		bool isTimedOut(int timeout_seconds) const;
		
		int getFd() const { return client_fd; }
		std::string getRequest() const { return request_buffer; }
    	int getServerIndex() const { return server_idx; }
    	std::string getResponse() const { return response_buffer; }
		size_t getBytesSent() const {return bytes_sent;}
		State getState() const { return Client::current_state; }
		int getPort() const { return port; }
		bool getKeepAlive() const { return keep_alive; }
    	Response *getResponseObj() { return res; }
		CgiContext &getCgiContext() { return cgi_context; }
		const CgiContext &getCgiContext() const { return cgi_context; }

    	void setState(State new_state);
		void setKeepAlive(const Request &req);
		void setResponseBuffer(std::string response) { response_buffer = response; }
		void setBytesSent(size_t bytes) { bytes_sent = bytes; }
		void setPort(int p) { port = p; }
		void setKeepAlive(bool set) {keep_alive = set;};
        void setResponseObj(Response *r) { res = r; }
};