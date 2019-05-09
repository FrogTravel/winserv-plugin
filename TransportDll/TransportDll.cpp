// TransportDll.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <stdio.h>
#include <malloc.h>

#define DEFAULT_PORT "27015"

const char* client(char* server_ip, GUID* plugin_id)
{
	// Initialize Winsock
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return nullptr;
	}

	struct addrinfo *result = nullptr,
		hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(server_ip, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return nullptr;
	}

	SOCKET connectSocket = INVALID_SOCKET;

	// Attempt to connect to an address until one succeeds
	for (struct addrinfo *ptr = result; ptr != nullptr; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (connectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %d\n", WSAGetLastError());
			WSACleanup();
			return nullptr;
		}

		// Connect to server.
		iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(connectSocket);
			connectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (connectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return nullptr;
	}

	char sendbuf[sizeof(GUID)];
	char recvbuf[1024];
	const int recvbuflen = sizeof(recvbuf);

	memcpy(sendbuf, plugin_id, sizeof(sendbuf));

	// Send an initial buffer
	iResult = send(connectSocket, (const char*)&sendbuf, (int)sizeof(sendbuf), 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return nullptr;
	}

	printf("Bytes Sent: %d\n", iResult);

	// shutdown the connection since no more data will be sent
	iResult = shutdown(connectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return nullptr;
	}

	// Receive until the peer closes the connection
	do {

		iResult = recv(connectSocket, (char*)&recvbuf, recvbuflen, 0);
		if (iResult > 0)
			printf("Bytes received: %d\n", iResult);
		else if (iResult == 0)
			printf("Connection closed\n");
		else
			printf("recv failed with error: %d\n", WSAGetLastError());

	} while (iResult > 0);

	printf("receive %s \n", recvbuf);

	// cleanup
	closesocket(connectSocket);
	WSACleanup();

	const auto len = strlen(recvbuf) + 1;
	const auto ret = (char*)malloc(len);
	memcpy(ret, recvbuf, len);
	return ret;
}

#define MAX_CHARS_FOR_IP 20
char server_ip[MAX_CHARS_FOR_IP];

extern "C" __declspec(dllexport) bool __cdecl SetServerEndpoint(char* ip)
{
	const auto len = strlen(ip) + 1; //including null-terminated character
	if (len > MAX_CHARS_FOR_IP) return false;
	memcpy(server_ip, ip, len);
	return true;
}

extern "C" __declspec(dllexport) const char* __cdecl GetCopyright(GUID pluginId)
{
	return client(server_ip, &pluginId);
}
