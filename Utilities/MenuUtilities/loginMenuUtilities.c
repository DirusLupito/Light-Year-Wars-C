/**
 * Implementation of the login menu utilities used by the client.
 * Provides functions to manage and render the login menu UI.
 * @author abmize
 * @file loginMenuUtilities.c
 */

#include "Utilities/MenuUtilities/loginMenuUtilities.h"

// Specs for the reusable components that make up the login screen.
static const MenuInputFieldSpec kLoginIpFieldSpec = {
    "Server IP",
    "Ex: 127.0.0.1",
    LOGIN_MENU_FIELD_HEIGHT,
    LOGIN_MENU_FIELD_SPACING,
    MENU_INPUT_FIELD_IP
};

static const MenuInputFieldSpec kLoginPortFieldSpec = {
    "Server Port",
    "Ex: 22311",
    LOGIN_MENU_FIELD_HEIGHT,
    LOGIN_MENU_FIELD_SPACING,
    MENU_INPUT_FIELD_INT
};

static const MenuInputFieldSpec kLoginNameFieldSpec = {
    "Player Name",
    "Min 1 - Max 30 characters",
    LOGIN_MENU_FIELD_HEIGHT,
    LOGIN_MENU_FIELD_SPACING,
    MENU_INPUT_FIELD_TEXT
};

static const char *kLoginButtonLabel = "Connect";

/**
 * Initializes component instances so the screen can be composed from primitives.
 * @param state Pointer to the LoginMenuUIState to initialize components for.
 */
static void LoginMenuInitComponents(LoginMenuUIState *state) {
    if (state == NULL) {
        return;
    }

    MenuInputFieldInitialize(&state->ipFieldComponent, &kLoginIpFieldSpec,
        state->ipBuffer, sizeof(state->ipBuffer), &state->ipLength);
    MenuInputFieldInitialize(&state->portFieldComponent, &kLoginPortFieldSpec,
        state->portBuffer, sizeof(state->portBuffer), &state->portLength);
    MenuInputFieldInitialize(&state->nameFieldComponent, &kLoginNameFieldSpec,
        state->nameBuffer, sizeof(state->nameBuffer), &state->nameLength);
    MenuButtonInitialize(&state->connectButtonComponent, kLoginButtonLabel,
        240.0f, LOGIN_MENU_BUTTON_HEIGHT);
}

/**
 * Computes the fixed height of the login menu panel.
 * @return The computed panel height in pixels.
 */
static float LoginMenuPanelHeight(const LoginMenuUIState *state) {
    (void)state;
    // Panel height is padding + field stack + button + padding.
    return LOGIN_MENU_PANEL_PADDING +
        kLoginIpFieldSpec.height + kLoginIpFieldSpec.spacingBelow +
        kLoginPortFieldSpec.height + kLoginPortFieldSpec.spacingBelow +
        kLoginNameFieldSpec.height + kLoginNameFieldSpec.spacingBelow +
        LOGIN_MENU_BUTTON_HEIGHT + LOGIN_MENU_PANEL_PADDING;
}

/**
 * Computes the total content height of the login menu UI.
 * Includes panel height and any additional status message space.
 * @param state Pointer to the LoginMenuUIState to evaluate.
 * @return The computed content height in pixels.
 */
static float LoginMenuContentHeight(const LoginMenuUIState *state) {
    float height = LoginMenuPanelHeight(state);

    // Status text sits below the panel and needs vertical breathing room.
    float statusPadding = MENU_GENERIC_TEXT_HEIGHT;
    if (state != NULL && state->statusMessage[0] != '\0') {
        statusPadding += MENU_GENERIC_TEXT_HEIGHT;
    }
    return height + statusPadding;
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

    float contentHeight = LoginMenuContentHeight(state);
    float clamped = MenuLayoutClampScroll(contentHeight, viewportHeight, state->scrollOffset);

    // Update the state with the clamped value.
    state->scrollOffset = clamped;
    return clamped;
}

/**
 * Computes layout rectangles for the login menu UI elements.
 * @param state Pointer to the LoginMenuUIState.
 * @param width The width of the window.
 * @param height The height of the window.
 * @param scrollOffset Current scroll offset.
 * @param panelOut Optional output for the panel rectangle.
 */
