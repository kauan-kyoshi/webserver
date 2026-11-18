#include "server.hpp"
#include "EchoHandler.hpp"
#include "HttpHandler.hpp"

#include <iostream>

int main(void)
{
    try
    {
		EchoHandler handler;
    	HttpHandler httpHandler(&handler);
    	Server server(&httpHandler);
		server.init();
		server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
