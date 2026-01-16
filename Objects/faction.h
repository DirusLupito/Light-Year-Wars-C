/**
 * Header file for the faction object.
 * @file Objects/faction.h
 * @author abmize
 */

#ifndef _FACTION_H_
#define _FACTION_H_

#include <stddef.h>

// A faction has an ID and a color.
// It represents a group of planets and starships controlled by a player or AI.
typedef struct Faction {
    int id;
    float color[4];
} Faction;

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

#endif // _FACTION_H_
