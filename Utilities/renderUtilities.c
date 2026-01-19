/**
 * Contains common utility functions for rendering objects using OpenGL 
 * for the Light Year Wars game.
 * @file Utilities/renderUtilities.c
 * @author abmize
 */
#include "Utilities/renderUtilities.h"

/**
 * Draws a hollow circle using OpenGL.
 * This method essentially approximates a circle by drawing a polygon with many sides.
 * @param cx The x-coordinate of the center of the circle.
 * @param cy The y-coordinate of the center of the circle.
 * @param radius The radius of the circle.
 * @param segments The number of segments to use for drawing the circle.
 * @param thickness The thickness of the circle outline.
 */
void DrawCircle(float cx, float cy, float radius, int segments, float thickness) {

    // Set the line width for drawing the circle
    glLineWidth(thickness);

    // Begin drawing a line loop
    // An OpenGL line loop connects a series of vertices with lines,
    // and then connects the last vertex back to the first.
    glBegin(GL_LINE_LOOP);

    // Calculate and specify each vertex of the circle
    for (int i = 0; i < segments; ++i) {

        // Angle of the line segment
        float angle = (float)i / (float)segments * 2.0f * (float)M_PI;

        // Calculate the x and y coordinates of the vertex
        // which lies on the circumference of the circle
        float x = cx + cosf(angle) * radius;
        float y = cy + sinf(angle) * radius;

        // Specify the vertex to OpenGL
        // This adds the vertex to the current drawing primitive (the line loop)
        glVertex2f(x, y);
    }

    // End the line loop
    glEnd();

    // Reset the line width to default (1.0f)
    glLineWidth(1.0f);
}

/**
 * Draws a filled circle using OpenGL.
 * @param cx The x-coordinate of the center of the circle.
 * @param cy The y-coordinate of the center of the circle.
 * @param radius The radius of the circle.
 * @param segments The number of segments to use for drawing the circle.
 */
void DrawFilledCircle(float cx, float cy, float radius, int segments) {
    if (radius <= 0.0f || segments < 3) {
        return;
    }

    // A triangle fan is a series of connected triangles
    // that share a common central vertex.
    // This is ideal for drawing filled circles,
    // as we can use the center of the circle as the common vertex,
    // and then create triangles that extend out to the circumference.
    // Each subtriangle is formed by the center vertex and two consecutive vertices on the circle's edge.
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / (float)segments * 2.0f * (float)M_PI;
        float x = cx + cosf(angle) * radius;
        float y = cy + sinf(angle) * radius;
        glVertex2f(x, y);
    }
    glEnd();

    // The line width does not need to be reset here since we did not change it.
}

/**
 * Draws a ring (hollow circle with thickness) using OpenGL.
 * Compared to DrawCircle, this fills the area between innerRadius and outerRadius,
 * rather than just drawing an outline of specified thickness. Thereby ensuring that the outer edge
 * is easy to line up with an existing object, like a planet's outer radius.
 * Were this done with DrawCircle, the outer edge could be slightly off since line thickness could
 * cause the outermost edge to extend beyond the intended radius.
 * @param cx The x-coordinate of the center of the ring.
 * @param cy The y-coordinate of the center of the ring.
 * @param innerRadius The inner radius of the ring.
 * @param outerRadius The outer radius of the ring.
 * @param segments The number of segments to use for drawing the ring.
 */
void DrawRing(float cx, float cy, float innerRadius, float outerRadius, int segments) {
    if (outerRadius <= 0.0f || innerRadius < 0.0f || innerRadius >= outerRadius || segments < 3) {
        return;
    }

    // We use a triangle strip to draw the ring.
    // A triangle strip is a series of connected triangles
    // where each triangle shares vertices with the previous one.
    // This is efficient for drawing shapes like rings,
    // as we can define the outer and inner edges of the ring
    // and let OpenGL connect them with triangles.
    // Each pair of vertices defines a quad segment of the ring,
    // which is made up of two triangles.
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / (float)segments * 2.0f * (float)M_PI;
        float cosAngle = cosf(angle);
        float sinAngle = sinf(angle);

        float outerX = cx + cosAngle * outerRadius;
        float outerY = cy + sinAngle * outerRadius;
        float innerX = cx + cosAngle * innerRadius;
        float innerY = cy + sinAngle * innerRadius;

        glVertex2f(outerX, outerY);
        glVertex2f(innerX, innerY);
    }
    glEnd();

    // The line width does not need to be reset here since we did not change it.
}

