/**
 * Implementation of lobby menu utilities shared by server and client.
 * Handles layout, rendering, and optional interaction for lobby settings.
 * @file Utilities/MenuUtilities/lobbyMenuUtilities.c
 * @author abmize
 */
#include "Utilities/MenuUtilities/lobbyMenuUtilities.h"

// Note that each of the following field
// definitions must correspond to the order of
// the LobbyMenuFocusTarget enum values
// and to each other.

// Labels for each input field in the lobby menu.
static const char *kFieldLabels[LOBBY_MENU_FIELD_COUNT] = {
    "Planet Count",
    "Faction Count",
    "Min Fleet Capacity",
    "Max Fleet Capacity",
    "Level Width",
    "Level Height",
    "Random Seed"
};

// Placeholder text for each input field in the lobby menu.
static const char *kFieldPlaceholders[LOBBY_MENU_FIELD_COUNT] = {
    "Total number of planets",
    "Playable faction slots (2 - 16)",
    "Minimum planet capacity",
    "Maximum planet capacity",
    "World width",
    "World height",
    "Seed (0 for default)"
};

// Types of values for each input field in the lobby menu.
static const LobbyMenuValueType kFieldTypes[LOBBY_MENU_FIELD_COUNT] = {
    LOBBY_MENU_VALUE_INT,
    LOBBY_MENU_VALUE_INT,
    LOBBY_MENU_VALUE_FLOAT,
    LOBBY_MENU_VALUE_FLOAT,
    LOBBY_MENU_VALUE_FLOAT,
    LOBBY_MENU_VALUE_FLOAT,
    LOBBY_MENU_VALUE_INT
};

/**
 * Computes the height of the lobby menu panel based on its contents.
 * Used for layout and scrolling calculations.
 * @param state Pointer to the LobbyMenuUIState to evaluate.
 * @return The computed panel height in pixels.
 */
static float LobbyMenuComputePanelHeight(const LobbyMenuUIState *state) {
    // Validate parameter.
    if (state == NULL) {
        return 0.0f;
    }

    // Field height should just be number of fields times field height plus spacing.
    float fieldsHeight = (float)LOBBY_MENU_FIELD_COUNT * LOBBY_MENU_FIELD_HEIGHT;
    if (LOBBY_MENU_FIELD_COUNT > 1) {
        fieldsHeight += (float)(LOBBY_MENU_FIELD_COUNT - 1) * LOBBY_MENU_FIELD_SPACING;
    }

    // Likewise, slot height is number of slots times row height plus spacing.
    float slotsHeight = (float)state->slotCount * LOBBY_MENU_SLOT_ROW_HEIGHT;
    if (state->slotCount > 1) {
        slotsHeight += (float)(state->slotCount - 1) * LOBBY_MENU_SLOT_ROW_SPACING;
    }

    // So the overall height is the sum of all sections plus padding.
    return LOBBY_MENU_TOP_PADDING + fieldsHeight + LOBBY_MENU_BUTTON_SECTION_SPACING +
        LOBBY_MENU_BUTTON_HEIGHT + LOBBY_MENU_SECTION_SPACING + slotsHeight + LOBBY_MENU_BOTTOM_PADDING;
}

/**
 * Computes the total content height of the lobby menu UI.
 * Includes panel height and any additional status message space.
 * @param state Pointer to the LobbyMenuUIState to evaluate.
 * @return The computed content height in pixels.
 */
static float LobbyMenuComputeContentHeight(const LobbyMenuUIState *state) {
    // We first figure out the height of the main panel.
    float contentHeight = LobbyMenuComputePanelHeight(state);
    float statusPadding = MENU_GENERIC_TEXT_HEIGHT;

    // If there's a status message, add space for it.
    if (state->statusMessage[0] != '\0') {
        statusPadding += MENU_GENERIC_TEXT_HEIGHT;
    }

    // Finally, return the total content height (panel + status).
    return contentHeight + statusPadding;
}

/**
 * Computes the base Y position for the lobby menu content
 * to achieve vertical centering within the viewport.
 * @param contentHeight The total height of the lobby menu content.
 * @param viewportHeight The height of the viewport area.
 * @return The computed base Y position in pixels.
 */
static float LobbyMenuComputeBaseY(float contentHeight, float viewportHeight) {
    // If viewport height is non-positive, just return a default offset.
    if (viewportHeight <= 0.0f) {
        return 16.0f;
    }

    // If content fits within the viewport, center it vertically.
    if (contentHeight <= viewportHeight) {
        float centered = (viewportHeight - contentHeight) * 0.5f;

        // And if centering goes too high, clamp to a minimum offset.
        if (centered < 16.0f) {
            centered = 16.0f;
        }
        return centered;
    }

    return 16.0f;
}

/**
 * Clamps the scroll offset of the lobby menu UI state
 * to ensure it stays within valid bounds.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param viewportHeight The height of the viewport area.
 * @return The clamped scroll offset in pixels.
 */
