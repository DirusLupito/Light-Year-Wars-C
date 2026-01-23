/**
 * Header for lobby menu utilities shared by server and client.
 * Provides state structures and helpers for displaying and interacting
 * with the lobby UI.
 * @file Utilities/MenuUtilities/lobbyMenuUtilities.h
 * @author abmize
 */
#ifndef _LOBBY_MENU_UTILITIES_H_
#define _LOBBY_MENU_UTILITIES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <windows.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Utilities/renderUtilities.h"
#include "Utilities/openglUtilities.h"
#include "Utilities/MenuUtilities/commonMenuUtilities.h"
#include "Utilities/MenuUtilities/colorPickerUtilities.h"

// Maximum number of player slots in the lobby.
#define LOBBY_MENU_MAX_SLOTS 16

// Maximum length of the status message string.
#define LOBBY_MENU_STATUS_MAX_LENGTH 127

// Number of input fields in the lobby menu.
#define LOBBY_MENU_FIELD_COUNT 7

// The padding between UI elements and the panel edges.
#define LOBBY_MENU_PANEL_PADDING 32.0f

// Minimum and maximum widths for the lobby menu panel.
#define LOBBY_MENU_PANEL_MIN_WIDTH 360.0f

// Maximum widths for the lobby menu panel.
#define LOBBY_MENU_PANEL_MAX_WIDTH 760.0f

// The padding at the top of the lobby menu panel.
#define LOBBY_MENU_TOP_PADDING 48.0f

// The padding at the bottom of the lobby menu panel.
#define LOBBY_MENU_BOTTOM_PADDING 40.0f

// Heigh of each input field in the lobby menu.
#define LOBBY_MENU_FIELD_HEIGHT 44.0f

// Spacing between input fields in the lobby menu.
#define LOBBY_MENU_FIELD_SPACING 20.0f

// Height of the start button in the lobby menu.
#define LOBBY_MENU_BUTTON_HEIGHT 50.0f

// Width of the start button in the lobby menu.
#define LOBBY_MENU_BUTTON_WIDTH 260.0f

// Spacing between the input fields and the button section.
#define LOBBY_MENU_BUTTON_SECTION_SPACING 28.0f

// Spacing between sections in the lobby menu.
#define LOBBY_MENU_SECTION_SPACING 30.0f

// Height of each slot row in the lobby menu.
#define LOBBY_MENU_SLOT_ROW_HEIGHT 26.0f

// Spacing between slot rows in the lobby menu.
#define LOBBY_MENU_SLOT_ROW_SPACING 6.0f


/**
 * Defines the possible focus targets within the lobby menu.
 * Used to track which input field is currently active.
 */
typedef enum LobbyMenuFocusTarget {
    LOBBY_MENU_FOCUS_NONE = -1,
    LOBBY_MENU_FOCUS_PLANET_COUNT = 0,
    LOBBY_MENU_FOCUS_FACTION_COUNT = 1,
    LOBBY_MENU_FOCUS_MIN_FLEET = 2,
    LOBBY_MENU_FOCUS_MAX_FLEET = 3,
    LOBBY_MENU_FOCUS_LEVEL_WIDTH = 4,
    LOBBY_MENU_FOCUS_LEVEL_HEIGHT = 5,
    LOBBY_MENU_FOCUS_RANDOM_SEED = 6
} LobbyMenuFocusTarget;

/**
 * Defines the types of values that can be entered in lobby menu fields.
 */
typedef enum LobbyMenuValueType {
    LOBBY_MENU_VALUE_INT = 0,
    LOBBY_MENU_VALUE_FLOAT = 1
} LobbyMenuValueType;

/**
 * Stores the generation settings for the lobby.
 * Currently includes planet count, faction count, 
 * fleet capacities, level dimensions, and random seed.
 */
typedef struct LobbyMenuGenerationSettings {
    int planetCount;
    int factionCount;
    float minFleetCapacity;
    float maxFleetCapacity;
    float levelWidth;
    float levelHeight;
    uint32_t randomSeed;
} LobbyMenuGenerationSettings;

/**
 * Stores UI state required for rendering and interacting with the lobby menu.
 * Contains current settings, input states, slot information, and status messages.
 */
