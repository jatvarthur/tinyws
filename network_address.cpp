#include "network.h"


ADDRESS_FAMILY NetworkAddress::family() const
{
	return reinterpret_cast<const sockaddr*>(m_addr)->sa_family;
}

NetworkAddress::operator sockaddr*()
{
	return reinterpret_cast<sockaddr*>(m_addr);
}

int NetworkAddress::length() const
{
	return sizeof(m_addr);
}

std::wstring NetworkAddress::toString() const
{
	return L"addr()";
}


Ip4Address::Ip4Address(uint32_t addr, uint16_t port)
{
	memset(&m_addr, 0, sizeof(m_addr));

	sockaddr_in* sa = reinterpret_cast<sockaddr_in*>(m_addr);
	sa->sin_family = AF_INET;
	sa->sin_addr.S_un.S_addr = htonl(addr);
	sa->sin_port = htons(port);
}

std::wstring Ip4Address::toString() const
{
	const sockaddr_in* sa = reinterpret_cast<const sockaddr_in*>(m_addr);
	wchar_t buf[INET_ADDRSTRLEN];
	InetNtopW(AF_INET, (LPVOID)(&sa->sin_addr), buf, INET_ADDRSTRLEN);

	return buf;
}