static void LoginMenuLayout(LoginMenuUIState *state,
    int width,
    int height,
    float scrollOffset,
    MenuUIRect *panelOut) {
    if (state == NULL) {
        return;
    }

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

    // Compute total content height.
    float contentHeight = LoginMenuContentHeight(state);

    // Compute positions to center the elements.
    float centerX = w * 0.5f;
    float baseY = MenuLayoutComputeBaseY(contentHeight, h);
    float panelTop = baseY - scrollOffset;
    float innerX = centerX - fieldWidth * 0.5f;
    float cursorY = panelTop + LOGIN_MENU_PANEL_PADDING;

    // Layout the input fields and button.
    MenuInputFieldLayout(&state->ipFieldComponent, innerX, cursorY, fieldWidth);
    cursorY += kLoginIpFieldSpec.height + kLoginIpFieldSpec.spacingBelow;

    MenuInputFieldLayout(&state->portFieldComponent, innerX, cursorY, fieldWidth);
    cursorY += kLoginPortFieldSpec.height + kLoginPortFieldSpec.spacingBelow;

    MenuInputFieldLayout(&state->nameFieldComponent, innerX, cursorY, fieldWidth);
    cursorY += kLoginNameFieldSpec.height + kLoginNameFieldSpec.spacingBelow;

    MenuButtonLayout(&state->connectButtonComponent, innerX, cursorY, fieldWidth);

    if (panelOut != NULL) {
        float panelWidth = fieldWidth + LOGIN_MENU_PANEL_PADDING * 2.0f;
        float panelHeight = LoginMenuPanelHeight(state);
        float panelX = centerX - panelWidth * 0.5f;
        *panelOut = MenuUIRectMake(panelX, panelTop, panelWidth, panelHeight);
    }
}

/**
 * Helper to keep focus state synchronized between the enum and the components.
 * @param state Pointer to the LoginMenuUIState to modify.
 * @param target Desired focus target.
 */
static void LoginMenuSetFocus(LoginMenuUIState *state, LoginMenuFocusTarget target) {
    if (state == NULL) {
        return;
    }

    state->focus = target;
    MenuInputFieldSetFocus(&state->ipFieldComponent, target == LOGIN_MENU_FOCUS_IP);
    MenuInputFieldSetFocus(&state->portFieldComponent, target == LOGIN_MENU_FOCUS_PORT);
    MenuInputFieldSetFocus(&state->nameFieldComponent, target == LOGIN_MENU_FOCUS_NAME);
}

/**
 * Initializes the login menu UI state to default values.
 * @param state Pointer to the LoginMenuUIState to initialize.
 */