static float LobbyMenuClampScroll(LobbyMenuUIState *state, float viewportHeight) {
    // Nothing to clamp if state is null.
    if (state == NULL) {
        return 0.0f;
    }

    // The maximum scroll offset is content height minus viewport height
    // (ensuring we don't scroll past the bottom).
    float maxScroll = LobbyMenuComputeContentHeight(state) - viewportHeight;
    if (maxScroll < 0.0f) {
        maxScroll = 0.0f;
    }

    // Clamp the current scroll offset within [0, maxScroll].
    float clamped = state->scrollOffset;
    if (clamped < 0.0f) {
        clamped = 0.0f;
    }
    if (clamped > maxScroll) {
        clamped = maxScroll;
    }

    // Update the state with the clamped value.
    state->scrollOffset = clamped;
    return clamped;
}

/**
 * Converts a LobbyMenuFocusTarget to its corresponding field index.
 * @param focus The LobbyMenuFocusTarget to convert.
 * @return The corresponding field index, or -1 if invalid.
 */
static int FocusToIndex(LobbyMenuFocusTarget focus) {
    // If the focus target is out of range, return -1.
    if (focus < LOBBY_MENU_FOCUS_PLANET_COUNT || focus > LOBBY_MENU_FOCUS_RANDOM_SEED) {
        return -1;
    }

    // Otherwise, return the integer value of the focus target.
    return (int)focus;
}

/**
 * Sets the text buffer and length for a specific input field.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param index Index of the field to update.
 * @param text Null-terminated string to set as the field text.
 */
static void SetFieldText(LobbyMenuUIState *state, size_t index, const char *text) {
    // Validate parameters.
    if (state == NULL || index >= LOBBY_MENU_FIELD_COUNT || text == NULL) {
        return;
    }

    // Copy the text into the field buffer, ensuring null-termination.
    size_t capacity = sizeof(state->fieldText[index]);
    strncpy(state->fieldText[index], text, capacity - 1);
    state->fieldText[index][capacity - 1] = '\0';

    // Update the field length.
    state->fieldLength[index] = strlen(state->fieldText[index]);
}

/**
 * Updates the input field text buffers from the current settings in the UI state.
 * @param state Pointer to the LobbyMenuUIState to read from.
 */
static void UpdateBuffersFromSettings(LobbyMenuUIState *state) {
    // Validate parameter.
    if (state == NULL) {
        return;
    }

    // Temporary buffer for holding string representations
    // of each setting value.
    char buffer[64];

    // Update each field's text buffer with the corresponding setting value. 

    snprintf(buffer, sizeof(buffer), "%d", state->settings.planetCount);
    SetFieldText(state, 0u, buffer);
    
    snprintf(buffer, sizeof(buffer), "%d", state->settings.factionCount);
    SetFieldText(state, 1u, buffer);

    snprintf(buffer, sizeof(buffer), "%g", state->settings.minFleetCapacity);
    SetFieldText(state, 2u, buffer);

    snprintf(buffer, sizeof(buffer), "%g", state->settings.maxFleetCapacity);
    SetFieldText(state, 3u, buffer);

    snprintf(buffer, sizeof(buffer), "%g", state->settings.levelWidth);
    SetFieldText(state, 4u, buffer);

    snprintf(buffer, sizeof(buffer), "%g", state->settings.levelHeight);
    SetFieldText(state, 5u, buffer);

    snprintf(buffer, sizeof(buffer), "%u", state->settings.randomSeed);
    SetFieldText(state, 6u, buffer);
}

/**
 * Clamps window dimensions to minimum values.
 * @param width Pointer to the width value to clamp.
 * @param height Pointer to the height value to clamp.
 */
static void ClampWindowDimensions(float *width, float *height) {
    // Ensure minimum dimensions of 1.0f to avoid issues.
    if (width != NULL && *width < 1.0f) {
        *width = 1.0f;
    }
    if (height != NULL && *height < 1.0f) {
        *height = 1.0f;
    }
}

/**
 * Computes layout rectangles for the lobby menu UI components.
 * @param state Pointer to the LobbyMenuUIState for slot count.
 * @param width Current width of the UI area.
 * @param height Current height of the UI area.
 * @param panelOut Pointer to store the panel rectangle, or NULL to ignore.
 * @param fieldRects Array to store field rectangles, or NULL to ignore.
 * @param buttonOut Pointer to store the button rectangle, or NULL to ignore.
 * @param slotAreaOut Pointer to store the slot area rectangle, or NULL to ignore.
 */
