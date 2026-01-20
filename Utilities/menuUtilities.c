/**
 * Implementation file for various menu UI related utilities.
 * Used by the Light Year Wars program to help render and manage
 * menu UI elements such as text input fields and buttons.
 * @author abmize
 * @file menuUtilities.c
 */

#include "Utilities/menuUtilities.h"

/**
 * Helper function to trim leading and trailing whitespace
 * from a given string in place.
 * @param text The string to trim. Must be null-terminated.
 */
void TrimWhitespace(char *text) {
    // Null means no text to trim.
    if (text == NULL) {
        return;
    }

    // Likewise, an empty string/just the null terminator means nothing to trim.
    size_t len = strlen(text);
    if (len == 0) {
        return;
    }

    // Find the first non-whitespace character from the start.
    size_t start = 0;
    while (start < len && isspace((unsigned char)text[start])) {
        start++;
    }

    // Find the last non-whitespace character from the end.
    size_t end = len;
    while (end > start && isspace((unsigned char)text[end - 1])) {
        end--;
    }

    // Shift the trimmed content to the start of the string,
    // or clear it if there's nothing but whitespace.
    size_t newLength = (end > start) ? (end - start) : 0;

    if (start > 0 && newLength > 0) {
        // We move the memory region containing the text
        // between start and end to the beginning of the string.
        // Note that we still need to add a null terminator after the new content.
        // Also note that memcpy is inappropriate here since the regions may overlap.

        memmove(text, text + start, newLength);
    } else if (start > 0) {
        text[0] = '\0';
        return;
    }

    // And here we do just that.
    text[newLength] = '\0';
}

/**
 * Helper function to create a MenuUIRect
 * given position and size.
 * @param x The X position of the rectangle.
 * @param y The Y position of the rectangle.
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 * @return The constructed MenuUIRect.
 */
static MenuUIRect MenuUIRectMake(float x, float y, float width, float height) {
    MenuUIRect rect;
    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;
    return rect;
}

/**
 * Helper function to determine if a point is within a MenuUIRect.
 * @param rect Pointer to the MenuUIRect.
 * @param x The X coordinate of the point.
 * @param y The Y coordinate of the point.
 * @return True if the point is within the rectangle, false otherwise.
 */
static bool MenuUIRectContains(const MenuUIRect *rect, float x, float y) {
    if (rect == NULL) {
        return false;
    }
    // Simple check against rectangle bounds.
    // Something interesting: For checking circles, we use the 2-norm (Euclidean distance).
    // For squares, it would be the infinity-norm (max of absolute differences).
    // And here its more or less the infinity-norm again, but split because our shape is not actually
    // a perfect square that the infinity-norm would define, but rather a rectangle.
    // So the question is, when do we more or less follow the p-norm formula for checking something
    // like inclusion, and when do we do something more like this? For the 1-norm, the shape is a diamond,
    // and we would follow the formula. So what about an ellipse? Would it be related to the 2-norm like
    // how the rectangle is related to the infinity-norm?
    return x >= rect->x && x <= rect->x + rect->width && y >= rect->y && y <= rect->y + rect->height;
}

/**
 * Helper function to compute the layout of the menu UI elements
 * based on the current window size. Will seek to center the elements
 * and size them appropriately.
 * @param width The width of the window.
 * @param height The height of the window.
 * @param panel Output parameter for the panel rectangle.
 * @param ipField Output parameter for the IP field rectangle.
 * @param portField Output parameter for the port field rectangle.
 * @param button Output parameter for the connect button rectangle.
 */
