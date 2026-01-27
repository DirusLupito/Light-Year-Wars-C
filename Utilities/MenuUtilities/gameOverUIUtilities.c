/**
 * Implementation of game over UI utilities.
 * Provides a reusable overlay panel to present victory/defeat state
 * and handle dismissal/return-to-lobby actions.
 * @file Utilities/MenuUtilities/gameOverUIUtilities.c
 * @author abmize
 */

#include "Utilities/MenuUtilities/gameOverUIUtilities.h"

// Button labels for client and server overlay actions.
static const char *kGameOverButtonClientLabel = "Ok";
static const char *kGameOverButtonServerLabel = "Return to Lobby";

// Panel sizing constants for the overlay.
static const float GAME_OVER_PANEL_WIDTH = 360.0f;
static const float GAME_OVER_PANEL_HEIGHT = 200.0f;
static const float GAME_OVER_PANEL_PADDING = 24.0f;

// Title text sizing for the overlay header.
static const float GAME_OVER_TITLE_TEXT_HEIGHT = 28.0f;
static const float GAME_OVER_TITLE_TEXT_WIDTH = 14.0f;

// Accent colors for victory and defeat.
static const float GAME_OVER_VICTORY_COLOR[4] = {0.25f, 0.9f, 0.3f, 1.0f};
static const float GAME_OVER_DEFEAT_COLOR[4] = {0.95f, 0.2f, 0.2f, 1.0f};
static const float GAME_OVER_NEUTRAL_COLOR[4] = {0.95f, 0.95f, 0.95f, 1.0f};

/**
 * Matches a faction ID to a Faction object within the level.
 * @param level Pointer to the Level to search.
 * @param factionId The faction id to locate.
 * @return Pointer to the faction or NULL if not found.
 */
static const Faction *GameOverUIResolveFaction(const Level *level, int factionId) {
    if (level == NULL || level->factions == NULL || factionId < 0) {
        return NULL;
    }

    // Iterate through factions to find a matching ID.
    for (size_t i = 0; i < level->factionCount; ++i) {
        if (level->factions[i].id == factionId) {
            return &level->factions[i];
        }
    }

    return NULL;
}

/**
 * Computes the overlay layout rects so hit testing and drawing stay in sync.
 * @param state Pointer to the GameOverUIState to read mode flags from.
 * @param width Current viewport width.
 * @param height Current viewport height.
 * @param panelOut Output for the panel rectangle.
 */
static void GameOverUIComputeLayout(GameOverUIState *state, int width, int height, MenuUIRect *panelOut) {
    if (state == NULL || panelOut == NULL) {
        return;
    }

    // We clamp dimensions to keep layout math stable on tiny viewports.
    float w = width > 1 ? (float)width : 1.0f;
    float h = height > 1 ? (float)height : 1.0f;

    float panelWidth = fminf(GAME_OVER_PANEL_WIDTH, w - GAME_OVER_PANEL_PADDING * 2.0f);
    if (panelWidth < 220.0f) {
        // We enforce a minimum so the button text remains readable.
        panelWidth = fminf(220.0f, w);
    }

    float panelHeight = GAME_OVER_PANEL_HEIGHT;
    float baseY = MenuLayoutComputeBaseY(panelHeight, h);
    float panelX = (w - panelWidth) * 0.5f;
    float panelY = baseY;

    panelOut->x = panelX;
    panelOut->y = panelY;
    panelOut->width = panelWidth;
    panelOut->height = panelHeight;

    // Button layout depends on the panel width, so we compute it here for consistency.
    float buttonY = panelY + panelHeight - GAME_OVER_PANEL_PADDING - state->actionButton.height;
    MenuButtonLayout(&state->actionButton, panelX + GAME_OVER_PANEL_PADDING, buttonY,
        panelWidth - GAME_OVER_PANEL_PADDING * 2.0f);
}

/**
 * Returns the title text associated with a result.
 * @param result The game-over result to represent.
 * @return A static string label.
 */
static const char *GameOverUITitleForResult(GameOverUIResult result) {
    if (result == GAME_OVER_UI_RESULT_VICTORY) {
        return "VICTORY";
    }

    if (result == GAME_OVER_UI_RESULT_DEFEAT) {
        return "DEFEAT";
    }

    return "GAME OVER";
}

/**
 * Returns the accent color for a result.
 * @param result The game-over result to represent.
 * @return Pointer to a RGBA color array.
 */
static const float *GameOverUIColorForResult(GameOverUIResult result) {
    if (result == GAME_OVER_UI_RESULT_VICTORY) {
        return GAME_OVER_VICTORY_COLOR;
    }

    if (result == GAME_OVER_UI_RESULT_DEFEAT) {
        return GAME_OVER_DEFEAT_COLOR;
    }

    return GAME_OVER_NEUTRAL_COLOR;
}

/**
 * Initializes the game over UI state.
 * @param state Pointer to the GameOverUIState to initialize.
 * @param serverMode True when running on the server, false for client usage.
 */
