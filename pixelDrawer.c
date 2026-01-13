/**
 * Simple pixel drawing program which opens a window and draws
 * pixels to it.
 * Made with help from https://www.youtube.com/watch?v=q1fMa8Hufmg
 * @file pixelDrawer.c
 * @author Alex Mize
*/
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

/** Number of white pixels to draw per frame. */
//#define WHITE_PIXELS_PER_FRAME 1000

/** How many frames it should take to fill the window with white pixels. */
//#define FRAMES_TO_FILL_WINDOW 1200

/** Initial radius of the circle to be drawn, in pixels */
#define INITIAL_CIRCLE_RADIUS 0

/** Initial side length of the triangle to be drawn, in pixels */
#define INITIAL_TRIANGLE_SIDE_LENGTH 100

/** Circle center x coordinate (how far from the left) */
//#define CIRCLE_CENTER_X 300

/** Circle center y coordinate (how far from the bottom) */
//#define CIRCLE_CENTER_Y 300

/** Number of black pixels to draw per frame. */
#define RANDOM_PIXELS_PER_FRAME 300

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
 * Draws a circle centered at circle_center_x, circle_center_y with radius circle_radius
 * @param circle_center_x x coordinate of the center of the circle
 * @param circle_center_y y coordinate of the center of the circle
 * @param circle_radius radius of the circle
