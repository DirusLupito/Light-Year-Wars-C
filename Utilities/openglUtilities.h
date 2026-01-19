/**
 * Header file for OpenGL utility functions,
 * which holds helper functions for initializing and 
 * managing shared OpenGL objects.
 * @author abmize
 * @file openglUtilities.h
 */
#ifndef _OPENGL_UTILITIES_H_
#define _OPENGL_UTILITIES_H_

#include <windows.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Structure to hold OpenGL font entry information.
// We keep track of the LOGFONT used to create the font
// and the base display list ID for rendering characters.
// The LOGFONTA structure defines the attributes of a font,
// such as height, width, weight, italicization, and typeface name
// among other properties.
/// The listBase is the starting index of a contiguous block of OpenGL display lists
/// that represent the glyphs for this font.
typedef struct OpenGLFontEntry {
    LOGFONTA logFont;
    GLuint listBase;
} OpenGLFontEntry;

// Maximum number of font entries we can store in the OpenGL context.
// For a small little game like this, 64 different fonts should be more than enough.
// But in a larger game or some sort of messaging application where you want to 
// support a wide variety of fonts, rather than have some structure with some
// preset limit on it's number of supported fonts, you might want to use a dynamic data structure
// like a linked list or a resizable array, or some other approach in which
// the number of fonts is only limited by available memory.
#define OPENGL_MAX_FONT_ENTRIES 64

// Structure to hold OpenGL context information for a window.
// We need to keep track of the device context and rendering context,
// as well as any shared resources like font display lists.
// This structure also has the current width and height of the rendering area.
// The fontEntryCount keeps track of how many font entries are currently stored
// in the fontEntries array.
typedef struct OpenGLContext {
    HDC deviceContext;
    HGLRC renderContext;
    int width;
    int height;
    size_t fontEntryCount;
    OpenGLFontEntry fontEntries[OPENGL_MAX_FONT_ENTRIES];
} OpenGLContext;

/**
 * Initializes OpenGL for a given window, and fills the OpenGLContext structure,
 * used by the caller to manage OpenGL resources.
 * @param context Pointer to the OpenGLContext structure to initialize.
 * @param windowHandle Handle to the window for which to initialize OpenGL.
 * @return true if initialization was successful, false otherwise.
 */
bool OpenGLInitializeForWindow(OpenGLContext *context, HWND windowHandle);

/**
 * Shuts down OpenGL for a given window, releasing all associated resources.
 * @param context Pointer to the OpenGLContext structure to shut down.
 * @param windowHandle Handle to the window for which to shut down OpenGL.
 */
void OpenGLShutdownForWindow(OpenGLContext *context, HWND windowHandle);

/**
 * Updates the OpenGL projection matrix to match the given width and height.
 * This should be called whenever the window is resized.
 * @param context Pointer to the OpenGLContext structure to update.
 * @param width The new width of the window.
 * @param height The new height of the window.
 */
void OpenGLUpdateProjection(OpenGLContext *context, int width, int height);

/**
 * Acquires an OpenGL font display list for the specified font properties.
 * If a matching font already exists in the context, its display list base is returned.
 * Otherwise, a new font is created and its display list base is returned.
 * Fails if the maximum number of font entries has been reached, or if there is some other error.
 * See https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-logfonta for
 * details on the LOGFONTA structure and its fields.
 * This approach allows for easy access to a multitude of bitmap fonts for OpenGL text rendering,
 * without needing to recreate fonts that have already been created.
 * From https://learnopengl.com/In-Practice/Text-Rendering:
 * 
 * This approach has several advantages and disadvantages. 
 * It is relatively easy to implement and because bitmap fonts are pre-rasterized, 
 * they're quite efficient. However, it is not particularly flexible. 
 * When you want to use a different font, you need to recompile a complete new bitmap font 
 * and the system is limited to a single resolution; zooming will quickly show pixelated edges. 
 * Furthermore, it is limited to a small character set, 
 * so Extended or Unicode characters are often out of the question. 
 * 
 * @param context Pointer to the OpenGLContext structure.
 * @param height Height of the font.
 * @param width Width of the font.
 * @param escapement Angle of escapement.
 * @param orientation Orientation angle.
 * @param weight Font weight.
 * @param italic Italic attribute option.
 * @param underline Underline attribute option.
 * @param strikeOut Strikeout attribute option.
 * @param charSet Character set identifier.
 * @param outputPrecision Output precision.
 * @param clipPrecision Clipping precision.
 * @param quality Output quality.
 * @param pitchAndFamily Family and pitch of the font.
 * @param faceName Typeface name.
 * @return The base display list ID for the acquired font, or 0 on failure.
 */
GLuint OpenGLAcquireFont(OpenGLContext *context, uint32_t height, uint32_t width, uint32_t escapement, uint32_t orientation,
    uint32_t weight, uint8_t italic, uint8_t underline, uint8_t strikeOut, uint8_t charSet, uint8_t outputPrecision,
    uint8_t clipPrecision, uint8_t quality, uint8_t pitchAndFamily, const char *faceName);

#endif /* _OPENGL_UTILITIES_H_ */
