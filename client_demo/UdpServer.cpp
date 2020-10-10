#include "pch.h"
#include "UdpServer.h"
#include <stdio.h>

UdpServer::UdpServer(int port)
{
	this->port = port;
	socket_fd = -1;
}

int UdpServer::Open()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 3), &wsaData) != 0)
	{
		printf("WSAStartup error\n");
		return 0;
	}
	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_fd < 0)
	{
		printf("socket error!\n");
		return 0;
	}

	sockaddr_in ser_addr;
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(port);//监听的端口
	ser_addr.sin_addr.S_un.S_addr = INADDR_ANY;//本机的端口号和IP地址

	//bind()将套接字描述符，即套接字号与套接字地址关联起来
	if (bind(socket_fd, (struct sockaddr*) & ser_addr, sizeof(ser_addr)) < 0)
	{
		printf("套接字描述符与套接字地址绑定失败\n");
		closesocket(socket_fd);//关闭socket
		WSACleanup();
		return 0;
	}

	return 0;
}

int UdpServer::Close()
{
	closesocket(socket_fd);
	WSACleanup();

	return 0;
}

int UdpServer::Read(char* data, int data_len)
{
	return recvfrom(socket_fd, data, data_len, 0, NULL, NULL);
}
