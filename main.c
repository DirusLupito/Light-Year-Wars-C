/**
 * Main file for Light Year Wars C. Holds the program entry point and main loop.
 * @file main.c
 * @author abmize
*/
#define UNICODE
#define _UNICODE
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <ws2tcpip.h>
#include <profileapi.h>
#include <wingdi.h>

//Used to exit main program loop.
static bool running = true;

struct {
    int width;
    int height;
    uint32_t *pixels;
} frame = {0};

#if RAND_MAX == 32767
#define Rand32() ((rand() << 16) + (rand() << 1) + (rand() & 1))
#else
#define Rand32() rand()
#endif

//Tells GDI about the pixel format
static BITMAPINFO frame_bitmap_info;
//Bitmap handle to encapsulate the bitmap data
static HBITMAP frame_bitmap = 0;
//Device context handle to point to the bitmap handle (redundant, but necessary to use GDI)
static HDC frame_device_context = 0;

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
        case WM_QUIT:
        case WM_DESTROY: {
            running = false;
        } break;

        //All window drawing has to happen inside the WM_PAINT message.
        case WM_PAINT: {
            static PAINTSTRUCT paint;
            static HDC device_context;
            //In order to enable window drawing, BeginPaint must be called.
            //It fills out the PAINTSTRUCT and gives a device context handle for painting
            device_context = BeginPaint(window_handle, &paint);
            //BitBlt will copy the pixel array data over to the window in the specified rectangle
            //It is given the window painting device context, 
            //and the left, top, width and height of the area which is to be (re)painted.
            //Here, it is possible to just pass in 0, 0, window width, window height, 
            //but instead the paint structure rectangle is passed in 
            //so as to only paint the area that needs to be painted.
            BitBlt(device_context,
                   paint.rcPaint.left, 
                   paint.rcPaint.top,
                   paint.rcPaint.right - paint.rcPaint.left, 
                   paint.rcPaint.bottom - paint.rcPaint.top,
                   frame_device_context,
                   paint.rcPaint.left, 
                   paint.rcPaint.top,
                   SRCCOPY);
            //If end paint is not called, everything seems to work, 
            //but the documentation says that it is necessary.
            EndPaint(window_handle, &paint);
        } break;

        //WM_SIZE is sent when the window is created or resized.
        //This makes it an ideal place to assign the size of the pixel array
        //and finish setting up the GDI bitmap.
        case WM_SIZE: {
            //Get the width and height of the window.
            frame_bitmap_info.bmiHeader.biWidth  = LOWORD(lParam);
            frame_bitmap_info.bmiHeader.biHeight = HIWORD(lParam);

            //If the bitmap object was already created, then delete it
            //then create a new bitmap with the unchanged info from before 
            //and the new width and height

            if(frame_bitmap) DeleteObject(frame_bitmap);
            //DIB_RGB_COLORS just tells CreateDIBSection what kind of data is being used
            //A pointer to the pixel array pointer is passed in
            //CreateDIBSection will fill the pixel array pointer with an address to some memory 
            //big enough to hold the type and quantity of pixels wanted, 
            //based on the width, height and bits per pixel.
            frame_bitmap = CreateDIBSection(NULL, 
            &frame_bitmap_info, 
            DIB_RGB_COLORS, (void**)&frame.pixels, 
            0, 
            0);
            //SelectObject is used to point the device context to it
            SelectObject(frame_device_context, frame_bitmap);
            //At this point the GDI objects and pixel array memory are setup

            frame.width =  LOWORD(lParam);
            frame.height = HIWORD(lParam);

        } break;


        default: {
            //If the message is not handled by this procedure, 
            //pass it to the default window procedure.
            return DefWindowProc(window_handle, msg, wParam, lParam);
        } break;
    }
    return 0;
}

static inline int64_t GetTicks() {
    LARGE_INTEGER ticks;
    if (!QueryPerformanceCounter(&ticks)) {
        printf("QueryPerformanceCounter failed.\n");
        return 0;
    }
    return ticks.QuadPart;
}

