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
    level->trailEffects = NULL;
    level->trailEffectCount = 0;
    level->trailEffectCapacity = 0;
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
    free(level->trailEffects);

    // After freeing, we set all members to NULL or zero
    // to avoid dangling pointers and stale data.
    level->factions = NULL;
    level->planets = NULL;
    level->starships = NULL;
    level->trailEffects = NULL;
    level->factionCount = 0;
    level->planetCount = 0;
    level->starshipCount = 0;
    level->starshipCapacity = 0;
    level->trailEffectCount = 0;
    level->trailEffectCapacity = 0;
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
        // Each starship is also associated with some data for its trail effects
        // so we allocate that memory here as well.
        level->trailEffects = (StarshipTrailEffect *)malloc(sizeof(StarshipTrailEffect) * starshipCapacity);
        if (level->trailEffects == NULL) {
            LevelRelease(level);
            return false;
        }
        level->trailEffectCapacity = starshipCapacity;
    } else {
        // If no starship capacity is requested, 
        // we do not need to allocate any memory for trail effects.
        level->trailEffects = NULL;
        level->trailEffectCapacity = 0;
    }

    // With all our allocations done, we can now simply fill in the level members.
    level->factionCount = factionCount;
    level->planetCount = planetCount;
    level->starshipCapacity = starshipCapacity;
    level->starshipCount = 0;
    level->trailEffectCount = 0;
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
 * Helper function to ensure the trail effect array has enough capacity
 * to hold at least minCapacity trail effects.
 * If the current capacity is less than minCapacity,
 * the function will attempt to resize the array to double its current capacity
 * until it meets or exceeds minCapacity.
 * If resizing fails, the function will return false.
 * @param level A pointer to the Level object.
 * @param minCapacity The minimum required capacity for trail effects.
 * @return true if the trail effect array has enough capacity, false otherwise.
 */
static bool EnsureTrailEffectCapacity(Level *level, size_t minCapacity) {
    // We don't need to do anything if we already have enough capacity.
    if (level->trailEffectCapacity >= minCapacity) {
        return true;
    }

    // New capacity equals double the current capacity, or 16 if current capacity is zero.
    // This is intentionally very similar to EnsureStarshipCapacity.
    size_t newCapacity = level->trailEffectCapacity == 0 ? 16 : level->trailEffectCapacity * 2;
    while (newCapacity < minCapacity) {
        newCapacity *= 2;
    }

    // Now that we know how much memory we need, we can actually ask for it here.
    StarshipTrailEffect *resized = (StarshipTrailEffect *)realloc(level->trailEffects, sizeof(StarshipTrailEffect) * newCapacity);
    if (resized == NULL) {
        return false;
    }

    // If realloc succeeded, we must update the level's trail effect pointer
    // and also our internal memory management data on how much memory we have allocated
    // to hold trail effects.
    level->trailEffects = resized;
    level->trailEffectCapacity = newCapacity;
    return true;
}

/**
 * Spawns a trail effect for the given starship in the level.
 * This function creates a StarshipTrailEffect based on the starship's current trail data
 * and adds it to the level's trail effects array.
 * If there is not enough capacity in the trail effects array, it will attempt to resize it.
 * @param level A pointer to the Level object.
 * @param ship A pointer to the Starship object for which to spawn the trail effect.
 */
