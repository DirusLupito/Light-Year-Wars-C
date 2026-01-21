/**
 * Header file for the player object.
 * @file Objects/player.h
 * @author abmize
 */
#ifndef _PLAYER_H_
#define _PLAYER_H_

#include <stdbool.h>
#include <stdint.h>
#include "Objects/faction.h"
#include "Utilities/networkUtilities.h"

// A player represents a user in the game.
// Each player is uniquely associated with a network/IPv4 address,
// a faction they uniquely control, whether they are awaiting full level data,
// and an inactivity timer used for timeouts on the server side.
typedef struct Player {
    const Faction *faction;
    int factionId;
    SOCKADDR_IN address;
    bool awaitingFullPacket;
    float inactivitySeconds;
} Player;

/**
 * Initializes a player with the given faction and network address.
 * @param player The player instance to initialize.
 * @param faction The faction controlled by the player.
 * @param address The network endpoint for the player.
 */
void PlayerInit(Player *player, const Faction *faction, const SOCKADDR_IN *address);

/**
 * Updates the stored endpoint for the player.
 * @param player The player to update.
 * @param address The new endpoint information.
 */
void PlayerUpdateEndpoint(Player *player, const SOCKADDR_IN *address);

/**
 * Assigns a faction to the player and records its identifier for later rebinding.
 * @param player The player to update.
 * @param faction The faction to assign. May be NULL to clear assignment.
 */
void PlayerSetFaction(Player *player, const Faction *faction);

/**
 * Checks whether the player matches the supplied address (by IPv4 address).
 * @param player The player to compare.
 * @param address The address to compare against.
 * @return true if the addresses share the same IPv4, false otherwise.
 */
bool PlayerMatchesAddress(const Player *player, const SOCKADDR_IN *address);

#endif // _PLAYER_H_