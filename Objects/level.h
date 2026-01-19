/**
 * Header file for the level object.
 * @file Objects/level.h
 * @author abmize
 */

#ifndef _LEVEL_H_
#define _LEVEL_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "Objects/faction.h"
#include "Objects/planet.h"
#include "Objects/starship.h"

// Packet type identifiers
// If the first 4 bytes of a packet equal one of these values,
// it indicates the type of packet being sent or received.

// Type value for a full level packet
#define LEVEL_PACKET_TYPE_FULL 1u

// Type value for a planet snapshot packet (planets only)
#define LEVEL_PACKET_TYPE_SNAPSHOT 2u

// Type value for a player assignment packet (server -> client)
#define LEVEL_PACKET_TYPE_ASSIGNMENT 3u

// Type value for a move order packet (client -> server)
#define LEVEL_PACKET_TYPE_MOVE_ORDER 4u

// Type value for a fleet launch packet (server -> clients)
#define LEVEL_PACKET_TYPE_FLEET_LAUNCH 5u

// Definitions of packet structures used for network transmission.
// We use #pragma pack(push, 1) to ensure there is no padding added by the compiler.
// This translates to "pack the following structures with 1-byte alignment".
// This is important for network packets to ensure consistent layout across 
// different compilers and architectures.
// And it also ensures the packet sizes are as expected.
#pragma pack(push, 1)

// A LevelPacketFactionInfo represents the network representation of a faction.
// It is used to communicate faction information over the network.
// A faction has an ID and a color (RGBA), so we include these fields here.
typedef struct LevelPacketFactionInfo {
    int32_t id;
    float color[4];
} LevelPacketFactionInfo;

// A LevelPacketPlanetFullInfo represents the full network representation of a planet.
// It is used in full level packets to communicate all planet information.
// A planet has a position, max fleet capacity, current fleet size,
// an owner faction ID, and a claimant faction ID.
typedef struct LevelPacketPlanetFullInfo {
    Vec2 position;
    float maxFleetCapacity;
    float currentFleetSize;
    int32_t ownerId;
    int32_t claimantId;
} LevelPacketPlanetFullInfo;

// A LevelPacketPlanetSnapshotInfo represents a snapshot network representation of a planet.
// It is used in snapshot level packets to communicate partial planet information.
// The only dynamic fields of a planet are its current fleet size,
// owner faction ID, and claimant faction ID.
typedef struct LevelPacketPlanetSnapshotInfo {
    float currentFleetSize;
    int32_t ownerId;
    int32_t claimantId;
} LevelPacketPlanetSnapshotInfo;

// A LevelPacketStarshipInfo represents the network representation of a starship.
// It is used to communicate starship information over the network.
// A starship has a position, velocity, owner faction ID, and target planet index.
typedef struct LevelPacketStarshipInfo {
    Vec2 position;
    Vec2 velocity;
    int32_t ownerId;
    int32_t targetPlanetIndex;
} LevelPacketStarshipInfo;

// A LevelFullPacket represents the header of a full level packet.
// It contains the type, dimensions, and counts of factions, planets, and starships.
// This should be followed by arrays of faction info, planet info, and starship info
// in that order to communicate the full state of the level.
typedef struct LevelFullPacket {
    uint32_t type;
    float width;
    float height;
    uint32_t factionCount;
    uint32_t planetCount;
    uint32_t starshipCount;
} LevelFullPacket;

// A LevelSnapshotPacket represents the header of a planet snapshot packet.
// It contains the type and count of planets whose ownership/fleet data follow.
// It is followed by an array of LevelPacketPlanetSnapshotInfo structures.
typedef struct LevelSnapshotPacket {
    uint32_t type;
    uint32_t planetCount;
} LevelSnapshotPacket;

// A LevelFleetLaunchPacket communicates a fleet launch initiated on the server.
// It includes the origin and destination planet indices along with the number of
// ships that should be spawned by clients to mirror the authoritative action.
// It also includes a random number generator state to ensure clients
// can replicate the same randomization used by the server for ship spawn positions.
typedef struct LevelFleetLaunchPacket {
    uint32_t type;
    int32_t originPlanetIndex;
    int32_t destinationPlanetIndex;
    int32_t shipCount;
    int32_t ownerFactionId;
    unsigned int shipSpawnRNGState;
} LevelFleetLaunchPacket;

// A LevelAssignmentPacket communicates the faction assigned to a player.
// The server sends this after a successful join so the client knows which
// faction they control.
typedef struct LevelAssignmentPacket {
    uint32_t type;
    int32_t factionId;
} LevelAssignmentPacket;

// A LevelMoveOrderPacket communicates a set of origin planets and a destination
// planet for fleet movement requests. It is sent by clients to the server.
// The packet is followed by originCount 32-bit planet indices.
typedef struct LevelMoveOrderPacket {
    uint32_t type;
    uint32_t originCount;
    int32_t destinationPlanetIndex;
    int32_t originPlanetIndices[];
} LevelMoveOrderPacket;
#pragma pack(pop)

// A LevelPacketBuffer holds a pointer to packet data and its size.
// It is used to manage memory for network packets.
// The data pointer should be allocated and freed appropriately.
// This is the abstract representation of a network packet in memory.
// The other packets defined above are the concrete representations of specific packet types,
// while this struct is a generic container for any packet data.
typedef struct LevelPacketBuffer {
    void *data;
    size_t size;
} LevelPacketBuffer;

// A level contains factions, planets, starships, and trail effects for those starships.
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
    StarshipTrailEffect *trailEffects;
    size_t trailEffectCount;
    size_t trailEffectCapacity;

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
bool LevelCreateFullPacketBuffer(const Level *level, LevelPacketBuffer *outBuffer);

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
bool LevelCreateSnapshotPacketBuffer(const Level *level, LevelPacketBuffer *outBuffer);

/**
 * Releases a level packet buffer created by LevelCreateFullPacketBuffer
 * or LevelCreateSnapshotPacketBuffer.
 * This function frees the memory allocated for the packet data
 * and resets the buffer fields to NULL and zero.
 * @param buffer A pointer to the LevelPacketBuffer to release.
 */
void LevelPacketBufferRelease(LevelPacketBuffer *buffer);

/**
 * Applies a full level packet to the provided Level instance.
 * This populates factions, planets, and starships based on the packet data.
 * Existing data in the level is replaced. The level must have been initialized.
 * @param level A pointer to the Level object to populate.
 * @param data A pointer to the packet data buffer.
 * @param size Size of the packet data buffer in bytes.
 * @return true if the packet was applied successfully, false otherwise.
 */
bool LevelApplyFullPacket(Level *level, const void *data, size_t size);

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
bool LevelApplySnapshot(Level *level, const void *data, size_t size);

#endif // _LEVEL_H_
