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

// Minimum distance in pixels the mouse must move
// for a left button drag to be considered a box selection 
// rather than a simple click upon LMB release.
#define BOX_SELECT_DRAG_THRESHOLD 6.0f

#endif // _CLIENT_H_
