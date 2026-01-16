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

// Pi, the mathematical constant. The ratio of a circle's circumference to its diameter.
#define M_PI 3.14159265358979323846

// Minimum number of segments to use when drawing circles to ensure smoothness.
#define MIN_CIRCLE_SEGMENTS 32

// Maximum number of segments to use when drawing circles to avoid excessive detail.
#define MAX_CIRCLE_SEGMENTS 256


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
void DrawFeatheredRing(float cx, float cy, float innerRadius, float outerRadius, float featherWidth, const float color[4]);

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
    const float innerColor[4], const float outerColor[4]);

/**
 * Draws a filled circle with feathered edges.
 * The feathering creates a smooth transition from the circle's color to transparency.
 * @param cx The x-coordinate of the circle's center.
 * @param cy The y-coordinate of the circle's center.
 * @param radius The radius of the circle.
 * @param featherWidth The width of the feathering effect at the edge of the circle.
 * @param color The RGBA color of the circle.
 */
void DrawFeatheredFilledInCircle(float cx, float cy, float radius, float featherWidth, const float color[4]);

/**
 * Computes the number of segments to use for drawing a circle of the given radius.
 * This helps ensure that circles are drawn smoothly regardless of their size.
 * Specifically, this formula aims for approximately 1.5 units of arc length per segment.
 * @param radius The radius of the circle.
 * @return The computed number of segments for the circle.
 */
int ComputeCircleSegments(float radius);

#endif // _RENDER_UTILITIES_H_
