/**
 * Header file for various menu UI related utilities.
 * Used by the Light Year Wars program to help render and manage
 * menu UI elements such as text input fields and buttons.
 * @author abmize
 * @file menuUtilities.h
 */

#ifndef _MENU_UTILITIES_H_
#define _MENU_UTILITIES_H_

#include <stddef.h>
#include <stdbool.h>
#include <windows.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "Utilities/renderUtilities.h"

// Maximum lengths for the menu input fields (excluding null terminator).

// Maximum length for the server IP address input field.
#define MENU_IP_MAX_LENGTH 63

// Maximum length for the server port input field.
// Biggest valid port is 65535, which is 5 digits.
#define MENU_PORT_MAX_LENGTH 5

// Maximum length for the status message.
#define MENU_STATUS_MAX_LENGTH 127

// UI layout constants

// Width of the entire menu panel.
#define MENU_FIELD_HEIGHT 44.0f

// Spacing between input fields.
#define MENU_FIELD_SPACING 28.0f

// Height of the Connect button.
#define MENU_BUTTON_HEIGHT 48.0f

// Padding around the menu panel.
#define MENU_PANEL_PADDING 32.0f

// Data structures

// Defines a rectangular area for a UI element.
// Used for hit testing and layout.
// x and y define the top-left corner.
typedef struct MenuUIRect {
    float x;
    float y;
    float width;
    float height;
} MenuUIRect;

// Defines the possible focus targets for the menu UI.
// MENU_FOCUS_NONE means no field is focused.
// MENU_FOCUS_IP means the IP address field is focused.
// MENU_FOCUS_PORT means the port field is focused.
// Used to track which input field is currently active
// for the sake of keyboard input and visual highlighting.
typedef enum MenuFocusTarget {
    MENU_FOCUS_NONE = 0,
    MENU_FOCUS_IP,
    MENU_FOCUS_PORT
} MenuFocusTarget;

// Defines the state of the menu UI.
// Tracks input field contents, focus state, mouse position,
// button states, and status messages.
typedef struct MenuUIState {
    char ipBuffer[MENU_IP_MAX_LENGTH + 1];
    size_t ipLength;
    char portBuffer[MENU_PORT_MAX_LENGTH + 1];
    size_t portLength;
    MenuFocusTarget focus;
    bool connectRequested;
    bool connectButtonPressed;
    float mouseX;
    float mouseY;
    char statusMessage[MENU_STATUS_MAX_LENGTH + 1];
} MenuUIState;

// Function declarations

/**
 * Helper function to trim leading and trailing whitespace
 * from a given string in place.
 * @param text The string to trim. Must be null-terminated.
 */
void TrimWhitespace(char *text);

/**
 * Initializes the menu UI state to default values.
 * @param state Pointer to the MenuUIState to initialize.
 */
void MenuUIInitialize(MenuUIState *state);

/**
 * Handles mouse movement events for the menu UI.
 * Updates the mouse position in the state.
 * @param state Pointer to the MenuUIState.
 * @param x The new mouse X position.
 * @param y The new mouse Y position.
 */
void MenuUIHandleMouseMove(MenuUIState *state, float x, float y);

/**
 * Handles mouse button down events for the menu UI.
 * Updates focus state and button press state based on mouse position.
 * @param state Pointer to the MenuUIState.
 * @param x The mouse X position at the time of the event.
 * @param y The mouse Y position at the time of the event.
 * @param width The width of the window.
 * @param height The height of the window.
 */
void MenuUIHandleMouseDown(MenuUIState *state, float x, float y, int width, int height);

/**
 * Handles mouse button up events for the menu UI.
 * Updates button press state and triggers connect requests if applicable.
 * @param state Pointer to the MenuUIState.
 * @param x The mouse X position at the time of the event.
 * @param y The mouse Y position at the time of the event.
 * @param width The width of the window.
 * @param height The height of the window.
 */
void MenuUIHandleMouseUp(MenuUIState *state, float x, float y, int width, int height);

/**
 * Handles character input events for the menu UI.
 * Appends valid characters to the focused input field.
 * @param state Pointer to the MenuUIState.
 * @param codepoint The Unicode codepoint of the character input.
 */
void MenuUIHandleChar(MenuUIState *state, unsigned int codepoint);

/**
 * Handles key down events for the menu UI.
 * Manages special keys like backspace, tab, enter, and escape.
 * @param state Pointer to the MenuUIState.
 * @param key The virtual key code of the key that was pressed.
 * @param shiftDown True if the Shift key is currently held down.
 */
void MenuUIHandleKeyDown(MenuUIState *state, WPARAM key, bool shiftDown);

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
bool MenuUIConsumeConnectRequest(MenuUIState *state, char *ipOut, size_t ipCapacity, char *portOut, size_t portCapacity);

/**
 * Sets the status message to be displayed in the menu UI.
 * @param state Pointer to the MenuUIState.
 * @param message The status message to set. If NULL, clears the message.
 */
void MenuUISetStatusMessage(MenuUIState *state, const char *message);

/**
 * Draws the menu UI elements using the provided OpenGL context.
 * @param state Pointer to the MenuUIState.
 * @param context Pointer to the OpenGLContext for rendering.
 * @param width The width of the window.
 * @param height The height of the window.
 */
void MenuUIDraw(MenuUIState *state, OpenGLContext *context, int width, int height);

#endif // _MENU_UTILITIES_H_
