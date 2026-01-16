/**
 * Main file for Light Year Wars C. Holds the program entry point and main loop.
 * @file main.c
 * @author abmize
*/
#include "Server/server.h"

// Used to exit the main loop.
static bool running = true;

// The device context and rendering context for OpenGL.
// A device context is a Windows structure that defines a set of graphic objects
// and their associated attributes, as well as the graphic modes that affect output.
static HDC window_device_context = NULL;

// The rendering context is an OpenGL structure that holds all of OpenGL's state information
// for drawing to a device context.
static HGLRC window_render_context = NULL;

// OpenGL font display list base and window dimensions.
// This is used for rendering text.
// If for example, this equals 1000, then the display list for the character 'A' (ASCII 65) would be 1065.
// The display list is a compiled set of OpenGL commands that can be executed more efficiently.
// This is useful for rendering text because each character can be drawn with a single call to the display list.
static GLuint font_display_list_base = 0;

// Window dimensions
static int window_width = 0;
static int window_height = 0;

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
static float snapshotAccumulator = 0.0f;

// Background color constants for rendering.
static const float BACKGROUND_GRADIENT_INNER_COLOR[4] = {0.36f, 0.30f, 0.43f, 1.0f};
static const float BACKGROUND_GRADIENT_OUTER_COLOR[4] = {BACKGROUND_COLOR_R, BACKGROUND_COLOR_G, BACKGROUND_COLOR_B, 1.0f};

