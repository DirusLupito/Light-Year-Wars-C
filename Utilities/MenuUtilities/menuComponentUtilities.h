/**
 * Header for shared menu UI component helpers.
 * Provides composable text fields, buttons, and layout helpers that can be
 * reused by different screens to keep menu logic modular and maintainable.
 * @file Utilities/MenuUtilities/menuComponentUtilities.h
 * @author abmize
 */
#ifndef _MENU_COMPONENT_UTILITIES_H_
#define _MENU_COMPONENT_UTILITIES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Utilities/openglUtilities.h"
#include "Utilities/renderUtilities.h"
#include "Utilities/MenuUtilities/commonMenuUtilities.h"

/**
 * Defines the validation mode for a text field.
 * Determines which characters are accepted when typing.
 */
typedef enum MenuInputFieldKind {
    MENU_INPUT_FIELD_INT = 0,
    MENU_INPUT_FIELD_FLOAT = 1,
    MENU_INPUT_FIELD_IP = 2,
    MENU_INPUT_FIELD_TEXT = 3
} MenuInputFieldKind;

/**
 * Immutable configuration for a text field component.
 * Describes visuals and validation but not the live buffer.
 */
typedef struct MenuInputFieldSpec {
    const char *label;
    const char *placeholder;
    float height;
    float spacingBelow;
    MenuInputFieldKind kind;
} MenuInputFieldSpec;

/**
 * Represents a text field that owns no string storage but draws and validates
 * against external buffers. This keeps UI plumbing separate from data storage.
 */
typedef struct MenuInputFieldComponent {
    MenuInputFieldSpec spec;
    MenuUIRect rect;
    char *buffer;
    size_t capacity;
    size_t *length;
    bool focused;
    bool editable;
} MenuInputFieldComponent;

/**
 * Represents a clickable button component.
 * Stores geometry, sizing preferences, and pressed state.
 */
typedef struct MenuButtonComponent {
    const char *label;
    float preferredWidth; // The preferred width is the width used for centering. The actual width is in rect.
    float height;
    MenuUIRect rect;
    bool pressed;
    bool enabled;
} MenuButtonComponent;

/**
 * Initializes a text field component with the given specification and backing buffer.
 * The field does not copy the label or buffer; callers must ensure their lifetime.
 * @param field Pointer to the component to initialize.
 * @param spec Pointer to the static specification describing the field.
 * @param buffer Mutable character buffer to edit.
 * @param capacity Total capacity of the buffer including null terminator.
 * @param length Pointer to the live length counter for the buffer.
 */
void MenuInputFieldInitialize(MenuInputFieldComponent *field,
    const MenuInputFieldSpec *spec,
    char *buffer,
    size_t capacity,
    size_t *length);

/**
 * Lays out a text field at the given position and width.
 * @param field Pointer to the component to update.
 * @param x Left coordinate of the field.
 * @param y Top coordinate of the field.
 * @param width Total width allocated to the field.
 */
void MenuInputFieldLayout(MenuInputFieldComponent *field, float x, float y, float width);

/**
 * Draws a text field using shared menu colors.
 * @param field Pointer to the component to draw.
 * @param context OpenGL context used for text rendering.
 * @param labelColor RGBA color for labels.
 * @param textColor RGBA color for typed text.
 * @param placeholderColor RGBA color for placeholder text.
 */
void MenuInputFieldDraw(const MenuInputFieldComponent *field,
    OpenGLContext *context,
    const float labelColor[4],
    const float textColor[4],
    const float placeholderColor[4]);

/**
 * Clears or applies focus to a text field.
 * @param field Pointer to the component to modify.
 * @param focused True to mark focused, false to clear.
 */
void MenuInputFieldSetFocus(MenuInputFieldComponent *field, bool focused);

/**
 * Handles a printable character input for the text field.
 * Respects the field's validation rules and buffer capacity.
 * @param field Pointer to the component to modify.
 * @param codepoint Unicode codepoint of the character being typed.
 * @return True if the character was appended, false otherwise.
 */
bool MenuInputFieldHandleChar(MenuInputFieldComponent *field, unsigned int codepoint);

/**
 * Removes the last character from the field's buffer, if any.
 * @param field Pointer to the component to modify.
 */
void MenuInputFieldHandleBackspace(MenuInputFieldComponent *field);

/**
 * Initializes a button component with the desired label and dimensions.
 * @param button Pointer to the component to initialize.
 * @param label Null-terminated label string to display.
 * @param preferredWidth Preferred width for centering within a layout region.
 * @param height Fixed height of the button.
 */
void MenuButtonInitialize(MenuButtonComponent *button, const char *label, float preferredWidth, float height);

/**
 * Lays out a button within an inner panel width, centering it horizontally.
 * @param button Pointer to the component to update.
 * @param x Left edge of the panel's inner content.
 * @param y Top coordinate for the button.
 * @param innerWidth Width available for the button to occupy.
 */
void MenuButtonLayout(MenuButtonComponent *button, float x, float y, float innerWidth);

/**
 * Draws the button using shared menu colors and hover feedback.
 * @param button Pointer to the component to draw.
 * @param context OpenGL context used for text rendering.
 * @param mouseX Current mouse X coordinate for hover detection.
 * @param mouseY Current mouse Y coordinate for hover detection.
 * @param enabled True if the button should render as interactive.
 */
void MenuButtonDraw(const MenuButtonComponent *button, OpenGLContext *context, float mouseX, float mouseY, bool enabled);

/**
 * Handles mouse button down for the button component.
 * @param button Pointer to the component to update.
 * @param x Mouse X coordinate at event time.
 * @param y Mouse Y coordinate at event time.
 * @return True if the button captured the press, false otherwise.
 */
bool MenuButtonHandleMouseDown(MenuButtonComponent *button, float x, float y);

/**
 * Handles mouse button up for the button component and reports activation.
 * @param button Pointer to the component to update.
 * @param x Mouse X coordinate at event time.
 * @param y Mouse Y coordinate at event time.
 * @param outActivated Output flag set true when a full click occurred.
 * @return True if the event was handled, false otherwise.
 */
bool MenuButtonHandleMouseUp(MenuButtonComponent *button, float x, float y, bool *outActivated);

/**
 * Computes a baseline Y coordinate used to vertically center content.
 * @param contentHeight Total height of content being laid out.
 * @param viewportHeight Available viewport height.
 * @return Base Y offset for top of the content block.
 */
float MenuLayoutComputeBaseY(float contentHeight, float viewportHeight);

/**
 * Clamps a scroll offset so that content stays within the viewport bounds.
 * @param contentHeight Total scrollable content height.
 * @param viewportHeight Height of the viewport.
 * @param scrollOffset Current scroll value to clamp.
 * @return The clamped scroll offset.
 */
float MenuLayoutClampScroll(float contentHeight, float viewportHeight, float scrollOffset);

#endif /* _MENU_COMPONENT_UTILITIES_H_ */
