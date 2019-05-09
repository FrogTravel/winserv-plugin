// Server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <list>
#include <stdio.h>
#define SERVICE
#include "../api.h"
#include "../plugin_id.h"
#define DEFAULT_PORT "27015"

#include <combaseapi.h>

LPCTSTR strKeyName = TEXT("SOFTWARE\\PluginSystem");
HANDLE handle_event = NULL;
HANDLE handle_thread = NULL;
DWORD dwFilter = REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_ATTRIBUTES | REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_SECURITY;
HKEY hKey;

SOCKET server_init(void)
{
	// Initialize Winsock
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		WSACleanup();
		return INVALID_SOCKET;
	}

	struct addrinfo *result = NULL,
		hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return INVALID_SOCKET;
	}

	// Create a SOCKET for connecting to server
	const SOCKET listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return INVALID_SOCKET;
	}

	// Setup the TCP listening socket
	iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(listenSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}

	freeaddrinfo(result);
	//closesocket(listenSocket);	//this should be done by calling server_stop
	//WSACleanup();					//this should be done by calling server_done
	return listenSocket;
}

void server_stop(const SOCKET listenSocket)
{
	closesocket(listenSocket);
}

void server_done(void)
{
	WSACleanup();
}

SOCKET server_listen(const SOCKET listenSocket)
{
	const int iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		return INVALID_SOCKET;
	}

	// Accept a client socket
	const SOCKET clientSocket = accept(listenSocket, NULL, NULL);
	if (clientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		return INVALID_SOCKET;
	}

	return clientSocket;
}

const char* GetPluginCopyright(GUID* guid);

int server_process_request(const SOCKET clientSocket)
{
    #define MAX_BUFFER 1024
	int iResult;
	char recvbuf[MAX_BUFFER];
	const int recvbuflen = sizeof(recvbuf);
	char sendbuf[MAX_BUFFER] = "Copyrigth Server 2000-2018";
	int sendbuflen = sizeof(sendbuf);

	// Receive until the peer shuts down the connection
	do {

		iResult = recv(clientSocket, (char*)&recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);

			if (iResult != 16)
			{
				printf("Invalid send buffer length \n");
			}
			else
			{
				auto ret = GetPluginCopyright((GUID*)recvbuf);
				memcpy(sendbuf, ret, strlen(ret) + 1); //TODO check overflow
				sendbuflen = strlen(ret) + 1;
			}

			const int iSendResult = send(clientSocket, (char*)&sendbuf, sendbuflen, 0);
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(clientSocket);
				return 1;
			}
			printf("Bytes sent: %d\n", iSendResult);
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else  {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(clientSocket);
			return 1;
		}

	} while (iResult > 0);

	// shutdown the connection since we're done
	iResult = shutdown(clientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(clientSocket);
		return 1;
	}

	// cleanup
	closesocket(clientSocket);
	return 0;
}

int server(int argc, char **argv)
{
	const SOCKET listenSocket = server_init();
	if (listenSocket == INVALID_SOCKET) return 1;

	while (1)
	{
		const SOCKET clientSocket = server_listen(listenSocket);
		if (clientSocket == INVALID_SOCKET) break;
		server_process_request(clientSocket);
	}

	server_stop(listenSocket);
	server_done();
	return 0;
}

typedef struct _DllListElement
{
	GUID pluginId;
	const wchar_t* DllName;
	pfn_GetCopyright GetCopyright;
} DllListElement;

std::list<DllListElement> g_DllList = {
	{ plugin1_id, L"Plugin1_1.dll", nullptr },
	{ plugin2_id, L"Pugin2.dll", nullptr }
};

const char* GetPluginCopyright(GUID* guid)
{
	printf("GetPluginCopyright: searching... \n");
	for (DllListElement element : g_DllList)
	{
		if (IsEqualGUID(element.pluginId, *guid))
		{
			printf("GetPluginCopyright: Guid was found \n");
			if (nullptr == element.GetCopyright)
			{
				//Resolve
				printf("GetPluginCopyright: Dll %S is loading \n", element.DllName);
				auto hLibrary = LoadLibrary(element.DllName);
				if (hLibrary)
				{
					printf("GetPluginCopyright: Dll %S was loaded \n", element.DllName);
					void* pfn = GetProcAddress(hLibrary, "GetCopyright");
					printf("GetPluginCopyright: _GetCopyright at %p \n", pfn);
					element.GetCopyright =
						reinterpret_cast<pfn_GetCopyright>(pfn);
				}
			}
			if (nullptr == element.GetCopyright) break;

			return element.GetCopyright();
		}
	}
	return "Error";
}

