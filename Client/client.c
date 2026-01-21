/**
 * Companion client for Light Year Wars C.
 * Implements the client-side logic, rendering, and user interaction.
 * @author abmize
 * @file client.c
 */
#include "Client/client.h"

// ---- Global variables ----

// -- Client state variables --

// Defines the current stage of the client application.
// Controls what logic and rendering is to be performed.
static ClientStage currentStage = CLIENT_STAGE_LOGIN_MENU;

// -- Menu UI variables --

// Tracks the state of the login menu UI while waiting to connect.
static LoginMenuUIState loginMenuUI = {0};

// -- Window and rendering variables --

// Running = true while the main loop should continue
// Otherwise if false, the application will exit.
static bool running = true;

// Aggregated OpenGL resources shared by the server window.
static OpenGLContext openglContext = {0};

// -- Networking variables --

// UDP socket used for communication with the server
// Will be INVALID_SOCKET if not initialized.
static SOCKET clientSocket = INVALID_SOCKET;

// Server address information.
// Will be valid if serverAddressValid is true.
static SOCKADDR_IN serverAddress;

// Indicates whether the serverAddress variable contains a valid address.
static bool serverAddressValid = false;

// Buffer for receiving data from the server.
// Size is arbitrary but should be large enough to hold expected packets.
static char recv_buffer[16384];

// Time since the last valid packet was received from the server.
static float timeSinceLastServerPacket = 0.0f;

// Timeout duration in seconds before considering the server unresponsive.
static const float SERVER_TIMEOUT_SECONDS = (float)SERVER_TIMEOUT_MS / 1000.0f;

// -- Game state variables --

// Current level state.
static Level level;

// Indicates whether the level has been fully initialized
// with a full packet from the server.
// If false, the level is not ready for simulation or rendering,
// and the client should wait for a packet detailing the full state of the level.
static bool levelInitialized = false;

// Indicates whether the client is currently awaiting a full packet
// from the server to (re)initialize the level state.
static bool awaitingFull = true;

// ID of the faction assigned to this client by the server.
// Will be -1 if no faction has been assigned yet.
static int32_t assignedFactionId = -1;

// Pointer to the faction object representing the client's local faction.
// Will be NULL if no faction is assigned or if the level is not initialized.
static const Faction *localFaction = NULL;

// -- Player interaction state variables --

// Tracks the player's current planet selection state.
static PlayerSelectionState selectionState = {0};

// Tracks the player's control groups, which are
// collections of selected planets assigned to number keys.
// When the assigned number key is pressed, the selection
// changes to the planets in that control group.
static PlayerControlGroups controlGroups = {0};

// Tracks the camera used for rendering and interaction.
static CameraState cameraState = {0};

// Indicates whether box selection mode is active.
static bool boxSelectActive = false;

// Indicates whether the player is currently dragging a box selection.
static bool boxSelectDragging = false;

// Starting position of the box selection drag in terms of screen coordinates.
// Used to determine if the mouse has moved enough to be considered a drag.
static Vec2 boxSelectStartScreen = {0.0f, 0.0f};

// Starting position of the box selection drag in terms of world coordinates.
// Used for rendering the selection box and determining selected planets.
static Vec2 boxSelectStartWorld = {0.0f, 0.0f};

// Current position of the mouse during box selection drag in world coordinates.
// Used for rendering the selection box and determining selected planets.
static Vec2 boxSelectCurrentWorld = {0.0f, 0.0f};

// -- Timing variables --

// Previous tick count used for calculating delta time between frames.
static int64_t previousTicks = 0;

// Frequency of the high-resolution performance counter.
// Used to convert tick counts to seconds.
static int64_t tickFrequency = 1;

// Forward declarations of static functions.

static LRESULT CALLBACK WindowProcessMessage(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam);
static void DrawSelectionHighlights(void);
static Planet *PickPlanetAt(Vec2 position, size_t *outIndex);
static void RefreshLocalFaction(void);
static void HandleFullPacketMessage(const uint8_t *data, size_t length);
static void HandleSnapshotPacketMessage(const uint8_t *data, size_t length);
static void HandleAssignmentPacketMessage(const uint8_t *data, size_t length);
static void HandleFleetLaunchPacketMessage(const uint8_t *data, size_t length);
static void HandleServerShutdownPacketMessage(const uint8_t *data, size_t length);
static void ResetConnectionToMenu(const char *statusMessage);
static const Faction *ResolveFactionById(int32_t factionId);
static void ProcessNetworkMessages(void);
static void SendJoinRequest(void);
static void SendDisconnectNotice(void);
static void ProcessMenuConnectRequest(void);
static void RenderFrame(float fps);
static void DrawSelectionBox(void);
static void ApplyBoxSelection(bool additive);
static void HandleClickSelection(Vec2 mousePos, bool additive);
static int ControlGroupIndexFromKey(WPARAM key);
static void UpdateCamera(HWND window_handle, float deltaTime);
static void ClampCameraToLevel(void);
static void ApplyCameraTransform(void);
static Vec2 ScreenToWorld(Vec2 screen);
static Vec2 WorldToScreen(Vec2 world);
static void RefreshCameraBounds(void);

/**
 * Helper function to draw selection highlights
 * around planets selected by the player.
 */
static void DrawSelectionHighlights(void) {
    // If the level is not initialized or there is no selection state, do nothing.
    if (!levelInitialized || selectionState.selectedPlanets == NULL) {
        return;
    }

    // Draw a white, semi-transparent ring around each selected planet.
    float highlightColor[4] = {1.0f, 1.0f, 1.0f, 0.85f};

    // Limit the count to the smaller of planetCount and selectionState.capacity.
    // selectionState.capacity should always be equal to planetCount,
    // but this check ensures we don't read out of bounds.
    size_t count = level.planetCount < selectionState.capacity ? level.planetCount : selectionState.capacity;

    // Iterate through all planets and draw highlights for selected ones.
    for (size_t i = 0; i < count; ++i) {
        if (!selectionState.selectedPlanets[i]) {
            continue;
        }

        float radius = PlanetGetOuterRadius(&level.planets[i]);
        DrawFeatheredRing(level.planets[i].position.x, level.planets[i].position.y,
            radius + 2.0f, radius + 5.0f, 1.2f, highlightColor);
    }
}

/**
 * Helper function to convert a key press into a control group index.
 * Supports both number row keys and numpad keys.
 * @param key The WPARAM key code from a keyboard event.
 * @return The control group index (0-9) or -1 if the key is not a valid control group key.
 */
static int ControlGroupIndexFromKey(WPARAM key) {

    // A WPARAM, or "word parameter", is a 32-bit or 64-bit value
    // used in Windows programming to pass message parameters.
    // In keyboard events, it represents the virtual key code of the pressed key.
    // It just so happens that the virtual key codes for '0'-'9' and
    // VK_NUMPAD0-VK_NUMPAD9 are contiguous ranges,
    // and that '0' and VK_NUMPAD0 both represent the digit 0,
    // which we map to control group index 9, while '1'-'9' map to indices 0-8.
    // This is done to align with QWERTY keyboard layouts where '1' is the first number key
    // along the top row, and '0' is the tenth key.

    // Check for number row keys '0'-'9'.
    if (key >= '0' && key <= '9') {
        int value = (int)(key - '0');
        return value == 0 ? 9 : value - 1;
    }

    // Check for numpad keys VK_NUMPAD0-VK_NUMPAD9.
    if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9) {
        int value = (int)(key - VK_NUMPAD0);
        return value == 0 ? 9 : value - 1;
    }

    // Not a valid control group key.
    return -1;
}

/**
 * Helper function to draw the box selection rectangle
 * during a box selection drag operation.
 */
