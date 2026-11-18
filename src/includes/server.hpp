#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <poll.h>
#include <map>

#include "client.hpp"
#include "IDataHandler.hpp"

class Server
{
private:
    IDataHandler* _handler;
    int _serverFd;
    bool _running;

    std::vector<struct pollfd> _pollFds;
    std::map<int, Client> _clients;

    void addClient(int clientFd);
    void removeClient(int clientFd);

public:
    Server(IDataHandler* handler);
    ~Server();

    void init();
    void run();
    void stop();
};

#endif