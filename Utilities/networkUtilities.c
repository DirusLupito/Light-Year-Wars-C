/**
 * Contains common utility functions for the Light Year Wars game's netcode.
 * @file Utilities/networkUtilities.c
 * @author abmize
 */

#include "Utilities/networkUtilities.h"

static void SendAssignmentPacket(Player *player, SOCKET sock);

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

/**
 * Sends a packet to a specific player containing the provided packet buffer,
 * detailing some sort of update about the level state. See the LevelPacketBuffer
 * structure definition in level.h for more information.
 * @param player The player to send the packet to.
 * @param sock The socket to use for sending.
 * @param packet The packet buffer to send.
 */
void SendPacketToPlayer(Player *player, SOCKET sock, const LevelPacketBuffer *packet) {
    // Basic validation of input pointers.
    if (player == NULL || packet == NULL || packet->data == NULL) {
        return;
    }

    // Check that the packet size is within valid limits for sendto.
    if (packet->size > (size_t)INT_MAX) {
        printf("Packet too large to send (size=%zu).\n", packet->size);
        return;
    }

    // Send the packet data to the player's address using sendto.
    int result = sendto(sock,
        (const char *)packet->data,
        (int)packet->size,
        0,
        (SOCKADDR *)&player->address,
        (int)sizeof(player->address));

    // Report any send errors.
    if (result == SOCKET_ERROR) {
        printf("sendto failed: %d\n", WSAGetLastError());
    }
}

/**
 * Sends the full level packet to a specific player.
 * Typically used when a player first joins the game or needs a full state update,
 * so they can synchronize not only the dynamic elements but also the static layout of the level.
 * @param player The player to send the packet to.
 * @param sock The socket to use for sending.
 * @param level The level whose full state is to be sent.
 * @return true if the packet was sent successfully, false otherwise.
 */
bool SendFullPacketToPlayer(Player *player, SOCKET sock, const Level *level) {
    // Basic validation of input pointers.
    if (player == NULL || level == NULL) {
        return false;
    }

    // Create the buffer for the full level packet.
    LevelPacketBuffer packet;
    if (!LevelCreateFullPacketBuffer(level, &packet)) {
        printf("Failed to build full level packet.\n");
        return false;
    }

    // Send the packet data to the player's address using sendto.
    bool success = false;
    if (packet.size <= (size_t)INT_MAX) {
        int result = sendto(sock,
            (const char *)packet.data,
            (int)packet.size,
            0,
            (SOCKADDR *)&player->address,
            (int)sizeof(player->address));

        if (result != SOCKET_ERROR) {
            success = true;
            player->awaitingFullPacket = false;
            SendAssignmentPacket(player, sock);
        } else {
            printf("sendto failed: %d\n", WSAGetLastError());
        }
    } else {
        printf("Full packet too large to send (size=%zu).\n", packet.size);
    }

    // Free the memory allocated for the packet buffer, as it is no longer needed.
    LevelPacketBufferRelease(&packet);
    return success;
}

/**
 * Broadcasts snapshot packets to all connected players.
 * Used to keep all the clients up to date with the important dynamic state of the level.
 * @param sock The socket to use for sending.
 * @param level The level whose snapshot is to be sent.
 * @param players The array of players to send the snapshot to.
 * @param playerCount The number of players in the array.
 */
void BroadcastSnapshots(SOCKET sock, const Level *level, Player *players, size_t playerCount) {
    // Basic validation of input pointers.
    if (players == NULL || level == NULL || playerCount == 0) {
        return;
    }

    // Create the buffer for the snapshot packet.
    // While there is only one packet, it is sent to multiple players,
    // for both the sake of efficiency and to ensure all players
    // receive the exact same snapshot data. No player should 
    // ever need a different snapshot than any other player
    // with the current implementation.
    // 
    // Were something like fog of war implemented in the future,
    // it might be necessary to create different snapshot packets
    // for different players, depending on what they are allowed to see.
    LevelPacketBuffer packet;
    if (!LevelCreateSnapshotPacketBuffer(level, &packet)) {
        printf("Failed to build snapshot packet.\n");
        return;
    }

    // Iterate over all players and send them the snapshot packet.
    for (size_t i = 0; i < playerCount; ++i) {
        SendPacketToPlayer(&players[i], sock, &packet);
    }

    // Free the memory allocated for the packet buffer, as it is no longer needed.
    LevelPacketBufferRelease(&packet);
}

