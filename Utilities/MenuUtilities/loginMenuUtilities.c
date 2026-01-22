/**
 * Implementation of the login menu utilities used by the client.
 * Provides functions to manage and render the login menu UI.
 * @author abmize
 * @file loginMenuUtilities.c
 */

#include "Utilities/MenuUtilities/loginMenuUtilities.h"

/**
 * Computes the fixed height of the login menu panel.
 * @return The computed panel height in pixels.
 */
static float LoginMenuPanelHeight(void) {
    // The login menu is just two fields plus button plus padding.
    return 2.0f * (LOGIN_MENU_FIELD_HEIGHT + LOGIN_MENU_FIELD_SPACING) +
        LOGIN_MENU_BUTTON_HEIGHT + 2.0f * LOGIN_MENU_PANEL_PADDING;
}

/**
 * Computes the total content height of the login menu UI.
 * Includes panel height and any additional status message space.
 * @param state Pointer to the LoginMenuUIState to evaluate.
 * @return The computed content height in pixels.
 */
static float LoginMenuContentHeight(const LoginMenuUIState *state) {
    // Get the panel height first.
    float height = LoginMenuPanelHeight();

    // Then add the padding for status message if present.
    float statusPadding = MENU_GENERIC_TEXT_HEIGHT;
    if (state != NULL && state->statusMessage[0] != '\0') {
        statusPadding += MENU_GENERIC_TEXT_HEIGHT;
    }
    return height + statusPadding;
}

/**
 * Computes the base Y position for the login menu panel
 * to center it vertically within the viewport if possible.
 * @param contentHeight The total content height of the login menu UI.
 * @param viewportHeight The height of the viewport.
 * @return The computed base Y position in pixels.
 */
static float LoginMenuBaseY(float contentHeight, float viewportHeight) {
    // If viewport height is non-positive, just return a default offset.
    if (viewportHeight <= 0.0f) {
        return 16.0f;
    }

    // If content fits within the viewport, center it vertically.
    if (contentHeight <= viewportHeight) {
        float centered = (viewportHeight - contentHeight) * 0.5f;
        // Clamp to the default minimum offset.
        if (centered < 16.0f) {
            centered = 16.0f;
        }
        return centered;
    }

    return 16.0f;
}

/**
 * Clamps the scroll offset of the login menu UI state
 * to ensure it stays within valid bounds.
 * @param state Pointer to the LoginMenuUIState to modify.
 * @param viewportHeight The height of the viewport area.
 * @return The clamped scroll offset in pixels.
 */
