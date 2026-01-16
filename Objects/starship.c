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

// Default constants for starship rendering and behavior.

// Controls the visual thickness of the starship trail lines.
static const float STARSHIP_TRAIL_LINE_WIDTH = 1.5f;

// Default/fallback color for starships without an owner.
static const float STARSHIP_DEFAULT_COLOR[4] = {0.7f, 0.7f, 0.7f, 1.0f};

// Glow effect parameters for starships.
// The higher the alpha, the more pronounced the glow.
static const float STARSHIP_GLOW_ALPHA = 0.12f;

// The bigger the radius, the larger the glow effect.
static const float STARSHIP_GLOW_RADIUS = 3.0f;

// forward declarations of static helper functions

static void StarshipTrailAdvanceAges(StarshipTrailSample *samples, size_t *count, float deltaTime);
static void StarshipTrailDrawStrip(const StarshipTrailSample *samples, size_t count, const float baseColor[4]);
static void StarshipDrawTrail(const Starship *ship, const float baseColor[4]);
static void StarshipDrawGlow(const Starship *ship, const float baseColor[4]);

/**
 * Helper for clamping a float value between 0.0f and 1.0f.
 * @param value The float value to clamp.
 * @return The clamped float value.
 */
static float Clamp01(float value) {
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

/**
 * Resolves (that is, determines) the color of the starship based on its owner faction.
 * If the starship has an owner, returns the owner's color.
 * Otherwise, returns a default gray color.
 * @param ship A pointer to the Starship object.
 * @param outColor An array of 4 floats to receive the RGBA color.
 */
void StarshipResolveColor(const Starship *ship, float outColor[4]) {
    // Basic validation of parameters.
    if (outColor == NULL) {
        return;
    }

    // If the ship has an owner, use the owner's color.
    // Otherwise, use the default color.
    if (ship != NULL && ship->owner != NULL) {
        const float *source = ship->owner->color;
        outColor[0] = source[0];
        outColor[1] = source[1];
        outColor[2] = source[2];
        outColor[3] = source[3];
    } else {
        outColor[0] = STARSHIP_DEFAULT_COLOR[0];
        outColor[1] = STARSHIP_DEFAULT_COLOR[1];
        outColor[2] = STARSHIP_DEFAULT_COLOR[2];
        outColor[3] = STARSHIP_DEFAULT_COLOR[3];
    }
}

/**
 * Initializes a starship trail effect based on the given starship's trail data.
 * Copies the trail samples from the starship into the effect,
 * and sets the effect's color to the specified color.
 * @param effect A pointer to the StarshipTrailEffect to initialize.
 * @param ship A pointer to the Starship whose trail data to use.
 * @param color An array of 4 floats representing the RGBA color for the trail effect.
 */
void StarshipTrailEffectInit(StarshipTrailEffect *effect, const Starship *ship, const float color[4]) {
    // Basic validation of parameters.
    if (effect == NULL) {
        return;
    }

    // If the ship is valid, copy its trail samples into the effect.
    // A trail sample is simply a position and age pair.
    effect->sampleCount = 0;
    if (ship != NULL && ship->trailCount > 0) {
        size_t copyCount = ship->trailCount;
        if (copyCount > STARSHIP_TRAIL_MAX_SAMPLES) {
            copyCount = STARSHIP_TRAIL_MAX_SAMPLES;
        }

        // Copy the samples from the ship's trail to the effect's samples.
        memcpy(effect->samples, ship->trail, copyCount * sizeof(StarshipTrailSample));
        effect->sampleCount = copyCount;
    }

    // Set the effect's color.
    const float *sourceColor = color != NULL ? color : STARSHIP_DEFAULT_COLOR;
    effect->color[0] = sourceColor[0];
    effect->color[1] = sourceColor[1];
    effect->color[2] = sourceColor[2];
    effect->color[3] = sourceColor[3];
}

/**
 * Updates the starship trail effect over time.
 * Advances the ages of the trail samples by deltaTime.
 * @param effect A pointer to the StarshipTrailEffect to update.
 * @param deltaTime The time elapsed since the last update, in seconds.
 */
void StarshipTrailEffectUpdate(StarshipTrailEffect *effect, float deltaTime) {
    // Basic validation of parameters.
    if (effect == NULL) {
        return;
    }

    // Advance the ages of the trail samples.
    StarshipTrailAdvanceAges(effect->samples, &effect->sampleCount, deltaTime);
}

/**
 * Determines if the starship trail effect is still alive.
 * A trail effect is considered alive if it has more than one sample.
 * @param effect A pointer to the StarshipTrailEffect to check.
 * @return true if the effect is alive, false otherwise.
 */
bool StarshipTrailEffectIsAlive(const StarshipTrailEffect *effect) {
    // NULL is considered not alive.
    if (effect == NULL) {
        return false;
    }

    // Otherwise anything with at least 2 samples is alive.
    return effect->sampleCount > 1;
}

/**
 * Draws the starship trail effect.
 * Renders the trail samples as a strip with fading colors.
 * @param effect A pointer to the StarshipTrailEffect to draw.
 */
void StarshipTrailEffectDraw(const StarshipTrailEffect *effect) {
    // Make sure the effect is valid.
    if (effect == NULL) {
        return;
    }

    // Now we delegate to the trail drawing helper.
    StarshipTrailDrawStrip(effect->samples, effect->sampleCount, effect->color);
}

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
    // that it does not require any dynamic memory allocation.
    ship.position = position;
    ship.velocity = Vec2ClampToLength(velocity, STARSHIP_MAX_SPEED);
    ship.owner = owner;
    ship.target = target;
    ship.trailCount = 0;
    ship.trailTimeSinceLastEmit = 0.0f;
    for (size_t i = 0; i < STARSHIP_TRAIL_MAX_SAMPLES; ++i) {
        ship.trail[i].position = position;
        ship.trail[i].age = 0.0f;
    }
    return ship;
}

