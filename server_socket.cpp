#include "network.h"

#define WORKER_WAIT_TIMEOUT		1500

static DWORD WINAPI ListenerThread(LPVOID param)
{
	ServerSocket* socket = reinterpret_cast<ServerSocket*>(param);

	for (;;) {
		socket->accept();
	}
}




static DWORD WINAPI WorkerThread(LPVOID param)
{
	ServerSocket* socket = reinterpret_cast<ServerSocket*>(param);

	for (;;) {
		DWORD nBytesRead;
		DWORD key;
		LPOVERLAPPED overlapped;

		if (!GetQueuedCompletionStatus(socket->completionPort(), &nBytesRead, &key, &overlapped, INFINITE)) {
			_trace.e(L"Worker error, can't get IO status, %d\n", GetLastError());
			continue;
		}

		ClientContext* context = static_cast<ClientContext*>(overlapped);
		context->processTransfer();
	}
}


ServerSocket::ServerSocket(Network& owner, 
		const NetworkAddress& address, Network::SocketType type, ServerSocketCallback* callback):
	Socket(owner, address, type),
	m_hCompletionPort(INVALID_HANDLE_VALUE),
	m_hListener(NULL),
	m_callback(callback)
{
	initialize();
}

ServerSocket::~ServerSocket()
{
	shutdown();
}

void ServerSocket::initialize()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	DWORD nWorkers = si.dwNumberOfProcessors;
	_trace.i(L"Initializing server socket at %s with %d workers.\n", m_address.toString().c_str(), nWorkers);

	m_socket = WSASocket(m_address.family(), 
		m_type == Network::Stream ? SOCK_STREAM : SOCK_DGRAM,  
		m_type == Network::Stream ? IPPROTO_TCP : IPPROTO_UDP,
		NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (m_socket == INVALID_SOCKET) {
		m_lastError = WSAGetLastError();
		_trace.e(L"Error creating socket, %d\n", m_lastError);
		shutdown();
		return;
	}

	if (bind(m_socket, m_address, m_address.length()) == SOCKET_ERROR) {
		m_lastError = WSAGetLastError();
		_trace.e(L"Error binding socket, %d\n", m_lastError);
		shutdown();
		return;
	}
	if (listen(m_socket, SOMAXCONN) == SOCKET_ERROR) {
		m_lastError = WSAGetLastError();
		_trace.e(L"Error binding socket, %d\n", m_lastError);
		shutdown();
		return;
	}

	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, nWorkers);
	if (m_hCompletionPort == 0) {
		m_lastError = GetLastError();
		_trace.e(L"Error creating completion port, %d\n", m_lastError);
		return;
	}

	m_workers.reserve(nWorkers);
	for (DWORD i = 0; i < nWorkers; ++i) {
		HANDLE hThread = CreateThread(NULL, 0, WorkerThread, this, 0, NULL);
		if (hThread != NULL) m_workers.push_back(hThread);

	}

	if (m_workers.size() == 0) {
		m_lastError = WSAEFAULT;
		_trace.e(L"Error creating workers\n");
		shutdown();
		return;
	}
	else if (m_workers.size() != nWorkers) {
		_trace.w(L"Not all workers created, requested %d, created %d\n", nWorkers, m_workers.size());
	}

	m_hListener = CreateThread(NULL, 0, ListenerThread, this, 0, NULL);
	if (m_hListener == NULL) {
		_trace.e(L"Error creating workers\n");
		shutdown();
		return;
	}
}

void ServerSocket::shutdown()
{
	// send threads exit signal
	if (m_workers.size() > 0) {
		for (DWORD i = 0; i < m_workers.size(); ++i)
			PostQueuedCompletionStatus(m_hCompletionPort, 0, 0, NULL);

		DWORD waitResult = WaitForMultipleObjects(m_workers.size(), &m_workers[0], TRUE, WORKER_WAIT_TIMEOUT);
		if (waitResult == WAIT_TIMEOUT || waitResult == WAIT_FAILED) {
			_trace.e(L"Wait timeout closing workers.\n");
		}

		for (DWORD i = 0; i < m_workers.size(); ++i)
			CloseHandle(m_workers[i]);
		m_workers.clear();
	}

	if (m_socket != INVALID_SOCKET) {
		::shutdown(m_socket, SD_BOTH);
		closesocket(m_socket);
	}

	if (m_hCompletionPort != 0 && m_hCompletionPort != INVALID_HANDLE_VALUE)
		CloseHandle(m_hCompletionPort);

	if (m_hListener != NULL) {
		DWORD waitResult = WaitForSingleObject(m_hListener, WORKER_WAIT_TIMEOUT);
		if (waitResult == WAIT_TIMEOUT || waitResult == WAIT_FAILED) {
			_trace.e(L"Wait timeout clising listner.\n");
			CloseHandle(m_hListener);
		}
		CloseHandle(m_hListener);
	}

	m_lastError = WSAENOTSOCK;
	m_socket = INVALID_SOCKET;
	m_hCompletionPort = INVALID_HANDLE_VALUE;
	m_hListener = NULL;
}

bool ServerSocket::isValid() const
{
	return 	m_socket != INVALID_SOCKET && m_hCompletionPort != INVALID_HANDLE_VALUE;
}

void ServerSocket::accept()
{
	NetworkAddress address;
	int addrLen = address.length();
	SOCKET clientSocket = WSAAccept(*this, address, &addrLen, NULL, NULL);

	ClientContext* context = m_callback->createContext();
	context->setAddress(address);
	context->setSocket(m_owner.attachClientSocket(clientSocket, address, m_type));

	CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSocket), m_hCompletionPort, 0, 0);
	PostQueuedCompletionStatus(m_hCompletionPort, 0, 1, context);
}