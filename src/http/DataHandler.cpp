#include "../includes/HttpHandler.hpp"

#include <cstdlib>
#include <sstream>

static const size_t MAX_HEADER_SIZE = 8192;
static const size_t MAX_BODY_SIZE = 1024 * 1024;

static std::string trim(const std::string& value)
{
	size_t first = value.find_first_not_of(" \t");
	size_t last = value.find_last_not_of(" \t");

	if (first == std::string::npos)
		return std::string();
	return value.substr(first, last - first + 1);
}

static std::string lower(const std::string& value)
{
	std::string result(value);
	for (size_t i = 0; i < result.size(); ++i)
	{
		if (result[i] >= 'A' && result[i] <= 'Z')
			result[i] = static_cast<char>(result[i] - 'A' + 'a');
	}
	return result;
}

static bool getContentLength(const std::string& headerBlock, size_t& length)
{
	size_t position = 0;
	while (position < headerBlock.size())
	{
		size_t end = headerBlock.find("\r\n", position);
		if (end == std::string::npos)
			end = headerBlock.size();
		size_t colon = headerBlock.find(':', position);
		if (colon != std::string::npos && colon < end &&
			lower(trim(headerBlock.substr(position, colon - position))) == "content-length")
		{
			std::string value = trim(headerBlock.substr(colon + 1, end - colon - 1));
			char* parsedEnd = NULL;
			unsigned long parsed = std::strtoul(value.c_str(), &parsedEnd, 10);
			if (value.empty() || *parsedEnd != '\0' || parsed > MAX_BODY_SIZE)
				return false;
			length = static_cast<size_t>(parsed);
			return true;
		}
		if (end == headerBlock.size())
			break;
		position = end + 2;
	}
	length = 0;
	return true;
}

HttpResponse::HttpResponse() : status(200), reason("OK")
{
}

HttpHandler::HttpHandler(IHttpCore* core) : _core(core)
{
}

bool HttpHandler::parseRequest(const std::string& raw, size_t headerEnd,
	size_t bodyLength, HttpRequest& request) const
{
	size_t lineEnd = raw.find("\r\n");
	if (lineEnd == std::string::npos)
		return false;

	std::istringstream firstLine(raw.substr(0, lineEnd));
	if (!(firstLine >> request.method >> request.target >> request.version))
		return false;
	if (request.version != "HTTP/1.0" && request.version != "HTTP/1.1")
		return false;

	size_t position = lineEnd + 2;
	while (position < headerEnd)
	{
		size_t end = raw.find("\r\n", position);
		if (end == std::string::npos || end > headerEnd)
			return false;
		size_t colon = raw.find(':', position);
		if (colon == std::string::npos || colon > end || colon == position)
			return false;
		std::string name = lower(trim(raw.substr(position, colon - position)));
		std::string value = trim(raw.substr(colon + 1, end - colon - 1));
		request.headers[name] = value;
		position = end + 2;
	}
	request.body = raw.substr(headerEnd + 4, bodyLength);
	return true;
}

void HttpHandler::queueError(Client& client, int status,
	const std::string& reason)
{
	HttpResponse response;
	response.status = status;
	response.reason = reason;
	response.body = reason + "\n";
	response.headers["Content-Type"] = "text/plain";
	queueResponse(client, response);
}

void HttpHandler::queueResponse(Client& client, const HttpResponse& response)
{
	std::ostringstream output;
	output << "HTTP/1.1 " << response.status << " " << response.reason << "\r\n";
	std::map<std::string, std::string>::const_iterator it;
	for (it = response.headers.begin(); it != response.headers.end(); ++it)
		output << it->first << ": " << it->second << "\r\n";
	output << "Content-Length: " << response.body.size() << "\r\n\r\n";
	output << response.body;
	client.bufferWrite += output.str();
	client.sendComplete = false;
}

void HttpHandler::onReceive(int clientFd, Client& client)
{
	while (!client.bufferRead.empty())
	{
		size_t headerEnd = client.bufferRead.find("\r\n\r\n");
		if (headerEnd == std::string::npos)
		{
			if (client.bufferRead.size() > MAX_HEADER_SIZE || client.readClose)
			{
				queueError(client, 400, "Bad Request");
				client.bufferRead.clear();
				client.requestComplete = true;
			}
			return;
		}
		if (headerEnd > MAX_HEADER_SIZE)
		{
			queueError(client, 431, "Request Header Fields Too Large");
			client.bufferRead.clear();
			client.requestComplete = true;
			return;
		}

		std::string headerBlock = client.bufferRead.substr(0, headerEnd);
		size_t contentLength = 0;
		if (!getContentLength(headerBlock, contentLength))
		{
			queueError(client, 400, "Bad Request");
			client.bufferRead.clear();
			client.requestComplete = true;
			return;
		}
		if (client.bufferRead.size() < headerEnd + 4 + contentLength)
		{
			if (client.readClose)
			{
				queueError(client, 400, "Bad Request");
				client.bufferRead.clear();
				client.requestComplete = true;
			}
			return;
		}

		HttpRequest request;
		if (!parseRequest(client.bufferRead, headerEnd, contentLength, request))
		{
			queueError(client, 400, "Bad Request");
			client.bufferRead.clear();
			client.requestComplete = true;
			return;
		}
		size_t consumed = headerEnd + 4 + contentLength;
		client.bufferRead.erase(0, consumed);
		client.requestComplete = true;

		if (_core == NULL)
		{
			queueError(client, 500, "Internal Server Error");
			return;
		}
		HttpResponse response;
		_core->handle(clientFd, request, response);
		queueResponse(client, response);
	}
}

void HttpHandler::onDisconnect(int clientFd, Client& client)
{
	if (!client.bufferRead.empty())
		onReceive(clientFd, client);
}