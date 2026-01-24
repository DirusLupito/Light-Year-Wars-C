/**
 * Implementation file for the player object.
 * A player represents a user in the game.
 * Each player is uniquely associated with a network/IPv4 address
 * and a faction they uniquely control.
 * @file Objects/player.c
 * Author information retained from original file.
 */

#include "Objects/player.h"
#include "Utilities/networkUtilities.h"

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
    PlayerSetFaction(player, faction);
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

    // Initialize inactivity timer to zero.
    // Used to track time since last packet from this player
    // on the server side for the sake of inactivity timeouts.
    player->inactivitySeconds = 0.0f;

    // Clear the player name so stale data is never rendered before a join request arrives.
    PlayerSetName(player, "");
}

/**
 * Assigns a faction to the player and records its identifier for later rebinding.
 * @param player The player to update.
 * @param faction The faction to assign.
 */
void PlayerSetFaction(Player *player, const Faction *faction) {
    // Basic validation of input pointer.
    if (player == NULL) {
        return;
    }

    player->faction = faction;
    
    // Store the faction ID, or -1 if no faction is assigned.
    player->factionId = (faction != NULL) ? faction->id : -1;
}

/**
 * Validates a proposed player name for ASCII range and length bounds.
 * This keeps UI and network code from accepting names that would break rendering or packets.
 * @param name Candidate null-terminated name string.
 * @return true if the name meets length and character requirements, false otherwise.
 */
bool PlayerValidateName(const char *name) {
    if (name == NULL) {
        return false;
    }

    // Ensure length is within bounds.
    size_t len = strnlen(name, PLAYER_NAME_MAX_LENGTH + 1);
    if (len < PLAYER_NAME_MIN_LENGTH || len > PLAYER_NAME_MAX_LENGTH) {
        return false;
    }

    // Ensure all characters are printable ASCII.
    for (size_t i = 0; i < len; ++i) {
        unsigned char ch = (unsigned char)name[i];
        if (ch < 32u || ch > 126u) {
            return false;
        }
    }

    return true;
}

/**
 * Assigns a validated name to the player or clears it if invalid.
 * Clearing on invalid input avoids leaking untrusted bytes into packets or UI text.
 * @param player The player to update.
 * @param name Candidate null-terminated name string.
 */
void PlayerSetName(Player *player, const char *name) {
    if (player == NULL) {
        return;
    }

    // If the name is invalid, we clear the player's name.
    if (!PlayerValidateName(name)) {
        player->name[0] = '\0';
        return;
    }

    // Copy the validated name into the player's name buffer.
    size_t len = strnlen(name, PLAYER_NAME_MAX_LENGTH);
    memcpy(player->name, name, len);
    player->name[len] = '\0';
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