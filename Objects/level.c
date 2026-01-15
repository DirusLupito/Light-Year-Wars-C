/**
 * Implements a level object.
 * It also has dimensions (width and height).
 * The level is the main container for the game state.
 * It is used by the game engine to manage the game world.
 * It manages the lifecycle and interactions of these objects.
 * It provides functions to initialize, release, configure, spawn starships,
 *  remove starships, and update the level state.
 * That said, the level does not handle rendering or user input directly.
 * @file Objects/level.c
 * @author abmize
 */

#include "Objects/level.h"

/**
 * Initializes a Level object to default values.
 * Level init expects a valid pointer to a Level object.
 * However, the pointer's internal members may be NULL or uninitialized.
 * @param level A pointer to the Level object to initialize.
 */
void LevelInit(Level *level) {
    // Check for NULL pointer
    // since we need a valid Level to initialize.
	if (level == NULL) {
		return;
	}

    // Basically we set all members to zero or NULL.
	level->factions = NULL;
	level->factionCount = 0;
	level->planets = NULL;
	level->planetCount = 0;
	level->starships = NULL;
	level->starshipCount = 0;
	level->starshipCapacity = 0;
	level->width = 0.0f;
	level->height = 0.0f;
}

/**
 * Releases resources held by a Level object.
 * Level release expects a valid pointer to a Level object.
 * However, the pointer's internal members may be NULL or uninitialized.
 * @param level A pointer to the Level object to release.
 */
void LevelRelease(Level *level) {
	if (level == NULL) {
		return;
	}

    // Since we assume level is the owner of its internal arrays,
    // and that the passed level pointer is valid, we can free them here.
    // Something else that I did not know, and that you may not know either,
    // is that according to the C standard, it is safe to call free() with a NULL pointer.
    // This means we do not need to check if each pointer is NULL before calling free().
    // It also means that it is safe to call free() on a Level that has not been fully configured.
	free(level->factions);
	free(level->planets);
	free(level->starships);

    // After freeing, we set all members to NULL or zero
    // to avoid dangling pointers and stale data.
	level->factions = NULL;
	level->planets = NULL;
	level->starships = NULL;
	level->factionCount = 0;
	level->planetCount = 0;
	level->starshipCount = 0;
	level->starshipCapacity = 0;
	level->width = 0.0f;
	level->height = 0.0f;
}

/**
 * Helper function to allocate an array of elements.
 * @param pointer A pointer to the pointer that will hold the allocated array.
 * @param elementSize The size of each element in bytes.
 * @param count The number of elements to allocate.
 * @return true if allocation was successful, false otherwise.
 */
static bool AllocateArray(void **pointer, size_t elementSize, size_t count) {

    // If we're asked to allocate zero elements,
    // we set the pointer to NULL and return true.
	if (count == 0) {
		*pointer = NULL;
		return true;
	}

    // Otherwise, we allocate the requested memory
    // and zero-initialize it using calloc.
	void *memory = calloc(count, elementSize);

	if (memory == NULL) {
        // If allocation failed, we return false.
		return false;
	}

    // If allocation succeeded, we set the given pointer passed by reference
    // to point to the newly allocated memory and return true.
	*pointer = memory;
	return true;
}

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
bool LevelConfigure(Level *level, size_t factionCount, size_t planetCount, size_t starshipCapacity) {
	if (level == NULL) {
		return false;
	}

    // We know at this point that level is valid,
    // so now we just need to make sure we do not leak memory
    // if the level was previously configured.
    // We also clean up any stale data here.
	LevelRelease(level);

    // Now with a clean level object we can allocate the internal arrays.

    // We first allocate factions.
	if (!AllocateArray((void **)&level->factions, sizeof(Faction), factionCount)) {
		LevelRelease(level);
		return false;
	}

    // Then we allocate planets.
	if (!AllocateArray((void **)&level->planets, sizeof(Planet), planetCount)) {
		LevelRelease(level);
		return false;
	}

    // And finally if we need starships, we allocate them here.
	if (starshipCapacity > 0) {
		level->starships = (Starship *)malloc(sizeof(Starship) * starshipCapacity);
		if (level->starships == NULL) {
			LevelRelease(level);
			return false;
		}
	}

    // With all our allocations done, we can now simply fill in the level members.
	level->factionCount = factionCount;
	level->planetCount = planetCount;
	level->starshipCapacity = starshipCapacity;
	level->starshipCount = 0;
	return true;
}

