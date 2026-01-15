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
