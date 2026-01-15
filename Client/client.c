/**
 * Companion client for Light Year Wars C.
 * Sends greetings to the server and waits for "Ok".
 * @author abmize
 * @file client.c
 */
#include "Client/client.h"

// Forward declarations of static functions
static bool HandleFullPacket(const uint8_t *data, size_t length);
static void HandleSnapshotPacket(const uint8_t *data, size_t length);

/**
 * Helper function to validate that the remaining packet length
 * is at least the expected size. Used to prevent reading beyond packet bounds.
 * @param length The remaining length of the packet.
 * @param expected The expected minimum size.
 * @return true if the remaining length is sufficient, false otherwise.
 */
static bool ValidateRemaining(size_t length, size_t expected) {
    // Simple check, just use the size_t comparison.
    if (length < expected) {
        printf("Packet truncated (expected %zu bytes, got %zu).\n", expected, length);
        return false;
    }
    return true;
}

/**
 * Handles a full level packet received from the server.
 * Parses and prints the contents of the packet.
 * For reference, a full level packet is one that contains
 * all the data about the level, including factions, planets, and starships.
 * Compaed to a snapshot packet, this is a super-set of data,
 * since a snapshot only contains the dynamic state of planets and starships.
 * (Planets don't move, or change their max size, factions don't change color, etc.)
 * @param data Pointer to the packet data.
 * @param length Length of the packet data in bytes.
 * @return true if the packet was handled successfully, false otherwise.
 */
static bool HandleFullPacket(const uint8_t *data, size_t length) {

    // First we validate that the packet is large enough to hold the header.
    // We need at least the header to proceed.
    if (length < sizeof(LevelFullPacket)) {
        printf("Full packet smaller than header.\n");
        return false;
    }

    // Now we can safely cast the data pointer to our header structure.
    const LevelFullPacket *header = (const LevelFullPacket *)data;

    // Let's verify that the packet type is what we expect.
    if (header->type != LEVEL_PACKET_TYPE_FULL) {
        printf("Unexpected packet type %u for full handler.\n", header->type);
        return false;
    }

    // Since we've been told by the header that we're getting a full packet,
    // we can now read the counts of factions, planets, and starships.
    size_t factionCount = header->factionCount;
    size_t planetCount = header->planetCount;
    size_t starshipCount = header->starshipCount;

    // Memory size required to store the entire packet is a simple formula,
    // we just multiply the counts of each object factions, planets, and starships
    // by their respective sizes and then add this all up along with the header size.
    size_t required = sizeof(LevelFullPacket)
        + factionCount * sizeof(LevelPacketFactionInfo)
        + planetCount * sizeof(LevelPacketPlanetFullInfo)
        + starshipCount * sizeof(LevelPacketStarshipInfo);

    // Now we perform another check in case of any network chicanery
    // that would cause us to read beyond the packet bounds.
    if (!ValidateRemaining(length, required)) {
        return false;
    }

    // Now that we know the packet is valid and large enough,
    // we can parse the contents.

    // The cursor represents our current position in the packet data.
    // (like a flashing cursor you might see when typing text in notepad)
    const uint8_t *cursor = data + sizeof(LevelFullPacket);

    // The packet order can be observed from the struct:
    // typedef struct LevelFullPacket {
    	// uint32_t type;
    	// float width;
    	// float height;
    	// uint32_t factionCount;
    	// uint32_t planetCount;
    	// uint32_t starshipCount;
    // } LevelFullPacket;
    // So after the header, we have factions, then planets, then starships.
    // #pragma pack(push, 1) and #pragma pack(pop) ensure there is no padding in these structs,
    // so we can safely calculate offsets based on sizes alone.
    const LevelPacketFactionInfo *factions = (const LevelPacketFactionInfo *)cursor;
    cursor += factionCount * sizeof(LevelPacketFactionInfo);
    const LevelPacketPlanetFullInfo *planets = (const LevelPacketPlanetFullInfo *)cursor;
    cursor += planetCount * sizeof(LevelPacketPlanetFullInfo);
    const LevelPacketStarshipInfo *starships = (const LevelPacketStarshipInfo *)cursor;

    // For now, we simply print out the contents of the packet to the console.
    printf("=== Full Level Packet ===\n");

    // Print out the level dimensions 
    printf("Dimensions: %.1f x %.1f\n", (double)header->width, (double)header->height);

    // Print out faction information
    printf("Factions: %zu\n", factionCount);
    for (size_t i = 0; i < factionCount; ++i) {
        printf("  Faction %d color=(%.2f, %.2f, %.2f, %.2f)\n",
            factions[i].id,
            (double)factions[i].color[0],
            (double)factions[i].color[1],
            (double)factions[i].color[2],
            (double)factions[i].color[3]);
    }

    // Print out planet information
    printf("Planets: %zu\n", planetCount);
    for (size_t i = 0; i < planetCount; ++i) {
        printf("  Planet %zu pos=(%.1f, %.1f) max=%.1f current=%.1f owner=%d claimant=%d\n",
            i,
            (double)planets[i].position.x,
            (double)planets[i].position.y,
            (double)planets[i].maxFleetCapacity,
            (double)planets[i].currentFleetSize,
            planets[i].ownerId,
            planets[i].claimantId);
    }

    // Print out starship information
    printf("Starships: %zu\n", starshipCount);
    for (size_t i = 0; i < starshipCount; ++i) {
        printf("  Ship %zu owner=%d target=%d pos=(%.1f, %.1f) vel=(%.1f, %.1f)\n",
            i,
            starships[i].ownerId,
            starships[i].targetPlanetIndex,
            (double)starships[i].position.x,
            (double)starships[i].position.y,
            (double)starships[i].velocity.x,
            (double)starships[i].velocity.y);
    }

    return true;
}

