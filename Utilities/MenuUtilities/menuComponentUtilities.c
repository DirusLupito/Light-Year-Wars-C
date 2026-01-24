/**
 * Implementation of shared menu UI components.
 * Provides reusable text field and button primitives plus layout helpers so
 * screens can compose UI without duplicating interaction logic.
 * @file Utilities/MenuUtilities/menuComponentUtilities.c
 * @author abmize
 */

#include "Utilities/MenuUtilities/menuComponentUtilities.h"

#include <ctype.h>
#include <math.h>
#include <string.h>

/**
 * Internal helper to determine if a character is allowed for a field type.
 * We gate input here so individual screens do not have to duplicate rules.
 * @param field Pointer to the component describing validation rules.
 * @param ch Character to test.
 * @return True if the character is permitted, false otherwise.
 */
static bool MenuInputFieldAcceptsChar(const MenuInputFieldComponent *field, char ch) {
    if (field == NULL) {
        return false;
    }

    // Depending on the field kind, apply different validation rules.
    switch (field->spec.kind) {
        // Integer field: only digits allowed.
        case MENU_INPUT_FIELD_INT:
            return isdigit((unsigned char)ch) != 0;

        // Float field: digits and one decimal point allowed.
        case MENU_INPUT_FIELD_FLOAT: {
            bool isDigit = isdigit((unsigned char)ch) != 0;
            bool isDot = ch == '.';
            if (!isDigit && !isDot) {
                return false;
            }
            if (isDot && field->buffer != NULL && strchr(field->buffer, '.') != NULL) {
                return false;
            }
            return true;
        }

        // IP field: digits and dots allowed.
        case MENU_INPUT_FIELD_IP:
            return isdigit((unsigned char)ch) != 0 || ch == '.';

        // Text field: all printable ASCII characters allowed.
        case MENU_INPUT_FIELD_TEXT:
            return ch >= 32 && ch <= 126;
        default:
            return false;
    }
}

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
    size_t *length) {
    if (field == NULL || spec == NULL || buffer == NULL || length == NULL) {
        return;
    }

    field->spec = *spec;
    field->rect = MenuUIRectMake(0.0f, 0.0f, spec->height, spec->height);
    field->buffer = buffer;
    field->capacity = capacity;
    field->length = length;
    field->focused = false;
    field->editable = true;
}

/**
 * Takes in a menu input field component and sets its rectangle
 * to the given x, y, width, and the height from its spec.
 * @param field Pointer to the component to update.
 * @param x Left coordinate of the field.
 * @param y Top coordinate of the field.
 * @param width Total width allocated to the field.
 */
void MenuInputFieldLayout(MenuInputFieldComponent *field, float x, float y, float width) {
    if (field == NULL) {
        return;
    }

    // Set the field rectangle based on input parameters and spec height.
    field->rect.x = x;
    field->rect.y = y;
    field->rect.width = width;
    field->rect.height = field->spec.height;
}

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
    const float placeholderColor[4]) {
    if (field == NULL || context == NULL) {
        return;
    }

    float outline[4] = MENU_INPUT_BOX_OUTLINE_COLOR;
    float fill[4] = MENU_INPUT_BOX_FILL_COLOR;

    // Focused or read-only states tweak alpha to give quick visual feedback.
    if (field->focused) {
        outline[3] = MENU_INPUT_BOX_FOCUSED_ALPHA;
    } else if (!field->editable) {
        outline[3] *= 0.45f;
        fill[3] *= 0.6f;
    }

    DrawOutlinedRectangle(field->rect.x, field->rect.y,
        field->rect.x + field->rect.width,
        field->rect.y + field->rect.height,
        outline, fill);

    // The y position for the label is slightly above the field
    // and not directly touching it so it remains readable.
    float labelY = field->rect.y - 6.0f;
    DrawScreenText(context, field->spec.label, field->rect.x, labelY,
        MENU_LABEL_TEXT_HEIGHT, MENU_LABEL_TEXT_WIDTH, labelColor);

    // Center text vertically; small left padding improves readability
    // in case the field touches some edge of an enclosing rectangle.
    const float textX = field->rect.x + 6.0f;
    const float textY = field->rect.y + (field->rect.height * 0.5f) + (MENU_INPUT_TEXT_HEIGHT * 0.5f);
    if (field->length != NULL && *field->length > 0) {
        DrawScreenText(context, field->buffer, textX, textY,
            MENU_INPUT_TEXT_HEIGHT, MENU_INPUT_TEXT_WIDTH, textColor);
    } else {
        DrawScreenText(context, field->spec.placeholder, textX, textY,
            MENU_INPUT_TEXT_HEIGHT, MENU_INPUT_TEXT_WIDTH, placeholderColor);
    }
}