/**
 * Draws a ring with softened edges by feathering both the inner and outer boundaries.
 * @param cx The x-coordinate of the center of the ring.
 * @param cy The y-coordinate of the center of the ring.
 * @param innerRadius Radius where the ring begins (fully transparent inside this radius).
 * @param outerRadius Radius where the ring ends (fully transparent beyond this radius).
 * @param segments Number of segments used to approximate the ring.
 * @param featherWidth Width of the fade zone applied to each edge.
 * @param color RGBA color applied to the solid portion of the ring.
 */
void DrawFeatheredRing(float cx, float cy, float innerRadius, float outerRadius, float featherWidth, const float color[4]) {
    // Basic validation of parameters.
    if (outerRadius <= 0.0f || innerRadius < 0.0f || innerRadius >= outerRadius || color == NULL) {
        return;
    }

    // Get the number of segments to use for drawing the ring based on its size.
    int segments = ComputeCircleSegments(outerRadius);
    if (segments <= 0) {
        return;
    }

    // If no feathering is requested, draw a standard ring.
    float alpha = color[3];
    if (featherWidth <= 0.0f || alpha <= 0.0f) {
        glColor4fv(color);
        DrawRing(cx, cy, innerRadius, outerRadius, segments);
        return;
    }

    // Feathering is essentially a gradient transition from opaque to transparent
    // at both the inner and outer edges of the ring.
    // The way it works is that we define two fade zones:
    // 1. Inner fade zone: from innerRadius to innerRadius + featherWidth
    // 2. Outer fade zone: from outerRadius - featherWidth to outerRadius
    // Within these zones, we interpolate the alpha value from 0 to the specified color alpha.
    // This creates a smooth transition effect.

    // Otherwise, we proceed with feathering.

    // Clamp the feather width to avoid excessive feathering.
    float ringWidth = outerRadius - innerRadius;
    float clampedFeather = fminf(featherWidth, ringWidth * 0.5f);

    // Calculate the inner and outer fade boundaries.
    float innerFadeStart = innerRadius;
    float innerFadeEnd = innerRadius + clampedFeather;
    float outerFadeStart = outerRadius - clampedFeather;
    float outerFadeEnd = outerRadius;

    // Enable blending for smooth transparency transitions.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw the inner feathered section
    float opaqueColor[4] = {color[0], color[1], color[2], alpha};
    float transparentColor[4] = {color[0], color[1], color[2], 0.0f};

    // If there is space between innerFadeStart and innerFadeEnd, draw that gradient
    if (innerFadeEnd > innerFadeStart) {
        DrawRadialGradientRing(cx, cy, innerFadeStart, innerFadeEnd, segments, transparentColor, opaqueColor);
    }

    // If there is a solid section between innerFadeEnd and outerFadeStart, draw that
    if (outerFadeEnd > outerFadeStart) {
        DrawRadialGradientRing(cx, cy, outerFadeStart, outerFadeEnd, segments, opaqueColor, transparentColor);
    }

    // If there is a solid section between innerFadeEnd and outerFadeStart, draw that
    if (outerFadeStart > innerFadeEnd) {
        glColor4fv(color);
        DrawRing(cx, cy, innerFadeEnd, outerFadeStart, segments);
    }

    // Cleanup: disable blending to restore OpenGL state.
    glDisable(GL_BLEND);
}

/**
 * Draws a radial gradient ring (or disc) using OpenGL.
 * The gradient smoothly transitions from the inner color at innerRadius
 * to the outer color at outerRadius around the given center point.
 * If innerRadius is zero, the gradient collapses to a solid disc fan.
 * @param cx The x-coordinate of the gradient's center.
 * @param cy The y-coordinate of the gradient's center.
 * @param innerRadius Radius at which the inner color is applied.
 * @param outerRadius Radius at which the outer color is applied.
 * @param segments Number of segments used to approximate the gradient circle.
 * @param innerColor RGBA color applied along the inner radius.
 * @param outerColor RGBA color applied along the outer radius.
 */
