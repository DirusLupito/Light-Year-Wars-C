/**
 * Header file for the player object.
 * @file Objects/player.h
 * @author abmize
 */
#ifndef _PLAYER_H_
#define _PLAYER_H_

#include <winsock2.h>
#include <windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include "Objects/faction.h"

// Maximum length for a player's display name (excluding null terminator).
#define PLAYER_NAME_MAX_LENGTH 30

// Minimum length for a player's display name (excluding null terminator).
#define PLAYER_NAME_MIN_LENGTH 1

// A player represents a user in the game.
// Each player is uniquely associated with a network/IPv4 address,
// a faction they uniquely control, whether they are awaiting full level data,
// and an inactivity timer used for timeouts on the server side.
typedef struct Player {
    const Faction *faction;
    int factionId;
    SOCKADDR_IN address;
    char name[PLAYER_NAME_MAX_LENGTH + 1];
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
 * Validates a proposed player name for ASCII and length bounds.
 * @param name Candidate null-terminated name string.
 * @return true if the name meets length and character requirements, false otherwise.
 */
bool PlayerValidateName(const char *name);

/**
 * Assigns a validated name to the player or clears it when invalid.
 * @param player The player to update.
 * @param name Candidate null-terminated name string.
 */
void PlayerSetName(Player *player, const char *name);

/**
 * Checks whether the player matches the supplied address (by IPv4 address).
 * @param player The player to compare.
 * @param address The address to compare against.
 * @return true if the addresses share the same IPv4, false otherwise.
 */
bool PlayerMatchesAddress(const Player *player, const SOCKADDR_IN *address);

#endif // _PLAYER_H_