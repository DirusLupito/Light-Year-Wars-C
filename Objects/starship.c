/**
 * Implements a starship object.
 * A starship represents a unit that can be sent from one planet to another.
 * A starship has a position, velocity, owner faction, and target planet.
 * A starship will always accelerate towards its target planet until it reaches its maximum speed.
 * This acceleration is constant, and defined by STARSHIP_ACCELERATION.
 * Upon reaching its target planet, the starship will be considered to have collided with it.
 * See planet.c's PlanetHandleIncomingShip function for handling the effects of a starship arriving at a planet.
 * @file Objects/starship.c
 * @author abmize
 */

#include "Objects/starship.h"

#include "Objects/planet.h"

/**
 * Creates a new starship with the specified position, velocity, owner, and target.
 * The starship's velocity is clamped to STARSHIP_MAX_SPEED.
 * @param position The initial position of the starship.
 * @param velocity The initial velocity of the starship.
 * @param owner A pointer to the Faction that owns the starship.
 * @param target A pointer to the Planet that is the target of the starship.
 * @return The created Starship object.
 */
Starship CreateStarship(Vec2 position, Vec2 velocity, const Faction *owner, Planet *target) {
    // We first make a Starship struct
	Starship ship;

    // Then we set its members based on the provided parameters.
    // We clamp the velocity to ensure it does not exceed STARSHIP_MAX_SPEED.
    // Much like Planet, a starship is such a simple object
    // that it does not require any special handling during creation.
    // No dynamic memory allocation is necessary here.
	ship.position = position;
	ship.velocity = Vec2ClampToLength(velocity, STARSHIP_MAX_SPEED);
	ship.owner = owner;
	ship.target = target;
	return ship;
}

/**
 * Updates the state of the starship over time.
 * This function accelerates the starship towards its target planet
 * and updates its position based on its velocity.
 * @param ship A pointer to the Starship object to update.
 * @param deltaTime The time elapsed since the last update, in seconds.
 */
void StarshipUpdate(Starship *ship, float deltaTime) {
	if (ship == NULL) {
		return;
	}

    // Target should never be NULL for a valid starship,
    // but we check anyway to be safe.
	if (ship->target != NULL) {
        // The relevant math is as follows:
        // We want to accelerate the starship towards its target planet.
        // To do this, we first compute the vector from the starship to the planet.
        // Then we normalize this vector to get the direction.
        // We then scale this direction by the acceleration constant and deltaTime
        // to get the change in velocity.
        // Finally, we add this change in velocity to the starship's current velocity.
        // We also clamp the resulting velocity to STARSHIP_MAX_SPEED.

        // So very basically:
        // toTarget = target.position - ship.position
        // direction = Normalize(toTarget)
        // acceleration = direction * (STARSHIP_ACCELERATION * deltaTime)
        // ship.velocity += acceleration
        // ship.velocity = ClampToLength(ship.velocity, STARSHIP_MAX_SPEED)
		Vec2 toTarget = Vec2Subtract(ship->target->position, ship->position);
		Vec2 direction = Vec2Normalize(toTarget);
		Vec2 acceleration = Vec2Scale(direction, STARSHIP_ACCELERATION * deltaTime);
		ship->velocity = Vec2Add(ship->velocity, acceleration);
		ship->velocity = Vec2ClampToLength(ship->velocity, STARSHIP_MAX_SPEED);
	}

    // Finally, we update the starship's position based on its velocity.
    // This is simply position += velocity * deltaTime.
	Vec2 delta = Vec2Scale(ship->velocity, deltaTime);
	ship->position = Vec2Add(ship->position, delta);
}

/**
 * Checks if the starship has collided with its target planet.
 * A collision is detected if the distance between the starship and the planet
 * is less than or equal to the sum of their collision radii.
 * @param ship A pointer to the Starship object to check for collision.
 * @return true if the starship has collided with its target planet, false otherwise.
 */
bool StarshipCheckCollision(const Starship *ship) {
	if (ship == NULL || ship->target == NULL) {
		return false;
	}

    // We get the combined collision radius of the planet and starship.
    // Then we compute the distance between the starship and planet.
	float collisionRadius = PlanetGetCollisionRadius(ship->target) + STARSHIP_RADIUS;
	Vec2 toTarget = Vec2Subtract(ship->position, ship->target->position);
	float distance = Vec2Length(toTarget);

    // If the distance is less than or equal to the collision radius, we have a collision.
	return distance <= collisionRadius;
}

/**
 * Draws the starship using OpenGL.
 * The starship is drawn as a filled circle at its position,
 * using the color of its owner faction if available.
 * If the starship has no owner, a default gray color is used.
 * @param ship A pointer to the Starship object to draw.
 */
void StarshipDraw(const Starship *ship) {
	if (ship == NULL) {
		return;
	}

	if (ship->owner != NULL) {
		glColor4fv(ship->owner->color);
	} else {
        // Default color for unowned starships (gray).
        // Though it should never happen in practice.
		glColor4f(0.7f, 0.7f, 0.7f, 1.0f);
	}

	DrawFilledCircle(ship->position.x, ship->position.y, STARSHIP_RADIUS, 20);
}
