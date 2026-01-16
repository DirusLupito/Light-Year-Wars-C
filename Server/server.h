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
#include "Objects/level.h"
#include "Objects/planet.h"
#include "Objects/starship.h"
#include "Objects/player.h"

// Port number for the server to listen on
#define SERVER_PORT 22311

// Maximum number of players the server can handle
// (somewhat arbitrary limit for simplicity, can be easily increased or decreased)
#define MAX_PLAYERS 16

// Interval at which to broadcast planet state snapshots to all clients (in seconds).
#define PLANET_STATE_BROADCAST_INTERVAL (1.0f / 20.0f) // 20 Hz

#endif // _SERVER_H_