static float LoginMenuClampScroll(LoginMenuUIState *state, float viewportHeight) {
    // Nothing to clamp
    if (state == NULL) {
        return 0.0f;
    }

    // The maximum scroll offset is content height minus viewport height
    // (ensuring we don't scroll past the bottom).
    float maxScroll = LoginMenuContentHeight(state) - viewportHeight;
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
 * Helper function to compute the layout of the login menu UI elements
 * based on the current window size. Will seek to center the elements
 * and size them appropriately.
 * @param width The width of the window.
 * @param height The height of the window.
 * @param panel Output parameter for the panel rectangle.
 * @param ipField Output parameter for the IP field rectangle.
 * @param portField Output parameter for the port field rectangle.
 * @param button Output parameter for the connect button rectangle.
 */
static void LoginMenuUIComputeLayout(const LoginMenuUIState *state,
    int width,
    int height,
    float scrollOffset,
    MenuUIRect *panel,
    MenuUIRect *ipField,
    MenuUIRect *portField,
    MenuUIRect *button) {
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
    float fieldWidth = fminf(420.0f, w - 2.0f * LOGIN_MENU_PANEL_PADDING);

    if (fieldWidth < 220.0f) {
        fieldWidth = 220.0f;
    }
    
    // Compute button width based on field width.
    float buttonWidth = fminf(fieldWidth, 240.0f);

    // Compute total content height.
    float contentHeight = LoginMenuContentHeight(state);

    // Compute positions to center the elements.
    float centerX = w * 0.5f;
    float baseY = LoginMenuBaseY(contentHeight, h);
    float panelTop = baseY - scrollOffset;
    float startY = panelTop + LOGIN_MENU_PANEL_PADDING;

    // Output the rectangles if requested.
    if (ipField != NULL) {
        float x = centerX - fieldWidth * 0.5f;
        *ipField = MenuUIRectMake(x, startY, fieldWidth, LOGIN_MENU_FIELD_HEIGHT);
    }

    if (portField != NULL) {
        float x = centerX - fieldWidth * 0.5f;
        float y = startY + LOGIN_MENU_FIELD_HEIGHT + LOGIN_MENU_FIELD_SPACING;
        *portField = MenuUIRectMake(x, y, fieldWidth, LOGIN_MENU_FIELD_HEIGHT);
    }

    if (button != NULL) {
        float x = centerX - buttonWidth * 0.5f;
        float y = startY + 2.0f * (LOGIN_MENU_FIELD_HEIGHT + LOGIN_MENU_FIELD_SPACING);
        *button = MenuUIRectMake(x, y, buttonWidth, LOGIN_MENU_BUTTON_HEIGHT);
    }

    // Finally, output the panel rectangle if requested.
    if (panel != NULL) {
        float top = startY - LOGIN_MENU_PANEL_PADDING;
        float bottom = startY + 2.0f * (LOGIN_MENU_FIELD_HEIGHT + LOGIN_MENU_FIELD_SPACING) + LOGIN_MENU_BUTTON_HEIGHT + LOGIN_MENU_PANEL_PADDING;
        float panelHeight = bottom - top;
        float panelWidth = fieldWidth + LOGIN_MENU_PANEL_PADDING * 2.0f;
        float x = centerX - panelWidth * 0.5f;
        *panel = MenuUIRectMake(x, top, panelWidth, panelHeight);
    }
}

/**
 * Helper function to backspace (remove last character)
 * from a specified input field in the login menu UI state.
 * @param state Pointer to the LoginMenuUIState.
 * @param target The focus target indicating which field to backspace.
 */
static void LoginMenuUIBackspaceField(LoginMenuUIState *state, LoginMenuFocusTarget target) {
    // Nothing to backspace if state is null.
    if (state == NULL) {
        return;
    }

    // Backspace the appropriate field
    // depending on the target.
    if (target == LOGIN_MENU_FOCUS_IP) {
        if (state->ipLength > 0) {
            state->ipLength--;
            state->ipBuffer[state->ipLength] = '\0';
        }
    } else if (target == LOGIN_MENU_FOCUS_PORT) {
        if (state->portLength > 0) {
            state->portLength--;
            state->portBuffer[state->portLength] = '\0';
        }
    }
}

/**
 * Helper function to append a character
 * to a specified input field in the login menu UI state.
 * @param state Pointer to the LoginMenuUIState.
 * @param target The focus target indicating which field to append to.
 * @param value The character to append.
 */
static void LoginMenuUIAppendToField(LoginMenuUIState *state, LoginMenuFocusTarget target, char value) {
    // Nothing to append to if state is null.
    if (state == NULL) {
        return;
    }

    // Append to the appropriate field.
    if (target == LOGIN_MENU_FOCUS_IP) {
        if (state->ipLength >= LOGIN_MENU_IP_MAX_LENGTH) {
            return;
        }
        state->ipBuffer[state->ipLength++] = value;
        state->ipBuffer[state->ipLength] = '\0';
    } else if (target == LOGIN_MENU_FOCUS_PORT) {
        if (state->portLength >= LOGIN_MENU_PORT_MAX_LENGTH) {
            return;
        }
        state->portBuffer[state->portLength++] = value;
        state->portBuffer[state->portLength] = '\0';
    }
}

/**
 * Initializes the login menu UI state to default values.
 * @param state Pointer to the LoginMenuUIState to initialize.
 */
void LoginMenuUIInitialize(LoginMenuUIState *state) {
    // If state is null, we don't have a good pointer to initialize.
    if (state == NULL) {
        return;
    }

    // Zero out the entire structure.
    memset(state, 0, sizeof(*state));

    // Set initial focus.
    state->focus = LOGIN_MENU_FOCUS_NONE;

    // Prepare default status message.
    LoginMenuUISetStatusMessage(state, "Enter the server IP and port, then click Connect.");
}

/**
 * Handles mouse movement events for the login menu UI.
 * Updates the mouse position in the state.
 * @param state Pointer to the LoginMenuUIState.
 * @param x The new mouse X position.
 * @param y The new mouse Y position.
 */
void LoginMenuUIHandleMouseMove(LoginMenuUIState *state, float x, float y) {
    // If state is null, we can't update anything.
    if (state == NULL) {
        return;
    }

    state->mouseX = x;
    state->mouseY = y;
}

/**
 * Handles mouse button down events for the login menu UI.
 * Updates focus state and button press state based on mouse position.
 * @param state Pointer to the LoginMenuUIState.
 * @param x The mouse X position at the time of the event.
 * @param y The mouse Y position at the time of the event.
 * @param width The width of the window.
 * @param height The height of the window.
 */
void LoginMenuUIHandleMouseDown(LoginMenuUIState *state, float x, float y, int width, int height) {
    // If state is null, we can't update anything.
    if (state == NULL) {
        return;
    }

    state->mouseX = x;
    state->mouseY = y;

    // Ensure scroll offset is within the proper bounds.
    float scrollOffset = LoginMenuClampScroll(state, (float)height);

    // Compute the layout to determine element positions.
    MenuUIRect panel;
    MenuUIRect ipField;
    MenuUIRect portField;
    MenuUIRect button;
    LoginMenuUIComputeLayout(state, width, height, scrollOffset, &panel, &ipField, &portField, &button);

    // Now that we know the element positions,
    // we can update focus and button states
    // based on where the mouse was clicked.

    if (MenuUIRectContains(&ipField, x, y)) {
        state->focus = LOGIN_MENU_FOCUS_IP;
    } else if (MenuUIRectContains(&portField, x, y)) {
        state->focus = LOGIN_MENU_FOCUS_PORT;
    } else if (!MenuUIRectContains(&panel, x, y)) {
        state->focus = LOGIN_MENU_FOCUS_NONE;
    }

    if (MenuUIRectContains(&button, x, y)) {
        state->connectButtonPressed = true;
    } else {
        state->connectButtonPressed = false;
    }
}

/**
 * Handles mouse button up events for the login menu UI.
 * Updates button press state and triggers connect requests if applicable.
 * @param state Pointer to the LoginMenuUIState.
 * @param x The mouse X position at the time of the event.
 * @param y The mouse Y position at the time of the event.
 * @param width The width of the window.
 * @param height The height of the window.
 */
void LoginMenuUIHandleMouseUp(LoginMenuUIState *state, float x, float y, int width, int height) {
    // If state is null, we can't update anything.
    if (state == NULL) {
        return;
    }

    state->mouseX = x;
    state->mouseY = y;

    // Ensure scroll offset is within the proper bounds.
    float scrollOffset = LoginMenuClampScroll(state, (float)height);

    // Only the button matters on mouse up.
    MenuUIRect button;
    LoginMenuUIComputeLayout(state, width, height, scrollOffset, NULL, NULL, NULL, &button);

    // If the button was pressed and the mouse is still over it,
    // we register a connect request.
    bool wasPressed = state->connectButtonPressed;
    state->connectButtonPressed = false;

    if (wasPressed && MenuUIRectContains(&button, x, y)) {
        state->connectRequested = true;
    }
}

/**
 * Adjusts the login menu scroll position in response to mouse wheel input.
 * Positive wheel steps scroll the content upward (toward earlier items).
 * @param state Pointer to the LoginMenuUIState.
 * @param height The height of the window.
 * @param wheelSteps Wheel delta expressed in multiples of WHEEL_DELTA (120).
 */
void LoginMenuUIHandleScroll(LoginMenuUIState *state, int height, float wheelSteps) {
    // If state is null, we can't update anything.
    if (state == NULL || height <= 0 || wheelSteps == 0.0f) {
        return;
    }

    // Adjust scroll offset based on wheel steps,
    // then ensure it's clamped within valid bounds.
    state->scrollOffset -= wheelSteps * SCROLL_PIXELS_PER_WHEEL;
    LoginMenuClampScroll(state, (float)height);
}

/**
 * Handles character input events for the login menu UI.
 * Appends valid characters to the focused input field.
 * @param state Pointer to the LoginMenuUIState.
 * @param codepoint The Unicode codepoint of the character input.
 */
void LoginMenuUIHandleChar(LoginMenuUIState *state, unsigned int codepoint) {
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
    if (state->focus == LOGIN_MENU_FOCUS_IP) {
        if (isdigit((unsigned char)value) || value == '.') {
            LoginMenuUIAppendToField(state, LOGIN_MENU_FOCUS_IP, value);
        }
    } else if (state->focus == LOGIN_MENU_FOCUS_PORT) {
        if (isdigit((unsigned char)value)) {
            LoginMenuUIAppendToField(state, LOGIN_MENU_FOCUS_PORT, value);
        }
    }
}

/**
 * Handles key down events for the login menu UI.
 * Manages special keys like backspace, tab, enter, and escape.
 * @param state Pointer to the LoginMenuUIState.
 * @param key The virtual key code of the key that was pressed.
 * @param shiftDown True if the Shift key is currently held down.
 */
void LoginMenuUIHandleKeyDown(LoginMenuUIState *state, WPARAM key, bool shiftDown) {
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
            LoginMenuUIBackspaceField(state, state->focus);
            break;

        // Tab switches focus between fields.
        case VK_TAB:
            if (state->focus == LOGIN_MENU_FOCUS_IP) {
                state->focus = shiftDown ? LOGIN_MENU_FOCUS_NONE : LOGIN_MENU_FOCUS_PORT;
            } else if (state->focus == LOGIN_MENU_FOCUS_PORT) {
                state->focus = shiftDown ? LOGIN_MENU_FOCUS_IP : LOGIN_MENU_FOCUS_NONE;
            } else {
                state->focus = shiftDown ? LOGIN_MENU_FOCUS_PORT : LOGIN_MENU_FOCUS_IP;
            }
            break;

        // Enter/return requests a connection if a field is focused.
        case VK_RETURN:
            if (state->focus == LOGIN_MENU_FOCUS_IP || state->focus == LOGIN_MENU_FOCUS_PORT) {
                state->connectRequested = true;
            }
            break;
        // Escape clears focus.
        case VK_ESCAPE:
            state->focus = LOGIN_MENU_FOCUS_NONE;
            break;
        default:
            // Other keys are not handled here.
            break;
    }
}

