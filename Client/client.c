/**
 * Companion client for Light Year Wars C.
 * Implements the client-side logic, rendering, and user interaction.
 * @author abmize
 * @file client.c
 */
#include "Client/client.h"
#include "Utilities/openglUtilities.h"
#include "Utilities/playerInterfaceUtilities.h"

// ---- Global variables ----

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
static char recv_buffer[2048];

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

// Tracks the player's current planet selection state.
static PlayerSelectionState selectionState = {0};

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
static const Faction *ResolveFactionById(int32_t factionId);
static void ProcessNetworkMessages(void);
static void SendJoinRequest(void);
static bool PromptForServerAddress(void);
static void RenderFrame(float fps);

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
 * @param data Pointer to the packet data.
 * @param length Length of the packet data.
 */
static void HandleFullPacketMessage(const uint8_t *data, size_t length) {
    // Delegate to LevelApplyFullPacket to handle the actual data.
    if (!LevelApplyFullPacket(&level, data, length)) {
        printf("Failed to apply full packet.\n");
        return;
    }

    // Mark the level as initialized and reset selection state,
    // in case of a reinitialization.
    levelInitialized = true;
    awaitingFull = false;

    // Reset the player's selection state in case they
    // had any selections prior to the full packet.
    PlayerSelectionReset(&selectionState, level.planetCount);
    RefreshLocalFaction();
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
    if (!PlanetSimulateFleetLaunch(origin, destination, &level, shipCount, owner)) {
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
        printf("sendto failed: %d\n", WSAGetLastError());
    }
}

/**
 * Prompts the user for the server IP address and port.
 * Allocates a console window for input/output if necessary.
 * @return true if a valid address was provided, false otherwise.
 */
