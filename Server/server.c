/**
 * Main file for Light Year Wars C. Holds the program entry point and main loop.
 * @file main.c
 * @author abmize
*/
#include "Server/server.h"

// Used to exit the main loop.
static bool running = true;

// Aggregated OpenGL resources shared by the server window.
static OpenGLContext openglContext = {0};

// The main game level representing the ongoing game state.
static Level level;

// Currently selected planet for sending fleets.
static Planet *selected_planet = NULL;

// Array of players connected to the server.
static Player players[MAX_PLAYERS];

// Current number of connected players.
static size_t playerCount = 0;

// Accumulator for snapshot timing.
// Used to track time elapsed since last snapshot broadcast.
static float planetStateAccumulator = 0.0f;
static SOCKET server_socket = INVALID_SOCKET;

// Camera state used for navigating the server's spectator view.
static CameraState cameraState = {0};

// Current stage of the server application.
static ServerStage currentStage = SERVER_STAGE_LOBBY;

// Lobby UI state used while in the lobby stage.
static LobbyMenuUIState lobbyMenuUI = {0};

// Current lobby generation settings.
// This also controls the default settings used when initializing a new lobby.
static LobbyMenuGenerationSettings lobbySettings = {
    .planetCount = 48,
    .factionCount = 4,
    .minFleetCapacity = 20.0f,
    .maxFleetCapacity = 70.0f,
    .levelWidth = 4800.0f,
    .levelHeight = 4800.0f,
    .randomSeed = 22311u
};

// Flag indicating whether the lobby state needs to be re-broadcasted to all players.
static bool lobbyStateDirty = true;

// RNG State used to calculate ship spawn positions.
static unsigned int shipSpawnRNGState = SHIP_SPAWN_SEED;

// Time in seconds the server shall wait for a message before considering
// a client to have timed out.
static const float CLIENT_TIMEOUT_SECONDS = (float)CLIENT_TIMEOUT_MS / 1000.0f;

// Forward declarations of static functions
static Player *FindPlayerByAddress(const SOCKADDR_IN *address);
static const Faction *FindAvailableFaction(void);
static Player *EnsurePlayerForAddress(const SOCKADDR_IN *address);
static void HandleMoveOrderPacket(const SOCKADDR_IN *sender, const uint8_t *data, size_t size);
static void HandleLobbyColorPacket(const SOCKADDR_IN *sender, const uint8_t *data, size_t size);
static bool LaunchFleetAndBroadcast(Planet *origin, Planet *destination);
static void RemovePlayer(Player *player);
static void UpdatePlayerTimeouts(float deltaTime);
static void BroadcastServerShutdown(void);
static Vec2 ScreenToWorld(Vec2 screen);
static void ClampCameraToLevel(void);
static void RefreshCameraBounds(void);
static void ApplyCameraTransform(void);
static void UpdateCamera(HWND window_handle, float deltaTime);
static void InitializeLobbyState(void);
static bool ConfigureLobbyFactions(size_t factionCount);
static void RebindPlayerFactions(void);
static void RefreshLobbySlots(void);
static void BroadcastLobbyStateToAll(void);
static void SendLobbyStateToPlayerInstance(Player *player);
static void ProcessLobbyUI();
static bool AttemptStartGame(void);
static int HighestAssignedFactionId(void);
static void ApplyFactionColorUpdate(int factionId, uint8_t r, uint8_t g, uint8_t b);

/**
 * Converts screen coordinates to world coordinates using the current camera state.
 * @param screen The screen coordinates to convert.
 * @return The corresponding world coordinates.
 */
static Vec2 ScreenToWorld(Vec2 screen) {
    return CameraScreenToWorld(&cameraState, screen);
}

/**
 * Clamps the camera position to ensure it does not go outside the level bounds.
 * Prevents losing track of where we are in the level by endlessly panning off into empty space.
 */
static void ClampCameraToLevel(void) {
    // If the camera zoom is invalid or the viewport size is zero, there's nothing that needs to be done.
    if (cameraState.zoom <= 0.0f || openglContext.width <= 0 || openglContext.height <= 0) {
        return;
    }

    // Calculate the size of the viewport in world units, taking zoom levels into account.
    float viewWidth = (float)openglContext.width / cameraState.zoom;
    float viewHeight = (float)openglContext.height / cameraState.zoom;
    CameraClampToBounds(&cameraState, viewWidth, viewHeight);
}

/**
 * Refreshes the camera bounds based on the current level size.
 * Should be called whenever the level is initialized or resized.
 */
static void RefreshCameraBounds(void) {
    // Update the camera bounds to match the level dimensions.
    CameraSetBounds(&cameraState, level.width, level.height);

    // Clamp the camera position to ensure it is within the new bounds.
    ClampCameraToLevel();
}

/**
 * Applies the camera transformation to the current OpenGL modelview matrix.
 * Sets up the scaling and translation based on the camera state.
 */
static void ApplyCameraTransform(void) {
    // If the zoom level is invalid, do not apply any transformation.
    if (cameraState.zoom <= 0.0f) {
        return;
    }

    // Apply scaling and translation to the modelview matrix.
    glScalef(cameraState.zoom, cameraState.zoom, 1.0f);

    // Apply translation to position the camera correctly.
    glTranslatef(-cameraState.position.x, -cameraState.position.y, 0.0f);
}

/**
 * Updates the camera position based on user input.
 * Supports keyboard panning and edge panning with the mouse.
 * @param window_handle The handle to the application window.
 * @param deltaTime The time elapsed since the last update, in seconds.
 */
static void UpdateCamera(HWND window_handle, float deltaTime) {
    // If deltaTime is invalid, there's nothing to do.
    if (deltaTime <= 0.0f) {
        return;
    }

    // Determine if the window is active or has input capture.
    bool hasCapture = GetCapture() == window_handle;
    bool windowActive = GetForegroundWindow() == window_handle;

    // Only allow camera movement if the window is active or has capture.
    bool allowInput = windowActive || hasCapture;
    if (!allowInput) {
        return;
    }

    // Measure of how much to move the camera.
    Vec2 displacement = Vec2Zero();

    // Handle keyboard input for camera movement.
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
        float keySpeed = SERVER_CAMERA_KEY_SPEED * deltaTime / cameraState.zoom;
        displacement.x += keyDirection.x * keySpeed;
        displacement.y += keyDirection.y * keySpeed;
    }

    // Handle edge panning based on mouse position.
    POINT cursorPosition;
    if (GetCursorPos(&cursorPosition)) {

        // Convert the cursor position from screen coordinates to client coordinates.
        POINT clientPoint = cursorPosition;
        ScreenToClient(window_handle, &clientPoint);

        // Only apply edge panning if input is allowed and the viewport size is valid.
        if (openglContext.width > 0 && openglContext.height > 0) {

             // Default is no direction
            Vec2 edgeDirection = Vec2Zero();

            // If the mouse is within CAMERA_EDGE_PAN_MARGIN pixels of any particular edge,
            // with a preference for left/top edges when exactly on the margin of both sides,
            // pan the camera in that direction.
            if (clientPoint.x <= SERVER_CAMERA_EDGE_MARGIN) {
                edgeDirection.x -= 1.0f;
            } else if (clientPoint.x >= openglContext.width - SERVER_CAMERA_EDGE_MARGIN) {
                edgeDirection.x += 1.0f;
            }

            if (clientPoint.y <= SERVER_CAMERA_EDGE_MARGIN) {
                edgeDirection.y -= 1.0f;
            } else if (clientPoint.y >= openglContext.height - SERVER_CAMERA_EDGE_MARGIN) {
                edgeDirection.y += 1.0f;
            }

            // Normalize the direction and apply speed.
            if (edgeDirection.x != 0.0f || edgeDirection.y != 0.0f) {
                edgeDirection = Vec2Normalize(edgeDirection);
                float edgeSpeed = SERVER_CAMERA_EDGE_SPEED * deltaTime / cameraState.zoom;
                displacement.x += edgeDirection.x * edgeSpeed;
                displacement.y += edgeDirection.y * edgeSpeed;
            }
        }
    }

    // If we are also doing box selection, update the current box selection position
    // so that way the box can be drawn correctly.
    if (displacement.x != 0.0f || displacement.y != 0.0f) {
        cameraState.position = Vec2Add(cameraState.position, displacement);
        ClampCameraToLevel();
    }
}

