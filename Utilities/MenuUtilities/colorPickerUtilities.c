/**
 * Implementation of color picker UI utilities.
 * Handles RGB picker state, permissions, and commit helpers.
 * @file Utilities/MenuUtilities/colorPickerUtilities.c
 * @author abmize
 */

#include "Utilities/MenuUtilities/colorPickerUtilities.h"

/**
 * Internal helper to reset slider drag cache.
 * We reset these values to avoid using stale geometry from previous frames.
 * @param state Pointer to the ColorPickerUIState to modify.
 */
static void ColorPickerResetDragCache(ColorPickerUIState *state) {
    if (state == NULL) {
        return;
    }

    state->dragging = false;
    state->channel = 0;
    state->sliderX = 0.0f;
    state->sliderWidth = 1.0f;
}

/**
 * Initializes a color picker UI state to defaults.
 * @param state Pointer to the ColorPickerUIState to initialize.
 */
void ColorPickerUIInitialize(ColorPickerUIState *state) {
    if (state == NULL) {
        return;
    }

    // Default to a closed, clean picker so nothing commits without intent.
    state->open = false;
    state->slotIndex = -1;
    state->dirty = false;
    ColorPickerResetDragCache(state);

    // Default to no pending commit so we only propagate deliberate changes.
    state->commitPending = false;
    state->commitFactionId = -1;
    state->commitR = 0;
    state->commitG = 0;
    state->commitB = 0;

    // Default to no edit permissions so the caller must opt in explicitly.
    state->allowAll = false;
    state->allowedFactionId = -1;
}

/**
 * Updates which faction IDs are allowed to edit colors.
 * @param state Pointer to the ColorPickerUIState to modify.
 * @param allowAll True to allow editing any slot color.
 * @param factionId Faction ID allowed to edit when allowAll is false.
 */
void ColorPickerUISetEditAuthority(ColorPickerUIState *state, bool allowAll, int factionId) {
    if (state == NULL) {
        return;
    }

    // When allowAll is true we ignore factionId to simplify permission checks.
    state->allowAll = allowAll;
    state->allowedFactionId = allowAll ? -1 : factionId;
}

/**
 * Returns whether the given faction ID is editable under current permissions.
 * @param state Pointer to the ColorPickerUIState to check.
 * @param factionId Faction ID to test.
 * @return True if editing is permitted, false otherwise.
 */
bool ColorPickerUICanEdit(const ColorPickerUIState *state, int factionId) {
    if (state == NULL || factionId < 0) {
        return false;
    }

    // allowAll supports server-side editing without per-slot checks.
    if (state->allowAll) {
        return true;
    }

    return state->allowedFactionId == factionId;
}

/**
 * Computes the height of the dropdown panel for the RGB sliders.
 * @return The total height in pixels.
 */
float ColorPickerUIHeight(void) {
    // Height is derived from slider stacking so UI layout stays consistent.
    return COLOR_PICKER_PANEL_PADDING * 2.0f
        + (COLOR_PICKER_SLIDER_HEIGHT * 3.0f)
        + (COLOR_PICKER_SLIDER_SPACING * 2.0f);
}

/**
 * Clamps a value to the [0, 1] range.
 * @param value The value to clamp.
 * @return The clamped result.
 */
