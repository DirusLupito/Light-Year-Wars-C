/**
 * Implementation file for the 2D vector object.
 * A Vec2 represents a point or vector in 2D space with x and y components.
 * This implementation provides basic vector operations such as addition,
 * subtraction, scaling, dot product, length calculation, normalization,
 * and clamping to a maximum length.
 * @file Objects/vec2.c
 * @author abmize
 */

#include "Objects/vec2.h"

/**
 * Returns a Vec2 initialized to (0.0f, 0.0f).
 * @return A Vec2 at the origin.
 */
Vec2 Vec2Zero(void) {
    Vec2 v = {0.0f, 0.0f};
    return v;
}

/**
 * Adds two Vec2 vectors component-wise.
 * @param a The first Vec2.
 * @param b The second Vec2.
 * @return The resulting Vec2 after addition.
 */
Vec2 Vec2Add(Vec2 a, Vec2 b) {
    Vec2 v = {a.x + b.x, a.y + b.y};
    return v;
}

/**
 * Subtracts vector b from vector a component-wise.
 * @param a The first Vec2.
 * @param b The second Vec2.
 * @return The resulting Vec2 after subtraction.
 */
Vec2 Vec2Subtract(Vec2 a, Vec2 b) {
    Vec2 v = {a.x - b.x, a.y - b.y};
    return v;
}

/**
 * Scales a Vec2 by a scalar value.
 * @param v The Vec2 to scale.
 * @param s The scalar value.
 * @return The scaled Vec2.
 */
Vec2 Vec2Scale(Vec2 v, float s) {
    Vec2 scaled = {v.x * s, v.y * s};
    return scaled;
}

/**
 * Calculates the dot product of two Vec2 vectors.
 * This is given by the formula: a.x * b.x + a.y * b.y.
 * @param a The first Vec2.
 * @param b The second Vec2.
 * @return The dot product as a float.
 */
float Vec2Dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

/**
 * Calculates the 2 norm, or Euclidean length, of a Vec2.
 * This is given by the formula: sqrt(v.x^2 + v.y^2).
 * @param v The Vec2 whose length is to be calculated.
 * @return The length of the Vec2 as a float.
 */
float Vec2Length(Vec2 v) {
    return sqrtf(Vec2Dot(v, v));
}

/**
 * Normalizes a Vec2 to have a length of 1.
 * If the vector's length is very small (<= 0.0001f), returns the zero vector.
 * This is done by dividing each component by the vector's length.
 * @param v The Vec2 to normalize.
 * @return The normalized Vec2.
 */
Vec2 Vec2Normalize(Vec2 v) {
    float len = Vec2Length(v);
    if (len <= 0.0001f) {
        return Vec2Zero();
    }
    return Vec2Scale(v, 1.0f / len);
}

/**
 * Clamps a Vec2 to a maximum length.
 * If the vector's length exceeds maxLength, it is scaled down to have length maxLength.
 * If the vector's length is very small (<= 0.0001f), it is returned unchanged.
 * @param v The Vec2 to clamp.
 * @param maxLength The maximum allowed length.
 * @return The clamped Vec2.
 */
Vec2 Vec2ClampToLength(Vec2 v, float maxLength) {
    float len = Vec2Length(v);
    if (len > maxLength && len > 0.0001f) {
        return Vec2Scale(v, maxLength / len);
    }
    return v;
}