static void DrawSelectionBox(void) {
    // If box selection is not active or not dragging, do nothing.
    if (!boxSelectActive || !boxSelectDragging) {
        return;
    }

    // Calculate the corners of the selection box in screen space.

    // Minimum world coordinates of the selection box (smallest x and y).
    Vec2 minWorld = {fminf(boxSelectStartWorld.x, boxSelectCurrentWorld.x),
        fminf(boxSelectStartWorld.y, boxSelectCurrentWorld.y)};

    // Maximum world coordinates of the selection box (largest x and y).
    Vec2 maxWorld = {fmaxf(boxSelectStartWorld.x, boxSelectCurrentWorld.x),
        fmaxf(boxSelectStartWorld.y, boxSelectCurrentWorld.y)};

    // Convert world coordinates to screen coordinates for rendering.
    Vec2 topLeft = WorldToScreen(minWorld);
    Vec2 bottomRight = WorldToScreen(maxWorld);

    // Determine the rectangle corners in screen space.
    float minX = fminf(topLeft.x, bottomRight.x);
    float maxX = fmaxf(topLeft.x, bottomRight.x);
    float minY = fminf(topLeft.y, bottomRight.y);
    float maxY = fmaxf(topLeft.y, bottomRight.y);

    // Outline color: same as the assigned faction color but more opaque.
    // Otherwise defaults to a blueish color.
    float outlineColor[4] = {0.0f, 0.8f, 1.0f, 0.7f};
    if (localFaction != NULL) {
        outlineColor[0] = localFaction->color[0];
        outlineColor[1] = localFaction->color[1];
        outlineColor[2] = localFaction->color[2];
    }

    // Fill color: same as the assigned faction color but semi-transparent.
    // Otherwise defaults to a blueish color.
    float fillColor[4] = {0.0f, 0.6f, 1.0f, 0.18f};
    if (localFaction != NULL) {
        fillColor[0] = localFaction->color[0];
        fillColor[1] = localFaction->color[1];
        fillColor[2] = localFaction->color[2];
    }
    
    DrawOutlinedRectangle(minX, minY, maxX, maxY, outlineColor, fillColor);
}

/**
 * Helper function to apply the box selection
 * to the player's selection state.
 * @param additive When true, the selected planets are added to the existing selection.
 */
static void ApplyBoxSelection(bool additive) {
    // If the level is not initialized or there is no local faction, do nothing.
    if (!levelInitialized || localFaction == NULL) {
        return;
    }

    // If there is no selection state, do nothing.
    if (selectionState.selectedPlanets == NULL || selectionState.capacity == 0) {
        return;
    }

    // If not additive, clear the existing selection first.
    if (!additive) {
        PlayerSelectionClear(&selectionState);
    }

    // Determine the bounds of the selection box.
    float minX = fminf(boxSelectStartWorld.x, boxSelectCurrentWorld.x);
    float maxX = fmaxf(boxSelectStartWorld.x, boxSelectCurrentWorld.x);
    float minY = fminf(boxSelectStartWorld.y, boxSelectCurrentWorld.y);
    float maxY = fmaxf(boxSelectStartWorld.y, boxSelectCurrentWorld.y);

    // Iterate through all planets and select those within the box
    // that are owned by the local faction.
    size_t limit = level.planetCount < selectionState.capacity ? level.planetCount : selectionState.capacity;
    for (size_t i = 0; i < limit; ++i) {
        const Planet *planet = &level.planets[i];
        if (planet->owner != localFaction) {
            continue;
        }

        // A planet's center position must be within the box to be selected.

        // Check if the planet's position is within the selection box.
        float px = planet->position.x;
        float py = planet->position.y;
        if (px < minX || px > maxX || py < minY || py > maxY) {
            continue;
        }

        // Set the planet as selected in this client's selection state.
        PlayerSelectionSet(&selectionState, i, true);
    }
}

/**
 * Helper function to handle a click selection
 * at the given mouse position.
 * @param mousePos The position of the mouse click.
 * @param additive When true, the clicked planet is added to the existing selection.
 */
static void HandleClickSelection(Vec2 mousePos, bool additive) {
    // If the level is not initialized or there is no local faction, 
    // there is no valid selection to be made.
    if (!levelInitialized || localFaction == NULL) {
        PlayerSelectionClear(&selectionState);
        return;
    }

    // Find the planet at the clicked position.
    size_t planetIndex = 0;
    Planet *clicked = PickPlanetAt(mousePos, &planetIndex);

    // If no planet was clicked, clear selection if not additive.
    if (clicked == NULL) {
        if (!additive) {
            PlayerSelectionClear(&selectionState);
        }
        return;
    }

    // If the clicked planet is not owned by the local faction,
    // clear selection if not additive.
    if (clicked->owner != localFaction) {
        if (!additive) {
            PlayerSelectionClear(&selectionState);
        }
        return;
    }

    // Toggle the selection state of the clicked planet.
    PlayerSelectionToggle(&selectionState, planetIndex, additive);
}

/**
 * Helper function to pick a planet at the given position.
 * Used for mouse picking. Will find the first planet whose outer radius
 * contains the given position.
 * @param position The position to check for a planet.
 * @param outIndex Optional output parameter to receive the index of the 
 *                 picked planet in the level's planet array.
 * @return Pointer to the picked Planet, or NULL if none found.
 */
static Planet *PickPlanetAt(Vec2 position, size_t *outIndex) {
    // No level, no planets.
    if (!levelInitialized) {
        return NULL;
    }

    // Simple approach which is fast enough as there will be bigger issues
    // with performance before this becomes a bottleneck: just iterate all planets
    // and check distance between the position and the planet center.
    for (size_t i = 0; i < level.planetCount; ++i) {
        Vec2 diff = Vec2Subtract(position, level.planets[i].position);
        float dist = Vec2Length(diff);

        // The outer radius is the biggest visible part of the planet,
        // so if the distance is less than that, we've clicked on the planet.
        if (dist < PlanetGetOuterRadius(&level.planets[i])) {

            // If requested, output the index of the picked planet.
            if (outIndex != NULL) {
                *outIndex = i;
            }
            return &level.planets[i];
        }
    }

    return NULL;
}

/**
 * Helper function to convert screen coordinates to world coordinates
 * using the current camera state.
 * Used when interpreting mouse positions into something like selections in the worlds space.
 * @param screen The screen coordinates to convert.
 * @return The corresponding world coordinates.
 */
static Vec2 ScreenToWorld(Vec2 screen) {
    return CameraScreenToWorld(&cameraState, screen);
}

/**
 * Helper function to convert world coordinates to screen coordinates
 * using the current camera state.
 * Used when rendering world objects to the screen.
 * @param world The world coordinates to convert.
 * @return The corresponding screen coordinates.
 */
static Vec2 WorldToScreen(Vec2 world) {
    return CameraWorldToScreen(&cameraState, world);
}

/**
 * Helper function to clamp the camera position
 * to ensure it does not go outside the level bounds.
 * Prevents players from losing track of where they are in the level
 * by endlessly panning the camera off into empty space.
 */
static void ClampCameraToLevel(void) {
    // If the camera zoom is invalid or the viewport size is zero,
    // there's nothing that needs to be done.
    if (cameraState.zoom <= 0.0f || openglContext.width <= 0 || openglContext.height <= 0) {
        return;
    }

    // Calculate the size of the viewport in world units,
    // taking zoom levels into account.
    float viewWidth = (float)openglContext.width / cameraState.zoom;
    float viewHeight = (float)openglContext.height / cameraState.zoom;

    // Delegate to the utility function to clamp the camera position.
    CameraClampToBounds(&cameraState, viewWidth, viewHeight);
}

/**
 * Helper function to refresh the camera bounds
 * based on the current level size.
 * Should be called whenever the level is initialized or resized.
 */