/**
 * Clears or applies focus to a text field.
 * @param field Pointer to the component to modify.
 * @param focused True to mark focused, false to clear.
 */
void MenuInputFieldSetFocus(MenuInputFieldComponent *field, bool focused) {
    if (field == NULL) {
        return;
    }
    field->focused = focused;
}

/**
 * Handles a printable character input for the text field.
 * Respects the field's validation rules and buffer capacity.
 * @param field Pointer to the component to modify.
 * @param codepoint Unicode codepoint of the character being typed.
 * @return True if the character was appended, false otherwise.
 */
bool MenuInputFieldHandleChar(MenuInputFieldComponent *field, unsigned int codepoint) {
    if (field == NULL || !field->editable || field->buffer == NULL || field->length == NULL) {
        return false;
    }

    // Only accept printable ASCII characters.
    if (codepoint < 32u || codepoint > 126u) {
        return false;
    }

    // Ensure there is space in the buffer for the new character plus null terminator.
    if (*field->length >= field->capacity - 1) {
        return false;
    }

    // Validate the character against the field's rules.
    char ch = (char)codepoint;
    if (!MenuInputFieldAcceptsChar(field, ch)) {
        return false;
    }

    // Append the character to the buffer and update length.
    field->buffer[*field->length] = ch;
    *field->length += 1;
    field->buffer[*field->length] = '\0';
    return true;
}

/**
 * Removes the last character from the field's buffer, if any.
 * @param field Pointer to the component to modify.
 */
void MenuInputFieldHandleBackspace(MenuInputFieldComponent *field) {
    if (field == NULL || field->buffer == NULL || field->length == NULL) {
        return;
    }

    // If the buffer is already empty, nothing to do.
    if (*field->length == 0) {
        return;
    }

    // Remove the last character and update the null terminator.
    *field->length -= 1;
    field->buffer[*field->length] = '\0';
}

/**
 * Initializes a button component with the desired label and dimensions.
 * @param button Pointer to the component to initialize.
 * @param label Null-terminated label string to display.
 * @param preferredWidth Preferred width for centering within a layout region.
 * @param height Fixed height of the button.
 */
void MenuButtonInitialize(MenuButtonComponent *button, const char *label, float preferredWidth, float height) {
    if (button == NULL || label == NULL) {
        return;
    }

    // Initialize all fields of the button component.
    button->label = label;
    button->preferredWidth = preferredWidth;
    button->height = height;
    button->rect = MenuUIRectMake(0.0f, 0.0f, preferredWidth, height);
    button->pressed = false;
    button->enabled = true;
}

/**
 * Lays out a button within an inner panel width, centering it horizontally.
 * @param button Pointer to the component to update.
 * @param x Left edge of the panel's inner content.
 * @param y Top coordinate for the button.
 * @param innerWidth Width available for the button to occupy.
 */
void MenuButtonLayout(MenuButtonComponent *button, float x, float y, float innerWidth) {
    if (button == NULL) {
        return;
    }

    // Compute the actual width to use, clamped to the inner width.
    float width = fminf(button->preferredWidth, innerWidth);
    if (width < 1.0f) {
        width = innerWidth;
    }

    // Center the button within the inner width.
    button->rect.x = x + (innerWidth - width) * 0.5f;
    button->rect.y = y;
    button->rect.width = width;
    button->rect.height = button->height;
}

/**
 * Draws the button using shared menu colors and hover feedback.
 * @param button Pointer to the component to draw.
 * @param context OpenGL context used for text rendering.
 * @param mouseX Current mouse X coordinate for hover detection.
 * @param mouseY Current mouse Y coordinate for hover detection.
 * @param enabled True if the button should render as interactive.
 */
