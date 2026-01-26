/**
 * Header file for the Basic AI personality.
 * @file basicAI.h
 * @author abmize
 */
#ifndef AI_BASICAI_H
#define AI_BASICAI_H

// Forward declarations prevent circular includes with aiPersonality.h.
typedef struct AIPersonality AIPersonality;
typedef struct PlanetPair PlanetPair;
struct Level;
struct Faction;

/**
 * The Basic AI personality instance.
 * This can be registered in the global AI personality list.
 */
extern AIPersonality BASIC_AI_PERSONALITY;


/**
 * The action function for the Basic AI personality.
 * The basic AI iterates over all planets owned by its faction.
 * If the planet has a current fleet size greater than or equal to its
 * max fleet capacity, it will launch a fleet to the nearest planet
 * not owned by its faction. If no such planet exists, no action is taken.
 * @param level A pointer to the current Level struct.
 * @param outPairCount A pointer to an integer where the number of returned pairs will be stored.
 * @param faction Pointer to the Faction for whom the AI is making decisions.
 * @return A list of PlanetPair structs representing the AI's decided actions.
 */
PlanetPair* basicAIDecideActions(struct AIPersonality *self, struct Level *level, int *outPairCount, struct Faction *faction);

#endif // AI_BASICAI_H