static void RefreshCameraBounds(void) {
    // Set the camera bounds to match the level dimensions.
    CameraSetBounds(&cameraState, level.width, level.height);

    // Clamp the camera position to ensure it is within the new bounds.
    ClampCameraToLevel();
}

/**
 * Helper function to apply the camera transformation
 * to the current OpenGL modelview matrix.
 * Sets up the scaling and translation based on the camera state.
 * Used when panning or zooming the view.
 */
static void ApplyCameraTransform(void) {
    // If the camera zoom is less than zero,
    // there's nothing to see.
    if (cameraState.zoom <= 0.0f) {
        return;
    }

    // Scale the view according to the zoom level,
    glScalef(cameraState.zoom, cameraState.zoom, 1.0f);

    // Then translate the view to center on the camera position.
    glTranslatef(-cameraState.position.x, -cameraState.position.y, 0.0f);
}

/**
 * Helper function to update the camera position
 * based on user input and elapsed time.
 * Supports keyboard panning and edge panning.
 * @param window_handle The handle to the application window.
 * @param deltaTime The time elapsed since the last update, in seconds.
 */
static void UpdateCamera(HWND window_handle, float deltaTime) {
    // If the level is not initialized or deltaTime is invalid,
    // there's nothing to do.
    if (!levelInitialized || deltaTime <= 0.0f) {
        return;
    }

    // Determine if the window is active or has input capture.
    bool hasCapture = GetCapture() == window_handle;
    bool windowActive = GetForegroundWindow() == window_handle;

    // Only allow camera movement if the window is active or has capture.
    bool allowInput = windowActive || hasCapture;

    // Used to compute how much to move the camera.
    Vec2 displacement = Vec2Zero();

    // Handle keyboard input for panning.
    if (allowInput) {
        // Default is no direction
        Vec2 keyDirection = Vec2Zero();

        // GetAsyncKeyState determines whether a key is up or down at the time the function 
        // is called the key, and whether it has been pressed since a previous call to GetAsyncKeyState.
        // WASD or Arrow Keys: On QWERTY keyboards these are the standard movement keys.
        // A = left, D = right, W = up, S = down.
        if ((GetAsyncKeyState(VK_LEFT) & 0x8000) != 0 || (GetAsyncKeyState('A') & 0x8000) != 0) {
            keyDirection.x -= 1.0f;
        }

        if ((GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0 || (GetAsyncKeyState('D') & 0x8000) != 0) {
            keyDirection.x += 1.0f;
        }

        if ((GetAsyncKeyState(VK_UP) & 0x8000) != 0 || (GetAsyncKeyState('W') & 0x8000) != 0) {
            keyDirection.y -= 1.0f;
        }
        
        if ((GetAsyncKeyState(VK_DOWN) & 0x8000) != 0 || (GetAsyncKeyState('S') & 0x8000) != 0) {
            keyDirection.y += 1.0f;
        }

        // Normalize the direction and apply speed.
        if (keyDirection.x != 0.0f || keyDirection.y != 0.0f) {
            keyDirection = Vec2Normalize(keyDirection);
            float speed = CAMERA_KEY_PAN_SPEED * deltaTime / cameraState.zoom;
            displacement.x += keyDirection.x * speed;
            displacement.y += keyDirection.y * speed;
        }
    }

    // Handle edge panning based on mouse position.
    POINT cursorPosition;
    if (GetCursorPos(&cursorPosition)) {

        // Convert the cursor position from screen coordinates to client coordinates.
        POINT clientPoint = cursorPosition;
        ScreenToClient(window_handle, &clientPoint);

        // Only apply edge panning if input is allowed and the viewport size is valid.
        if (allowInput && openglContext.width > 0 && openglContext.height > 0) {

            // Default is no direction
            Vec2 edgeDirection = Vec2Zero();

            // If the mouse is within CAMERA_EDGE_PAN_MARGIN pixels of any particular edge,
            // with a preference for left/top edges when exactly on the margin of both sides,
            // pan the camera in that direction.
            if (clientPoint.x <= CAMERA_EDGE_PAN_MARGIN) {
                edgeDirection.x -= 1.0f;
            } else if (clientPoint.x >= openglContext.width - CAMERA_EDGE_PAN_MARGIN) {
                edgeDirection.x += 1.0f;
            }

            if (clientPoint.y <= CAMERA_EDGE_PAN_MARGIN) {
                edgeDirection.y -= 1.0f;
            } else if (clientPoint.y >= openglContext.height - CAMERA_EDGE_PAN_MARGIN) {
                edgeDirection.y += 1.0f;
            }

            // Normalize the direction and apply speed.
            if (edgeDirection.x != 0.0f || edgeDirection.y != 0.0f) {
                edgeDirection = Vec2Normalize(edgeDirection);
                float speed = CAMERA_EDGE_PAN_SPEED * deltaTime / cameraState.zoom;
                displacement.x += edgeDirection.x * speed;
                displacement.y += edgeDirection.y * speed;
            }
        }

        // If we are also doing box selection, update the current box selection position
        // so that way the box can be drawn correctly.
        if (boxSelectActive) {
            Vec2 screen = {(float)clientPoint.x, (float)clientPoint.y};
            boxSelectCurrentWorld = ScreenToWorld(screen);

            if (!boxSelectDragging) {
                float dx = screen.x - boxSelectStartScreen.x;
                float dy = screen.y - boxSelectStartScreen.y;
                if (fabsf(dx) >= BOX_SELECT_DRAG_THRESHOLD || fabsf(dy) >= BOX_SELECT_DRAG_THRESHOLD) {
                    boxSelectDragging = true;
                }
            }
        }
    }

    // If there is any displacement, update the camera position
    if (displacement.x != 0.0f || displacement.y != 0.0f) {
        cameraState.position = Vec2Add(cameraState.position, displacement);
        ClampCameraToLevel();
    }
}

/**
 * Helper function to resolve a faction by its ID.
 * Searches the current level's factions for a matching ID.
 * @param factionId The ID of the faction to find.
 * @return Pointer to the Faction if found, NULL otherwise.
 */
static const Faction *ResolveFactionById(int32_t factionId) {
    // If the level is not initialized or has no factions, cannot resolve.
    if (!levelInitialized || level.factions == NULL) {
        return NULL;
    }

    // Search through the factions to find one with the matching ID.
    for (size_t i = 0; i < level.factionCount; ++i) {
        if (level.factions[i].id == factionId) {
            return &level.factions[i];
        }
    }

    return NULL;
}

/**
 * Helper function to refresh the local faction pointer
 * based on the assigned faction ID and current level state.
 * Used whenever the level is initialized or the assignment changes.
 */
static void RefreshLocalFaction(void) {
    localFaction = NULL;
    if (!levelInitialized || assignedFactionId < 0) {
        return;
    }

    localFaction = ResolveFactionById(assignedFactionId);
}

/**
 * Handles a full packet message received from the server.
 * Applies the full level state contained in the packet.
 * Will fill in the level structure and mark it as initialized.
 * Will also reset player selection, control groups, and camera state.
 * Sets the client stage to the game stage upon success.
 * @param data Pointer to the packet data.
 * @param length Length of the packet data.
 */
static void HandleFullPacketMessage(const uint8_t *data, size_t length) {
    // Delegate to LevelApplyFullPacket to handle the actual data.
    if (!LevelApplyFullPacket(&level, data, length)) {
        printf("Failed to apply full packet.\n");

        // We let the client know in the login menu UI if we failed to load the level.
        if (currentStage == CLIENT_STAGE_LOGIN_MENU) {
            LoginMenuUISetStatusMessage(&loginMenuUI, "Failed to load level data from server.");
        }
        return;
    }

    // Mark the level as initialized and reset selection state,
    // in case of a reinitialization.
    levelInitialized = true;
    awaitingFull = false;

    // Reset the player's selection state in case they
    // had any selections prior to the full packet.
    if (!PlayerSelectionReset(&selectionState, level.planetCount)) {
        printf("Failed to reset selection state.\n");
    }

    // Also reset control groups to clear any prior groups that may 
    // be out of sync with the new level state.
    if (!PlayerControlGroupsReset(&controlGroups, level.planetCount)) {
        printf("Failed to reset control groups.\n");
    }

    // Reset box selection state.
    boxSelectActive = false;
    boxSelectDragging = false;

    // Reset camera to default position and zoom.
    cameraState.position = Vec2Zero();
    CameraSetZoom(&cameraState, 1.0f);
    RefreshCameraBounds();
    RefreshLocalFaction();

    // Update the min zoom based on the new level size.
    cameraState.minZoom = CAMERA_MIN_ZOOM / (fmaxf(level.width, level.height) / 2000.0f);

    // Switch to the appropriate stage for gameplay.
    currentStage = CLIENT_STAGE_GAME;
}

/**
 * Handles a snapshot packet message received from the server.
 * Applies the snapshot data to update the current level state.
 * @param data Pointer to the packet data.
 * @param length Length of the packet data.
 */
static void HandleSnapshotPacketMessage(const uint8_t *data, size_t length) {
    // If the level is not initialized, cannot apply a snapshot.
    if (!levelInitialized) {
        return;
    }

    // Delegate to LevelApplySnapshot to handle the actual data.
    if (!LevelApplySnapshot(&level, data, length)) {
        printf("Failed to apply snapshot packet.\n");
        awaitingFull = true;
        return;
    }

    // We successfully applied the snapshot,
    // make sure that the local faction pointer is up to date.
    RefreshLocalFaction();

    // And refresh camera bounds in case the level size changed.
    RefreshCameraBounds();
}

/**
 * Handles an assignment packet message received from the server.
 * An assignment packet assigns a faction to the client.
 * @param data Pointer to the packet data.
 * @param length Length of the packet data.
 */
static void HandleAssignmentPacketMessage(const uint8_t *data, size_t length) {
    // If the level is not initialized, we can still process the assignment.
    if (length < sizeof(LevelAssignmentPacket)) {
        return;
    }

    // Cast the data to a LevelAssignmentPacket and extract the faction ID.
    const LevelAssignmentPacket *packet = (const LevelAssignmentPacket *)data;
    if (packet->type != LEVEL_PACKET_TYPE_ASSIGNMENT) {
        return;
    }

    // Update the assigned faction ID and refresh the local faction pointer.
    assignedFactionId = packet->factionId;
    RefreshLocalFaction();
}

/**
 * Helper function to reset the client connection
 * and return to the login menu.
 * Cleans up all session state and closes the socket.
 * @param statusMessage Optional status message to display in the login menu.
 */
static void ResetConnectionToMenu(const char *statusMessage) {
    // Best effort to close the existing socket so we stop receiving packets.
    if (clientSocket != INVALID_SOCKET) {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }

    // Reset server address state.
    serverAddressValid = false;
    awaitingFull = false;
    levelInitialized = false;
    assignedFactionId = -1;
    localFaction = NULL;
    timeSinceLastServerPacket = 0.0f;

    // Clear any player interaction state so we start fresh when reconnecting.
    PlayerSelectionReset(&selectionState, 0);
    PlayerControlGroupsReset(&controlGroups, 0);
    boxSelectActive = false;
    boxSelectDragging = false;

    // Release the level data so stale state is not reused.
    LevelRelease(&level);

    // Reset camera so menu rendering starts from a known state.
    cameraState.position = Vec2Zero();
    CameraSetZoom(&cameraState, 1.0f);
    RefreshCameraBounds();

    // Return to the login menu and inform the user.
    currentStage = CLIENT_STAGE_LOGIN_MENU;
    if (statusMessage != NULL) {
        LoginMenuUISetStatusMessage(&loginMenuUI, statusMessage);
    }
}

/**
 * Handles a server shutdown packet message received from the server.
 * Forces the client back to the login menu and tears down any active session.
 * @param data Pointer to the packet data.
 * @param length Length of the packet data.
 */
static void HandleServerShutdownPacketMessage(const uint8_t *data, size_t length) {
    // Basic validation of input parameters.
    if (data == NULL || length < sizeof(LevelServerShutdownPacket)) {
        return;
    }

    // Cast the data to a LevelServerShutdownPacket and verify the type.
    const LevelServerShutdownPacket *packet = (const LevelServerShutdownPacket *)data;
    if (packet->type != LEVEL_PACKET_TYPE_SERVER_SHUTDOWN) {
        return;
    }

    // Clean up all client state related to the active session.
    // This includes:
    // - Closing the client socket
    // - Resetting server address state
    // - Resetting level state
    // - Resetting player interaction state
    // - Resetting camera state
    // - Returning to the login menu

    ResetConnectionToMenu("Disconnected: server closed.");
}

/**
 * Handles a fleet launch packet message received from the server.
 * Processes the fleet launch information and updates the level state.
 * Allows clients to simulate fleet launches initiated by other players,
 * without needing to receive any information about the starships themselves.
 * @param data Pointer to the packet data.
 * @param length Length of the packet data.
 */
static void HandleFleetLaunchPacketMessage(const uint8_t *data, size_t length) {
    // Basic validation of input parameters.
    if (!levelInitialized || data == NULL) {
        return;
    }

    if (length < sizeof(LevelFleetLaunchPacket)) {
        return;
    }

    // Cast the data to a LevelFleetLaunchPacket and extract the information.
    const LevelFleetLaunchPacket *packet = (const LevelFleetLaunchPacket *)data;
    if (packet->type != LEVEL_PACKET_TYPE_FLEET_LAUNCH) {
        return;
    }

    // Extract launch details from the packet.
    int32_t originIndex = packet->originPlanetIndex;
    int32_t destinationIndex = packet->destinationPlanetIndex;
    int32_t shipCount = packet->shipCount;
    unsigned int shipSpawnRNGState = packet->shipSpawnRNGState;

    // Validate the extracted launch details.
    if (originIndex < 0 || destinationIndex < 0 || shipCount <= 0) {
        return;
    }

    if ((size_t)originIndex >= level.planetCount || (size_t)destinationIndex >= level.planetCount) {
        return;
    }

    // Figure out the origin and destination planets in terms of this client's level state.
    Planet *origin = &level.planets[originIndex];
    Planet *destination = &level.planets[destinationIndex];

    // Determine the owner faction for the launched fleet.
    const Faction *owner = ResolveFactionById(packet->ownerFactionId);
    if (owner == NULL) {
        owner = origin->owner;
    }

    // Simulate the fleet launch on the client side.
    if (!PlanetSimulateFleetLaunch(origin, destination, &level, shipCount, owner, &shipSpawnRNGState)) {
        return;
    }
}

/**
 * Processes incoming network messages from the server.
 * Handles different packet types and updates the level state accordingly.
 */
static void ProcessNetworkMessages(void) {
    // If the client socket is not valid, there is nothing to process.
    if (clientSocket == INVALID_SOCKET) {
        return;
    }

    // Continuously receive packets until there are no more to process.
    // for (;;) loop will break when there are no more packets.
    for (;;) {
        // Receive data from the server.
        SOCKADDR_IN fromAddress;

        // Figure out the size of the fromAddress structure.
        int fromLength = sizeof(fromAddress);

        // Receive data into the recv_buffer.
        int received = recvfrom(clientSocket,
            recv_buffer,
            (int)sizeof(recv_buffer),
            0,
            (struct sockaddr *)&fromAddress,
            &fromLength);

        // In case of an error, break the loop.
        // This could include WSAEWOULDBLOCK, which
        // should means that there are no more packets to read.
        // Other errors could indicate a real problem, and are thus logged.
        if (received == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                printf("recvfrom failed: %d\n", error);
            }
            break;
        }

        // If no data was received, continue to the next iteration.
        if (received == 0) {
            continue;
        }

        // If a server address is set, ignore packets from other addresses.
        if (serverAddressValid && fromAddress.sin_addr.s_addr != serverAddress.sin_addr.s_addr) {
            continue;
        }

        timeSinceLastServerPacket = 0.0f;

        // If the received data is smaller than a uint32_t,
        // we cannot determine the packet type.
        // That is because we assume the first 4 bytes of every packet
        // contains the packet type as a uint32_t.
        if (received < (int)sizeof(uint32_t)) {
            int capped = received < (int)sizeof(recv_buffer) - 1 ? received : (int)sizeof(recv_buffer) - 1;
            recv_buffer[capped] = '\0';
            printf("Server: %s\n", recv_buffer);
            continue;
        }

        // Extract the packet type from the beginning of the received data.
        uint32_t packetType = 0;
        memcpy(&packetType, recv_buffer, sizeof(uint32_t));
        const uint8_t *payload = (const uint8_t *)recv_buffer;
        size_t payloadSize = (size_t)received;

        // Delegate handling based on the packet type.
        if (packetType == LEVEL_PACKET_TYPE_FULL) {
            HandleFullPacketMessage(payload, payloadSize);
        } else if (packetType == LEVEL_PACKET_TYPE_SNAPSHOT) {
            HandleSnapshotPacketMessage(payload, payloadSize);
        } else if (packetType == LEVEL_PACKET_TYPE_ASSIGNMENT) {
            HandleAssignmentPacketMessage(payload, payloadSize);
        } else if (packetType == LEVEL_PACKET_TYPE_FLEET_LAUNCH) {
            HandleFleetLaunchPacketMessage(payload, payloadSize);
        } else if (packetType == LEVEL_PACKET_TYPE_SERVER_SHUTDOWN) {
            HandleServerShutdownPacketMessage(payload, payloadSize);
            break;
        } else {
            printf("Unknown packet type %u (%d bytes).\n", packetType, received);
        }
    }
}

