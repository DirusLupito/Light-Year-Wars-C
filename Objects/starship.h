/**
 * Header file for the starship object.
 * @file Objects/starship.h
 * @author abmize
 */

#ifndef _STARSHIP_H_
#define _STARSHIP_H_

#include <stdbool.h>
#include <GL/gl.h>

#include "Objects/vec2.h"
#include "Objects/faction.h"
#include "Utilities/renderUtilities.h"

// Forward declaration in order to avoid circular dependency.
struct Planet;

// Starship constants

// Starship radius for collision detection and rendering.
#define STARSHIP_RADIUS 4.0f

// Maximum speed of a starship in pixels per second.
#define STARSHIP_MAX_SPEED 75.0f

// Acceleration of a starship in pixels per second squared.
#define STARSHIP_ACCELERATION 90.0f

// Initial speed of a starship when created in pixels per second.
// Used when a planet launches its fleet of starships.
#define STARSHIP_INITIAL_SPEED 45.0f

// A starship represents a unit that can be sent from one planet to another.
// A starship has a position, velocity, owner faction, and target planet.
// A starship will always accelerate towards its target planet until it reaches its maximum speed.
// This acceleration is constant, and defined by STARSHIP_ACCELERATION.
// Upon reaching its target planet, the starship will be considered to have collided with it.
// See planet.c's PlanetHandleIncomingShip function for handling the effects of a starship arriving at a planet.
typedef struct Starship {
	Vec2 position;
	Vec2 velocity;
	const Faction *owner;
	struct Planet *target;
} Starship;

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