typedef struct LobbyMenuUIState {
    LobbyMenuGenerationSettings settings; /* Last committed settings */
    bool editable; /* True when inputs are interactive. */
    LobbyMenuFocusTarget focus; /* Currently focused field for keyboard input. */
    bool startButtonPressed; /* Tracks pressed state for mouse handling. */
    bool startRequested; /* Latched when the Start Game button is activated. */
    float mouseX;
    float mouseY;
    float scrollOffset; /* Vertical scroll position for overflowing content. */
    char statusMessage[LOBBY_MENU_STATUS_MAX_LENGTH + 1];

    /* Field text buffers and lengths mirror editable inputs. */
    char fieldText[LOBBY_MENU_FIELD_COUNT][32];
    size_t fieldLength[LOBBY_MENU_FIELD_COUNT];

    /* Lobby slot information shared for rendering. */
    size_t slotCount;
    int slotFactionIds[LOBBY_MENU_MAX_SLOTS];
    bool slotOccupied[LOBBY_MENU_MAX_SLOTS];
    int highlightedFactionId; /* Slot to highlight (e.g. local faction). */

    /* Slot colors (RGBA 0-1) for display and editing. */
    float slotColors[LOBBY_MENU_MAX_SLOTS][4];
    bool slotColorValid[LOBBY_MENU_MAX_SLOTS];

    /* Color picker state for RGB selection. */
    ColorPickerUIState colorPicker;
} LobbyMenuUIState;

/**
 * Initializes the lobby menu UI state.
 * Sets default values and configures editability.
 * @param state Pointer to the LobbyMenuUIState to initialize.
 * @param editable True to make inputs interactive, false for read-only.
 */
void LobbyMenuUIInitialize(LobbyMenuUIState *state, bool editable);

/**
 * Sets whether the lobby menu inputs are editable.
 * Updates focus and button states accordingly.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param editable True to make inputs interactive, false for read-only.
 */
void LobbyMenuUISetEditable(LobbyMenuUIState *state, bool editable);

/**
 * Sets the current lobby generation settings in the UI state.
 * Updates input field buffers to reflect the new settings.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param settings Pointer to the LobbyMenuGenerationSettings to apply.
 */
void LobbyMenuUISetSettings(LobbyMenuUIState *state, const LobbyMenuGenerationSettings *settings);

/**
 * Retrieves and validates the current lobby generation settings from the UI state.
 * Parses input field buffers and updates the settings structure.
 * @param state Pointer to the LobbyMenuUIState to read from.
 * @param outSettings Pointer to store the parsed LobbyMenuGenerationSettings.
 * @return True if settings were successfully parsed and valid, false otherwise.
 */
bool LobbyMenuUIGetSettings(LobbyMenuUIState *state, LobbyMenuGenerationSettings *outSettings);

/**
 * Sets the number of slots in the lobby UI.
 * Updates slot information arrays accordingly.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param slotCount The desired number of slots (max LOBBY_MENU_MAX_SLOTS).
 */
void LobbyMenuUISetSlotCount(LobbyMenuUIState *state, size_t slotCount);

/**
 * Sets information for a specific lobby slot.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param index Index of the slot to update (0 to LOBBY_MENU_MAX_SLOTS - 1).
 * @param factionId Faction ID assigned to the slot, or -1 if unassigned.
 * @param occupied True if the slot is occupied, false otherwise.
 */
void LobbyMenuUISetSlotInfo(LobbyMenuUIState *state, size_t index, int factionId, bool occupied);

/**
 * Sets the display color for a specific lobby slot.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param index Index of the slot to update (0 to LOBBY_MENU_MAX_SLOTS - 1).
 * @param color RGBA color in 0-1 range.
 */
void LobbyMenuUISetSlotColor(LobbyMenuUIState *state, size_t index, const float color[4]);

/**
 * Sets the highlighted faction ID in the lobby UI.
 * Used to visually distinguish a specific slot (e.g. local player).
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param factionId Faction ID to highlight, or -1 for none.
 */
void LobbyMenuUISetHighlightedFactionId(LobbyMenuUIState *state, int factionId);

