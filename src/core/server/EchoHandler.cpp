#include "EchoHandler.hpp"

void EchoHandler::handle(int clientFd, const HttpRequest& request,
    HttpResponse& response)
{
    (void)clientFd;
    response.headers["Content-Type"] = "text/plain";
    response.body = request.body;
}