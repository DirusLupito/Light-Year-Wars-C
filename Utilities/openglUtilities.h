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

// Structure to hold OpenGL context information for a window.
// We need to keep track of the device context and rendering context,
// as well as any shared resources like font display lists.
// This structure also has the current width and height of the rendering area.
typedef struct OpenGLContext {
    HDC deviceContext;
    HGLRC renderContext;
    GLuint fontDisplayListBase;
    int width;
    int height;
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


#endif /* _OPENGL_UTILITIES_H_ */
