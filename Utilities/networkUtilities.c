/**
 * Contains common utility functions for the Light Year Wars game's netcode.
 * @file Utilities/networkUtilities.c
 * @author abmize
 */

#include "Utilities/networkUtilities.h"

/**
 * Initializes Winsock.
 * @return true if successful, false otherwise.
 */
bool InitializeWinsock() {
    // WSADATA is a structure that is filled with information about the Windows Sockets implementation.
    WSADATA wsaData;
    // WSAStartup is a function that initializes the Windows Sockets DLL.
    // Here, we are requesting version 2.2 of the Windows Sockets specification.
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        // WSAGetLastError retrieves the error code for the last Windows Sockets operation that failed.
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}

/**
 * Creates a UDP socket.
 * @return The created socket, or INVALID_SOCKET on failure.
 */
SOCKET CreateUDPSocket() {
    // AF_INET specifies the address family (IPv4)
    // SOCK_DGRAM specifies the socket type (datagram/UDP)
    // IPPROTO_UDP specifies the protocol (UDP)
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
    }
    return sock;
}

/**
 * Binds a socket to a specific port on INADDR_ANY.
 * @param sock The socket to bind.
 * @param port The port to bind to.
 * @return true if successful, false otherwise.
 */
bool BindSocket(SOCKET sock, int port) {
    // Set up the local address structure
    SOCKADDR_IN local_address;
    // AF_INET says that the address family is IPv4 for this socket
    local_address.sin_family = AF_INET;
    // The assignment of the port number is wrapped in a call to htons (host to network short), 
    // this converts the port number from whatever endianness the executing machine has, to big-endian.
    local_address.sin_port = htons(port);
    // INADDR_ANY means that we will accept messages sent to any of the local machine's IP addresses 
    // (of course, it has to be sent to the correct port as well, 6767 in this case).
    local_address.sin_addr.s_addr = INADDR_ANY;

    // bind() takes a socket, a pointer to a sockaddr structure (we have to cast our SOCKADDR_IN pointer to a SOCKADDR pointer),
    // and the size of the address structure.
    if (bind(sock, (SOCKADDR*)&local_address, sizeof(local_address)) == SOCKET_ERROR) {
        // The documentation also says we should have a call to WSACleanup when we're done with Winsock
        // but apparently when exiting, Windows automatically cleans up anyway.
        // So unless we want to keep running the program after socket errors, it won't make much difference.
        printf("bind failed: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}

/**
 * Sets a socket to non-blocking mode.
 * @param sock The socket to modify.
 * @return true if successful, false otherwise.
 */
bool SetNonBlocking(SOCKET sock) {
    // recvfrom is a blocking function, so the program will wait at that call until a message is received.
    // We don't want it to block though, so we set the socket to non-blocking mode.
    u_long mode = 1;
    // ioctlsocket is used to control aspects of the socket's behavior.
    // The first argument is the socket to modify.
    // The second argument is the command to execute (FIONBIO to set non-blocking mode).
    // The third argument is a pointer to the value to set (mode).
    // Had mode been 0, it would set the socket to blocking mode.
    // With it set to 1, recvfrom will return immediately if there is no data to read.
    // bytes_received will be SOCKET_ERROR and WSAGetLastError will return WSAEWOULDBLOCK.
    if (ioctlsocket(sock, FIONBIO, &mode) != 0) {
        printf("ioctlsocket failed: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}

/**
 * Sets the receive timeout for a socket.
 * @param sock The socket to modify.
 * @param milliseconds The timeout in milliseconds.
 * @return true if successful, false otherwise.
 */
bool SetSocketTimeout(SOCKET sock, int milliseconds) {
    DWORD timeout = milliseconds;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
        printf("setsockopt failed: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}

/**
 * Creates a SOCKADDR_IN from an IP string and port.
 * @param ip The IP address string.
 * @param port The port number.
 * @param outAddr Pointer to the SOCKADDR_IN structure to fill.
 * @return true if successful, false otherwise.
 */
bool CreateAddress(const char* ip, int port, SOCKADDR_IN* outAddr) {
    if (!outAddr) return false;
    memset(outAddr, 0, sizeof(SOCKADDR_IN));
    outAddr->sin_family = AF_INET;
    outAddr->sin_port = htons(port);
    if (InetPtonA(AF_INET, ip, &outAddr->sin_addr) != 1) {
        printf("Invalid address / Address not supported\n");
        return false;
    }
    return true;
}