void DrawRadialGradientRing(float cx, float cy, float innerRadius, float outerRadius, int segments,
    const float innerColor[4], const float outerColor[4]) {
    if (outerRadius <= 0.0f || innerRadius < 0.0f || innerRadius > outerRadius || segments < 3) {
        return;
    }

    // Render a triangle strip that alternates between the outer and inner radii.
    // Each segment contributes two vertices: one on the outer ring with the outer color,
    // and one on the inner ring with the inner color. This produces a smooth radial gradient.
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= segments; ++i) {
        // Solve for the angle around the circle
        float angle = (float)i / (float)segments * 2.0f * (float)M_PI;
        float cosAngle = cosf(angle);
        float sinAngle = sinf(angle);

        // Now we find the outer vertex position and color
        float outerX = cx + cosAngle * outerRadius;
        float outerY = cy + sinAngle * outerRadius;
        glColor4fv(outerColor);
        glVertex2f(outerX, outerY);

        // And find the inner vertex position and color
        float innerX = cx + cosAngle * innerRadius;
        float innerY = cy + sinAngle * innerRadius;
        glColor4fv(innerColor);
        glVertex2f(innerX, innerY);
    }
    glEnd();
}

/**
 * Computes the number of segments to use for drawing a circle of the given radius.
 * This helps ensure that circles are drawn smoothly regardless of their size.
 * Specifically, this formula aims for approximately 1.5 units of arc length per segment.
 * @param radius The radius of the circle.
 * @return The computed number of segments for the circle.
 */
int ComputeCircleSegments(float radius) {
    // We use the absolute value of the radius to avoid negative values.
    float absRadius = fabsf(radius);
    if (absRadius <= 0.0f) {
        // And if the absolute radius is zero or somehow negative, 
        // we cannot draw a circle.
        return 0;
    }

    // Calculate circumference = 2 * pi * r
    float circumference = absRadius * 2.0f * (float)M_PI;

    // And then determine segments such that each segment covers about 1.5 units of arc length.
    int segments = (int)ceilf(circumference / 1.5f);

    // Clamp segments to a reasonable range to avoid too few or too many segments.
    if (segments < MIN_CIRCLE_SEGMENTS) {
        segments = MIN_CIRCLE_SEGMENTS;
    } else if (segments > MAX_CIRCLE_SEGMENTS) {
        segments = MAX_CIRCLE_SEGMENTS;
    }

    return segments;
}

/**
 * Draws a filled circle with feathered edges.
 * The feathering creates a smooth transition from the circle's color to transparency.
 * @param cx The x-coordinate of the circle's center.
 * @param cy The y-coordinate of the circle's center.
 * @param radius The radius of the circle.
 * @param featherWidth The width of the feathering effect at the edge of the circle.
 * @param color The RGBA color of the circle.
 */
void DrawFeatheredFilledInCircle(float cx, float cy, float radius, float featherWidth, const float color[4]) {
    // Basic validation of parameters.
    if (color == NULL || radius <= 0.0f) {
        return;
    }

    // Get the number of segments to use for drawing the circle based on its size.
    int segments = ComputeCircleSegments(radius);
    if (segments <= 0) {
        return;
    }

    // Feathering here works by drawing a solid inner circle
    // and then overlaying a radial gradient ring that fades to transparency
    // over the specified feather width.

    // Clamp the feather width to avoid excessive feathering.
    float clampedFeather = featherWidth;
    if (clampedFeather < 0.0f) {
        clampedFeather = 0.0f;
    }
    if (clampedFeather > radius) {
        clampedFeather = radius;
    }

    // Draw the solid inner circle (non-feathered part).
    float innerRadius = radius - clampedFeather;
    if (innerRadius > 0.0f) {
        glColor4fv(color);
        DrawFilledCircle(cx, cy, innerRadius, segments);
    } else {
        innerRadius = 0.0f;
    }

    // If no feathering is requested, we are done.
    if (clampedFeather <= 0.0f) {
        if (innerRadius <= 0.0f) {
            glColor4fv(color);
            DrawFilledCircle(cx, cy, radius, segments);
        }
        return;
    }

    // Otherwise, draw the feathered edge as a radial gradient ring.
    float innerColor[4] = {color[0], color[1], color[2], color[3]};
    float outerColor[4] = {color[0], color[1], color[2], 0.0f};
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    DrawRadialGradientRing(cx, cy, innerRadius, radius, segments, innerColor, outerColor);
    glDisable(GL_BLEND);
}

