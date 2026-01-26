/**
 * Header for color picker UI utilities.
 * Provides shared state and helpers for RGB color picker interactions.
 * @file Utilities/MenuUtilities/colorPickerUtilities.h
 * @author abmize
 */
#ifndef _COLOR_PICKER_UTILITIES_H_
#define _COLOR_PICKER_UTILITIES_H_

#include <stdbool.h>
#include <stdint.h>

// Size of the color swatch displayed in slot rows.
#define COLOR_PICKER_SWATCH_SIZE 36.0f

// Padding between the color swatch and the slot area edge.
#define COLOR_PICKER_SWATCH_PADDING 6.0f

// Padding inside the color picker dropdown panel.
#define COLOR_PICKER_PANEL_PADDING 10.0f

// Height of each color slider bar.
#define COLOR_PICKER_SLIDER_HEIGHT 12.0f

// Spacing between color slider bars.
#define COLOR_PICKER_SLIDER_SPACING 8.0f

// Width reserved for color slider labels ("R", "G", "B").
#define COLOR_PICKER_SLIDER_LABEL_WIDTH 18.0f

/**
 * Stores state for the RGB color picker dropdown.
 * Tracks interaction, permissions, and pending commit state.
 */
typedef struct ColorPickerUIState {
    bool open; /* True while the dropdown is visible. */
    int slotIndex; /* Slot index currently being edited. */
    bool dirty; /* Tracks whether changes were made before closing. */
    bool dragging; /* True while a slider drag is active. */
    int channel; /* 0=R,1=G,2=B */
    float sliderX; /* Cached slider X to avoid recomputing during drag. */
    float sliderWidth; /* Cached slider width to avoid recomputing during drag. */

    bool commitPending; /* True when a color commit should be consumed. */
    int commitFactionId; /* Faction ID the commit applies to. */
    uint8_t commitR; /* 0-255 red channel. */
    uint8_t commitG; /* 0-255 green channel. */
    uint8_t commitB; /* 0-255 blue channel. */

    bool allowAll; /* True when any slot can be edited (server). */
    int allowedFactionId; /* Faction ID allowed to edit color when allowAll is false. */
} ColorPickerUIState;

/**
 * Initializes a color picker UI state to defaults.
 * @param state Pointer to the ColorPickerUIState to initialize.
 */
void ColorPickerUIInitialize(ColorPickerUIState *state);

/**
 * Updates which faction IDs are allowed to edit colors.
 * @param state Pointer to the ColorPickerUIState to modify.
 * @param allowAll True to allow editing any slot color.
 * @param factionId Faction ID allowed to edit when allowAll is false.
 */
void ColorPickerUISetEditAuthority(ColorPickerUIState *state, bool allowAll, int factionId);

/**
 * Returns whether the given faction ID is editable under current permissions.
 * @param state Pointer to the ColorPickerUIState to check.
 * @param factionId Faction ID to test.
 * @return True if editing is permitted, false otherwise.
 */
bool ColorPickerUICanEdit(const ColorPickerUIState *state, int factionId);

/**
 * Computes the height of the dropdown panel for the RGB sliders.
 * @return The total height in pixels.
 */
float ColorPickerUIHeight(void);

/**
 * Clamps a value to the [0, 1] range.
 * @param value The value to clamp.
 * @return The clamped result.
 */
float ColorPickerClamp01(float value);

/**
 * Converts a 0-1 float color channel to a 0-255 byte.
 * @param value The color channel value.
 * @return The rounded byte value.
 */
uint8_t ColorPickerColorToByte(float value);

/**
 * Opens the color picker for the specified slot.
 * @param state Pointer to the ColorPickerUIState to modify.
 * @param slotIndex Slot index to edit.
 */
void ColorPickerUIOpen(ColorPickerUIState *state, int slotIndex);

/**
 * Closes the color picker and optionally queues a commit.
 * @param state Pointer to the ColorPickerUIState to modify.
 * @param commit True to queue a commit if changes were made.
 * @param color RGBA color currently displayed for the slot.
 * @param factionId Faction ID to include with any commit.
 */
void ColorPickerUIClose(ColorPickerUIState *state, bool commit, const float color[4], int factionId);

/**
 * Begins an active drag operation on a slider.
 * @param state Pointer to the ColorPickerUIState to modify.
 * @param channel The channel being dragged (0=R,1=G,2=B).
 * @param sliderX Left X coordinate of the slider track.
 * @param sliderWidth Width of the slider track.
 */
void ColorPickerUIBeginDrag(ColorPickerUIState *state, int channel, float sliderX, float sliderWidth);

/**
 * Ends any active slider drag.
 * @param state Pointer to the ColorPickerUIState to modify.
 */
void ColorPickerUIEndDrag(ColorPickerUIState *state);

/**
 * Updates the active slider drag with a new mouse position.
 * @param state Pointer to the ColorPickerUIState to read.
 * @param mouseX Current mouse X position.
 * @param color RGBA color array to update (0-1 range).
 * @param outDirty Output flag set true when color was modified.
 */
void ColorPickerUIUpdateDrag(const ColorPickerUIState *state, float mouseX, float color[4], bool *outDirty);

/**
 * Consumes a pending color commit, if present.
 * @param state Pointer to the ColorPickerUIState to modify.
 * @param outFactionId Output faction ID.
 * @param outR Output red channel (0-255).
 * @param outG Output green channel (0-255).
 * @param outB Output blue channel (0-255).
 * @return True if a commit was consumed, false otherwise.
 */
bool ColorPickerUIConsumeCommit(ColorPickerUIState *state, int *outFactionId, uint8_t *outR, uint8_t *outG, uint8_t *outB);

#endif // _COLOR_PICKER_UTILITIES_H_
