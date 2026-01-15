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
void DrawSmoothRing(float cx, float cy, float innerRadius, float outerRadius, int segments,
    float featherWidth, const float color[4]) {
    if (outerRadius <= 0.0f || innerRadius < 0.0f || innerRadius >= outerRadius || segments < 3 || color == NULL) {
        return;
    }

    float alpha = color[3];
    if (featherWidth <= 0.0f || alpha <= 0.0f) {
        glColor4fv(color);
        DrawRing(cx, cy, innerRadius, outerRadius, segments);
        return;
    }

    float ringWidth = outerRadius - innerRadius;
    float clampedFeather = fminf(featherWidth, ringWidth * 0.5f);

    float innerFadeStart = innerRadius;
    float innerFadeEnd = innerRadius + clampedFeather;
    float outerFadeStart = outerRadius - clampedFeather;
    float outerFadeEnd = outerRadius;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float opaqueColor[4] = {color[0], color[1], color[2], alpha};
    float transparentColor[4] = {color[0], color[1], color[2], 0.0f};

    if (innerFadeEnd > innerFadeStart) {
        DrawRadialGradientRing(cx, cy, innerFadeStart, innerFadeEnd, segments, transparentColor, opaqueColor);
    }

    if (outerFadeEnd > outerFadeStart) {
        DrawRadialGradientRing(cx, cy, outerFadeStart, outerFadeEnd, segments, opaqueColor, transparentColor);
    }

    if (outerFadeStart > innerFadeEnd) {
        glColor4fv(color);
        DrawRing(cx, cy, innerFadeEnd, outerFadeStart, segments);
    }

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
