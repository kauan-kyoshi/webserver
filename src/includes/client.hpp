#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <ctime>
#include <string>

struct Client
{
	int fd;
    std::string bufferRead;
    std::string bufferWrite;
	bool readClose;
	bool recvComplete;
	bool requestComplete;
	bool sendComplete;
	time_t lastActivity;
};

#endif