/**
 * Draws a radial background gradient.
 * @param width The width of the area to draw the gradient in.
 * @param height The height of the area to draw the gradient in.
 */
void DrawBackgroundGradient(int width, int height) {
    // Basic validation of input dimensions.
    if (width <= 0 || height <= 0) {
        return;
    }

    // Calculate center and radius for the gradient
    float centerX = (float)width * 0.5f;
    float centerY = (float)height * 0.5f;
    float halfWidth = (float)width * 0.5f;
    float halfHeight = (float)height * 0.5f;
    float radius = sqrtf(halfWidth * halfWidth + halfHeight * halfHeight) * 1.05f;

    // Enable blending for smooth transparency transitions.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    DrawRadialGradientRing(centerX, centerY, 0.0f, radius, 128,
        (float[4])BACKGROUND_GRADIENT_INNER_COLOR, (float[4])BACKGROUND_GRADIENT_OUTER_COLOR);

    // Disable blending after drawing the gradient.
    glDisable(GL_BLEND);
}

/**
 * Helper function to draw an outlined rectangle using
 * the given outline and fill colors at the specified coordinates.
 * Helpful for drawing the selection box during box selection.
 * @param x1 The x-coordinate of one corner of the box.
 * @param y1 The y-coordinate of one corner of the box.
 * @param x2 The x-coordinate of the opposite corner of the box.
 * @param y2 The y-coordinate of the opposite corner of the box.
 * @param outlineColor The RGBA color of the box outline.
 * @param fillColor The RGBA color of the box fill.
 */
void DrawOutlinedRectangle(float x1, float y1, float x2, float y2,
    const float outlineColor[4], const float fillColor[4]) {

        // Like glPushAttrib/glPopAttrib, glPushMatrix/glPopMatrix
        // allow us to save and restore the current OpenGL state.
        glPushMatrix();

        // We use the identity matrix as our modelview matrix
        // since we do not want to do any weird transformations to
        // our selection box; it's just a box.
        glLoadIdentity();

        // glPushAttrib tells OpenGL to save the current state of certain attributes,
        // in this case, the enable state and color buffer state.
        // This is done in order to modify these states for our drawing operations
        // without permanently changing the global OpenGL state.
        // Think of it like how when you write assembly code, you push registers onto the stack
        // before modifying them, and then pop them back to restore their original values.
        glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Draw the filled rectangle first
        glColor4fv(fillColor);
        glBegin(GL_QUADS);
        glVertex2f(x1, y1);
        glVertex2f(x2, y1);
        glVertex2f(x2, y2);
        glVertex2f(x1, y2);
        glEnd();

        // Draw the outline of the rectangle
        glColor4fv(outlineColor);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x1, y1);
        glVertex2f(x2, y1);
        glVertex2f(x2, y2);
        glVertex2f(x1, y2);
        glEnd();

        // glPopAttrib restores the previously saved OpenGL state.
        glPopAttrib();

        // glPopMatrix restores the previous matrix state.
        glPopMatrix();
    }

/**
 * Renders text directly onto the screen at the specified coordinates.
 * @param context The OpenGL context containing rendering information.
 * @param text The null-terminated string of text to render.
 * @param x The x-coordinate on the screen to start rendering the text.
 * @param y The y-coordinate on the screen to start rendering the text.
 * @param fontPixelHeight The desired height of the font in pixels, or default of 16 if <= 0.
 * @param color The RGBA color to use for rendering the text, or NULL for default white.
 */
