/**
 * Header file for the faction object.
 * @file Objects/faction.h
 * @author abmize
 */

#ifndef _FACTION_H_
#define _FACTION_H_

#include <stddef.h>
#include <stdbool.h>

// Forward declaration since we can't include AI/aiPersonality.h here without 
// causing circular dependencies.
typedef struct AIPersonality AIPersonality;

// A faction has an ID and a color.
// It represents a group of planets and starships controlled by a player or AI.
typedef struct Faction {
    int id;
    float color[4];
    AIPersonality *aiPersonality;
    int teamNumber; /* Determines which factions are friendly. */
    int sharedControlNumber; /* Determines which factions share control/archon permissions. */
} Faction;

// Sentinel value indicating a faction is not assigned to any team.
#define FACTION_TEAM_NONE (-1)

// Sentinel value indicating shared control is disabled for a faction.
#define FACTION_SHARED_CONTROL_NONE (-1)

/**
 * Creates a new faction with the specified ID and color.
 * @param id The ID of the faction.
 * @param r The red component of the faction's color (0.0 to 1.0).
 * @param g The green component of the faction's color (0.0 to 1.0).
 * @param b The blue component of the faction's color (0.0 to 1.0).
 * @return The created Faction object.
 */
Faction CreateFaction(int id, float r, float g, float b);

/**
 * Sets the color of the specified faction.
 * @param faction A pointer to the Faction object to modify.
 * @param r The red component of the faction's color (0.0 to 1.0).
 * @param g The green component of the faction's color (0.0 to 1.0).
 * @param b The blue component of the faction's color (0.0 to 1.0).
 */
void FactionSetColor(Faction *faction, float r, float g, float b);

/**
 * Assigns an AI personality to the faction or clears it for human control.
 * @param faction A pointer to the Faction object to modify.
 * @param personality Pointer to the AI personality to assign, or NULL for none.
 */
void FactionSetAIPersonality(Faction *faction, AIPersonality *personality);

/**
 * Sets the team number for the specified faction.
 * A negative value clears the team assignment.
 * @param faction A pointer to the Faction object to modify.
 * @param teamNumber The team number to assign, or a negative value for none.
 */
void FactionSetTeamNumber(Faction *faction, int teamNumber);

/**
 * Sets the shared control/archon number for the specified faction.
 * A negative value clears the shared control assignment.
 * @param faction A pointer to the Faction object to modify.
 * @param sharedControlNumber The shared control/archon number, or a negative value for none.
 */
void FactionSetSharedControlNumber(Faction *faction, int sharedControlNumber);

/**
 * Determines whether two factions are on the same team.
 * This considers identical factions friendly even if they have no team.
 * @param a Pointer to the first faction.
 * @param b Pointer to the second faction.
 * @return true if the factions are friendly, false otherwise.
 */
bool FactionIsFriendly(const Faction *a, const Faction *b);

/**
 * Determines whether two factions share control permissions.
 * Shared control requires matching team numbers and shared control numbers.
 * Identical factions always share control.
 * @param a Pointer to the first faction.
 * @param b Pointer to the second faction.
 * @return true if the factions can control each other, false otherwise.
 */
bool FactionSharesControl(const Faction *a, const Faction *b);

#endif // _FACTION_H_
