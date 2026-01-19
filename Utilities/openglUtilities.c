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

    // Fill out the OpenGLContext structure for the caller
    context->deviceContext = deviceContext;
    context->renderContext = renderContext;
    context->width = width;
    context->height = height;
    context->fontEntryCount = 0;
    ZeroMemory(context->fontEntries, sizeof(context->fontEntries));

    // Set the initial projection matrix based on the window size
    if (width > 0 && height > 0) {
        OpenGLUpdateProjection(context, width, height);
    }

    // Preload a default font used by most UI text.
    OpenGLAcquireFont(context, -16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Segoe UI");

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

        // Delete any font display lists created for this context.
        for (size_t i = 0; i < context->fontEntryCount; ++i) {
            if (context->fontEntries[i].listBase != 0) {
                glDeleteLists(context->fontEntries[i].listBase, 96);
                context->fontEntries[i].listBase = 0;
            }
        }
        context->fontEntryCount = 0;
        ZeroMemory(context->fontEntries, sizeof(context->fontEntries));

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

/**
 * Compares two LOGFONTA structures for equality.
 * For now, two LOGFONTA structures are considered equal
 * if and only if all their fields are identical.
 * @param lhs Pointer to the first LOGFONTA structure.
 * @param rhs Pointer to the second LOGFONTA structure.
 * @return true if the two LOGFONTA structures are equal, false otherwise.
 */
static bool OpenGLFontsEqual(const LOGFONTA *lhs, const LOGFONTA *rhs) {
    return memcmp(lhs, rhs, sizeof(LOGFONTA)) == 0;
}

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
    uint8_t clipPrecision, uint8_t quality, uint8_t pitchAndFamily, const char *faceName) {

    // Basic validation of input pointers.
    if (context == NULL || context->deviceContext == NULL) {
        return 0;
    }

    // Taken from the Windows API documentation for LOGFONTA:
    // typedef struct tagLOGFONTA {
    //   LONG lfHeight;
    //   LONG lfWidth;
    //   LONG lfEscapement;
    //   LONG lfOrientation;
    //   LONG lfWeight;
    //   BYTE lfItalic;
    //   BYTE lfUnderline;
    //   BYTE lfStrikeOut;
    //   BYTE lfCharSet;
    //   BYTE lfOutPrecision;
    //   BYTE lfClipPrecision;
    //   BYTE lfQuality;
    //   BYTE lfPitchAndFamily;
    //   CHAR lfFaceName[LF_FACESIZE];
    // } LOGFONTA, *PLOGFONTA, *NPLOGFONTA, *LPLOGFONTA;
    //
    // According to the win32 documentation at 
    // https://learn.microsoft.com/en-us/windows/win32/winprog/windows-data-types:
    // LONG is a signed 32-bit integer.
    // BYTE is an unsigned 8-bit integer.
    // CHAR is an 8-bit signed integer.

    // Structure to hold the desired font properties.
    LOGFONTA desiredFont;

    // Zero out any uninitialized fields.
    ZeroMemory(&desiredFont, sizeof(desiredFont));

    // Then we fill out the fields based on the input parameters.
    desiredFont.lfHeight = (LONG)height;
    desiredFont.lfWidth = (LONG)width;
    desiredFont.lfEscapement = (LONG)escapement;
    desiredFont.lfOrientation = (LONG)orientation;
    desiredFont.lfWeight = (LONG)weight;
    desiredFont.lfItalic = (BYTE)italic;
    desiredFont.lfUnderline = (BYTE)underline;
    desiredFont.lfStrikeOut = (BYTE)strikeOut;
    desiredFont.lfCharSet = (BYTE)charSet;
    desiredFont.lfOutPrecision = (BYTE)outputPrecision;
    desiredFont.lfClipPrecision = (BYTE)clipPrecision;
    desiredFont.lfQuality = (BYTE)quality;
    desiredFont.lfPitchAndFamily = (BYTE)pitchAndFamily;
    if (faceName != NULL) {
        strncpy(desiredFont.lfFaceName, faceName, LF_FACESIZE - 1);
        desiredFont.lfFaceName[LF_FACESIZE - 1] = '\0';
    }

    // Now we check if a matching font already exists in the context.
    for (size_t i = 0; i < context->fontEntryCount; ++i) {
        if (OpenGLFontsEqual(&context->fontEntries[i].logFont, &desiredFont)) {
            // If such a font exists, return its display list base.
            return context->fontEntries[i].listBase;
        }
    }

    // Otherwise, we need to create a new font.

    // But to create a new font, we first need to check if we have space
    // in the fontEntries array.
    if (context->fontEntryCount >= OPENGL_MAX_FONT_ENTRIES) {
        return 0;
    }

    // Now that we know we have enough space, we can create the font.

    // CreateFontIndirectA creates a logical font with the specified characteristics.
    // Note that at this point, this is a logical font, not yet associated with any device context.
    HFONT font = CreateFontIndirectA(&desiredFont);
    if (font == NULL) {
        return 0;
    }

    // The logical font still needs a few things done to it before we can use it for OpenGL rendering.
    // Such as...

    // ... generating a set of display lists for the font characters.
    // This display list base will be returned to the caller.
    // The values 32 to 127 (96 characters) correspond to the printable ASCII characters.
    GLuint listBase = glGenLists(96);
    if (listBase == 0) {
        DeleteObject(font);
        return 0;
    }

    // The font still needs...

    // ... to be selected into the device context.
    // This makes the font the current font for the device context,
    // allowing us to create bitmaps for the font characters, 
    // bitmaps which... 

    // From https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-selectobject,
    // the arguments for SelectObject are:
    // HDC hdc: Handle to the device context.
    // HGDIOBJ hgdiobj: Handle to the object to be selected (in this case, the font).
    // Since we are selecting a new font, SelectObject returns the handle to the previous font,
    // which we need to save so we can restore it later.
    
    HFONT oldFont = (HFONT)SelectObject(context->deviceContext, font);

    // ... wglUseFontBitmapsA creates here. These bitmaps are stored in the generated display lists,
    // and are used by OpenGL to render text. By taking something like a simple char, or a string,
    // and calling the appropriate display list, OpenGL can render the corresponding character(s)
    // for those characters to the screen by utilizing those characters as the index into the display list.

    // From https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-wglusefontbitmapsa,
    // the arguments for wglUseFontBitmapsA are:
    // HDC hdc: Handle to the device context.
    // DWORD first: The first character to create a bitmap for (32 = space character).
    // DWORD count: The number of characters to create bitmaps for (96 = characters 32 to 127).
    // DWORD listBase: The starting index of the display lists to store the bitmaps in.
    if (!wglUseFontBitmapsA(context->deviceContext, 32, 96, listBase)) {
        glDeleteLists(listBase, 96);
        listBase = 0;
    }

    // Now that we have created the font bitmaps, we can restore the previous font
    // in the device context, and delete the logical font object, as it is no longer needed.
    SelectObject(context->deviceContext, oldFont);
    DeleteObject(font);

    // If something went wrong during wglUseFontBitmapsA, we clean up and return 0.
    if (listBase == 0) {
        return 0;
    }

    // Finally, we store the new font entry in the context's fontEntries array.
    context->fontEntries[context->fontEntryCount].logFont = desiredFont;
    context->fontEntries[context->fontEntryCount].listBase = listBase;

    // And increment the font entry count.
    context->fontEntryCount++;

    return listBase;
}
