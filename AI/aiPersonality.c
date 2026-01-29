
/**
 * Defines the global AI personality registry.
 * @file aiPersonality.c
 * @author abmize
 */

#include "AI/aiPersonality.h"

/**
 * Global registry of AI personalities.
 * This is kept in a single array so UI and gameplay systems can iterate easily.
 */
AIPersonality *g_aiPersonalities[AI_PERSONALITY_COUNT] = {
    AI_PERSONALITIES
};
