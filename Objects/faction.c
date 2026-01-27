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
    // Default to no team and no shared control so factions start hostile.
    faction.teamNumber = FACTION_TEAM_NONE;
    faction.sharedControlNumber = FACTION_SHARED_CONTROL_NONE;
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

/**
 * Sets the team number for the specified faction.
 * A negative value clears the team assignment.
 * @param faction A pointer to the Faction object to modify.
 * @param teamNumber The team number to assign, or a negative value for none.
 */
void FactionSetTeamNumber(Faction *faction, int teamNumber) {
    if (faction == NULL) {
        return;
    }

    // Negative values signal no team so hostile behavior is preserved.
    faction->teamNumber = teamNumber >= 0 ? teamNumber : FACTION_TEAM_NONE;
}

/**
 * Sets the shared control/archon number for the specified faction.
 * A negative value clears the shared control assignment.
 * @param faction A pointer to the Faction object to modify.
 * @param sharedControlNumber The shared control number, or a negative value for none.
 */
void FactionSetSharedControlNumber(Faction *faction, int sharedControlNumber) {
    if (faction == NULL) {
        return;
    }

    // Negative values disable shared control so only explicit matches enable it.
    faction->sharedControlNumber = sharedControlNumber >= 0 ? sharedControlNumber : FACTION_SHARED_CONTROL_NONE;
}

/**
 * Determines whether two factions are on the same team.
 * This considers identical factions friendly even if they have no team.
 * @param a Pointer to the first faction.
 * @param b Pointer to the second faction.
 * @return true if the factions are friendly, false otherwise.
 */
bool FactionIsFriendly(const Faction *a, const Faction *b) {
    // Without both factions, we cannot establish any relationship.
    if (a == NULL || b == NULL) {
        return false;
    }

    // A faction is always friendly to itself to preserve normal ownership logic.
    if (a == b) {
        return true;
    }

    // If either faction has no team, they remain hostile by default.
    if (a->teamNumber == FACTION_TEAM_NONE || b->teamNumber == FACTION_TEAM_NONE) {
        return false;
    }

    return a->teamNumber == b->teamNumber;
}

/**
 * Determines whether two factions share control permissions.
 * Shared control requires matching team numbers and shared control numbers.
 * Identical factions always share control.
 * @param a Pointer to the first faction.
 * @param b Pointer to the second faction.
 * @return true if the factions can control each other, false otherwise.
 */
bool FactionSharesControl(const Faction *a, const Faction *b) {
    // Without both factions, no shared control is possible.
    if (a == NULL || b == NULL) {
        return false;
    }

    // A faction can always control itself.
    if (a == b) {
        return true;
    }

    // Shared control only applies to friendly factions with matching control numbers.
    if (!FactionIsFriendly(a, b)) {
        return false;
    }

    if (a->sharedControlNumber == FACTION_SHARED_CONTROL_NONE || b->sharedControlNumber == FACTION_SHARED_CONTROL_NONE) {
        return false;
    }

    return a->sharedControlNumber == b->sharedControlNumber;
}
