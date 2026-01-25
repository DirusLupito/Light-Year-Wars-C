/**
 * Implementation of lobby preview utilities.
 * Shares preview generation, input handling, and rendering between client and server.
 * @file Utilities/MenuUtilities/lobbyPreviewUtilities.c
 * @author abmize
 */
#include "Utilities/MenuUtilities/lobbyPreviewUtilities.h"

/**
 * Computes the preview viewport rectangle inside the preview panel.
 * @param lobbyUI Pointer to the lobby UI state for layout queries.
 * @param context OpenGL context for sizing.
 * @param panelOut Optional output for the preview panel rectangle.
 * @param viewOut Output rectangle for the preview viewport region.
 * @return True if the preview viewport is valid, false otherwise.
 */
static bool LobbyPreviewGetViewRect(const LobbyMenuUIState *lobbyUI,
    OpenGLContext *context,
    MenuUIRect *panelOut,
    MenuUIRect *viewOut) {
    if (lobbyUI == NULL || context == NULL || viewOut == NULL || context->width <= 0 || context->height <= 0) {
        return false;
    }

    MenuUIRect panel;
    // The layout helper does not mutate the lobby UI, but its signature is non-const.
    // We cast here to avoid duplicating layout logic just for const-correctness.
    if (!LobbyMenuUIGetPreviewPanelRect((LobbyMenuUIState *)lobbyUI, context->width, context->height, &panel)) {
        return false;
    }

    if (panelOut != NULL) {
        *panelOut = panel;
    }

    // Leave space for the header and padding inside the preview panel.
    viewOut->x = panel.x + LOBBY_MENU_PREVIEW_PANEL_PADDING;
    viewOut->y = panel.y + LOBBY_MENU_PREVIEW_PANEL_HEADER_HEIGHT + LOBBY_MENU_PREVIEW_PANEL_PADDING;
    viewOut->width = panel.width - 2.0f * LOBBY_MENU_PREVIEW_PANEL_PADDING;
    viewOut->height = panel.height - LOBBY_MENU_PREVIEW_PANEL_HEADER_HEIGHT - 2.0f * LOBBY_MENU_PREVIEW_PANEL_PADDING;

    // Reject degenerate sizes so the camera math stays stable.
    if (viewOut->width <= 1.0f || viewOut->height <= 1.0f) {
        return false;
    }

    return true;
}

/**
 * Applies the preview camera transformation to the OpenGL modelview matrix.
 * @param preview Pointer to the LobbyPreviewContext supplying the camera state.
 */
static void LobbyPreviewApplyCameraTransform(const LobbyPreviewContext *preview) {
    if (preview == NULL || preview->camera.zoom <= 0.0f) {
        return;
    }

    // Scale first so translations are applied in world units.
    glScalef(preview->camera.zoom, preview->camera.zoom, 1.0f);

    // Translate to align the camera's top-left world coordinate with the viewport.
    glTranslatef(-preview->camera.position.x, -preview->camera.position.y, 0.0f);
}

/**
 * Clamps the preview camera to the bounds of the generated preview level.
 * @param preview Pointer to the LobbyPreviewContext to clamp.
 * @param viewRect Rectangle describing the preview viewport in pixels.
 */
static void LobbyPreviewClampCamera(LobbyPreviewContext *preview, const MenuUIRect *viewRect) {
    if (preview == NULL || viewRect == NULL || preview->camera.zoom <= 0.0f) {
        return;
    }

    // Convert viewport size into world units for clamping.
    float viewWidth = viewRect->width / preview->camera.zoom;
    float viewHeight = viewRect->height / preview->camera.zoom;
    CameraClampToBounds(&preview->camera, viewWidth, viewHeight);
}

/**
 * Resets the preview camera to show the generated level comfortably.
 * @param preview Pointer to the LobbyPreviewContext to update.
 * @param viewRect Rectangle describing the preview viewport in pixels.
 */
