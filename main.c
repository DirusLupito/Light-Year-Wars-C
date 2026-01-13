/**
 * Main file for Light Year Wars C. Holds the program entry point and main loop.
 * @file main.c
 * @author abmize
*/
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

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

    // Counts how long each loop of the main program takes in milliseconds.
    uint64_t frame_time = 0;
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

        // Here is where game logic and pixel drawing code would go.
        // For example, to fill the screen with random colors:
        for(int y = 0; y < frame.height; y++) {
            for(int x = 0; x < frame.width; x++) {
                frame.pixels[y * frame.width + x] = 0xFF000000 | (Rand32() & 0x00FFFFFF);
            }
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