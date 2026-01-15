/**
 * Implements a planet object.
 * A planet represents an object that can be owned by a faction.
 * It has the capacity to hold a fleet of starships.
 * It can be captured by factions through incoming starships.
 * A planet can also be claimed or unclaimed.
 * Unclaimed planets do not belong to any faction and can be captured by any faction sending starships to it.
 * Unclaimed planets do not inherently generate starships over time, so outside of incoming starships, their fleet size remains static.
 * Claimed planets will gradually adjust their fleet size towards their max fleet capacity over time.
 * A claimant is the faction who has currently put the most resource towards capturing an unclaimed planet.
 * Should the claimant succeed in sending enough ships such that currentFleetSize meets or exceeds maxFleetCapacity,
 * the planet becomes owned by the claimant faction.
 * @file Objects/planet.c
 * @author abmize
 */

#include "Objects/planet.h"

#include "Objects/level.h"
#include "Objects/starship.h"

// Used for drawing unowned planets.
static const float NEUTRAL_COLOR[4] = {0.6f, 0.6f, 0.6f, 1.0f};
// The glow alpha values for neutral, claimed, and owned planets.
// This will control how intense the glow effect is when rendering.
// Higher alpha means a more pronounced glow.
static const float NEUTRAL_GLOW_ALPHA = 0.08f;
static const float CLAIM_GLOW_ALPHA = 0.12f;
static const float OWNED_GLOW_ALPHA = 0.16f;

// This feathering value controls how soft the edges of the planet ring are.
// Higher values mean softer edges.
static const float PLANET_RING_FEATHER = 1.5f;

// Forward declaration of helper function for drawing planet glow.
static void PlanetDrawGlow(const Planet *planet, float outerRadius);

/**
 * Creates a new planet with the specified position, max fleet capacity, and owner.
 * The planet's current fleet size is initialized to 0.0f.
 * The planet's claimant is initialized to NULL.
 * @param position The position of the planet.
 * @param maxFleetCapacity The maximum fleet capacity of the planet.
 * @param owner A pointer to the Faction that owns the planet, or NULL if unowned.
 * @return The created Planet object.
 */
Planet CreatePlanet(Vec2 position, float maxFleetCapacity, const Faction *owner) {
    // We first make a Planet struct
	Planet planet;

    // Then we set its members based on the provided parameters
    // and sensible defaults.
    // A planet is a sufficiently simple object
    // that it does not require any special handling during creation.
    // No dynamic memory allocation is necessary here.
	planet.position = position;
	planet.maxFleetCapacity = fmaxf(maxFleetCapacity, 0.0f);
	planet.currentFleetSize = 0.0f;
	planet.owner = owner;
	planet.claimant = NULL;
	return planet;
}

/**
 * Updates the state of the planet over time.
 * This function adjusts the planet's current fleet size towards its max fleet capacity
 * based on whether it is owned or unowned, and whether it has a claimant.
 * @param planet A pointer to the Planet object to update.
 * @param deltaTime The time elapsed since the last update, in seconds.
 */
void PlanetUpdate(Planet *planet, float deltaTime) {
	if (planet == NULL) {
		return;
	}

    // Planets have different behaviors based on their ownership status.

    // 1. Unowned planets with no claimant do not change their fleet size.
    // 2. Unowned planets with a claimant clamp their fleet size between 0 and max capacity.
    //    Note that ownership transfer logic is handled in PlanetHandleIncomingShip.
    // 3. Owned planets adjust their fleet size towards max capacity at a fixed rate.

    // Handle unowned planets first.
	if (planet->owner == NULL) {
		if (planet->claimant == NULL) {
            // No owner and no claimant means no change.
			planet->currentFleetSize = 0.0f;
		} else {
            // No owner but has a claimant means we clamp the fleet size.
			if (planet->currentFleetSize < 0.0f) {
				planet->currentFleetSize = 0.0f;
			}
			if (planet->currentFleetSize > planet->maxFleetCapacity) {
				planet->currentFleetSize = planet->maxFleetCapacity;
			}
		}
		return;
	}

    // At this point, we know the planet is owned.
    // So we adjust the fleet size towards max capacity.
	float target = planet->maxFleetCapacity;
	if (planet->currentFleetSize > target) {
        // Reduce fleet size towards target.
		planet->currentFleetSize -= PLANET_FLEET_ADJUST_RATE * deltaTime;
		if (planet->currentFleetSize < target) {
			planet->currentFleetSize = target;
		}
	} else if (planet->currentFleetSize < target) {
        // Increase fleet size towards target.
		planet->currentFleetSize += PLANET_FLEET_ADJUST_RATE * deltaTime;
		if (planet->currentFleetSize > target) {
			planet->currentFleetSize = target;
		}
	}

    // Ensure fleet size does not go negative.
	if (planet->currentFleetSize < 0.0f) {
		planet->currentFleetSize = 0.0f;
	}
}