*/
void drawCircle(int circle_center_x, int circle_center_y, int circle_radius) {

    //Mark the center with a single red pixel surrounded by green pixels

    //Make sure that drawing the center won't segfault
    if (circle_center_x > 0 
        && circle_center_x < frame.width 
        && circle_center_y > 0 
        && circle_center_y < frame.height) {
        frame.pixels[circle_center_x + circle_center_y*frame.width] = 0x00FF0000;
        frame.pixels[circle_center_x + circle_center_y*frame.width - 1] = 0x0000FF00;
        frame.pixels[circle_center_x + circle_center_y*frame.width + 1] = 0x0000FF00;
        frame.pixels[circle_center_x + (circle_center_y-1)*frame.width] = 0x0000FF00;
        frame.pixels[circle_center_x + (circle_center_y+1)*frame.width] = 0x0000FF00;
    }

    //Draw nothing else for radii less than or equal to 1
    if (circle_radius <= 1) {
        return;
    }

    //Calculate and write the upper semi-circle, y = sqrt(r^2 - x^2)
    for (int x = -circle_radius; x <= circle_radius; x++) {
        int y = sqrt(circle_radius*circle_radius - x*x);
        if (circle_center_x + x >= 0 
            && circle_center_x + x < frame.width 
            && circle_center_y + y >= 0 
            && circle_center_y + y < frame.height) {
            frame.pixels[(circle_center_x + x) + (circle_center_y + y)*frame.width] = 0x00FFFFFF;
            //To avoid artifacts from the rounding error in the sqrt function,
            //also fill in the above and below pixels
            if (circle_center_x + x >= 0 
                && circle_center_x + x < frame.width 
                && circle_center_y + y - 1 >= 0 
                && circle_center_y + y - 1 < frame.height) {
                frame.pixels[(circle_center_x + x) + (circle_center_y + y - 1)*frame.width] = 0x000000FF;
            }
            if (circle_center_x + x >= 0 
                && circle_center_x + x < frame.width 
                && circle_center_y + y + 1 >= 0 
                && circle_center_y + y + 1 < frame.height) {
                frame.pixels[(circle_center_x + x) + (circle_center_y + y + 1)*frame.width] = 0x000000FF;
            }
        }
        //In order to keep the circle from having gaps in it,
        //draw a line from the current x-y value straight up until
        //hitting the previous y value or the next y value, depending
        //on if x is less than or greater than the center of the circle

        //Handle the case in which x is less than the center of the circle
        if(x < 0) {
            int next_y = sqrt(circle_radius*circle_radius - (x+1)*(x+1));

            //Draw the line
            for (int i = y; i < next_y; i++) {
                if (circle_center_x + x >= 0 
                    && circle_center_x + x < frame.width 
                    && circle_center_y + i >= 0 
                    && circle_center_y + i < frame.height) {
                    frame.pixels[(circle_center_x + x) + (circle_center_y + i)*frame.width] = 0x0000FF00;
                }
            }
        } else if (x > 0) {
            //Handle the case in which x is greater than the center of the circle

            int prev_y = sqrt(circle_radius*circle_radius - (x-1)*(x-1));

            //Draw the line
            for (int i = y; i < prev_y; i++) {
                if (circle_center_x + x >= 0 
                    && circle_center_x + x < frame.width 
                    && circle_center_y + i >= 0 
                    && circle_center_y + i < frame.height) {
                    frame.pixels[(circle_center_x + x) + (circle_center_y + i)*frame.width] = 0x0000FF00;
                }
            }
        }
    }
        
    //Calculate and write the lower semi-circle, y = -sqrt(r^2 - x^2)
    for (int x = -circle_radius; x <= circle_radius; x++) {
        int y = -sqrt(circle_radius*circle_radius - x*x);
        if (circle_center_x + x >= 0 
            && circle_center_x + x < frame.width 
            && circle_center_y + y >= 0 
            && circle_center_y + y < frame.height) {
            frame.pixels[(circle_center_x + x) + (circle_center_y + y)*frame.width] = 0x00FFFFFF;
            //To avoid artifacts from the rounding error in the sqrt function,
            //also fill in the above and below pixels
            if (circle_center_x + x >= 0 
                && circle_center_x + x < frame.width 
                && circle_center_y + y - 1 >= 0 
                && circle_center_y + y - 1 < frame.height) {
                frame.pixels[(circle_center_x + x) + (circle_center_y + y - 1)*frame.width] = 0x000000FF;
            }
            if (circle_center_x + x >= 0 
                && circle_center_x + x < frame.width 
                && circle_center_y + y + 1 >= 0 
                && circle_center_y + y + 1 < frame.height) {
                frame.pixels[(circle_center_x + x) + (circle_center_y + y + 1)*frame.width] = 0x000000FF;
            }
        }

        //In order to keep the circle from having gaps in it,
        //draw a line from the current x-y value straight down until
        //hitting the previous y value or the next y value, depending
        //on if x is less than or greater than the center of the circle

        //Handle the case in which x is less than the center of the circle
        if(x < 0) {
            int next_y = -sqrt(circle_radius*circle_radius - (x+1)*(x+1));

            //Draw the line
            for (int i = y; i > next_y; i--) {
                if (circle_center_x + x >= 0 
                    && circle_center_x + x < frame.width 
                    && circle_center_y + i >= 0 
                    && circle_center_y + i < frame.height) {
                    frame.pixels[(circle_center_x + x) + (circle_center_y + i)*frame.width] = 0x0000FF00;
                }
            }
        } else if (x > 0) {
            //Handle the case in which x is greater than the center of the circle

            int prev_y = -sqrt(circle_radius*circle_radius - (x-1)*(x-1));

            //Draw the line
            for (int i = y; i > prev_y; i--) {
                if (circle_center_x + x >= 0 
                    && circle_center_x + x < frame.width 
                    && circle_center_y + i >= 0 
                    && circle_center_y + i < frame.height) {
                    frame.pixels[(circle_center_x + x) + (circle_center_y + i)*frame.width] = 0x0000FF00;
                }
            }
        }
    }
}

