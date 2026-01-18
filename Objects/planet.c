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

// The alpha value used for the owned planet glow effect.
static const float GLOW_ALPHA = 0.25f;

// Alpha value used for tinting the owned planet glow with the faction color.
static const float OWNED_GLOW_ALPHA = 0.5f;

// Multiplier for the radius used when computing the glow effect.
static const float GLOW_RADIUS_MULTIPLIER = 1.5f;

// Multiplier for the radius used when tinting the owned planet glow with the faction color.
static const float OWNED_GLOW_RADIUS_MULTIPLIER = 1.4f;

// The feathering values controls how soft the edges of the planet ring and disc/inner circle are.
// Higher values mean softer edges.
static const float PLANET_RING_FEATHER = 1.5f;
static const float PLANET_DISC_FEATHER = 1.2f;

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
        // This is harsher than the increase rate for two reasons:
        // 1. Gameplay reason: We don't want mega death fleets to be able
        // to exist for long periods of time, at least not ones that can originate
        // from a single planet. This encourages players to spread out
        // their fleet production across multiple planets rather than
        // relying on a single overpowered planet which is only slowly
        // draining its excess ships.
        // 2. Visual reason: When a planet is over capacity, it gets bigger.
        // On certain maps this can cause a planet to visually overlap or
        // even completely cover nearby planets, which looks bad and can
        // make it hard to see what's going on or select those covered planets.

        // We decrease at an exponential (in terms of the difference between
        // current and target) rate to ensure that even massively overcapacity
        // planets return to normal size in a reasonable timeframe.
        
        float excess = planet->currentFleetSize - target;
        float reduction = excess * PLANET_FLEET_REDUCTION_MULTIPLIER * deltaTime;
        planet->currentFleetSize -= reduction;
        if (planet->currentFleetSize < target) {
            planet->currentFleetSize = target;
        }
    } else if (planet->currentFleetSize < target) {
        // Increase fleet size towards target.
        planet->currentFleetSize += PLANET_FLEET_BUILD_RATE * deltaTime;
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
 * with a minimum value of 1.0f to ensure visibility.
 * @param planet A pointer to the Planet object.
 * @return The outer radius of the planet.
 */
float PlanetGetOuterRadius(const Planet *planet) {
    if (planet == NULL) {
        return 0.0f;
    }

    // Calculate the outer radius based on max fleet capacity.
    float base = planet->maxFleetCapacity * PLANET_RADIUS_SCALE;

    // Minimum outer radius is 1.0f for visibility.
    return fmaxf(base, 1.0f);
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
    // It is given by:
    // innerRadius = (outerRadius - PLANET_RING_THICKNESS / 2) * (clampedFleetSize / maxFleetCapacity)
    // We subtract out half the ring thickness to ensure the inner radius
    // does not exceed the outer radius when at max capacity, 
    // and we halve it because we want to make sure there's no annoying visual
    // gap between the inner circle and the ring when at max capacity. 
    float outerRadius = PlanetGetOuterRadius(planet);
    float innerLimit = outerRadius - PLANET_RING_THICKNESS / 2;

    // Planet's inner radius can exceed the outer radius if over capacity,
    // which is a useful visual indicator of overcapacity.

    // Rather than clamping the fleet size with ClampFleetSize,
    // we just make sure it is not negative here.
    float clampedFleetSize = fmaxf(planet->currentFleetSize, 0.0f);
    float ratio = clampedFleetSize / planet->maxFleetCapacity;

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
 * This function draws a circle which is filled inwards up to a certain radius,
 * at which point it feathers out to transparency,
 * based on the planet's current fleet size and max fleet capacity.
 * On the inside, a second pair of faint circles are drawn which glow towards the center of the planet.
 * The size also depends on the claim progress.
 * The circle is drawn in the color of the claimant faction.
 * @param planet A pointer to the Planet object.
 */
static void DrawClaimProgress(const Planet *planet) {
    // Basic validation of parameters.
    if (planet->claimant == NULL) {
        return;
    }

    // The outer radius is solely based on max fleet capacity.
    float outerRadius = PlanetGetOuterRadius(planet);

    // The inner edge, within which the circle feathers out to transparency
    // comes from the ratio of maxFleetCapacity to currentFleetSize.
    // We want to draw a fully filled circle if currentFleetSize == maxFleetCapacity,
    // and a fully transparent circle if currentFleetSize == 0.0f, 
    // with a continuous transition in between.
    // So we compute the radius as follows:
    // innerRadius = innerEdge * (1 - (clampedFleetSize / maxFleetCapacity))
    // Note the 1 - (...) part, which inverts the ratio since we are filling inwards,
    // rather than outwards like when drawing a claimed planet circle.

    // Where innerEdge is (outerRadius - PLANET_RING_THICKNESS), 
    // representing the point where the planet's ring would normally end
    // and transition into the space inside the planet.
    float innerEdge = fmaxf(outerRadius - PLANET_RING_THICKNESS, 0.0f);
    float ratio = 0.0f;
    float clampedFleetSize = ClampFleetSize(planet, planet->currentFleetSize);
    if (planet->maxFleetCapacity > 0.0f) {
        ratio = (float)1 - clampedFleetSize / planet->maxFleetCapacity;
    }

    float innerRadius = innerEdge * ratio;

    // If the inner radius is zero or negative, nothing to draw.
    if (innerRadius <= 0.0f) {
        return;
    }

    // Draw the feathered ring representing claim progress.
    DrawFeatheredRing(planet->position.x, planet->position.y,
        innerRadius, outerRadius, PLANET_RING_FEATHER, planet->claimant->color);

    // We also draw a pair of faint/transparent circle which smoothly transitions to transparency
    // at the center of the planet. It grows larger as the claim progresses.
    // Sort of like the glow highlight effect used for owned planets.
    float highlightInnerColor[4] = {planet->claimant->color[0], planet->claimant->color[1],
        planet->claimant->color[2], OWNED_GLOW_ALPHA};
    float highlightOuterColor[4] = {planet->claimant->color[0], planet->claimant->color[1],
        planet->claimant->color[2], 0.0f};

    float glowHighlightInnerColor[4] = {1.0f, 1.0f, 1.0f, GLOW_ALPHA};
    float glowHighlightOuterColor[4] = {1.0f, 1.0f, 1.0f, 0.0f};

    // Must blending for the highlight effect.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // We must again invert the inner radius ratio
    // since we are filling outwards from the center.
    float innerGlowRadius = innerEdge * (float) (1 - ratio);

    // Draw the faction colored bits of the inner highlight.
    DrawRadialGradientRing(planet->position.x, planet->position.y,
        0.0f, innerGlowRadius * GLOW_RADIUS_MULTIPLIER, 128, highlightInnerColor, highlightOuterColor);

    // We also make it glow a bit.
    DrawRadialGradientRing(planet->position.x, planet->position.y,
        0.0f, innerGlowRadius * OWNED_GLOW_RADIUS_MULTIPLIER, 128, glowHighlightInnerColor, glowHighlightOuterColor);
    // Clean up blending state.
    glDisable(GL_BLEND);
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

    // Determine if the planet is over capacity for drawing purposes
    bool overCapacity = outerRadius <= innerRadius;

    // Draw the ring.
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
    DrawFeatheredRing(planet->position.x, planet->position.y, ringInner, outerRadius, PLANET_RING_FEATHER, ringColor);

    // Draw the inner filled circle representing the current fleet size.
    // If the planet is over capacity, we draw the filled circle using the outer radius.
    if (planet->owner != NULL) {
        // If owned, draw in owner's color.

        // Radius depends on overcapacity status.
        // If over capacity, we use the inner radius. Otherwise, we use the min of innerRadius and outerRadius.
        float radius = overCapacity ? innerRadius : fminf(innerRadius, outerRadius);
        DrawFeatheredFilledInCircle(planet->position.x, planet->position.y, radius, PLANET_DISC_FEATHER, planet->owner->color);

        // We also add a subtle white/glow effect which tapers off steadily towards the exterior of the planet,
        // and we tinge this glow with the faction's color.

        // Enable blending for the glow effect.
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Controls the glow's color at the center of the planet.
        float innerGlowColor[4] = {1.0f, 1.0f, 1.0f, GLOW_ALPHA};

        // Controls the glow's color at the outer edge of the planet.
        float outerGlowColor[4] = {1.0f, 1.0f, 1.0f, 0.0f};

        // Controls the faction color tint applied to the glow.
        float factionGlowColorInner[4] = {planet->owner->color[0], planet->owner->color[1],
            planet->owner->color[2], OWNED_GLOW_ALPHA};

        float factionGlowColorOuter[4] = {planet->owner->color[0], planet->owner->color[1],
            planet->owner->color[2], 0.0f};

        // Draw the faction colored glow.
        DrawRadialGradientRing(planet->position.x, planet->position.y,
            0.0f, radius * OWNED_GLOW_RADIUS_MULTIPLIER, 128, factionGlowColorInner, factionGlowColorOuter);
        
        // Draw the white glow.
        DrawRadialGradientRing(planet->position.x, planet->position.y,
            0.0f, radius * GLOW_RADIUS_MULTIPLIER, 128, innerGlowColor, outerGlowColor);

        // Reset blending after drawing the glow.
        glDisable(GL_BLEND);
    } else if (planet->claimant != NULL) {
        // If unowned but claimed, draw in claimant's color.
        DrawClaimProgress(planet);
    }
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
    // If no ships are available, we cannot send a fleet, so we return false.
    int shipCount = (int)floorf(origin->currentFleetSize);
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
 * Simulates sending a fleet from the origin planet to the destination planet.
 * This function is similar to PlanetSendFleet but allows specifying the number of starships
 * to send and an optional owner override for the spawned starships.
 * This is useful for simulating fleet launches without altering the origin planet's state,
 * as the server and clients no longer need to tell each other about all the starships being sent,
 * and can instead just agree on the fleet launch event, then due to the deterministic nature of the starship spawning,
 * they can each simulate the same starship spawns independently.
 * @param origin A pointer to the origin Planet object.
 * @param destination A pointer to the destination Planet object.
 * @param level A pointer to the Level object where starships will be spawned.
 * @param shipCount The number of starships to send.
 * @param ownerOverride An optional pointer to the Faction that will own the spawned starships.
 *                      If NULL, the origin planet's owner will be used.
 * @return true if the fleet was successfully simulated, false otherwise.
 */
bool PlanetSimulateFleetLaunch(Planet *origin, Planet *destination, Level *level,
    int shipCount, const Faction *ownerOverride) {
    // Basic validation of parameters.
    if (origin == NULL || destination == NULL || level == NULL) {
        return false;
    }

    if (origin == destination || shipCount <= 0) {
        return false;
    }

    // Determine the owner for the spawned starships.
    const Faction *owner = ownerOverride != NULL ? ownerOverride : origin->owner;
    if (owner == NULL) {
        return false;
    }

    // If the origin planet has no owner, we simulate that it becomes owned by the
    // ownerOverride faction upon launching the fleet.
    // This way, if a client has seriously lagged behind and the planet was unowned for them,
    // they can still correctly simulate the fleet launch as intended by the server,
    // and somewhat more accurately reflect the level's state (at least, that's the idea).
    if (origin->owner == NULL) {
        origin->owner = owner;
    }

    // Analogous to PlanetSendFleet, we reduce the origin planet's fleet size to 0.0f
    // since we are sending all available ships.
    origin->currentFleetSize = 0.0f;
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
