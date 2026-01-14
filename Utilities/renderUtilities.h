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

#endif // _RENDER_UTILITIES_H_
