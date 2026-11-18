#include "../../includes/server.hpp"
#include <unistd.h>

Server::Server (IDataHandler* handler) 
	:_handler(handler),
    _serverFd(-1),
    _running(false)
{
}

Server::~Server ()
{
	for (std::map<int,Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		close(it->first);
	}
    if(_serverFd != -1)
        close(_serverFd);
}
