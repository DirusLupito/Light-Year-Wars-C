/**
 * Header file for the level object.
 * @file Objects/level.h
 * @author abmize
 */

#ifndef _LEVEL_H_
#define _LEVEL_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "Objects/faction.h"
#include "Objects/planet.h"
#include "Objects/starship.h"

// A level contains factions, planets, and starships.
// It also has dimensions (width and height).
// The level is the main container for the game state.
// It is used by the game engine to manage the game world.
// It manages the lifecycle and interactions of these objects.
// It provides functions to initialize, release, configure, spawn starships, remove starships, and update the level state.
// That said, the level does not handle rendering or user input directly.
typedef struct Level {
	Faction *factions;
	size_t factionCount;

	Planet *planets;
	size_t planetCount;

	Starship *starships;
	size_t starshipCount;
	size_t starshipCapacity;

	float width;
	float height;
} Level;

/**
 * Initializes a Level object to default values.
 * Level init expects a valid pointer to a Level object.
 * However, the pointer's internal members may be NULL or uninitialized.
 * @param level A pointer to the Level object to initialize.
 */
void LevelInit(Level *level);

/**
 * Releases resources held by a Level object.
 * Level release expects a valid pointer to a Level object.
 * However, the pointer's internal members may be NULL or uninitialized.
 * @param level A pointer to the Level object to release.
 */
void LevelRelease(Level *level);

/**
 * Configures a Level object with the specified number of factions, planets, and starship capacity.
 * This function allocates memory for the internal arrays of the Level object.
 * If the Level object was previously configured, it will be released first
 * as a simple and effective if not wholly efficient way to avoid memory leaks.
 * @param level A pointer to the Level object to configure.
 * @param factionCount The number of factions to allocate memory for.
 * @param planetCount The number of planets to allocate memory for.
 * @param starshipCapacity The initial capacity for starships to allocate memory for.
 * @return true if the configuration was successful, false otherwise.
 */
bool LevelConfigure(Level *level, size_t factionCount, size_t planetCount, size_t starshipCapacity);

/**
 * Spawns a new starship in the level.
 * This function will first check if there is enough memory allocated for starships
 * to fit the new starship. If not, it will attempt to resize the starship array.
 * If resizing fails, the function will return NULL.
 * Otherwise, it will create the starship and add it to the level's starship array.
 * It will then return a pointer to the newly spawned starship.
 * @param level A pointer to the Level object.
 * @param position The initial position of the starship.
 * @param velocity The initial velocity of the starship.
 * @param owner A pointer to the Faction that owns the starship.
 * @param target A pointer to the Planet that is the target of the starship.
 * @return A pointer to the spawned Starship, or NULL if spawning failed.
 */
Starship *LevelSpawnStarship(Level *level, Vec2 position, Vec2 velocity, const Faction *owner, Planet *target);

/**
 * Removes a starship from the level at the specified index.
 * This function will remove the starship by replacing it with the last starship
 * in the array and decrementing the starship count.
 * This approach avoids shifting elements in the array, which is more efficient.
 * If the array looks like:
 * ```
 * [*], [*], [*], [*], [ ], [ ]
 * ```
 * where [*] are starships and [ ] are empty slots,
 * and we remove the starship at index 1, the array will become:
 * ```
 * [*], [ ], [*], [*], [ ], [ ]
 * ```
 * then the last starship will be moved to index 1:
 * ```
 * [*], [*], [*], [ ], [ ], [ ]
 * ```
 * and the starship count will be decremented.
 * @param level A pointer to the Level object.
 * @param index The index of the starship to remove.
 */
void LevelRemoveStarship(Level *level, size_t index);

/**
 * Updates the state of the level and its contained objects.
 * This function updates all planets and starships in the level.
 * It processes starship movements and checks for collisions with their target planets.
 * If a starship collides with its target planet, it is removed from the level
 * and the planet handles the incoming ship.
 * For planet and starship updates, the respective update functions are called.
 * See PlanetUpdate and StarshipUpdate for more details on their behavior during updates.
 * @param level A pointer to the Level object to update.
 * @param deltaTime The time elapsed since the last update, in seconds.
 */
void LevelUpdate(Level *level, float deltaTime);

#endif // _LEVEL_H_
