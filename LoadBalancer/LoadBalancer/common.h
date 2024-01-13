#ifndef COMMON_H
#define COMMON_H

#define _winsock_deprecated_no_warnings
#pragma comment(lib, "ws2_32.lib")
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define BUFFER_SIZE 256
#define SafeCloseHandle(handle) if(handle) CloseHandle(handle)
#define MAX_SMART_METERS 10

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <time.h>
#include <Windows.h>

typedef struct message_st {
	bool returnData;
	unsigned int messageSize;
	char data[BUFFER_SIZE];
} MESSAGEW;

int numOfWorkers = 0;
bool shuttingDown = false;

typedef struct data_st {
	char data[BUFFER_SIZE];
	struct data_st* next;
} DATA;

CRITICAL_SECTION csOutput;

void print(const char* format, ...) {
	va_list args; 
	va_start(args, format); 
	EnterCriticalSection(&csOutput);
	printf("\n");
	vprintf(format, args);
	LeaveCriticalSection(&csOutput);
	va_end(args);
}

bool initSockets()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

void nonBlock(SOCKET* socket) {
	unsigned long nonBlockingMode = 1;
	int iResult = ioctlsocket(*socket, FIONBIO, &nonBlockingMode);
	if (iResult == SOCKET_ERROR) {
		printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
	}
}

SOCKET connectSocket(unsigned short port) {
	if (!initSockets())
		return INVALID_SOCKET;

	SOCKET temp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (connect(temp, (SOCKADDR*)&address, sizeof(address)) == SOCKET_ERROR)
	{
		printf("Unable to connect to server.\n");
		closesocket(temp);
		WSACleanup();
		return INVALID_SOCKET;
	}

	nonBlock(&temp);

	return temp;
}

SOCKET setListenSocket(PCSTR port) {
	SOCKET listenSocket = INVALID_SOCKET;

	if (initSockets() == false)
	{
		return INVALID_SOCKET;
	}

	addrinfo* resultingAddress = NULL;
	addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;  
	hints.ai_socktype = SOCK_STREAM; 
	hints.ai_protocol = IPPROTO_TCP; 
	hints.ai_flags = AI_PASSIVE;     

	int iResult = getaddrinfo(NULL, port, &hints, &resultingAddress);
	if (iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return INVALID_SOCKET;
	}

	listenSocket = socket(AF_INET,   
		SOCK_STREAM,  
		IPPROTO_TCP); 

	if (listenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);
		WSACleanup();
		return INVALID_SOCKET;
	}

	iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);
		closesocket(listenSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}

	nonBlock(&listenSocket);

	freeaddrinfo(resultingAddress);

	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}

	return listenSocket;
}
#endif // !COMMON_H

