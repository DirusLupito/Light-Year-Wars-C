/**
 * Defines the the Basic AI personality.
 * This AI personality does nothing,
 * and will always return 0 and NULL from its action function.
 * @file basicAI.c
 * @author abmize
 */
#include "Objects/level.h"
#include "AI/basicAI.h"
#include "AI/aiPersonality.h"

/**
 * The Basic AI personality instance.
 * This exists so the global registry can reference a single, shared personality.
 */
AIPersonality BASIC_AI_PERSONALITY = {
    "Basic",
    basicAIDecideActions
};

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
PlanetPair* basicAIDecideActions(struct AIPersonality *self, struct Level *level, int *outPairCount, struct Faction *faction) {
    // Basic error checking.
    if (self == NULL || level == NULL || outPairCount == NULL || faction == NULL) {
        return NULL;
    }
    
    // Prepare a dynamic array to hold the planet pairs.
    PlanetPair *pairs = NULL;
    int pairCapacity = 0;

    // Iterate over all planets in the level.
    for (size_t i = 0; i < level->planetCount; ++i) {
        Planet *origin = &level->planets[i];

        // Only consider planets owned by the AI's faction.
        if (origin->owner != faction) {
            continue;
        }

        // Check if the planet is ready to launch a fleet.
        if (origin->currentFleetSize < origin->maxFleetCapacity) {
            continue;
        }

        // Find the nearest enemy planet.
        Planet *nearestEnemy = NULL;
        float nearestDistanceSq = -1.0f;

        for (size_t j = 0; j < level->planetCount; ++j) {
            Planet *destination = &level->planets[j];

            // Skip planets owned by the AI's faction.
            if (destination->owner == faction) {
                continue;
            }

            // Calculate squared distance to avoid sqrt for performance.
            float dx = destination->position.x - origin->position.x;
            float dy = destination->position.y - origin->position.y;
            float distanceSq = dx * dx + dy * dy;

            // Update nearest enemy planet if closer.
            if (nearestEnemy == NULL || distanceSq < nearestDistanceSq) {
                nearestEnemy = destination;
                nearestDistanceSq = distanceSq;
            }
        }

        // If a nearest enemy planet was found, add the pair to the list.
        if (nearestEnemy != NULL) {
            // Resize the pairs array if necessary.
            if (*outPairCount >= pairCapacity) {
                pairCapacity = pairCapacity == 0 ? 4 : pairCapacity * 2;
                pairs = realloc(pairs, pairCapacity * sizeof(PlanetPair));
            }

            // Add the new planet pair.
            pairs[*outPairCount].origin = origin;
            pairs[*outPairCount].destination = nearestEnemy;
            (*outPairCount)++;
        }
    }

    return pairs;
}