/**
 * Clamps the given fleet size to be within valid bounds for the planet.
 * Specifically, it ensures the fleet size is between 0 and the planet's max fleet capacity.
 * @param planet A pointer to the Planet object.
 * @param fleetSize The fleet size to clamp.
 * @return The clamped fleet size.
 */
static float ClampFleetSize(const Planet *planet, float fleetSize) {
    // If the planet is null, that behavior is undefined.
    // Should not be possible if used correctly.

    // If the planet has no max capacity, return 0.0f.
	if (planet->maxFleetCapacity <= 0.0f) {
		return 0.0f;
	}

    // Otherwise, clamp the fleet size between 0 and max capacity.
	float clamped = fleetSize;
	if (clamped < 0.0f) {
		clamped = 0.0f;
	}

	float cap = planet->maxFleetCapacity;
	if (clamped > cap) {
		clamped = cap;
	}

	return clamped;
}

/**
 * Gets the outer radius of the planet based on its max fleet capacity.
 * The outer radius is calculated as maxFleetCapacity * PLANET_RADIUS_SCALE,
 * with a minimum value of 12.0f to ensure visibility.
 * @param planet A pointer to the Planet object.
 * @return The outer radius of the planet.
 */
float PlanetGetOuterRadius(const Planet *planet) {
	if (planet == NULL) {
		return 0.0f;
	}

    // Calculate the outer radius based on max fleet capacity.
	float base = planet->maxFleetCapacity * PLANET_RADIUS_SCALE;

    // Minimum outer radius is 12.0f.
	return fmaxf(base, 12.0f);
}

/**
 * Gets the inner radius of the planet based on its current fleet size.
 * The inner radius is calculated as a proportion of the outer radius,
 * scaled by the ratio of currentFleetSize to maxFleetCapacity.
 * If the planet is over capacity, the inner radius will be equal to the outer radius.
 * @param planet A pointer to the Planet object.
 * @return The inner radius of the planet.
 */
float PlanetGetInnerRadius(const Planet *planet) {
	if (planet == NULL || planet->maxFleetCapacity <= 0.0f) {
		return 0.0f;
	}

    // The inner radius is based on the current fleet size ratio.
    // The specific formula ensures it stays within the ring thickness.
    // It is given by:
    // innerRadius = (outerRadius - PLANET_RING_THICKNESS) * (clampedFleetSize / maxFleetCapacity)
	float outerRadius = PlanetGetOuterRadius(planet);
	float innerLimit = outerRadius - PLANET_RING_THICKNESS;
	float ratio = ClampFleetSize(planet, planet->currentFleetSize) / planet->maxFleetCapacity;

    // Ensure we do not return a negative inner radius.
	return fmaxf(innerLimit * ratio, 0.0f);
}

/**
 * Gets the collision radius of the planet.
 * The collision radius is defined as the maximum of the outer and inner radii.
 * This radius is used for collision detection with incoming starships.
 * @param planet A pointer to the Planet object.
 * @return The collision radius of the planet.
 */