static void LobbyPreviewResetCamera(LobbyPreviewContext *preview, const MenuUIRect *viewRect) {
    if (preview == NULL || viewRect == NULL || !preview->levelInitialized) {
        return;
    }

    CameraSetBounds(&preview->camera, preview->level.width, preview->level.height);

    // Fit the level inside the preview viewport while honoring zoom limits.
    float zoomX = viewRect->width / fmaxf(preview->level.width, 1.0f);
    float zoomY = viewRect->height / fmaxf(preview->level.height, 1.0f);
    float targetZoom = fminf(zoomX, zoomY);

    // Allow zooming out far enough to capture the full level.
    // We clamp against the original limits so we do not permanently relax constraints.
    preview->camera.minZoom = fminf(preview->baseMinZoom, targetZoom);
    preview->camera.maxZoom = preview->baseMaxZoom;
    CameraSetZoom(&preview->camera, targetZoom);

    // Center the camera so the full level is visible on open.
    preview->camera.position.x = (preview->level.width - viewRect->width / preview->camera.zoom) * 0.5f;
    preview->camera.position.y = (preview->level.height - viewRect->height / preview->camera.zoom) * 0.5f;
    LobbyPreviewClampCamera(preview, viewRect);
}

/**
 * Generates the preview level using the provided settings and slot colors.
 * @param preview Pointer to the LobbyPreviewContext to update.
 * @param settings Pointer to lobby generation settings.
 * @param slotColors Slot color array for faction previews.
 * @param slotColorValid Flags indicating which slot colors are valid.
 * @param slotCount Number of slots to use for preview coloring.
 * @return True if generation succeeded, false otherwise.
 */
static bool LobbyPreviewBuildLevel(LobbyPreviewContext *preview,
    const LobbyMenuGenerationSettings *settings,
    const float slotColors[LOBBY_MENU_MAX_SLOTS][4],
    const bool slotColorValid[LOBBY_MENU_MAX_SLOTS],
    size_t slotCount) {
    if (preview == NULL || settings == NULL) {
        return false;
    }

    size_t factionCount = (size_t)settings->factionCount;
    size_t planetCount = (size_t)settings->planetCount;

    // Allocate level storage sized to lobby settings.
    size_t initialShipCapacity = factionCount * (size_t)((settings->minFleetCapacity + settings->maxFleetCapacity) * 0.5f);
    if (!LevelConfigure(&preview->level, factionCount, planetCount, initialShipCapacity)) {
        preview->levelInitialized = false;
        return false;
    }

    // Apply faction colors from lobby slots to keep the preview consistent.
    size_t colorCount = slotCount;
    if (colorCount > LOBBY_MENU_MAX_SLOTS) {
        colorCount = LOBBY_MENU_MAX_SLOTS;
    }

    // Create factions with assigned or fallback colors.
    for (size_t i = 0; i < factionCount; ++i) {
        // Fallback to a neutral gray if no slot color is assigned.
        float fallback[4] = {0.8f, 0.8f, 0.8f, 1.0f};
        const float *color = fallback;
        if (i < colorCount && slotColorValid != NULL && slotColorValid[i]) {
            color = slotColors[i];
        }
        preview->level.factions[i] = CreateFaction((int)i, color[0], color[1], color[2]);
    }

    // Generate the randomized planet layout using the current settings.
    if (!GenerateRandomLevel(&preview->level,
        planetCount,
        factionCount,
        settings->minFleetCapacity,
        settings->maxFleetCapacity,
        settings->levelWidth,
        settings->levelHeight,
        settings->randomSeed)) {
        preview->levelInitialized = false;
        return false;
    }

    preview->levelInitialized = true;
    return true;
}

/**
 * Initializes the lobby preview context and its camera constraints.
 * @param preview Pointer to the LobbyPreviewContext to initialize.
 * @param minZoom Minimum zoom level for the preview camera.
 * @param maxZoom Maximum zoom level for the preview camera.
 */