/**
 * Helper function to ensure the starship array has enough capacity
 * to hold at least minCapacity starships.
 * If the current capacity is less than minCapacity,
 * the function will attempt to resize the array to double its current capacity
 * until it meets or exceeds minCapacity.
 * If resizing fails, the function will return false.
 * @param level A pointer to the Level object.
 * @param minCapacity The minimum required capacity for starships.
 * @return true if the starship array has enough capacity, false otherwise.
 */
static bool EnsureStarshipCapacity(Level *level, size_t minCapacity) {

    // We don't need to do anything if we already have enough capacity.
	if (level->starshipCapacity >= minCapacity) {
		return true;
	}

    // New capacity equals double the current capacity, or 16 if current capacity is zero.
	size_t newCapacity = level->starshipCapacity == 0 ? 16 : level->starshipCapacity * 2;
	while (newCapacity < minCapacity) {
		newCapacity *= 2;
	}

    // Now that we know how much memory we need, we can actually ask for it here.
	Starship *resized = (Starship *)realloc(level->starships, sizeof(Starship) * newCapacity);
	if (resized == NULL) {
		return false;
	}

    // If realloc succeeded, we must update the level's starship pointer
    // and also our internal memory management data on how much memory we have allocated
    // to hold starships.
	level->starships = resized;
	level->starshipCapacity = newCapacity;
	return true;
}

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
Starship *LevelSpawnStarship(Level *level, Vec2 position, Vec2 velocity, const Faction *owner, Planet *target) {
	if (level == NULL || target == NULL) {
		return NULL;
	}

    // Before we can spawn any starship, we must ensure we have enough memory allocated.
	if (!EnsureStarshipCapacity(level, level->starshipCount + 1)) {
		return NULL;
	}

    // Now that we know we have enough memory, we can create the starship
    // and add it to the level's starship array.
    // See starship.c and starship.h for more details on starship creation.
	Starship ship = CreateStarship(position, velocity, owner, target);

    // We must add the ship to the Level's starship array,
    // to keep track of it, give it to a different variable
    // so we can return a pointer to it,
    // and finally increment the starship count so our internal
    // memory management knows how much memory is actually in use
    // by starships and where the next free block of memory is.
	level->starships[level->starshipCount] = ship;
	Starship *stored = &level->starships[level->starshipCount];
	level->starshipCount += 1;
	return stored;
}

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
void LevelRemoveStarship(Level *level, size_t index) {
	if (level == NULL || index >= level->starshipCount) {
		return;
	}

    // To remove the starship at the given index,
    // we simply replace it with the last starship in the array
    // and decrement the starship count.
    // This implementation also assumes that starship objects are simple 
    // enough that they are able to be overwritten without any special handling
    // or freeing of internal resources.
    // Should any malloc'd or resource-owning members be added to the Starship struct in the future,
    // this function will need to be updated to properly release those resources
	size_t last = level->starshipCount - 1;
	if (index != last) {
		level->starships[index] = level->starships[last];
	}
	level->starshipCount -= 1;
}

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
void LevelUpdate(Level *level, float deltaTime) {
	if (level == NULL) {
		return;
	}

    // First we update all planets in the level.
	for (size_t i = 0; i < level->planetCount; ++i) {
		PlanetUpdate(&level->planets[i], deltaTime);
	}

    // And then we update all starships in the level.
    // While updating each starship, we also check for collisions with their target planets.
    // StarshipUpdate is primarily responsible for moving the starship towards its target planet,
    // while interactions between starships and planets are handled here in LevelUpdate.
    // If a starship collides with its target planet, we call PlanetHandleIncomingShip
    // to let the planet process the incoming ship,
    // and then we remove the starship from the level.
    // See starship and planet implementations for more details on the behavior
    // of their functions during updates and collisions.
	size_t i = 0;
	while (i < level->starshipCount) {
		Starship *ship = &level->starships[i];
		StarshipUpdate(ship, deltaTime);
		if (StarshipCheckCollision(ship)) {
			PlanetHandleIncomingShip(ship->target, ship);
			LevelRemoveStarship(level, i);
			continue;
		}
		++i;
	}
}