float PlanetGetCollisionRadius(const Planet *planet) {
	if (planet == NULL) {
		return 0.0f;
	}

	float outerRadius = PlanetGetOuterRadius(planet);
	float innerRadius = PlanetGetInnerRadius(planet);
	return fmaxf(outerRadius, innerRadius);
}

/**
 * Helper function to draw the claim progress of an unowned planet.
 * This function draws a filled circle representing the claim progress
 * based on the planet's current fleet size and max fleet capacity.
 * The circle is drawn in the color of the claimant faction.
 * @param planet A pointer to the Planet object.
 * @param outerRadius The outer radius of the planet.
 */
static void DrawClaimProgress(const Planet *planet, float outerRadius) {
	if (planet->claimant == NULL) {
		return;
	}

	float innerEdge = fmaxf(outerRadius - PLANET_RING_THICKNESS, 0.0f);
	float ratio = 0.0f;
	if (planet->maxFleetCapacity > 0.0f) {
		ratio = ClampFleetSize(planet, planet->currentFleetSize) / planet->maxFleetCapacity;
	}
	float radius = innerEdge * ratio;

	if (radius <= 0.0f) {
		return;
	}

	if (planet->claimant != NULL) {
		glColor4fv(planet->claimant->color);
	}
	DrawFilledCircle(planet->position.x, planet->position.y, radius, 128);
}

/**
 * Draws the planet using OpenGL.
 * This function renders the planet's ring, inner circle, and claim progress if applicable.
 * We use the planet's owner and claimant to determine colors.
 * Meanwhile, the size of the planet on the screen is based on its fleet capacity.
 * Its position on the screen is determined by its position member.
 * @param planet A pointer to the Planet object to draw.
 */
void PlanetDraw(const Planet *planet) {
	if (planet == NULL) {
		return;
	}

	float outerRadius = PlanetGetOuterRadius(planet);
	float innerRadius = PlanetGetInnerRadius(planet);

	// We first draw the glow effect for the planet.
	PlanetDrawGlow(planet, outerRadius);

    // Determine if the planet is over capacity.
	bool overCapacity = planet->currentFleetSize > planet->maxFleetCapacity && planet->owner != NULL;

    // If the planet is not over capacity, draw the ring.
    // Otherwise, we skip drawing the ring since it would be fully encompassed by the filled circle
    // drawn for the number of ships present on the planet.
	if (!overCapacity) {
		float ringColor[4];
		if (planet->owner != NULL) {
			ringColor[0] = planet->owner->color[0];
			ringColor[1] = planet->owner->color[1];
			ringColor[2] = planet->owner->color[2];
			ringColor[3] = planet->owner->color[3];
		} else {
			ringColor[0] = NEUTRAL_COLOR[0];
			ringColor[1] = NEUTRAL_COLOR[1];
			ringColor[2] = NEUTRAL_COLOR[2];
			ringColor[3] = NEUTRAL_COLOR[3];
		}

		float ringInner = fmaxf(outerRadius - PLANET_RING_THICKNESS, 0.0f);
		DrawSmoothRing(planet->position.x, planet->position.y, ringInner, outerRadius, 128,
			PLANET_RING_FEATHER, ringColor);
	}

    // Draw the inner filled circle representing the current fleet size.
    // If the planet is over capacity, we draw the filled circle using the outer radius.
	if (planet->owner != NULL) {
        // If owned, draw in owner's color.
		float radius = overCapacity ? outerRadius : innerRadius;
		if (radius > 0.0f) {
			glColor4fv(planet->owner->color);
			DrawFilledCircle(planet->position.x, planet->position.y, radius, 128);
		}
	} else if (planet->claimant != NULL) {
        // If unowned but claimed, draw in claimant's color.
		DrawClaimProgress(planet, outerRadius);
	}
}

/**
 * Draws the glow effect around the planet.
 * The glow color and intensity depend on whether the planet is owned, claimed, or unowned.
 * @param planet A pointer to the Planet object.
 * @param outerRadius The outer radius of the planet.
 */
