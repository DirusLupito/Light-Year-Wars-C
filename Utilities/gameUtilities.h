/**
 * Header file for the game utility functions file.
 * @file Utilities/gameUtilities.h
 * @author abmize
 */
#ifndef _GAME_UTILITIES_H_
#define _GAME_UTILITIES_H_

#include <profileapi.h>
#include <stdint.h>
#include <stdio.h>

/**
 * Gets the current tick count using QueryPerformanceCounter.
 * @return The current tick count.
 */
int64_t GetTicks();

/**
 * Gets the tick frequency using QueryPerformanceFrequency.
 * @return The tick frequency.
 */
int64_t GetTickFrequency();

#endif // _GAME_UTILITIES_H_
