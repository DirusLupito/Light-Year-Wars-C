/**
 * Utility helpers for 2D camera handling used by the Light Year Wars viewers.
 * Provides basic state tracking, conversions between screen and world space,
 * and clamping against level bounds.
 */
#ifndef _CAMERA_UTILITIES_H_
#define _CAMERA_UTILITIES_H_

#include <stdbool.h>
#include <float.h>
#include <math.h>
#include <stddef.h>

#include "Objects/vec2.h"

// Structure holding the state of a simple 2D camera.
// Holds necessary information for the camera's view of the world.
// These components are:
// - position: The top-left corner of the camera's view in world coordinates.
// - zoom: The current zoom level of the camera (1.0 = 100%).
// - minZoom: The minimum allowed zoom level.
// - maxZoom: The maximum allowed zoom level.
// - levelWidth: The width of the level in world units.
// - levelHeight: The height of the level in world units.
typedef struct CameraState {
    Vec2 position;
    float zoom;
    float minZoom;
    float maxZoom;
    float levelWidth;
    float levelHeight;
} CameraState;

/**
 * Initializes a CameraState structure to default values.
 * @param camera Pointer to the CameraState to initialize.
 */
void CameraInitialize(CameraState *camera);

/**
 * Sets the bounds of the camera based on the level dimensions.
 * @param camera Pointer to the CameraState to modify.
 * @param levelWidth The width of the level in world units.
 * @param levelHeight The height of the level in world units.
 */
void CameraSetBounds(CameraState *camera, float levelWidth, float levelHeight);

/**
 * Clamps the camera position to ensure it does not go outside the level bounds.
 * @param camera Pointer to the CameraState to modify.
 * @param viewWidth The width of the camera's view in world units.
 * @param viewHeight The height of the camera's view in world units.
 */
void CameraClampToBounds(CameraState *camera, float viewWidth, float viewHeight);

/**
 * Converts screen coordinates to world coordinates using the camera state.
 * @param camera Pointer to the CameraState to use for conversion.
 * @param screen The screen coordinates to convert.
 * @return The corresponding world coordinates.
 */
Vec2 CameraScreenToWorld(const CameraState *camera, Vec2 screen);

/**
 * Converts world coordinates to screen coordinates using the camera state.
 * @param camera Pointer to the CameraState to use for conversion.
 * @param world The world coordinates to convert.
 * @return The corresponding screen coordinates.
 */
Vec2 CameraWorldToScreen(const CameraState *camera, Vec2 world);

/**
 * Sets the camera's zoom level, clamped to min and max zoom.
 * @param camera Pointer to the CameraState to modify.
 * @param zoom The desired zoom level.
 * @return true if the zoom level was changed, false otherwise.
 */
bool CameraSetZoom(CameraState *camera, float zoom);

#endif /* _CAMERA_UTILITIES_H_ */