static void ComputeLayout(const LobbyMenuUIState *state,
    int width,
    int height,
    float scrollOffset,
    MenuUIRect *panelOut,
    MenuUIRect fieldRects[LOBBY_MENU_FIELD_COUNT],
    MenuUIRect *buttonOut,
    MenuUIRect *slotAreaOut) {

    // Ensure valid window dimensions.
    float w = (float)width;
    float h = (float)height;
    ClampWindowDimensions(&w, &h);

    // Calculate panel width within allowed bounds.

    // We want some padding on the sides, so start with that.
    float panelWidth = fminf(LOBBY_MENU_PANEL_MAX_WIDTH, w - 2.0f * LOBBY_MENU_PANEL_PADDING);

    // Enforce minimum width.
    if (panelWidth < LOBBY_MENU_PANEL_MIN_WIDTH) {
        panelWidth = LOBBY_MENU_PANEL_MIN_WIDTH;
    }

    if (panelWidth > w) {
        panelWidth = w;
    }

    // Now that we have panel width, calculate heights.
    
    // We want to calculate total height based on fields, button, slots, and padding.

    // The overall height is:
    //   top padding + fields height + button section spacing +
    //   button height + section spacing + slots height + bottom padding

    float fieldsHeight = (float)LOBBY_MENU_FIELD_COUNT * LOBBY_MENU_FIELD_HEIGHT;
    if (LOBBY_MENU_FIELD_COUNT > 1) {
        fieldsHeight += (float)(LOBBY_MENU_FIELD_COUNT - 1) * LOBBY_MENU_FIELD_SPACING;
    }

    float slotsHeight = (float)state->slotCount * LOBBY_MENU_SLOT_ROW_HEIGHT;
    if (state->slotCount > 1) {
        slotsHeight += (float)(state->slotCount - 1) * LOBBY_MENU_SLOT_ROW_SPACING;
    }

    float panelHeight = LOBBY_MENU_TOP_PADDING + fieldsHeight + LOBBY_MENU_BUTTON_SECTION_SPACING +
        LOBBY_MENU_BUTTON_HEIGHT + LOBBY_MENU_SECTION_SPACING + slotsHeight + LOBBY_MENU_BOTTOM_PADDING;


    // Compute content height for centering calculations.
    float contentHeight = panelHeight;
    if (state->statusMessage[0] != '\0') {
        contentHeight += MENU_GENERIC_TEXT_HEIGHT;
    }
    contentHeight += MENU_GENERIC_TEXT_HEIGHT;

    // Compute panel position to center it horizontally
    // and position it vertically based on scroll offset.
    float panelX = (w - panelWidth) * 0.5f;
    float baseY = LobbyMenuComputeBaseY(contentHeight, h);
    float panelY = baseY - scrollOffset;

    // If the pointer we've been given for
    // storing the panel rectangle is non-NULL,
    // we shall store the computed values.
    if (panelOut != NULL) {
        panelOut->x = panelX;
        panelOut->y = panelY;
        panelOut->width = panelWidth;
        panelOut->height = panelHeight;
    }

    // Compute inner rectangles for fields, button, and slot area.

    // This will be within the panel, accounting for padding.
    float innerX = panelX + LOBBY_MENU_PANEL_PADDING;
    float innerWidth = panelWidth - 2.0f * LOBBY_MENU_PANEL_PADDING;
    float y = panelY + LOBBY_MENU_TOP_PADDING;

    // Compute field rectangles if requested.
    if (fieldRects != NULL) {

        // These will be stacked vertically.
        // These are where the user will type in
        // data for the number of planets, factions, etc.
        for (size_t i = 0; i < LOBBY_MENU_FIELD_COUNT; ++i) {
            fieldRects[i].x = innerX;
            fieldRects[i].y = y;
            fieldRects[i].width = innerWidth;
            fieldRects[i].height = LOBBY_MENU_FIELD_HEIGHT;

            y += LOBBY_MENU_FIELD_HEIGHT;
            if (i + 1 < LOBBY_MENU_FIELD_COUNT) {
                y += LOBBY_MENU_FIELD_SPACING;
            }
        }
    } else {
        // If field rectangles are not requested, just advance y by the total fields height.
        y += fieldsHeight;
    }

    // Compute button rectangle if requested.
    y += LOBBY_MENU_BUTTON_SECTION_SPACING;


    // The button is centered within the inner width.
    // It shall be located below the fields but above the player slots.

    float buttonWidth = fminf(LOBBY_MENU_BUTTON_WIDTH, innerWidth);
    float buttonX = innerX + (innerWidth - buttonWidth) * 0.5f;
    if (buttonOut != NULL) {
        buttonOut->x = buttonX;
        buttonOut->y = y;
        buttonOut->width = buttonWidth;
        buttonOut->height = LOBBY_MENU_BUTTON_HEIGHT;
    }

    // Advance y past the button and section spacing.
    y += LOBBY_MENU_BUTTON_HEIGHT + LOBBY_MENU_SECTION_SPACING;


    // Compute slot area rectangle if requested.
    // This rectangle will contain the list of lobby slots
    // for players/factions.
    
    if (slotAreaOut != NULL) {
        slotAreaOut->x = innerX;
        slotAreaOut->y = y;
        slotAreaOut->width = innerWidth;
        slotAreaOut->height = slotsHeight;
    }
}

/**
 * Initializes the lobby menu UI state.
 * Sets default values and configures editability.
 * @param state Pointer to the LobbyMenuUIState to initialize.
 * @param editable True to make inputs interactive, false for read-only.
 */
