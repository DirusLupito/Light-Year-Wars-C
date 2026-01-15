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
#include "Objects/level.h"
#include "Objects/planet.h"
#include "Objects/starship.h"
#include "Objects/player.h"

// Macros
#if RAND_MAX == 32767
#define Rand32() ((rand() << 16) + (rand() << 1) + (rand() & 1))
#else
#define Rand32() rand()
#endif

// Port number for the server to listen on
#define SERVER_PORT 6767

// Color of the background (RGBA)
#define BACKGROUND_COLOR_R 0.3f
#define BACKGROUND_COLOR_G 0.25f
#define BACKGROUND_COLOR_B 0.29f
#define BACKGROUND_COLOR_A 1.0f

// Maximum number of players the server can handle
// (somewhat arbitrary limit for simplicity, can be easily increased or decreased)
#define MAX_PLAYERS 16

#endif // _SERVER_H_
