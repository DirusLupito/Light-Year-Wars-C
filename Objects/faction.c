/**
 * Implements a faction object.
 * A faction represents a group of planets and starships controlled by a player or AI.
 * A faction has an ID and a color.
 * @file Objects/faction.c
 * @author abmize
 */

#include "Objects/faction.h"
#include "AI/aiPersonality.h"

/**
 * Helper function to set color components.
 * The alpha component (0.0 to 1.0), is locked at 1.0.
 * This component could be extended in the future if needed 
 * to allow for transparent faction colors.
 * @param color The color array to modify (must have at least 4 elements).
 * @param r The red component (0.0 to 1.0).
 * @param g The green component (0.0 to 1.0).
 * @param b The blue component (0.0 to 1.0).
 */
static void SetColorComponents(float color[4], float r, float g, float b) {
    color[0] = r;
    color[1] = g;
    color[2] = b;
    color[3] = 1.0f;
}

/**
 * Creates a new faction with the specified ID and color.
 * @param id The ID of the faction.
 * @param r The red component of the faction's color (0.0 to 1.0).
 * @param g The green component of the faction's color (0.0 to 1.0).
 * @param b The blue component of the faction's color (0.0 to 1.0).
 * @return The created Faction object.
 */
Faction CreateFaction(int id, float r, float g, float b) {
    Faction faction;
    faction.id = id;
    SetColorComponents(faction.color, r, g, b);
    // Default to no AI so human control is the default.
    faction.aiPersonality = NULL;
    return faction;
}

/**
 * Sets the color of the specified faction.
 * @param faction A pointer to the Faction object to modify.
 * @param r The red component of the faction's color (0.0 to 1.0).
 * @param g The green component of the faction's color (0.0 to 1.0).
 * @param b The blue component of the faction's color (0.0 to 1.0).
 */
void FactionSetColor(Faction *faction, float r, float g, float b) {
    if (faction == NULL) {
        return;
    }

    SetColorComponents(faction->color, r, g, b);
}

/**
 * Assigns an AI personality to the faction or clears it for human control.
 * @param faction A pointer to the Faction object to modify.
 * @param personality Pointer to the AI personality to assign, or NULL for none.
 */
void FactionSetAIPersonality(Faction *faction, AIPersonality *personality) {
    if (faction == NULL) {
        return;
    }

    // We store the pointer directly so the AI personality registry stays authoritative.
    faction->aiPersonality = personality;
}
