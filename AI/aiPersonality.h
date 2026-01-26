/**
 * Represents an abstract AI personality.
 * 
 * All AI personalities must implement a function which
 * takes in a pointer to a Level struct and a faction to play for and returns a list
 * of pairs, or tuples, of planets. The first planet in each pair
 * is the source planet, and the second planet is the target planet.
 * Each source planet will launch a fleet to the corresponding target planet.
 * 
 * Furthermore, if any new AI personalities are added,
 * the following must be done:
 * 1. A new .c and .h file must be created in the AI/ directory
 *   implementing the new AI personality.
 * 2. The new AI personality must be registered in here under the
 *   AI_PERSONALITY_COUNT and AI_PERSONALITIES definitions.
 * 3. The new AI personality's header file must be included here.
 * 4. The makefile must be updated to compile the new AI personality source file.
 * 
 * After these steps, the new AI personality will be available for use.
 * 
 * This file also defines various constants used by all AI related code.
 * @file aiPersonality.h
 * @author abmize
 */

#ifndef AI_AIPERSONALITY_H
#define AI_AIPERSONALITY_H

#include "Objects/planet.h"
#include "AI/idleAI.h"
#include "AI/basicAI.h"

// Forward declaration keeps the header lightweight since only pointers are used here.
struct Level;
struct Faction;

// Defines the frequency, in hertz, at which the AI's action function should be called.
#define AI_ACTION_RATE 2 

// Defines the number of different AI personalities currently implemented.
// Update this when registering a new AI personality.
#define AI_PERSONALITY_COUNT 2

// Defines all currently implemented AI personalities.
// New AI personalities must be registered here.
#define AI_PERSONALITIES \
    &IDLE_AI_PERSONALITY, \
    &BASIC_AI_PERSONALITY


/**
 * Represents a pair of planets: an origin and a destination.
 * This struct is used to specify fleet launch orders for the AI.
 */
typedef struct PlanetPair {
    struct Planet *origin;
    struct Planet *destination;
} PlanetPair;


/**
 * Represents an abstract AI personality.
 * All AI personalities have a name.
 * 
 * All AI personalities must implement a function which
 * takes in a pointer to a Level struct and a faction to play for and returns a list
 * of pairs, or tuples, of planets. The first planet in each pair
 * is the source planet, and the second planet is the target planet.
 * Each source planet will launch a fleet to the corresponding target planet.
 * Within the function, the AI may analyze the level state any way it sees fit.
 * The function should return a dynamically allocated array of PlanetPair structs,
 * and set the outPairCount parameter to the number of pairs returned, or NULL and
 * 0 if no actions are to be taken.
 * 
 * The caller is responsible for freeing the returned array.
 */
typedef struct AIPersonality {
    const char *name; /** Identifier for the AI personality */

    /** 
     * Decides the actions for the AI based on the current level state.
     * @param self Pointer to the AIPersonality instance.
     * @param level Pointer to the Level struct representing the current game state.
     * @param outPairCount Pointer to an integer where the function will store
     *                     the number of PlanetPair structs returned.
     * @param faction Pointer to the Faction for whom the AI is making decisions.
     * @return A dynamically allocated array of PlanetPair structs representing
     *         the actions to take. The caller is responsible for freeing this array.
     */
    PlanetPair* (*decideActions)(struct AIPersonality *self, struct Level *level, int *outPairCount, struct Faction *faction);
} AIPersonality;

/**
 * Global registry of AI personalities.
 * The ordering of this array should match the order in AI_PERSONALITIES.
 */
extern AIPersonality *g_aiPersonalities[AI_PERSONALITY_COUNT];

#endif // AI_AIPERSONALITY_H