static inline int64_t GetTickFrequency() {
    LARGE_INTEGER frequency;
    if (!QueryPerformanceFrequency(&frequency)) {
        printf("QueryPerformanceFrequency failed.\n");
        return 0;
    }
    return frequency.QuadPart;
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

    // Create the window class to hold information about the window.
    static WNDCLASS window_class = {0};
    //L = wide character string literal
    //This name is used to reference the window class later.
    static const wchar_t window_class_name[] = L"PixelDrawer";
    //Set up the window class name.
    window_class.lpszClassName = window_class_name;
    //Set up a pointer to a function that windows will call to handle events, or messages.
    window_class.lpfnWndProc = WindowProcessMessage;
    window_class.hInstance = hInstance;

    //Register the window class with windows.
    if (!RegisterClass(&window_class)) {
        printf("RegisterClass failed.\n");
        exit(1);
    }

    //Set up the bitmap info.
    frame_bitmap_info.bmiHeader.biSize = sizeof(frame_bitmap_info.bmiHeader);
    //Number of color planes is always 1.
    frame_bitmap_info.bmiHeader.biPlanes = 1;
    //Bits per pixel
    //8 bits per byte, a byte for each of red, green, blue, and a filler byte.
    frame_bitmap_info.bmiHeader.biBitCount = 32;
    //Compression type is uncompressed RGB.
    frame_bitmap_info.bmiHeader.biCompression = BI_RGB;
    //Create the device context handle.
    frame_device_context = CreateCompatibleDC(0);

    //Create the window.
    HWND window_handle = CreateWindow(window_class_name, //Name of the window class.
                                      L"Light Year Wars", //Title of the window.
                                      WS_OVERLAPPEDWINDOW, //Window style.
                                      CW_USEDEFAULT, //Initial horizontal position of the window.
                                      CW_USEDEFAULT, //Initial vertical position of the window.
                                      CW_USEDEFAULT, //Initial width of the window.
                                      CW_USEDEFAULT, //Initial height of the window.
                                      NULL, //Handle to the parent window.
                                      NULL, //Handle to the menu.
                                      hInstance, //Handle to the instance of the program.
                                      NULL); //Pointer to the window creation data.
    //Handle any errors.
    if (!window_handle) {
        printf("CreateWindow failed.\n");
        exit(1);
    }

    //Actually show the window.
    ShowWindow(window_handle, nCmdShow);

    bool red = true;
    printf("Starting UDP listener on port 6767...\n");

    // We set up a UDP socket to listen for messages from some other computer.
    // We will then respond to it with "Ok", then change the screen color from red to blue or vice versa.

    // WinSock initialization
    // WSADATA is a structure that is filled with information about the Windows Sockets implementation.
    WSADATA wsaData;
    // WSAStartup is a function that initializes the Windows Sockets DLL.
    // Here, we are requesting version 2.2 of the Windows Sockets specification.
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        // WSAGetLastError retrieves the error code for the last Windows Sockets operation that failed.
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return EXIT_FAILURE;
    }
    // Create a UDP socket
    // AF_INET specifies the address family (IPv4)
    // SOCK_DGRAM specifies the socket type (datagram/UDP)
    // IPPROTO_UDP specifies the protocol (UDP)
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // recvfrom is a blocking function, so the program will wait at that call until a message is received.
    // We don't want it to block though, so we set the socket to non-blocking mode.
    u_long mode = 1; // 1 to enable non-blocking mode

    // ioctlsocket is used to control aspects of the socket's behavior.
    // The first argument is the socket to modify.
    // The second argument is the command to execute (FIONBIO to set non-blocking mode).
    // The third argument is a pointer to the value to set (mode).
    // Had mode been 0, it would set the socket to blocking mode.
    // With it set to 1, recvfrom will return immediately if there is no data to read.
    // bytes_received will be SOCKET_ERROR and WSAGetLastError will return WSAEWOULDBLOCK.
    ioctlsocket(sock, FIONBIO, &mode);

    if (sock == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        // The documentation also says we should have a call to WSACleanup when we're done with Winsock
        // but apparently when exiting, Windows automatically cleans up anyway.
        return EXIT_FAILURE;
    }

    // Set up the local address structure
    SOCKADDR_IN local_address;
    // AF_INET says that the address family is IPv4 for this socket
    local_address.sin_family = AF_INET;
    // The assignment of the port number is wrapped in a call to htons (host to network short), 
    // this converts the port number from whatever endianness the executing machine has, to big-endian.
    local_address.sin_port = htons( 6767 ); 
    // INADDR_ANY means that we will accept messages sent to any of the local machine's IP addresses 
    // (of course, it has to be sent to the correct port as well, 6767 in this case).
    local_address.sin_addr.s_addr = INADDR_ANY;
    // Bind the socket to the local address and port number
    // bind() takes a socket, a pointer to a sockaddr structure (we have to cast our SOCKADDR_IN pointer to a SOCKADDR pointer),
    // and the size of the address structure.
    if( bind( sock, (SOCKADDR*)&local_address, sizeof( local_address ) ) == SOCKET_ERROR )
    {
        printf( "bind failed: %d", WSAGetLastError() );
        return EXIT_FAILURE;
    }

    // Buffer to hold incoming messages
    char recv_buffer[512];

    // Delta time
    int64_t previous_ticks = GetTicks();
    int64_t frequency = GetTickFrequency();

    printf("Entering main program loop...\n");

    //Main program loop.
    while (running) {
        //Handle any messages sent to the window.
        static MSG message = {0};
        //Check for the next message and remove it from the message queue.
        while(PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
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
            const char *response_message = "Ok";

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
            // Toggle the screen color between red and blue
            red = !red;
        }

        // Here is where game logic and pixel drawing code would go.
        // For example, to fill the screen with red pixels then alternate to blue:
        if(red) {
            for(int y = 0; y < frame.height; y++) {
                for(int x = 0; x < frame.width; x++) {
                    frame.pixels[y * frame.width + x] = 0x000000FF; // Red in 0xAABBGGRR format
                }
            }
        } else {
            for(int y = 0; y < frame.height; y++) {
                for(int x = 0; x < frame.width; x++) {
                    frame.pixels[y * frame.width + x] = 0x00FF0000; // Blue in 0xAABBGGRR format
                }
            }
        }

        // If the game window is big enough, also display an FPS counter in the top-left corner
        if(frame.width >= 10 && frame.height >= 10) {
            int64_t current_ticks = GetTicks();
            float delta_time = (float)(current_ticks - previous_ticks) / (float)frequency;
            previous_ticks = current_ticks;
            float fps = 1.0f / delta_time;
            // Simple FPS counter drawing (white text on black background)
            int fps_int = (int)(fps + 0.5f);
            char fps_string[16];
            snprintf(fps_string, sizeof(fps_string), "FPS: %d", fps_int);
            // fps_string must be a TCHAR string for Windows API
            SetBkColor(frame_device_context, RGB(0, 0, 0)); // Black background
            SetTextColor(frame_device_context, RGB(255, 255, 255)); // White text
            TextOutA(frame_device_context, 10, 10, fps_string, (int)strlen(fps_string));    
        }

        //In games, it is usually desirable to redraw the full window many times per second
        //InvalidateRect marks a section of the window as invalid and needing to be redrawn.
        //Passing in NULL invalidates the entire window.
        InvalidateRect(window_handle, NULL, FALSE);
        //UpdateWindow immediately passes a WM_PAINT message to WindowProcessMessage 
        //rather than waiting until the next message processing loop
        UpdateWindow(window_handle);
    }

    return EXIT_SUCCESS;
}