// Forward declarations of static functions
static Player *FindPlayerByAddress(const SOCKADDR_IN *address);
static const Faction *FindAvailableFaction(void);
static Player *EnsurePlayerForAddress(const SOCKADDR_IN *address);
static bool SendFullPacketToPlayer(Player *player, SOCKET sock);
static void BroadcastSnapshots(SOCKET sock);
static void SendPacketToPlayer(Player *player, SOCKET sock, const LevelPacketBuffer *packet);
static bool InitializeOpenGL(HWND window_handle);
static void ShutdownOpenGL(HWND window_handle);
static void UpdateProjection(int width, int height);
static void DrawBackgroundGradient(int width, int height);

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
            window_width = LOWORD(lParam);
            window_height = HIWORD(lParam);

            // We need to update the OpenGL projection matrix to match the new window size.
            // If we don't do this, the aspect ratio will be incorrect
            // and the rendered scene will appear stretched or squashed.
            UpdateProjection(window_width, window_height);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            Vec2 mousePos = {(float)x, (float)y};

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
                if (selected_planet && selected_planet != clicked) {
                    PlanetSendFleet(selected_planet, clicked, &level);
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
 * Sends a packet to the specified player.
 * @param player A pointer to the Player to send the packet to.
 * @param sock The socket to use for sending the packet.
 * @param packet A pointer to the LevelPacketBuffer containing the packet data.
 */
static void SendPacketToPlayer(Player *player, SOCKET sock, const LevelPacketBuffer *packet) {
    // Basic validation of input pointers.
    if (player == NULL || packet == NULL || packet->data == NULL) {
        return;
    }

    // Ensure the packet size does not exceed INT_MAX
    // as sendto expects an int for the size parameter.
    if (packet->size > (size_t)INT_MAX) {
        printf("Packet too large to send (size=%zu).\n", packet->size);
        return;
    }

    // Actually send the packet using sendto.
    // Argments:
    // sock - the socket to send on
    // (const char *)packet->data - pointer to the data to send
    // (int)packet->size - size of the data to send
    // 0 - flags (none)
    // (SOCKADDR *)&player->address - pointer to the destination address
    // (int)sizeof(player->address) - size of the destination address
    int result = sendto(sock,
        (const char *)packet->data,
        (int)packet->size,
        0,
        (SOCKADDR *)&player->address,
        (int)sizeof(player->address));

    // Check for and report any error
    if (result == SOCKET_ERROR) {
        printf("sendto failed: %d\n", WSAGetLastError());
    }
}

/**
 * Sends a full level packet to the specified player.
 * @param player A pointer to the Player to send the packet to.
 * @param sock The socket to use for sending the packet.
 * @return true if the packet was sent successfully, false otherwise.
 */
static bool SendFullPacketToPlayer(Player *player, SOCKET sock) {
    // Basic validation of input pointer.
    if (player == NULL) {
        return false;
    }

    // Create the full level packet buffer.
    LevelPacketBuffer packet;
    if (!LevelCreateFullPacketBuffer(&level, &packet)) {
        printf("Failed to build full level packet.\n");
        return false;
    }

    // Send the packet to the player.
    bool success = false;
    if (packet.size <= (size_t)INT_MAX) {
        // Actually send the packet using sendto.
        // Argments:
        // sock - the socket to send on
        // (const char *)packet.data - pointer to the data to send
        // (int)packet.size - size of the data to send
        // 0 - flags (none)
        // (SOCKADDR *)&player->address - pointer to the destination address
        // (int)sizeof(player->address) - size of the destination address
        int result = sendto(sock,
            (const char *)packet.data,
            (int)packet.size,
            0,
            (SOCKADDR *)&player->address,
            (int)sizeof(player->address));

        // Check for and report any error
        if (result != SOCKET_ERROR) {
            success = true;

            // This player now knows the full level state,
            // so they only need snapshots going forward.
            player->awaitingFullPacket = false;
        } else {
            printf("sendto failed: %d\n", WSAGetLastError());
        }
    } else {
        // In case the packet is too large to send,
        // we report an error for clarity.
        printf("Full packet too large to send (size=%zu).\n", packet.size);
    }

    // Since we've sent the packet (or failed to),
    // we can release the packet buffer now.
    LevelPacketBufferRelease(&packet);
    return success;
}

/**
 * Broadcasts level snapshot packets to all connected players.
 * @param sock The socket to use for sending the packets.
 */
static void BroadcastSnapshots(SOCKET sock) {
    // No point in continuing if there are no players.
    if (playerCount == 0) {
        return;
    }

    // Create the snapshot packet buffer.
    LevelPacketBuffer packet;
    if (!LevelCreateSnapshotPacketBuffer(&level, &packet)) {
        printf("Failed to build snapshot packet.\n");
        return;
    }

    // Send the same data to all players 
    // to save on memory and processing,
    // and to ensure all players have more or
    // less consistent data.
    for (size_t i = 0; i < playerCount; ++i) {
        SendPacketToPlayer(&players[i], sock, &packet);
    }

    // Since we've sent the packet to all players,
    // we can release the packet buffer now.
    LevelPacketBufferRelease(&packet);
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
    static const wchar_t window_class_name[] = L"PixelDrawer";

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
                                      L"Light Year Wars", // Window title (seen in title bar)
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
    if (!InitializeOpenGL(window_handle)) {
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
        ShutdownOpenGL(window_handle);
        return EXIT_FAILURE;
    }

    // Create a UDP socket
    SOCKET sock = CreateUDPSocket();
    if (sock == INVALID_SOCKET) {
        ShutdownOpenGL(window_handle);
        return EXIT_FAILURE;
    }

    // Set non-blocking mode
    if (!SetNonBlocking(sock)) {
        ShutdownOpenGL(window_handle);
        return EXIT_FAILURE;
    }

    // Bind the socket to the local address and port number
    if (!BindSocket(sock, SERVER_PORT)) {
        ShutdownOpenGL(window_handle);
        return EXIT_FAILURE;
    }

    // Buffer to hold incoming messages
    char recv_buffer[512];

    // Delta time calculation variables
    int64_t previous_ticks = GetTicks();
    int64_t frequency = GetTickFrequency();

    printf("Generating a random level...\n");
    LevelInit(&level);
    float level_width = (float)window_width > 0 ? (float)window_width : 800.0f;
    float level_height = (float)window_height > 0 ? (float)window_height : 600.0f;
    GenerateRandomLevel(&level, 4, 2, 5.0f, 15.0f, level_width, level_height, 0);

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

            // Null-terminate the received data to make it a valid string
            recv_buffer[bytes_received] = '\0';
            bool isJoinRequest = bytes_received >= 4 && strncmp(recv_buffer, "JOIN", 4) == 0;

            // Handle join requests
            if (isJoinRequest) {
                // Ensure a player exists for the sender's address
                // and send them the full level packet.
                // This will also check if the server is full
                // and return NULL if no more players can be accepted.
                Player *player = EnsurePlayerForAddress(&sender_address);
                if (player != NULL) {
                    SendFullPacketToPlayer(player, sock);
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
                    
                    // Check for and report any error
                    if (sent == SOCKET_ERROR) {
                        printf("sendto failed: %d\n", WSAGetLastError());
                    }
                }
            }
        } else {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                printf("recvfrom failed: %d\n", error);
            }
        }

        // Calculate delta time
        int64_t current_ticks = GetTicks();
        float delta_time = (float)(current_ticks - previous_ticks) / (float)frequency;
        previous_ticks = current_ticks;

        // Update the level state
        LevelUpdate(&level, delta_time);

        // Keep track of time for snapshot broadcasting
        // Snapshots shall be broadcasted at 1 Hz
        snapshotAccumulator += delta_time;
        while (snapshotAccumulator >= 1.0f) {
            BroadcastSnapshots(sock);
            snapshotAccumulator -= 1.0f;
        }

        // Calculate frames per second (FPS)
        float fps = 0.0f;

        // Make sure not to divide by zero if we lag too hard
        // (lag is bad enough, no reason to make it worse with a crash)
        if (delta_time > 0.0f) {
            fps = 1.0f / delta_time;
        }

        if (window_device_context && window_render_context) {
            // Sets the clear color for the window (background color)
            glClearColor(BACKGROUND_COLOR_R, BACKGROUND_COLOR_G, BACKGROUND_COLOR_B, BACKGROUND_COLOR_A);

            // Clear the color buffer to the clear color
            glClear(GL_COLOR_BUFFER_BIT);

            // Reset the modelview matrix
            glMatrixMode(GL_MODELVIEW);

            // Load the identity matrix to reset any previous transformations
            glLoadIdentity();

            if (window_width > 0 && window_height > 0) {
                // Draw the background gradient
                DrawBackgroundGradient(window_width, window_height);

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
            }

            // Display FPS in the top-left corner
            if (font_display_list_base && window_width >= 10 && window_height >= 10) {
                char fps_string[32];
                snprintf(fps_string, sizeof(fps_string), "FPS: %.0f", fps);

                // Text color is white
                glColor3f(1.0f, 1.0f, 1.0f);

                // Text position is 10 pixels from the left and 20 pixels from the top
                glRasterPos2f(10.0f, 20.0f);

                // Render the FPS string using the font display lists
                glPushAttrib(GL_LIST_BIT);

                // Set the base display list for character rendering
                // This is done by subtracting 32 from font_display_list_base
                // because our display lists start at ASCII character 32 (space character).
                glListBase(font_display_list_base - 32);

                // Call the display lists for each character in the fps_string
                // This renders the string to the screen.
                glCallLists((GLsizei)strlen(fps_string), GL_UNSIGNED_BYTE, fps_string);

                // Restore the previous display list state
                glPopAttrib();
            }

            SwapBuffers(window_device_context);
        }
    }

    // At this point in the code, we are exiting the main loop and need to clean up resources.
    closesocket(sock);
    WSACleanup();
    LevelRelease(&level);
    ShutdownOpenGL(window_handle);
    return EXIT_SUCCESS;
}

