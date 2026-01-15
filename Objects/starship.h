/**
 * Header file for the starship object.
 * @file Objects/starship.h
 * @author abmize
 */

#ifndef _STARSHIP_H_
#define _STARSHIP_H_

#include <stdbool.h>
#include <stddef.h>
#include <GL/gl.h>
#include <string.h>

#include "Objects/vec2.h"
#include "Objects/faction.h"
#include "Utilities/renderUtilities.h"

// Forward declaration in order to avoid circular dependency.
struct Planet;

// Starship constants

// Starship radius for collision detection and rendering.
#define STARSHIP_RADIUS 1.0f

// Maximum speed of a starship in pixels per second.
#define STARSHIP_MAX_SPEED 75.0f

// Acceleration of a starship in pixels per second squared.
#define STARSHIP_ACCELERATION 90.0f

// Initial speed of a starship when created in pixels per second.
// Used when a planet launches its fleet of starships.
#define STARSHIP_INITIAL_SPEED 45.0f

// How long the starship trail lasts in seconds.
// This will control the age at which trail samples are removed.
#define STARSHIP_TRAIL_LENGTH_SECONDS 1.0f

// Maximum number of samples in the starship trail.
#define STARSHIP_TRAIL_MAX_SAMPLES 24

// Minimum distance the starship must travel before emitting a new trail sample.
#define STARSHIP_TRAIL_MIN_DISTANCE 2.5f

// Time interval in seconds between emitting new trail samples.
#define STARSHIP_TRAIL_EMIT_INTERVAL 0.05f

// A starship trail sample represents a single point in the starship's trail.
// It contains the position of the sample and its age in seconds.
typedef struct StarshipTrailSample {
	Vec2 position;
	float age;
} StarshipTrailSample;

// A starship trail effect represents the visual trail left behind by a starship.
// It contains an array of trail samples, the count of valid samples,
// and the color used for rendering the trail.
typedef struct StarshipTrailEffect {
	StarshipTrailSample samples[STARSHIP_TRAIL_MAX_SAMPLES];
	size_t sampleCount;
	float color[4];
} StarshipTrailEffect;

// A starship represents a unit that can be sent from one planet to another.
// A starship has a position, velocity, owner faction, and target planet.
// A starship also has a trail of samples for rendering its trail effect, and a timer
// which helps determine when to emit new trail samples.
// A starship will always accelerate towards its target planet until it reaches its maximum speed.
// This acceleration is constant, and defined by STARSHIP_ACCELERATION.
// Upon reaching its target planet, the starship will be considered to have collided with it.
// See planet.c's PlanetHandleIncomingShip function for handling the effects of a starship arriving at a planet.
typedef struct Starship {
	Vec2 position;
	Vec2 velocity;
	const Faction *owner;
	struct Planet *target;
	size_t trailCount;
	float trailTimeSinceLastEmit;
	StarshipTrailSample trail[STARSHIP_TRAIL_MAX_SAMPLES];
} Starship;

/**
 * Resolves (that is, determines) the color of the starship based on its owner faction.
 * If the starship has an owner, returns the owner's color.
 * Otherwise, returns a default gray color.
 * @param ship A pointer to the Starship object.
 * @param outColor An array of 4 floats to receive the RGBA color.
 */
void StarshipResolveColor(const Starship *ship, float outColor[4]);

/**
 * Initializes a starship trail effect based on the given starship's trail data.
 * Copies the trail samples from the starship into the effect,
 * and sets the effect's color to the specified color.
 * @param effect A pointer to the StarshipTrailEffect to initialize.
 * @param ship A pointer to the Starship whose trail data to use.
 * @param color An array of 4 floats representing the RGBA color for the trail effect.
 */
void StarshipTrailEffectInit(StarshipTrailEffect *effect, const Starship *ship, const float color[4]);

/**
 * Updates the starship trail effect over time.
 * Advances the ages of the trail samples by deltaTime.
 * @param effect A pointer to the StarshipTrailEffect to update.
 * @param deltaTime The time elapsed since the last update, in seconds.
 */
void StarshipTrailEffectUpdate(StarshipTrailEffect *effect, float deltaTime);

/**
 * Determines if the starship trail effect is still alive.
 * A trail effect is considered alive if it has more than one sample.
 * @param effect A pointer to the StarshipTrailEffect to check.
 * @return true if the effect is alive, false otherwise.
 */
bool StarshipTrailEffectIsAlive(const StarshipTrailEffect *effect);

/**
 * Draws the starship trail effect.
 * Renders the trail samples as a strip with fading colors.
 * @param effect A pointer to the StarshipTrailEffect to draw.
 */
void StarshipTrailEffectDraw(const StarshipTrailEffect *effect);

/**
 * Creates a new starship with the specified position, velocity, owner, and target.
 * The starship's velocity is clamped to STARSHIP_MAX_SPEED.
 * @param position The initial position of the starship.
 * @param velocity The initial velocity of the starship.
 * @param owner A pointer to the Faction that owns the starship.
 * @param target A pointer to the Planet that is the target of the starship.
 * @return The created Starship object.
 */
Starship CreateStarship(Vec2 position, Vec2 velocity, const Faction *owner, struct Planet *target);

/**
 * Updates the state of the starship over time.
 * This function accelerates the starship towards its target planet
 * and updates its position based on its velocity.
 * @param ship A pointer to the Starship object to update.
 * @param deltaTime The time elapsed since the last update, in seconds.
 */
void StarshipUpdate(Starship *ship, float deltaTime);

/**
 * Checks if the starship has collided with its target planet.
 * A collision is detected if the distance between the starship and the planet
 * is less than or equal to the sum of their collision radii.
 * @param ship A pointer to the Starship object to check for collision.
 * @return true if the starship has collided with its target planet, false otherwise.
 */
bool StarshipCheckCollision(const Starship *ship);

/**
 * Draws the starship using OpenGL.
 * The starship is drawn as a filled circle at its position,
 * using the color of its owner faction if available.
 * If the starship has no owner, a default gray color is used.
 * @param ship A pointer to the Starship object to draw.
 */
void StarshipDraw(const Starship *ship);

#endif // _STARSHIP_H_