/**
 * Initializes the lobby state and UI.
 * Configures factions and refreshes lobby slots.
 */
static void InitializeLobbyState(void) {
    // Initialize the lobby UI state.
    LobbyMenuUIInitialize(&lobbyMenuUI, true);
    LobbyMenuUISetColorEditAuthority(&lobbyMenuUI, true, -1);

    // Set the initial settings in the lobby UI.
    LobbyMenuUISetSettings(&lobbyMenuUI, &lobbySettings);

    // Clear any existing player slots.
    LobbyMenuUIClearSlots(&lobbyMenuUI);

    // Set the initial status message.
    LobbyMenuUISetStatusMessage(&lobbyMenuUI, "Adjust settings and press Start Game.");

    // Try to configure factions based on the current settings.
    if (!ConfigureLobbyFactions((size_t)lobbySettings.factionCount)) {
        printf("Failed to configure lobby factions.\n");
    }

    // Refresh the lobby slots to match current player assignments.
    RefreshLobbySlots();

    // Mark the lobby state as dirty to ensure it is (re)broadcasted.
    lobbyStateDirty = true;
}

/**
 * Configures the level factions for the lobby based on the specified faction count.
 * Initializes faction colors and sets level dimensions.
 * @param factionCount The number of factions to configure.
 * @return True if configuration was successful, false otherwise.
 */
static bool ConfigureLobbyFactions(size_t factionCount) {

    // Try to configure the level with the specified number of factions.
    if (!LevelConfigure(&level, factionCount, 0, 0)) {
        printf("Failed to configure lobby factions.\n");
        return false;
    }

    // Initialize faction colors in a simple pattern for visibility.
    for (size_t i = 0; i < factionCount; ++i) {
        // We first calculate a shade based on the faction index to ensure distinct colors.
        float shade = 0.55f + 0.35f * ((float)(i % 3) / 2.0f);

        // Then we assign colors cycling through red, green, and blue components.
        level.factions[i] = CreateFaction((int)i, shade, 0.4f + 0.2f * (float)(i % 2), 0.6f + 0.25f * (float)((i + 1) % 2));
    }

    // Set the level dimensions based on the current lobby settings.
    level.width = lobbySettings.levelWidth;
    level.height = lobbySettings.levelHeight;
    return true;
}

/**
 * Rebinds player faction pointers based on their assigned faction IDs.
 * Called after the level factions have been reconfigured to ensure players
 * reference the correct faction instances.
 */
static void RebindPlayerFactions(void) {

    // Iterate through all connected players and update their faction pointers.
    for (size_t i = 0; i < playerCount; ++i) {
        Player *player = &players[i];
        int id = player->factionId;
        const Faction *faction = NULL;

        // Find the faction in the level that matches the player's assigned faction ID.
        for (size_t j = 0; j < level.factionCount; ++j) {
            if (level.factions[j].id == id) {
                faction = &level.factions[j];
                break;
            }
        }

        // If we found a matching faction, assign it to the player.
        if (faction != NULL) {
            player->faction = faction;
            player->factionId = faction->id;
        } else {
            // No matching faction found; clear the player's faction reference.
            player->faction = NULL;
        }
    }
}

/**
 * Refreshes the lobby slots in the UI to reflect current player assignments.
 * Updates each slot's occupied status based on connected players.
 */
static void RefreshLobbySlots(void) {
    // Set the total number of slots in the lobby UI to match the faction count.
    LobbyMenuUISetSlotCount(&lobbyMenuUI, (size_t)lobbySettings.factionCount);


    // Then iterate through each slot and determine if it is occupied by any player.
    for (size_t i = 0; i < (size_t)lobbySettings.factionCount; ++i) {
        bool occupied = false;
        for (size_t j = 0; j < playerCount; ++j) {
            if (players[j].factionId == (int)i) {
                occupied = true;
                break;
            }
        }
        // Update the slot info in the lobby UI.
        LobbyMenuUISetSlotInfo(&lobbyMenuUI, i, (int)i, occupied);

        // Also set the slot color based on the faction color.
        if (level.factions != NULL && i < level.factionCount) {
            LobbyMenuUISetSlotColor(&lobbyMenuUI, i, level.factions[i].color);
        }
    }

    // Now clear any highlighted faction ID.
    LobbyMenuUISetHighlightedFactionId(&lobbyMenuUI, -1);
}

/**
 * Determines the highest assigned faction ID among all connected players.
 * Used to ensure lobby settings are valid before starting the game.
 * @return The highest faction ID assigned to any player, or -1 if no players are connected.
 */
static int HighestAssignedFactionId(void) {
    int maxId = -1;

    // Iterate through all players to find the maximum faction ID.
    for (size_t i = 0; i < playerCount; ++i) {
        if (players[i].factionId > maxId) {
            maxId = players[i].factionId;
        }
    }
    return maxId;
}

/**
 * Applies a faction color update to the level and lobby UI state.
 * @param factionId The faction ID to update.
 * @param r Red channel (0-255).
 * @param g Green channel (0-255).
 * @param b Blue channel (0-255).
 */
static void ApplyFactionColorUpdate(int factionId, uint8_t r, uint8_t g, uint8_t b) {
    if (factionId < 0 || (size_t)factionId >= level.factionCount || level.factions == NULL) {
        return;
    }

    // Convert 0-255 color channels to 0-1 float range.
    float rf = (float)r / 255.0f;
    float gf = (float)g / 255.0f;
    float bf = (float)b / 255.0f;
    FactionSetColor(&level.factions[factionId], rf, gf, bf);

    // Update the lobby UI slot color as well 
    // (the thing displayed to the right of the player/faction name).
    float color[4] = {rf, gf, bf, 1.0f};
    LobbyMenuUISetSlotColor(&lobbyMenuUI, (size_t)factionId, color);
    lobbyStateDirty = true;
}

/**
 * Builds the lobby state packet and slot info array based on current settings and player assignments
 * to be sent to players to inform them of the current lobby state.
 * @param packet Pointer to the LevelLobbyStatePacket to populate.
 * @param slots Array of LevelLobbySlotInfo to populate.
 */
