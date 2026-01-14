/**
 * Companion client for Light Year Wars C.
 * Sends greetings to the server and waits for "Ok".
 * @author abmize
 * @file client.c
 */
#include "Client/client.h"

int main() {
    // Force printf to flush immediately
    setvbuf(stdout, NULL, _IONBF, 0);

    // Ask user for server IP and port
    char Server_IP[INET_ADDRSTRLEN];
    int Server_Port;
    printf("Enter server IP address: ");
    scanf("%s", Server_IP);
    printf("Enter server port: ");
    scanf("%d", &Server_Port);

    SOCKET client_socket;
    struct sockaddr_in server_addr;
    char buffer[512];
    int server_addr_len = sizeof(server_addr);
    const char *message = "Hello from client";
    bool received_ok = false;

    // Initialize Winsock
    if (!InitializeWinsock()) {
        return EXIT_FAILURE;
    }

    // Create socket
    client_socket = CreateUDPSocket();
    if (client_socket == INVALID_SOCKET) {
        WSACleanup();
        return EXIT_FAILURE;
    }

    // Set receive timeout to 1 second
    if (!SetSocketTimeout(client_socket, 1000)) {
        printf("setsockopt failed\n");
    }

    // Server address setup
    if (!CreateAddress(Server_IP, Server_Port, &server_addr)) {
        closesocket(client_socket);
        WSACleanup();
        return EXIT_FAILURE;
    }

    printf("Client started. Sending to %s:%d\n", Server_IP, Server_Port);

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
