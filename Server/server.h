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
#include "Utilities/gameUtilities.h"
#include "Utilities/networkUtilities.h"
#include "Utilities/renderUtilities.h"

// Macros
#if RAND_MAX == 32767
#define Rand32() ((rand() << 16) + (rand() << 1) + (rand() & 1))
#else
#define Rand32() rand()
#endif

#define SERVER_PORT 6767

#endif // _SERVER_H_
