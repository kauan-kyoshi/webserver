#ifndef ECHO_HANDLER_HPP
#define ECHO_HANDLER_HPP

#include "HttpHandler.hpp"

class EchoHandler : public IHttpCore
{
public:
    virtual void handle(int clientFd, const HttpRequest& request,
        HttpResponse& response);
};

#endif