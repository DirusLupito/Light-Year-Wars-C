/**
 * Header for lobby preview utilities.
 * Provides a reusable preview panel that renders a generated level
 * inside the lobby UI on both client and server.
 * @file Utilities/MenuUtilities/lobbyPreviewUtilities.h
 * @author abmize
 */
#ifndef _LOBBY_PREVIEW_UTILITIES_H_
#define _LOBBY_PREVIEW_UTILITIES_H_

#include <stdbool.h>
#include <stddef.h>
#include <winsock2.h>
#include <windows.h>
#include <math.h>
#include <string.h>
#include "Objects/level.h"
#include "Utilities/cameraUtilities.h"
#include "Utilities/openglUtilities.h"
#include "Utilities/MenuUtilities/lobbyMenuUtilities.h"
#include "Utilities/renderUtilities.h"
#include "Utilities/gameUtilities.h"
#include "Objects/faction.h"
#include "Objects/planet.h"

/**
 * Stores all runtime state needed to render and interact with the lobby preview panel.
 * This keeps the preview implementation shared between client and server.
 */
typedef struct LobbyPreviewContext {
    Level level; /* Generated preview level. */
    bool levelInitialized; /* True when the preview level contains valid data. */
    bool dirty; /* True when the preview needs regeneration. */

    CameraState camera; /* Camera used for navigating the preview panel. */
    float baseMinZoom; /* Original min zoom so fit-to-level can lower bounds safely. */
    float baseMaxZoom; /* Original max zoom to restore after layout changes. */
    bool dragging; /* True while the preview is being dragged. */
    Vec2 dragStartScreen; /* Mouse position at drag start (screen coordinates). */
    Vec2 dragStartCamera; /* Camera position at drag start. */

    float viewWidth; /* Cached preview viewport width for layout change detection. */
    float viewHeight; /* Cached preview viewport height for layout change detection. */
    bool openLast; /* Tracks previous preview open state for toggle detection. */
} LobbyPreviewContext;

/**
 * Initializes the lobby preview context and its camera constraints.
 * @param preview Pointer to the LobbyPreviewContext to initialize.
 * @param minZoom Minimum zoom level for the preview camera.
 * @param maxZoom Maximum zoom level for the preview camera.
 */
void LobbyPreviewInitialize(LobbyPreviewContext *preview, float minZoom, float maxZoom);

/**
 * Releases memory owned by the preview level.
 * @param preview Pointer to the LobbyPreviewContext to release.
 */
void LobbyPreviewRelease(LobbyPreviewContext *preview);

/**
 * Resets transient preview state for a fresh lobby session.
 * @param preview Pointer to the LobbyPreviewContext to reset.
 */
void LobbyPreviewReset(LobbyPreviewContext *preview);

/**
 * Marks the preview as dirty so it regenerates on the next update.
 * @param preview Pointer to the LobbyPreviewContext to modify.
 */
void LobbyPreviewMarkDirty(LobbyPreviewContext *preview);

/**
 * Updates preview generation and camera edge panning while the preview is open.
 * @param preview Pointer to the LobbyPreviewContext to update.
 * @param lobbyUI Pointer to the lobby UI state for layout queries.
 * @param settings Pointer to lobby generation settings.
 * @param settingsValid True if the settings are valid for generation.
 * @param slotColors Slot color array for faction previews.
 * @param slotColorValid Flags indicating which slot colors are valid.
 * @param slotCount Number of slots to use for preview coloring.
 * @param windowHandle Handle to the preview window.
 * @param deltaTime Time elapsed since last update.
 * @param edgeMargin Margin in pixels for edge panning.
 * @param edgeSpeed Speed in world units per second for edge panning.
 * @param context OpenGL context used for viewport sizing.
 * @return True if the preview was updated, false otherwise.
 */
bool LobbyPreviewUpdate(LobbyPreviewContext *preview,
    const LobbyMenuUIState *lobbyUI,
    const LobbyMenuGenerationSettings *settings,
    bool settingsValid,
    const float slotColors[LOBBY_MENU_MAX_SLOTS][4],
    const bool slotColorValid[LOBBY_MENU_MAX_SLOTS],
    size_t slotCount,
    HWND windowHandle,
    float deltaTime,
    float edgeMargin,
    float edgeSpeed,
    OpenGLContext *context);

/**
 * Handles mouse button down for preview dragging.
 * @param preview Pointer to the LobbyPreviewContext to update.
 * @param lobbyUI Pointer to the lobby UI state for layout queries.
 * @param windowHandle Handle to the preview window.
 * @param x Mouse X coordinate in client space.
 * @param y Mouse Y coordinate in client space.
 * @param context OpenGL context for viewport sizing.
 * @return True if the event was consumed by the preview, false otherwise.
 */
bool LobbyPreviewHandleMouseDown(LobbyPreviewContext *preview,
    const LobbyMenuUIState *lobbyUI,
    HWND windowHandle,
    int x,
    int y,
    OpenGLContext *context);

/**
 * Handles mouse movement for preview dragging.
 * @param preview Pointer to the LobbyPreviewContext to update.
 * @param lobbyUI Pointer to the lobby UI state for layout queries.
 * @param x Mouse X coordinate in client space.
 * @param y Mouse Y coordinate in client space.
 * @param context OpenGL context for viewport sizing.
 * @return True if the event was consumed by the preview, false otherwise.
 */
bool LobbyPreviewHandleMouseMove(LobbyPreviewContext *preview,
    const LobbyMenuUIState *lobbyUI,
    int x,
    int y,
    OpenGLContext *context);

/**
 * Handles mouse button up for preview dragging.
 * @param preview Pointer to the LobbyPreviewContext to update.
 * @param lobbyUI Pointer to the lobby UI state for layout queries.
 * @param windowHandle Handle to the preview window.
 * @param x Mouse X coordinate in client space.
 * @param y Mouse Y coordinate in client space.
 * @param context OpenGL context for viewport sizing.
 * @return True if the event was consumed by the preview, false otherwise.
 */
bool LobbyPreviewHandleMouseUp(LobbyPreviewContext *preview,
    const LobbyMenuUIState *lobbyUI,
    HWND windowHandle,
    int x,
    int y,
    OpenGLContext *context);

/**
 * Handles mouse wheel zooming inside the preview viewport.
 * @param preview Pointer to the LobbyPreviewContext to update.
 * @param lobbyUI Pointer to the lobby UI state for layout queries.
 * @param windowHandle Handle to the preview window.
 * @param wheelDelta Raw wheel delta from WM_MOUSEWHEEL.
 * @param x Mouse X coordinate in client space.
 * @param y Mouse Y coordinate in client space.
 * @param zoomFactor Zoom factor to apply per wheel step.
 * @param context OpenGL context for viewport sizing.
 * @return True if the event was consumed by the preview, false otherwise.
 */
bool LobbyPreviewHandleMouseWheel(LobbyPreviewContext *preview,
    const LobbyMenuUIState *lobbyUI,
    HWND windowHandle,
    int wheelDelta,
    int x,
    int y,
    float zoomFactor,
    OpenGLContext *context);

/**
 * Renders the preview panel and generated level, if visible.
 * @param preview Pointer to the LobbyPreviewContext to render.
 * @param lobbyUI Pointer to the lobby UI state for layout queries.
 * @param context OpenGL context used for rendering.
 */
void LobbyPreviewRender(LobbyPreviewContext *preview,
    const LobbyMenuUIState *lobbyUI,
    OpenGLContext *context);

#endif /* _LOBBY_PREVIEW_UTILITIES_H_ */