void LobbyPreviewInitialize(LobbyPreviewContext *preview, float minZoom, float maxZoom) {
    if (preview == NULL) {
        return;
    }

    // Clear the preview state so we start from a known baseline.
    memset(preview, 0, sizeof(*preview));
    LevelInit(&preview->level);
    CameraInitialize(&preview->camera);
    // Store base limits so later fit-to-level logic can temporarily lower min zoom.
    preview->baseMinZoom = minZoom;
    preview->baseMaxZoom = maxZoom;
    preview->camera.minZoom = minZoom;
    preview->camera.maxZoom = maxZoom;
    preview->dirty = true;
}

/**
 * Releases memory owned by the preview level.
 * @param preview Pointer to the LobbyPreviewContext to release.
 */
void LobbyPreviewRelease(LobbyPreviewContext *preview) {
    if (preview == NULL) {
        return;
    }

    LevelRelease(&preview->level);
    preview->levelInitialized = false;
}

/**
 * Resets transient preview state for a fresh lobby session.
 * @param preview Pointer to the LobbyPreviewContext to reset.
 */
void LobbyPreviewReset(LobbyPreviewContext *preview) {
    if (preview == NULL) {
        return;
    }

    // Clear interaction state so no dragging carries over between sessions.
    preview->dragging = false;
    preview->openLast = false;
    preview->viewWidth = 0.0f;
    preview->viewHeight = 0.0f;
    // Keep base zoom limits intact while clearing transient preview state.
    preview->dirty = true;
    preview->levelInitialized = false;
    LevelRelease(&preview->level);
}

/**
 * Marks the preview as dirty so it regenerates on the next update.
 * @param preview Pointer to the LobbyPreviewContext to modify.
 */
