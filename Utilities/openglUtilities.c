/**
 * Implements OpenGL utility functions for initializing, shutting down,
 * and updating the OpenGL context for a given window.
 * @author abmize
 * @file openglUtilities.c
 */
#include "Utilities/openglUtilities.h"

/**
 * Initializes OpenGL for a given window, and fills the OpenGLContext structure,
 * used by the caller to manage OpenGL resources.
 * @param context Pointer to the OpenGLContext structure to initialize.
 * @param windowHandle Handle to the window for which to initialize OpenGL.
 * @return true if initialization was successful, false otherwise.
 */
bool OpenGLInitializeForWindow(OpenGLContext *context, HWND windowHandle) {
    // Basic validation of input pointers.
    if (context == NULL || windowHandle == NULL) {
        return false;
    }

    // A PIXELFORMATDESCRIPTOR describes the pixel format of a drawing surface.
    // It specifies properties such as color depth, buffer types, and other attributes.
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), // Size of this structure
        1, // Version number
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // Flags (draw to window, support OpenGL, double buffering)
        PFD_TYPE_RGBA, // The kind of framebuffer. RGBA or palette.
        32, // Color depth in bits
        0, 0, 0, 0, 0, 0, // From left to right: Red bits, Red shift, Green bits, Green shift, Blue bits, Blue shift
        0, // Alpha bits
        0, // Alpha shift
        0, // Accumulation buffer bits
        0, 0, 0, 0, // From left to right: Accumulation red bits, green bits, blue bits, alpha bits
        24, // Depth buffer bits
        8, // Stencil buffer bits
        0, // Auxiliary buffer bits
        PFD_MAIN_PLANE, // Layer type (main drawing layer)
        0, // Reserved
        0, 0, 0 // Layer masks (ignored for main plane)
    };

    // Get the device context for the window
    // This essentially prepares the window for OpenGL rendering
    // by obtaining a handle to its device context.
    // A device context is a Windows structure that defines a set of graphic objects
    // and their associated attributes, as well as the graphic modes that affect output.
    HDC deviceContext = GetDC(windowHandle);

    // Check if device context retrieval was successful
    if (deviceContext == NULL) {
        printf("GetDC failed.\n");
        return false;
    }

    // A pixel format defines how pixels are stored and represented in a drawing surface.
    // This can be things like color depth, buffer types, and other attributes.

    // Choose a pixel format that matches the desired properties
    int pixelFormat = ChoosePixelFormat(deviceContext, &pfd);

    // Check if pixel format selection was successful
    if (pixelFormat == 0) {
        printf("ChoosePixelFormat failed.\n");
        ReleaseDC(windowHandle, deviceContext);
        return false;
    }

    // Set the chosen pixel format for the device context
    if (!SetPixelFormat(deviceContext, pixelFormat, &pfd)) {
        printf("SetPixelFormat failed.\n");
        ReleaseDC(windowHandle, deviceContext);
        return false;
    }

    // The rendering context is an OpenGL structure that holds all of OpenGL's state information
    // for drawing to a device context. It is required for any OpenGL rendering to occur.
    HGLRC renderContext = wglCreateContext(deviceContext);
    if (renderContext == NULL) {
        printf("wglCreateContext failed.\n");
        ReleaseDC(windowHandle, deviceContext);
        return false;
    }

    // Make the rendering context current.
    // Were it not made current, OpenGL commands would not affect this context.
    if (!wglMakeCurrent(deviceContext, renderContext)) {
        printf("wglMakeCurrent failed.\n");
        wglDeleteContext(renderContext);
        ReleaseDC(windowHandle, deviceContext);
        return false;
    }

    // Set up some basic OpenGL state
    // By default, OpenGL enables depth testing and flat shading.
    // For this application, we want to disable depth testing
    // and use smooth shading for better visual quality.
    glDisable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    // Enable blending for transparent colors and anti-aliasing
    // These may be reset by the caller, especially in the various render utility functions.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Enable line smoothing for anti-aliased lines.
    // Likewise, the caller may reset this state as needed.
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    // Set the initial viewport and projection matrix
    // This ensures that the OpenGL rendering area matches the window size.
    RECT clientRect;
    int width = 0;
    int height = 0;
    if (GetClientRect(windowHandle, &clientRect)) {
        width = clientRect.right - clientRect.left;
        height = clientRect.bottom - clientRect.top;
    }

    // OpenGL font display list base.
    // This is used for rendering text.
    // If for example, this equals 1000, then the display list for the character 'A' (ASCII 65) would be 1065.
    // The display list is a compiled set of OpenGL commands that can be executed more efficiently.
    // This is useful for rendering text because each character can be drawn with a single call to the display list.
    GLuint fontBase = 0;

    // Create a font for rendering text in OpenGL
    HFONT font = CreateFontA(-16, // Height of font. This is negative to indicate character height rather than cell height
        0,  // Width of font. 0 means default width based on height
        0,  // Angle of escapement. Escapement is the angle between the baseline of a character and the x-axis of the device.
        0,  // Orientation angle. This is the angle between the baseline of a character and the x-axis of the device.
        FW_NORMAL,  // Font weight. FW_NORMAL is normal weight.
        FALSE,  // Italic attribute option
        FALSE,  // Underline attribute option
        FALSE,  // Strikeout attribute option
        ANSI_CHARSET,  // Character set identifier
        OUT_TT_PRECIS,  // Output precision. TT means TrueType.
        CLIP_DEFAULT_PRECIS,  // Clipping precision
        ANTIALIASED_QUALITY,  // Output quality
        FF_DONTCARE | DEFAULT_PITCH, // Family and pitch of the font (don't care about family, default pitch)
        "Segoe UI"); // Typeface name

    // If font creation was successful, create display lists.
    if (font != NULL) {

        // Generate display lists for 96 characters (from ASCII 32 to 127)
        fontBase = glGenLists(96);

        // If display list generation was successful, create the font bitmaps
        if (fontBase != 0) {

            // HFONT is a handle to a font object
            // Here we select the created font into the device context
            // so that we can create bitmaps for the font characters.
            HFONT oldFont = (HFONT)SelectObject(deviceContext, font);

            // wglUseFontBitmapsA creates a set of bitmap display lists for the glyphs
            // of the currently selected font in the specified device context.
            if (!wglUseFontBitmapsA(deviceContext, 32, 96, fontBase)) {
                // If wglUseFontBitmapsA fails, we clean up the generated display lists.
                glDeleteLists(fontBase, 96);
                fontBase = 0;
            }

            // Restore the previous font in the device context.
            SelectObject(deviceContext, oldFont);
        }

        // We can delete the font object now, as the display lists have been created.
        DeleteObject(font);
    }


    // Fill out the OpenGLContext structure for the caller
    context->deviceContext = deviceContext;
    context->renderContext = renderContext;
    context->fontDisplayListBase = fontBase;
    context->width = width;
    context->height = height;

    // Set the initial projection matrix based on the window size
    if (width > 0 && height > 0) {
        OpenGLUpdateProjection(context, width, height);
    }

    return true;
}


