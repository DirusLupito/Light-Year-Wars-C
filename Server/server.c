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

// Forward declarations of static functions
static Player *FindPlayerByAddress(const SOCKADDR_IN *address);
static const Faction *FindAvailableFaction(void);
static Player *EnsurePlayerForAddress(const SOCKADDR_IN *address);
static void HandleMoveOrderPacket(const SOCKADDR_IN *sender, const uint8_t *data, size_t size);
static bool LaunchFleetAndBroadcast(Planet *origin, Planet *destination);
static Vec2 ScreenToWorld(Vec2 screen);
static void ClampCameraToLevel(void);
static void RefreshCameraBounds(void);
static void ApplyCameraTransform(void);
static void UpdateCamera(HWND window_handle, float deltaTime);

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
 * Window procedure that handles messages sent to the window.
 * @param window_handle Handle to the window.
 * @param msg The message.
 * @param wParam Additional message information.
 * @param lParam Additional message information.
 * @return The result of the message processing and depends on the message sent.
*/
LRESULT CALLBACK WindowProcessMessage(HWND window_handle, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE: {
            DestroyWindow(window_handle);
            return 0;
        }
        case WM_DESTROY: {
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

            // Update the camera bounds to match the new level size.
            ClampCameraToLevel();
            return 0;
        }
        case WM_LBUTTONDOWN: {
            // Get the mouse cursor position in client coordinates.
            int x = (int)(short)LOWORD(lParam);
            int y = (int)(short)HIWORD(lParam);
            Vec2 mouseScreen = {(float)x, (float)y};

            // Then convert to world coordinates.
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

    // Log the new player registration
    // and report their assigned faction
    // and IP address.
    char ipBuffer[INET_ADDRSTRLEN] = {0};
    if (InetNtopA(AF_INET, (void *)&address->sin_addr, ipBuffer, sizeof(ipBuffer)) == NULL) {
        snprintf(ipBuffer, sizeof(ipBuffer), "unknown");
    }
    printf("Registered player for %s assigned faction %d\n", ipBuffer, faction->id);

    return player;
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
    if (!PlanetSendFleet(origin, destination, &level)) {
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
            (int32_t)shipCount, ownerId);
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

    printf("Generating a random level...\n");
    LevelInit(&level);
    CameraInitialize(&cameraState);
    cameraState.minZoom = SERVER_CAMERA_MIN_ZOOM;
    cameraState.maxZoom = SERVER_CAMERA_MAX_ZOOM;
    float level_width = 1600.0f;
    float level_height = 1200.0f;
    GenerateRandomLevel(&level, 24, 4, 5.0f, 30.0f, level_width, level_height, 233);

    // Once the level is generated, we can set the camera bounds.
    RefreshCameraBounds();

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

            // Check if the packet is a JOIN request.
            if (bytes_received >= 4 && strncmp(recv_buffer, "JOIN", 4) == 0) {
                handled = true;

                // Null-terminate so we can safely treat it as text.
                recv_buffer[bytes_received] = '\0';

                // Ensure a player exists for the sender's address
                // and send them the full level packet alongside their assignment.
                Player *player = EnsurePlayerForAddress(&sender_address);
                if (player != NULL) {
                    SendFullPacketToPlayer(player, sock, &level);
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

            // If the packet wasn't handled yet, we check if it's a move order packet.
            if (!handled && bytes_received >= (int)sizeof(uint32_t)) {
                uint32_t packetType = 0;
                memcpy(&packetType, recv_buffer, sizeof(uint32_t));

                if (packetType == LEVEL_PACKET_TYPE_MOVE_ORDER) {
                    HandleMoveOrderPacket(&sender_address, (const uint8_t *)recv_buffer, (size_t)bytes_received);
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

        // Update the camera based on input
        UpdateCamera(window_handle, delta_time);

        // Update the level state
        LevelUpdate(&level, delta_time);

        // Keep track of time for planet state broadcasting
        // Broadcasts now run at 20 Hz to keep ownership in sync.
        planetStateAccumulator += delta_time;
        while (planetStateAccumulator >= PLANET_STATE_BROADCAST_INTERVAL) {
            BroadcastSnapshots(sock, &level, players, playerCount);
            planetStateAccumulator -= PLANET_STATE_BROADCAST_INTERVAL;
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

                // Save the current matrix state
                // before applying camera transformations
                // as it may change the modelview matrix.
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

            // Display FPS in the top-left corner
            if (openglContext.fontDisplayListBase && openglContext.width >= 10 && openglContext.height >= 10) {
                char fps_string[32];
                snprintf(fps_string, sizeof(fps_string), "FPS: %.0f", fps);

                // Text color is white
                glColor3f(1.0f, 1.0f, 1.0f);

                // Text only needs an identity modelview matrix
                glLoadIdentity();
                // Text position is 10 pixels from the left and 20 pixels from the top
                glRasterPos2f(10.0f, 20.0f);

                // Render the FPS string using the font display lists
                glPushAttrib(GL_LIST_BIT);

                // Set the base display list for character rendering
                // This is done by subtracting 32 from the display list base
                // because our display lists start at ASCII character 32 (space character).
                glListBase(openglContext.fontDisplayListBase - 32);

                // Call the display lists for each character in the fps_string
                // This renders the string to the screen.
                glCallLists((GLsizei)strlen(fps_string), GL_UNSIGNED_BYTE, fps_string);

                // Restore the previous display list state
                glPopAttrib();
            }

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
