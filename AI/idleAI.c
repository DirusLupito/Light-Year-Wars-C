/**
 * Defines the the Idle AI personality.
 * This AI personality does nothing,
 * and will always return 0 and NULL from its action function.
 * @file idleAI.c
 * @author abmize
 */
#include "AI/idleAI.h"
#include "AI/aiPersonality.h"

/**
 * The Idle AI personality instance.
 * This exists so the global registry can reference a single, shared personality.
 */
AIPersonality IDLE_AI_PERSONALITY = {
    "Idle",
    idleAIDecideActions
};

/**
 * The action function for the Idle AI personality.
 * This function does nothing and always returns NULL and 0.
 * @param self A pointer to the AI personality struct (not used).
 * @param level A pointer to the current Level struct (not used).
 * @param outPairCount A pointer to an integer where the number of returned pairs will be stored.
 * @return Always returns NULL.
 */
PlanetPair* idleAIDecideActions(struct AIPersonality *self, struct Level *level, int *outPairCount) {
    // Idle AI ignores its inputs by design to avoid influencing the simulation.
    (void)self;
    (void)level;

    // Idle AI does nothing, so we set the output count to 0 and return NULL.
    if (outPairCount) {
        *outPairCount = 0;
    }
    return NULL;
}