/**
 * Sends a join request to the server.
 * This is used to request joining the game session.
 * Upon receiving a join request, the server should respond
 * with a full level packet and an assignment packet.
 */
static void SendJoinRequest(void) {
    // If the server address is not valid or the client socket is not valid, do nothing.
    if (!serverAddressValid || clientSocket == INVALID_SOCKET) {
        return;
    }

    // Prepare and send the join request message.
    // For now, this is a simple text message "JOIN",
    // with the client's IP also implicitly known on the server side
    // from the source address of the received packet.
    const char *joinMessage = "JOIN";
    int result = sendto(clientSocket,
        joinMessage,
        (int)strlen(joinMessage),
        0,
        (struct sockaddr *)&serverAddress,
        (int)sizeof(serverAddress));

    // Report any send errors.
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        printf("sendto failed: %d\n", error);

        // In case of an error while in the menu stage,
        // let the user know via the menu UI.
        if (currentStage == CLIENT_STAGE_LOGIN_MENU) {
            char status[LOGIN_MENU_STATUS_MAX_LENGTH + 1];
            snprintf(status, sizeof(status), "Failed to send join request (error %d).", error);
            LoginMenuUISetStatusMessage(&loginMenuUI, status);
        }
    }
}

/**
 * Sends a disconnect notice to the server so it can free the client's slot.
 * Best effort only; failures are logged but otherwise ignored.
 */
