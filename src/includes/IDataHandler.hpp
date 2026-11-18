#ifndef IDATAHANDLER_HPP
#define IDATAHANDLER_HPP

#include "client.hpp"

class IDataHandler
{
public:
    virtual ~IDataHandler() {}

    virtual void onReceive(int clientFd, Client& client) = 0;
    virtual void onDisconnect(int clientFd, Client& client)
    {
        (void)clientFd;
        (void)client;
    }
    virtual void onSendComplete(int clientFd, Client& client)
    {
        (void)clientFd;
        (void)client;
    }
};

#endif