/**
 * Header file for the Idle AI personality.
 * @file idleAI.h
 * @author abmize
 */
#ifndef AI_IDLEAI_H
#define AI_IDLEAI_H

// Forward declarations prevent circular includes with aiPersonality.h.
typedef struct AIPersonality AIPersonality;
typedef struct PlanetPair PlanetPair;
struct Level;

/**
 * The Idle AI personality instance.
 * This can be registered in the global AI personality list.
 */
extern AIPersonality IDLE_AI_PERSONALITY;

/**
 * The action function for the Idle AI personality.
 * This function does nothing and always returns NULL and 0.
 * @param level A pointer to the current Level struct.
 * @param outPairCount A pointer to an integer where the number of returned pairs will be stored.
 * @return Always returns NULL.
 */
PlanetPair* idleAIDecideActions(struct AIPersonality *self, struct Level *level, int *outPairCount);

#endif // AI_IDLEAI_H
