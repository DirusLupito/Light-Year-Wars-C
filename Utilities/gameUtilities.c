/**
 * Contains common utility functions for the Light Year Wars game.
 * @file Utilities/gameUtilities.c
 * @author abmize
 */

#include "Utilities/gameUtilities.h"

// Included here to avoid circular dependency issues.
#include "Objects/planet.h"

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
unsigned int NextRandom(unsigned int *state) {
    const unsigned int a = 1664525u;
    const unsigned int c = 1013904223u;
    *state = a * (*state) + c;
    return *state;
}

/**
 * Helper function to generate a random float in the range [minValue, maxValue).
 * @param state Pointer to the current state of the random number generator.
 * @param minValue The minimum value of the range (inclusive).
 * @param maxValue The maximum value of the range (exclusive).
 * @return A random float in the specified range.
 */
float RandomRange(unsigned int *state, float minValue, float maxValue) {
    unsigned int value = NextRandom(state);
    // Normalize to [0.0, 1.0)
    // By ANDing with 0x00FFFFFFu, we get the lower 24 bits,
    // which gives us a value in the range [0, 16777215].
    // Dividing by 16777216.0f (which is 2^24) normalizes it to [0.0, 1.0).
    // Why 24 bits? It apparently provides a good balance between performance 
    // and randomness quality for floats.
    float normalized = (float)(value & 0x00FFFFFFu) / (float)0x01000000u;
    return minValue + (maxValue - minValue) * normalized;
}

/**
 * Helper function to convert HSV color to RGB color.
 * @param h Hue component (0.0 to 360.0).
 * @param s Saturation component (0.0 to 1.0).
 * @param v Value component (0.0 to 1.0).
 * @param out Output array for RGB components (must have at least 3 elements).
 */
void HSVtoRGB(float h, float s, float v, float out[3]) {
    float c = v * s;
    float hh = fmodf(h, 360.0f) / 60.0f;
    float x = c * (1.0f - fabsf(fmodf(hh, 2.0f) - 1.0f));
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;

    if (hh >= 0.0f && hh < 1.0f) {
        r = c;
        g = x;
    } else if (hh < 2.0f) {
        r = x;
        g = c;
    } else if (hh < 3.0f) {
        g = c;
        b = x;
    } else if (hh < 4.0f) {
        g = x;
        b = c;
    } else if (hh < 5.0f) {
        r = x;
        b = c;
    } else {
        r = c;
        b = x;
    }

    float m = v - c;
    out[0] = r + m;
    out[1] = g + m;
    out[2] = b + m;
}

/**
 * Helper function to calculate the distance between two Vec2 points.
 * @param a The first Vec2 point.
 * @param b The second Vec2 point.
 * @return The distance between the two points.
 */
static float Distance(Vec2 a, Vec2 b) {
    // First, we compute the difference vector between a and b.
    Vec2 delta = Vec2Subtract(a, b);

    // Then, we return the length of that difference vector.
    return Vec2Length(delta);
}

/**
 * Helper function to generate a random position for a planet
 * within the level boundaries, ensuring it stays within the specified radius from the edges.
 * @param state Pointer to the current state of the random number generator.
 * @param width The width of the level.
 * @param height The height of the level.
 * @param radius The radius to keep from the edges.
 * @return A Vec2 representing the random position.
 */
static Vec2 RandomPlanetPosition(unsigned int *state, float width, float height, float radius) {
    float margin = radius + 20.0f;
    float x = RandomRange(state, margin, fmaxf(width - margin, margin));
    float y = RandomRange(state, margin, fmaxf(height - margin, margin));
    Vec2 position = {x, y};
    return position;
}

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
    unsigned int seed) {

    // Basic validation of parameters.
    if (level == NULL) {
        return false;
    }

    // We need at least one planet and at least two factions,
    // but not more factions than planets.
    if (planetCount == 0 || factionCount < 2 || factionCount > planetCount) {
        return false;
    }

    // Fleet capacities must be positive and min must not exceed max.
    if (minFleetCapacity <= 0.0f || maxFleetCapacity < minFleetCapacity) {
        return false;
    }

    // Level dimensions must be positive.
    if (width <= 0.0f || height <= 0.0f) {
        return false;
    }

    // Used for random number generation.
    // If seed is zero, we use a default seed for reproducibility.
    unsigned int state = (seed == 0u) ? 0x12345678u : seed;

    // Configure the level with the required number of factions and planets.
    // We also allocate an initial capacity for starships.
    size_t initialShipCapacity = planetCount * 4;
    if (!LevelConfigure(level, factionCount, planetCount, initialShipCapacity)) {
        return false;
    }

    // Now that the level is configured, we can fill in its details.
    level->width = width;
    level->height = height;
    level->starshipCount = 0;

    // Here, we create factions with distinct colors.
    // We use HSV color space to ensure good color distribution.
    float baseHue = RandomRange(&state, 0.0f, 360.0f);

    // Distribute hues evenly among factions.
    float hueStep = 360.0f / (float)factionCount;
    for (size_t i = 0; i < factionCount; ++i) {
        // Calculate hue for this faction.
        float hue = baseHue + hueStep * (float)i;
        float rgb[3];
        // Then convert HSV to RGB for actual display and storage.
        HSVtoRGB(hue, 0.6f, 0.95f, rgb);
        level->factions[i] = CreateFaction((int)i, rgb[0], rgb[1], rgb[2]);
    }

    // Now we create planets with random positions and fleet capacities.
    // We try to ensure planets do not overlap by checking distances.

    // Minimum separation padding between planets.
    const float separationPadding = 25.0f;
    for (size_t i = 0; i < planetCount; ++i) {
        // Create a planet with random fleet capacity within the range [minFleetCapacity, maxFleetCapacity].
        float capacity = RandomRange(&state, minFleetCapacity, maxFleetCapacity);
        Planet planet = CreatePlanet(Vec2Zero(), capacity, NULL);

        // Attempt to place the planet without overlapping existing planets.
        bool placed = false;

        // We try up to 64 times to find a non-overlapping position.
        for (int attempts = 0; attempts < 64 && !placed; ++attempts) {
            Vec2 candidate = RandomPlanetPosition(&state, width, height, PlanetGetOuterRadius(&planet));
            placed = true;
            for (size_t j = 0; j < i; ++j) {
                float minDistance = PlanetGetOuterRadius(&planet) + PlanetGetOuterRadius(&level->planets[j]) + separationPadding;
                if (Distance(candidate, level->planets[j].position) < minDistance) {
                    placed = false;
                    break;
                }
            }
            if (placed) {
                planet.position = candidate;
            }
        }

        if (!placed) {
            // If we failed to place the planet after many attempts,
            // we just place it anyway, potentially overlapping.
            planet.position = RandomPlanetPosition(&state, width, height, PlanetGetOuterRadius(&planet));
        }

        // For the first factionCount planets, assign each to a different faction.
        // The rest remain unowned.
        // This ensures each faction starts with exactly one planet.
        // We also spawn these planets with full fleet capacity, for a faster start.
        planet.currentFleetSize = (i < factionCount) ? planet.maxFleetCapacity : 0.0f;
        planet.claimant = NULL;
        planet.owner = (i < factionCount) ? &level->factions[i] : NULL;
        level->planets[i] = planet;
    }

    return true;
}
