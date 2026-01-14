/**
 * Companion client for Light Year Wars C.
 * Sends greetings to the server and waits for "Ok".
 * @file client.c
 */
#define UNICODE
#define _UNICODE
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define SERVER_PORT 6767
#define SERVER_IP "107.13.173.255"

int main() {
    // Force printf to flush immediately
    setvbuf(stdout, NULL, _IONBF, 0);

    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    char buffer[512];
    int server_addr_len = sizeof(server_addr);
    const char *message = "Hello from client";
    bool received_ok = false;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return EXIT_FAILURE;
    }

    // Create socket
    client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client_socket == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return EXIT_FAILURE;
    }

    // Set receive timeout to 1 second
    DWORD timeout = 1000;
    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
        printf("setsockopt failed\n");
    }

    // Server address setup
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    // Convert IP address from string to binary form
    // Using InetPtonA explicitly since we are passing a char* string
    if (InetPtonA(AF_INET, SERVER_IP, &server_addr.sin_addr) != 1) {
        printf("Invalid address / Address not supported\n");
        closesocket(client_socket);
        WSACleanup();
        return EXIT_FAILURE;
    }

    printf("Client started. Sending to %s:%d\n", SERVER_IP, SERVER_PORT);

    while (!received_ok) {
        // Send message
        printf("Sending message...\n");
        int send_result = sendto(client_socket, message, (int)strlen(message), 0, (struct sockaddr *)&server_addr, server_addr_len);
        if (send_result == SOCKET_ERROR) {
            printf("sendto failed: %d\n", WSAGetLastError());
        }

        // Wait for response
        struct sockaddr_in from_addr;
        int from_len = sizeof(from_addr);
        
        // Reset buffer
        memset(buffer, 0, sizeof(buffer));

        int bytes_received = recvfrom(client_socket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&from_addr, &from_len);

        if (bytes_received != SOCKET_ERROR) {
            buffer[bytes_received] = '\0';
            printf("Received: %s\n", buffer);

            // Check if we got "Ok"
            if (strcmp(buffer, "Ok") == 0) {
                printf("Received 'Ok' from server. Exiting.\n");
                received_ok = true;
            }
        } else {
            int error = WSAGetLastError();
            if (error == WSAETIMEDOUT) {
                printf("Timeout waiting for response. Retrying...\n");
            } else {
                printf("recvfrom failed: %d\n", error);
            }
        }

        if (!received_ok) {
            Sleep(1000); // Wait a bit before retrying if we didn't succeed
        }
    }

    closesocket(client_socket);
    WSACleanup();
    return EXIT_SUCCESS;
}