static void SendDisconnectNotice(void) {
    // If the server address is not valid or the client socket is not valid, do nothing.
    if (!serverAddressValid || clientSocket == INVALID_SOCKET) {
        return;
    }

    // Prepare and send the disconnect packet.
    LevelClientDisconnectPacket packet = {0};
    packet.type = LEVEL_PACKET_TYPE_CLIENT_DISCONNECT;

    int result = sendto(clientSocket,
        (const char *)&packet,
        (int)sizeof(packet),
        0,
        (struct sockaddr *)&serverAddress,
        (int)sizeof(serverAddress));

    // Report any send errors.
    if (result == SOCKET_ERROR) {
        printf("disconnect notice sendto failed: %d\n", WSAGetLastError());
    }
}

/**
 * Processes a connect request from the menu UI.
 * Validates the input IP address and port,
 * creates a UDP socket, and sends a join request to the server.
 */
static void ProcessMenuConnectRequest(void) {
    // Allocate buffers for the IP address and port.
    char ip[LOGIN_MENU_IP_MAX_LENGTH + 1];
    char portBuffer[LOGIN_MENU_PORT_MAX_LENGTH + 1];

    // Try to see if the user has requested to connect to a server.
    // This function must be called regularly to check for requests 
    // (triggered by the user clicking the Connect button).
    if (!LoginMenuUIConsumeConnectRequest(&loginMenuUI, ip, sizeof(ip), portBuffer, sizeof(portBuffer))) {
        // No connect request to process.
        return;
    }

    // At this point, we have a connect request to process.
    // First, validate the input IP address and port.

    if (serverAddressValid) {
        SendDisconnectNotice();
    }

    serverAddressValid = false;
    memset(&serverAddress, 0, sizeof(serverAddress));

    // Clean up any leading/trailing whitespace from the inputs.
    TrimWhitespace(ip);
    TrimWhitespace(portBuffer);

    // Validate IP address.
    if (ip[0] == '\0') {
        // Triggered if the user did not enter an IP address.
        LoginMenuUISetStatusMessage(&loginMenuUI, "Please enter a server IP address.");
        return;
    }

    if (portBuffer[0] == '\0') {
        // Triggered if the user did not enter a port.
        LoginMenuUISetStatusMessage(&loginMenuUI, "Please enter a server port.");
        return;
    }

    // Pointer to track the end of the parsed number.
    char *endPtr = NULL;

    // We convert the port string to a 64 bit integer not
    // because somehow port numbers could be that big, but rather
    // since we want to tell less well informed users that the port must be
    // between 1 and 65535, rather than just "not a valid number",
    // or worse yet, have a long get type cast into a 16 bit unsigned integer
    // which would be registered as a valid port number, but not the one the user intended.

    long portValue = strtol(portBuffer, &endPtr, 10);
    if (endPtr == NULL || *endPtr != '\0') {
        LoginMenuUISetStatusMessage(&loginMenuUI, "Port must be a number.");
        return;
    }

    if (portValue <= 0 || portValue > 65535) {
        LoginMenuUISetStatusMessage(&loginMenuUI, "Port must be between 1 and 65535.");
        return;
    }

    SOCKADDR_IN newAddress = {0};
    if (!CreateAddress(ip, (int)portValue, &newAddress)) {
        LoginMenuUISetStatusMessage(&loginMenuUI, "Failed to parse server address.");
        return;
    }

    // Port and IP seem to be valid, proceed to create the socket and connect. 

    // Clean up any existing socket first.
    if (clientSocket != INVALID_SOCKET) {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }

    // Create a new UDP socket.
    clientSocket = CreateUDPSocket();
    if (clientSocket == INVALID_SOCKET) {
        LoginMenuUISetStatusMessage(&loginMenuUI, "Failed to create UDP socket.");
        return;
    }

    // The socket must be non-blocking to avoid stalling the main loop
    // when playing the game. It's a real time application after all.
    // Of course, if in the future the application is multi-threaded
    // such that network operations are done in a separate thread,
    // this requirement could be relaxed.

    if (!SetNonBlocking(clientSocket)) {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
        LoginMenuUISetStatusMessage(&loginMenuUI, "Failed to configure socket.");
        return;
    }

    serverAddress = newAddress;
    serverAddressValid = true;

    // Mark that we are now awaiting a full level packet
    // and (re)set relevant state for the game.
    
    awaitingFull = true;
    levelInitialized = false;
    assignedFactionId = -1;
    localFaction = NULL;
    timeSinceLastServerPacket = 0.0f;

    // While waiting for the full packet, update the login menu status message
    // so the user knows we are trying to connect.
    char status[LOGIN_MENU_STATUS_MAX_LENGTH + 1];
    snprintf(status, sizeof(status), "Connecting to %s:%ld...", ip, portValue);
    LoginMenuUISetStatusMessage(&loginMenuUI, status);

    // Delegate to helper function to send the actual join request.
    SendJoinRequest();
}