void LobbyMenuUIInitialize(LobbyMenuUIState *state, bool editable) {
    // Validate parameter.
    if (state == NULL) {
        return;
    }

    // Clear the state to default values.
    memset(state, 0, sizeof(*state));
    state->editable = editable;
    state->focus = LOBBY_MENU_FOCUS_NONE;
    state->highlightedFactionId = -1;
}

/**
 * Sets whether the lobby menu inputs are editable.
 * Updates focus and button states accordingly.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param editable True to make inputs interactive, false for read-only.
 */
void LobbyMenuUISetEditable(LobbyMenuUIState *state, bool editable) {
    // Validate parameter.
    if (state == NULL) {
        return;
    }

    state->editable = editable;
    if (!editable) {
        // If making read-only, clear focus and button states.
        state->focus = LOBBY_MENU_FOCUS_NONE;
        state->startButtonPressed = false;
    }
}

/**
 * Sets the current lobby generation settings in the UI state.
 * Updates input field buffers to reflect the new settings.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param settings Pointer to the LobbyMenuGenerationSettings to apply.
 */
void LobbyMenuUISetSettings(LobbyMenuUIState *state, const LobbyMenuGenerationSettings *settings) {
    // Validate parameters.
    if (state == NULL || settings == NULL) {
        return;
    }

    state->settings = *settings;

    // When we set new settings, we need to update the input field buffers
    // since new values are now present.
    UpdateBuffersFromSettings(state);
}

/**
 * Retrieves and validates the current lobby generation settings from the UI state.
 * Parses input field buffers and updates the settings structure.
 * @param state Pointer to the LobbyMenuUIState to read from.
 * @param outSettings Pointer to store the parsed LobbyMenuGenerationSettings.
 * @return True if settings were successfully parsed and valid, false otherwise.
 */