/**
 * Updates the state of the starship over time.
 * This function accelerates the starship towards its target planet
 * and updates its position based on its velocity.
 * If the starship has a trail, it also updates the trail samples.
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

    // If the starship has a trail, we update the trail samples.
    ship->trailTimeSinceLastEmit += deltaTime;

    // We first age all existing trail samples.
    StarshipTrailAdvanceAges(ship->trail, &ship->trailCount, deltaTime);

    // Then we consider emitting a new trail sample (position + age).
    bool emitSample = false;

    // We emit a new sample if:
    // 1. There are no existing samples.
    // 2. The starship has moved far enough from the last sample.
    // 3. Enough time has passed since the last sample was emitted.
    // Otherwise, we do not emit a new sample.
    if (ship->trailCount == 0) {
        // Since there are no samples, we must emit one.
        emitSample = true;
    } else {
        // There are existing samples, so we check distance and time.
        Vec2 toLast = Vec2Subtract(ship->position, ship->trail[0].position);
        float distance = Vec2Length(toLast);

        // We emit a new sample if either the distance or time criteria is met.
        if (distance >= STARSHIP_TRAIL_MIN_DISTANCE || ship->trailTimeSinceLastEmit >= STARSHIP_TRAIL_EMIT_INTERVAL) {
            emitSample = true;
        }
    }

    // If we decided to emit a new sample, we shift existing samples down
    // and insert the new sample at the front.
    if (emitSample) {
        // First we determine how many samples to copy.
        // If we are at max capacity, we leave off the last sample.
        // This ensures we do not exceed STARSHIP_TRAIL_MAX_SAMPLES.
        // The memmove below will take this last sample and overwrite it
        // with the second to last sample, effectively dropping it.
        size_t copyCount = ship->trailCount;
        if (copyCount >= STARSHIP_TRAIL_MAX_SAMPLES) {
            copyCount = STARSHIP_TRAIL_MAX_SAMPLES - 1;
        }

        // Then we shift the existing samples down by one.
        // We use memmove to handle overlapping memory regions safely.
        if (copyCount > 0) {
            memmove(&ship->trail[1], &ship->trail[0], copyCount * sizeof(StarshipTrailSample));
        }

        // Finally, we insert the new sample at the front.
        ship->trail[0].position = ship->position;
        ship->trail[0].age = 0.0f;
        ship->trailCount = copyCount + 1;
        ship->trailTimeSinceLastEmit = 0.0f;
    }
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
 * Furthermore, the starship's glow and trail effects are also drawn
 * with their respective helper functions, called here.
 * @param ship A pointer to the Starship object to draw.
 */