/**
 * Renders a single frame of the client.
 * Draws the background, planets, starships, trails, and UI elements.
 * @param fps The current frames per second, used for the FPS display.
 */
static void RenderFrame(float fps) {
    // If OpenGL is not initialized, do nothing.
    if (!openglContext.deviceContext || !openglContext.renderContext) {
        return;
    }

    // Clear the screen to the background color.
    glClearColor(BACKGROUND_COLOR_R, BACKGROUND_COLOR_G, BACKGROUND_COLOR_B, BACKGROUND_COLOR_A);
    glClear(GL_COLOR_BUFFER_BIT);

    // Reset the modelview matrix.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // If we have a valid window size, draw the game elements.
    if (openglContext.width > 0 && openglContext.height > 0) {
        // Add a soft glow gradient to the background.
        DrawBackgroundGradient(openglContext.width, openglContext.height);

        // Depending on the current stage, draw either the game or the menu UI.
        if (currentStage == CLIENT_STAGE_GAME && levelInitialized) {
            // Save the current matrix state.
            glPushMatrix();

            // Then apply the current camera settings to update the view.
            ApplyCameraTransform();

            // Draw each planet in the level.
            for (size_t i = 0; i < level.planetCount; ++i) {
                PlanetDraw(&level.planets[i]);
            }

            // Draw selection highlights around selected planets.
            DrawSelectionHighlights();

            // Draw each starship trail effect.
            for (size_t i = 0; i < level.trailEffectCount; ++i) {
                StarshipTrailEffectDraw(&level.trailEffects[i]);
            }

            // Draw each starship in the level.
            for (size_t i = 0; i < level.starshipCount; ++i) {
                StarshipDraw(&level.starships[i]);
            }

            // Restore the previous matrix state.
            glPopMatrix();
        }
    }

    // Don't need the level to be initialized to draw some UI elements.
    if (currentStage == CLIENT_STAGE_GAME) {
        // In case we are doing box selection, draw the selection box.
        DrawSelectionBox();

        // Draw the FPS, faction ID, and selection count in the top-left corner.
        int textPositionFromTop = 20;
        int textPositionFromLeft = 10;
        if (levelInitialized && openglContext.width >= textPositionFromLeft && openglContext.height >= textPositionFromTop) {
            char infoString[160];
            int selectionCount = selectionState.count;
            int factionId = assignedFactionId >= 0 ? assignedFactionId : -1;
            snprintf(infoString, sizeof(infoString),
                "FPS: %.0f\nFaction ID: %d\nNumber of Selected Planets: %d",
                fps,
                factionId,
                selectionCount);

            float textColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
            float textSize = 16.0f;
            DrawScreenText(&openglContext, infoString, (float)textPositionFromLeft, (float)textPositionFromTop, textSize, textSize / 2, textColor);
        }
    } else {
        // Only true if we are in the login menu stage 
        // (CLIENT_STAGE_LOGIN_MENU and CLIENT_STAGE_GAME are the only stages supported currently).
        LoginMenuUIDraw(&loginMenuUI, &openglContext, openglContext.width, openglContext.height);
    }

    // Swap the front and back buffers to display the rendered frame.
    // There's two buffers, one being displayed while the other is drawn to.
    // Swapping them makes the newly drawn frame visible, while taking the
    // previously displayed buffer off-screen for the next frame's drawing.
    SwapBuffers(openglContext.deviceContext);
}