static void MenuUIComputeLayout(int width, int height, MenuUIRect *panel, MenuUIRect *ipField, MenuUIRect *portField, MenuUIRect *button) {
    // We try to ensure width and height are at least 1.0f
    // to avoid weirdness with very small windows or zero division, etc.
    float w = (float)width;
    float h = (float)height;

    if (w < 1.0f) {
        w = 1.0f;
    }

    if (h < 1.0f) {
        h = 1.0f;
    }

    // Compute field and button sizes based on window size.
    float fieldWidth = fminf(420.0f, w - 2.0f * MENU_PANEL_PADDING);

    if (fieldWidth < 220.0f) {
        fieldWidth = 220.0f;
    }
    
    // Compute button width based on field width.
    float buttonWidth = fminf(fieldWidth, 240.0f);

    // Compute positions to center the elements.
    float centerX = w * 0.5f;
    float startY = h * 0.5f - MENU_FIELD_HEIGHT - MENU_FIELD_SPACING;

    // Output the rectangles if requested.
    if (ipField != NULL) {
        float x = centerX - fieldWidth * 0.5f;
        *ipField = MenuUIRectMake(x, startY, fieldWidth, MENU_FIELD_HEIGHT);
    }

    if (portField != NULL) {
        float x = centerX - fieldWidth * 0.5f;
        float y = startY + MENU_FIELD_HEIGHT + MENU_FIELD_SPACING;
        *portField = MenuUIRectMake(x, y, fieldWidth, MENU_FIELD_HEIGHT);
    }

    if (button != NULL) {
        float x = centerX - buttonWidth * 0.5f;
        float y = startY + 2.0f * (MENU_FIELD_HEIGHT + MENU_FIELD_SPACING);
        *button = MenuUIRectMake(x, y, buttonWidth, MENU_BUTTON_HEIGHT);
    }

    // Finally, output the panel rectangle if requested.
    if (panel != NULL) {
        float top = startY - MENU_PANEL_PADDING;
        float bottom = startY + 2.0f * (MENU_FIELD_HEIGHT + MENU_FIELD_SPACING) + MENU_BUTTON_HEIGHT + MENU_PANEL_PADDING;
        float panelHeight = bottom - top;
        float panelWidth = fieldWidth + MENU_PANEL_PADDING * 2.0f;
        float x = centerX - panelWidth * 0.5f;
        *panel = MenuUIRectMake(x, top, panelWidth, panelHeight);
    }
}

/**
 * Helper function to backspace (remove last character)
 * from a specified input field.
 * @param state Pointer to the MenuUIState.
 * @param target The focus target indicating which field to backspace.
 */
static void MenuUIBackspaceField(MenuUIState *state, MenuFocusTarget target) {
    // Nothing to backspace if state is null.
    if (state == NULL) {
        return;
    }

    // Backspace the appropriate field
    // depending on the target.
    if (target == MENU_FOCUS_IP) {
        if (state->ipLength > 0) {
            state->ipLength--;
            state->ipBuffer[state->ipLength] = '\0';
        }
    } else if (target == MENU_FOCUS_PORT) {
        if (state->portLength > 0) {
            state->portLength--;
            state->portBuffer[state->portLength] = '\0';
        }
    }
}

/**
 * Helper function to append a character
 * to a specified input field.
 * @param state Pointer to the MenuUIState.
 * @param target The focus target indicating which field to append to.
 * @param value The character to append.
 */
static void MenuUIAppendToField(MenuUIState *state, MenuFocusTarget target, char value) {
    // Nothing to append to if state is null.
    if (state == NULL) {
        return;
    }

    // Append to the appropriate field.
    if (target == MENU_FOCUS_IP) {
        if (state->ipLength >= MENU_IP_MAX_LENGTH) {
            return;
        }
        state->ipBuffer[state->ipLength++] = value;
        state->ipBuffer[state->ipLength] = '\0';
    } else if (target == MENU_FOCUS_PORT) {
        if (state->portLength >= MENU_PORT_MAX_LENGTH) {
            return;
        }
        state->portBuffer[state->portLength++] = value;
        state->portBuffer[state->portLength] = '\0';
    }
}

/**
 * Initializes the menu UI state to default values.
 * @param state Pointer to the MenuUIState to initialize.
 */
