/**
 * Header file for the render utility functions file.
 * @file Utilities/renderUtilities.h
 * @author abmize
 */
#ifndef _RENDER_UTILITIES_H_
#define _RENDER_UTILITIES_H_

#include <stdint.h>
#include <GL/gl.h>
#include <math.h>

// Type Definitions
typedef struct {
    int width;
    int height;
    uint32_t *pixels;
} Frame;

// Constant Definitions
# define M_PI 3.14159265358979323846 // Sadly undefined in math.h

/**
 * Draws a hollow circle using OpenGL.
 * @param cx The x-coordinate of the center of the circle.
 * @param cy The y-coordinate of the center of the circle.
 * @param radius The radius of the circle.
 * @param segments The number of segments to use for drawing the circle.
 * @param thickness The thickness of the circle outline.
 */
void DrawCircle(float cx, float cy, float radius, int segments, float thickness);

/**
 * Draws a filled circle using OpenGL.
 * @param cx The x-coordinate of the center of the circle.
 * @param cy The y-coordinate of the center of the circle.
 * @param radius The radius of the circle.
 * @param segments The number of segments to use for drawing the circle.
 */
void DrawFilledCircle(float cx, float cy, float radius, int segments);

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
void DrawRing(float cx, float cy, float innerRadius, float outerRadius, int segments);

#endif // _RENDER_UTILITIES_H_
