#pragma once

#include <WinSock2.h>

class UdpServer
{
public:
	UdpServer(int port);

	int Open();
	int Close();
	int Read(char* data, int data_len);

private:
	int port;
	SOCKET socket_fd;
};

