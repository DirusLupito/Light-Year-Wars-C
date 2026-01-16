/**
 * Implementation file for the player object.
 * A player represents a user in the game.
 * Each player is uniquely associated with a network/IPv4 address
 * and a faction they uniquely control.
 * @file Objects/player.c
 * Author information retained from original file.
 */

#include "Objects/player.h"

#include <string.h>

/**
 * Initializes a player with the given faction and network address.
 * @param player The player instance to initialize.
 * @param faction The faction controlled by the player.
 * @param address The network endpoint for the player.
 */
void PlayerInit(Player *player, const Faction *faction, const SOCKADDR_IN *address) {
    // Basic validation of input pointer.
    if (player == NULL) {
        return;
    }

    // We set the player's faction and address assuming the faction pointer is valid.
    // If the address pointer is NULL, we initialize the address to a default value.
    player->faction = faction;
    if (address != NULL) {
        player->address = *address;
    } else {
        memset(&player->address, 0, sizeof(player->address));
        player->address.sin_family = AF_INET;
    }

    // By default, we assume the player is awaiting a full packet,
    // since they don't have any other way to get the static level
    // data (after all, snapshots only send dynamic data).
    player->awaitingFullPacket = true;
}

/**
 * Updates the stored endpoint for the player.
 * @param player The player to update.
 * @param address The new endpoint information.
 */
void PlayerUpdateEndpoint(Player *player, const SOCKADDR_IN *address) {
    // Basic validation of input pointers.
    if (player == NULL || address == NULL) {
        return;
    }

    // sin_family should remain unchanged as it indicates the address family (IPv4).
    player->address.sin_family = address->sin_family;

    // sin_port is the port number, and sin_addr is the IPv4 address.
    player->address.sin_port = address->sin_port;
    player->address.sin_addr = address->sin_addr;
}

/**
 * Checks whether the player matches the supplied address (by IPv4 address).
 * @param player The player to compare.
 * @param address The address to compare against.
 * @return true if the addresses share the same IPv4, false otherwise.
 */
bool PlayerMatchesAddress(const Player *player, const SOCKADDR_IN *address) {
    // Basic validation of input pointers.
    if (player == NULL || address == NULL) {
        return false;
    }

    // We compare only the IPv4 addresses (sin_addr).
    // This considers two players from the same IP but different ports
    // as the same player.
    return player->address.sin_addr.s_addr == address->sin_addr.s_addr;
}