/**
 * Window procedure that handles messages sent to the window.
 * @param window_handle Handle to the window.
 * @param msg The message.
 * @param wParam Additional message information.
 * @param lParam Additional message information.
 * @return The result of the message processing and depends on the message sent.
*/
LRESULT CALLBACK WindowProcessMessage(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        // WM_CLOSE is sent when the user tries to close the window.
        // Compared to WM_DESTROY, this message can be ignored or canceled.
        case WM_CLOSE:
            DestroyWindow(window_handle);
            return 0;

        // WM_DESTROY is sent when the window is being destroyed.
        // Compared to WM_CLOSE, this is a more final message.
        case WM_DESTROY:
            SendDisconnectNotice();
            running = false;
            PostQuitMessage(0);
            return 0;

        //All window drawing has to happen inside the WM_PAINT message.
        case WM_PAINT: {
            PAINTSTRUCT paint;

            //In order to enable window drawing, BeginPaint must be called.
            //It fills out the PAINTSTRUCT and gives a device context handle for painting
            BeginPaint(window_handle, &paint);

            //We don't actually use the device context from BeginPaint,
            //as we are using OpenGL for all rendering.

            //If end paint is not called, everything seems to work, 
            //but the documentation says that it is necessary.
            EndPaint(window_handle, &paint);
            return 0;
        }
        
        //WM_SIZE is sent when the window is created or resized.
        //This makes it an ideal place to assign the size of the OpenGL viewport.
        case WM_SIZE: {
            // LOWORD and HIWORD extract the low-order and high-order words from lParam,
            // as it is expected that if a WM_SIZE message is sent,
            // the width and height of the client area are packed into lParam.
            int newWidth = LOWORD(lParam);
            int newHeight = HIWORD(lParam);

            // We need to update the OpenGL projection matrix to match the new window size.
            // If we don't do this, the aspect ratio will be incorrect
            // and the rendered scene will appear stretched or squashed.
            OpenGLUpdateProjection(&openglContext, newWidth, newHeight);
            ClampCameraToLevel();
            return 0;
        }

        // This message indicates a mouse button was pressed.
        // lParam contains the x and y coordinates of the mouse cursor.
        case WM_LBUTTONDOWN: {
            int x = (int)(short)LOWORD(lParam);
            int y = (int)(short)HIWORD(lParam);

            if (currentStage == CLIENT_STAGE_LOGIN_MENU) {
                LoginMenuUIHandleMouseDown(&loginMenuUI, (float)x, (float)y, openglContext.width, openglContext.height);
                return 0;
            }

            // Nothing to select if the level is not initialized or there is no local faction.
            if (!levelInitialized || localFaction == NULL) {
                PlayerSelectionClear(&selectionState);
                return 0;
            }

            // Get the mouse position from lParam.
            Vec2 screen = {(float)x, (float)y};

            // Initialize box selection state.
            boxSelectStartScreen = screen;

            // Convert screen coordinates to world coordinates.
            boxSelectStartWorld = ScreenToWorld(screen);
            boxSelectCurrentWorld = boxSelectStartWorld;

            boxSelectActive = true;
            boxSelectDragging = false;

            // Capture the mouse input to this window
            // so we continue to receive mouse move and button up events
            // even if the cursor leaves the window area.
            SetCapture(window_handle);
            return 0;
        }

        // This message indicates the mouse has moved.
        // wParam contains flags indicating which buttons are pressed.
        // lParam contains the x and y coordinates of the mouse cursor.
        case WM_MOUSEMOVE: {
            int x = (int)(short)LOWORD(lParam);
            int y = (int)(short)HIWORD(lParam);

            if (currentStage == CLIENT_STAGE_LOGIN_MENU) {
                LoginMenuUIHandleMouseMove(&loginMenuUI, (float)x, (float)y);
                return 0;
            }

            if (!boxSelectActive || (wParam & MK_LBUTTON) == 0) {
                return 0;
            }

            // Get the current mouse position from lParam.
            Vec2 screen = {(float)x, (float)y};
            boxSelectCurrentWorld = ScreenToWorld(screen);

            // If the mouse has not yet moved enough to be considered dragging,
            // check if it has now exceeded the drag threshold.
            if (!boxSelectDragging) {
                float dx = screen.x - boxSelectStartScreen.x;
                float dy = screen.y - boxSelectStartScreen.y;
                if (fabsf(dx) >= BOX_SELECT_DRAG_THRESHOLD || fabsf(dy) >= BOX_SELECT_DRAG_THRESHOLD) {
                    // If so, mark as dragging so we can start drawing the selection box
                    // and apply box selection on mouse up.
                    boxSelectDragging = true;
                }
            }
            return 0;
        }

        // This message indicates a mouse button was released.
        // lParam contains the x and y coordinates of the mouse cursor.
        case WM_LBUTTONUP: {
            int x = (int)(short)LOWORD(lParam);
            int y = (int)(short)HIWORD(lParam);

            if (currentStage == CLIENT_STAGE_LOGIN_MENU) {
                LoginMenuUIHandleMouseUp(&loginMenuUI, (float)x, (float)y, openglContext.width, openglContext.height);
                return 0;
            }

            // If box selection is active, release mouse capture
            // so we stop receiving mouse events.
            if (boxSelectActive) {
                ReleaseCapture();
            }

            // Get the mouse position from lParam.
            Vec2 screen = {(float)x, (float)y};
            Vec2 mouseWorld = ScreenToWorld(screen);
            boxSelectCurrentWorld = mouseWorld;

            // If the clicked (boxed) planet (planets) is (are) not owned by the local faction,
            // and the Shift key is not pressed, clear the current selection.
            // We AND the result of GetKeyState with 0x8000 to check if the high-order bit is set,
            // which indicates that the key is currently down.
            // Somewhat unrelated, but GetKeyState will return a short. Two bits in the short have a meaning:
            // - The high-order bit (0x8000) indicates whether the key is currently down.
            // - The low-order bit (0x0001) indicates whether the key is toggled (for toggle keys like Caps Lock).
            // - The windows help page documenatation is unclear about what any of the other bits mean.
            bool additive = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            if (boxSelectActive && boxSelectDragging) {
                // If we were dragging, apply box selection.
                ApplyBoxSelection(additive);
            } else {
                // Otherwise, we treat it as a click selection.
                HandleClickSelection(mouseWorld, additive);
            }

            // Regardless of what happened, end box selection state.
            boxSelectActive = false;
            boxSelectDragging = false;
            return 0;
        }

        // Right mouse button is used to issue move orders to selected planets.
        case WM_RBUTTONDOWN: {

            // Right click does nothing in the menu.
            if (currentStage == CLIENT_STAGE_LOGIN_MENU) {
                return 0;
            }

            // Cannot issue orders if the level is not initialized or there is no selection.
            if (!levelInitialized || selectionState.count == 0) {
                return 0;
            }

            // Get the mouse position from lParam.
            int x = (int)(short)LOWORD(lParam);
            int y = (int)(short)HIWORD(lParam);
            Vec2 screen = {(float)x, (float)y};
            Vec2 mousePos = ScreenToWorld(screen);

            // Get the planet at the mouse position, if any.
            size_t planetIndex = 0;
            Planet *clicked = PickPlanetAt(mousePos, &planetIndex);
            if (clicked == NULL) {
                return 0;
            }

            // When sending a move order we need to tell the server about it.
            // Otherwise, neither this client nor other clients will know about the move order.
            // That's because we don't actually simulate our own move orders locally, at least
            // not until we are told by the server in its broadcast of the move order to all
            // clients. This is to ensure consistency between all clients.
            const SOCKADDR_IN *targetAddress = serverAddressValid ? &serverAddress : NULL;
            PlayerSendMoveOrder(&selectionState, clientSocket, targetAddress, &level, planetIndex);
            return 0;
        }

        // Mousewheel movement is for zooming the camera in and out.
        case WM_MOUSEWHEEL: {
            
            // The mouse wheel currently does nothing in the menu.
            if (currentStage == CLIENT_STAGE_LOGIN_MENU) {
                return 0;
            }

            // Nothing to zoom if no level is initialized.
            if (!levelInitialized) {
                return 0;
            }

            // The wheel delta indicates the amount and direction of wheel movement.
            // A positive value indicates forward movement (away from the user),
            // while a negative value indicates backward movement (towards the user).
            // In typical RTS games, zooming out is done by scrolling the wheel backward (towards the user),
            // and zooming in is done by scrolling the wheel forward (away from the user).
            int wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);

            // No zoom change if there is no wheel movement.
            if (wheelDelta == 0) {
                return 0;
            }

            // Get the mouse cursor position in client coordinates.
            POINT cursor = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ScreenToClient(window_handle, &cursor);
            Vec2 screen = {(float)cursor.x, (float)cursor.y};

            // Determine the world position under the mouse cursor
            // before changing the zoom level.
            Vec2 focusWorld = ScreenToWorld(screen);

            // Default target zoom is the current zoom.
            float targetZoom = cameraState.zoom;

            // Any wheel delta > 0 means zoom in, < 0 means zoom out.
            // We don't care how far the wheel moved, just the direction.
            if (wheelDelta > 0) {
                targetZoom *= CAMERA_ZOOM_FACTOR;
            } else {
                targetZoom /= CAMERA_ZOOM_FACTOR;
            }

            // Apply the new zoom level to the camera.
            float previousZoom = cameraState.zoom;
            if (CameraSetZoom(&cameraState, targetZoom) && fabsf(cameraState.zoom - previousZoom) > 0.0001f) {
                // To keep the point under the mouse cursor fixed in world space,
                // we need to adjust the camera position accordingly.
                cameraState.position.x = focusWorld.x - screen.x / cameraState.zoom;
                cameraState.position.y = focusWorld.y - screen.y / cameraState.zoom;
                ClampCameraToLevel();
            }
            return 0;
        }

        // WM_KEYDOWN is sent when a key is pressed.
        // wParam contains the virtual-key code of the key that was pressed.
        // lParam contains additional information about the key event.
        case WM_KEYDOWN: {
            
            if (currentStage == CLIENT_STAGE_LOGIN_MENU) {
                bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                LoginMenuUIHandleKeyDown(&loginMenuUI, wParam, shiftDown);
                return 0;
            }

            // Handle control group and selection shortcuts.
            bool handled = false;

            // Check if Control or Shift keys are held down
            // (used for control group management).
            bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

            // Check for F2 key to select all owned planets.
            if (wParam == VK_F2 && localFaction != NULL) {
                // Delegate to PlayerSelectionSelectOwned to select all owned planets.
                PlayerSelectionSelectOwned(&selectionState, &level, localFaction, shiftDown);

                // Mark the event as handled
                // so we don't pass it to the default window procedure.
                handled = true;
            } else if (localFaction != NULL) {
                // Check for number keys (0-9 and numpad 0-9) for control groups.
                int groupIndex = ControlGroupIndexFromKey(wParam);

                // If a valid control group key was pressed, handle it (invalid key returns -1).
                if (groupIndex >= 0) {
                    if (ctrlDown) {
                        // ctrl + number = overwrite control group with new selection
                        PlayerControlGroupsOverwrite(&controlGroups, (size_t)groupIndex, &selectionState);
                    } else if (shiftDown && selectionState.count > 0) {
                        // shift + number = add current selection to control group
                        PlayerControlGroupsAdd(&controlGroups, (size_t)groupIndex, &selectionState);
                    } else {
                        // number alone = recall control group selection
                        PlayerControlGroupsApply(&controlGroups,
                            (size_t)groupIndex,
                            &level,
                            localFaction,
                            &selectionState,
                            shiftDown);
                    }
                    // Mark the event as handled
                    // so we don't pass it to the default window procedure.
                    handled = true;
                }
            }

            // If we handled the key event, return 0 to indicate success.
            if (handled) {
                return 0;
            }

            // If the key was not handled, pass it to the default window procedure.
            return DefWindowProc(window_handle, msg, wParam, lParam);
        }

        // WM_CHAR is sent when a character is input.
        // wParam contains the character code of the key that was pressed.
        // Compared to WM_KEYDOWN, which deals with virtual key codes,
        // WM_CHAR takes into account keyboard layout and modifier keys.
        // When a key is pressed, case WM_KEYDOWN is sent first,
        // followed by WM_CHAR if the key corresponds to a character,
        // according to TranslateMessage.
        // https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-char
        case WM_CHAR: {
            if (currentStage == CLIENT_STAGE_LOGIN_MENU) {
                LoginMenuUIHandleChar(&loginMenuUI, (unsigned int)wParam);
                return 0;
            }

            // Outside of the menu, we don't handle character input,
            // as instead we handle key presses in WM_KEYDOWN.
            return DefWindowProc(window_handle, msg, wParam, lParam);
        }
        default:
            //If the message is not handled by this procedure, 
            //pass it to the default window procedure.
            return DefWindowProc(window_handle, msg, wParam, lParam);
    }
}

