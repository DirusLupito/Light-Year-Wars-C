/**
 * Header file for the planet object.
 * @file Objects/planet.h
 * @author abmize
 */

#ifndef _PLANET_H_
#define _PLANET_H_

#include <stdbool.h>
#include <GL/gl.h>
#include <math.h>

#include "Utilities/renderUtilities.h"
#include "Objects/faction.h"
#include "Objects/vec2.h"

// Forward declarations
// of necessary structs to avoid circular dependencies.
struct Level;
struct Starship;

// Planet constants

// Planet radius is scaled based on max fleet capacity.
// So here, PLANET_RADIUS_SCALE tells us that if a planet has a max fleet capacity of 1.0f,
// its outer radius will be 3.5f units.
#define PLANET_RADIUS_SCALE 3.5f

// The thickness of the planet's ring that indicates fleet capacity.
#define PLANET_RING_THICKNESS 10.0f

// The rate at which the planet's current fleet size increases towards its max fleet capacity.
// This is measured in starships per second.
// Should a planet have fewer starships than its max capacity,
// it will gain starships at this rate until it reaches max capacity.
#define PLANET_FLEET_BUILD_RATE 2.0f

// The multiplier used when decreasing fleet size when over capacity.
// This is applied to the difference between currentFleetSize and maxFleetCapacity.
// Every second, the portion of the fleet size above max capacity
// is multiplied by this value to determine how much to reduce it by.
// Set this to a value within the range (0.0f, 1.0f) for proper exponential decay behavior.
// A value of 1.0f would mean no reduction at all,
// while a value of 0.0f would mean instant reduction to max capacity.
#define PLANET_FLEET_REDUCTION_MULTIPLIER 0.5f

// A planet represents an object that can be owned by a faction.
// It has the capacity to hold a fleet of starships.
// It can be captured by factions through incoming starships.
// A planet can also be claimed or unclaimed.
// Unclaimed planets do not belong to any faction and can be captured by any faction sending starships to it.
// Unclaimed planets do not inherently generate starships over time, so outside of incoming starships, their fleet size remains static.
// Claimed planets will gradually adjust their fleet size towards their max fleet capacity over time.
// A claimant is the faction who has currently put the most resource towards capturing an unclaimed planet.
// Should the claimant succeed in sending enough ships such that currentFleetSize meets or exceeds maxFleetCapacity,
// the planet becomes owned by the claimant faction.
typedef struct Planet {
    Vec2 position;
    float maxFleetCapacity;
    float currentFleetSize;
    const Faction *owner;
    const Faction *claimant;
} Planet;

/**
 * Creates a new planet with the specified position, max fleet capacity, and owner.
 * The planet's current fleet size is initialized to 0.0f.
 * The planet's claimant is initialized to NULL.
 * @param position The position of the planet.
 * @param maxFleetCapacity The maximum fleet capacity of the planet.
 * @param owner A pointer to the Faction that owns the planet, or NULL if unowned.
 * @return The created Planet object.
 */
Planet CreatePlanet(Vec2 position, float maxFleetCapacity, const Faction *owner);

/**
 * Updates the state of the planet over time.
 * This function adjusts the planet's current fleet size towards its max fleet capacity
 * based on whether it is owned or unowned, and whether it has a claimant.
 * @param planet A pointer to the Planet object to update.
 * @param deltaTime The time elapsed since the last update, in seconds.
 */
void PlanetUpdate(Planet *planet, float deltaTime);

/**
 * Draws the planet using OpenGL.
 * This function renders the planet's ring, inner circle, and claim progress if applicable.
 * We use the planet's owner and claimant to determine colors.
 * Meanwhile, the size of the planet on the screen is based on its fleet capacity.
 * Its position on the screen is determined by its position member.
 * @param planet A pointer to the Planet object to draw.
 */
void PlanetDraw(const Planet *planet);

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
bool PlanetSendFleet(Planet *origin, Planet *destination, struct Level *level);

/**
 * Simulates sending a fleet from the origin planet to the destination planet.
 * This function is similar to PlanetSendFleet but allows specifying the number of starships
 * to send and an optional owner override for the spawned starships.
 * This is useful for simulating fleet launches without altering the origin planet's state.
 * @param origin A pointer to the origin Planet object.
 * @param destination A pointer to the destination Planet object.
 * @param level A pointer to the Level object where starships will be spawned.
 * @param shipCount The number of starships to send.
 * @param ownerOverride An optional pointer to the Faction that will own the spawned starships.
 *                      If NULL, the origin planet's owner will be used.
 * @return true if the fleet was successfully simulated, false otherwise.
 */
bool PlanetSimulateFleetLaunch(Planet *origin, Planet *destination, struct Level *level,
    int shipCount, const Faction *ownerOverride);

/**
 * Handles an incoming starship to the planet.
 * This function processes the interaction between the incoming starship and the planet.
 * Depending on the ownership and claimant status of the planet,
 * it updates the planet's current fleet size, owner, and claimant accordingly.
 * @param planet A pointer to the Planet object receiving the starship.
 * @param ship A pointer to the incoming Starship object.
 */
void PlanetHandleIncomingShip(Planet *planet, const struct Starship *ship);

/**
 * Gets the outer radius of the planet based on its max fleet capacity.
 * The outer radius is calculated as maxFleetCapacity * PLANET_RADIUS_SCALE,
 * with a minimum value of 12.0f to ensure visibility.
 * @param planet A pointer to the Planet object.
 * @return The outer radius of the planet.
 */
float PlanetGetOuterRadius(const Planet *planet);

/**
 * Gets the inner radius of the planet based on its current fleet size.
 * The inner radius is calculated as a proportion of the outer radius,
 * scaled by the ratio of currentFleetSize to maxFleetCapacity.
 * If the planet is over capacity, the inner radius will be equal to the outer radius.
 * @param planet A pointer to the Planet object.
 * @return The inner radius of the planet.
 */
float PlanetGetInnerRadius(const Planet *planet);

/**
 * Gets the collision radius of the planet.
 * The collision radius is defined as the maximum of the outer and inner radii.
 * This radius is used for collision detection with incoming starships.
 * @param planet A pointer to the Planet object.
 * @return The collision radius of the planet.
 */
float PlanetGetCollisionRadius(const Planet *planet);

#endif // _PLANET_H_