void StarshipDraw(const Starship *ship) {
    if (ship == NULL) {
        return;
    }

    // Default/fallback color for the starship.
    float color[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    // Resolve the starship's color based on its owner.
    StarshipResolveColor(ship, color);

    // Draw the starship's glow and trail effects.
    StarshipDrawGlow(ship, color);
    StarshipDrawTrail(ship, color);

    // Set the OpenGL color to the starship's color.
    glColor4fv(color);

    // Draw the starship as a filled circle at its position.
    DrawFilledCircle(ship->position.x, ship->position.y, STARSHIP_RADIUS, 20);
}

/**
 * Helper function to draw the starship's trail.
 * @param ship A pointer to the Starship whose trail to draw.
 * @param baseColor An array of 4 floats representing the RGBA color for the trail.
 */
static void StarshipDrawTrail(const Starship *ship, const float baseColor[4]) {
    StarshipTrailDrawStrip(ship->trail, ship->trailCount, baseColor);
}

/**
 * Helper function to advance the ages of starship trail samples.
 * Also removes any samples that have exceeded the maximum trail length.
 * @param samples An array of StarshipTrailSample objects.
 * @param count A pointer to the number of samples in the array.
 *              This value will be updated if samples are removed.
 * @param deltaTime The time elapsed since the last update, in seconds.
 */
static void StarshipTrailAdvanceAges(StarshipTrailSample *samples, size_t *count, float deltaTime) {
    // Basic validation of parameters.
    if (samples == NULL || count == NULL) {
        return;
    }

    // First we advance the age of all samples
    // by adding the deltaTime to each sample's age.
    size_t currentCount = *count;
    for (size_t i = 0; i < currentCount; ++i) {
        samples[i].age += deltaTime;
    }

    // Then we remove any samples that have exceeded the maximum trail length.
    // We do this by simply reducing the count until we find a sample
    // that is still within the valid age range, as we assume samples are stored
    // in sorted order from newest to oldest.
    while (currentCount > 0 && samples[currentCount - 1].age > STARSHIP_TRAIL_LENGTH_SECONDS) {
        currentCount--;
    }

    // Finally, we update the count to reflect any removed samples.
    *count = currentCount;
}

/**
 * Helper function to draw a starship trail as a line strip.
 * The trail fades out over its length based on the age of each sample.
 * @param samples An array of StarshipTrailSample objects.
 * @param count The number of samples in the array.
 * @param baseColor An array of 4 floats representing the RGBA color for the trail.
 */
static void StarshipTrailDrawStrip(const StarshipTrailSample *samples, size_t count, const float baseColor[4]) {
    // Basic validation of parameters.
    // We need at least 2 samples to draw a trail between them.
    if (samples == NULL || baseColor == NULL || count < 2) {
        return;
    }

    // Extract out the base color components for easier access.
    float r = baseColor[0];
    float g = baseColor[1];
    float b = baseColor[2];
    float a = baseColor[3];

    // Tell OpenGL that we want to draw lines with a specific width.
    glLineWidth(STARSHIP_TRAIL_LINE_WIDTH);

    // What glEnable(GL_BLEND) does is enable blending in OpenGL.
    // Blending is a technique used to combine the color of a source pixel (the pixel being drawn)
    // with the color of a destination pixel (the pixel already present in the framebuffer)
    glEnable(GL_BLEND);

    // glBlendFunc specifies how the blending is done.
    // The parameters GL_SRC_ALPHA and GL_ONE_MINUS_SRC_ALPHA mean that
    // the source color is multiplied by its alpha value,
    // and the destination color is multiplied by (1 - source alpha).
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Now we draw the trail as a line strip.
    // Each sample's color alpha is modulated based on its age,
    // so older samples are more transparent.
    // This gives rise to a sort of fade effect along the trail,
    // with older portions of the trail fading out into the background.
    glBegin(GL_LINE_STRIP);
    for (size_t index = count; index-- > 0;) {
        // Fetch the sample.
        const StarshipTrailSample *sample = &samples[index];

        // Compute the life factor (0.0 to 1.0) based on age.
        // Newer samples have life closer to 1.0, older samples closer to 0.0.
        float life = Clamp01(1.0f - (sample->age / STARSHIP_TRAIL_LENGTH_SECONDS));

        // Set the color with modulated alpha.
        glColor4f(r, g, b, a * life);

        // Specify the vertex position.
        glVertex2f(sample->position.x, sample->position.y);
    }

    // End the line strip.
    glEnd();

    // Reset the blending function back to standard alpha blending.
    glDisable(GL_BLEND);

    // Reset the line width to default (1.0f).
    glLineWidth(1.0f);
}

/**
 * Helper function to draw the starship's glow effect.
 * @param ship A pointer to the Starship whose glow to draw.
 * @param baseColor An array of 4 floats representing the RGBA color for the glow.
 */
static void StarshipDrawGlow(const Starship *ship, const float baseColor[4]) {
    // Basic validation of parameters.
    if (ship == NULL || baseColor == NULL) {
        return;
    }

    // Set up the inner and outer colors for the glow effect.
    float innerColor[4] = {baseColor[0], baseColor[1], baseColor[2], STARSHIP_GLOW_ALPHA};
    float outerColor[4] = {baseColor[0], baseColor[1], baseColor[2], 0.0f};
    float glowOuterRadius = STARSHIP_RADIUS + STARSHIP_GLOW_RADIUS;

    // Draw the radial gradient ring for the glow effect.
    // We enable blending and set the blend function to additive blending
    // to create a glowing effect where colors add light to the background.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    DrawRadialGradientRing(ship->position.x, ship->position.y,
        0.0f, glowOuterRadius, 32, innerColor, outerColor);

    // And then we reset the blending function back to standard alpha blending. 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
}
