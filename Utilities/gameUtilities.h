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
#include "Objects/starship.h"
#include "Objects/vec2.h"

// Forward declarations
// of necessary structs to avoid circular dependencies.
struct Level;
struct Starship;
struct Planet;

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

/**
 * Helper function to generate the next random number
 * using a linear congruential generator (LCG), given
 * the current state (previous random number).
 * Implements the following recurrence relation:
 * X_{n+1} = (a * X_n + c) mod m
 * where:
 * a = 1664525
 * c = 1013904223
 * m = 2^32
 * Since we are using unsigned int, the modulus operation is implicit due to overflow.
 * The constants a and c were found on the Internet and Wikipedia's page on LCGs
 * says they come from ranqd1.
 * @param state Pointer to the current state of the random number generator.
 * @return The next random number in the sequence.
 */
unsigned int NextRandom(unsigned int *state);

/**
 * Helper function to generate a random float in the range [minValue, maxValue).
 * @param state Pointer to the current state of the random number generator.
 * @param minValue The minimum value of the range (inclusive).
 * @param maxValue The maximum value of the range (exclusive).
 * @return A random float in the specified range.
 */
float RandomRange(unsigned int *state, float minValue, float maxValue);

#endif // _GAME_UTILITIES_H_