void GameOverUIInitialize(GameOverUIState *state, bool serverMode) {
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->serverMode = serverMode;
    state->visible = false;
    state->acknowledged = false;
    state->result = GAME_OVER_UI_RESULT_NONE;
    state->actionPending = false;
    state->actionPressed = false;

    // We swap the label based on role so the call-to-action matches behavior.
    const char *label = serverMode ? kGameOverButtonServerLabel : kGameOverButtonClientLabel;
    MenuButtonInitialize(&state->actionButton, label, 220.0f, 44.0f);
    state->actionButton.enabled = true;
}

/**
 * Resets the game over UI to its hidden default state.
 * @param state Pointer to the GameOverUIState to reset.
 */
void GameOverUIReset(GameOverUIState *state) {
    if (state == NULL) {
        return;
    }

    // We clear transient flags so a new match can show the overlay again.
    state->visible = false;
    state->acknowledged = false;
    state->result = GAME_OVER_UI_RESULT_NONE;
    state->actionPending = false;
    state->actionPressed = false;
    state->actionButton.pressed = false;
}

/**
 * Shows the overlay with the supplied result.
 * @param state Pointer to the GameOverUIState to modify.
 * @param result Outcome to display.
 */
void GameOverUIShowResult(GameOverUIState *state, GameOverUIResult result) {
    if (state == NULL) {
        return;
    }

    // We force visibility so users cannot miss the game-over state.
    state->visible = true;
    state->acknowledged = false;
    state->result = result;
}

/**
 * Returns whether the overlay is currently visible.
 * @param state Pointer to the GameOverUIState to read.
 * @return True if visible, false otherwise.
 */
bool GameOverUIIsVisible(const GameOverUIState *state) {
    if (state == NULL) {
        return false;
    }

    return state->visible;
}

/**
 * Returns whether the client has already dismissed the overlay.
 * @param state Pointer to the GameOverUIState to read.
 * @return True if acknowledged, false otherwise.
 */
bool GameOverUIIsAcknowledged(const GameOverUIState *state) {
    if (state == NULL) {
        return false;
    }

    return state->acknowledged;
}

/**
 * Handles mouse movement for hover feedback.
 * @param state Pointer to the GameOverUIState to modify.
 * @param x Mouse X coordinate.
 * @param y Mouse Y coordinate.
 */
void GameOverUIHandleMouseMove(GameOverUIState *state, float x, float y) {
    if (state == NULL || !state->visible) {
        return;
    }

    state->mouseX = x;
    state->mouseY = y;
}

/**
 * Handles mouse button down events for the overlay.
 * @param state Pointer to the GameOverUIState to modify.
 * @param x Mouse X coordinate.
 * @param y Mouse Y coordinate.
 * @param width Current viewport width.
 * @param height Current viewport height.
 * @return True if the event was consumed, false otherwise.
 */
bool GameOverUIHandleMouseDown(GameOverUIState *state, float x, float y, int width, int height) {
    if (state == NULL || !state->visible) {
        return false;
    }

    state->mouseX = x;
    state->mouseY = y;

    MenuUIRect panel;
    GameOverUIComputeLayout(state, width, height, &panel);

    // We allow clicks only within the overlay so gameplay input never leaks through.
    state->actionPressed = MenuButtonHandleMouseDown(&state->actionButton, x, y);
    return true;
}

/**
 * Handles mouse button up events for the overlay.
 * @param state Pointer to the GameOverUIState to modify.
 * @param x Mouse X coordinate.
 * @param y Mouse Y coordinate.
 * @param width Current viewport width.
 * @param height Current viewport height.
 * @return True if the event was consumed, false otherwise.
 */
bool GameOverUIHandleMouseUp(GameOverUIState *state, float x, float y, int width, int height) {
    if (state == NULL || !state->visible) {
        return false;
    }

    state->mouseX = x;
    state->mouseY = y;

    MenuUIRect panel;
    GameOverUIComputeLayout(state, width, height, &panel);

    if (state->actionPressed) {
        bool activated = false;
        MenuButtonHandleMouseUp(&state->actionButton, x, y, &activated);
        if (activated) {
            // We defer actions to the main loop so UI handling stays straightforward.
            state->actionPending = true;
        }
        state->actionPressed = false;
        state->actionButton.pressed = false;
    }

    return true;
}

/**
 * Consumes any pending action from the overlay button.
 * @param state Pointer to the GameOverUIState to modify.
 * @return True if an action was consumed, false otherwise.
 */
bool GameOverUIConsumeAction(GameOverUIState *state) {
    if (state == NULL || !state->actionPending) {
        return false;
    }

    state->actionPending = false;

    // Clients should be able to dismiss the overlay so it does not block their view.
    if (!state->serverMode) {
        state->visible = false;
        state->acknowledged = true;
    }

    return true;
}

/**
 * Draws the game over overlay.
 * @param state Pointer to the GameOverUIState to render.
 * @param context OpenGL context used for rendering.
 * @param width Current viewport width.
 * @param height Current viewport height.
 */
