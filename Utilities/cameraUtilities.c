/**
 * Implements helper routines for maintaining a simple 2D camera.
 */
#include "Utilities/cameraUtilities.h"

static float ClampFloat(float value, float minValue, float maxValue) {
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

/**
 * Initializes a CameraState structure to default values.
 * @param camera Pointer to the CameraState to initialize.
 */
void CameraInitialize(CameraState *camera) {
    // Basic validation of input pointer.
    if (camera == NULL) {
        return;
    }

    // Set default values. (zero position, 1.0 zoom, 
    // 1/4 min zoom, 4.0 max zoom, zero level size)
    camera->position = Vec2Zero();
    camera->zoom = 1.0f;
    camera->minZoom = 0.25f;
    camera->maxZoom = 4.0f;
    camera->levelWidth = 0.0f;
    camera->levelHeight = 0.0f;
}

/**
 * Sets the bounds of the camera based on the level dimensions.
 * @param camera Pointer to the CameraState to modify.
 * @param levelWidth The width of the level in world units.
 * @param levelHeight The height of the level in world units.
 */
void CameraSetBounds(CameraState *camera, float levelWidth, float levelHeight) {
    // Basic validation of input pointer.
    if (camera == NULL) {
        return;
    }

    // Set the level dimensions, ensuring they are non-negative.
    camera->levelWidth = levelWidth >= 0.0f ? levelWidth : 0.0f;
    camera->levelHeight = levelHeight >= 0.0f ? levelHeight : 0.0f;
}

/**
 * Clamps the camera position to ensure it does not go outside the level bounds.
 * @param camera Pointer to the CameraState to modify.
 * @param viewWidth The width of the camera's view in world units.
 * @param viewHeight The height of the camera's view in world units.
 */
void CameraClampToBounds(CameraState *camera, float viewWidth, float viewHeight) {
    // Basic validation of input pointer.
    if (camera == NULL) {
        return;
    }

    // Calculate the maximum allowed position based on level size and view size.
    float maxX = camera->levelWidth - viewWidth;
    float maxY = camera->levelHeight - viewHeight;

    // Clamp the camera position within the allowed range.
    if (maxX >= 0.0f) {
        camera->position.x = ClampFloat(camera->position.x, 0.0f, maxX);
    } else {
        camera->position.x = maxX * 0.5f;
    }

    if (maxY >= 0.0f) {
        camera->position.y = ClampFloat(camera->position.y, 0.0f, maxY);
    } else {
        camera->position.y = maxY * 0.5f;
    }
}

/**
 * Converts screen coordinates to world coordinates using the camera state.
 * @param camera Pointer to the CameraState to use for conversion.
 * @param screen The screen coordinates to convert.
 * @return The corresponding world coordinates.
 */
Vec2 CameraScreenToWorld(const CameraState *camera, Vec2 screen) {
    // Basic validation of input pointer.
    if (camera == NULL || camera->zoom <= FLT_EPSILON) {
        return screen;
    }

    // Convert screen to world coordinates.
    // The formula is:
    // world.x = screen.x / camera.zoom + camera.position.x
    // world.y = screen.y / camera.zoom + camera.position.y
    Vec2 world = {0.0f, 0.0f};
    world.x = screen.x / camera->zoom + camera->position.x;
    world.y = screen.y / camera->zoom + camera->position.y;
    return world;
}

/**
 * Converts world coordinates to screen coordinates using the camera state.
 * @param camera Pointer to the CameraState to use for conversion.
 * @param world The world coordinates to convert.
 * @return The corresponding screen coordinates.
 */
Vec2 CameraWorldToScreen(const CameraState *camera, Vec2 world) {
    // Basic validation of input pointer.
    if (camera == NULL) {
        return world;
    }

    // Convert world to screen coordinates.
    // The formula is:
    // screen.x = (world.x - camera.position.x) * camera.zoom
    // screen.y = (world.y - camera.position.y) * camera.zoom
    Vec2 screen = {0.0f, 0.0f};
    screen.x = (world.x - camera->position.x) * camera->zoom;
    screen.y = (world.y - camera->position.y) * camera->zoom;
    return screen;
}

/**
 * Sets the camera's zoom level, clamped to min and max zoom.
 * @param camera Pointer to the CameraState to modify.
 * @param zoom The desired zoom level.
 * @return true if the zoom level was changed, false otherwise.
 */
bool CameraSetZoom(CameraState *camera, float zoom) {
    // Basic validation of input pointer.
    if (camera == NULL) {
        return false;
    }

    // Clamp the zoom level and determine if it changed.
    float clamped = ClampFloat(zoom, camera->minZoom, camera->maxZoom);
    bool changed = fabsf(clamped - camera->zoom) > FLT_EPSILON;
    camera->zoom = clamped;
    return changed;
}