/**
 * Configures which faction colors the local user is allowed to edit.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param allowAll True to allow editing any slot color (server).
 * @param factionId Faction ID allowed to edit when allowAll is false (client).
 */
void LobbyMenuUISetColorEditAuthority(LobbyMenuUIState *state, bool allowAll, int factionId);

/**
 * Clears all lobby slots in the UI state.
 * Resets slot count and marks all slots as unoccupied.
 * @param state Pointer to the LobbyMenuUIState to modify.
 */
void LobbyMenuUIClearSlots(LobbyMenuUIState *state);

/**
 * Sets the status message displayed in the lobby UI.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param message Null-terminated string message to set, or NULL to clear.
 */
void LobbyMenuUISetStatusMessage(LobbyMenuUIState *state, const char *message);

/**
 * Handles mouse movement events within the lobby UI.
 * Updates internal mouse position state.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param x Current mouse X position.
 * @param y Current mouse Y position.
 */
void LobbyMenuUIHandleMouseMove(LobbyMenuUIState *state, float x, float y);

/**
 * Handles mouse button down events within the lobby UI.
 * Updates focus and button states based on mouse position.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param x Mouse X position at event.
 * @param y Mouse Y position at event.
 * @param width Current width of the UI area.
 * @param height Current height of the UI area.
 */
void LobbyMenuUIHandleMouseDown(LobbyMenuUIState *state, float x, float y, int width, int height);

/**
 * Handles mouse button up events within the lobby UI.
 * Updates button states and triggers actions as needed.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param x Mouse X position at event.
 * @param y Mouse Y position at event.
 * @param width Current width of the UI area.
 * @param height Current height of the UI area.
 */
void LobbyMenuUIHandleMouseUp(LobbyMenuUIState *state, float x, float y, int width, int height);

/**
 * Handles character input events for text fields in the lobby UI.
 * Updates the focused input field with the entered character.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param codepoint Unicode codepoint of the entered character.
 */
void LobbyMenuUIHandleChar(LobbyMenuUIState *state, unsigned int codepoint);

/**
 * Handles key down events for keyboard input in the lobby UI.
 * Manages navigation, editing, and special key actions.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param key Virtual key code of the pressed key.
 * @param shiftDown True if the Shift key is currently held down.
 */
void LobbyMenuUIHandleKeyDown(LobbyMenuUIState *state, WPARAM key, bool shiftDown);

/**
 * Consumes and clears any pending start game request from the lobby UI.
 * @param state Pointer to the LobbyMenuUIState to check.
 * @return True if a start request was pending, false otherwise.
 */
bool LobbyMenuUIConsumeStartRequest(LobbyMenuUIState *state);

/**
 * Consumes a committed color change from the lobby UI.
 * @param state Pointer to the LobbyMenuUIState.
 * @param outFactionId Output faction ID whose color changed.
 * @param outR Output red channel (0-255).
 * @param outG Output green channel (0-255).
 * @param outB Output blue channel (0-255).
 * @return True if a color change was pending, false otherwise.
 */
bool LobbyMenuUIConsumeColorCommit(LobbyMenuUIState *state, int *outFactionId, uint8_t *outR, uint8_t *outG, uint8_t *outB);

/**
 * Renders the lobby menu UI using OpenGL.
 * Draws panels, input fields, buttons, and status messages.
 * @param state Pointer to the LobbyMenuUIState to render.
 * @param context Pointer to the OpenGLContext for rendering.
 * @param width Current width of the UI area.
 * @param height Current height of the UI area.
 */
void LobbyMenuUIDraw(LobbyMenuUIState *state, OpenGLContext *context, int width, int height);

/**
 * Adjusts the lobby menu scroll position in response to mouse wheel input.
 * Positive wheel steps scroll the content upward (toward earlier items).
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param height Current height of the UI area.
 * @param wheelSteps Wheel delta expressed in multiples of WHEEL_DELTA (120).
 */
void LobbyMenuUIHandleScroll(LobbyMenuUIState *state, int height, float wheelSteps);

#endif /* _LOBBY_MENU_UTILITIES_H_ */