void GameOverUIDraw(GameOverUIState *state, OpenGLContext *context, int width, int height) {
    if (state == NULL || context == NULL || !state->visible || width <= 0 || height <= 0) {
        return;
    }

    MenuUIRect panel;
    GameOverUIComputeLayout(state, width, height, &panel);

    float outline[4] = MENU_PANEL_OUTLINE_COLOR;
    float fill[4] = MENU_PANEL_FILL_COLOR;
    DrawOutlinedRectangle(panel.x, panel.y, panel.x + panel.width, panel.y + panel.height, outline, fill);

    const char *title = GameOverUITitleForResult(state->result);
    const float *titleColor = GameOverUIColorForResult(state->result);

    // Center the title so the result reads as the focal point of the overlay.
    float titleWidth = (float)strlen(title) * GAME_OVER_TITLE_TEXT_WIDTH;
    float titleX = panel.x + (panel.width - titleWidth) * 0.5f;
    float titleY = panel.y + GAME_OVER_PANEL_PADDING + GAME_OVER_TITLE_TEXT_HEIGHT;
    DrawScreenText(context, title, titleX, titleY, GAME_OVER_TITLE_TEXT_HEIGHT, GAME_OVER_TITLE_TEXT_WIDTH, titleColor);

    // Draw the action button below the title.
    MenuButtonDraw(&state->actionButton, context, state->mouseX, state->mouseY, state->actionButton.enabled);
}

/**
 * Determines whether a game-ending winner has been established.
 * Teams win when all owned factions and starships share the same team.
 * Free-for-all wins are allowed when the single remaining owner has no team.
 * @param level Pointer to the Level to evaluate.
 * @param outTeamNumber Output for the winning team number.
 * @param outWinningFactionId Output for the winning faction id when team is none.
 * @return True if a winning team or faction is detected, false otherwise.
 */
bool GameOverUIComputeWinningTeam(const Level *level, int *outTeamNumber, int *outWinningFactionId) {
    if (level == NULL || level->factions == NULL || level->factionCount == 0) {
        return false;
    }

    int winningTeam = FACTION_TEAM_NONE;
    int winningFactionId = -1;
    bool teamSet = false;
    
    // We can just iterate over all planets and starships directly,
    // checking their owners' team numbers.
    for (size_t i = 0; i < level->planetCount; ++i) {
        const Faction *owner = level->planets[i].owner;
        // No owner, therefore doesn't impact victory conditions.
        if (owner == NULL) {
            continue;
        }

        int teamNumber = owner->teamNumber;
        if (!teamSet) {
            winningTeam = teamNumber;
            teamSet = true;
            // Only used if the winning team is FACTION_TEAM_NONE.
            winningFactionId = owner->id;
        } else if (teamNumber != winningTeam) {
            return false;
        } else if (winningTeam == FACTION_TEAM_NONE) {
            // If the winning team is FACTION_TEAM_NONE, we need to check and see if 
            // this planet is owned by a different faction.
            if (owner->id != winningFactionId) {
                return false;
            }
        }
    }

    // Starships must also belong to the same team to prevent stalemates from ending early.
    for (size_t i = 0; i < level->starshipCount; ++i) {
        const Faction *owner = level->starships[i].owner;
        if (owner == NULL || owner->teamNumber != winningTeam) {
            return false;
        }

        // If the winning team is FACTION_TEAM_NONE, we need to check and see if 
        // this starship is owned by a different faction.
        if (winningTeam == FACTION_TEAM_NONE && owner->id != winningFactionId) {
            return false;
        }
    }

    if (outTeamNumber != NULL) {
        *outTeamNumber = winningTeam;
    }

    if (outWinningFactionId != NULL) {
        *outWinningFactionId = winningFactionId;
    }

    return true;
}

/**
 * Determines the victory/defeat result for a specific faction.
 * @param level Pointer to the Level containing factions.
 * @param factionId Faction id to evaluate.
 * @param winningTeam The team number that won.
 * @param winningFactionId Winning faction id when the team is FACTION_TEAM_NONE.
 * @return The result for the faction.
 */
GameOverUIResult GameOverUIGetResultForFaction(const Level *level, int factionId, int winningTeam, int winningFactionId) {
    if (level == NULL) {
        return GAME_OVER_UI_RESULT_NONE;
    }

    const Faction *faction = GameOverUIResolveFaction(level, factionId);
    if (faction == NULL) {
        return GAME_OVER_UI_RESULT_NONE;
    }

    // We must distinguish team wins from free-for-all wins to avoid false victories.
    if (winningTeam == FACTION_TEAM_NONE) {
        return faction->id == winningFactionId ? GAME_OVER_UI_RESULT_VICTORY : GAME_OVER_UI_RESULT_DEFEAT;
    }

    return faction->teamNumber == winningTeam ? GAME_OVER_UI_RESULT_VICTORY : GAME_OVER_UI_RESULT_DEFEAT;
}
