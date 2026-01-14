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
