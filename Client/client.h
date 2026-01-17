/**
 * Header file for the Light Year Wars client application.
 * @author abmize
 * @file Client/client.h
 */
#ifndef _CLIENT_H_
#define _CLIENT_H_

#define UNICODE
#define _UNICODE
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "Utilities/networkUtilities.h"
#include "Objects/level.h"
#include "Utilities/gameUtilities.h"
#include "Utilities/renderUtilities.h"
#include "Objects/planet.h"
#include "Objects/starship.h"
#include "Utilities/openglUtilities.h"
#include "Utilities/playerInterfaceUtilities.h"
#include "Utilities/cameraUtilities.h"

// Minimum distance in pixels the mouse must move
// for a left button drag to be considered a box selection 
// rather than a simple click upon LMB release.
#define BOX_SELECT_DRAG_THRESHOLD 6.0f

// Howfar the mouse must be from the edge of the window
// before edge panning begins (in pixels).
#define CAMERA_EDGE_PAN_MARGIN 24.0f

// Speed at which the camera pans when using keyboard input (in world units per second).
#define CAMERA_KEY_PAN_SPEED 480.0f

// Speed at which the camera pans when using edge panning (in world units per second).
#define CAMERA_EDGE_PAN_SPEED 420.0f

// Minimum zoom level for the camera.
#define CAMERA_MIN_ZOOM 0.5f

// Maximum zoom level for the camera.
#define CAMERA_MAX_ZOOM 2.75f

// How much the zoom changes each time a scroll of the mouse wheel is detected.
#define CAMERA_ZOOM_FACTOR 1.1f

#endif // _CLIENT_H_
