/**
 * Implementation file for various menu UI related utilities.
 * Used by the Light Year Wars program to help render and manage
 * menu UI elements such as text input fields and buttons.
 * @author abmize
 * @file commonMenuUtilities.c
 */

#include "Utilities/MenuUtilities/commonMenuUtilities.h"

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
MenuUIRect MenuUIRectMake(float x, float y, float width, float height) {
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
bool MenuUIRectContains(const MenuUIRect *rect, float x, float y) {
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
