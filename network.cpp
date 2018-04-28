#include "network.h"

LONG Network::s_nInstances = 0;
int Network::s_initResult = WSANOTINITIALISED;

Network::Network()
{
	int count = InterlockedIncrement(&s_nInstances);
	if (count == 1)
		initialize();
}

Network::~Network()
{
	int count = InterlockedDecrement(&s_nInstances);
	if (count == 0)
		shutdown();
}

bool Network::isValid() const
{
	return s_initResult == 0;
}

void Network::close(Socket* socket)
{
	auto it = std::find(m_sockets.begin(), m_sockets.end(), socket);
	if (it == m_sockets.end()) {
		_trace.w(L"Trying to close unowned socket\n");
		return;
	}

	m_sockets.erase(it);
	delete socket;		
}


void Network::initialize()
{
	s_initResult = WSAStartup(MAKEWORD(2, 2), &m_netInfo);;
	if (s_initResult != 0) {
		_trace.e(L"Error initializing WSA, %d\n", s_initResult);
	}

}

void Network::shutdown()
{
	for (size_t i = 0; i < m_sockets.size(); ++i)
		delete m_sockets[i];
	m_sockets.clear();

	if (s_initResult == 0)
		WSACleanup();

	s_initResult = WSANOTINITIALISED;
}

ServerSocket* Network::listen(const NetworkAddress& address, Network::SocketType type, ServerSocketCallback* callback)
{
	if (callback == NULL) {
		_trace.e(L"Invalid callback in Network::listen()\n");
		return nullptr;
	}

	ServerSocket* result = new ServerSocket(*this, address, type, callback);
	m_sockets.push_back(result);
	
	return result;
}

ClientSocket* Network::attachClientSocket(SOCKET socket, const NetworkAddress& address, SocketType type)
{
	ClientSocket* result = new ClientSocket(*this, socket, address, type);
	m_sockets.push_back(result);

	return result;
}