/**
 * Broadcasts a fleet launch packet to all connected players.
 * Used to inform clients of a new fleet being launched from one planet to another
 * so they can simulate its movement. This approach allows clients to remain synchronized
 * with the server's authoritative game state, while also minimizing the amount of data sent,
 * and letting them see the starships (which they would otherwise need several packets to describe).
 * @param sock The socket to use for sending.
 * @param players The array of players to send the packet to.
 * @param playerCount The number of players in the array.
 * @param originPlanetIndex The index of the origin planet.
 * @param destinationPlanetIndex The index of the destination planet.
 * @param shipCount The number of ships being launched.
 * @param ownerFactionId The faction ID of the fleet owner.
 * @param shipSpawnRNGState The RNG state used to randomize ship spawn positions.
 *                          Sent to clients so they can simulate the same starship spawns.
 */
void BroadcastFleetLaunch(SOCKET sock, Player *players, size_t playerCount,
    int32_t originPlanetIndex, int32_t destinationPlanetIndex,
    int32_t shipCount, int32_t ownerFactionId, unsigned int shipSpawnRNGState) {
    
    // Basic validation of input pointers.
    if (players == NULL || playerCount == 0) {
        return;
    }

    // No point in sending a packet with zero or negative ships.
    if (shipCount <= 0) {
        return;
    }

    // Create the fleet launch packet.
    // While there is only one packet, it is sent to multiple players,
    // for both the sake of efficiency and to ensure all players
    // receive the exact same snapshot data. No player should 
    // ever need a different snapshot than any other player
    // with the current implementation.
    // 
    // Were something like fog of war implemented in the future,
    // it might be necessary to create different snapshot packets
    // for different players, depending on whether they are allowed to see
    // a faraway fleet launch or not.
    LevelFleetLaunchPacket packet = {0};
    packet.type = LEVEL_PACKET_TYPE_FLEET_LAUNCH;
    packet.originPlanetIndex = originPlanetIndex;
    packet.destinationPlanetIndex = destinationPlanetIndex;
    packet.shipCount = shipCount;
    packet.ownerFactionId = ownerFactionId;
    packet.shipSpawnRNGState = shipSpawnRNGState;

    // Iterate over all players and send them the fleet launch packet.
    for (size_t i = 0; i < playerCount; ++i) {
        int result = sendto(sock,
            (const char *)&packet,
            (int)sizeof(packet),
            0,
            (SOCKADDR *)&players[i].address,
            (int)sizeof(players[i].address));

        if (result == SOCKET_ERROR) {
            printf("sendto failed: %d\n", WSAGetLastError());
        }
    }
}

/**
 * Sends a level assignment packet to a specific player.
 * This packet informs the player of their assigned faction ID.
 * @param player The player to send the packet to.
 * @param sock The socket to use for sending.
 */
static void SendAssignmentPacket(Player *player, SOCKET sock) {
    // Basic validation of input pointers.
    if (player == NULL) {
        return;
    }

    // Create the level assignment packet.
    LevelAssignmentPacket packet = {0};
    packet.type = LEVEL_PACKET_TYPE_ASSIGNMENT;
    packet.factionId = player->faction != NULL ? (int32_t)player->faction->id : -1;

    // Send the assignment packet to the player's address using sendto.
    int result = sendto(sock,
        (const char *)&packet,
        (int)sizeof(packet),
        0,
        (SOCKADDR *)&player->address,
        (int)sizeof(player->address));

    if (result == SOCKET_ERROR) {
        printf("sendto failed: %d\n", WSAGetLastError());
    }
}
