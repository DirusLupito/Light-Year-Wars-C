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

// Forward declarations of static functions
static bool InitializeOpenGL(HWND window_handle);
static void ShutdownOpenGL(HWND window_handle);
static void UpdateProjection(int width, int height);

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
        default: {
            //If the message is not handled by this procedure, 
            //pass it to the default window procedure.
            return DefWindowProc(window_handle, msg, wParam, lParam);
        }
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

    float opacity = 1.0f;
    printf("Starting UDP listener on port %d...\n", SERVER_PORT);

    // We set up a UDP socket to listen for messages from some other computer.
    // We will then respond to it with "Ok", then change the screen color from red to blue or vice versa.

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
        // then we process the message and respond with "Ok"
        if (bytes_received != SOCKET_ERROR) {

            // Null-terminate the received data to make it a valid string
            recv_buffer[bytes_received] = '\0';

            // Print the received message (for debugging purposes)
            printf("Received message: %s\n", recv_buffer);

            // If we received a message, we respond with "Ok"
            const char* response_message = "Ok";

            // Send the response back to the sender
            // sendto takes the following arguments here:
            // 1. The socket to send data on. In this case, our UDP socket

            // 2. A buffer containing the data to send. Here, we use response_message.

            // 3. The length of the data to send. Here, we use strlen(response_message).

            // 4. Flags to modify the behavior of sendto. Here, we use 0 for no special behavior.

            // 5. A pointer to a sockaddr structure containing the recipient's address.
            //    We cast our SOCKADDR_IN pointer to a SOCKADDR pointer.

            // 6. The size of the recipient address structure.
            int bytes_sent = sendto(sock, response_message, (int)strlen(response_message), 0,
                                    (SOCKADDR*)&sender_address, sender_address_size);

            // In case of an error (determined by bytes_sent being SOCKET_ERROR), we print the error
            if (bytes_sent == SOCKET_ERROR) {
                printf("sendto failed: %d\n", WSAGetLastError());
            }
            // Reset opacity to make the window fully blue
            opacity = 1.0f;
        }

        // Calculate delta time
        int64_t current_ticks = GetTicks();
        float delta_time = (float)(current_ticks - previous_ticks) / (float)frequency;
        previous_ticks = current_ticks;

        // Decrease opacity over time
        opacity -= 0.5f * delta_time;

        // Clamp opacity to [0.0, 1.0]
        if (opacity < 0.0f) {
            opacity = 0.0f;
        }

        // Calculate frames per second (FPS)
        float fps = 0.0f;

        // Make sure not to divide by zero if we lag too hard
        // (lag is bad enough, no reason to make it worse with a crash)
        if (delta_time > 0.0f) {
            fps = 1.0f / delta_time;
        }

        if (window_device_context && window_render_context) {
            float background_red = 1.0f - opacity;
            float background_blue = opacity;
            glClearColor(background_red, 0.0f, background_blue, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            if (window_width > 0 && window_height > 0) {
                glColor3f(1.0f, 1.0f, 1.0f);
                DrawCircle((float)window_width * 0.5f,
                            (float)window_height * 0.5f,
                            50.0f,
                            128,
                            2.0f);
            }

            if (font_display_list_base && window_width >= 10 && window_height >= 10) {
                char fps_string[32];
                snprintf(fps_string, sizeof(fps_string), "FPS: %.0f", fps);
                glColor3f(1.0f, 1.0f, 1.0f);
                glRasterPos2f(10.0f, 20.0f);
                glPushAttrib(GL_LIST_BIT);
                glListBase(font_display_list_base - 32);
                glCallLists((GLsizei)strlen(fps_string), GL_UNSIGNED_BYTE, fps_string);
                glPopAttrib();
            }

            SwapBuffers(window_device_context);
        }
    }

    // At this point in the code, we are exiting the main loop and need to clean up resources.
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