void MenuUIInitialize(MenuUIState *state) {
    // If state is null, we don't have a good pointer to initialize.
    if (state == NULL) {
        return;
    }

    // Zero out the entire structure.
    memset(state, 0, sizeof(*state));

    // Set initial focus.
    state->focus = MENU_FOCUS_NONE;

    // Prepare default status message.
    MenuUISetStatusMessage(state, "Enter the server IP and port, then click Connect.");
}

/**
 * Handles mouse movement events for the menu UI.
 * Updates the mouse position in the state.
 * @param state Pointer to the MenuUIState.
 * @param x The new mouse X position.
 * @param y The new mouse Y position.
 */
void MenuUIHandleMouseMove(MenuUIState *state, float x, float y) {
    // If state is null, we can't update anything.
    if (state == NULL) {
        return;
    }

    state->mouseX = x;
    state->mouseY = y;
}

/**
 * Handles mouse button down events for the menu UI.
 * Updates focus state and button press state based on mouse position.
 * @param state Pointer to the MenuUIState.
 * @param x The mouse X position at the time of the event.
 * @param y The mouse Y position at the time of the event.
 * @param width The width of the window.
 * @param height The height of the window.
 */
void MenuUIHandleMouseDown(MenuUIState *state, float x, float y, int width, int height) {
    // If state is null, we can't update anything.
    if (state == NULL) {
        return;
    }

    state->mouseX = x;
    state->mouseY = y;

    // Compute the layout to determine element positions.
    MenuUIRect panel;
    MenuUIRect ipField;
    MenuUIRect portField;
    MenuUIRect button;
    MenuUIComputeLayout(width, height, &panel, &ipField, &portField, &button);

    // Now that we know the element positions,
    // we can update focus and button states
    // based on where the mouse was clicked.

    if (MenuUIRectContains(&ipField, x, y)) {
        state->focus = MENU_FOCUS_IP;
    } else if (MenuUIRectContains(&portField, x, y)) {
        state->focus = MENU_FOCUS_PORT;
    } else if (!MenuUIRectContains(&panel, x, y)) {
        state->focus = MENU_FOCUS_NONE;
    }

    if (MenuUIRectContains(&button, x, y)) {
        state->connectButtonPressed = true;
    } else {
        state->connectButtonPressed = false;
    }
}

/**
 * Handles mouse button up events for the menu UI.
 * Updates button press state and triggers connect requests if applicable.
 * @param state Pointer to the MenuUIState.
 * @param x The mouse X position at the time of the event.
 * @param y The mouse Y position at the time of the event.
 * @param width The width of the window.
 * @param height The height of the window.
 */
void MenuUIHandleMouseUp(MenuUIState *state, float x, float y, int width, int height) {
    // If state is null, we can't update anything.
    if (state == NULL) {
        return;
    }

    state->mouseX = x;
    state->mouseY = y;

    // Only the button matters on mouse up.
    MenuUIRect button;
    MenuUIComputeLayout(width, height, NULL, NULL, NULL, &button);

    // If the button was pressed and the mouse is still over it,
    // we register a connect request.
    bool wasPressed = state->connectButtonPressed;
    state->connectButtonPressed = false;

    if (wasPressed && MenuUIRectContains(&button, x, y)) {
        state->connectRequested = true;
    }
}

/**
 * Handles character input events for the menu UI.
 * Appends valid characters to the focused input field.
 * @param state Pointer to the MenuUIState.
 * @param codepoint The Unicode codepoint of the character input.
 */
void MenuUIHandleChar(MenuUIState *state, unsigned int codepoint) {
    // If state is null, we can't update anything.
    if (state == NULL) {
        return;
    }

    // Only process printable ASCII characters for now.
    if (codepoint < 32u || codepoint > 126u) {
        return;
    }

    char value = (char)codepoint;

    // We only append to the field that is currently focused.
    // For the IP field, we allow digits and dots.
    // For the port field, we allow only digits.
    if (state->focus == MENU_FOCUS_IP) {
        if (isdigit((unsigned char)value) || value == '.') {
            MenuUIAppendToField(state, MENU_FOCUS_IP, value);
        }
    } else if (state->focus == MENU_FOCUS_PORT) {
        if (isdigit((unsigned char)value)) {
            MenuUIAppendToField(state, MENU_FOCUS_PORT, value);
        }
    }
}

