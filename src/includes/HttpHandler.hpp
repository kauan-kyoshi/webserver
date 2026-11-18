#ifndef HTTP_HANDLER_HPP
#define HTTP_HANDLER_HPP

#include <map>
#include <string>

#include "IDataHandler.hpp"

struct HttpRequest
{
	std::string method;
	std::string target;
	std::string version;
	std::map<std::string, std::string> headers;
	std::string body;
};

struct HttpResponse
{
	int status;
	std::string reason;
	std::map<std::string, std::string> headers;
	std::string body;

	HttpResponse();
};

class IHttpCore
{
public:
	virtual ~IHttpCore() {}
	virtual void handle(int clientFd, const HttpRequest& request,
		HttpResponse& response) = 0;
};

class HttpHandler : public IDataHandler
{
private:
	IHttpCore* _core;

	bool parseRequest(const std::string& raw, size_t headerEnd,
		size_t bodyLength, HttpRequest& request) const;
	void queueError(Client& client, int status, const std::string& reason);
	void queueResponse(Client& client, const HttpResponse& response);

public:
	explicit HttpHandler(IHttpCore* core);
	virtual ~HttpHandler() {}

	virtual void onReceive(int clientFd, Client& client);
	virtual void onDisconnect(int clientFd, Client& client);
};

#endif
