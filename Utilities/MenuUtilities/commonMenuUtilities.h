/**
 * Header file for various menu UI related utilities.
 * Used by the Light Year Wars program to help render and manage
 * menu UI elements such as text input fields and buttons.
 * @author abmize
 * @file commonMenuUtilities.h
 */

#ifndef _COMMON_MENU_UTILITIES_H_
#define _COMMON_MENU_UTILITIES_H_

#include <stdbool.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

// Height of label text for input fields.
#define MENU_LABEL_TEXT_HEIGHT 20.0f

// Width of label text for input fields.
#define MENU_LABEL_TEXT_WIDTH 10.0f

// Height of text within input fields.
#define MENU_INPUT_TEXT_HEIGHT 22.0f

// Width of text within input fields.
#define MENU_INPUT_TEXT_WIDTH 11.0f

// Height of text within buttons.
#define MENU_BUTTON_TEXT_HEIGHT 24.0f

// Width of text within buttons.
#define MENU_BUTTON_TEXT_WIDTH 12.0f

// Height of generic text
#define MENU_GENERIC_TEXT_HEIGHT 18.0f

// Width of generic text
#define MENU_GENERIC_TEXT_WIDTH 9.0f

// Log in panel outline color
#define MENU_PANEL_OUTLINE_COLOR {0.9f, 0.9f, 0.9f, 0.45f}

// log in panel fill color
#define MENU_PANEL_FILL_COLOR {0.1f, 0.1f, 0.12f, 0.75f}

// Label text color
#define MENU_LABEL_TEXT_COLOR {0.95f, 0.95f, 0.95f, 1.0f}

// Input text color
#define MENU_INPUT_TEXT_COLOR {0.98f, 0.98f, 0.98f, 1.0f}

// Placeholder text color
#define MENU_PLACEHOLDER_TEXT_COLOR {0.7f, 0.72f, 0.76f, 1.0f}

// Default outline color of input box when not focused
#define MENU_INPUT_BOX_OUTLINE_COLOR {0.45f, 0.7f, 1.0f, 0.6f}

// Alpha value of input box outline when focused
#define MENU_INPUT_BOX_FOCUSED_ALPHA 0.95f

// Fill color of input box
#define MENU_INPUT_BOX_FILL_COLOR {0.08f, 0.1f, 0.18f, 0.85f}

// Outline color of button
#define MENU_BUTTON_OUTLINE_COLOR {0.45f, 0.7f, 1.0f, 0.75f}

// Fill color of button
#define MENU_BUTTON_FILL_COLOR {0.12f, 0.2f, 0.35f, 0.9f}

// Fill color of hovered button
#define MENU_BUTTON_HOVER_FILL_COLOR {0.16f, 0.28f, 0.44f, 0.9f}

// Represents a rectangular area for UI layout and hit testing.
// Coordinates use a top-left origin with positive width and height.
typedef struct MenuUIRect {
    float x;
    float y;
    float width;
    float height;
} MenuUIRect;

/**
 * Helper function to trim leading and trailing whitespace
 * from a given string in place.
 * @param text The string to trim. Must be null-terminated.
 */
void TrimWhitespace(char *text);

/**
 * Helper function to create a MenuUIRect
 * given position and size.
 * @param x The X position of the rectangle.
 * @param y The Y position of the rectangle.
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 * @return The constructed MenuUIRect.
 */
MenuUIRect MenuUIRectMake(float x, float y, float width, float height);

/**
 * Helper function to determine if a point is within a MenuUIRect.
 * @param rect Pointer to the MenuUIRect.
 * @param x The X coordinate of the point.
 * @param y The Y coordinate of the point.
 * @return True if the point is within the rectangle, false otherwise.
 */
bool MenuUIRectContains(const MenuUIRect *rect, float x, float y);

#endif // _COMMON_MENU_UTILITIES_H_