/**
 * Draws a 45 45 90 triangle with peak at 
 * triangle_top_x, triangle_top_y and with side length side_length
 * @param triangle_top_x x coordinate of the center of the triangle
 * @param triangle_top_y y coordinate of the center of the triangle
 * @param side_length side length of the triangle
*/
void drawTriangle(int triangle_top_x, int triangle_top_y, int side_length) {
    //Mark the top with a single red pixel

    //Make sure that drawing the top won't segfault
    if (triangle_top_x > 0 
        && triangle_top_x < frame.width 
        && triangle_top_y > 0 
        && triangle_top_y < frame.height) {
        frame.pixels[triangle_top_x + triangle_top_y*frame.width] = 0x00FF0000;
    }
    
    //Draw nothing else for side lengths less than or equal to 1
    if (side_length <= 1) {
        return;
    }

    //Draw the lower edge of the triangle

    //x coordinate of the leftmost pixel of the lower edge
    int l_left_x = triangle_top_x - side_length/2;

    //y coordinate of the lower edge
    int l_edge_y = triangle_top_y - side_length/2;

    //Actually draw the lower edge
    for (int x = l_left_x; x <= l_left_x + side_length; x++) {
        //Make sure that drawing the lower edge won't segfault
        if (x >= 0 
            && x < frame.width 
            && l_edge_y >= 0 
            && l_edge_y < frame.height) {
            frame.pixels[x + l_edge_y*frame.width] = 0x00FFFFFF;
        }
    }

    //Draw the left edge of the triangle
    //Do this by finding all the points y = mx + b, where
    //m = 1 and b = l_edge_y + 1

    for (int x = l_left_x; x <= triangle_top_x; x++) {
        //Make sure that drawing the left edge won't segfault
        if (x >= 0 
            && x < frame.width 
            && l_edge_y + (x - l_left_x) + 1 >= 0 
            && l_edge_y + (x - l_left_x) + 1 < frame.height) {
            frame.pixels[x + (l_edge_y + x - l_left_x + 1)*frame.width] = 0x00FFFFFF;
        }
    }

    //Draw the right edge of the triangle
    //Do this by finding all the points y = mx + b, where
    //m = -1 and b = l_edge_y + triangle_top_x - l_left_x + 1

    for (int x = triangle_top_x + 1; x <= l_left_x + side_length; x++) {
        //Make sure that drawing the right edge won't segfault
        if (x >= 0 
            && x < frame.width 
            && l_edge_y + (l_left_x + side_length - x) + 1 >= 0 
            && l_edge_y + (l_left_x + side_length - x) + 1 < frame.height) {
            frame.pixels[x + (l_edge_y + l_left_x + side_length - x + 1)*frame.width] = 0x00FFFFFF;
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
                                      L"Pixel Drawer", //Title of the window.
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

    //Circle radius
    int circle_radius = INITIAL_CIRCLE_RADIUS;

    //Circle center x coordinate (how far from the left)
    int circle_center_x = frame.width/2;

    //Circle center y coordinate (how far from the bottom)
    int circle_center_y = frame.height/2;

    //Triangle side length
    int side_length = INITIAL_TRIANGLE_SIDE_LENGTH;

    //Triangle top x coordinate (how far from the left)
    int triangle_top_x = frame.width/2;

    //Triangle top y coordinate (how far from the bottom)
    int triangle_top_y = frame.height/2;


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

        //Draw a circle centered at circle_center_x, circle_center_y with radius circle_radius
        drawCircle(circle_center_x, circle_center_y, circle_radius);

        //Draw a triangle with a peak at triangle_top_x, triangle_top_y 
        //and with side length side_length
        drawTriangle(triangle_top_x, triangle_top_y, side_length);

        //Increase the side length after each frame
        side_length++;

        //Reset the side length if it is larger than the height or width of the window
        if (side_length > frame.width || side_length > frame.height) {
            side_length = 0;
        }

        //Randomize the coordinates of the triangle peak after each frame
        //Make sure that a divide by zero error won't occur
        if (frame.width == 0 || frame.height == 0) {
            triangle_top_x = 0;
            triangle_top_y = 0;
        } else {
            triangle_top_x = Rand32()%frame.width;
            triangle_top_y = Rand32()%frame.height;
        }

        //Increase the circle radius after each frame
        circle_radius++;

        //Reset the circle radius if it is larger than half the height or width of the window
        if (circle_radius > frame.width/3 || circle_radius > frame.height/3) {
            circle_radius = 0;
        }

        //Set RANDOM_PIXELS_PER_FRAME random pixels to a random color.
        for (int i = 0; i < RANDOM_PIXELS_PER_FRAME; i++) {
            //Set a random pixel to a random color.

            //Make sure frame.width*frame.height is not 0
            if (frame.width*frame.height == 0) {
                break;
            }
            frame.pixels[Rand32()%(frame.width*frame.height)] = Rand32();
        }

        //Update the center of the frame to account for any window resizing
        circle_center_x = frame.width/2;
        circle_center_y = frame.height/2;

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