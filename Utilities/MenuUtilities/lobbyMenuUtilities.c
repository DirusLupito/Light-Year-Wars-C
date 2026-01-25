/**
 * Implementation of lobby menu utilities shared by server and client.
 * Handles layout, rendering, and optional interaction for lobby settings.
 * @file Utilities/MenuUtilities/lobbyMenuUtilities.c
 * @author abmize
 */
#include "Utilities/MenuUtilities/lobbyMenuUtilities.h"

// Each field is described once here and reused for layout, validation, and rendering.
static const MenuInputFieldSpec kFieldSpecs[LOBBY_MENU_FIELD_COUNT] = {
    {"Planet Count", "Total number of planets", LOBBY_MENU_FIELD_HEIGHT, LOBBY_MENU_FIELD_SPACING, MENU_INPUT_FIELD_INT},
    {"Faction Count", "Playable faction slots (2 - 16)", LOBBY_MENU_FIELD_HEIGHT, LOBBY_MENU_FIELD_SPACING, MENU_INPUT_FIELD_INT},
    {"Min Fleet Capacity", "Minimum planet capacity", LOBBY_MENU_FIELD_HEIGHT, LOBBY_MENU_FIELD_SPACING, MENU_INPUT_FIELD_FLOAT},
    {"Max Fleet Capacity", "Maximum planet capacity", LOBBY_MENU_FIELD_HEIGHT, LOBBY_MENU_FIELD_SPACING, MENU_INPUT_FIELD_FLOAT},
    {"Level Width", "World width", LOBBY_MENU_FIELD_HEIGHT, LOBBY_MENU_FIELD_SPACING, MENU_INPUT_FIELD_FLOAT},
    {"Level Height", "World height", LOBBY_MENU_FIELD_HEIGHT, LOBBY_MENU_FIELD_SPACING, MENU_INPUT_FIELD_FLOAT},
    {"Random Seed", "Seed (0 for default)", LOBBY_MENU_FIELD_HEIGHT, LOBBY_MENU_FIELD_SPACING, MENU_INPUT_FIELD_INT}
};

// Labels for the preview toggle button.
static const char *kPreviewButtonLabelOpen = "Preview Level";
static const char *kPreviewButtonLabelClose = "Close Preview";

/**
 * Computes the total vertical height occupied by the stacked input fields.
 * @return Accumulated height including per-field spacing.
 */
static float LobbyMenuFieldsHeight(void) {
    float height = 0.0f;
    for (size_t i = 0; i < LOBBY_MENU_FIELD_COUNT; ++i) {
        height += kFieldSpecs[i].height;
        if (i + 1 < LOBBY_MENU_FIELD_COUNT) {
            height += kFieldSpecs[i].spacingBelow;
        }
    }
    return height;
}

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

    // Field height is driven by the spec table so any new field inherits sizing automatically.
    float fieldsHeight = LobbyMenuFieldsHeight();

    // Likewise, slot height is number of slots times row height plus spacing.
    float slotsHeight = (float)state->slotCount * LOBBY_MENU_SLOT_ROW_HEIGHT;
    if (state->slotCount > 1) {
        slotsHeight += (float)(state->slotCount - 1) * LOBBY_MENU_SLOT_ROW_SPACING;
    }

    // Add extra space if a color picker dropdown is open.
    if (state->colorPicker.open && state->colorPicker.slotIndex >= 0 && state->colorPicker.slotIndex < (int)state->slotCount) {
        slotsHeight += ColorPickerUIHeight();
    }

    float buttonHeight = LOBBY_MENU_BUTTON_HEIGHT;
    if (state != NULL && state->startButton.height > 0.0f) {
        buttonHeight = state->startButton.height;
    }

    // The preview button sits below the start button, so we add its height and spacing.
    float previewButtonHeight = LOBBY_MENU_PREVIEW_BUTTON_HEIGHT;
    if (state != NULL && state->previewButton.height > 0.0f) {
        previewButtonHeight = state->previewButton.height;
    }
    float buttonsHeight = buttonHeight + LOBBY_MENU_PREVIEW_BUTTON_SPACING + previewButtonHeight;

    // So the overall height is the sum of all sections plus padding.
    return LOBBY_MENU_TOP_PADDING + fieldsHeight + LOBBY_MENU_BUTTON_SECTION_SPACING +
        buttonsHeight + LOBBY_MENU_SECTION_SPACING + slotsHeight + LOBBY_MENU_BOTTOM_PADDING;
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

    float contentHeight = LobbyMenuComputeContentHeight(state);
    float clamped = MenuLayoutClampScroll(contentHeight, viewportHeight, state->scrollOffset);
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
 * Synchronizes the focused field between the enum and the component array.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param focus Desired focus target.
 */
