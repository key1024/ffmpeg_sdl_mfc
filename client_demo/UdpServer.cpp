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
	ser_addr.sin_port = htons(port);//�����Ķ˿�
	ser_addr.sin_addr.S_un.S_addr = INADDR_ANY;//�����Ķ˿ںź�IP��ַ

	//bind()���׽��������������׽��ֺ����׽��ֵ�ַ��������
	if (bind(socket_fd, (struct sockaddr*) & ser_addr, sizeof(ser_addr)) < 0)
	{
		printf("�׽������������׽��ֵ�ַ��ʧ��\n");
		closesocket(socket_fd);//�ر�socket
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