static bool PromptForServerAddress(void) {

    // Check if we need to allocate a console.
    if (GetConsoleWindow() == NULL) {
        // If so, we try to allocate one.
        if (!AllocConsole()) {
            printf("AllocConsole failed.\n");
            return false;
        }
    }

    // Redirect standard input and output to the console (freopen = "file reopen").
    // Will make stdin and stdout point to the console window.
    FILE *consoleIn = freopen("CONIN$", "r", stdin);
    FILE *consoleOut = freopen("CONOUT$", "w", stdout);

    // Disable output buffering for the console output.
    // Helps ensure that printf output appears immediately.
    (void)consoleIn;
    if (consoleOut != NULL) {
        setvbuf(stdout, NULL, _IONBF, 0);
    }

    // Prompt the user for IP address and port.
    char ip[INET_ADDRSTRLEN] = {0};
    int port = 0;

    // An ip address is at most 15 characters long (xxx.xxx.xxx.xxx) plus null terminator.
    printf("Enter server IP address: ");
    if (scanf("%15s", ip) != 1) {
        return false;
    }

    // Prompt for port number (should be no more than 2^16-1 = 65535).
    printf("Enter server port: ");
    if (scanf("%d", &port) != 1) {
        return false;
    }

    // Try to create the server address structure.
    if (!CreateAddress(ip, port, &serverAddress)) {
        return false;
    }

    // We have a valid server address now, so mark it as such.
    serverAddressValid = true;
    return true;
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

        // If the level is initialized, draw the planets, starships, and starship trails.
        if (levelInitialized) {

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
        }
    }

    // Draw the FPS, faction ID, and selection count in the top-left corner.
    if (openglContext.fontDisplayListBase && openglContext.width >= 10 && openglContext.height >= 10) {
        // Line 1, topmost: FPS
        char line1[64];

        // Line 2, middle: Faction ID
        char line2[64];

        // Line 3, bottom: Number of selected planets
        char line3[64];
        snprintf(line1, sizeof(line1), "FPS: %.0f", fps);
        int factionDisplay = localFaction != NULL ? localFaction->id : -1;
        snprintf(line2, sizeof(line2), "Faction: %d", factionDisplay);
        snprintf(line3, sizeof(line3), "Selection: %zu", selectionState.count);

        // White color for text
        glColor3f(1.0f, 1.0f, 1.0f);

        // Set the raster position for the first line of text (FPS)
        // 10 pixels from the left, 20 pixels from the top
        glRasterPos2f(10.0f, 20.0f);

        // glPushAttrib and glPopAttrib are used to save and restore
        // the current OpenGL state related to display lists.
        glPushAttrib(GL_LIST_BIT);

        // glListBase sets the base value for display lists.
        glListBase(openglContext.fontDisplayListBase - 32);

        // glCallLists renders the text using the display lists.
        // It takes the length of the string, the type of data (unsigned byte),
        // and a pointer to the string data.
        glCallLists((GLsizei)strlen(line1), GL_UNSIGNED_BYTE, line1);

        // Second line (Faction ID), 10 pixels from the left, 36 pixels from the top
        // We add 16 pixels to the y-coordinate to move down for the next line
        // since each line is approximately 16 pixels high (see font size in openglUtilities.c).
        glRasterPos2f(10.0f, 36.0f);
        glCallLists((GLsizei)strlen(line2), GL_UNSIGNED_BYTE, line2);

        // Third line (Selection count), 10 pixels from the left, 52 pixels from the top
        glRasterPos2f(10.0f, 52.0f);
        glCallLists((GLsizei)strlen(line3), GL_UNSIGNED_BYTE, line3);
        glPopAttrib();
    }

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
        case WM_CLOSE:
            DestroyWindow(window_handle);
            return 0;
        case WM_DESTROY:
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
            return 0;
        }

        // This message indicates a mouse button was pressed.
        // lParam contains the x and y coordinates of the mouse cursor.
        case WM_LBUTTONDOWN: {
            // Nothing to select if the level is not initialized or there is no local faction.
            if (!levelInitialized || localFaction == NULL) {
                PlayerSelectionClear(&selectionState);
                return 0;
            }

            // Get the mouse position from lParam.
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            Vec2 mousePos = {(float)x, (float)y};

            // Get the planet at the mouse position, if any.
            size_t planetIndex = 0;
            Planet *clicked = PickPlanetAt(mousePos, &planetIndex);
            if (clicked == NULL) {
                // If the player is holding down shift, they're trying to add to their selection,
                // so we don't clear the selection when clicking empty space.
                if ((GetKeyState(VK_SHIFT) & 0x8000) == 0) {
                    PlayerSelectionClear(&selectionState);
                }
                return 0;
            }

            // If the clicked planet is not owned by the local faction, clear selection unless Shift is held.
            if (clicked->owner != localFaction) {
                // If the clicked planet is not owned by the local faction,
                // and the Shift key is not pressed, clear the current selection.
                // We AND the result of GetKeyState with 0x8000 to check if the high-order bit is set,
                // which indicates that the key is currently down.
                // Somewhat unrelated, but GetKeyState will return a short. Two bits in the short have a meaning:
                // - The high-order bit (0x8000) indicates whether the key is currently down.
                // - The low-order bit (0x0001) indicates whether the key is toggled (for toggle keys like Caps Lock).
                // - The windows help page documenatation is unclear about what any of the other bits mean.
                if ((GetKeyState(VK_SHIFT) & 0x8000) == 0) {
                    PlayerSelectionClear(&selectionState);
                }
                return 0;
            }

            // If we reach here, the clicked planet is owned by the local faction.
            // Holding down shift means the player wants to add/remove from their current selection,
            // as opposed to replacing it entirely (which is the default behavior).
            bool additive = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            PlayerSelectionToggle(&selectionState, planetIndex, additive);
            return 0;
        }

        // Right mouse button is used to issue move orders to selected planets.
        case WM_RBUTTONDOWN: {
            // Cannot issue orders if the level is not initialized or there is no selection.
            if (!levelInitialized || selectionState.count == 0) {
                return 0;
            }

            // Get the mouse position from lParam.
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            Vec2 mousePos = {(float)x, (float)y};

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

    // Ask the user for the server address.
    if (!PromptForServerAddress()) {
        printf("Failed to obtain server address.\n");
        return EXIT_FAILURE;
    }

    // Initialize Winsock for networking.
    if (!InitializeWinsock()) {
        return EXIT_FAILURE;
    }

    // Create the UDP socket for communication with the server.
    clientSocket = CreateUDPSocket();
    if (clientSocket == INVALID_SOCKET) {
        WSACleanup();
        return EXIT_FAILURE;
    }

    // Set the socket to non-blocking mode.
    // As we are running the main loop in a single thread,
    // we cannot afford to block on network operations.
    if (!SetNonBlocking(clientSocket)) {
        closesocket(clientSocket);
        WSACleanup();
        return EXIT_FAILURE;
    }

    // Send a join request to the server to initiate the connection.
    SendJoinRequest();

    // As the server processes the join request, we proceed to create the window
    // which will graphically display the game state.

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

        // Process any incoming network messages from the server.
        ProcessNetworkMessages();

        // Calculate delta time since the last frame.
        int64_t currentTicks = GetTicks();
        float deltaTime = (float)(currentTicks - previousTicks) / (float)tickFrequency;
        previousTicks = currentTicks;

        // If we have a level and time has passed, we must update the level state.
        if (levelInitialized && deltaTime > 0.0f) {
            LevelUpdate(&level, deltaTime);
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
    LevelRelease(&level);

    if (clientSocket != INVALID_SOCKET) {
        closesocket(clientSocket);
    }
    WSACleanup();

    OpenGLShutdownForWindow(&openglContext, window_handle);
    return EXIT_SUCCESS;
}