static void PlanetDrawGlow(const Planet *planet, float outerRadius) {
	// Basic validation of parameters.
	if (planet == NULL || outerRadius <= 0.0f) {
		return;
	}

	// Determine the glow color and alpha based on ownership status.
	// We first initialize to neutral/default values.
	const float *sourceColor = NEUTRAL_COLOR;
	float alpha = NEUTRAL_GLOW_ALPHA;

	// Then we override based on ownership.
	if (planet->owner != NULL) {
		sourceColor = planet->owner->color;
		alpha = OWNED_GLOW_ALPHA;
	} else if (planet->claimant != NULL) {
		sourceColor = planet->claimant->color;
		alpha = CLAIM_GLOW_ALPHA;
	}

	// And then, we draw the radial gradient ring for the glow effect.
	float innerColor[4] = {sourceColor[0], sourceColor[1], sourceColor[2], alpha};
	float outerColor[4] = {sourceColor[0], sourceColor[1], sourceColor[2], 0.0f};
	float haloInnerRadius = outerRadius;
	float haloOuterRadius = outerRadius + fmaxf(outerRadius * 0.55f, 28.0f);

	// What glEnable(GL_BLEND) does is enable blending in OpenGL.
	// Blending is a technique used to combine the color of a source pixel (the pixel being drawn)
	// with the color of a destination pixel (the pixel already present in the framebuffer)
	glEnable(GL_BLEND);

	// glBlendFunc specifies how the blending is done.
	// The parameters GL_SRC_ALPHA and GL_ONE
	// mean that the source color is multiplied by its alpha value,
	// and the destination color is multiplied by 1 (i.e., added as-is).
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	// Finally, we draw the radial gradient ring.
	DrawRadialGradientRing(planet->position.x, planet->position.y,
		haloInnerRadius, haloOuterRadius, 64, innerColor, outerColor);

	// After drawing, we reset the blending function back to the standard alpha blending.
	// This is important to ensure that subsequent drawing operations are not 
	// affected by our custom blending mode.
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// We then disable blending to clean up the OpenGL state.
	glDisable(GL_BLEND);
}

/**
 * Spawns starships in a circular formation around the origin planet,
 * heading towards the destination planet.
 * Used instead of some random scatter to ensure deterministic behavior,
 * which is particularly useful for consistent results in multiplayer scenarios.
 * @param level A pointer to the Level object where starships will be spawned.
 * @param origin A pointer to the origin Planet object.
 * @param destination A pointer to the destination Planet object.
 * @param shipCount The number of starships to spawn.
 */
static void SpawnShipCircle(Level *level, Planet *origin, Planet *destination, int shipCount) {
	if (shipCount <= 0) {
		return;
	}

	float outerRadius = PlanetGetOuterRadius(origin);
	float spawnRadius = outerRadius + STARSHIP_RADIUS * 1.5f;
	float angleStep = (float)(2.0 * M_PI) / (float)shipCount;

    // We spawn the starships evenly spaced in a circle around the origin planet
    // starting from angle 0 and moving counter-clockwise.
	for (int i = 0; i < shipCount; ++i) {

        // Calculate the spawn position and velocity for each starship.
		float angle = angleStep * i;
		Vec2 direction = {cosf(angle), sinf(angle)};
		Vec2 position = Vec2Add(origin->position, Vec2Scale(direction, spawnRadius));
		Vec2 velocity = Vec2Scale(direction, STARSHIP_INITIAL_SPEED);
        
        // Spawn the starship heading towards the destination planet.
		if (LevelSpawnStarship(level, position, velocity, origin->owner, destination) == NULL) {
			break;
		}
	}
}

