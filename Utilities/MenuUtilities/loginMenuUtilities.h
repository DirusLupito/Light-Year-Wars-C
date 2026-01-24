/**
 * Header for login menu specific utilities.
 * Provides state structures and helpers used by the client login screen.
 * @author abmize
 * @file loginMenuUtilities.h
 */
#ifndef _LOGIN_MENU_UTILITIES_H_
#define _LOGIN_MENU_UTILITIES_H_

#include "Objects/player.h"
#include <stddef.h>
#include <stdbool.h>
#include <windows.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include "Utilities/renderUtilities.h"
#include "Utilities/MenuUtilities/commonMenuUtilities.h"
#include "Utilities/MenuUtilities/menuComponentUtilities.h"

// Maximum length for the server IP address input field.
// We only support IPv4 addresses for now, 
// so 15 characters (excluding null terminator) is sufficient.
#define LOGIN_MENU_IP_MAX_LENGTH 15

// Maximum length for the server port input field.
// Biggest valid port is 65535, which is 5 digits.
#define LOGIN_MENU_PORT_MAX_LENGTH 5

// Maximum length for the status message.
#define LOGIN_MENU_STATUS_MAX_LENGTH 127

// Maximum length for the player name field on the login screen.
#define LOGIN_MENU_NAME_MAX_LENGTH PLAYER_NAME_MAX_LENGTH

// Width of the entire menu panel.
#define LOGIN_MENU_FIELD_HEIGHT 44.0f

// Spacing between input fields.
#define LOGIN_MENU_FIELD_SPACING 28.0f

// Height of the Connect button.
#define LOGIN_MENU_BUTTON_HEIGHT 48.0f

// Padding around the menu panel.
#define LOGIN_MENU_PANEL_PADDING 32.0f

// Defines the possible focus targets for the login menu UI.
// LOGIN_MENU_FOCUS_NONE means no field is focused.
// LOGIN_MENU_FOCUS_IP means the IP address field is focused.
// LOGIN_MENU_FOCUS_PORT means the port field is focused.
// Used to track which input field is currently active
// for the sake of keyboard input and visual highlighting.
typedef enum LoginMenuFocusTarget {
    LOGIN_MENU_FOCUS_NONE = 0,
    LOGIN_MENU_FOCUS_IP,
    LOGIN_MENU_FOCUS_PORT,
    LOGIN_MENU_FOCUS_NAME
} LoginMenuFocusTarget;

// Defines the state of the log in menu UI.
// Tracks input field contents, focus state, mouse position,
// button states, and status messages.
typedef struct LoginMenuUIState {
    char ipBuffer[LOGIN_MENU_IP_MAX_LENGTH + 1];
    size_t ipLength;
    char portBuffer[LOGIN_MENU_PORT_MAX_LENGTH + 1];
    size_t portLength;
    char nameBuffer[LOGIN_MENU_NAME_MAX_LENGTH + 1];
    size_t nameLength;
    LoginMenuFocusTarget focus;
    bool connectRequested;
    bool connectButtonPressed;
    float mouseX;
    float mouseY;
    float scrollOffset; /* Vertical scroll position for overflowing content. */

    /* Componentized UI elements for reuse across screens. */
    MenuInputFieldComponent ipFieldComponent;
    MenuInputFieldComponent portFieldComponent;
    MenuInputFieldComponent nameFieldComponent;
    MenuButtonComponent connectButtonComponent;
    char statusMessage[LOGIN_MENU_STATUS_MAX_LENGTH + 1];
} LoginMenuUIState;

/**
 * Initializes the login menu UI state to default values.
 * @param state Pointer to the LoginMenuUIState to initialize.
 */
void LoginMenuUIInitialize(LoginMenuUIState *state);

/**
 * Handles mouse movement events for the login menu UI.
 * Updates the mouse position in the state.
 * @param state Pointer to the LoginMenuUIState.
 * @param x The new mouse X position.
 * @param y The new mouse Y position.
 */
void LoginMenuUIHandleMouseMove(LoginMenuUIState *state, float x, float y);

/**
 * Handles mouse button down events for the login menu UI.
 * Updates focus state and button press state based on mouse position.
 * @param state Pointer to the LoginMenuUIState.
 * @param x The mouse X position at the time of the event.
 * @param y The mouse Y position at the time of the event.
 * @param width The width of the window.
 * @param height The height of the window.
 */
void LoginMenuUIHandleMouseDown(LoginMenuUIState *state, float x, float y, int width, int height);

/**
 * Handles mouse button up events for the login menu UI.
 * Updates button press state and triggers connect requests if applicable.
 * @param state Pointer to the LoginMenuUIState.
 * @param x The mouse X position at the time of the event.
 * @param y The mouse Y position at the time of the event.
 * @param width The width of the window.
 * @param height The height of the window.
 */
void LoginMenuUIHandleMouseUp(LoginMenuUIState *state, float x, float y, int width, int height);

/**
 * Handles character input events for the login menu UI.
 * Appends valid characters to the focused input field.
 * @param state Pointer to the LoginMenuUIState.
 * @param codepoint The Unicode codepoint of the character input.
 */
void LoginMenuUIHandleChar(LoginMenuUIState *state, unsigned int codepoint);

/**
 * Handles key down events for the login menu UI.
 * Manages special keys like backspace, tab, enter, and escape.
 * @param state Pointer to the LoginMenuUIState.
 * @param key The virtual key code of the key that was pressed.
 * @param shiftDown True if the Shift key is currently held down.
 */
void LoginMenuUIHandleKeyDown(LoginMenuUIState *state, WPARAM key, bool shiftDown);

/**
 * Consumes a connect request from the  login menu UI, if one exists.
 * Copies the IP address and port to the provided output buffers.
 * @param state Pointer to the LoginMenuUIState.
 * @param ipOut Output buffer for the IP address.
 * @param ipCapacity Capacity of the IP address output buffer.
 * @param portOut Output buffer for the port.
 * @param portCapacity Capacity of the port output buffer.
 * @param nameOut Output buffer for the player name.
 * @param nameCapacity Capacity of the player name output buffer.
 * @return True if a connect request was consumed, false otherwise.
 */
bool LoginMenuUIConsumeConnectRequest(LoginMenuUIState *state, char *ipOut, size_t ipCapacity, char *portOut, size_t portCapacity, char *nameOut, size_t nameCapacity);

/**
 * Sets the status message to be displayed in the login menu UI.
 * @param state Pointer to the LoginMenuUIState.
 * @param message The status message to set. If NULL, clears the message.
 */
void LoginMenuUISetStatusMessage(LoginMenuUIState *state, const char *message);

/**
 * Draws the login menu UI elements using the provided OpenGL context.
 * @param state Pointer to the LoginMenuUIState.
 * @param context Pointer to the OpenGLContext for rendering.
 * @param width The width of the window.
 * @param height The height of the window.
 */
void LoginMenuUIDraw(LoginMenuUIState *state, OpenGLContext *context, int width, int height);

/**
 * Adjusts the login menu scroll position in response to mouse wheel input.
 * Positive wheel steps scroll the content upward (toward earlier items).
 * @param state Pointer to the LoginMenuUIState.
 * @param width The width of the window.
 * @param height The height of the window.
 * @param wheelSteps Wheel delta expressed in multiples of WHEEL_DELTA (120).
 */
void LoginMenuUIHandleScroll(LoginMenuUIState *state, int height, float wheelSteps);

#endif // _LOGIN_MENU_UTILITIES_H_
