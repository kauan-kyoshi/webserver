#include "../../includes/server.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <set>

static const int POLL_TIMEOUT_MS = 1000;
static const time_t CLIENT_TIMEOUT = 30;

void Server::addClient(int clientFd)
{
    struct pollfd clientPoll;

    clientPoll.fd = clientFd;
    clientPoll.events = POLLIN;
    clientPoll.revents = 0;

    _pollFds.push_back(clientPoll);
	Client client = Client();
	client.fd = clientFd;
	client.lastActivity = time(NULL);
	_clients[clientFd] = client;
}

void Server::removeClient(int clientFd)
{
    close(clientFd);
    _clients.erase(clientFd);

    for (std::vector<struct pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it)
    {
        if (it->fd == clientFd)
        {
            _pollFds.erase(it);
            break;
        }
    }
}


void Server::run ()
{
	struct pollfd serverPoll;

	serverPoll.fd = _serverFd;
	serverPoll.events = POLLIN;
	serverPoll.revents = 0;

	_pollFds.push_back(serverPoll);
	_running = true;
	while(_running)
	{
		size_t polledCount = _pollFds.size();
		std::set <int> clientsToRemove;
		int result = poll(
			&_pollFds[0],
			polledCount,
			POLL_TIMEOUT_MS
		);
		if (result == -1)
		{
			if (errno == EINTR)
				continue;
			throw std::runtime_error(strerror(errno));
		}
		time_t now = time(NULL);
		for(size_t i = 0; i < polledCount; i++)
		{
			if (_pollFds[i].fd != _serverFd &&
				now - _clients[_pollFds[i].fd].lastActivity >= CLIENT_TIMEOUT)
			{
				clientsToRemove.insert(_pollFds[i].fd);
				continue;
			}
			if(_pollFds[i].revents & POLLNVAL)
			{
				if(_pollFds[i].fd == _serverFd)
					throw std::runtime_error(strerror(errno));
				clientsToRemove.insert(_pollFds[i].fd);
				continue;
			}
			if ((_pollFds[i].revents & POLLHUP ) && !(_pollFds[i].revents & POLLIN))
				{
					if (_pollFds[i].fd != _serverFd)
					{
						clientsToRemove.insert(_pollFds[i].fd);
						continue;
					}
				}
			if(_pollFds[i].revents & POLLERR)
			{
				if(_pollFds[i].fd == _serverFd)
					throw std::runtime_error("Error on server Socket");
				clientsToRemove.insert(_pollFds[i].fd);
				continue;
			}
			if(_pollFds[i].revents & POLLIN)
			{
				if (_pollFds[i].fd == _serverFd)
				{
					while (true)
					{
						int clientFD = accept(_serverFd, NULL, NULL);

						if (clientFD == -1)
						{
							if (errno == EAGAIN || errno == EWOULDBLOCK)
								break;
							if (errno == EINTR)
								continue;
							break;
						}
						if (fcntl(clientFD, F_SETFL, O_NONBLOCK) == -1)
						{
							close(clientFD);
							continue;
						}
						
						addClient(clientFD);
					}
				}
				else
				{
					char buffer[4089];
					std::map<int, Client>::iterator clientIt;
					bool receivedData = false;

					clientIt = _clients.find(_pollFds[i].fd);
					if (clientIt == _clients.end())
					{
						std::cerr << "Client state not found for fd " << _pollFds[i].fd << std::endl;
						clientsToRemove.insert(_pollFds[i].fd);
						continue;
					}
					ssize_t bytes_read = recv(
						_pollFds[i].fd,
						buffer,
						sizeof(buffer),
						0
						);
					if (bytes_read > 0)
						{
							clientIt->second.bufferRead.append(buffer, bytes_read);
							receivedData = true;
							clientIt->second.lastActivity = now;
						}
						else if (bytes_read == 0)
						{
							clientIt->second.readClose = true;
							_pollFds[i].events &= ~POLLIN;
						}
						else
						{
							_pollFds[i].events |= POLLIN;
						}
					if (receivedData)
					{
						_handler->onReceive(_pollFds[i].fd, clientIt->second);
						if (!clientIt->second.bufferWrite.empty())
							_pollFds[i].events |= POLLOUT;
					}
					if (clientIt->second.readClose)
						_handler->onDisconnect(_pollFds[i].fd, clientIt->second);

				}
			}
			if(_pollFds[i].revents & POLLOUT)
			{
				std::map <int, Client>::iterator clientIt;

				clientIt = _clients.find(_pollFds[i].fd);
				if (clientIt == _clients.end())
				{
				    clientsToRemove.insert(_pollFds[i].fd);
					continue;
				}
				ssize_t bytes_sent = send(
						_pollFds[i].fd,
						clientIt->second.bufferWrite.c_str(),
						clientIt->second.bufferWrite.size(),
						0
					);
				if (bytes_sent > 0)
					{
						clientIt->second.bufferWrite.erase(0, bytes_sent);
						clientIt->second.lastActivity = now;
					}
				else
					{
						_pollFds[i].events |= POLLOUT;
					}
				if (clientIt->second.bufferWrite.empty())
				{
					_pollFds[i].events &= ~POLLOUT;
					clientIt->second.sendComplete = true;
					_handler->onSendComplete(_pollFds[i].fd, clientIt->second);
					if (clientIt->second.readClose)
						clientsToRemove.insert(_pollFds[i].fd);
				}

			}
		}
		for (std::set<int>::iterator it = clientsToRemove.begin(); it != clientsToRemove.end(); ++it)
		{
		    removeClient(*it);
		}
	}
}