static void LevelSpawnTrailEffect(Level *level, const Starship *ship) {
    // Basic validation of parameters.
    if (level == NULL || ship == NULL) {
        return;
    }

    // Ensure we have enough capacity for a new trail effect.
    if (!EnsureTrailEffectCapacity(level, level->trailEffectCount + 1)) {
        return;
    }

    // Create the trail effect based on the starship's trail data.
    float color[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    // Get the starship's color for the trail effect.
    StarshipResolveColor(ship, color);

    // Initialize the trail effect.
    StarshipTrailEffect effect = {0};
    StarshipTrailEffectInit(&effect, ship, color);

    // Only add the effect if it has alive samples.
    if (!StarshipTrailEffectIsAlive(&effect)) {
        return;
    }

    // Add the effect to the level's trail effects array.
    level->trailEffects[level->trailEffectCount] = effect;
    level->trailEffectCount += 1;
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
 * Finds a faction by its ID within the level.
 * @param level A pointer to the Level object.
 * @param factionId The ID of the faction to locate.
 * @return A pointer to the matching Faction, or NULL if not found.
 */
static const Faction *FindFactionById(const Level *level, int32_t factionId) {
    // Basic validation of parameters.
    if (level == NULL || level->factions == NULL) {
        return NULL;
    }

    // Iterate over all factions to find the one with the matching ID.
    for (size_t i = 0; i < level->factionCount; ++i) {
        if (level->factions[i].id == factionId) {
            return &level->factions[i];
        }
    }

    // At this point, we've checked all factions and found no match.
    return NULL;
}

/**
 * Finds a planet by its index within the level.
 * @param level A pointer to the Level object.
 * @param index The index of the planet to retrieve.
 * @return A pointer to the Planet, or NULL if the index is invalid.
 */
static Planet *FindPlanetByIndex(Level *level, size_t index) {
    // Basic validation of parameters.
    if (level == NULL || level->planets == NULL) {
        return NULL;
    }

    if (index >= level->planetCount) {
        return NULL;
    }

    // Return a pointer to the planet at the specified index.
    return &level->planets[index];
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

    // Next we update all trail effects in the level.
    // While updating each trail effect, we also check if it is still alive.
    // If a trail effect is no longer alive, we remove it from the level.
    // Similar to starship removal, we replace dead trail effects with the last effect
    // in the array and decrement the trail effect count.
    size_t trailIndex = 0;
    while (trailIndex < level->trailEffectCount) {
        StarshipTrailEffect *effect = &level->trailEffects[trailIndex];
        StarshipTrailEffectUpdate(effect, deltaTime);
        if (!StarshipTrailEffectIsAlive(effect)) {
            size_t last = level->trailEffectCount - 1;
            if (trailIndex != last) {
                level->trailEffects[trailIndex] = level->trailEffects[last];
            }
            level->trailEffectCount -= 1;
            continue;
        }
        trailIndex += 1;
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
            LevelSpawnTrailEffect(level, ship);
            LevelRemoveStarship(level, i);
            continue;
        }
        ++i;
    }
}

/**
 * Applies a full level packet to the provided Level instance.
 * This populates factions, planets, and starships based on the packet data.
 * Existing data in the level is replaced. The level must have been initialized.
 * @param level A pointer to the Level object to populate.
 * @param data A pointer to the packet data buffer.
 * @param size Size of the packet data buffer in bytes.
 * @return true if the packet was applied successfully, false otherwise.
 */
bool LevelApplyFullPacket(Level *level, const void *data, size_t size) {
    // Basic validation of parameters.
    if (level == NULL || data == NULL) {
        return false;
    }

    if (size < sizeof(LevelFullPacket)) {
        return false;
    }

    // Interpret the data as a LevelFullPacket.
    const uint8_t *bytes = (const uint8_t *)data;
    const LevelFullPacket *packet = (const LevelFullPacket *)bytes;
    if (packet->type != LEVEL_PACKET_TYPE_FULL) {
        return false;
    }

    // Extract counts from the packet.
    size_t factionCount = (size_t)packet->factionCount;
    size_t planetCount = (size_t)packet->planetCount;
    size_t starshipCount = (size_t)packet->starshipCount;

    // Calculate the required size for the full packet data.
    size_t requiredSize = sizeof(LevelFullPacket)
        + factionCount * sizeof(LevelPacketFactionInfo)
        + planetCount * sizeof(LevelPacketPlanetFullInfo)
        + starshipCount * sizeof(LevelPacketStarshipInfo);
    
    // If the provided size is less than the required size
    // for the full packet, return false since the data is incomplete.
    if (size < requiredSize) {
        return false;
    }

    // We initialize the capacity for the starships array to either
    // the number of starships in the packet, or 16 if there are none.
    size_t desiredStarshipCapacity = starshipCount > 0 ? starshipCount : 16u;

    // Configure the level with the extracted counts.
    if (!LevelConfigure(level, factionCount, planetCount, desiredStarshipCapacity)) {
        return false;
    }

    // Assign level dimensions from the packet.
    level->width = packet->width;
    level->height = packet->height;

    // Using a cursor to traverse the packet data, we populate factions, planets, and starships.
    const uint8_t *cursor = bytes + sizeof(LevelFullPacket);

    // Populate factions.
    const LevelPacketFactionInfo *factionInfo = (const LevelPacketFactionInfo *)cursor;
    for (size_t i = 0; i < factionCount; ++i) {
        level->factions[i].id = (int)factionInfo[i].id;
        for (size_t c = 0; c < 4; ++c) {
            level->factions[i].color[c] = factionInfo[i].color[c];
        }

        // Network packets do not carry AI assignments, so we clear them here.
        level->factions[i].aiPersonality = NULL;
    }

    // Move the cursor past the faction data, to what better be planet data.
    cursor += factionCount * sizeof(LevelPacketFactionInfo);
    const LevelPacketPlanetFullInfo *planetInfo = (const LevelPacketPlanetFullInfo *)cursor;
    for (size_t i = 0; i < planetCount; ++i) {
        Planet *planet = &level->planets[i];
        planet->position = planetInfo[i].position;
        planet->maxFleetCapacity = planetInfo[i].maxFleetCapacity;
        planet->currentFleetSize = planetInfo[i].currentFleetSize;
        planet->owner = FindFactionById(level, planetInfo[i].ownerId);
        planet->claimant = FindFactionById(level, planetInfo[i].claimantId);
    }

    // Move the cursor past the planet data, to what better be starship data.
    cursor += planetCount * sizeof(LevelPacketPlanetFullInfo);
    const LevelPacketStarshipInfo *starshipInfo = (const LevelPacketStarshipInfo *)cursor;

    // Double check we have enough capacity for starships and trail effects 
    // now that we have a second look at the starship count.
    if (!EnsureStarshipCapacity(level, starshipCount)) {
        return false;
    }

    // Ensure we have enough capacity for trail effects for the starships.
    if (!EnsureTrailEffectCapacity(level, starshipCount)) {
        return false;
    }


    // Populate starships.
    level->starshipCount = starshipCount;
    level->trailEffectCount = 0;

    // Iterate over each starship info and create the corresponding starship
    // using the provided data.
    for (size_t i = 0; i < starshipCount; ++i) {
        const LevelPacketStarshipInfo *info = &starshipInfo[i];
        const Faction *owner = FindFactionById(level, info->ownerId);
        Planet *target = NULL;
        if (info->targetPlanetIndex >= 0) {
            target = FindPlanetByIndex(level, (size_t)info->targetPlanetIndex);
        }

        Starship ship = CreateStarship(info->position, info->velocity, owner, target);
        ship.position = info->position;
        ship.velocity = info->velocity;
        ship.owner = owner;
        ship.target = target;
        level->starships[i] = ship;
    }

    return true;
}

/**
 * Applies a snapshot packet to the provided Level instance.
 * Static level data (positions, faction colors, etc.) must already be present.
 * Dynamic fields such as planet ownership, fleet sizes, and starships are
 * overwritten with the data from the snapshot.
 * @param level A pointer to the Level object to update.
 * @param data A pointer to the packet data buffer.
 * @param size Size of the packet data buffer in bytes.
 * @return true if the packet was applied successfully, false otherwise.
 */
bool LevelApplySnapshot(Level *level, const void *data, size_t size) {
    // Basic validation of parameters.
    if (level == NULL || data == NULL) {
        return false;
    }

    if (size < sizeof(LevelSnapshotPacket)) {
        return false;
    }

    // Interpret the data as a LevelSnapshotPacket.
    const uint8_t *bytes = (const uint8_t *)data;
    const LevelSnapshotPacket *packet = (const LevelSnapshotPacket *)bytes;
    if (packet->type != LEVEL_PACKET_TYPE_SNAPSHOT) {
        return false;
    }

    // A snapshot packet only contains planet snapshot info.

    // Extract planet count from the packet.
    size_t planetCount = (size_t)packet->planetCount;

    // Calculate the required size for the snapshot packet data.
    size_t requiredSize = sizeof(LevelSnapshotPacket)
        + planetCount * sizeof(LevelPacketPlanetSnapshotInfo);

    // If the provided size is less than the required size
    // for the snapshot packet, return false since the data is incomplete.
    if (size < requiredSize) {
        return false;
    }

    // Make sure this local level instance is compatible with the snapshot data.
    if (level->planets == NULL || level->factions == NULL) {
        return false;
    }

    if (planetCount != level->planetCount) {
        return false;
    }

    // Advance the cursor past the snapshot header to the planet snapshot info.
    const uint8_t *cursor = bytes + sizeof(LevelSnapshotPacket);
    const LevelPacketPlanetSnapshotInfo *planetInfo = (const LevelPacketPlanetSnapshotInfo *)cursor;

    // For each planet, update its dynamic fields from the snapshot data.
    for (size_t i = 0; i < planetCount; ++i) {
        Planet *planet = &level->planets[i];
        planet->currentFleetSize = planetInfo[i].currentFleetSize;
        planet->owner = FindFactionById(level, planetInfo[i].ownerId);
        planet->claimant = FindFactionById(level, planetInfo[i].claimantId);
    }

    return true;
}

/**
 * Converts a faction pointer into its ID for network transmission.
 * If the faction pointer is NULL, returns -1.
 * @param faction A pointer to the Faction object.
 * @return The ID of the faction, or -1 if the faction is NULL.
 */
static int32_t ResolveFactionId(const Faction *faction) {
    return (faction != NULL) ? (int32_t)faction->id : -1;
}

/**
 * Converts a planet pointer into its index in the 
 * level's planet array for network transmission.
 * We return -1 if the planet pointer is NULL,
 * or if the planet does not belong to the given level.
 * This function assumes that the level and its planet array are valid.
 * @param level A pointer to the Level object.
 * @param planet A pointer to the Planet object.
 * @return The index of the planet in the level's planet array, or -1 if not found.
 */
static int32_t ResolvePlanetIndex(const Level *level, const Planet *planet) {
    // Basic validation of input pointers.
    if (level == NULL || planet == NULL || level->planets == NULL) {
        return -1;
    }

    // We assume that planets are stored in a contiguous array,
    // so we can calculate the index by pointer arithmetic.
    ptrdiff_t index = planet - level->planets;
    if (index < 0 || (size_t)index >= level->planetCount) {
        return -1;
    }

    return (int32_t)index;
}

/**
 * Creates a full level packet buffer for network transmission.
 * This function allocates memory for the packet buffer and fills it with
 * the full state of the level, including factions, planets, and starships.
 * The caller is responsible for releasing the packet buffer using
 * LevelPacketBufferRelease when done.
 * @param level A pointer to the Level object.
 * @param outBuffer A pointer to a LevelPacketBuffer to receive the packet data.
 * @return true if the packet buffer was created successfully, false otherwise.
 */
bool LevelCreateFullPacketBuffer(const Level *level, LevelPacketBuffer *outBuffer) {
    // Basic validation of input pointers.
    if (outBuffer == NULL) {
        return false;
    }

    // The outBuffer better start empty and free'd 
    // as otherwise this line will leak memory.
    outBuffer->data = NULL;
    outBuffer->size = 0u;

    // No point in continuing if level is NULL.
    if (level == NULL) {
        return false;
    }

    // We need to know how many factions, planets, and starships we have
    size_t factionCount = level->factionCount;
    size_t planetCount = level->planetCount;
    size_t starshipCount = level->starshipCount;

    // If any of these counts exceed UINT32_MAX, we cannot represent them in the packet
    // so we fail early.
    if (factionCount > UINT32_MAX || planetCount > UINT32_MAX || starshipCount > UINT32_MAX) {
        return false;
    }

    // Now we can calculate the total size of the packet buffer we need to allocate.
    // It's a simple sum of the sizes of the header and all the arrays,
    // with each array itself simply being the count of elements times the size of each element.
    size_t totalSize = sizeof(LevelFullPacket)
        + factionCount * sizeof(LevelPacketFactionInfo)
        + planetCount * sizeof(LevelPacketPlanetFullInfo)
        + starshipCount * sizeof(LevelPacketStarshipInfo);

    // This will be the buffer that holds all our packet data.
    uint8_t *buffer = (uint8_t *)malloc(totalSize);
    if (buffer == NULL) {
        return false;
    }

    // Now that we have the buffer allocated, we can start filling it in.
    // We start with the header.
    LevelFullPacket *header = (LevelFullPacket *)buffer;
    header->type = LEVEL_PACKET_TYPE_FULL;
    header->width = level->width;
    header->height = level->height;
    header->factionCount = (uint32_t)factionCount;
    header->planetCount = (uint32_t)planetCount;
    header->starshipCount = (uint32_t)starshipCount;

    // Then we fill in the faction info array.
    // We use a cursor to keep track of where we are in the buffer.
    uint8_t *cursor = buffer + sizeof(LevelFullPacket);
    LevelPacketFactionInfo *factionInfo = (LevelPacketFactionInfo *)cursor;
    for (size_t i = 0; i < factionCount; ++i) {
        factionInfo[i].id = (int32_t)level->factions[i].id;
        for (size_t c = 0; c < 4; ++c) {
            factionInfo[i].color[c] = level->factions[i].color[c];
        }
    }

    // Now we advance the cursor past the faction info array
    // and fill in the planet info array.
    cursor += factionCount * sizeof(LevelPacketFactionInfo);
    LevelPacketPlanetFullInfo *planetInfo = (LevelPacketPlanetFullInfo *)cursor;
    for (size_t i = 0; i < planetCount; ++i) {
        const Planet *planet = &level->planets[i];
        planetInfo[i].position = planet->position;
        planetInfo[i].maxFleetCapacity = planet->maxFleetCapacity;
        planetInfo[i].currentFleetSize = planet->currentFleetSize;
        planetInfo[i].ownerId = ResolveFactionId(planet->owner);
        planetInfo[i].claimantId = ResolveFactionId(planet->claimant);
    }

    // Finally, we advance the cursor past the planet info array
    // and fill in the starship info array.
    cursor += planetCount * sizeof(LevelPacketPlanetFullInfo);
    LevelPacketStarshipInfo *starshipInfo = (LevelPacketStarshipInfo *)cursor;
    for (size_t i = 0; i < starshipCount; ++i) {
        const Starship *ship = &level->starships[i];
        starshipInfo[i].position = ship->position;
        starshipInfo[i].velocity = ship->velocity;
        starshipInfo[i].ownerId = ResolveFactionId(ship->owner);
        starshipInfo[i].targetPlanetIndex = ResolvePlanetIndex(level, ship->target);
    }

    // We've filled in the entire buffer now,
    // so we can set the outBuffer fields and return success.
    outBuffer->data = buffer;
    outBuffer->size = totalSize;
    return true;
}

/**
 * Creates a level snapshot packet buffer for network transmission.
 * This function allocates memory for the packet buffer and fills it with
 * the dynamic state of the level relevant to planets (ownership, fleets).
 * Starships are intentionally excluded so clients can simulate them locally.
 * The caller is responsible for releasing the packet buffer using
 * LevelPacketBufferRelease when done.
 * @param level A pointer to the Level object.
 * @param outBuffer A pointer to a LevelPacketBuffer to receive the packet data.
 * @return true if the packet buffer was created successfully, false otherwise.
 */
bool LevelCreateSnapshotPacketBuffer(const Level *level, LevelPacketBuffer *outBuffer) {
    // Basic validation of input pointers.
    if (outBuffer == NULL) {
        return false;
    }

    // The outBuffer better start empty and free'd 
    // as otherwise this line will leak memory.
    outBuffer->data = NULL;
    outBuffer->size = 0u;

    // No point in continuing if level is NULL.
    if (level == NULL) {
        return false;
    }

    // We only need to know how many planets we have
    size_t planetCount = level->planetCount;

    // If the count exceeds UINT32_MAX, we cannot represent it in the packet
    if (planetCount > UINT32_MAX) {
        return false;
    }

    // Now we can calculate the total size of the packet buffer we need to allocate.
    // It's a simple sum of the sizes of the header and all the arrays,
    // with each array itself simply being the count of elements times the size of each element.
    size_t totalSize = sizeof(LevelSnapshotPacket)
        + planetCount * sizeof(LevelPacketPlanetSnapshotInfo);

    // This will be the buffer that holds all our packet data.
    uint8_t *buffer = (uint8_t *)malloc(totalSize);
    if (buffer == NULL) {
        return false;
    }

    // Now that we have the buffer allocated, we can start filling it in.
    // We start with the header.
    LevelSnapshotPacket *header = (LevelSnapshotPacket *)buffer;
    header->type = LEVEL_PACKET_TYPE_SNAPSHOT;
    header->planetCount = (uint32_t)planetCount;

    // Then we fill in the planet snapshot info array.
    uint8_t *cursor = buffer + sizeof(LevelSnapshotPacket);
    LevelPacketPlanetSnapshotInfo *planetInfo = (LevelPacketPlanetSnapshotInfo *)cursor;
    for (size_t i = 0; i < planetCount; ++i) {
        const Planet *planet = &level->planets[i];
        planetInfo[i].currentFleetSize = planet->currentFleetSize;
        planetInfo[i].ownerId = ResolveFactionId(planet->owner);
        planetInfo[i].claimantId = ResolveFactionId(planet->claimant);
    }

    // We've filled in the entire buffer now,
    // so we can set the outBuffer fields and return success.
    outBuffer->data = buffer;
    outBuffer->size = totalSize;
    return true;
}

/**
 * Releases a level packet buffer created by LevelCreateFullPacketBuffer
 * or LevelCreateSnapshotPacketBuffer.
 * This function frees the memory allocated for the packet data
 * and resets the buffer fields to NULL and zero.
 * @param buffer A pointer to the LevelPacketBuffer to release.
 */
void LevelPacketBufferRelease(LevelPacketBuffer *buffer) {
    // Basic validation of input pointer.
    if (buffer == NULL) {
        return;
    }

    // Free the packet data and reset the buffer fields.
    // Buffer better not have been already released,
    // as otherwise this line will double-free memory.
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0u;
}
