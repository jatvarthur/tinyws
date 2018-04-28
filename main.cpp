#include <stdio.h>
#include <vector>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include "network.h"


class WSClientContext: public ClientContext
{
public:
	virtual void processTransfer() override
	{
		DWORD nBytesRead;
		char bufData[2048];
		WSABUF buf;
		buf.buf = bufData;
		buf.len = sizeof(bufData);

		DWORD flags = 0;
		int result = WSARecv(*m_socket, &buf, 1, &nBytesRead, &flags, this, NULL);
		if (result = SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
			_trace.e(L"Error reading, %d\n", WSAGetLastError());
		}

		if (strncmp(bufData, "GET / HTTP", 10) == 0) {
			const char* content = "<html><title>Hello, World!</title><body><h1>Hello, socket world!</h1></body></html>";
			sprintf(bufData, "HTTP/1.1 200 OK\r\nContent-Length:%d\r\n\r\n%s", strlen(content), content);
			nBytesRead = strlen(bufData);
			result = WSASend(*m_socket, &buf, 1, &nBytesRead, flags, this, NULL);
		}
	}

};

class WSServerCallback: public ServerSocketCallback
{
public:
	virtual ClientContext* createContext() override
	{
		return new WSClientContext();
	}
};


HANDLE g_hStopEvent = 0;

BOOL WINAPI ConsoleCtrlHandler(DWORD Ctrl)
{
	switch (Ctrl){
	case CTRL_C_EVENT:      // Falls through..
	case CTRL_CLOSE_EVENT:
		SetEvent(g_hStopEvent);
		return TRUE;
	default:
		return FALSE;
	}
}

int main()
{
	Network network;
	WSServerCallback callback;

	Ip4Address listenAddress(INADDR_ANY, 8080);
	ServerSocket* socket = network.listen(listenAddress, Network::Stream, &callback);

	if (!socket->isValid()) {
		_trace.e(L"Can't start network listener.\n");
		return EXIT_FAILURE;
	}

	g_hStopEvent = CreateEvent(0, FALSE, FALSE, 0);
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

	WaitForSingleObject(g_hStopEvent, INFINITE);

	SetConsoleCtrlHandler(NULL, FALSE);

	return EXIT_SUCCESS;
}