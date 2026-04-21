#include "../../inc/core/Connection.hpp"

void Connection::handleRequest()
{
	char buff[1025];
	ssize_t bread;

	bread = recv(_client_fd, buff, sizeof(buff), 0);
	if (bread == 0)
	{
		throw ConnectionClosed();
		return;
	}
	if (bread == -1)
	{
		throw std::runtime_error("Error: recv failed.");
	}
	buff[bread] = 0;
	_request.append(buff);
}