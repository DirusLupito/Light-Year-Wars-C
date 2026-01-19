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
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>

#include "Objects/level.h"
#include "Objects/player.h"

typedef struct Level Level;
typedef struct Player Player;
typedef struct LevelPacketBuffer LevelPacketBuffer;

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

/**
 * Sends a packet to a specific player containing the provided packet buffer,
 * detailing some sort of update about the level state. See the LevelPacketBuffer
 * structure definition in level.h for more information.
 * @param player The player to send the packet to.
 * @param sock The socket to use for sending.
 * @param packet The packet buffer to send.
 */
void SendPacketToPlayer(Player *player, SOCKET sock, const LevelPacketBuffer *packet);

/**
 * Sends the full level packet to a specific player.
 * Typically used when a player first joins the game or needs a full state update,
 * so they can synchronize not only the dynamic elements but also the static layout of the level.
 * @param player The player to send the packet to.
 * @param sock The socket to use for sending.
 * @param level The level whose full state is to be sent.
 * @return true if the packet was sent successfully, false otherwise.
 */
bool SendFullPacketToPlayer(Player *player, SOCKET sock, const Level *level);

/**
 * Broadcasts snapshot packets to all connected players.
 * Used to keep all the clients up to date with the important dynamic state of the level.
 * @param sock The socket to use for sending.
 * @param level The level whose snapshot is to be sent.
 * @param players The array of players to send the snapshot to.
 * @param playerCount The number of players in the array.
 */
void BroadcastSnapshots(SOCKET sock, const Level *level, Player *players, size_t playerCount);

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
    int32_t shipCount, int32_t ownerFactionId, unsigned int shipSpawnRNGState);

#endif // _NETWORK_UTILITIES_H_