float ColorPickerClamp01(float value) {
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

/**
 * Converts a 0-1 float color channel to a 0-255 byte.
 * @param value The color channel value.
 * @return The rounded byte value.
 */
uint8_t ColorPickerColorToByte(float value) {
    // We round so values like 0.5 map cleanly to 128 instead of truncating.
    float clamped = ColorPickerClamp01(value);
    int rounded = (int)(clamped * 255.0f + 0.5f);
    if (rounded < 0) {
        rounded = 0;
    }
    if (rounded > 255) {
        rounded = 255;
    }
    return (uint8_t)rounded;
}

/**
 * Opens the color picker for the specified slot.
 * @param state Pointer to the ColorPickerUIState to modify.
 * @param slotIndex Slot index to edit.
 */
void ColorPickerUIOpen(ColorPickerUIState *state, int slotIndex) {
    if (state == NULL) {
        return;
    }

    // Reset transient interaction state whenever the picker is opened.
    state->open = true;
    state->slotIndex = slotIndex;
    state->dirty = false;
    ColorPickerResetDragCache(state);
}

/**
 * Closes the color picker and optionally queues a commit.
 * @param state Pointer to the ColorPickerUIState to modify.
 * @param commit True to queue a commit if changes were made.
 * @param color RGBA color currently displayed for the slot.
 * @param factionId Faction ID to include with any commit.
 */
void ColorPickerUIClose(ColorPickerUIState *state, bool commit, const float color[4], int factionId) {
    if (state == NULL) {
        return;
    }

    // Only queue a commit when the user actually changed something.
    if (commit && state->dirty && color != NULL && factionId >= 0) {
        state->commitPending = true;
        state->commitFactionId = factionId;
        state->commitR = ColorPickerColorToByte(color[0]);
        state->commitG = ColorPickerColorToByte(color[1]);
        state->commitB = ColorPickerColorToByte(color[2]);
    }

    state->open = false;
    state->slotIndex = -1;
    state->dirty = false;
    ColorPickerResetDragCache(state);
}

/**
 * Begins an active drag operation on a slider.
 * @param state Pointer to the ColorPickerUIState to modify.
 * @param channel The channel being dragged (0=R,1=G,2=B).
 * @param sliderX Left X coordinate of the slider track.
 * @param sliderWidth Width of the slider track.
 */
void ColorPickerUIBeginDrag(ColorPickerUIState *state, int channel, float sliderX, float sliderWidth) {
    if (state == NULL) {
        return;
    }

    // Cache slider geometry so drag updates don't depend on recomputing layout.
    state->dragging = true;
    state->channel = channel;
    state->sliderX = sliderX;
    state->sliderWidth = sliderWidth;
}

/**
 * Ends any active slider drag.
 * @param state Pointer to the ColorPickerUIState to modify.
 */
void ColorPickerUIEndDrag(ColorPickerUIState *state) {
    if (state == NULL) {
        return;
    }

    // Dragging state is transient and should always be reset on mouse up.
    state->dragging = false;
}

/**
 * Updates the active slider drag with a new mouse position.
 * @param state Pointer to the ColorPickerUIState to read.
 * @param mouseX Current mouse X position.
 * @param color RGBA color array to update (0-1 range).
 * @param outDirty Output flag set true when color was modified.
 */
void ColorPickerUIUpdateDrag(const ColorPickerUIState *state, float mouseX, float color[4], bool *outDirty) {
    if (state == NULL || color == NULL || outDirty == NULL) {
        return;
    }

    if (!state->dragging || state->sliderWidth <= 0.0f) {
        return;
    }

    // We normalize by slider width so the value tracks consistently across sizes.
    float t = (mouseX - state->sliderX) / state->sliderWidth;
    t = ColorPickerClamp01(t);

    if (state->channel >= 0 && state->channel < 3) {
        color[state->channel] = t;
        color[3] = 1.0f;
        *outDirty = true;
    }
}

/**
 * Consumes a pending color commit, if present.
 * @param state Pointer to the ColorPickerUIState to modify.
 * @param outFactionId Output faction ID.
 * @param outR Output red channel (0-255).
 * @param outG Output green channel (0-255).
 * @param outB Output blue channel (0-255).
 * @return True if a commit was consumed, false otherwise.
 */
bool ColorPickerUIConsumeCommit(ColorPickerUIState *state, int *outFactionId, uint8_t *outR, uint8_t *outG, uint8_t *outB) {
    if (state == NULL || outFactionId == NULL || outR == NULL || outG == NULL || outB == NULL) {
        return false;
    }

    if (!state->commitPending) {
        return false;
    }

    // We clear the pending flag so each commit is consumed exactly once.
    *outFactionId = state->commitFactionId;
    *outR = state->commitR;
    *outG = state->commitG;
    *outB = state->commitB;
    state->commitPending = false;
    return true;
}