void LobbyPreviewMarkDirty(LobbyPreviewContext *preview) {
    if (preview == NULL) {
        return;
    }

    preview->dirty = true;
}

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
    OpenGLContext *context) {
    if (preview == NULL || lobbyUI == NULL || settings == NULL || context == NULL || deltaTime <= 0.0f) {
        return false;
    }

    // Only update preview interactions when the preview panel is visible.
    if (!LobbyMenuUIIsPreviewOpen(lobbyUI)) {
        preview->dragging = false;
        preview->openLast = false;
        return false;
    }

    MenuUIRect viewRect;
    if (!LobbyPreviewGetViewRect(lobbyUI, context, NULL, &viewRect)) {
        return false;
    }

    // Regenerate preview content when settings change or when newly opened.
    bool openedNow = !preview->openLast;
    preview->openLast = true;
    if (preview->dirty || openedNow) {
        if (!settingsValid) {
            // Keep dirty true so we retry when settings become valid again.
            preview->levelInitialized = false;
            preview->dirty = true;
            return true;
        }

        preview->dirty = false;
        if (LobbyPreviewBuildLevel(preview, settings, slotColors, slotColorValid, slotCount)) {
            LobbyPreviewResetCamera(preview, &viewRect);
        }
    }

    if (!preview->levelInitialized) {
        return true;
    }

    // Refresh the camera when the preview viewport resizes.
    if (fabsf(preview->viewWidth - viewRect.width) > 0.5f || fabsf(preview->viewHeight - viewRect.height) > 0.5f) {
        preview->viewWidth = viewRect.width;
        preview->viewHeight = viewRect.height;
        LobbyPreviewResetCamera(preview, &viewRect);
    }

    // Only edge-pan when the window is active and the cursor is inside the preview viewport.
    if (windowHandle == NULL || GetForegroundWindow() != windowHandle) {
        return true;
    }

    // We get the cursor position in screen space and convert to client space
    // Then we check if it's inside the preview viewport.

    POINT cursor;
    if (!GetCursorPos(&cursor)) {
        return true;
    }
    ScreenToClient(windowHandle, &cursor);

    if (!MenuUIRectContains(&viewRect, (float)cursor.x, (float)cursor.y)) {
        return true;
    }

    float dx = 0.0f;
    float dy = 0.0f;

    if (cursor.x < (int)(viewRect.x + edgeMargin)) {
        dx -= 1.0f;
    } else if (cursor.x > (int)(viewRect.x + viewRect.width - edgeMargin)) {
        dx += 1.0f;
    }

    if (cursor.y < (int)(viewRect.y + edgeMargin)) {
        dy -= 1.0f;
    } else if (cursor.y > (int)(viewRect.y + viewRect.height - edgeMargin)) {
        dy += 1.0f;
    }

    if (dx != 0.0f || dy != 0.0f) {
        float speed = edgeSpeed * deltaTime / preview->camera.zoom;
        preview->camera.position.x += dx * speed;
        preview->camera.position.y += dy * speed;
        LobbyPreviewClampCamera(preview, &viewRect);
    }

    return true;
}

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
    OpenGLContext *context) {
    if (preview == NULL || lobbyUI == NULL || context == NULL) {
        return false;
    }

    if (!LobbyMenuUIIsPreviewOpen(lobbyUI)) {
        return false;
    }

    MenuUIRect viewRect;
    if (!LobbyPreviewGetViewRect(lobbyUI, context, NULL, &viewRect)) {
        return false;
    }

    if (!MenuUIRectContains(&viewRect, (float)x, (float)y)) {
        return false;
    }

    // Capture the mouse so dragging continues even if the cursor leaves the viewport.
    preview->dragging = true;
    preview->dragStartScreen = (Vec2){(float)x, (float)y};
    preview->dragStartCamera = preview->camera.position;
    if (windowHandle != NULL) {
        SetCapture(windowHandle);
    }
    return true;
}

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
    OpenGLContext *context) {
    if (preview == NULL || lobbyUI == NULL || context == NULL) {
        return false;
    }

    if (!preview->dragging) {
        return false;
    }

    MenuUIRect viewRect;
    if (!LobbyPreviewGetViewRect(lobbyUI, context, NULL, &viewRect) || preview->camera.zoom <= 0.0f) {
        return false;
    }

    float dx = (float)x - preview->dragStartScreen.x;
    float dy = (float)y - preview->dragStartScreen.y;

    // Move the camera opposite the cursor so panning feels natural.
    preview->camera.position.x = preview->dragStartCamera.x - dx / preview->camera.zoom;
    preview->camera.position.y = preview->dragStartCamera.y - dy / preview->camera.zoom;
    LobbyPreviewClampCamera(preview, &viewRect);
    return true;
}

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
    OpenGLContext *context) {
    (void)lobbyUI;
    (void)x;
    (void)y;
    (void)context;

    if (preview == NULL) {
        return false;
    }

    if (!preview->dragging) {
        return false;
    }

    // Release capture so normal UI can regain mouse control.
    preview->dragging = false;
    if (windowHandle != NULL) {
        ReleaseCapture();
    }
    return true;
}

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
    OpenGLContext *context) {
    (void)windowHandle;
    if (preview == NULL || lobbyUI == NULL || context == NULL || wheelDelta == 0) {
        return false;
    }

    if (!LobbyMenuUIIsPreviewOpen(lobbyUI)) {
        return false;
    }

    MenuUIRect viewRect;
    if (!LobbyPreviewGetViewRect(lobbyUI, context, NULL, &viewRect)) {
        return false;
    }

    if (!MenuUIRectContains(&viewRect, (float)x, (float)y)) {
        return false;
    }

    // Convert to preview-local screen coordinates before converting to world space.
    Vec2 local = {(float)x - viewRect.x, (float)y - viewRect.y};
    Vec2 focusWorld = CameraScreenToWorld(&preview->camera, local);

    float targetZoom = preview->camera.zoom;
    if (wheelDelta > 0) {
        targetZoom *= zoomFactor;
    } else {
        targetZoom /= zoomFactor;
    }

    float previousZoom = preview->camera.zoom;
    if (CameraSetZoom(&preview->camera, targetZoom) && fabsf(preview->camera.zoom - previousZoom) > 0.0001f) {
        // Adjust the camera so the point under the cursor stays fixed.
        preview->camera.position.x = focusWorld.x - local.x / preview->camera.zoom;
        preview->camera.position.y = focusWorld.y - local.y / preview->camera.zoom;
        LobbyPreviewClampCamera(preview, &viewRect);
    }

    // Always consume the wheel when inside the preview viewport.
    return true;
}