void DrawScreenText(OpenGLContext *context, const char *text, float x, float y,
    float fontPixelHeight, const float color[4]) {

    // Basic validation of input parameters.
    if (context == NULL || text == NULL) {
        return;
    }

    if (context->deviceContext == NULL || context->renderContext == NULL) {
        return;
    }

    if (context->width <= 0 || context->height <= 0) {
        return;
    }

    // If no valid font height is provided, we use 16 pixels as a default.
    if (fontPixelHeight <= 0.0f) {
        fontPixelHeight = 16.0f;
    }

    // We want to ensure the font height is a positive integer value,
    // so we take the absolute value and round it to the nearest whole number.

    float absHeight = fabsf(fontPixelHeight);
    int roundedHeight = (int)(absHeight + 0.5f);
    if (roundedHeight <= 0) {
        roundedHeight = 1;
    }

    // We have a dedicated helper function OpenGLAcquireFont which can create or retrieve
    // a font display list based on the specified font parameters.
    GLuint listBase = OpenGLAcquireFont(context, 
        -1 * roundedHeight, // Height of font. This is negative to indicate character height rather than cell height
        0, // Width of font. 0 means default width based on height
        0, // Angle of escapement. Escapement is the angle between the baseline of a character and the x-axis of the device.
        0, // Orientation angle. This is the angle between the baseline of a character and the x-axis of the device.
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
        
    // A listBase of 0 indicates failure to acquire the font.
    if (listBase == 0) {
        return;
    }

    // Determine the final color to use for rendering the text.

    // By default, we use white color if no color is provided.
    float defaultColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    const float *finalColor = color != NULL ? color : defaultColor;

    // We save the current OpenGL state before modifying it for text rendering.
    glPushAttrib(GL_CURRENT_BIT | GL_LIST_BIT | GL_ENABLE_BIT);

    // We disable texturing and lighting to ensure the text is rendered correctly
    // without any unwanted effects (this is UI text, as opposed to some sort of 
    // text being rendered into some game world scene).

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    
    // Apply the desired text color.
    glColor4fv(finalColor);

    // We subtract 32 from the list base because
    // the first 32 ASCII characters are non-printable control characters,
    // and as such our font from OpenGLAcquireFont does not include them.
    glListBase(listBase - 32);

    // Will make any subsequent matrix operations affect the modelview matrix.
    glMatrixMode(GL_MODELVIEW);

    // Save the current matrix state.
    glPushMatrix();
    
    // Load the identity matrix to reset any previous transformations.
    glLoadIdentity();

    // Now we render the text line by line, handling newlines appropriately.
    
    // The starting y-coordinate for the first line of text.
    float lineY = y;
    const char *lineStart = text;

    // Loop through each line in the text until we reach the null terminator.
    while (*lineStart != '\0') {
        
        // We find the end of the current line by searching for a newline character,
        // and then once we know the line length, we can render that line.
        const char *lineEnd = strchr(lineStart, '\n');
        if (lineEnd == NULL) {
            // If no newline is found, the line goes until the end of the string.
            lineEnd = lineStart + strlen(lineStart);
        }

        // Determine the length of the current line.
        size_t lineLength = (size_t)(lineEnd - lineStart);

        // Render the current line at the specified (x, lineY) position.
        glRasterPos2f(x, lineY);
        if (lineLength > 0) {
            // The glCallLists function executes a list of display lists.
            // Here, we use it to render each character in the current line.
            glCallLists((GLsizei)lineLength, GL_UNSIGNED_BYTE, lineStart);
        }

        // If we reached the end of the string, we are done.
        if (*lineEnd == '\0') {
            break;
        }

        // Move to the start of the next line,
        // and adjust the y-coordinate for the next line 
        // based on the font pixel height.
        lineStart = lineEnd + 1;
        lineY += fontPixelHeight;
    }

    // Reset OpenGL state to what it was before rendering the text.
    glPopMatrix();
    glPopAttrib();
}