/**
 * Initializes OpenGL for the given window.
 * @param window_handle Handle to the window.
 * @return true if successful, false otherwise.
 */
static bool InitializeOpenGL(HWND window_handle) {
    
    // A PIXELFORMATDESCRIPTOR describes the pixel format of a drawing surface.
    // It specifies properties such as color depth, buffer types, and other attributes.
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), // Size of this structure
        1, // Version number
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // Flags (draw to window, support OpenGL, double buffering)
        PFD_TYPE_RGBA, // The kind of framebuffer. RGBA or palette.
        32, // Color depth in bits
        0, 0, 0, 0, 0, 0, // From left to right: Red bits, Red shift, Green bits, Green shift, Blue bits, Blue shift
        0, // Alpha bits
        0, // Alpha shift
        0, // Accumulation buffer bits
        0, 0, 0, 0, // From left to right: Accumulation red bits, green bits, blue bits, alpha bits
        24, // Depth buffer bits
        8, // Stencil buffer bits
        0, // Auxiliary buffer bits
        PFD_MAIN_PLANE, // Layer type (main drawing layer)
        0, // Reserved
        0, 0, 0 // Layer masks (ignored for main plane)
    };

    // Get the device context for the window
    // This essentially prepares the window for OpenGL rendering
    // by obtaining a handle to its device context.
    window_device_context = GetDC(window_handle);

    // Check if device context retrieval was successful
    if (!window_device_context) {
        printf("GetDC failed.\n");
        return false;
    }

    // A pixel format defines how pixels are stored and represented in a drawing surface.
    // This can be things like color depth, buffer types, and other attributes.

    // Choose a pixel format that matches the desired properties
    int pixel_format = ChoosePixelFormat(window_device_context, &pfd);

    // Check if pixel format selection was successful
    if (pixel_format == 0) {
        printf("ChoosePixelFormat failed.\n");
        ReleaseDC(window_handle, window_device_context);
        window_device_context = NULL;
        return false;
    }

    // Set the chosen pixel format for the device context
    if (!SetPixelFormat(window_device_context, pixel_format, &pfd)) {
        printf("SetPixelFormat failed.\n");
        ReleaseDC(window_handle, window_device_context);
        window_device_context = NULL;
        return false;
    }

    // An OpenGL rendering context is a data structure that stores all of OpenGL's state information
    // for drawing to a device context. It is required for any OpenGL rendering to occur.

    // Create the OpenGL rendering context
    window_render_context = wglCreateContext(window_device_context);
    if (!window_render_context) {
        printf("wglCreateContext failed.\n");
        ReleaseDC(window_handle, window_device_context);
        window_device_context = NULL;
        return false;
    }

    // Make the rendering context current
    if (!wglMakeCurrent(window_device_context, window_render_context)) {
        printf("wglMakeCurrent failed.\n");
        wglDeleteContext(window_render_context);
        window_render_context = NULL;
        ReleaseDC(window_handle, window_device_context);
        window_device_context = NULL;
        return false;
    }

    // Set up some basic OpenGL state
    // By default, OpenGL enables depth testing and flat shading.
    // For this application, we want to disable depth testing
    // and use smooth shading for better visual quality.
    glDisable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    // Enable blending for transparent colors and anti-aliasing
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Enable line smoothing for anti-aliased lines
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    // Set the initial viewport and projection matrix
    // This ensures that the OpenGL rendering area matches the window size.
    RECT client_rect;
    if (GetClientRect(window_handle, &client_rect)) {
        window_width = client_rect.right - client_rect.left;
        window_height = client_rect.bottom - client_rect.top;
        UpdateProjection(window_width, window_height);
    }

    // Create a font for rendering text in OpenGL
    HFONT font = CreateFontA(-16, // Height of font. This is negative to indicate character height rather than cell height
                             0, // Width of font. 0 means default width based on height
                             0, // Angle of escapement. Escapement is the angle between the baseline of a character and the x-axis of the device.
                             0, // Orientation angle. This is the angle between the baseline of a character and the x-axis of the device.
                             FW_NORMAL, // Font weight. FW_NORMAL is normal weight.
                             FALSE, // Italic attribute option
                             FALSE, // Underline attribute option
                             FALSE, // Strikeout attribute option
                             ANSI_CHARSET, // Character set identifier
                             OUT_TT_PRECIS, // Output precision. TT means TrueType.
                             CLIP_DEFAULT_PRECIS, // Clipping precision
                             ANTIALIASED_QUALITY, // Output quality
                             FF_DONTCARE | DEFAULT_PITCH, // Family and pitch of the font (don't care about family, default pitch)
                             "Segoe UI"); // Typeface name

    // If font creation was successful, create display lists for characters 32 to 127
    if (font) {
        // Generate display lists for 96 characters (from ASCII 32 to 127)
        font_display_list_base = glGenLists(96);

        // If display list generation was successful, create the font bitmaps
        if (font_display_list_base) {

            // HFONT is a handle to a font object
            // Here we select the created font into the device context
            // so that we can create bitmaps for the font characters.
            HFONT old_font = (HFONT)SelectObject(window_device_context, font);

            // wglUseFontBitmapsA creates a set of bitmap display lists for the glyphs
            // of the currently selected font in the specified device context.
            if (!wglUseFontBitmapsA(window_device_context, 32, 96, font_display_list_base)) {

                // If wglUseFontBitmapsA fails, we clean up the generated display lists
                glDeleteLists(font_display_list_base, 96);

                // Reset font_display_list_base to 0 to indicate failure
                font_display_list_base = 0;
            }

            // Restore the previous font in the device context
            SelectObject(window_device_context, old_font);
        }

        // We can delete the font object now, as the display lists have been created
        DeleteObject(font);
    }

    return true;
}