static void BuildLobbyPacket(LevelLobbyStatePacket *packet, LevelLobbySlotInfo *slots) {
    // Validate parameters.
    if (packet == NULL || slots == NULL) {
        return;
    }

    // Clear the packet and populate it with current lobby settings.
    memset(packet, 0, sizeof(*packet));
    packet->type = LEVEL_PACKET_TYPE_LOBBY_STATE;
    packet->factionCount = (uint32_t)lobbySettings.factionCount;
    packet->planetCount = (uint32_t)lobbySettings.planetCount;
    packet->minFleetCapacity = lobbySettings.minFleetCapacity;
    packet->maxFleetCapacity = lobbySettings.maxFleetCapacity;
    packet->levelWidth = lobbySettings.levelWidth;
    packet->levelHeight = lobbySettings.levelHeight;
    packet->randomSeed = lobbySettings.randomSeed;
    packet->occupiedCount = (uint32_t)playerCount;

    // Determine the total number of slots to populate.
    // Will either be the faction count or the max slots allowed.
    size_t slotCount = (size_t)lobbySettings.factionCount;
    if (slotCount > LOBBY_MENU_MAX_SLOTS) {
        slotCount = LOBBY_MENU_MAX_SLOTS;
    }

    // Populate each slot's info based on player assignments.
    for (size_t i = 0; i < slotCount; ++i) {
        slots[i].factionId = (int32_t)i;
        slots[i].occupied = 0u;

        // Clear reserved bytes, then iterate through players to see if any occupy this slot.
        // We also set the slot color based on the faction color.

        memset(slots[i].reserved, 0, sizeof(slots[i].reserved));
        memset(slots[i].color, 0, sizeof(slots[i].color));

        if (i < level.factionCount && level.factions != NULL) {
            memcpy(slots[i].color, level.factions[i].color, sizeof(slots[i].color));
        }
        for (size_t j = 0; j < playerCount; ++j) {
            if (players[j].factionId == (int)i) {
                slots[i].occupied = 1u;
                break;
            }
        }
    }
}

/**
 * Broadcasts the current lobby state to all connected players.
 * Used to inform all players of the current lobby state, including faction slots and level settings.
 * Typically called whenever there is a change in the lobby, such as a player joining, leaving, 
 * or changing their faction, or when the server updates level settings.
 */
static void BroadcastLobbyStateToAll(void) {
    // If there are no players or the server socket is invalid, there's nothing to do.
    if (server_socket == INVALID_SOCKET || playerCount == 0) {
        return;
    }

    // Build the lobby state packet and slot info array.
    LevelLobbyStatePacket packet;
    LevelLobbySlotInfo slots[LOBBY_MENU_MAX_SLOTS] = {0};
    BuildLobbyPacket(&packet, slots);

    // Broadcast the lobby state to all connected players.
    BroadcastLobbyState(server_socket, players, playerCount, &packet, slots);
}

/**
 * Sends the current lobby state to a specific player.
 * Used when a player joins the lobby to inform them of the current state
 * of said lobby, including faction slots and level settings.
 * @param player The player to send the lobby state to.
 */
static void SendLobbyStateToPlayerInstance(Player *player) {
    // Validate parameters.
    if (player == NULL || server_socket == INVALID_SOCKET) {
        return;
    }

    // Build the lobby state packet and slot info array.
    LevelLobbyStatePacket packet;
    LevelLobbySlotInfo slots[LOBBY_MENU_MAX_SLOTS] = {0};
    BuildLobbyPacket(&packet, slots);

    // Send the lobby state to the specified player.
    SendLobbyStateToPlayer(player, server_socket, &packet, slots);
}

/**
 * Processes the lobby UI input and handles start game requests.
 * Updates lobby settings based on user input and attempts to start the game when requested.
 */
static void ProcessLobbyUI(void) {

    // Parse the current settings from the lobby UI.
    LobbyMenuGenerationSettings parsed;
    if (LobbyMenuUIGetSettings(&lobbyMenuUI, &parsed)) {
        // Check if any settings have changed.
        bool factionChanged = parsed.factionCount != lobbySettings.factionCount;
        bool settingsChanged = memcmp(&parsed, &lobbySettings, sizeof(parsed)) != 0;

        // If so, validate and apply the new settings.
        if (settingsChanged) {

            // We need to ensure the faction count is sufficient for the connected players.
            // We determine the highest assigned faction ID and ensure the count is at least that + 1.
            int highestAssigned = HighestAssignedFactionId();
            int minNeeded = (int)playerCount;
            if (highestAssigned >= 0 && highestAssigned + 1 > minNeeded) {
                minNeeded = highestAssigned + 1;
            }

            // If the new faction count is insufficient, set a status message and do not apply changes.
            if (parsed.factionCount < minNeeded) {
                char status[128];
                snprintf(status, sizeof(status), "Increase the faction count to at least %d.", minNeeded);
                LobbyMenuUISetStatusMessage(&lobbyMenuUI, status);
            } else {
                // Apply the new settings.
                lobbySettings = parsed;
                if (factionChanged) {
                    ConfigureLobbyFactions((size_t)lobbySettings.factionCount);
                    RebindPlayerFactions();
                    RefreshLobbySlots();
                }
                lobbyStateDirty = true;
                LobbyMenuUISetStatusMessage(&lobbyMenuUI, "Adjust settings and press Start Game.");
            }
        }
    }

    // Check if a start game request has been made.
    if (LobbyMenuUIConsumeStartRequest(&lobbyMenuUI)) {
        if (!AttemptStartGame()) {
            // AttemptStartGame will update the status message on failure.
        }
    }

    // Apply any committed color changes from the server lobby UI.
    int factionId = -1;
    uint8_t r = 0, g = 0, b = 0;
    if (LobbyMenuUIConsumeColorCommit(&lobbyMenuUI, &factionId, &r, &g, &b)) {
        ApplyFactionColorUpdate(factionId, r, g, b);
    }
}

/**
 * Attempts to start the game using the current lobby settings.
 * Validates settings and generates the level before transitioning to the game stage.
 * @return True if the game was started successfully, false otherwise.
 */
