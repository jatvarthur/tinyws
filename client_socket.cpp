#include "network.h"


ClientSocket::ClientSocket(Network& owner,
	SOCKET socket, const NetworkAddress& address, Network::SocketType type):
	Socket(owner, address, type)	
{
	m_socket = socket;
}

ClientSocket::~ClientSocket()
{
	shutdown();
}

void ClientSocket::shutdown()
{
	if (m_socket != INVALID_SOCKET) {
		::shutdown(m_socket, SD_BOTH);
		closesocket(m_socket);
	}

	m_lastError = WSAENOTSOCK;
	m_socket = INVALID_SOCKET;
}
