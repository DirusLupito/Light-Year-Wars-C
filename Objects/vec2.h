/**
 * Header file for the 2D vector object.
 * @file Objects/vec2.h
 * @author abmize
 */

#ifndef _VEC2_H_
#define _VEC2_H_

#include <math.h>

// A Vec2 represents a point or vector in 2D space with x and y components.
typedef struct Vec2 {
    float x;
    float y;
} Vec2;

/**
 * Returns a Vec2 initialized to (0.0f, 0.0f).
 * @return A Vec2 at the origin.
 */
Vec2 Vec2Zero(void);

/**
 * Adds two Vec2 vectors component-wise.
 * @param a The first Vec2.
 * @param b The second Vec2.
 * @return The resulting Vec2 after addition.
 */
Vec2 Vec2Add(Vec2 a, Vec2 b);

/**
 * Subtracts vector b from vector a component-wise.
 * @param a The first Vec2.
 * @param b The second Vec2.
 * @return The resulting Vec2 after subtraction.
 */
Vec2 Vec2Subtract(Vec2 a, Vec2 b);

/**
 * Scales a Vec2 by a scalar value.
 * @param v The Vec2 to scale.
 * @param s The scalar value.
 * @return The scaled Vec2.
 */
Vec2 Vec2Scale(Vec2 v, float s);

/**
 * Calculates the dot product of two Vec2 vectors.
 * This is given by the formula: a.x * b.x + a.y * b.y.
 * @param a The first Vec2.
 * @param b The second Vec2.
 * @return The dot product as a float.
 */
float Vec2Dot(Vec2 a, Vec2 b);

/**
 * Calculates the 2 norm, or Euclidean length, of a Vec2.
 * This is given by the formula: sqrt(v.x^2 + v.y^2).
 * @param v The Vec2 whose length is to be calculated.
 * @return The length of the Vec2 as a float.
 */
float Vec2Length(Vec2 v);

/**
 * Normalizes a Vec2 to have a length of 1.
 * If the vector's length is very small (<= 0.0001f), returns the zero vector.
 * This is done by dividing each component by the vector's length.
 * @param v The Vec2 to normalize.
 * @return The normalized Vec2.
 */
Vec2 Vec2Normalize(Vec2 v);

/**
 * Clamps a Vec2 to a maximum length.
 * If the vector's length exceeds maxLength, it is scaled down to have length maxLength.
 * If the vector's length is very small (<= 0.0001f), it is returned unchanged.
 * @param v The Vec2 to clamp.
 * @param maxLength The maximum allowed length.
 * @return The clamped Vec2.
 */
Vec2 Vec2ClampToLength(Vec2 v, float maxLength);

#endif // _VEC2_H_