static bool AttemptStartGame(void) {
    LobbyMenuGenerationSettings parsed;

    // Validate and retrieve the current settings from the lobby UI.
    if (!LobbyMenuUIGetSettings(&lobbyMenuUI, &parsed)) {
        LobbyMenuUISetStatusMessage(&lobbyMenuUI, "Please enter valid values for all fields.");
        return false;
    }

    // Ensure the faction count is sufficient for the connected players.
    int highestAssigned = HighestAssignedFactionId();
    int minNeeded = (int)playerCount;
    if (highestAssigned >= 0 && highestAssigned + 1 > minNeeded) {
        minNeeded = highestAssigned + 1;
    }

    // If the faction count is insufficient, set a status message and do not start the game.
    if (parsed.factionCount < minNeeded) {
        char status[128];
        snprintf(status, sizeof(status), "Increase the faction count to at least %d.", minNeeded);
        LobbyMenuUISetStatusMessage(&lobbyMenuUI, status);
        return false;
    }

    // We save into a temporary object the level's current factions
    // so that way we can properly configure the level for generation
    // without losing the existing faction instances.
    Faction *existingFactions = NULL;
    size_t existingFactionCount = level.factionCount;
    if (existingFactionCount > 0 && level.factions != NULL) {
        existingFactions = malloc(sizeof(Faction) * existingFactionCount);
        if (existingFactions != NULL) {
            for (size_t i = 0; i < existingFactionCount; ++i) {
                existingFactions[i].id = level.factions[i].id;
                size_t colorSize = sizeof(level.factions[i].color);
                memcpy(existingFactions[i].color, level.factions[i].color, colorSize);
            }
        }
    }

    // Debug, check faction colors of the level before reconfiguration.
    for (size_t i = 0; i < existingFactionCount; ++i) {
        Faction *faction = &existingFactions[i];
        printf("Existing Faction %d Color: R=%.2f G=%.2f B=%.2f\n",
            faction->id, faction->color[0], faction->color[1], faction->color[2]);
    }

    // Configure the level with the correct planet and starship capacity
    // (before, it only had the correct faction count).

    // We allocate an initial capacity for starships equal to the maximum
    // number of ships that could be spawned if all the initial factions
    // were to launch fleets from all their starting planets at once.
    // We know that each faction starts with one planet,
    // and as the code is currently written, each faction's starting planet
    // has a maximum fleet capacity equal to minFleetCapacity + maxFleetCapacity / 2.
    // Therefore, the maximum number of ships that could be spawned at once
    // is factionCount * (minFleetCapacity + maxFleetCapacity) / 2.
    size_t initialStarshipCapacity = (size_t)parsed.factionCount *
        (size_t)((parsed.minFleetCapacity + parsed.maxFleetCapacity) / 2.0f);
    if (!LevelConfigure(&level, (size_t)parsed.factionCount, (size_t)parsed.planetCount, initialStarshipCapacity)) {

        LobbyMenuUISetStatusMessage(&lobbyMenuUI, "Failed to configure level with the provided settings.");
        if (existingFactions != NULL) {
            free(existingFactions);
        }

        return false;
    }

    // Restore existing faction instances where possible.
    // We overwrite the id and color of the newly created factions
    // with those from the existing factions to preserve player assignments.
    if (existingFactions != NULL) {
        for (size_t i = 0; i < existingFactionCount; ++i) {
            Faction *existing = &existingFactions[i];
            // We assume that the faction count is equal 
            // to the number of newly created factions.
            Faction *newFaction = &level.factions[i];
            newFaction->id = existing->id;

            // Copy color.
            size_t colorSize = sizeof(existing->color);
            memcpy(newFaction->color, existing->color, colorSize);
        }

        // We can now free the temporary storage.
        free(existingFactions);
    }


    // Try to generate the level with the provided settings.
    if (!GenerateRandomLevel(&level,
            (size_t)parsed.planetCount,
            (size_t)parsed.factionCount,
            parsed.minFleetCapacity,
            parsed.maxFleetCapacity,
            parsed.levelWidth,
            parsed.levelHeight,
            parsed.randomSeed)) {
        LobbyMenuUISetStatusMessage(&lobbyMenuUI, "Failed to generate level with the provided settings.");
        return false;
    }

    for (size_t i = 0; i < level.factionCount; ++i) {
        Faction *faction = &level.factions[i];
        printf("Generated Faction %d Color: R=%.2f G=%.2f B=%.2f\n",
            faction->id, faction->color[0], faction->color[1], faction->color[2]);
    }

    // We successfully generated the level, so apply the new settings.
    // We need to ensure all players are correctly bound to their factions,
    // as well as refresh camera bounds and reset game state.
    lobbySettings = parsed;
    RebindPlayerFactions();
    RefreshCameraBounds();
    shipSpawnRNGState = SHIP_SPAWN_SEED;
    planetStateAccumulator = 0.0f;
    selected_planet = NULL;
    CameraInitialize(&cameraState);
    cameraState.minZoom = SERVER_CAMERA_MIN_ZOOM / (fmaxf(level.width, level.height) / 2000.0f);
    cameraState.maxZoom = SERVER_CAMERA_MAX_ZOOM;
    RefreshCameraBounds();

    // Notify all players that the game is starting and send them the full level packet.
    if (playerCount > 0) {
        BroadcastStartGame(server_socket, players, playerCount);
        for (size_t i = 0; i < playerCount; ++i) {
            players[i].awaitingFullPacket = true;
            SendFullPacketToPlayer(&players[i], server_socket, &level);
        }
    }

    // Transition to the game stage.
    currentStage = SERVER_STAGE_GAME;

    // Clear the lobby state dirty flag and update the UI.
    lobbyStateDirty = false;
    LobbyMenuUISetStatusMessage(&lobbyMenuUI, NULL);
    LobbyMenuUISetEditable(&lobbyMenuUI, false);
    printf("Starting game with %zu players.\n", playerCount);
    return true;
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
        case WM_CLOSE: {
            DestroyWindow(window_handle);
            return 0;
        }

        // WM_DESTROY is sent when the window is being destroyed.
        // Compared to WM_CLOSE, this is a more final message.
        case WM_DESTROY: {
            BroadcastServerShutdown();
            running = false;
            PostQuitMessage(0);
            return 0;
        }

        //All window drawing has to happen inside the WM_PAINT message.
        case WM_PAINT: {
            PAINTSTRUCT paint;
            //In order to enable window drawing, BeginPaint must be called.
            //It fills out the PAINTSTRUCT and gives a device context handle for painting
            BeginPaint(window_handle, &paint);
            
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

            // Update the camera bounds to match the new level size when in game.
            if (currentStage == SERVER_STAGE_GAME) {
                ClampCameraToLevel();
            }
            return 0;
        }
        case WM_LBUTTONDOWN: {
            // Get the mouse cursor position in client coordinates.
            int x = (int)(short)LOWORD(lParam);
            int y = (int)(short)HIWORD(lParam);
            Vec2 mouseScreen = {(float)x, (float)y};

            // Then handle lobby UI interactions.
            if (currentStage == SERVER_STAGE_LOBBY) {
                LobbyMenuUIHandleMouseDown(&lobbyMenuUI, (float)x, (float)y, openglContext.width, openglContext.height);
                return 0;
            }

            // Otherwise if in game, handle planet selection and fleet launching.
            Vec2 mousePos = ScreenToWorld(mouseScreen);

            Planet *clicked = NULL;
            for (size_t i = 0; i < level.planetCount; ++i) {
                Vec2 diff = Vec2Subtract(mousePos, level.planets[i].position);
                float dist = Vec2Length(diff);
                if (dist < PlanetGetOuterRadius(&level.planets[i])) {
                    clicked = &level.planets[i];
                    break;
                }
            }

            if (clicked) {
                // Every client must know about fleet launches,
                // so it must be broadcast to all connected players.
                if (selected_planet && selected_planet != clicked) {
                    LaunchFleetAndBroadcast(selected_planet, clicked);
                    selected_planet = NULL;
                } else if (clicked->owner != NULL) {
                    if (selected_planet == clicked) {
                        selected_planet = NULL;
                    } else {
                        selected_planet = clicked;
                    }
                }
            } else {
                selected_planet = NULL;
            }
            return 0;
        }

        // On mouse move, we need to update the lobby UI hover state.
        // Nothing in the game stage currently uses mouse move events.
        case WM_MOUSEMOVE: {
            int x = (int)(short)LOWORD(lParam);
            int y = (int)(short)HIWORD(lParam);

            if (currentStage == SERVER_STAGE_LOBBY) {
                LobbyMenuUIHandleMouseMove(&lobbyMenuUI, (float)x, (float)y);
            }
            return 0;
        }

        // Likewise, on mouse up, we need to inform the lobby UI.
        // Nothing in the game stage currently uses mouse up events.
        case WM_LBUTTONUP: {
            int x = (int)(short)LOWORD(lParam);
            int y = (int)(short)HIWORD(lParam);

            if (currentStage == SERVER_STAGE_LOBBY) {
                LobbyMenuUIHandleMouseUp(&lobbyMenuUI, (float)x, (float)y, openglContext.width, openglContext.height);
            }
            return 0;
        }

        // Mousewheel movement is for zooming the camera in and out.
        case WM_MOUSEWHEEL: {

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

            
            // Calculate the number of wheel steps (notches) moved.
            // We use steps rather than the pure delta value
            // because the delta can vary depending on the mouse settings.
            float wheelSteps = (float)wheelDelta / (float)WHEEL_DELTA;

            // The mouse scrolls up/down to scroll the lobby menu up/down.
            if (currentStage == SERVER_STAGE_LOBBY) {
                LobbyMenuUIHandleScroll(&lobbyMenuUI, openglContext.height, wheelSteps);
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
                targetZoom *= SERVER_CAMERA_ZOOM_FACTOR;
            } else {
                targetZoom /= SERVER_CAMERA_ZOOM_FACTOR;
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

        // Handle keyboard input for lobby UI.
        // WM_KEYDOWN is sent when a key is pressed down.
        // It is suitable for handling non-text input, such as arrow keys
        // or keys like shift.
        case WM_KEYDOWN: {
            if (currentStage == SERVER_STAGE_LOBBY) {
                bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                LobbyMenuUIHandleKeyDown(&lobbyMenuUI, wParam, shiftDown);
                return 0;
            }
            return DefWindowProc(window_handle, msg, wParam, lParam);
        }

        // Compared to WM_KEYDOWN, WM_CHAR is more suitable for text input,
        // as it accounts for keyboard layout and modifier keys.
        case WM_CHAR: {
            if (currentStage == SERVER_STAGE_LOBBY) {
                LobbyMenuUIHandleChar(&lobbyMenuUI, (unsigned int)wParam);
                return 0;
            }
            return DefWindowProc(window_handle, msg, wParam, lParam);
        }
        default: {
            //If the message is not handled by this procedure, 
            //pass it to the default window procedure.
            return DefWindowProc(window_handle, msg, wParam, lParam);
        }
    }
}

/**
 * Finds a player by their network address.
 * @param address A pointer to the SOCKADDR_IN structure representing the player's address.
 * @return A pointer to the Player if found, NULL otherwise.
 */
static Player *FindPlayerByAddress(const SOCKADDR_IN *address) {
    // Basic validation of input pointer.
    if (address == NULL) {
        return NULL;
    }

    // Search through the connected players for a matching address.
    for (size_t i = 0; i < playerCount; ++i) {
        if (PlayerMatchesAddress(&players[i], address)) {
            return &players[i];
        }
    }

    // No matching player found.
    return NULL;
}

/**
 * Removes a player from the active player list, making their faction available again.
 * @param player Pointer to the Player to remove.
 */
static void RemovePlayer(Player *player) {
    // If the player pointer is invalid or there are no players, nothing to do.
    if (player == NULL || playerCount == 0) {
        return;
    }

    // If the player is not in the active list, nothing to do.
    size_t index = (size_t)(player - players);
    if (index >= playerCount) {
        return;
    }

    // Store some info for logging before we remove the player.

    // Get the player's faction.
    const Faction *faction = player->faction;
    char ipBuffer[INET_ADDRSTRLEN] = {0};

    // Get the player's IP address as a string.
    if (InetNtopA(AF_INET, (void *)&player->address.sin_addr, ipBuffer, sizeof(ipBuffer)) == NULL) {
        snprintf(ipBuffer, sizeof(ipBuffer), "unknown");
    }

    // Move the last active player into the removed slot to keep the array packed.
    size_t lastIndex = playerCount - 1;
    if (index != lastIndex) {
        players[index] = players[lastIndex];
    }
    players[lastIndex] = (Player){0};
    playerCount -= 1;

    // Log the player removal,
    // and mark their faction as available again.
    int factionId = faction != NULL ? faction->id : -1;
    printf("Released player slot for %s (faction %d). Remaining players: %zu\n", ipBuffer, factionId, playerCount);

    // When a player is removed, we need to refresh the lobby slots
    // to reflect the newly available faction.
    RefreshLobbySlots();

    // Since we have changed the lobby state, mark it as dirty.
    lobbyStateDirty = true;
}

/**
 * Updates the inactivity timers for all connected players
 * and removes any players that have timed out.
 * @param deltaTime The time elapsed since the last update, in seconds.
 */
static void UpdatePlayerTimeouts(float deltaTime) {
    // No players, or no time? No problem, nothing to do.
    if (playerCount == 0 || deltaTime <= 0.0f) {
        return;
    }

    // We use a while loop with an index so that we can
    // properly handle player removals while iterating.
    // A for loop would skip players if one is removed
    // unless we decrement the index, and the idea
    // of messing with a for loop index rather than just using
    // a while loop with an explicit index seems worse.
    size_t index = 0;
    while (index < playerCount) {
        // Increment the inactivity timer for this player.
        Player *player = &players[index];
        player->inactivitySeconds += deltaTime;

        // Check and handle timeout.
        if (player->inactivitySeconds >= CLIENT_TIMEOUT_SECONDS) {
            if (server_socket != INVALID_SOCKET) {
                LevelServerDisconnectPacket packet = {0};
                packet.type = LEVEL_PACKET_TYPE_SERVER_DISCONNECT;
                snprintf(packet.reason, sizeof(packet.reason), "Disconnected: inactive for too long.");
                int result = sendto(server_socket,
                    (const char *)&packet,
                    (int)sizeof(packet),
                    0,
                    (SOCKADDR *)&player->address,
                    (int)sizeof(player->address));
                if (result == SOCKET_ERROR) {
                    printf("disconnect notice sendto failed: %d\n", WSAGetLastError());
                }
            }
            char ipBuffer[INET_ADDRSTRLEN] = {0};
            if (InetNtopA(AF_INET, (void *)&player->address.sin_addr, ipBuffer, sizeof(ipBuffer)) == NULL) {
                snprintf(ipBuffer, sizeof(ipBuffer), "unknown");
            }

            // Log the timeout and remove the player.
            printf("Disconnecting inactive player %s after %.0f ms of silence.\n",
                ipBuffer, player->inactivitySeconds * 1000.0f);
            RemovePlayer(player);
            continue;
        }
        ++index;
    }
}

/**
 * Finds an available faction that is not currently assigned to any player.
 * @return A pointer to the available Faction, or NULL if none are available.
 */
static const Faction *FindAvailableFaction(void) {
    // If there are no factions, something pretty odd is happening.
    // Return NULL to indicate failure.
    if (level.factions == NULL) {
        return NULL;
    }

    // Search through the factions to find one not in use by any player.
    for (size_t i = 0; i < level.factionCount; ++i) {
        const Faction *candidate = &level.factions[i];
        bool inUse = false;

        // Inefficient, but with such a small number of players and factions,
        // this is acceptable for simplicity.

        // We search through all connected players
        // to see if any of them are using this faction.
        for (size_t j = 0; j < playerCount; ++j) {
            if (players[j].faction == candidate) {
                inUse = true;
                break;
            }
        }
        if (!inUse) {
            return candidate;
        }
    }

    // All factions are in use.
    return NULL;
}

/**
 * Ensures that a player exists for the given address.
 * If a player already exists for the address, updates their endpoint and marks them as awaiting a full packet.
 * If no player exists, creates a new player with an available faction.
 * @param address A pointer to the SOCKADDR_IN structure representing the player's address.
 * @return A pointer to the Player if successful, NULL otherwise.
 */
static Player *EnsurePlayerForAddress(const SOCKADDR_IN *address) {
    // Basic validation of input pointer.
    if (address == NULL) {
        return NULL;
    }

    // Check if a player already exists for this address.
    Player *existing = FindPlayerByAddress(address);
    if (existing != NULL) {
        // If somehow a player already exists for this address,
        // we just update their endpoint and mark them as awaiting a full packet.
        PlayerUpdateEndpoint(existing, address);
        existing->awaitingFullPacket = true;

        // We also reset their inactivity timer
        // since they have just sent us a packet.
        existing->inactivitySeconds = 0.0f;
        return existing;
    }

    // If there are already maximum players connected, we cannot add a new one.
    if (playerCount >= MAX_PLAYERS) {
        return NULL;
    }

    // Find an available faction for the new player.
    const Faction *faction = FindAvailableFaction();
    if (faction == NULL) {
        return NULL;
    }

    // Create and initialize the new player
    // using the found faction and provided address.
    Player *player = &players[playerCount++];
    PlayerInit(player, faction, address);

    // Reset inactivity timer since this is a new player.
    player->inactivitySeconds = 0.0f;

    // Log the new player registration
    // and report their assigned faction
    // and IP address.
    char ipBuffer[INET_ADDRSTRLEN] = {0};
    if (InetNtopA(AF_INET, (void *)&address->sin_addr, ipBuffer, sizeof(ipBuffer)) == NULL) {
        snprintf(ipBuffer, sizeof(ipBuffer), "unknown");
    }
    printf("Registered player for %s assigned faction %d\n", ipBuffer, faction->id);

    // When a new player joins, we need to refresh the lobby slots
    // to reflect the newly occupied faction.
    RefreshLobbySlots();

    // Since we have changed the lobby state, mark it as dirty.
    lobbyStateDirty = true;

    return player;
}

/**
 * Broadcasts a shutdown notice to all connected players so their clients can return to the menu.
 */
static void BroadcastServerShutdown(void) {
    // If there is no valid server socket or no players, nothing to do.
    if (server_socket == INVALID_SOCKET || playerCount == 0) {
        return;
    }

    // Prepare the server shutdown packet.
    // It's just a simple packet with the type field set,
    // as no additional data is needed.
    LevelServerDisconnectPacket packet = {0};
    packet.type = LEVEL_PACKET_TYPE_SERVER_DISCONNECT;
    snprintf(packet.reason, sizeof(packet.reason), "Disconnected: server closed.");

    // Every player needs to know about the server shutdown.
    for (size_t i = 0; i < playerCount; ++i) {
        int result = sendto(server_socket,
            (const char *)&packet,
            (int)sizeof(packet),
            0,
            (SOCKADDR *)&players[i].address,
            (int)sizeof(players[i].address));

        // Log any send errors.
        if (result == SOCKET_ERROR) {
            printf("server shutdown sendto failed: %d\n", WSAGetLastError());
        }
    }
}

/**
 * Processes a lobby color update packet received from a client.
 * Valid only during the lobby stage and for the sender's own faction.
 * @param sender Address of the client that sent the packet.
 * @param data Pointer to the packet data.
 * @param size Size of the packet data.
 */
static void HandleLobbyColorPacket(const SOCKADDR_IN *sender, const uint8_t *data, size_t size) {
    if (sender == NULL || data == NULL || size < sizeof(LevelLobbyColorPacket)) {
        return;
    }

    if (currentStage != SERVER_STAGE_LOBBY) {
        return;
    }

    const LevelLobbyColorPacket *packet = (const LevelLobbyColorPacket *)data;
    if (packet->type != LEVEL_PACKET_TYPE_LOBBY_COLOR) {
        return;
    }

    Player *player = FindPlayerByAddress(sender);
    if (player == NULL || player->faction == NULL) {
        return;
    }

    // If the player somehow sent a color update for a faction they do not own, ignore it.
    // Players should not be allowed to change other factions' colors.
    if (player->factionId != packet->factionId) {
        return;
    }

    ApplyFactionColorUpdate(packet->factionId, packet->r, packet->g, packet->b);
}

/**
 * Launches a fleet from the origin planet to the destination planet
 * and broadcasts the launch to all connected players to allow their
 * clients to simulate the fleet movement.
 * @param origin Pointer to the origin Planet.
 * @param destination Pointer to the destination Planet.
 * @return true if the fleet was successfully launched and broadcast, false otherwise.
 */
static bool LaunchFleetAndBroadcast(Planet *origin, Planet *destination) {
    // Basic validation of input pointers.
    if (origin == NULL || destination == NULL) {
        return false;
    }

    if (origin == destination) {
        return false;
    }

    if (level.planets == NULL || level.planetCount == 0) {
        return false;
    }

    if (origin < level.planets || destination < level.planets) {
        return false;
    }

    if (origin >= level.planets + level.planetCount) {
        return false;
    }

    if (destination >= level.planets + level.planetCount) {
        return false;
    }

    // Determine the indices of the origin and destination planets.
    size_t originIndex = (size_t)(origin - level.planets);
    size_t destinationIndex = (size_t)(destination - level.planets);

    // Determine the number of ships to launch based on the origin planet's current fleet size.
    int shipCount = (int)floorf(origin->currentFleetSize);
    if (shipCount <= 0) {
        return false;
    }

    // Determine the owner faction for the launched fleet.
    const Faction *owner = origin->owner;

    
    // Get a random seed to use for the fleet launch.
    // shipSpawnRNGState will have its value updated inside PlanetSendFleet.
    // Will be sent to all clients so they can simulate the same starship spawns.
    // This ensures that all clients simulate the same starship positions and movements.
    NextRandom(&shipSpawnRNGState);

    // Keeps track of the previous state of the ship spawn RNG
    // so that it can be sent to clients to allow them to simulate
    // the same starship spawn positions.
    // PlanetSendFleet will update shipSpawnRNGState as it generates random positions,
    // hence we need to save the old value here.
    unsigned int oldShipSpawnRNGState = shipSpawnRNGState;

    // Server side fleet launch.
    // Will mutate server state to appropriately represent the launched fleet.
    if (!PlanetSendFleet(origin, destination, &level, &shipSpawnRNGState)) {
        return false;
    }

    // Broadcast the fleet launch to all connected players, provided of course
    // that we have a valid server socket.
    // The current approach works with multiple planet launches by simply having
    // this overall function called multiple times in succession for each launch.
    // This may be somewhat inefficient compared to having a single call where each origin
    // planet is specified then compressed, perhaps with a single int, or multiple ints
    // being used to hold the origin planet indices, with them being divided by 2,
    // and then % 2 telling if the index is part of the origin or destination,
    // with enough ints being used to hold all the origin indices followed by
    // the single destination index.
    if (server_socket != INVALID_SOCKET) {
        int32_t ownerId = owner != NULL ? (int32_t)owner->id : -1;
        BroadcastFleetLaunch(server_socket, players, playerCount,
            (int32_t)originIndex, (int32_t)destinationIndex,
            (int32_t)shipCount, ownerId, oldShipSpawnRNGState);
    }

    return true;
}

/**
 * Processes a move order packet received from a client.
 * Validates ownership of origin planets before issuing fleet movements.
 * @param sender Address of the client that sent the packet.
 * @param data Pointer to the packet data.
 * @param size Size of the packet data.
 */
static void HandleMoveOrderPacket(const SOCKADDR_IN *sender, const uint8_t *data, size_t size) {
    // Basic validation of input pointers and packet size.
    if (sender == NULL || data == NULL) {
        return;
    }

    if (size < sizeof(LevelMoveOrderPacket)) {
        return;
    }

    // Move orders are only valid during the game stage.
    if (currentStage != SERVER_STAGE_GAME) {
        return;
    }

    // Cast the data to a LevelMoveOrderPacket and extract the information.
    const LevelMoveOrderPacket *packet = (const LevelMoveOrderPacket *)data;
    if (packet->type != LEVEL_PACKET_TYPE_MOVE_ORDER) {
        return;
    }

    // Check that the packet size matches the expected size
    // based on the number of origin planets specified.
    size_t originCount = (size_t)packet->originCount;
    size_t requiredSize = sizeof(LevelMoveOrderPacket) + originCount * sizeof(int32_t);
    if (size < requiredSize || originCount == 0) {
        return;
    }

    // Find the player that sent this packet.
    Player *player = FindPlayerByAddress(sender);
    if (player == NULL || player->faction == NULL) {
        return;
    }

    // Player sent a packet, ergo they are active.
    player->inactivitySeconds = 0.0f;

    // Validate the destination planet.
    int32_t destinationIndex = packet->destinationPlanetIndex;
    if (destinationIndex < 0 || (size_t)destinationIndex >= level.planetCount) {
        return;
    }

    Planet *destination = &level.planets[destinationIndex];
    const int32_t *originIndices = packet->originPlanetIndices;

    // Validate all origin planets before issuing orders.
    // Rather than rejecting the entire order if any origin planet is invalid,
    // we simply skip over invalid origins. This allows players to issue
    // move orders without being overly penalized for mistakes,
    // like if their client is out of date and they try to move from a planet
    // they no longer own.

    // We create a temporary array to hold valid origin indices.
    int32_t *validOrigins = (int32_t *)malloc(originCount * sizeof(int32_t));

    if (validOrigins == NULL) {
        // Compared to other failures, this one is severe enough
        // to warrant logging an error message.
        printf("Failed to allocate memory for validating move order origins.\n");
        return;
    }

    // Then we validate each origin planet, and only keep those
    // that the player actually owns.
    size_t validOriginCount = 0;
    for (size_t i = 0; i < originCount; ++i) {
        int32_t originIndex = originIndices[i];

        // A negative index or out-of-bounds index is invalid.
        if (originIndex < 0 || (size_t)originIndex >= level.planetCount) {
            continue;
        }

        Planet *origin = &level.planets[originIndex];

        // A planet not owned by the player's faction is invalid.
        if (origin->owner != player->faction) {
            continue;
        }

        validOrigins[validOriginCount++] = originIndex;
    }

    // If no valid origins remain, we can simply free and return.
    if (validOriginCount == 0) {
        free(validOrigins);
        return;
    }

    // Execute the move orders now that validation has succeeded.
    for (size_t i = 0; i < validOriginCount; ++i) {
        int32_t originIndex = validOrigins[i];
        Planet *origin = &level.planets[originIndex];
        if (origin == destination) {
            continue;
        }

        if (!LaunchFleetAndBroadcast(origin, destination)) {
            // Optional debug logging for rejected orders.
        }
    }

    // Clean up the temporary array.
    free(validOrigins);
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

    // Create the window class to hold information about the window.
    static WNDCLASS window_class = {0};

    //L = wide character string literal
    //This name is used to reference the window class later.
    static const wchar_t window_class_name[] = L"LightYearWarsServer";

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
    window_class.lpszClassName = window_class_name;

    //Set up a pointer to a function that windows will call to handle events, or messages.
    window_class.lpfnWndProc = WindowProcessMessage;

    // The instance handle is a unique identifier for the application.
    // You might see it used in various Windows API functions to identify the application.
    window_class.hInstance = hInstance;

    //Register the window class with windows.
    if (!RegisterClass(&window_class)) {
        printf("RegisterClass failed.\n");
        exit(1);
    }

    //This creates the window.
    HWND window_handle = CreateWindow(window_class_name, // Name of the window class
                                      L"Light Year Wars - Server", // Window title (seen in title bar)
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
        exit(1);
    }

    // If this isn't called, the window will be created but not shown.
    // So you might see the program running in the task manager,
    // but there won't be any visible window on the screen.
    ShowWindow(window_handle, nCmdShow);

    // Initialize OpenGL for the created window
    if (!OpenGLInitializeForWindow(&openglContext, window_handle)) {
        printf("OpenGL initialization failed.\n");
        DestroyWindow(window_handle);
        return EXIT_FAILURE;
    }

    printf("Starting UDP listener on port %d...\n", SERVER_PORT);

    // We set up a UDP socket to listen for messages from clients.
    // UDP is used to allow for low-latency communication,
    // which is important for real-time applications like games.

    // WinSock initialization
    if (!InitializeWinsock()) {
        // It's very important to clean up OpenGL resources if initialization fails.
        // Without this cleanup, resources like device contexts and rendering contexts
        // could remain allocated, leading to memory leaks and other mysterious issues
        // like windows having to be force shut down because it thinks some resources are still in use.
        OpenGLShutdownForWindow(&openglContext, window_handle);
        return EXIT_FAILURE;
    }

    // Create a UDP socket
    SOCKET sock = CreateUDPSocket();
    if (sock == INVALID_SOCKET) {
        OpenGLShutdownForWindow(&openglContext, window_handle);
        return EXIT_FAILURE;
    }

    // Set non-blocking mode
    if (!SetNonBlocking(sock)) {
        OpenGLShutdownForWindow(&openglContext, window_handle);
        return EXIT_FAILURE;
    }

    // Bind the socket to the local address and port number
    if (!BindSocket(sock, SERVER_PORT)) {
        OpenGLShutdownForWindow(&openglContext, window_handle);
        return EXIT_FAILURE;
    }

    server_socket = sock;

    // Buffer to hold incoming messages
    char recv_buffer[512];

    // Delta time calculation variables
    int64_t previous_ticks = GetTicks();
    int64_t frequency = GetTickFrequency();

    LevelInit(&level);
    CameraInitialize(&cameraState);
    cameraState.minZoom = SERVER_CAMERA_MIN_ZOOM;
    cameraState.maxZoom = SERVER_CAMERA_MAX_ZOOM;
    InitializeLobbyState();

    printf("Entering main program loop...\n");

    //Main program loop.
    while (running) {
        //Handle any messages sent to the window.
        MSG message;
        //Check for the next message and remove it from the message queue.
        while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
            //Takes virtual keystrokes and adds any applicable character messages to the queue.
            TranslateMessage(&message);

            //Sends the message to the window procedure which handles messages.
            DispatchMessage(&message);
        }

        // Here we wait for a UDP message
        
        // Flags tell recvfrom how to behave, 0 means no special behavior
        int flags = 0;

        // Structure to hold the address of the sender
        SOCKADDR_IN sender_address;

        // sender_address_size must be initialized to the size of the structure
        // It will be filled with the actual size of the sender address when we pass its address to recvfrom
        int sender_address_size = sizeof(sender_address);

        // Receive data from the socket
        // recvfrom takes the following arguments here:
        // 1. The socket to receive data on. In this case, our UDP socket.

        // 2. A buffer to hold the incoming data. Here, we use recv_buffer.

        // 3. The size of the buffer. Here, we use sizeof(recv_buffer) - 1.
        //    Note that sizeof(recv_buffer) - 1 is used to leave space for a null terminator.

        // 4. Flags to modify the behavior of recvfrom. Here, we use 0 for no special behavior.

        // 5. A pointer to a sockaddr structure that will be filled with the sender's address.
        //    We cast our SOCKADDR_IN pointer to a SOCKADDR pointer.

        // 6. A pointer to an integer that initially contains the size of the sender address structure.
        //    It will be filled with the actual size of the sender address after the call.
        int bytes_received = recvfrom(sock, recv_buffer, sizeof(recv_buffer) - 1, flags,
                                      (SOCKADDR*)&sender_address, &sender_address_size);

        // If data was received successfully (bytes_received is not SOCKET_ERROR)
        // then we process the received data.
        if (bytes_received != SOCKET_ERROR) {
            bool handled = false;

            // A player just sent us a packet, so they are active.
            Player *senderPlayer = FindPlayerByAddress(&sender_address);
            if (senderPlayer != NULL) {
                senderPlayer->inactivitySeconds = 0.0f;
            }

            // Check if the packet is a JOIN request.
            if (bytes_received >= 4 && strncmp(recv_buffer, "JOIN", 4) == 0) {
                handled = true;

                // Null-terminate so we can safely treat it as text.
                recv_buffer[bytes_received] = '\0';

                // Ensure a player exists for the sender's address
                // and respond based on the current server stage.
                Player *player = EnsurePlayerForAddress(&sender_address);
                if (player != NULL) {
                    // In case of a new player from a new address,
                    // we reset their inactivity timer.
                    // The earlier code for recieving a packet
                    // and finding the sender player only handles existing players.
                    player->inactivitySeconds = 0.0f;

                    if (currentStage == SERVER_STAGE_GAME) {
                        SendFullPacketToPlayer(player, sock, &level);
                    } else {
                        SendAssignmentPacket(player, sock);
                        SendLobbyStateToPlayerInstance(player);
                    }
                } else {
                    // If there is no available faction or the server is full,
                    // we send a SERVER_FULL message back to the client.
                    const char *fullMessage = "SERVER_FULL";
                    int sent = sendto(sock,
                        fullMessage,
                        (int)strlen(fullMessage),
                        0,
                        (SOCKADDR *)&sender_address,
                        sender_address_size);

                    if (sent == SOCKET_ERROR) {
                        printf("sendto failed: %d\n", WSAGetLastError());
                    }
                }
            }

            // If the packet wasn't handled yet, we check if it's a move order packet
            // or a client disconnect packet.
            if (!handled && bytes_received >= (int)sizeof(uint32_t)) {
                uint32_t packetType = 0;
                memcpy(&packetType, recv_buffer, sizeof(uint32_t));

                if (packetType == LEVEL_PACKET_TYPE_MOVE_ORDER) {
                    HandleMoveOrderPacket(&sender_address, (const uint8_t *)recv_buffer, (size_t)bytes_received);
                    handled = true;
                } else if (packetType == LEVEL_PACKET_TYPE_LOBBY_COLOR) {
                    HandleLobbyColorPacket(&sender_address, (const uint8_t *)recv_buffer, (size_t)bytes_received);
                    handled = true;
                } else if (packetType == LEVEL_PACKET_TYPE_CLIENT_DISCONNECT) {
                    // It's a disconnect notice from a client.
                    // We need to figure out which player is disconnecting
                    // and remove them from the active player list.
                    Player *player = FindPlayerByAddress(&sender_address);
                    if (player != NULL) {
                        RemovePlayer(player);
                    } else {
                        // If we can't find the player, we just log it and ignore.
                        char ipBuffer[INET_ADDRSTRLEN] = {0};
                        if (InetNtopA(AF_INET, (void *)&sender_address.sin_addr, ipBuffer, sizeof(ipBuffer)) == NULL) {
                            snprintf(ipBuffer, sizeof(ipBuffer), "unknown");
                        }
                        printf("Disconnect notice from unknown sender %s ignored.\n", ipBuffer);
                    }
                    handled = true;
                }
            }
        } else {
            // If recvfrom failed, we check the error code.
            // If it's not WSAEWOULDBLOCK, we log the error,
            // as WSAEWOULDBLOCK simply indicates that there was no data to read
            // in non-blocking mode, which is expected behavior.
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                printf("recvfrom failed: %d\n", error);
            }
        }

        // Calculate delta time
        int64_t current_ticks = GetTicks();
        float delta_time = (float)(current_ticks - previous_ticks) / (float)frequency;
        previous_ticks = current_ticks;

        // Depending on the current server stage, 
        // we either update the game state or process the lobby UI.
        if (currentStage == SERVER_STAGE_GAME) {
            // Update the camera based on input
            UpdateCamera(window_handle, delta_time);

            // Update the level state
            LevelUpdate(&level, delta_time);
        } else {
            ProcessLobbyUI();
        }

        // Update player timeouts with the elapsed time.
        UpdatePlayerTimeouts(delta_time);

        // Broadcast planet state updates at fixed intervals during the game stage.
        if (currentStage == SERVER_STAGE_GAME) {
            // Keep track of time for planet state broadcasting
            // Broadcasts now run at 20 Hz to keep ownership in sync.
            planetStateAccumulator += delta_time;
            while (planetStateAccumulator >= PLANET_STATE_BROADCAST_INTERVAL) {
                BroadcastSnapshots(sock, &level, players, playerCount);
                planetStateAccumulator -= PLANET_STATE_BROADCAST_INTERVAL;
            }
        } else if (lobbyStateDirty) {
            BroadcastLobbyStateToAll();
            lobbyStateDirty = false;
        }

        // Calculate frames per second (FPS)
        float fps = 0.0f;

        // Make sure not to divide by zero if we lag too hard
        // (lag is bad enough, no reason to make it worse with a crash)
        if (delta_time > 0.0f) {
            fps = 1.0f / delta_time;
        }

        if (openglContext.deviceContext && openglContext.renderContext) {
            // Sets the clear color for the window (background color)
            glClearColor(BACKGROUND_COLOR_R, BACKGROUND_COLOR_G, BACKGROUND_COLOR_B, BACKGROUND_COLOR_A);

            // Clear the color buffer to the clear color
            glClear(GL_COLOR_BUFFER_BIT);

            // Reset the modelview matrix
            glMatrixMode(GL_MODELVIEW);

            // Load the identity matrix to reset any previous transformations
            glLoadIdentity();

            if (openglContext.width > 0 && openglContext.height > 0) {
                // Draw the background gradient
                DrawBackgroundGradient(openglContext.width, openglContext.height);

                // We only draw the game elements if we are in the game stage.
                if (currentStage == SERVER_STAGE_GAME) {
                    // Save the current matrix state before applying camera transformations.
                    glPushMatrix();

                    // Apply camera transformations (translation and scaling)
                    // to allow for panning and zooming.
                    ApplyCameraTransform();

                    // Draw each planet in the level
                    for (size_t i = 0; i < level.planetCount; ++i) {
                        PlanetDraw(&level.planets[i]);
                    }

                    // Draw each starship trail effect
                    for (size_t i = 0; i < level.trailEffectCount; ++i) {
                        StarshipTrailEffectDraw(&level.trailEffects[i]);
                    }

                    // If a planet is selected, draw a ring around it
                    if (selected_planet != NULL) {
                        float radius = PlanetGetOuterRadius(selected_planet);
                        float highlightColor[4] = {1.0f, 1.0f, 1.0f, 0.85f};
                        DrawFeatheredRing(selected_planet->position.x, selected_planet->position.y,
                            radius + 2.0f, radius + 5.0f, 1.2f, highlightColor);
                    }

                    // Draw each starship in the level
                    for (size_t i = 0; i < level.starshipCount; ++i) {
                        StarshipDraw(&level.starships[i]);
                    }

                    // Restore the previous matrix state
                    glPopMatrix();
                }
            }

            // We only need to draw the lobby UI if we are in the lobby stage.
            if (currentStage == SERVER_STAGE_LOBBY) {
                LobbyMenuUIDraw(&lobbyMenuUI, &openglContext, openglContext.width, openglContext.height);
            }

            // Display FPS in the top-left corner
            // We draw this after all other rendering to ensure it's visible on top.

            int textPositionFromTop = 20;
            int textPositionFromLeft = 10;

            if (openglContext.width >= textPositionFromLeft && openglContext.height >= textPositionFromTop) {
                char fpsString[32];
                snprintf(fpsString, sizeof(fpsString), "FPS: %.0f", fps);

                float textColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
                float textSize = 16.0f;
                DrawScreenText(&openglContext, fpsString, (float)textPositionFromLeft, (float)textPositionFromTop, textSize, textSize / 2, textColor);
            }

            // Swap the front and back buffers to display the rendered frame.
            // There's two buffers, one being displayed while the other is drawn to.
            // Swapping them makes the newly drawn frame visible, while taking the
            // previously displayed buffer off-screen for the next frame's drawing.
            SwapBuffers(openglContext.deviceContext);
        }
    }

    // At this point in the code, we are exiting the main loop and need to clean up resources.
    closesocket(sock);
    server_socket = INVALID_SOCKET;
    WSACleanup();
    LevelRelease(&level);
    OpenGLShutdownForWindow(&openglContext, window_handle);
    return EXIT_SUCCESS;
}
