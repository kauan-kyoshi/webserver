#include "../../includes/server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <signal.h>

static const int DEFAULT_BACKLOG = 128;

void Server::init (){
    struct sockaddr_in address;
    int option = 1;

    signal(SIGPIPE, SIG_IGN);

    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd == -1)
        throw std::runtime_error(strerror(errno));

    int flags = fcntl(_serverFd, F_GETFL, 0);
    if (flags == -1)
        throw std:: runtime_error(strerror(errno));    
    if (fcntl(_serverFd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std:: runtime_error(strerror(errno));

    if (setsockopt(
        _serverFd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &option,
        sizeof(option)
    ) == -1)
        throw std::runtime_error(strerror(errno));

    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    for(size_t i = 0; i < sizeof(address.sin_zero); i++)
        address.sin_zero[i] = 0;
    if (bind(
        _serverFd, 
        reinterpret_cast<struct sockaddr *>(&address), 
        sizeof(address)
    ) == -1)
        throw std::runtime_error(strerror(errno));

    if (listen(_serverFd, DEFAULT_BACKLOG) == -1)
        throw std::runtime_error(strerror(errno));
}