/**
 * Handles key down events for the menu UI.
 * Manages special keys like backspace, tab, enter, and escape.
 * @param state Pointer to the MenuUIState.
 * @param key The virtual key code of the key that was pressed.
 * @param shiftDown True if the Shift key is currently held down.
 */
void MenuUIHandleKeyDown(MenuUIState *state, WPARAM key, bool shiftDown) {
    // If state is null, we can't update anything.
    if (state == NULL) {
        return;
    }

    // Depending on the key, we perform different actions.
    switch (key) {
        // Both backspace and delete remove the last character 
        // (imagine as having a cursor locked to the end of the field).
        case VK_BACK:
        case VK_DELETE:
            MenuUIBackspaceField(state, state->focus);
            break;

        // Tab switches focus between fields.
        case VK_TAB:
            if (state->focus == MENU_FOCUS_IP) {
                state->focus = shiftDown ? MENU_FOCUS_NONE : MENU_FOCUS_PORT;
            } else if (state->focus == MENU_FOCUS_PORT) {
                state->focus = shiftDown ? MENU_FOCUS_IP : MENU_FOCUS_NONE;
            } else {
                state->focus = shiftDown ? MENU_FOCUS_PORT : MENU_FOCUS_IP;
            }
            break;

        // Enter/return requests a connection if a field is focused.
        case VK_RETURN:
            if (state->focus == MENU_FOCUS_IP || state->focus == MENU_FOCUS_PORT) {
                state->connectRequested = true;
            }
            break;
        // Escape clears focus.
        case VK_ESCAPE:
            state->focus = MENU_FOCUS_NONE;
            break;
        default:
            // Other keys are not handled here.
            break;
    }
}

/**
 * Consumes a connect request from the menu UI, if one exists.
 * Copies the IP address and port to the provided output buffers.
 * @param state Pointer to the MenuUIState.
 * @param ipOut Output buffer for the IP address.
 * @param ipCapacity Capacity of the IP address output buffer.
 * @param portOut Output buffer for the port.
 * @param portCapacity Capacity of the port output buffer.
 * @return True if a connect request was consumed, false otherwise.
 */
bool MenuUIConsumeConnectRequest(MenuUIState *state, char *ipOut, size_t ipCapacity, char *portOut, size_t portCapacity) {
    // If state or output buffers are null, we can't proceed.
    if (state == NULL || ipOut == NULL || portOut == NULL) {
        return false;
    }

    // If no connect request is pending, return false.
    if (!state->connectRequested) {
        return false;
    }

    // Otherwise, copy the IP and port to the output buffers
    // and clear the connect request flag.
    state->connectRequested = false;

    // If the output buffers are too small, we truncate the data.
    if (ipCapacity > 0) {
        strncpy(ipOut, state->ipBuffer, ipCapacity - 1);
        ipOut[ipCapacity - 1] = '\0';
    }

    if (portCapacity > 0) {
        strncpy(portOut, state->portBuffer, portCapacity - 1);
        portOut[portCapacity - 1] = '\0';
    }

    return true;
}

/**
 * Sets the status message to be displayed in the menu UI.
 * @param state Pointer to the MenuUIState.
 * @param message The status message to set. If NULL, clears the message.
 */
void MenuUISetStatusMessage(MenuUIState *state, const char *message) {
    // If state is null, we can't update anything.
    if (state == NULL) {
        return;
    }

    // If message is null, we clear the status message.
    if (message == NULL) {
        state->statusMessage[0] = '\0';
        return;
    }

    // Otherwise, we copy the message into the status buffer,
    // truncating if necessary.
    strncpy(state->statusMessage, message, MENU_STATUS_MAX_LENGTH);
    state->statusMessage[MENU_STATUS_MAX_LENGTH] = '\0';
}