static void LobbyMenuSetFocus(LobbyMenuUIState *state, LobbyMenuFocusTarget focus) {
    if (state == NULL) {
        return;
    }

    state->focus = focus;
    for (size_t i = 0; i < LOBBY_MENU_FIELD_COUNT; ++i) {
        MenuInputFieldSetFocus(&state->inputFields[i], focus == (LobbyMenuFocusTarget)i);
    }
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
 * Initializes all UI components so the lobby screen can be composed from primitives.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param editable True when inputs should start in an editable state.
 */
static void LobbyMenuInitComponents(LobbyMenuUIState *state, bool editable) {
    if (state == NULL) {
        return;
    }

    for (size_t i = 0; i < LOBBY_MENU_FIELD_COUNT; ++i) {
        MenuInputFieldInitialize(&state->inputFields[i], &kFieldSpecs[i],
            state->fieldText[i], sizeof(state->fieldText[i]), &state->fieldLength[i]);
        state->inputFields[i].editable = editable;
    }

    MenuButtonInitialize(&state->startButton, "Start Game", LOBBY_MENU_BUTTON_WIDTH, LOBBY_MENU_BUTTON_HEIGHT);
    state->startButton.enabled = editable;

    // Preview button is always enabled so users can toggle the preview in read-only lobbies.
    MenuButtonInitialize(&state->previewButton, kPreviewButtonLabelOpen,
        LOBBY_MENU_PREVIEW_BUTTON_WIDTH, LOBBY_MENU_PREVIEW_BUTTON_HEIGHT);
    state->previewButton.enabled = true;
}

/**
 * Updates the preview button label based on the current open state.
 * @param state Pointer to the LobbyMenuUIState to modify.
 */
static void LobbyMenuUpdatePreviewButtonLabel(LobbyMenuUIState *state) {
    if (state == NULL) {
        return;
    }

    // We swap the label so users understand whether the preview will open or close.
    state->previewButton.label = state->previewOpen ? kPreviewButtonLabelClose : kPreviewButtonLabelOpen;
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
static void ComputeLayout(LobbyMenuUIState *state,
    int width,
    int height,
    float scrollOffset,
    MenuUIRect *panelOut,
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

    float fieldsHeight = LobbyMenuFieldsHeight();

    float slotsHeight = (float)state->slotCount * LOBBY_MENU_SLOT_ROW_HEIGHT;
    if (state->slotCount > 1) {
        slotsHeight += (float)(state->slotCount - 1) * LOBBY_MENU_SLOT_ROW_SPACING;
    }

    if (state->colorPicker.open && state->colorPicker.slotIndex >= 0 && state->colorPicker.slotIndex < (int)state->slotCount) {
        slotsHeight += ColorPickerUIHeight();
    }

    float panelHeight = LOBBY_MENU_TOP_PADDING + fieldsHeight + LOBBY_MENU_BUTTON_SECTION_SPACING +
        state->startButton.height + LOBBY_MENU_PREVIEW_BUTTON_SPACING + state->previewButton.height +
        LOBBY_MENU_SECTION_SPACING + slotsHeight + LOBBY_MENU_BOTTOM_PADDING;


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


    // For each input field, compute its layout.
    for (size_t i = 0; i < LOBBY_MENU_FIELD_COUNT; ++i) {
        MenuInputFieldLayout(&state->inputFields[i], innerX, y, innerWidth);
        y += kFieldSpecs[i].height;
        if (i + 1 < LOBBY_MENU_FIELD_COUNT) {
            y += kFieldSpecs[i].spacingBelow;
        }
    }

    // Compute button rectangle if requested.
    y += LOBBY_MENU_BUTTON_SECTION_SPACING;
    MenuButtonLayout(&state->startButton, innerX, y, innerWidth);
    y += state->startButton.height + LOBBY_MENU_PREVIEW_BUTTON_SPACING;
    MenuButtonLayout(&state->previewButton, innerX, y, innerWidth);
    y += state->previewButton.height + LOBBY_MENU_SECTION_SPACING;
    
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
    LobbyMenuInitComponents(state, editable);
    LobbyMenuSetFocus(state, LOBBY_MENU_FOCUS_NONE);
    state->highlightedFactionId = -1;
    state->previewOpen = false;
    state->previewButtonPressed = false;
    LobbyMenuUpdatePreviewButtonLabel(state);

    // Reset picker state so there is no stale interaction or pending commits.
    ColorPickerUIInitialize(&state->colorPicker);

    for (size_t i = 0; i < LOBBY_MENU_MAX_SLOTS; ++i) {
        state->slotColors[i][0] = 1.0f;
        state->slotColors[i][1] = 1.0f;
        state->slotColors[i][2] = 1.0f;
        state->slotColors[i][3] = 1.0f;
        state->slotColorValid[i] = false;
    }
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
    for (size_t i = 0; i < LOBBY_MENU_FIELD_COUNT; ++i) {
        state->inputFields[i].editable = editable;
    }
    state->startButton.enabled = editable;
    if (!editable) {
        // If making read-only, clear focus and button states.
        LobbyMenuSetFocus(state, LOBBY_MENU_FOCUS_NONE);
        state->startButtonPressed = false;
        state->startButton.pressed = false;
    }
}

/**
 * Returns whether the lobby preview panel is currently open.
 * @param state Pointer to the LobbyMenuUIState to read.
 * @return True if the preview is open, false otherwise.
 */
bool LobbyMenuUIIsPreviewOpen(const LobbyMenuUIState *state) {
    if (state == NULL) {
        return false;
    }

    return state->previewOpen;
}

/**
 * Sets whether the lobby preview panel is open.
 * Updates the preview button label accordingly.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param open True to open the preview, false to close it.
 */
void LobbyMenuUISetPreviewOpen(LobbyMenuUIState *state, bool open) {
    if (state == NULL) {
        return;
    }

    state->previewOpen = open;
    LobbyMenuUpdatePreviewButtonLabel(state);
}

/**
 * Retrieves the computed main panel rectangle for the lobby menu.
 * Useful for aligning external UI elements like the preview panel.
 * @param state Pointer to the LobbyMenuUIState to read.
 * @param width Current width of the UI area.
 * @param height Current height of the UI area.
 * @param panelOut Output rectangle for the lobby panel.
 * @return True if the panel rectangle was computed, false otherwise.
 */
bool LobbyMenuUIGetPanelRect(LobbyMenuUIState *state, int width, int height, MenuUIRect *panelOut) {
    if (state == NULL || panelOut == NULL || width <= 0 || height <= 0) {
        return false;
    }

    // Clamp scroll so the panel is computed in a stable location.
    float scrollOffset = LobbyMenuClampScroll(state, (float)height);
    ComputeLayout(state, width, height, scrollOffset, panelOut, NULL);
    return true;
}

/**
 * Retrieves the computed preview panel rectangle aligned to the lobby panel.
 * @param state Pointer to the LobbyMenuUIState to read.
 * @param width Current width of the UI area.
 * @param height Current height of the UI area.
 * @param previewOut Output rectangle for the preview panel.
 * @return True if the preview rectangle was computed, false otherwise.
 */
bool LobbyMenuUIGetPreviewPanelRect(LobbyMenuUIState *state, int width, int height, MenuUIRect *previewOut) {
    if (state == NULL || previewOut == NULL || width <= 0 || height <= 0) {
        return false;
    }

    MenuUIRect panel;
    if (!LobbyMenuUIGetPanelRect(state, width, height, &panel)) {
        return false;
    }

    // Compute available horizontal space to the right of the main panel.
    // We measure this so the preview never overlaps the lobby panel.
    float available = (float)width - (panel.x + panel.width) - LOBBY_MENU_PREVIEW_PANEL_MARGIN;
    if (available <= 1.0f) {
        return false;
    }

    // We want a square preview viewport, which means the panel width needs to
    // match the panel height minus the header (padding cancels out).
    float targetWidth = panel.height - LOBBY_MENU_PREVIEW_PANEL_HEADER_HEIGHT;

    // Clamp the preview width to reasonable bounds while honoring available space.
    // We prioritize the square target so the preview reads as a consistent window.
    float previewWidth = targetWidth;
    if (previewWidth > available) {
        previewWidth = available;
    }
    if (previewWidth > LOBBY_MENU_PREVIEW_PANEL_MAX_WIDTH) {
        previewWidth = LOBBY_MENU_PREVIEW_PANEL_MAX_WIDTH;
    }
    if (previewWidth < LOBBY_MENU_PREVIEW_PANEL_MIN_WIDTH) {
        previewWidth = fminf(available, LOBBY_MENU_PREVIEW_PANEL_MIN_WIDTH);
    }

    // If the preview becomes too narrow, skip rendering to avoid layout overlap.
    if (previewWidth < 80.0f) {
        return false;
    }

    previewOut->x = panel.x + panel.width + LOBBY_MENU_PREVIEW_PANEL_MARGIN;
    previewOut->y = panel.y;
    previewOut->width = previewWidth;
    previewOut->height = panel.height;
    return true;
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

        MenuInputFieldKind kind = kFieldSpecs[i].kind;

        if (kind == MENU_INPUT_FIELD_INT) {
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
        state->slotColorValid[i] = false;
        state->slotNames[i][0] = '\0';
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
void LobbyMenuUISetSlotInfo(LobbyMenuUIState *state, size_t index, int factionId, bool occupied, const char *playerName) {
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
    if (occupied && playerName != NULL) {
        strncpy(state->slotNames[index], playerName, PLAYER_NAME_MAX_LENGTH);
        state->slotNames[index][PLAYER_NAME_MAX_LENGTH] = '\0';
    } else {
        state->slotNames[index][0] = '\0';
    }
}

/**
 * Sets the display color for a specific lobby slot.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param index Index of the slot to update (0 to LOBBY_MENU_MAX_SLOTS - 1).
 * @param color RGBA color in 0-1 range.
 */
void LobbyMenuUISetSlotColor(LobbyMenuUIState *state, size_t index, const float color[4]) {
    if (state == NULL || color == NULL || index >= LOBBY_MENU_MAX_SLOTS) {
        return;
    }

    // Update the slot color, clamping each component to [0, 1].
    state->slotColors[index][0] = ColorPickerClamp01(color[0]);
    state->slotColors[index][1] = ColorPickerClamp01(color[1]);
    state->slotColors[index][2] = ColorPickerClamp01(color[2]);
    state->slotColors[index][3] = ColorPickerClamp01(color[3]);
    state->slotColorValid[index] = true;
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
 * Configures which faction colors the local user is allowed to edit.
 * @param state Pointer to the LobbyMenuUIState to modify.
 * @param allowAll True to allow editing any slot color (server).
 * @param factionId Faction ID allowed to edit when allowAll is false (client).
 */
void LobbyMenuUISetColorEditAuthority(LobbyMenuUIState *state, bool allowAll, int factionId) {
    if (state == NULL) {
        return;
    }

    ColorPickerUISetEditAuthority(&state->colorPicker, allowAll, factionId);
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
        state->slotColorValid[i] = false;
        state->slotNames[i][0] = '\0';
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

    // If the color picker is open and dragging, update the color.
    // Otherwise, nothing more to do beyond updating mouse position.
    if (!state->colorPicker.open || !state->colorPicker.dragging || state->colorPicker.slotIndex < 0) {
        return;
    }

    int slotIndex = state->colorPicker.slotIndex;
    if (slotIndex >= 0 && slotIndex < (int)state->slotCount) {
        bool dirty = false;
        ColorPickerUIUpdateDrag(&state->colorPicker, x, state->slotColors[slotIndex], &dirty);
        if (dirty) {
            state->slotColorValid[slotIndex] = true;
            state->colorPicker.dirty = true;
        }
    }
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
    MenuUIRect slotArea;

    // Figure out where everything is laid out.
    ComputeLayout(state, width, height, scrollOffset, &panel, &slotArea);

    // Handle color picker interactions first (independent of editability).
    bool clickedColorUI = false;
    bool colorActionHandled = false;

    if (state->slotCount > 0) {
        float rowY = slotArea.y;

        // For each slot, check if the color swatch or picker was clicked.
        for (size_t i = 0; i < state->slotCount; ++i) {
            int factionId = state->slotFactionIds[i];

            // Figure out the swatch rectangle for this slot.
            float swatchX = slotArea.x + slotArea.width - COLOR_PICKER_SWATCH_PADDING - COLOR_PICKER_SWATCH_SIZE;
            float swatchY = rowY + (LOBBY_MENU_SLOT_ROW_HEIGHT - COLOR_PICKER_SWATCH_SIZE) * 0.5f;
            MenuUIRect swatchRect = MenuUIRectMake(swatchX, swatchY, COLOR_PICKER_SWATCH_SIZE, COLOR_PICKER_SWATCH_SIZE);

            // If the swatch was clicked, handle opening/closing the color picker.
            if (MenuUIRectContains(&swatchRect, x, y)) {
                bool canEdit = ColorPickerUICanEdit(&state->colorPicker, factionId);

                // If editable, toggle the color picker for this slot.
                if (canEdit) {
                    clickedColorUI = true;
                    // If the picker is already open for this slot, close it.
                    if (state->colorPicker.open && state->colorPicker.slotIndex == (int)i) {
                        ColorPickerUIClose(&state->colorPicker, true, state->slotColors[i], factionId);
                    } else {
                        // If another slot's picker is open, close it first.
                        if (state->colorPicker.open && state->colorPicker.slotIndex >= 0 && state->colorPicker.slotIndex < (int)state->slotCount) {
                            int openFactionId = state->slotFactionIds[state->colorPicker.slotIndex];
                            ColorPickerUIClose(&state->colorPicker, true, state->slotColors[state->colorPicker.slotIndex], openFactionId);
                        }
                        ColorPickerUIOpen(&state->colorPicker, (int)i);
                    }
                    colorActionHandled = true;
                }

                // If not editable, just note that the color UI was clicked.
                if (!canEdit && state->colorPicker.open && state->colorPicker.slotIndex == (int)i) {
                    clickedColorUI = true;
                }
                break;
            }

            // If the color picker is open for this slot, check if a slider was clicked.
            if (state->colorPicker.open && state->colorPicker.slotIndex == (int)i) {
                float pickerY = rowY + LOBBY_MENU_SLOT_ROW_HEIGHT + LOBBY_MENU_SLOT_ROW_SPACING;
                MenuUIRect pickerRect = MenuUIRectMake(slotArea.x, pickerY, slotArea.width, ColorPickerUIHeight());

                // Check if the click is within the picker area.
                if (MenuUIRectContains(&pickerRect, x, y)) {
                    clickedColorUI = true;

                    float sliderX = pickerRect.x + COLOR_PICKER_PANEL_PADDING + COLOR_PICKER_SLIDER_LABEL_WIDTH;
                    float sliderWidth = pickerRect.width - (COLOR_PICKER_PANEL_PADDING * 2.0f) - COLOR_PICKER_SLIDER_LABEL_WIDTH;

                    // For each color channel, check if its slider was clicked.
                    for (int channel = 0; channel < 3; ++channel) {
                        float sliderY = pickerRect.y + COLOR_PICKER_PANEL_PADDING + channel * (COLOR_PICKER_SLIDER_HEIGHT + COLOR_PICKER_SLIDER_SPACING);
                        MenuUIRect sliderRect = MenuUIRectMake(sliderX, sliderY, sliderWidth, COLOR_PICKER_SLIDER_HEIGHT);

                        // If this slider was clicked, start dragging.
                        if (MenuUIRectContains(&sliderRect, x, y)) {
                            float t = (x - sliderX) / sliderWidth;
                            t = ColorPickerClamp01(t);
                            state->slotColors[i][channel] = t;
                            state->slotColors[i][3] = 1.0f;
                            state->slotColorValid[i] = true;
                            state->colorPicker.dirty = true;
                            ColorPickerUIBeginDrag(&state->colorPicker, channel, sliderX, sliderWidth);
                            colorActionHandled = true;
                            break;
                        }
                    }

                    break;
                }
            }

            // Advance to the next slot row.
            rowY += LOBBY_MENU_SLOT_ROW_HEIGHT + LOBBY_MENU_SLOT_ROW_SPACING;
            if (state->colorPicker.open && state->colorPicker.slotIndex == (int)i) {
                rowY += ColorPickerUIHeight();
            }
        }
    }

    // If the color picker is open but no color UI was clicked, close it.
    if (state->colorPicker.open && !clickedColorUI) {
        int openSlot = state->colorPicker.slotIndex;

        // Close the picker, committing the color if valid.
        if (openSlot >= 0 && openSlot < (int)state->slotCount) {
            int openFactionId = state->slotFactionIds[openSlot];
            ColorPickerUIClose(&state->colorPicker, true, state->slotColors[openSlot], openFactionId);
        } else {
            // Out of range slot, just close without committing.
            ColorPickerUIClose(&state->colorPicker, false, NULL, -1);
        }
    }

    // If a color action was handled or the color UI was clicked, we're done.
    if (colorActionHandled || clickedColorUI) {
        return;
    }

    // If the click is outside the panel, clear focus and button states.
    if (!MenuUIRectContains(&panel, x, y)) {
        LobbyMenuSetFocus(state, LOBBY_MENU_FOCUS_NONE);
        state->startButtonPressed = false;
        state->startButton.pressed = false;
        return;
    }

    // Handle preview button regardless of editability so read-only clients can toggle it.
    state->previewButtonPressed = MenuButtonHandleMouseDown(&state->previewButton, x, y);
    if (state->previewButtonPressed) {
        // Clear focus so the preview button press doesn't keep fields highlighted.
        LobbyMenuSetFocus(state, LOBBY_MENU_FOCUS_NONE);
        return;
    }

    // If editable (the lobby is being modified by some authoritative user),
    // check if any fields or the start button were clicked.
    if (state->editable) {
        bool focusedField = false;
        for (size_t i = 0; i < LOBBY_MENU_FIELD_COUNT; ++i) {
            // If the click is within this field, set focus to it.
            if (MenuUIRectContains(&state->inputFields[i].rect, x, y)) {
                LobbyMenuSetFocus(state, (LobbyMenuFocusTarget)i);
                focusedField = true;
                break;
            }
        }

        if (!focusedField) {
            LobbyMenuSetFocus(state, LOBBY_MENU_FOCUS_NONE);
        }

        // If the click is within the start button, mark it as pressed.
        state->startButtonPressed = MenuButtonHandleMouseDown(&state->startButton, x, y);
    } else {
        // Otherwise, if not editable, clear focus and button states.
        LobbyMenuSetFocus(state, LOBBY_MENU_FOCUS_NONE);
        state->startButtonPressed = false;
        state->startButton.pressed = false;
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

    // Stop any color slider dragging.
    ColorPickerUIEndDrag(&state->colorPicker);

    // Handle preview button release regardless of editability.
    if (state->previewButtonPressed) {
        bool previewActivated = false;
        MenuButtonHandleMouseUp(&state->previewButton, x, y, &previewActivated);
        if (previewActivated) {
            state->previewOpen = !state->previewOpen;
            LobbyMenuUpdatePreviewButtonLabel(state);
        }
        state->previewButtonPressed = false;
        state->previewButton.pressed = false;
    }

    // If not editable, ignore any start button presses.
    if (!state->editable) {
        state->startButtonPressed = false;
        state->startButton.pressed = false;
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
    ComputeLayout(state, width, height, scrollOffset, NULL, NULL);

    bool activated = false;
    MenuButtonHandleMouseUp(&state->startButton, x, y, &activated);
    if (activated) {
        state->startRequested = true;
    }

    // Clear the pressed state regardless of activation to avoid sticky input.
    state->startButtonPressed = false;
    state->startButton.pressed = false;

    // We also need to ensure scroll offset is clamped
    // here because mouse up may have caused changes
    // to the UI that affect layout, such as closing the color picker.
    LobbyMenuClampScroll(state, (float)height);
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

    MenuInputFieldHandleChar(&state->inputFields[index], codepoint);
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
            if (index >= 0) {
                MenuInputFieldHandleBackspace(&state->inputFields[index]);
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
            LobbyMenuSetFocus(state, (LobbyMenuFocusTarget)next);
            break;
        }

        // Enter requests to start the game.
        case VK_RETURN:
            state->startRequested = true;
            break;

        // Escape clears focus.
        case VK_ESCAPE:
            LobbyMenuSetFocus(state, LOBBY_MENU_FOCUS_NONE);
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
 * Consumes a committed color change from the lobby UI.
 * @param state Pointer to the LobbyMenuUIState.
 * @param outFactionId Output faction ID whose color changed.
 * @param outR Output red channel (0-255).
 * @param outG Output green channel (0-255).
 * @param outB Output blue channel (0-255).
 * @return True if a color change was pending, false otherwise.
 */
bool LobbyMenuUIConsumeColorCommit(LobbyMenuUIState *state, int *outFactionId, uint8_t *outR, uint8_t *outG, uint8_t *outB) {
    return ColorPickerUIConsumeCommit(&state->colorPicker, outFactionId, outR, outG, outB);
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
    MenuUIRect slotArea;

    // Figure out where everything is laid out and hydrate component rectangles.
    ComputeLayout(state, width, height, scrollOffset, &panel, &slotArea);

    // Draw the main panel background.
    float panelOutline[4] = MENU_PANEL_OUTLINE_COLOR;
    float panelFill[4] = MENU_PANEL_FILL_COLOR;
    DrawOutlinedRectangle(panel.x, panel.y, panel.x + panel.width, panel.y + panel.height, panelOutline, panelFill);

    // Draw each input field.
    const float labelColor[4] = MENU_LABEL_TEXT_COLOR;
    const float textColor[4] = MENU_INPUT_TEXT_COLOR;
    const float placeholderColor[4] = MENU_PLACEHOLDER_TEXT_COLOR;

    for (size_t i = 0; i < LOBBY_MENU_FIELD_COUNT; ++i) {
        MenuInputFieldDraw(&state->inputFields[i], context, labelColor, textColor, placeholderColor);
    }

    // Draw the start game button.
    MenuButtonDraw(&state->startButton, context, state->mouseX, state->mouseY, state->startButton.enabled);

    // Draw the preview toggle button.
    MenuButtonDraw(&state->previewButton, context, state->mouseX, state->mouseY, state->previewButton.enabled);

    // Draw the faction slots area.
    float slotsHeaderY = slotArea.y - 6.0f;

    // Draw the "Faction Slots" header.
    DrawScreenText(context, "Faction Slots", slotArea.x, slotsHeaderY, MENU_LABEL_TEXT_HEIGHT, MENU_LABEL_TEXT_WIDTH, labelColor);

    float rowY = slotArea.y;
    for (size_t i = 0; i < state->slotCount; ++i) {
        int factionId = state->slotFactionIds[i];

        // Determine the display name for the slot
        // which depends on whether it's occupied and has a custom name.
        // Occupied slots with an empty name show "Occupied" to avoid confusion.
        bool occupied = state->slotOccupied[i];
        const char *displayName = NULL;
        if (occupied && state->slotNames[i][0] != '\0') {
            displayName = state->slotNames[i];
        } else if (occupied) {
            displayName = "Occupied";
        } else {
            displayName = "Empty";
        }

        // Construct the slot line text.

        // If the faction ID is valid, include it in the text.
        // Otherwise, just show the slot number and status.
        char line[96];
        if (factionId >= 0) {
            snprintf(line, sizeof(line), "Slot %zu (Faction %d): %s", i + 1, factionId, displayName);
        } else {
            snprintf(line, sizeof(line), "Slot %zu: %s", i + 1, displayName);
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

        // Draw the color swatch to the right side of the slot row.
        float swatchX = slotArea.x + slotArea.width - COLOR_PICKER_SWATCH_PADDING - COLOR_PICKER_SWATCH_SIZE;
        float swatchY = rowY + (LOBBY_MENU_SLOT_ROW_HEIGHT - COLOR_PICKER_SWATCH_SIZE) * 0.5f;
        float swatchOutline[4] = MENU_INPUT_BOX_OUTLINE_COLOR;
        float swatchFill[4] = {0.2f, 0.2f, 0.2f, 0.9f};
        if (state->slotColorValid[i]) {
            swatchFill[0] = state->slotColors[i][0];
            swatchFill[1] = state->slotColors[i][1];
            swatchFill[2] = state->slotColors[i][2];
            swatchFill[3] = 0.95f;
        }
        DrawOutlinedRectangle(swatchX, swatchY, swatchX + COLOR_PICKER_SWATCH_SIZE,
            swatchY + COLOR_PICKER_SWATCH_SIZE, swatchOutline, swatchFill);

        // Draw the color picker dropdown if this slot is active.
        if (state->colorPicker.open && state->colorPicker.slotIndex == (int)i) {
            float pickerY = rowY + LOBBY_MENU_SLOT_ROW_HEIGHT + LOBBY_MENU_SLOT_ROW_SPACING;
            MenuUIRect pickerRect = MenuUIRectMake(slotArea.x, pickerY, slotArea.width, ColorPickerUIHeight());

            float pickerOutline[4] = MENU_PANEL_OUTLINE_COLOR;
            float pickerFill[4] = MENU_PANEL_FILL_COLOR;
            DrawOutlinedRectangle(pickerRect.x, pickerRect.y, pickerRect.x + pickerRect.width,
                pickerRect.y + pickerRect.height, pickerOutline, pickerFill);

            const char *labels[3] = {"R", "G", "B"};
            float sliderX = pickerRect.x + COLOR_PICKER_PANEL_PADDING + COLOR_PICKER_SLIDER_LABEL_WIDTH;
            float sliderWidth = pickerRect.width - (COLOR_PICKER_PANEL_PADDING * 2.0f) - COLOR_PICKER_SLIDER_LABEL_WIDTH;

            // For each color channel, draw its slider.
            for (int channel = 0; channel < 3; ++channel) {
                float sliderY = pickerRect.y + COLOR_PICKER_PANEL_PADDING + channel * (COLOR_PICKER_SLIDER_HEIGHT + COLOR_PICKER_SLIDER_SPACING);
                MenuUIRect sliderRect = MenuUIRectMake(sliderX, sliderY, sliderWidth, COLOR_PICKER_SLIDER_HEIGHT);

                float sliderOutline[4] = MENU_INPUT_BOX_OUTLINE_COLOR;
                float sliderFill[4] = MENU_INPUT_BOX_FILL_COLOR;
                DrawOutlinedRectangle(sliderRect.x, sliderRect.y, sliderRect.x + sliderRect.width,
                    sliderRect.y + sliderRect.height, sliderOutline, sliderFill);

                // The default fill color depends on the channel.
                float fillColor[4] = {0.4f, 0.4f, 0.4f, 0.9f};

                // Channel 0, which is red has a reddish fill.
                // Channel 1 (green) is greenish, and channel 2 (blue) is blueish.
                if (channel == 0) {
                    fillColor[0] = 0.95f; fillColor[1] = 0.2f; fillColor[2] = 0.2f;
                } else if (channel == 1) {
                    fillColor[0] = 0.2f; fillColor[1] = 0.95f; fillColor[2] = 0.2f;
                } else {
                    fillColor[0] = 0.2f; fillColor[1] = 0.4f; fillColor[2] = 0.95f;
                }

                // Draw the filled portion of the slider based on the current color value.
                float value = ColorPickerClamp01(state->slotColors[i][channel]);
                float filledWidth = sliderRect.width * value;
                if (filledWidth > 1.0f) {
                    float fillOutline[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                    DrawOutlinedRectangle(sliderRect.x, sliderRect.y,
                        sliderRect.x + filledWidth, sliderRect.y + sliderRect.height,
                        fillOutline, fillColor);
                }

                // Figure out and then draw the channel label to the left of the slider.

                float labelX = pickerRect.x + COLOR_PICKER_PANEL_PADDING;
                float labelY = sliderRect.y + sliderRect.height + (MENU_GENERIC_TEXT_HEIGHT * 0.2f);
                DrawScreenText(context, labels[channel], labelX, labelY,
                    MENU_GENERIC_TEXT_HEIGHT, MENU_GENERIC_TEXT_WIDTH, labelColor);
            }

            rowY += ColorPickerUIHeight();
        }

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