/**
 * Sends a fleet from the origin planet to the destination planet.
 * This function checks if the origin planet is owned and has enough fleet size to send.
 * It also ensures that the origin and destination planets are not the same.
 * If valid, it spawns starships in the level heading towards the destination planet.
 * Specifically, the number of starships sent is equal to the rounded current fleet size of the origin planet.
 * The origin planet's current fleet size is reduced to 0.0f after sending.
 * @param origin A pointer to the origin Planet object.
 * @param destination A pointer to the destination Planet object.
 * @param level A pointer to the Level object where starships will be spawned.
 * @return true if the fleet was successfully sent, false otherwise.
 */
bool PlanetSendFleet(Planet *origin, Planet *destination, Level *level) {
    // Basic validation of parameters.
	if (origin == NULL || destination == NULL || level == NULL) {
		return false;
	}

    // A planet cannot send a fleet to itself.
	if (origin == destination) {
		return false;
	}

    // The origin planet must be owned to send a fleet.
	if (origin->owner == NULL) {
		return false;
	}

    // Determine the number of starships to send.
    // We round the current fleet size to the nearest integer.
	int shipCount = (int)floorf(origin->currentFleetSize + 0.5f);
	if (shipCount <= 0) {
		return false;
	}

    // Reduce the origin planet's fleet size to 0.0f
    // since we are sending all available ships.
	origin->currentFleetSize = 0.0f;

    // Actually spawn the starship objects into the level.
	SpawnShipCircle(level, origin, destination, shipCount);
	return true;
}

/**
 * Handles an incoming starship to the planet.
 * This function processes the interaction between the incoming starship and the planet.
 * Depending on the ownership and claimant status of the planet,
 * it updates the planet's current fleet size, owner, and claimant accordingly.
 * @param planet A pointer to the Planet object receiving the starship.
 * @param ship A pointer to the incoming Starship object.
 */
void PlanetHandleIncomingShip(Planet *planet, const Starship *ship) {
	if (planet == NULL || ship == NULL || ship->owner == NULL) {
		return;
	}

    // We assume that the ship has an owner faction.
	const Faction *attacker = ship->owner;

    // Handle the interaction based on the planet's ownership status.
	if (planet->owner != NULL) {

        // Planet is owned.
		if (planet->owner == attacker) {
            // One ship from owner increases fleet size
            // by 1.0f.
			planet->currentFleetSize += 1.0f;
			return;
		}

        // Otherwise one ship from attacker decreases fleet size
        // by 1.0f.
		planet->currentFleetSize -= 1.0f;
		if (planet->currentFleetSize < 0.0f) {
            // In case of negative fleet size,
            // it indicates the defending forces have been
            // totally reduced, and the current owners are
            // no longer capable of exercising control over the planet.
            // Thus, ownership transfers to the attacker.

            // We need to make sure that the leftovers from the attacker which finished
            // off the defending forces are carried over to the new ownership.
			float surplus = -planet->currentFleetSize;
			planet->owner = attacker;
			planet->claimant = NULL;
			planet->currentFleetSize = fmaxf(surplus, 1.0f);
		}
		return;
	}

    // Planet is unowned and unclaimed.
	if (planet->claimant == NULL) {
        // First ship from attacker claims the planet.
		planet->claimant = attacker;
		planet->currentFleetSize = 1.0f;
		return;
	}

    // Planet is unowned but claimed.
	if (planet->claimant == attacker) {
        // One ship from claimant increases fleet size
        // by 1.0f.
		planet->currentFleetSize += 1.0f;
		if (planet->currentFleetSize >= planet->maxFleetCapacity && planet->maxFleetCapacity > 0.0f) {
            // If the claimant's fleet size meets or exceeds max capacity,
            // they successfully capture the planet.
			planet->owner = planet->claimant;
			planet->claimant = NULL;
			planet->currentFleetSize = planet->maxFleetCapacity;
		}
		return;
	}

    // Ship is from a different faction than the claimant.
    // One ship from attacker decreases fleet size by 1.0f,
    // thereby interrupting the claimant's progress towards capturing the planet.
	planet->currentFleetSize -= 1.0f;
	if (planet->currentFleetSize <= 0.0f) {
		planet->claimant = attacker;
		planet->currentFleetSize = 1.0f;
	}
}