void LoginMenuUIInitialize(LoginMenuUIState *state) {
    if (state == NULL) {
        return;
    }

    // Zero out the entire structure.
    memset(state, 0, sizeof(*state));

    // Initialize components and set default focus.
    LoginMenuInitComponents(state);
    LoginMenuSetFocus(state, LOGIN_MENU_FOCUS_NONE);
    state->connectButtonComponent.enabled = true;

    // Prepare default status message.
    LoginMenuUISetStatusMessage(state, "Enter the server IP, port, and your username, then click Connect.");
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
    LoginMenuLayout(state, width, height, scrollOffset, &panel);

    // Now that we know the element positions,
    // we can update focus and button states
    // based on where the mouse was clicked.
    
    if (!MenuUIRectContains(&panel, x, y)) {
        LoginMenuSetFocus(state, LOGIN_MENU_FOCUS_NONE);
        state->connectButtonPressed = false;
        state->connectButtonComponent.pressed = false;
        return;
    }

    if (MenuUIRectContains(&state->ipFieldComponent.rect, x, y)) {
        LoginMenuSetFocus(state, LOGIN_MENU_FOCUS_IP);
    } else if (MenuUIRectContains(&state->portFieldComponent.rect, x, y)) {
        LoginMenuSetFocus(state, LOGIN_MENU_FOCUS_PORT);
    } else if (MenuUIRectContains(&state->nameFieldComponent.rect, x, y)) {
        LoginMenuSetFocus(state, LOGIN_MENU_FOCUS_NAME);
    } else {
        LoginMenuSetFocus(state, LOGIN_MENU_FOCUS_NONE);
    }

    state->connectButtonPressed = MenuButtonHandleMouseDown(&state->connectButtonComponent, x, y);
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
    LoginMenuLayout(state, width, height, scrollOffset, NULL);

    // If the button was released over the button, we consider it activated.

    bool activated = false;
    MenuButtonHandleMouseUp(&state->connectButtonComponent, x, y, &activated);
    state->connectButtonPressed = state->connectButtonComponent.pressed;

    if (activated) {
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

    // We only append to the field that is currently focused.
    // For the IP field, we allow digits and dots.
    // For the port field, we allow only digits.
    if (state->focus == LOGIN_MENU_FOCUS_IP) {
        MenuInputFieldHandleChar(&state->ipFieldComponent, codepoint);
    } else if (state->focus == LOGIN_MENU_FOCUS_PORT) {
        MenuInputFieldHandleChar(&state->portFieldComponent, codepoint);
    } else if (state->focus == LOGIN_MENU_FOCUS_NAME) {
        MenuInputFieldHandleChar(&state->nameFieldComponent, codepoint);
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
            if (state->focus == LOGIN_MENU_FOCUS_IP) {
                MenuInputFieldHandleBackspace(&state->ipFieldComponent);
            } else if (state->focus == LOGIN_MENU_FOCUS_PORT) {
                MenuInputFieldHandleBackspace(&state->portFieldComponent);
            } else if (state->focus == LOGIN_MENU_FOCUS_NAME) {
                MenuInputFieldHandleBackspace(&state->nameFieldComponent);
            }
            break;

        // Tab switches focus between fields.
        case VK_TAB:
            if (state->focus == LOGIN_MENU_FOCUS_IP) {
                LoginMenuSetFocus(state, shiftDown ? LOGIN_MENU_FOCUS_NONE : LOGIN_MENU_FOCUS_PORT);
            } else if (state->focus == LOGIN_MENU_FOCUS_PORT) {
                LoginMenuSetFocus(state, shiftDown ? LOGIN_MENU_FOCUS_IP : LOGIN_MENU_FOCUS_NAME);
            } else if (state->focus == LOGIN_MENU_FOCUS_NAME) {
                LoginMenuSetFocus(state, shiftDown ? LOGIN_MENU_FOCUS_PORT : LOGIN_MENU_FOCUS_NONE);
            } else {
                LoginMenuSetFocus(state, shiftDown ? LOGIN_MENU_FOCUS_NAME : LOGIN_MENU_FOCUS_IP);
            }
            break;

        // Enter/return requests a connection if a field is focused.
        case VK_RETURN:
            if (state->focus == LOGIN_MENU_FOCUS_IP || state->focus == LOGIN_MENU_FOCUS_PORT || state->focus == LOGIN_MENU_FOCUS_NAME) {
                state->connectRequested = true;
            }
            break;
        // Escape clears focus.
        case VK_ESCAPE:
            LoginMenuSetFocus(state, LOGIN_MENU_FOCUS_NONE);
            break;

        default:
            // Other keys are not handled here.
            break;
    }
}

/**
 * Consumes a connect request from the login menu UI, if one exists.
 * Copies the IP address, port, and player name to the provided output buffers.
 * @param state Pointer to the LoginMenuUIState.
 * @param ipOut Output buffer for the IP address.
 * @param ipCapacity Capacity of the IP address output buffer.
 * @param portOut Output buffer for the port.
 * @param portCapacity Capacity of the port output buffer.
 * @return True if a connect request was consumed, false otherwise.
 */
bool LoginMenuUIConsumeConnectRequest(LoginMenuUIState *state,
    char *ipOut,
    size_t ipCapacity,
    char *portOut,
    size_t portCapacity,
    char *nameOut,
    size_t nameCapacity) {

    // If state or output buffers are null, we can't proceed.
    if (state == NULL || ipOut == NULL || portOut == NULL || nameOut == NULL) {
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

    if (nameCapacity > 0) {
        strncpy(nameOut, state->nameBuffer, nameCapacity - 1);
        nameOut[nameCapacity - 1] = '\0';
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
    LoginMenuLayout(state, width, height, scrollOffset, &panel);

    // Draw the panel background as a semi-transparent, outlined rectangle.

    // Panel colors

    // Slightly light outline, dark fill.
    float panelOutline[4] = MENU_PANEL_OUTLINE_COLOR;
    float panelFill[4] = MENU_PANEL_FILL_COLOR;
    DrawOutlinedRectangle(panel.x, panel.y, panel.x + panel.width, panel.y + panel.height, panelOutline, panelFill);

    // Draw the input fields and the connect button.
    // Also draw labels and current contents.
    const float labelColor[4] = MENU_LABEL_TEXT_COLOR;
    const float textColor[4] = MENU_INPUT_TEXT_COLOR;
    const float placeholderColor[4] = MENU_PLACEHOLDER_TEXT_COLOR;

    // IP field
    MenuInputFieldDraw(&state->ipFieldComponent, context, labelColor, textColor, placeholderColor);

    // Port field
    MenuInputFieldDraw(&state->portFieldComponent, context, labelColor, textColor, placeholderColor);
    
    // Player Name field
    MenuInputFieldDraw(&state->nameFieldComponent, context, labelColor, textColor, placeholderColor);

    // Connect button
    MenuButtonDraw(&state->connectButtonComponent, context, state->mouseX, state->mouseY, state->connectButtonComponent.enabled);

    // Finally, draw the status message below the panel if it exists.
    // We want it horizontally centered below the panel,
    // and far enough below to not interfere with the panel.
    if (state->statusMessage[0] != '\0') {
        float statusTextWidth = (float)strlen(state->statusMessage) * MENU_GENERIC_TEXT_WIDTH;
        float statusTextX = panel.x + (panel.width * 0.5f) - (statusTextWidth * 0.5f);
        float statusTextY = panel.y + panel.height + MENU_GENERIC_TEXT_HEIGHT;
        DrawScreenText(context, state->statusMessage, statusTextX, statusTextY, MENU_GENERIC_TEXT_HEIGHT, MENU_GENERIC_TEXT_WIDTH, labelColor);
    }
}