/**
 * Shuts down OpenGL for a given window, releasing all associated resources.
 * @param context Pointer to the OpenGLContext structure to shut down.
 * @param windowHandle Handle to the window for which to shut down OpenGL.
 */
void OpenGLShutdownForWindow(OpenGLContext *context, HWND windowHandle) {
    // Basic validation of input pointers.
    if (context == NULL) {
        return;
    }

    // Delete the OpenGL rendering context and release the device context
    if (context->renderContext != NULL) {

        // If we created display lists for the font, delete them
        if (context->fontDisplayListBase != 0) {
            glDeleteLists(context->fontDisplayListBase, 96);
            context->fontDisplayListBase = 0;
        }

        // Turn off the current rendering context
        // wglMakeCurrent(NULL, NULL) disassociates the current rendering context from the device context.
        // This is important to do before deleting the rendering context itself.
        wglMakeCurrent(NULL, NULL);

        // Here, we actually delete the rendering context
        wglDeleteContext(context->renderContext);
        context->renderContext = NULL;
    }

    // Release the device context for the window
    if (context->deviceContext != NULL) {

        // ReleaseDC releases the device context previously obtained by GetDC.
        ReleaseDC(windowHandle, context->deviceContext);
        context->deviceContext = NULL;
    }

    // Clear out the rest of the context structure
    context->width = 0;
    context->height = 0;
}


/**
 * Updates the OpenGL projection matrix to match the given width and height.
 * This should be called whenever the window is resized.
 * @param context Pointer to the OpenGLContext structure to update.
 * @param width The new width of the window.
 * @param height The new height of the window.
 */
void OpenGLUpdateProjection(OpenGLContext *context, int width, int height) {
    // If the rendering context is not initialized, we cannot update the projection
    if (context == NULL || context->renderContext == NULL || width <= 0 || height <= 0) {
        return;
    }

    // glViewport sets the viewport, which is the rectangular area of the window where OpenGL will draw.
    // It's parameters are from left to right:
    // x-axis position of the lower left corner of the viewport rectangle, in pixels.
    // y-axis position of the lower left corner of the viewport rectangle, in pixels.
    // width of the viewport.
    // height of the viewport.
    glViewport(0, 0, width, height);

    // Set up an orthographic projection matrix
    // This defines a 2D projection where objects are the same size regardless of their depth.
    glMatrixMode(GL_PROJECTION);

    // glLoadIdentity resets the current matrix to the identity matrix.
    // If we don't do this, the new projection matrix will be multiplied with the existing one,
    // leading to incorrect transformations.
    glLoadIdentity();

    // glOrtho defines a 2D orthographic projection matrix.
    // The parameters specify the clipping volume:
    // left, right, bottom, top, near, far
    glOrtho(0.0, (GLdouble)width, (GLdouble)height, 0.0, -1.0, 1.0);

    // Switch back to the modelview matrix mode
    // This is the matrix mode we will use for rendering objects.
    glMatrixMode(GL_MODELVIEW);

    // Update the stored width and height in the context
    context->width = width;
    context->height = height;
}
