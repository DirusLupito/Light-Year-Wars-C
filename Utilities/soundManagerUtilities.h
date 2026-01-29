/**
 * Header for sound manager utilities.
 * Provides a centralized set of code tools for triggering UI/gameplay sound cues
 * so the implementation can be swapped without touching gameplay code.
 * @file Utilities/soundManagerUtilities.h
 * @author abmize
 */
#ifndef _SOUND_MANAGER_UTILITIES_H_
#define _SOUND_MANAGER_UTILITIES_H_

#include <stdbool.h>

/**
 * Initializes the sound manager and enables playback.
 * This sets up any OS resources required by the current sound backend.
 */
void SoundManagerInitialize(void);

/**
 * Shuts down the sound manager and disables playback.
 * This is kept explicit so future audio backends can release resources cleanly.
 */
void SoundManagerShutdown(void);

/**
 * Plays a soft cue when a starship collides with a planet.
 * The manager throttles this sound to avoid overwhelming the audio mix.
 */
void SoundManagerPlayShipImpact(void);

/**
 * Plays a gentle cue when a planet changes ownership.
 * This is used for both direct captures and claimant-to-owner transitions.
 */
void SoundManagerPlayPlanetCaptured(void);

/**
 * Sets the master volume for generated tones.
 * The value is clamped to the range [0.0, 1.0] where 0 is silent.
 * @param volume Normalized volume scalar.
 */
void SoundManagerSetVolume(float volume);

/**
 * Configures a lightweight reverb (echo) applied to generated tones.
 * When disabled, the reverb path is bypassed to keep CPU usage minimal.
 * @param enabled True to enable reverb, false to disable.
 * @param delayMs Delay in milliseconds before the echo is mixed in.
 * @param decay Linear decay factor in the range [0.0, 1.0].
 */
void SoundManagerSetReverb(bool enabled, float delayMs, float decay);

#endif // _SOUND_MANAGER_UTILITIES_H_