/**
 * Renders the preview panel and generated level, if visible.
 * @param preview Pointer to the LobbyPreviewContext to render.
 * @param lobbyUI Pointer to the lobby UI state for layout queries.
 * @param context OpenGL context used for rendering.
 */
void LobbyPreviewRender(LobbyPreviewContext *preview,
    const LobbyMenuUIState *lobbyUI,
    OpenGLContext *context) {
    if (preview == NULL || lobbyUI == NULL || context == NULL) {
        return;
    }

    if (!LobbyMenuUIIsPreviewOpen(lobbyUI)) {
        return;
    }

    MenuUIRect panelRect;
    MenuUIRect viewRect;
    if (!LobbyPreviewGetViewRect(lobbyUI, context, &panelRect, &viewRect)) {
        return;
    }

    // Draw the preview panel shell using menu colors for consistency.
    float panelOutline[4] = MENU_PANEL_OUTLINE_COLOR;
    float panelFill[4] = MENU_PANEL_FILL_COLOR;
    DrawOutlinedRectangle(panelRect.x, panelRect.y, panelRect.x + panelRect.width, panelRect.y + panelRect.height, panelOutline, panelFill);

    // Label the panel so players understand what the preview represents.
    float labelColor[4] = MENU_LABEL_TEXT_COLOR;
    float headerX = panelRect.x + LOBBY_MENU_PREVIEW_PANEL_PADDING;
    float headerY = panelRect.y + LOBBY_MENU_PREVIEW_PANEL_HEADER_HEIGHT - 6.0f;
    DrawScreenText(context, "Level Preview", headerX, headerY, MENU_LABEL_TEXT_HEIGHT, MENU_LABEL_TEXT_WIDTH, labelColor);

    if (!preview->levelInitialized) {
        float textX = panelRect.x + LOBBY_MENU_PREVIEW_PANEL_PADDING;
        float textY = panelRect.y + LOBBY_MENU_PREVIEW_PANEL_HEADER_HEIGHT + MENU_GENERIC_TEXT_HEIGHT;
        DrawScreenText(context, "Preview unavailable", textX, textY, MENU_GENERIC_TEXT_HEIGHT, MENU_GENERIC_TEXT_WIDTH, labelColor);
        return;
    }

    // Clip rendering to the preview viewport.
    GLint previousViewport[4] = {0, 0, 0, 0};

    // glGetIntegerv with previousViewport will essentially 
    // let us save the current viewport settings before we change them.
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    // glEnable GL_SCISSOR_TEST has the effect of limiting the drawing area
    // to the defined scissor box, which we set to the preview viewport.
    glEnable(GL_SCISSOR_TEST);

    // glScissor meanwhile defines the scissor box dimensions.
    glScissor((GLint)viewRect.x,
        (GLint)(context->height - (viewRect.y + viewRect.height)),
        (GLint)viewRect.width,
        (GLint)viewRect.height);

    // While glViewport defines the drawable area of the window.
    glViewport((GLint)viewRect.x,
        (GLint)(context->height - (viewRect.y + viewRect.height)),
        (GLint)viewRect.width,
        (GLint)viewRect.height);

    // Configure a projection matrix sized to the preview viewport.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, (GLdouble)viewRect.width, (GLdouble)viewRect.height, 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    DrawBackgroundGradient((int)viewRect.width, (int)viewRect.height);

    glPushMatrix();
    LobbyPreviewApplyCameraTransform(preview);

    // Draw each planet in the preview level.
    for (size_t i = 0; i < preview->level.planetCount; ++i) {
        PlanetDraw(&preview->level.planets[i]);
    }

    glPopMatrix();

    // Restore matrices and viewport state.
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_SCISSOR_TEST);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
}
