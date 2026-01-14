/**
 * Header file for the network utility functions file.
 * @file Utilities/networkUtilities.h
 * @author abmize
 */
#ifndef _NETWORK_UTILITIES_H_
#define _NETWORK_UTILITIES_H_

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdbool.h>

/**
 * Initializes Winsock.
 * @return true if successful, false otherwise.
 */
bool InitializeWinsock();

/**
 * Creates a UDP socket.
 * @return The created socket, or INVALID_SOCKET on failure.
 */
SOCKET CreateUDPSocket();

/**
 * Binds a socket to a specific port on INADDR_ANY.
 * @param sock The socket to bind.
 * @param port The port to bind to.
 * @return true if successful, false otherwise.
 */
bool BindSocket(SOCKET sock, int port);

/**
 * Sets a socket to non-blocking mode.
 * @param sock The socket to modify.
 * @return true if successful, false otherwise.
 */
bool SetNonBlocking(SOCKET sock);

/**
 * Sets the receive timeout for a socket.
 * @param sock The socket to modify.
 * @param milliseconds The timeout in milliseconds.
 * @return true if successful, false otherwise.
 */
bool SetSocketTimeout(SOCKET sock, int milliseconds);

/**
 * Creates a SOCKADDR_IN from an IP string and port.
 * @param ip The IP address string.
 * @param port The port number.
 * @param outAddr Pointer to the SOCKADDR_IN structure to fill.
 * @return true if successful, false otherwise.
 */
bool CreateAddress(const char* ip, int port, SOCKADDR_IN* outAddr);

#endif // _NETWORK_UTILITIES_H_