/**
 * Consumes a connect request from the  login menu UI, if one exists.
 * Copies the IP address and port to the provided output buffers.
 * @param state Pointer to the LoginMenuUIState.
 * @param ipOut Output buffer for the IP address.
 * @param ipCapacity Capacity of the IP address output buffer.
 * @param portOut Output buffer for the port.
 * @param portCapacity Capacity of the port output buffer.
 * @return True if a connect request was consumed, false otherwise.
 */
bool LoginMenuUIConsumeConnectRequest(LoginMenuUIState *state, char *ipOut, size_t ipCapacity, char *portOut, size_t portCapacity) {
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
 * Sets the status message to be displayed in the login menu UI.
 * @param state Pointer to the LoginMenuUIState.
 * @param message The status message to set. If NULL, clears the message.
 */
void LoginMenuUISetStatusMessage(LoginMenuUIState *state, const char *message){
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
    strncpy(state->statusMessage, message, LOGIN_MENU_STATUS_MAX_LENGTH);
    state->statusMessage[LOGIN_MENU_STATUS_MAX_LENGTH] = '\0';
}

/**
 * Draws the login menu UI elements using the provided OpenGL context.
 * @param state Pointer to the LoginMenuUIState.
 * @param context Pointer to the OpenGLContext for rendering.
 * @param width The width of the window.
 * @param height The height of the window.
 */
void LoginMenuUIDraw(LoginMenuUIState *state, OpenGLContext *context, int width, int height) {
    // If state or context is null, or dimensions are invalid, we can't draw anything.
    if (state == NULL || context == NULL || width <= 0 || height <= 0) {
        return;
    }

    // Ensure scroll offset is within the proper bounds.
    float scrollOffset = LoginMenuClampScroll(state, (float)height);

    // We retrieve rectangles for all UI elements.
    MenuUIRect panel;
    MenuUIRect ipField;
    MenuUIRect portField;
    MenuUIRect button;
    LoginMenuUIComputeLayout(state, width, height, scrollOffset, &panel, &ipField, &portField, &button);

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
    if (state->focus == LOGIN_MENU_FOCUS_IP) {
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
    if (state->focus == LOGIN_MENU_FOCUS_PORT) {
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
