/**
 * Header for game over UI utilities.
 * Provides a reusable overlay panel to present victory/defeat state
 * and handle dismissal/return-to-lobby actions.
 * @file Utilities/MenuUtilities/gameOverUIUtilities.h
 * @author abmize
 */
#ifndef _GAME_OVER_UI_UTILITIES_H_
#define _GAME_OVER_UI_UTILITIES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "Objects/level.h"
#include "Objects/faction.h"
#include "Utilities/MenuUtilities/menuComponentUtilities.h"
#include "Utilities/MenuUtilities/commonMenuUtilities.h"

/**
 * Enumerates the possible game-over outcomes.
 */
typedef enum GameOverUIResult {
    GAME_OVER_UI_RESULT_NONE = 0,
    GAME_OVER_UI_RESULT_VICTORY = 1,
    GAME_OVER_UI_RESULT_DEFEAT = 2
} GameOverUIResult;

/**
 * Stores state for the game over overlay UI.
 */
typedef struct GameOverUIState {
    bool visible; /* True when the overlay is visible. */
    bool acknowledged; /* True once the client dismissed the overlay. */
    bool serverMode; /* True when running on the server for return-to-lobby behavior. */
    GameOverUIResult result; /* Cached result for rendering the title text. */
    bool actionPending; /* True when the button was activated and awaits consumption. */
    bool actionPressed; /* True while the action button is pressed. */
    float mouseX; /* Cached mouse X coordinate for hover feedback. */
    float mouseY; /* Cached mouse Y coordinate for hover feedback. */
    MenuButtonComponent actionButton; /* The dismiss/return button component. */
} GameOverUIState;

/**
 * Initializes the game over UI state.
 * @param state Pointer to the GameOverUIState to initialize.
 * @param serverMode True when running on the server, false for client usage.
 */
void GameOverUIInitialize(GameOverUIState *state, bool serverMode);

/**
 * Resets the game over UI to its hidden default state.
 * @param state Pointer to the GameOverUIState to reset.
 */
void GameOverUIReset(GameOverUIState *state);

/**
 * Shows the overlay with the supplied result.
 * @param state Pointer to the GameOverUIState to modify.
 * @param result Outcome to display.
 */
void GameOverUIShowResult(GameOverUIState *state, GameOverUIResult result);

/**
 * Returns whether the overlay is currently visible.
 * @param state Pointer to the GameOverUIState to read.
 * @return True if visible, false otherwise.
 */
bool GameOverUIIsVisible(const GameOverUIState *state);

/**
 * Returns whether the client has already dismissed the overlay.
 * @param state Pointer to the GameOverUIState to read.
 * @return True if acknowledged, false otherwise.
 */
bool GameOverUIIsAcknowledged(const GameOverUIState *state);

/**
 * Handles mouse movement for hover feedback.
 * @param state Pointer to the GameOverUIState to modify.
 * @param x Mouse X coordinate.
 * @param y Mouse Y coordinate.
 */
void GameOverUIHandleMouseMove(GameOverUIState *state, float x, float y);

/**
 * Handles mouse button down events for the overlay.
 * @param state Pointer to the GameOverUIState to modify.
 * @param x Mouse X coordinate.
 * @param y Mouse Y coordinate.
 * @param width Current viewport width.
 * @param height Current viewport height.
 * @return True if the event was consumed, false otherwise.
 */
bool GameOverUIHandleMouseDown(GameOverUIState *state, float x, float y, int width, int height);

/**
 * Handles mouse button up events for the overlay.
 * @param state Pointer to the GameOverUIState to modify.
 * @param x Mouse X coordinate.
 * @param y Mouse Y coordinate.
 * @param width Current viewport width.
 * @param height Current viewport height.
 * @return True if the event was consumed, false otherwise.
 */
bool GameOverUIHandleMouseUp(GameOverUIState *state, float x, float y, int width, int height);

/**
 * Consumes any pending action from the overlay button.
 * @param state Pointer to the GameOverUIState to modify.
 * @return True if an action was consumed, false otherwise.
 */
bool GameOverUIConsumeAction(GameOverUIState *state);

/**
 * Draws the game over overlay.
 * @param state Pointer to the GameOverUIState to render.
 * @param context OpenGL context used for rendering.
 * @param width Current viewport width.
 * @param height Current viewport height.
 */
void GameOverUIDraw(GameOverUIState *state, OpenGLContext *context, int width, int height);

/**
 * Determines whether a game-ending winner has been established.
 * Teams win when all owned factions and starships share the same team.
 * Free-for-all wins are allowed when the single remaining owner has no team.
 * @param level Pointer to the Level to evaluate.
 * @param outTeamNumber Output for the winning team number.
 * @param outWinningFactionId Output for the winning faction id when team is none.
 * @return True if a winning team or faction is detected, false otherwise.
 */
bool GameOverUIComputeWinningTeam(const Level *level, int *outTeamNumber, int *outWinningFactionId);

/**
 * Determines the victory/defeat result for a specific faction.
 * @param level Pointer to the Level containing factions.
 * @param factionId Faction id to evaluate.
 * @param winningTeam The team number that won.
 * @param winningFactionId Winning faction id when the team is FACTION_TEAM_NONE.
 * @return The result for the faction.
 */
GameOverUIResult GameOverUIGetResultForFaction(const Level *level, int factionId, int winningTeam, int winningFactionId);

#endif /* _GAME_OVER_UI_UTILITIES_H_ */
