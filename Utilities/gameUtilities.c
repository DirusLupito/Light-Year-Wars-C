/**
 * Contains common utility functions for the Light Year Wars game.
 * @file Utilities/gameUtilities.c
 * @author abmize
 */

#include "Utilities/gameUtilities.h"

/**
 * Gets the current tick count using QueryPerformanceCounter.
 * @return The current tick count.
 */
int64_t GetTicks() {
    LARGE_INTEGER ticks;
    if (!QueryPerformanceCounter(&ticks)) {
        printf("QueryPerformanceCounter failed.\n");
        return 0;
    }
    return ticks.QuadPart;
}

/**
 * Gets the tick frequency using QueryPerformanceFrequency.
 * @return The tick frequency.
 */
int64_t GetTickFrequency() {
    LARGE_INTEGER frequency;
    if (!QueryPerformanceFrequency(&frequency)) {
        printf("QueryPerformanceFrequency failed.\n");
        return 0;
    }
    return frequency.QuadPart;
}