/**
 * Shuts down OpenGL and releases resources.
 * @param window_handle Handle to the window.
 */
static void ShutdownOpenGL(HWND window_handle) {

    // Delete the OpenGL rendering context and release the device context
    if (window_render_context) {

        // If we created display lists for the font, delete them
        if (font_display_list_base) {
            glDeleteLists(font_display_list_base, 96);
            font_display_list_base = 0;
        }

        // Turn off the current rendering context
        // wglMakeCurrent(NULL, NULL) disassociates the current rendering context from the device context.
        // This is important to do before deleting the rendering context itself.
        wglMakeCurrent(NULL, NULL);

        // Here, we actually delete the rendering context
        wglDeleteContext(window_render_context);
        window_render_context = NULL;
    }

    // Release the device context for the window
    if (window_device_context) {

        // ReleaseDC releases the device context previously obtained by GetDC.
        ReleaseDC(window_handle, window_device_context);
        window_device_context = NULL;
    }
}

/**
 * Updates the OpenGL projection matrix based on the new window size.
 * @param width The new width of the window.
 * @param height The new height of the window.
 */
static void UpdateProjection(int width, int height) {

    // If the rendering context is not initialized, we cannot update the projection
    if (!window_render_context || width <= 0 || height <= 0) {
        return;
    }

    // glViewport sets the viewport, which is the rectangular area of the window where OpenGL will draw.
    // It's parameters are from left to right:
    // x-axis position of the lower left corner of the viewport rectangle, in pixels.
    // y-axis position of the lower left corner of the viewport rectangle, in pixels.
    // width of the viewport.
    // height of the viewport.
    glViewport(0, 0, width, height);

    // Set up an orthographic projection matrix
    // This defines a 2D projection where objects are the same size regardless of their depth.
    glMatrixMode(GL_PROJECTION);

    // glLoadIdentity resets the current matrix to the identity matrix.
    // If we don't do this, the new projection matrix will be multiplied with the existing one,
    // leading to incorrect transformations.
    glLoadIdentity();

    // glOrtho defines a 2D orthographic projection matrix.
    // The parameters specify the clipping volume:
    // left, right, bottom, top, near, far
    glOrtho(0.0, (GLdouble)width, (GLdouble)height, 0.0, -1.0, 1.0);

    // Switch back to the modelview matrix mode
    // This is the matrix mode we will use for rendering objects.
    glMatrixMode(GL_MODELVIEW);
}

/**
 * Draws a radial background gradient.
 * @param width The width of the area to draw the gradient in.
 * @param height The height of the area to draw the gradient in.
 */
static void DrawBackgroundGradient(int width, int height) {
    // Basic validation of input dimensions.
    if (width <= 0 || height <= 0) {
        return;
    }

    // Calculate center and radius for the gradient
    float centerX = (float)width * 0.5f;
    float centerY = (float)height * 0.5f;
    float halfWidth = (float)width * 0.5f;
    float halfHeight = (float)height * 0.5f;
    float radius = sqrtf(halfWidth * halfWidth + halfHeight * halfHeight) * 1.05f;

    // Ensure blending is disabled for the gradient
    // to avoid unintended color mixing
    // with the background clear color.
    glDisable(GL_BLEND);
    DrawRadialGradientRing(centerX, centerY, 0.0f, radius, 128,
        BACKGROUND_GRADIENT_INNER_COLOR, BACKGROUND_GRADIENT_OUTER_COLOR);
}