int prepare_registry() {
	/*
Open registry key to handle
*/
	long result = RegOpenKeyEx(
		HKEY_CURRENT_USER,
		L"SOFTWARE\\PluginSystem",
		0,
		KEY_ALL_ACCESS,
		&hKey);

	if (result != ERROR_SUCCESS) {
		printf("ERROR %d", result);
	}


	/*
	Get Number of registers
	*/
	// Reading
	DWORD cValues;              // number of values for key 
	DWORD cbMaxValueData;       // longest value data 
	DWORD retCode;
	DWORD cchValue = 16383;
	wchar_t guid_string[256];
	wchar_t buffer[256];

	retCode = RegQueryInfoKey(
		hKey,            // key handle 
		NULL,            // buffer for class name 
		NULL,            // size of class string 
		NULL,            // reserved 
		NULL,            // number of subkeys 
		NULL,            // longest subkey size 
		NULL,            // longest class string 
		&cValues,        // number of values for this key 
		NULL,            // longest value name 
		&cbMaxValueData, // longest value data 
		NULL,            // security descriptor 
		NULL);

	/*
	Enum registry
	TODO add iteration
	*/
	cchValue = 16383;
	DWORD lpData = cbMaxValueData;

	guid_string[0] = L'\0';
	buffer[0] = L'\0';

	long resEnum = RegEnumValueW(
		hKey,
		0,
		guid_string,
		&cchValue,
		NULL,
		NULL,
		(LPBYTE)buffer,
		&lpData
	);

	if (resEnum != ERROR_SUCCESS) {
		printf("ERROR: %d", resEnum);
		return -1;
	}

	/*
	Get new element
	*/
	IID guid;

	const auto resultIIDFromString = IIDFromString(guid_string, &guid);
	if (resultIIDFromString != S_OK) {
		printf("Enable to convert %S to GUID \n", guid_string);
		return NULL;
	}

	bool upd = false;
	for (auto i = begin(g_DllList); i != end(g_DllList); i++)
	{
		auto element = *i;
		if (IsEqualGUID(element.pluginId, guid))
		{
			*i = DllListElement({ guid, buffer, nullptr });
			printf("PluginList: Plugin with GUID %S was updated\n", guid_string);
			printf("PluginList: New plugin name - %S \n", buffer);
			upd = true;
			break;
		}
	}
	// If this dll is new add new instance to list
	if (!upd) {
		g_DllList.push_back(DllListElement({ guid, buffer, nullptr }));
		printf("PluginList: Plugin with GUID %S was added\n", guid_string);
		printf("PluginList: New plugin name - %S \n", buffer);
	}
}

DWORD WINAPI listener_thread(LPVOID lpParam) {
	DWORD dwWaitResult;

	while (true) {
		dwWaitResult = WaitForSingleObject(handle_event, INFINITE);
		switch (dwWaitResult) {
		case WAIT_OBJECT_0: {
			prepare_registry();
			//reissue the notify command
			RegNotifyChangeKeyValue(hKey, TRUE, dwFilter, handle_event, TRUE);
			break;
		}
		default:
			return 0;
		}
	}
	return 1;
}


int main(int argc, char* argv[])
{
	DWORD num_thread;

	// Open the key for PluginList and PerfMon 
	long sts = RegOpenKeyEx(HKEY_CURRENT_USER, strKeyName, 0, KEY_ALL_ACCESS, &hKey);

	// Runing thread
	if (sts == ERROR_SUCCESS) {
		handle_event = CreateEvent(NULL, TRUE, FALSE, L"Register Event");
		handle_thread = CreateThread(NULL, 0, listener_thread, NULL, 0, &num_thread);

		RegNotifyChangeKeyValue(hKey, TRUE, dwFilter, handle_event, TRUE);
	}



	server(argc, argv);
	return 0;
}

/*
#else
HMODULE hLibrary = LoadLibrary(L"TransportDll.dll");
if (hLibrary == nullptr)
{
	printf("LoadLibrary failed %lu \n", GetLastError());
	return -1;
}

void* p = (void*)GetProcAddress(hLibrary, "GetCopyright");
if (nullptr == p)
{
	printf("GetProcAddress failed %lu \n", GetLastError());
	return -1;
}
GetCopyright = reinterpret_cast<pfn_GetCopyright>(p);
printf("Copyright = %s \n", GetCopyright(plugin1_id));
#endif
*/