bool LobbyMenuUIGetSettings(LobbyMenuUIState *state, LobbyMenuGenerationSettings *outSettings) {
    // Validate parameters.
    if (state == NULL || outSettings == NULL) {
        return false;
    }

    // Temporary settings structure to hold parsed values.
    LobbyMenuGenerationSettings parsed = state->settings;

    // For each input field, parse the text buffer
    for (size_t i = 0; i < LOBBY_MENU_FIELD_COUNT; ++i) {
        // Buffer to hold a copy of the field text for parsing.
        char buffer[sizeof(state->fieldText[i])];
        strncpy(buffer, state->fieldText[i], sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';

        // Clean off any leading/trailing whitespace.
        TrimWhitespace(buffer);

        // If the buffer is empty, parsing fails.
        if (buffer[0] == '\0') {
            return false;
        }

        // Parse the value based on the expected type.
        char *endPtr = NULL;

        // Currently, there are only int and float types.
        if (kFieldTypes[i] == LOBBY_MENU_VALUE_INT) {
            // strtol returns a long
            long value = strtol(buffer, &endPtr, 10);

            // If endPtr is NULL or didn't reach the end
            // (caused by invalid characters), parsing fails.
            if (endPtr == NULL || *endPtr != '\0') {
                return false;
            }

            // Assign the parsed value to the appropriate setting field.
            if (i == 0) {
                parsed.planetCount = (int)value;
            } else if (i == 1) {
                parsed.factionCount = (int)value;
            } else if (i == 6) {
                // If random seed is negative, fail parsing
                if (value < 0) {
                    return false;
                }
                parsed.randomSeed = (uint32_t)value;
            }
        } else {
            // We parse as a double since in the future we may desire higher precision,
            // though we only need float precision currently.
            float value = (float)strtod(buffer, &endPtr);

            // If endPtr is NULL or didn't reach the end
            // (caused by invalid characters), parsing fails.
            if (endPtr == NULL || *endPtr != '\0') {
                return false;
            }

            // Assign the parsed value to the appropriate setting field.
            if (i == 2) {
                parsed.minFleetCapacity = value;
            } else if (i == 3) {
                parsed.maxFleetCapacity = value;
            } else if (i == 4) {
                parsed.levelWidth = value;
            } else if (i == 5) {
                parsed.levelHeight = value;
            }
        }
    }

    // Now we're done with parsing. Validate the parsed settings.

    if (parsed.planetCount <= 0) {
        return false;
    }
    if (parsed.factionCount < 2 || parsed.factionCount > LOBBY_MENU_MAX_SLOTS) {
        return false;
    }
    if (parsed.factionCount > parsed.planetCount) {
        return false;
    }
    if (parsed.levelWidth <= 0.0f || parsed.levelHeight <= 0.0f) {
        return false;
    }
    if (parsed.minFleetCapacity <= 0.0f || parsed.maxFleetCapacity < parsed.minFleetCapacity) {
        return false;
    }

    // If we reach here, all parsing and validation succeeded.
    // Update the UI state and output settings.
    state->settings = parsed;
    *outSettings = parsed;
    return true;
}

/**
 * Sets the number of slots in the lobby UI.
 * Updates slot information arrays accordingly.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param slotCount The desired number of slots (max LOBBY_MENU_MAX_SLOTS).
 */
void LobbyMenuUISetSlotCount(LobbyMenuUIState *state, size_t slotCount) {
    // Validate parameter.
    if (state == NULL) {
        return;
    }

    // Clamp slot count to maximum allowed.
    if (slotCount > LOBBY_MENU_MAX_SLOTS) {
        slotCount = LOBBY_MENU_MAX_SLOTS;
    }

    // Initialize new slots to unoccupied with no faction.
    for (size_t i = slotCount; i < LOBBY_MENU_MAX_SLOTS; ++i) {
        state->slotFactionIds[i] = -1;
        state->slotOccupied[i] = false;
    }

    // Update the slot count.
    state->slotCount = slotCount;
}

/**
 * Sets information for a specific lobby slot.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param index Index of the slot to update (0 to LOBBY_MENU_MAX_SLOTS - 1).
 * @param factionId Faction ID assigned to the slot, or -1 if unassigned.
 * @param occupied True if the slot is occupied, false otherwise.
 */
void LobbyMenuUISetSlotInfo(LobbyMenuUIState *state, size_t index, int factionId, bool occupied) {
    // Validate parameters.
    if (state == NULL || index >= LOBBY_MENU_MAX_SLOTS) {
        return;
    }

    // If the index is beyond the current slot count, update the count.
    // This way, we should always have enough slots to hold the info.
    if (index >= state->slotCount) {
        state->slotCount = index + 1;
        if (state->slotCount > LOBBY_MENU_MAX_SLOTS) {
            state->slotCount = LOBBY_MENU_MAX_SLOTS;
        }
    }

    // Update the slot information.
    state->slotFactionIds[index] = factionId;
    state->slotOccupied[index] = occupied;
}

/**
 * Sets the highlighted faction ID in the lobby UI.
 * Used to visually distinguish a specific slot (e.g. local player).
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param factionId Faction ID to highlight, or -1 for none.
 */
void LobbyMenuUISetHighlightedFactionId(LobbyMenuUIState *state, int factionId) {
    // Validate parameter.
    if (state == NULL) {
        return;
    }

    state->highlightedFactionId = factionId;
}

/**
 * Clears all lobby slots in the UI state.
 * Resets slot count and marks all slots as unoccupied.
 * @param state Pointer to the LobbyMenuUIState to modify.
 */
void LobbyMenuUIClearSlots(LobbyMenuUIState *state) {
    // Validate parameter.
    if (state == NULL) {
        return;
    }

    // Reset slot count and clear slot info.
    state->slotCount = 0;
    for (size_t i = 0; i < LOBBY_MENU_MAX_SLOTS; ++i) {
        state->slotFactionIds[i] = -1;
        state->slotOccupied[i] = false;
    }
}

/**
 * Sets the status message displayed in the lobby UI.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param message Null-terminated string message to set, or NULL to clear.
 */
void LobbyMenuUISetStatusMessage(LobbyMenuUIState *state, const char *message) {
    // Validate parameter.
    if (state == NULL) {
        return;
    }

    // If message is NULL, clear the status message.
    if (message == NULL) {
        state->statusMessage[0] = '\0';
        return;
    }

    // Otherwise, copy the message into the status buffer, ensuring null-termination.
    strncpy(state->statusMessage, message, LOBBY_MENU_STATUS_MAX_LENGTH);
    state->statusMessage[LOBBY_MENU_STATUS_MAX_LENGTH] = '\0';
}

/**
 * Handles mouse movement events within the lobby UI.
 * Updates internal mouse position state.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param x Current mouse X position.
 * @param y Current mouse Y position.
 */
void LobbyMenuUIHandleMouseMove(LobbyMenuUIState *state, float x, float y) {
    // Validate parameter.
    if (state == NULL) {
        return;
    }

    state->mouseX = x;
    state->mouseY = y;
}

/**
 * Handles mouse button down events within the lobby UI.
 * Updates focus and button states based on mouse position.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param x Mouse X position at event.
 * @param y Mouse Y position at event.
 * @param width Current width of the UI area.
 * @param height Current height of the UI area.
 */
void LobbyMenuUIHandleMouseDown(LobbyMenuUIState *state, float x, float y, int width, int height) {
    // If the state is NULL, there's nothing to do.
    if (state == NULL) {
        return;
    }

    state->mouseX = x;
    state->mouseY = y;

    float scrollOffset = LobbyMenuClampScroll(state, (float)height);

    MenuUIRect panel;
    MenuUIRect fields[LOBBY_MENU_FIELD_COUNT];
    MenuUIRect button;

    // Figure out where everything is laid out.
    ComputeLayout(state, width, height, scrollOffset, &panel, fields, &button, NULL);

    // If the click is outside the panel, clear focus and button states.
    if (!MenuUIRectContains(&panel, x, y)) {
        state->focus = LOBBY_MENU_FOCUS_NONE;
        state->startButtonPressed = false;
        return;
    }

    // If editable (the lobby is being modified by some authoritative user),
    // check if any fields or the button were clicked.
    if (state->editable) {
        bool focusedField = false;
        for (size_t i = 0; i < LOBBY_MENU_FIELD_COUNT; ++i) {
            // If the click is within this field, set focus to it.
            if (MenuUIRectContains(&fields[i], x, y)) {
                state->focus = (LobbyMenuFocusTarget)i;
                focusedField = true;
                break;
            }
        }

        if (!focusedField) {
            state->focus = LOBBY_MENU_FOCUS_NONE;
        }

        // If the click is within the start button, mark it as pressed.
        if (MenuUIRectContains(&button, x, y)) {
            state->startButtonPressed = true;
        } else {
            state->startButtonPressed = false;
        }
    } else {
        // Otherwise, if not editable, clear focus and button states.
        state->focus = LOBBY_MENU_FOCUS_NONE;
        state->startButtonPressed = false;
    }
}

/**
 * Handles mouse button up events within the lobby UI.
 * Updates button states and triggers actions as needed.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param x Mouse X position at event.
 * @param y Mouse Y position at event.
 * @param width Current width of the UI area.
 * @param height Current height of the UI area.
 */
void LobbyMenuUIHandleMouseUp(LobbyMenuUIState *state, float x, float y, int width, int height) {
    // If the state is NULL, there's nothing to do.
    if (state == NULL) {
        return;
    }

    state->mouseX = x;
    state->mouseY = y;

    // If not editable, ignore any button presses.
    if (!state->editable) {
        state->startButtonPressed = false;
        return;
    }

    // The only thing that cares about mouse up is the start button,
    // so if it wasn't pressed, we can return early.
    if (!state->startButtonPressed) {
        return;
    }

    // We need to ensure that we've not scrolled above the top
    // or below the bottom of the content.
    float scrollOffset = LobbyMenuClampScroll(state, (float)height);
    MenuUIRect button;

    // Compute layout to find the button rectangle.
    ComputeLayout(state, width, height, scrollOffset, NULL, NULL, &button, NULL);

    // Then check if the mouse is still within the button.
    if (MenuUIRectContains(&button, x, y)) {
        state->startRequested = true;
    }

    // Clear the pressed state regardless.
    // If the mouse winds up outside the button, we don't want it to stay pressed.
    // Meanwhile, if it was inside, we've already marked startRequested.
    state->startButtonPressed = false;
}

/**
 * Adjusts the lobby menu scroll position in response to mouse wheel input.
 * Positive wheel steps scroll the content upward (toward earlier items).
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param height Current height of the UI area.
 * @param wheelSteps Wheel delta expressed in multiples of WHEEL_DELTA (120).
 */
void LobbyMenuUIHandleScroll(LobbyMenuUIState *state, int height, float wheelSteps) {
    // Validate parameters.
    if (state == NULL || height <= 0 || wheelSteps == 0.0f) {
        return;
    }

    // Adjust scroll offset based on wheel steps and predefined pixels per wheel step.
    // Positive wheel steps scroll up, so we subtract from the offset.
    state->scrollOffset -= wheelSteps * SCROLL_PIXELS_PER_WHEEL;

    // Ensure scroll offset is clamped within valid bounds.
    LobbyMenuClampScroll(state, (float)height);
}

/**
 * Handles character input events for text fields in the lobby UI.
 * Updates the focused input field with the entered character.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param codepoint Unicode codepoint of the entered character.
 */
void LobbyMenuUIHandleChar(LobbyMenuUIState *state, unsigned int codepoint) {
    // Validate parameters.
    if (state == NULL || !state->editable) {
        return;
    }

    // Determine which field is focused.
    int index = FocusToIndex(state->focus);
    if (index < 0 || index >= (int)LOBBY_MENU_FIELD_COUNT) {
        // No valid field is focused, nothing to do.
        return;
    }

    // Check if we have space to add another character.
    if (state->fieldLength[index] >= sizeof(state->fieldText[index]) - 1) {
        return;
    }

    // We now have both a valid field and space to add a character.
    // Validate the character based on the field type.
    char ch = (char)codepoint;
    if (kFieldTypes[index] == LOBBY_MENU_VALUE_INT) {
        // Integer fields only accept digits.
        if (!isdigit((unsigned char)ch)) {
            return;
        }
    } else {
        // Float fields accept digits and at most one dot.
        bool isDigit = isdigit((unsigned char)ch) != 0;
        bool isDot = ch == '.';
        if (!isDigit && !isDot) {
            return;
        }
        if (isDot && strchr(state->fieldText[index], '.') != NULL) {
            return;
        }
    }

    // Append the character to the field text, updating length and null-termination accordingly.
    state->fieldText[index][state->fieldLength[index]] = ch;
    state->fieldLength[index] += 1;
    state->fieldText[index][state->fieldLength[index]] = '\0';
}

/**
 * Handles key down events for keyboard input in the lobby UI.
 * Manages navigation, editing, and special key actions.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param key Virtual key code of the pressed key.
 * @param shiftDown True if the Shift key is currently held down.
 */
void LobbyMenuUIHandleKeyDown(LobbyMenuUIState *state, WPARAM key, bool shiftDown) {
    // If the state is NULL or not editable, there's nothing to do.
    if (state == NULL || !state->editable) {
        return;
    }

    // Determine which field is focused.
    int index = FocusToIndex(state->focus);
    switch (key) {
        // Backspace deletes the last character in the focused field.
        case VK_BACK:
            if (index >= 0 && state->fieldLength[index] > 0) {
                state->fieldLength[index] -= 1;
                state->fieldText[index][state->fieldLength[index]] = '\0';
            }
            break;

        // Tab navigates between fields.
        // Shift+Tab goes backwards.
        // It will wrap around from last to first and vice versa.
        case VK_TAB: {
            if (LOBBY_MENU_FIELD_COUNT == 0) {
                break;
            }
            int next = index;
            if (next < 0) {
                next = 0;
            } else if (shiftDown) {
                next = (next - 1 + (int)LOBBY_MENU_FIELD_COUNT) % (int)LOBBY_MENU_FIELD_COUNT;
            } else {
                next = (next + 1) % (int)LOBBY_MENU_FIELD_COUNT;
            }
            state->focus = (LobbyMenuFocusTarget)next;
            break;
        }

        // Enter requests to start the game.
        case VK_RETURN:
            state->startRequested = true;
            break;

        // Escape clears focus.
        case VK_ESCAPE:
            state->focus = LOBBY_MENU_FOCUS_NONE;
            break;
            
        // No other actions for other keys.
        default:
            break;
    }
}

/**
 * Consumes and clears any pending start game request from the lobby UI.
 * @param state Pointer to the LobbyMenuUIState to check.
 * @return True if a start request was pending, false otherwise.
 */
bool LobbyMenuUIConsumeStartRequest(LobbyMenuUIState *state) {
    // Validate parameter.
    if (state == NULL) {
        return false;
    }

    // If no start was requested, return false.
    if (!state->startRequested) {
        return false;
    }

    // Otherwise, clear the request and return true.
    state->startRequested = false;
    return true;
}

/**
 * Renders the lobby menu UI using OpenGL.
 * Draws panels, input fields, buttons, and status messages.
 * @param state Pointer to the LobbyMenuUIState to render.
 * @param context Pointer to the OpenGLContext for rendering.
 * @param width Current width of the UI area.
 * @param height Current height of the UI area.
 */
void LobbyMenuUIDraw(LobbyMenuUIState *state, OpenGLContext *context, int width, int height) {
    // Validate parameters.
    if (state == NULL || context == NULL || width <= 0 || height <= 0) {
        return;
    }

    // Ensure valid scroll offset.
    float scrollOffset = LobbyMenuClampScroll(state, (float)height);

    MenuUIRect panel;
    MenuUIRect fields[LOBBY_MENU_FIELD_COUNT];
    MenuUIRect button;
    MenuUIRect slotArea;

    // Figure out where everything is laid out.
    ComputeLayout(state, width, height, scrollOffset, &panel, fields, &button, &slotArea);

    // Draw the main panel background.
    float panelOutline[4] = MENU_PANEL_OUTLINE_COLOR;
    float panelFill[4] = MENU_PANEL_FILL_COLOR;
    DrawOutlinedRectangle(panel.x, panel.y, panel.x + panel.width, panel.y + panel.height, panelOutline, panelFill);

    // Draw each input field.
    const float labelColor[4] = MENU_LABEL_TEXT_COLOR;
    const float textColor[4] = MENU_INPUT_TEXT_COLOR;
    const float placeholderColor[4] = MENU_PLACEHOLDER_TEXT_COLOR;

    for (size_t i = 0; i < LOBBY_MENU_FIELD_COUNT; ++i) {
        const MenuUIRect *field = &fields[i];
        float outline[4] = MENU_INPUT_BOX_OUTLINE_COLOR;

        // Adjust outline alpha based on focus and editability.
        if (state->focus == (LobbyMenuFocusTarget)i) {
            outline[3] = MENU_INPUT_BOX_FOCUSED_ALPHA;
        } else if (!state->editable) {
            outline[3] *= 0.45f;
        }

        // Fill color also adjusts based on editability.
        float fill[4] = MENU_INPUT_BOX_FILL_COLOR;
        if (!state->editable) {
            fill[3] *= 0.6f;
        }

        DrawOutlinedRectangle(field->x, field->y, field->x + field->width, field->y + field->height, outline, fill);

        // Draw the label above the field.
        float labelY = field->y - 6.0f;
        DrawScreenText(context, kFieldLabels[i], field->x, labelY, MENU_LABEL_TEXT_HEIGHT, MENU_LABEL_TEXT_WIDTH, labelColor);

        // If the field has text, draw it; otherwise, draw the placeholder.
        if (state->fieldLength[i] > 0) {
            float textX = field->x + 6.0f;
            float textY = field->y + (field->height / 2.0f) + (MENU_INPUT_TEXT_HEIGHT / 2.0f);
            DrawScreenText(context, state->fieldText[i], textX, textY, MENU_INPUT_TEXT_HEIGHT, MENU_INPUT_TEXT_WIDTH, textColor);
        } else {
            float placeholderX = field->x + 6.0f;
            float placeholderY = field->y + (field->height / 2.0f) + (MENU_INPUT_TEXT_HEIGHT / 2.0f);
            DrawScreenText(context, kFieldPlaceholders[i], placeholderX, placeholderY, MENU_INPUT_TEXT_HEIGHT, MENU_INPUT_TEXT_WIDTH, placeholderColor);
        }
    }

    // Draw the start game button.

    // Check if the mouse is hovering over the button, and adjust colors accordingly.
    bool hoverButton = MenuUIRectContains(&button, state->mouseX, state->mouseY);
    float buttonOutline[4] = MENU_BUTTON_OUTLINE_COLOR;
    float buttonFillHover[4] = MENU_BUTTON_HOVER_FILL_COLOR;
    float buttonFillDefault[4] = MENU_BUTTON_FILL_COLOR;
    float *buttonFill = hoverButton ? buttonFillHover : buttonFillDefault;
    if (!state->editable) {
        buttonFill[3] *= 0.6f;
        buttonOutline[3] *= 0.6f;
    }

    DrawOutlinedRectangle(button.x, button.y, button.x + button.width, button.y + button.height, buttonOutline, buttonFill);

    // Draw the button text centered within the button horizontally and vertically.
    const char *buttonText = "Start Game";
    float buttonTextWidth = (float)strlen(buttonText) * MENU_BUTTON_TEXT_WIDTH;
    float buttonTextX = button.x + (button.width / 2.0f) - (buttonTextWidth / 2.0f);
    float buttonTextY = button.y + (button.height / 2.0f) + (MENU_BUTTON_TEXT_HEIGHT / 2.0f);
    DrawScreenText(context, buttonText, buttonTextX, buttonTextY, MENU_BUTTON_TEXT_HEIGHT, MENU_BUTTON_TEXT_WIDTH, textColor);

    // Draw the faction slots area.
    float slotsHeaderY = slotArea.y - 6.0f;

    // Draw the "Faction Slots" header.
    DrawScreenText(context, "Faction Slots", slotArea.x, slotsHeaderY, MENU_LABEL_TEXT_HEIGHT, MENU_LABEL_TEXT_WIDTH, labelColor);

    float rowY = slotArea.y;
    for (size_t i = 0; i < state->slotCount; ++i) {
        int factionId = state->slotFactionIds[i];
        bool occupied = state->slotOccupied[i];

        // Construct the slot line text.

        // If the faction ID is valid, include it in the text.
        // Otherwise, just show the slot number and status.
        char line[96];
        if (factionId >= 0) {
            snprintf(line, sizeof(line), "Slot %zu (Faction %d): %s", i + 1, factionId, occupied ? "Occupied" : "Empty");
        } else {
            snprintf(line, sizeof(line), "Slot %zu: %s", i + 1, occupied ? "Occupied" : "Empty");
        }

        // Determine slot text color based on whether it's highlighted.
        // Highlighted slots use a special color to stand out.
        // Slots are typically highlighted for the local player's faction.
        float slotColor[4] = MENU_LABEL_TEXT_COLOR;
        if (state->highlightedFactionId == factionId && factionId >= 0) {
            slotColor[0] = 1.0f;
            slotColor[1] = 0.95f;
            slotColor[2] = 0.6f;
        }

        // Draw the slot line text centered vertically within the row.
        float textY = rowY + (LOBBY_MENU_SLOT_ROW_HEIGHT / 2.0f) + (MENU_GENERIC_TEXT_HEIGHT / 2.0f);
        DrawScreenText(context, line, slotArea.x, textY, MENU_GENERIC_TEXT_HEIGHT, MENU_GENERIC_TEXT_WIDTH, slotColor);

        rowY += LOBBY_MENU_SLOT_ROW_HEIGHT + LOBBY_MENU_SLOT_ROW_SPACING;
    }

    // Finally, at the bottom just below the panel, draw the status message if any.
    if (state->statusMessage[0] != '\0') {
        float statusWidth = (float)strlen(state->statusMessage) * MENU_GENERIC_TEXT_WIDTH;
        float statusX = panel.x + (panel.width / 2.0f) - (statusWidth / 2.0f);
        float statusY = panel.y + panel.height + MENU_GENERIC_TEXT_HEIGHT;
        DrawScreenText(context, state->statusMessage, statusX, statusY, MENU_GENERIC_TEXT_HEIGHT, MENU_GENERIC_TEXT_WIDTH, labelColor);
    }
}