/**
 * Starting point for the program.
 * @param hInstance Handle to the current instance of the program.
 * @param hPrevInstance Handle to the previous instance of the program.
 * @param pCmdLine Pointer to a null-terminated string specifying the command
 *                line arguments for the application, excluding the program name.
 * @param nCmdShow Specifies how the window is to be shown.
 * @return 0 if the program terminates successfully, non-zero otherwise.
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow) {
    // Force printf to flush immediately
    setvbuf(stdout, NULL, _IONBF, 0);

    // Suppresses -Wunused-parameter warning for hPrevInstance and pCmdLine
    (void)hPrevInstance;
    (void)pCmdLine;

    // Initialize the level structure
    // so that it is ready to be used when we receive data from the server.
    LevelInit(&level);

    // Initialize the camera state.
    CameraInitialize(&cameraState);
    cameraState.minZoom = CAMERA_MIN_ZOOM;
    cameraState.maxZoom = CAMERA_MAX_ZOOM;

    // Initialize the login menu UI state so the player can enter connection info.
    LoginMenuUIInitialize(&loginMenuUI);

    // Initialize Winsock for networking.
    if (!InitializeWinsock()) {
        return EXIT_FAILURE;
    }

    // Proceed to create the window which will host the menu and gameplay.

    // Create the window class to hold information about the window.
    static WNDCLASS window_class = {0};

    //L = wide character string literal
    static const wchar_t window_class_name[] = L"LightYearWarsClient";

    // Window class style affects the behavior of the window.
    // Here, we use CS_OWNDC to allocate a unique device context for the window.
    // This is important for OpenGL rendering, as it ensures that the rendering context
    // is tied to a specific device context that won't be shared with other windows.
    // Were it to be shared, it could lead to rendering issues and performance problems.
    // For example, let's say two windows share the same device context,
    // and one window changes the pixel format or other attributes of the device context.
    // This could inadvertently affect the rendering of the other window,
    // leading to unexpected visual artifacts or performance degradation.
    window_class.style = CS_OWNDC;

    //Set up a pointer to a function that windows will call to handle events, or messages.
    window_class.lpfnWndProc = WindowProcessMessage;

    // The instance handle is a unique identifier for the application.
    // You might see it used in various Windows API functions to identify the application.
    window_class.hInstance = hInstance;
    window_class.lpszClassName = window_class_name;

    // Register the window class with windows.
    if (!RegisterClass(&window_class)) {
        printf("RegisterClass failed.\n");
        closesocket(clientSocket);
        WSACleanup();
        return EXIT_FAILURE;
    }

    // This creates the window.
    HWND window_handle = CreateWindow(window_class_name, // Name of the window class
        L"Light Year Wars - Client", // Window title (seen in title bar)
        WS_OVERLAPPEDWINDOW, // Window style (standard window with title bar and borders)
        CW_USEDEFAULT, // Initial horizontal position of the window
        CW_USEDEFAULT, // Initial vertical position of the window
        CW_USEDEFAULT, // Initial width of the window
        CW_USEDEFAULT, // Initial height of the window
        NULL, // Handle to parent window (there is none here)
        NULL, // Handle to menu (there is none here)
        hInstance, // Handle to application instance
        NULL); // Pointer to window creation data (not used here)

    // Check if window creation was successful
    if (!window_handle) {
        printf("CreateWindow failed.\n");
        closesocket(clientSocket);
        WSACleanup();
        return EXIT_FAILURE;
    }

    // If this isn't called, the window will be created but not shown.
    // So you might see the program running in the task manager,
    // but there won't be any visible window on the screen.
    ShowWindow(window_handle, nCmdShow);

    // Initialize OpenGL for the created window
    if (!OpenGLInitializeForWindow(&openglContext, window_handle)) {
        printf("OpenGL initialization failed.\n");
        DestroyWindow(window_handle);
        closesocket(clientSocket);
        WSACleanup();
        return EXIT_FAILURE;
    }

    // Set up timing variables for the main loop's delta time calculation.
    previousTicks = GetTicks();
    tickFrequency = GetTickFrequency();

    // Set the cursor to be a regular arrow cursor.
    SetCursor(LoadCursor(NULL, IDC_ARROW));

    // Main loop
    while (running) {
        // Handle any messages sent to the window.
        MSG message;

        // Check for the next message and remove it from the message queue.
        while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
            // Takes virtual keystrokes and adds any applicable character messages to the queue.
            TranslateMessage(&message);

            // Sends the message to the window procedure which handles messages.
            DispatchMessage(&message);
        }

        // Process any connect requests from the menu UI.
        // If we are not in the menu stage, this will do essentially nothing
        ProcessMenuConnectRequest();

        // Process any incoming network messages from the server.
        ProcessNetworkMessages();

        // Calculate delta time since the last frame.
        int64_t currentTicks = GetTicks();
        float deltaTime = (float)(currentTicks - previousTicks) / (float)tickFrequency;
        previousTicks = currentTicks;

        // Update the time since we last received a packet from the server.
        if (clientSocket != INVALID_SOCKET && serverAddressValid && deltaTime > 0.0f) {
            timeSinceLastServerPacket += deltaTime;
            if (timeSinceLastServerPacket >= SERVER_TIMEOUT_SECONDS) {
                ResetConnectionToMenu("Disconnected: server timed out.");
            }
        }

        // We only have the concept of a camera in the game stage,
        // and likewise we only have a level to update in the game stage.
        if (currentStage == CLIENT_STAGE_GAME) {
            // Update the camera based on user input and elapsed time.
            UpdateCamera(window_handle, deltaTime);

            // If we have a level and time has passed, we must update the level state.
            if (levelInitialized && deltaTime > 0.0f) {
                LevelUpdate(&level, deltaTime);
            }
        }

        // Calculate frames per second (FPS) for display.
        float fps = 0.0f;
        if (deltaTime > 0.0001f) {
            fps = 1.0f / deltaTime;
        }

        // Now that we've updated and processed everything, its time to render the frame.
        RenderFrame(fps);
    }

    // At this point, we are exiting the main loop and need to clean up resources.
    PlayerSelectionFree(&selectionState);
    PlayerControlGroupsFree(&controlGroups);
    LevelRelease(&level);

    if (clientSocket != INVALID_SOCKET) {
        closesocket(clientSocket);
    }
    WSACleanup();

    OpenGLShutdownForWindow(&openglContext, window_handle);
    return EXIT_SUCCESS;
}