/**
 * Handles a snapshot level packet received from the server.
 * Parses and prints the contents of the packet.
 * A snapshot packet contains only the dynamic state of planets and starships.
 * @param data Pointer to the packet data.
 * @param length Length of the packet data in bytes.
 */
static void HandleSnapshotPacket(const uint8_t *data, size_t length) {

    // First we validate that the packet is large enough to hold the header.
    // We need at least the header to proceed.
    if (length < sizeof(LevelSnapshotPacket)) {
        printf("Snapshot packet smaller than header.\n");
        return;
    }

    // Now we can safely cast the data pointer to our header structure.
    const LevelSnapshotPacket *header = (const LevelSnapshotPacket *)data;
    if (header->type != LEVEL_PACKET_TYPE_SNAPSHOT) {
        printf("Unexpected packet type %u for snapshot handler.\n", header->type);
        return;
    }

    // Now that we know the packet is a snapshot,
    // we can read the counts of planets and starships.
    size_t planetCount = header->planetCount;
    size_t starshipCount = header->starshipCount;


    // Memory size required to store the entire packet is a simple formula,
    // we just multiply the counts of each object planets and starships
    // by their respective sizes and then add this all up along with the header size.
    size_t required = sizeof(LevelSnapshotPacket)
        + planetCount * sizeof(LevelPacketPlanetSnapshotInfo)
        + starshipCount * sizeof(LevelPacketStarshipInfo);

    // Again, like with the full packet, we perform a check
    // in case of any network corruption that would cause us to read beyond the packet
    if (!ValidateRemaining(length, required)) {
        return;
    }

    // And we also reuse the same cursor technique to parse the packet contents.
    // The packet order can be observed from the struct:
    // typedef struct LevelSnapshotPacket {
    // 	uint32_t type;
    // 	uint32_t planetCount;
    // 	uint32_t starshipCount;
    // } LevelSnapshotPacket;
    // So after the header, we have planets, then starships.
    const uint8_t *cursor = data + sizeof(LevelSnapshotPacket);
    const LevelPacketPlanetSnapshotInfo *planets = (const LevelPacketPlanetSnapshotInfo *)cursor;
    cursor += planetCount * sizeof(LevelPacketPlanetSnapshotInfo);
    const LevelPacketStarshipInfo *starships = (const LevelPacketStarshipInfo *)cursor;

    // For now, we simply print out the contents of the packet to the console.
    printf("=== Snapshot Packet ===\n");

    // Print out planet information
    printf("Planets: %zu\n", planetCount);
    for (size_t i = 0; i < planetCount; ++i) {
        printf("  Planet %zu owner=%d claimant=%d current=%.1f\n",
            i,
            planets[i].ownerId,
            planets[i].claimantId,
            (double)planets[i].currentFleetSize);
    }

    // Print out starship information
    printf("Starships: %zu\n", starshipCount);
    for (size_t i = 0; i < starshipCount; ++i) {
        printf("  Ship %zu owner=%d target=%d pos=(%.1f, %.1f) vel=(%.1f, %.1f)\n",
            i,
            starships[i].ownerId,
            starships[i].targetPlanetIndex,
            (double)starships[i].position.x,
            (double)starships[i].position.y,
            (double)starships[i].velocity.x,
            (double)starships[i].velocity.y);
    }
}

/**
 * Main entry point for the client application.
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure.
 */
