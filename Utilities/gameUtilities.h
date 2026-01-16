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
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include "Objects/level.h"
#include "Objects/planet.h"
#include "Objects/starship.h"
#include "Objects/vec2.h"
#include "Objects/level.h"

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

/**
 * Generates a random level with the specified parameters.
 * @param level A pointer to the Level object to populate.
 * @param planetCount The number of planets to generate.
 * @param factionCount The number of factions to generate.
 * @param minFleetCapacity The minimum fleet capacity for generated planets.
 * @param maxFleetCapacity The maximum fleet capacity for generated planets.
 * @param width The width of the level.
 * @param height The height of the level.
 * @param seed The seed for the random number generator.
 * @return true if the level was generated successfully, false otherwise.
 */
bool GenerateRandomLevel(Level *level,
    size_t planetCount,
    size_t factionCount,
    float minFleetCapacity,
    float maxFleetCapacity,
    float width,
    float height,
    unsigned int seed);

#endif // _GAME_UTILITIES_H_
