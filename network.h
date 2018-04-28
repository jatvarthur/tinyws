#ifndef N_NETWORK_H_
#define N_NETWORK_H_

#include <stdio.h>
#include <vector>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

class Socket;
class ClientSocket;
class ServerSocket;
class ServerSocketCallback;


/**
 * Address of a computer in a network.
 */
class NetworkAddress
{
public:
	/// Family for this address.
	ADDRESS_FAMILY family() const;

	/// Cast to struct sockaddr* for passing it to WinSock API.
	operator sockaddr*();

	/// Length in bytes of the stored address.
	int length() const;

	/// Returns string representation of the address.
	virtual std::wstring toString() const;

protected:
	// 32 bytes to hold an IPv4/IPv6 (and probably other families) address.
	uint8_t m_addr[32];

};

/**
 * IPv4 address.
 */
class Ip4Address: public NetworkAddress
{
public:
	/// Constructs address from IP and port.
	Ip4Address(uint32_t addr, uint16_t port);

public:
	/// Returns string representation of the address.
	virtual std::wstring toString() const override;
};

/**
 * Main networking library class. Performs WSA initialization/shutdown
 * and impelements basic networking operations.
 * Owns sockets and connections.
 */
class Network
{
public:
	/// Socket types.
	enum SocketType {
		Stream = SOCK_STREAM,
		Dgram  = SOCK_DGRAM,
	};

public:
	/// Default constructor initializes WSA.
	Network();

	/// Shuts down all application networking.
	~Network();

public:
	/// Checks object for validity, valid network is properly initialized.
	bool isValid() const;

	/// Opens new server socket and starts listening on it.
	ServerSocket* listen(
		const NetworkAddress& address,	/// address to bind socket to
		SocketType type,				/// type of socket
		ServerSocketCallback* callback	/// Callback to handle ServerScoket evemts
	);

	/// Closes the specified socket, if this network instance owns it.
	/// After closing pointer becomes invalid and should not be used.
	void close(Socket* socket);

	/// Attaches existing client socket to this network for managing.
	ClientSocket* attachClientSocket(SOCKET socket, const NetworkAddress& address, SocketType type);

protected:
	/// Initializes networking.
	void initialize();

	/// Cleans up and closes networking.
	void shutdown();

protected:
	/// Count if instance of the network. First instance initializes WSA,
	/// last instance closes it.
	static LONG s_nInstances;

	/// Whether the network was initialized properly.
	static int s_initResult;

	/// Networking subsystem data.
	WSADATA m_netInfo;

	/// All open sockets.
	std::vector<Socket*> m_sockets;

};

/**
 * Generic socket.
 */
class Socket
{
	friend class Network;
public:
	/// Closes socket and cleans up resources.
	virtual void close() { m_owner.close(this); }

	/// Checks if the socket is open and ready to transfer.
	virtual bool isValid() const { return m_socket != INVALID_SOCKET; }

	/// Cast to SOCKET to pass it to socket API.
	operator SOCKET() const { return m_socket; }

protected:
	/// Creates socket and assotiates it with its owner.
	Socket(Network& owner, const NetworkAddress& address, Network::SocketType type):
		m_owner(owner),
		m_address(address),
		m_type(type),
		m_lastError(WSAENOTSOCK),
		m_socket(INVALID_SOCKET)
	{}

	/// Destructor is protected, only Network can manage sockets. 
	/// Use close() for explicitly closing a scoket.
	virtual ~Socket() {}

	// Can't copy or assign.
	Socket(const Socket& rhs) = delete;
	const Socket& operator=(const Socket& rhs) = delete;

protected:
	/// Owner of this socket.
	Network& m_owner;

	/// Address this socket bound to.
	NetworkAddress m_address;

	/// Last operation error.
	DWORD m_lastError;

	/// Type of the socket.
	Network::SocketType m_type;

	/// Socket.
	SOCKET m_socket;

};

/**
 * Client socket.
 */
class ClientSocket: public Socket
{
	friend class Network;
protected:
	/// Creates and assignes existing socket.
	ClientSocket(Network& owner, SOCKET socket, const NetworkAddress& address, Network::SocketType type);

	~ClientSocket();

protected:
	void shutdown();

};

/**
 * Client context.
 */
class ClientContext: public OVERLAPPED
{
public:
	/// Creates and initializes undelying OVERLAPPED.
	ClientContext();

	/// Virtual destructor.
	virtual ~ClientContext() noexcept;

	/// Sets address.
	void setAddress(const NetworkAddress& value) { m_address = value; }

	/// Sets client socket.
	void setSocket(ClientSocket* value) { m_socket = value; }

	/// Processes transfer.
	virtual void processTransfer() = 0;

protected:
	/// Address this context communicates with.
	NetworkAddress m_address;

	/// Client socket for receiving/sending.
	ClientSocket* m_socket;

};

/**
 * Callback class for server sockets.
 */
class ServerSocketCallback
{
public:
	/// Factory method to create client context.
	virtual ClientContext* createContext() = 0;

};

/**
 * Server socket. Listens to incoming requests and forwards
 * them to worker threads through I/O completion ports.
 * Owns pool of worker threads
 */
class ServerSocket: public Socket
{
	friend class Network;
public:
	/// Completion port for asynchronous I/O.
	HANDLE completionPort() { return m_hCompletionPort; }

public:
	/// Checks if the socket is open and ready to transfer.
	virtual bool isValid() const override;

	/// Runs accept cycle and forwards connections to worker threads to serve.
	void accept();

protected:
	ServerSocket(Network& owner, const NetworkAddress& address, Network::SocketType type, ServerSocketCallback* callback);

	~ServerSocket();

protected:
	void initialize();

	void shutdown();

protected:
	/// Completion port for asynchronous I/O.
	HANDLE m_hCompletionPort;

	/// Workers pool.
	std::vector<HANDLE> m_workers;

	/// Listener thread.
	HANDLE m_hListener;

	/// Callback for socket events.
	ServerSocketCallback* m_callback;

};

/// Tracing.
class Trace
{
public:
	/// Message severity.
	enum Severity {
		TRACE_DEBUG = 0,
		TRACE_INFO,
		TRACE_WARNING,
		TRACE_ERROR
	};

public:
	/// Constructs trace object with the DEBUG severity.
	Trace();

public:
	/// Returns current trace severity.
	Severity severity() const { return m_severity; }

	/// Sets current trace severity.
	void  setSeverity(Severity value) { m_severity = value; }

public:
	/// Writes generic trace message.
	void f(Severity severity, const wchar_t* fmt, ...);

	/// Writes a DEBUG message.
	void d(const wchar_t* fmt, ...);

	/// Writes an INFO message.
	void i(const wchar_t* fmt, ...);

	/// Writes a WARNING message.
	void w(const wchar_t* fmt, ...);

	/// Writes an ERROR message.
	void e(const wchar_t* fmt, ...);

protected:
	virtual void write(Severity severity, const wchar_t* fmt, va_list args);

protected:
	/// Configured severity for the trace.
	Severity m_severity;
};

/// Global tracing instance.
extern Trace& _trace;

#endif // !N_NETWORK_H_