void MenuButtonDraw(const MenuButtonComponent *button, OpenGLContext *context, float mouseX, float mouseY, bool enabled) {
    if (button == NULL || context == NULL) {
        return;
    }

    // Determine if the mouse is hovering over the button.
    bool hover = MenuUIRectContains(&button->rect, mouseX, mouseY) && enabled;
    float outline[4] = MENU_BUTTON_OUTLINE_COLOR;
    float fill[4] = MENU_BUTTON_FILL_COLOR;
    float hoverFill[4] = MENU_BUTTON_HOVER_FILL_COLOR;

    const float *fillChoice = hover ? hoverFill : fill;

    // Dim the button when disabled so users know the action is unavailable.
    if (!enabled) {
        outline[3] *= 0.6f;
        fill[3] *= 0.6f;
        hoverFill[3] *= 0.6f;
        fillChoice = hover ? hoverFill : fill;
    }

    DrawOutlinedRectangle(button->rect.x, button->rect.y,
        button->rect.x + button->rect.width,
        button->rect.y + button->rect.height,
        outline, fillChoice);

    // The text color also dims when disabled.
    float textColor[4] = MENU_INPUT_TEXT_COLOR;
    if (!enabled) {
        textColor[3] *= 0.7f;
    }

    // Center the label text within the button.
    float textWidth = (float)strlen(button->label) * MENU_BUTTON_TEXT_WIDTH;
    float textX = button->rect.x + (button->rect.width * 0.5f) - (textWidth * 0.5f);
    float textY = button->rect.y + (button->rect.height * 0.5f) + (MENU_BUTTON_TEXT_HEIGHT * 0.5f);
    DrawScreenText(context, button->label, textX, textY, MENU_BUTTON_TEXT_HEIGHT, MENU_BUTTON_TEXT_WIDTH, textColor);
}

/**
 * Handles mouse button down for the button component.
 * @param button Pointer to the component to update.
 * @param x Mouse X coordinate at event time.
 * @param y Mouse Y coordinate at event time.
 * @return True if the button captured the press, false otherwise.
 */
bool MenuButtonHandleMouseDown(MenuButtonComponent *button, float x, float y) {
    if (button == NULL || !button->enabled) {
        return false;
    }

    // Set the pressed state if the mouse is within the button rectangle.
    button->pressed = MenuUIRectContains(&button->rect, x, y);
    return button->pressed;
}

/**
 * Handles mouse button up for the button component and reports activation.
 * @param button Pointer to the component to update.
 * @param x Mouse X coordinate at event time.
 * @param y Mouse Y coordinate at event time.
 * @param outActivated Output flag set true when a full click occurred.
 * @return True if the event was handled, false otherwise.
 */
bool MenuButtonHandleMouseUp(MenuButtonComponent *button, float x, float y, bool *outActivated) {
    if (button == NULL || !button->enabled) {
        return false;
    }

    // Check if the button was previously pressed.
    bool wasPressed = button->pressed;
    button->pressed = false;

    // !wasPressed means the button wasn't engaged, so nothing to do.
    // The user probably clicked down elsewhere and released here.
    if (!wasPressed) {
        return false;
    }

    // Determine if the mouse is still within the button rectangle to consider it activated.
    if (outActivated != NULL) {
        *outActivated = MenuUIRectContains(&button->rect, x, y);
    }

    return true;
}

/**
 * Computes a baseline Y coordinate used to vertically center content.
 * @param contentHeight Total height of content being laid out.
 * @param viewportHeight Available viewport height.
 * @return Base Y offset for top of the content block.
 */
float MenuLayoutComputeBaseY(float contentHeight, float viewportHeight) {
    if (viewportHeight <= 0.0f) {
        return 16.0f;
    }

    // If content fits within the viewport, center it vertically with a minimum margin.
    if (contentHeight <= viewportHeight) {
        float centered = (viewportHeight - contentHeight) * 0.5f;
        if (centered < 16.0f) {
            centered = 16.0f;
        }
        return centered;
    }

    return 16.0f;
}

/**
 * Clamps a scroll offset so that content stays within the viewport bounds.
 * @param contentHeight Total scrollable content height.
 * @param viewportHeight Height of the viewport.
 * @param scrollOffset Current scroll value to clamp.
 * @return The clamped scroll offset.
 */
float MenuLayoutClampScroll(float contentHeight, float viewportHeight, float scrollOffset) {
    // The maximum scroll offset is content height minus viewport height.
    float maxScroll = contentHeight - viewportHeight;

    // If content fits within the viewport, no scrolling is needed.
    if (maxScroll < 0.0f) {
        maxScroll = 0.0f;
    }

    // Otherwise, clamp the scroll offset within valid bounds.
    float clamped = scrollOffset;
    if (clamped < 0.0f) {
        clamped = 0.0f;
    }
    if (clamped > maxScroll) {
        clamped = maxScroll;
    }
    return clamped;
}