/**
 * Draws the menu UI elements using the provided OpenGL context.
 * @param state Pointer to the MenuUIState.
 * @param context Pointer to the OpenGLContext for rendering.
 * @param width The width of the window.
 * @param height The height of the window.
 */
void MenuUIDraw(MenuUIState *state, OpenGLContext *context, int width, int height) {
    // If state or context is null, or dimensions are invalid, we can't draw anything.
    if (state == NULL || context == NULL || width <= 0 || height <= 0) {
        return;
    }

    // We retrieve rectangles for all UI elements.
    MenuUIRect panel;
    MenuUIRect ipField;
    MenuUIRect portField;
    MenuUIRect button;
    MenuUIComputeLayout(width, height, &panel, &ipField, &portField, &button);

    // Draw the panel background as a semi-transparent, outlined rectangle.

    // Panel colors

    // Slightly light outline, dark fill.
    float panelOutline[4] = MENU_PANEL_OUTLINE_COLOR;
    float panelFill[4] = MENU_PANEL_FILL_COLOR;
    DrawOutlinedRectangle(panel.x, panel.y, panel.x + panel.width, panel.y + panel.height, panelOutline, panelFill);

    // Draw the IP and port fields and the connect button.
    // Also draw labels and current contents.
    const float labelColor[4] = MENU_LABEL_TEXT_COLOR;
    const float textColor[4] = MENU_INPUT_TEXT_COLOR;
    const float placeholderColor[4] = MENU_PLACEHOLDER_TEXT_COLOR;

    // IP Field

    float ipOutline[4] = MENU_INPUT_BOX_OUTLINE_COLOR;
    
    // If focused, we change the outline color to let
    // the user know.
    if (state->focus == MENU_FOCUS_IP) {
        ipOutline[3] = MENU_INPUT_BOX_FOCUSED_ALPHA;
    }

    float ipFill[4] = MENU_INPUT_BOX_FILL_COLOR;
    DrawOutlinedRectangle(ipField.x, ipField.y, ipField.x + ipField.width, ipField.y + ipField.height, ipOutline, ipFill);

    // We just want to draw the label slightly above the field.
    const char *ipLabel = "Server IP";
    float ipLabelX = ipField.x;
    float ipLabelY = ipField.y - 4.0f;
    DrawScreenText(context, ipLabel, ipLabelX, ipLabelY, MENU_LABEL_TEXT_HEIGHT, MENU_LABEL_TEXT_WIDTH, labelColor);

    // Draw the current IP or placeholder.
    // We want to center the IP text in the field vertically,
    // while having it only a little bit in from the left horizontally.
    if (state->ipLength > 0) {
        float ipTextX = ipField.x + 4.0f;
        float ipTextY = ipField.y + (ipField.height / 2.0f) + (MENU_INPUT_TEXT_HEIGHT / 2.0f);
        DrawScreenText(context, state->ipBuffer, ipTextX, ipTextY, MENU_INPUT_TEXT_HEIGHT, MENU_INPUT_TEXT_WIDTH, textColor);
    } else {
        const char *ipPlaceholderText = "Ex: 127.0.0.1";
        float placeholderX = ipField.x + 4.0f;
        float placeholderY = ipField.y + (ipField.height / 2.0f) + (MENU_INPUT_TEXT_HEIGHT / 2.0f);
        DrawScreenText(context, ipPlaceholderText, placeholderX, placeholderY, MENU_INPUT_TEXT_HEIGHT, MENU_INPUT_TEXT_WIDTH, placeholderColor);
    }

    // Port Field

    float portOutline[4] = MENU_INPUT_BOX_OUTLINE_COLOR;

    // If focused, let the user know.
    if (state->focus == MENU_FOCUS_PORT) {
        portOutline[3] = MENU_INPUT_BOX_FOCUSED_ALPHA;
    }

    float portFill[4] = MENU_INPUT_BOX_FILL_COLOR;
    DrawOutlinedRectangle(portField.x, portField.y, portField.x + portField.width, portField.y + portField.height, portOutline, portFill);

    const char *portLabel = "Server Port";

    // We just want to draw the label slightly above the field.
    float portLabelX = portField.x;
    float portLabelY = portField.y - 4.0f;

    DrawScreenText(context, portLabel, portLabelX, portLabelY, MENU_LABEL_TEXT_HEIGHT, MENU_LABEL_TEXT_WIDTH, labelColor);

    // Draw the current port or placeholder.
    // We want to center the port text in the field vertically,
    // while having it only a little bit in from the left horizontally.
    if (state->portLength > 0) {
        float portTextX = portField.x + 4.0f;
        float portTextY = portField.y + (portField.height / 2.0f) + (MENU_INPUT_TEXT_HEIGHT / 2.0f);
        DrawScreenText(context, state->portBuffer, portTextX, portTextY, MENU_INPUT_TEXT_HEIGHT, MENU_INPUT_TEXT_WIDTH, textColor);
    } else {
        const char *portPlaceholderText = "Ex: 22311";
        float placeholderX = portField.x + 4.0f;
        float placeholderY = portField.y + (portField.height / 2.0f) + (MENU_INPUT_TEXT_HEIGHT / 2.0f);
        DrawScreenText(context, portPlaceholderText, placeholderX, placeholderY, MENU_INPUT_TEXT_HEIGHT, MENU_INPUT_TEXT_WIDTH, placeholderColor);
    }

    // Connect Button

    // Change appearance if hovered.
    bool hover = MenuUIRectContains(&button, state->mouseX, state->mouseY);
    
    float buttonOutline[4] = MENU_BUTTON_OUTLINE_COLOR;

    float *buttonFill = hover ? (float[])MENU_BUTTON_HOVER_FILL_COLOR : (float[])MENU_BUTTON_FILL_COLOR;

    DrawOutlinedRectangle(button.x, button.y, button.x + button.width, button.y + button.height, buttonOutline, buttonFill);

    // We want the text to be centered inside the panel.
    // The height of the text is easy, it just comes from the constant.
    // The width can be estimated based on character count and average character width.
    const char *buttonText = "Connect";
    float buttonTextWidth = strlen(buttonText) * MENU_BUTTON_TEXT_WIDTH;

    // To center, we figure out how big the enclosing rectangle is,
    // then since we have the text width and are giving the top left position,
    // from which to begin drawing, we can just realize that half the width
    // should be on either side of the center, and so the top left X position
    // is the center minus half the text width:
    float buttonTextX = button.x + (button.width / 2.0f) - (buttonTextWidth / 2.0f);

    // To center vertically, we do the same thing but with height.
    float buttonTextY = button.y + (button.height / 2.0f) + (MENU_BUTTON_TEXT_HEIGHT / 2.0f);

    DrawScreenText(context, buttonText, buttonTextX, buttonTextY, MENU_BUTTON_TEXT_HEIGHT, MENU_BUTTON_TEXT_WIDTH, textColor);

    // Finally, draw the status message below the panel if it exists.
    // We want it horizontally centered below the panel,
    // and far enough below to not interfere with the panel.
    if (state->statusMessage[0] != '\0') {
        float statusTextWidth = strlen(state->statusMessage) * MENU_GENERIC_TEXT_WIDTH;
        float statusTextX = panel.x + (panel.width / 2.0f) - (statusTextWidth / 2.0f);
        float statusTextY = panel.y + panel.height + MENU_GENERIC_TEXT_HEIGHT;
        DrawScreenText(context, state->statusMessage, statusTextX, statusTextY, MENU_GENERIC_TEXT_HEIGHT, MENU_GENERIC_TEXT_WIDTH, labelColor);
    }
}