int main() {
    // Force printf to flush immediately
    setvbuf(stdout, NULL, _IONBF, 0);

    // Ask user for server IP and port
    char serverIp[INET_ADDRSTRLEN];
    int serverPort;
    printf("Enter server IP address: ");
    scanf("%s", serverIp);
    printf("Enter server port: ");
    scanf("%d", &serverPort);

    // Initialize Winsock
    if (!InitializeWinsock()) {
        return EXIT_FAILURE;
    }

    // Create socket
    SOCKET clientSocket = CreateUDPSocket();
    if (clientSocket == INVALID_SOCKET) {
        WSACleanup();
        return EXIT_FAILURE;
    }

    // Set receive timeout to 1 second
    if (!SetSocketTimeout(clientSocket, 1000)) {
        printf("setsockopt failed\n");
    }

    // Server address setup
    struct sockaddr_in serverAddress;
    if (!CreateAddress(serverIp, serverPort, &serverAddress)) {
        closesocket(clientSocket);
        WSACleanup();
        return EXIT_FAILURE;
    }

    // Send initial JOIN message to server
    // to request joining the game.
    // The server will respond with either a full level packet
    // or a SERVER_FULL message if it cannot accept more players.
    printf("Client started. Sending JOIN to %s:%d\n", serverIp, serverPort);

    // Handle sending the JOIN message
    const char *joinMessage = "JOIN";
    int sendResult = sendto(clientSocket, joinMessage, (int)strlen(joinMessage), 0,
        (struct sockaddr *)&serverAddress, sizeof(serverAddress));

    // Check for send errors
    if (sendResult == SOCKET_ERROR) {
        printf("Initial sendto failed: %d\n", WSAGetLastError());
    }

    // Main receive loop

    // awaitingFull indicates whether we are still waiting for the full packet.
    bool awaitingFull = true;

    // running indicates whether we should keep running the main loop.
    bool running = true;

    // Buffer to hold incoming packets
    char buffer[1024];

    while (running) {
        // Receive data from server

        struct sockaddr_in fromAddress;
        int fromLength = sizeof(fromAddress);

        int bytesReceived = recvfrom(clientSocket,
            buffer,
            (int)sizeof(buffer),
            0,
            (struct sockaddr *)&fromAddress,
            &fromLength);

        // If recvfrom failed, we check if it was a timeout
        if (bytesReceived == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error == WSAETIMEDOUT) {
                if (awaitingFull) {
                    // If we time out while waiting for the full packet,
                    // we resend the JOIN message to the server.
                    // Maybe the server didn't get it the first time.
                    sendResult = sendto(clientSocket, joinMessage, (int)strlen(joinMessage), 0,
                        (struct sockaddr *)&serverAddress, sizeof(serverAddress));

                    // Check for send errors
                    if (sendResult == SOCKET_ERROR) {
                        printf("Resend JOIN failed: %d\n", WSAGetLastError());
                    }
                }
                // Otherwise, we know that we've just timed out waiting for data,
                // so we simply continue the loop.
                continue;
            }

            // Some other error occurred, we print it and exit the loop
            printf("recvfrom failed: %d\n", error);
            break;
        }

        if (bytesReceived == 0) {
            // No data received, continue waiting
            continue;
        }

        // Process the received packet
        // If the packet is smaller than 4 bytes, 
        // we assume it's part of a text message
        // like "SERVER_FULL".
        // We will add however much of the assumed (sub)string we received
        // as a null-terminated string to the buffer.
        // As we piece together the string, we also print it out.
        // Furthermore, if it turns out to be "SERVER_FULL",
        // we exit the main loop.
        if (bytesReceived < (int)sizeof(uint32_t)) {
            // Handle as text message
            // We cap the received size to the buffer size minus one
            // to leave space for the null terminator.
            int capped = bytesReceived < (int)sizeof(buffer) - 1 ? bytesReceived : (int)sizeof(buffer) - 1;
            buffer[capped] = '\0';

            // Print the server message
            printf("Server says: %s\n", buffer);
            if (strcmp(buffer, "SERVER_FULL") == 0) {
                // In the special case of SERVER_FULL,
                // we exit the main loop.
                running = false;
            }
            continue;
        }

        // At this point we have at least 4 bytes,
        // so we can safely read the packet type.
        const uint8_t *data = (const uint8_t *)buffer;

        // Stores the packet type in a local variable
        uint32_t packetType;

        // Copy the packet type from the data.
        // Packet type better be aligned properly here,
        // and better be the first 4 bytes of the packet,
        // in the same endianness as the client,
        // since we are directly copying from the data buffer.
        memcpy(&packetType, data, sizeof(uint32_t));

        // Now we can handle the packet based on its type.
        if (packetType == LEVEL_PACKET_TYPE_FULL) {
            // It's a full level packet, telling us
            // the entire state of the level.
            if (HandleFullPacket(data, (size_t)bytesReceived)) {
                awaitingFull = false;
            }
        } else if (packetType == LEVEL_PACKET_TYPE_SNAPSHOT) {
            // It's a snapshot packet, giving us
            // an update on the dynamic state of the level.
            if (awaitingFull) {
                printf("Snapshot received before full packet. Waiting for full packet...\n");
                continue;
            }
            HandleSnapshotPacket(data, (size_t)bytesReceived);
        } else {
            // Something went wrong, unknown packet type.
            // Debug print the unknown type.
            printf("Unknown packet type %u (length=%d).\n", packetType, bytesReceived);
        }
    }

    // At this point, we're done running the main loop,
    // so we clean up resources and exit.
    closesocket(clientSocket);
    WSACleanup();
    return EXIT_SUCCESS;
}
