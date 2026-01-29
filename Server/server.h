/**
 * Header file for the Light Year Wars server application.
 * @author abmize
 * @file Server/server.h
 */
#ifndef _SERVER_H_
#define _SERVER_H_

#define UNICODE
#define _UNICODE
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <ws2tcpip.h>
#include <wingdi.h>
#include <string.h>
#include <GL/gl.h>
#include <limits.h>
#include "Utilities/gameUtilities.h"
#include "Utilities/networkUtilities.h"
#include "Utilities/renderUtilities.h"
#include "Utilities/openglUtilities.h"
#include "Utilities/cameraUtilities.h"
#include "Utilities/MenuUtilities/lobbyMenuUtilities.h"
#include "Utilities/MenuUtilities/lobbyPreviewUtilities.h"
#include "Utilities/MenuUtilities/gameOverUIUtilities.h"
#include "Utilities/soundManagerUtilities.h"
#include "Objects/level.h"
#include "Objects/planet.h"
#include "Objects/starship.h"
#include "Objects/player.h"
#include "AI/aiPersonality.h"

// Port number for the server to listen on
#define SERVER_PORT 22311

// Maximum number of players the server can handle
// (somewhat arbitrary limit for simplicity, can be easily increased or decreased)
#define MAX_PLAYERS 16

// Interval at which to broadcast planet state snapshots to all clients (in seconds).
#define PLANET_STATE_BROADCAST_INTERVAL (1.0f / 20.0f) // 20 Hz

// Howfar the mouse must be from the edge of the window
// before edge panning begins (in pixels).
#define SERVER_CAMERA_EDGE_MARGIN 24.0f

// Speed at which the camera pans when using keyboard input (in world units per second).
#define SERVER_CAMERA_KEY_SPEED 480.0f

// Speed at which the camera pans when using edge panning (in world units per second).
#define SERVER_CAMERA_EDGE_SPEED 420.0f

// Minimum zoom level for the camera.
#define SERVER_CAMERA_MIN_ZOOM 0.5f

// Maximum zoom level for the camera.
#define SERVER_CAMERA_MAX_ZOOM 2.75f

// How much the zoom changes each time a scroll of the mouse wheel is detected.
#define SERVER_CAMERA_ZOOM_FACTOR 1.1f

// Default seed used for ship spawn position RNG if none is provided.
#define SHIP_SPAWN_SEED 0x12345678u

// Time in milliseconds the server shall wait for a message before considering
// a client to have timed out.
#define CLIENT_TIMEOUT_MS 1800000

/**
 * Defines the various stages the server application can be in.
 * Used to determine which logic and rendering to perform.
 */
typedef enum ServerStage {
    SERVER_STAGE_LOBBY = 0,
    SERVER_STAGE_GAME = 1
} ServerStage;

#endif // _SERVER_H_
