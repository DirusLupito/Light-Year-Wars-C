/**
 * Implementation of sound manager utilities.
 * Uses lightweight Windows-generated tones so no asset files are required.
 * Keeping all sound logic centralized allows for easy swapping of backends later.
 * @file Utilities/soundManagerUtilities.c
 * @author abmize
 */

#include "Utilities/soundManagerUtilities.h"

#include <windows.h>
#include <mmsystem.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// --- Internal data types ---

// Describes a single tone step within a sound cue.
// Frequency controls how high or low the pitch is (0 = silence).
// DurationMs controls how long the tone plays.
// PauseMs controls the silence after the tone before the next step.
typedef struct SoundToneStep {
    DWORD frequency;
    DWORD durationMs;
    DWORD pauseMs;
} SoundToneStep;

// Describes a sequence of tone steps and its cooldown state.
// steps points to an array of SoundToneStep.
// stepCount is the number of steps in the array.
// cooldownMs is the minimum time between plays of this sequence.
// lastPlayedMs tracks when the sequence was last played.
typedef struct SoundToneSequence {
    const SoundToneStep *steps;
    size_t stepCount;
    DWORD cooldownMs;
    DWORD lastPlayedMs;
} SoundToneSequence;

// Describes a playback request with optional ownership of step data.
// steps points to an array of SoundToneStep.
// stepCount is the number of steps in the array.
// ownsSteps indicates if this struct owns the steps array and should free it.
typedef struct SoundTonePlayback {
    SoundToneStep *steps;
    size_t stepCount;
    bool ownsSteps;
} SoundTonePlayback;

// --- Static state ---

// Tracks whether the sound manager is enabled.
// Used to toggle sound playback globally.
static bool soundEnabled = false;

// Protects cooldown timestamps so multiple threads do not race.
static CRITICAL_SECTION soundLock;

// Tracks whether the sound lock has been initialized.
static bool soundLockInitialized = false;

// Master volume applied to all synthesized tones.
static float soundMasterVolume = 0.2f;

// Reverb configuration used when mixing echo into generated audio.
static bool reverbEnabled = false;
static float reverbDelayMs = 240.0f;
static float reverbDecay = 0.15f;

// RNG state used to vary musical choices while keeping them deterministic.
static uint32_t soundRngState = 0u;
static bool soundRngSeeded = false;

// Audio format constants for synthesized output.
// The PCM format explained: Basically, if sound is some sort of combination
// of sine waves x(t) which is continuous for real values of t,
// then PCM approximates that continuous signal by sampling it
// so that we instead have x[n] for integer values of n only.
// The sample rate controls how many samples per second are taken.
// In this case, for 1 second of audio, we take 44100 samples.
// Each sample is represented as a 16-bit signed integer,
// meaning values range from -32768 to +32767.
// These values represent the amplitude of the sound wave at that sample point in time.
// When played back at the correct sample rate, the ear perceives the intended sound.

static const DWORD SOUND_SAMPLE_RATE = 44100u;
static const float SOUND_PI = 3.14159265358979323846f;

// The base amplitude controls the overall loudness of generated tones.
static const float SOUND_BASE_AMPLITUDE = 0.35f;

// Reverb is implemented as output[n] = input[n] + decay * input[n - delaySamples]
// In doing so, we essentially can think of reverb as a simple echo effect.
// Given some original sound wave, we mix in a delayed and decayed version of that sound wave
// to create the perception of space and depth.

// The headroom factor controls how much volume we reserve
// to avoid clipping when mixing in the reverb.
static const float SOUND_REVERB_HEADROOM = 0.6f;

// An attack "envelope" is a short fade-in at the start of a tone,
// while a release envelope is a short fade-out at the end of a tone.
// Basically, the numbers below (in units of milliseconds) control
// how quickly the tone ramps up from silence to full volume (attack)
// and how quickly it ramps back down to silence (release).

static const DWORD SOUND_ATTACK_MS = 60u;
static const DWORD SOUND_RELEASE_MS = 50u;

// A fade-out at the end of the entire sound sequence
// helps avoid abrupt cutoffs that can cause clicks.
static const DWORD SOUND_TAIL_FADE_MS = 80u;

// Chromatic scale (C) offers maximum variety while we keep step sizes small for softness.
// These sounds are used to generate the random ship sound effects.
// Modern Western tuning usually uses 12-tone equal temperament (12-TET).
// In 12-TET, each semitone (where in terms of the fundamentals a semitone is the interval between two adjacent frequency keys on a piano keyboard)
// is a constant multiplicative ratio: 2^(1/12).
// According to Wikipediam "In modern times, 12-ET is usually tuned relative to a standard pitch of 440 Hz, 
// called A440, meaning one note, A4 (the A in the 4th octave of a typical 88-key piano), 
// is tuned to 440 hertz and all other notes are defined as some multiple of semitones apart from it, 
// either higher or lower in frequency. 
//
// So here, we are defining the frequencies of the chromatic scale starting from C5 (523.25 Hz) up to B5 (987.77 Hz).
// A handy chart mapping note names to frequencies can be found here:
// https://mixbutton.com/music-tools/frequency-and-pitch/music-note-to-frequency-chart

static const DWORD SOUND_CHROMATIC_FREQS[] = {
    523u, 554u, 587u, 622u, 659u, 698u, 740u, 784u, 831u, 880u, 932u, 988u
};
static const size_t SOUND_CHROMATIC_COUNT = sizeof(SOUND_CHROMATIC_FREQS) / sizeof(SOUND_CHROMATIC_FREQS[0]);

// Here, we have a subset of the chromatic scale representing whole tones only (where a whole tone is an interval of two semitones).

// static const DWORD SOUND_WHOLE_TONE_FREQS[] = { 523u, 587u, 659u, 740u, 831u, 932u };
// static const size_t SOUND_WHOLE_TONE_COUNT = sizeof(SOUND_WHOLE_TONE_FREQS) / sizeof(SOUND_WHOLE_TONE_FREQS[0]);

// And here is a pentatonic scale (C major pentatonic).

// static const DWORD SOUND_PENTATONIC_FREQS[] = { 523u, 587u, 659u, 784u, 880u, 1046u };
// static const size_t SOUND_PENTATONIC_COUNT = sizeof(SOUND_PENTATONIC_FREQS) / sizeof(SOUND_PENTATONIC_FREQS[0]);

/**
 * Ensures the sound lock exists before use.
 * This keeps helper calls safe even if they happen before initialization.
 */
static void SoundManagerEnsureLock(void) {
    if (!soundLockInitialized) {
        InitializeCriticalSection(&soundLock);
        soundLockInitialized = true;
    }
}

/**
 * Ensures the RNG has a non-zero seed so randomized tones vary per session.
 * We seed lazily to avoid extra OS calls when audio is never used. 
 */
static void SoundManagerEnsureRngSeed(void) {
    if (!soundRngSeeded) {
        // Tick-based seeding is adequate for audio variation without extra dependencies.
        soundRngState = (uint32_t)GetTickCount();
        if (soundRngState == 0u) {
            // We avoid a zero seed so the LCG does not degenerate.
            soundRngState = 0xA5A5A5A5u;
        }
        soundRngSeeded = true;
    }
}

/**
 * Advances the RNG state and returns the next value.
 * Implements the following recurrence relation:
 * X_{n+1} = (a * X_n + c) mod m
 * where:
 * a = 1664525
 * c = 1013904223
 * m = 2^32
 * Since we are using unsigned int, the modulus operation is implicit due to overflow.
 * The constants a and c were found on the Internet and Wikipedia's page on LCGs
 * says they come from ranqd1.
 * @return The next pseudo-random value.
 */
static uint32_t SoundManagerNextRandom(void) {
    SoundManagerEnsureRngSeed();

    // LCG parameters balance period with fast computation.
    soundRngState = (soundRngState * 1664525u) + 1013904223u;
    return soundRngState;
}

/**
 * Returns a random integer in the inclusive range [minValue, maxValue].
 * @param minValue Minimum value to return.
 * @param maxValue Maximum value to return.
 * @return The bounded random value.
 */
static int SoundManagerRandomRange(int minValue, int maxValue) {
    if (maxValue <= minValue) {
        return minValue;
    }

    uint32_t value = SoundManagerNextRandom();
    uint32_t range = (uint32_t)(maxValue - minValue + 1);
    return minValue + (int)(value % range);
}

// --- Tone data ---

// Ship impact/attack cue.
static const SoundToneStep kShipImpactSteps[] = {
    { 523u, 35u, 8u },  // C5
    { 659u, 45u, 0u }   // E5
};

// Ownership change cue.
static const SoundToneStep kPlanetCapturedSteps[] = {
    { 392u, 60u, 10u }, // G4
    { 494u, 60u, 10u }, // B4
    { 587u, 80u, 0u }   // D5
};

static SoundToneSequence shipImpactSequence = {
    kShipImpactSteps,
    sizeof(kShipImpactSteps) / sizeof(kShipImpactSteps[0]),
    90u,
    0u
};

static SoundToneSequence planetCapturedSequence = {
    kPlanetCapturedSteps,
    sizeof(kPlanetCapturedSteps) / sizeof(kPlanetCapturedSteps[0]),
    250u,
    0u
};

// --- Internal helpers ---

/**
 * Plays a tone sequence on a background thread so gameplay remains responsive.
 * @param param Pointer to a SoundToneSequence describing the tone steps.
 * @return Thread exit code (unused).
 */
static DWORD WINAPI SoundManagerPlaySequenceThread(LPVOID param) {
    SoundTonePlayback *playback = (SoundTonePlayback *)param;
    if (playback == NULL || playback->steps == NULL || playback->stepCount == 0u) {
        return 0u;
    }

    // We re-check the global flag so shutdown can silence any in-flight threads.
    if (!soundEnabled) {
        if (playback->ownsSteps) {
            free(playback->steps);
        }
        free(playback);
        return 0u;
    }

    // Snapshot tunables so a mid-playback change cannot desync the buffer math.
    // We don't want to have a lock on the entire playback duration
    // since that would block other sound requests.
    // So we just grab the values we need up front, then release the lock.
    SoundManagerEnsureLock();
    EnterCriticalSection(&soundLock);
    float volume = soundMasterVolume;
    bool localReverbEnabled = reverbEnabled;
    float localReverbDelay = reverbDelayMs;
    float localReverbDecay = reverbDecay;
    LeaveCriticalSection(&soundLock);

    // Clamp volume defensively so the mixer never produces clipping.
    if (volume < 0.0f) {
        volume = 0.0f;
    } else if (volume > 1.0f) {
        volume = 1.0f;
    }

    // Determine total sample count so we can render into one contiguous buffer.
    size_t totalSamples = 0;
    for (size_t i = 0; i < playback->stepCount; ++i) {
        const SoundToneStep *step = &playback->steps[i];
        DWORD stepMs = step->durationMs + step->pauseMs;
        totalSamples += (size_t)((SOUND_SAMPLE_RATE * (uint64_t)stepMs) / 1000u);
    }

    if (totalSamples == 0) {
        return 0u;
    }

    // Allocate a mono 16-bit PCM buffer for the sequence.
    int16_t *samples = (int16_t *)malloc(totalSamples * sizeof(int16_t));
    if (samples == NULL) {
        return 0u;
    }

    // Zero the buffer so silence is the default when no tone is active.
    memset(samples, 0, totalSamples * sizeof(int16_t));

    // Render each tone directly into the buffer.
    size_t cursor = 0;
    for (size_t i = 0; i < playback->stepCount; ++i) {
        const SoundToneStep *step = &playback->steps[i];
        DWORD toneSamples = (DWORD)((SOUND_SAMPLE_RATE * (uint64_t)step->durationMs) / 1000u);
        DWORD pauseSamples = (DWORD)((SOUND_SAMPLE_RATE * (uint64_t)step->pauseMs) / 1000u);

        if (step->frequency > 0u && toneSamples > 0u) {
            float phaseIncrement = 2.0f * SOUND_PI * ((float)step->frequency / (float)SOUND_SAMPLE_RATE);
            float phase = 0.0f;

            // The base amplitude keeps tones musical instead of harsh.
            float amplitude = SOUND_BASE_AMPLITUDE * volume;
            if (localReverbEnabled) {
                // Reserve headroom so the reverb mix does not clip and crackle.
                float headroom = 1.0f - (localReverbDecay * SOUND_REVERB_HEADROOM);
                if (headroom < 0.25f) {
                    headroom = 0.25f;
                }
                amplitude *= headroom;
            }

            // Attack/release smoothing avoids discontinuities that cause pops.
            DWORD attackSamples = (DWORD)((SOUND_SAMPLE_RATE * (uint64_t)SOUND_ATTACK_MS) / 1000u);
            DWORD releaseSamples = (DWORD)((SOUND_SAMPLE_RATE * (uint64_t)SOUND_RELEASE_MS) / 1000u);
            if (attackSamples * 2 > toneSamples) {
                attackSamples = toneSamples / 2u;
            }
            if (releaseSamples * 2 > toneSamples) {
                releaseSamples = toneSamples / 2u;
            }

            for (DWORD s = 0; s < toneSamples && cursor + s < totalSamples; ++s) {
                float envelope = 1.0f;
                if (attackSamples > 0u && s < attackSamples) {
                    // We ramp in to avoid an abrupt jump from silence.
                    envelope = (float)s / (float)attackSamples;
                } else if (releaseSamples > 0u && s + releaseSamples >= toneSamples) {
                    // We ramp out to avoid cutting mid-wave.
                    envelope = (float)(toneSamples - s) / (float)releaseSamples;
                }

                // Calculate the sample value and write it to the buffer.
                // We clamp to the range of int16_t [-2^15, 2^15-1] to avoid overflow.

                float sample = sinf(phase) * amplitude * envelope;
                int32_t scaled = (int32_t)(sample * 32767.0f);
                if (scaled > 32767) {
                    scaled = 32767;
                } else if (scaled < -32768) {
                    scaled = -32768;
                }
                samples[cursor + s] = (int16_t)scaled;
                phase += phaseIncrement;
                if (phase > 2.0f * SOUND_PI) {
                    phase -= 2.0f * SOUND_PI;
                }
            }
        }

        // Advance by tone + pause to leave natural spacing between steps.
        cursor += (size_t)toneSamples + (size_t)pauseSamples;
        if (cursor >= totalSamples) {
            break;
        }
    }

    // Apply a lightweight reverb by mixing a delayed copy of the buffer.
    if (localReverbEnabled && localReverbDecay > 0.0f) {
        if (localReverbDelay < 1.0f) {
            localReverbDelay = 1.0f;
        }
        if (localReverbDecay > 1.0f) {
            localReverbDecay = 1.0f;
        }

        size_t delaySamples = (size_t)((SOUND_SAMPLE_RATE * (uint64_t)localReverbDelay) / 1000u);
        if (delaySamples > 0 && delaySamples < totalSamples) {
            for (size_t i = delaySamples; i < totalSamples; ++i) {
                int32_t mixed = samples[i] + (int32_t)(samples[i - delaySamples] * localReverbDecay);
                if (mixed > 32767) {
                    mixed = 32767;
                } else if (mixed < -32768) {
                    mixed = -32768;
                }
                samples[i] = (int16_t)mixed;
            }
        }
    }

    // Fade the tail to silence so the buffer never cuts off mid-wave.
    DWORD tailFadeSamples = (DWORD)((SOUND_SAMPLE_RATE * (uint64_t)SOUND_TAIL_FADE_MS) / 1000u);
    if (tailFadeSamples > 0u && tailFadeSamples < totalSamples) {
        size_t fadeStart = totalSamples - tailFadeSamples;
        for (size_t i = fadeStart; i < totalSamples; ++i) {
            float t = (float)(totalSamples - i) / (float)tailFadeSamples;
            int32_t scaled = (int32_t)((float)samples[i] * t);
            if (scaled > 32767) {
                scaled = 32767;
            } else if (scaled < -32768) {
                scaled = -32768;
            }
            samples[i] = (int16_t)scaled;
        }
    }

    // Prepare and submit the buffer to the Windows wave output device.
    // A WAVEFORMATEX structure describes the audio format.
    // https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/ns-mmeapi-waveformatex
    WAVEFORMATEX format = {0};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 1;
    format.nSamplesPerSec = SOUND_SAMPLE_RATE;
    format.wBitsPerSample = 16;
    format.nBlockAlign = (format.nChannels * format.wBitsPerSample) / 8;
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

    // An HWAVEOUT handle represents the output device.
    HWAVEOUT waveOutHandle = NULL;

    // An MMRESULT indicates success or failure of multimedia functions.
    // waveOutOpen opens the default waveform output device.
    // Documentation for waveOutOpen: https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/nf-mmeapi-waveoutopen
    // It has the following arguments:
    // 1. Pointer to an HWAVEOUT handle that receives the device handle.
    // 2. Device identifier (WAVE_MAPPER selects the default device).
    // 3. Pointer to a WAVEFORMATEX structure that describes the desired format.
    // 4. Callback (not used here, so we pass 0 and CALLBACK_NULL).
    // 5. Instance data (not used here, so we pass 0).
    // 6. Flags (CALLBACK_NULL indicates no callback mechanism).

    MMRESULT openResult = waveOutOpen(&waveOutHandle, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL);
    if (openResult != MMSYSERR_NOERROR || waveOutHandle == NULL) {
        free(samples);
        return 0u;
    }

    // A WAVEHDR structure describes the audio buffer to play.
    // https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/ns-mmeapi-wavehdr
    WAVEHDR header = {0};
    header.lpData = (LPSTR)samples;
    header.dwBufferLength = (DWORD)(totalSamples * sizeof(int16_t));

    // waveOutPrepareHeader prepares the buffer for playback.
    // https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/nf-mmeapi-waveoutprepareheader
    // It returns MMSYSERR_NOERROR on success.
    // It takes the following arguments:
    // 1. The HWAVEOUT handle representing the output device.
    // 2. Pointer to the WAVEHDR structure describing the buffer.
    // 3. Size of the WAVEHDR structure.
    if (waveOutPrepareHeader(waveOutHandle, &header, sizeof(header)) == MMSYSERR_NOERROR) {
        // https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/nf-mmeapi-waveoutwrite
        // waveOutWrite submits the buffer for playback.
        // It returns MMSYSERR_NOERROR on success.
        // It takes the following arguments:
        // 1. The HWAVEOUT handle representing the output device.
        // 2. Pointer to the WAVEHDR structure describing the buffer.
        // 3. Size of the WAVEHDR structure.

        // In terms of actually playing the sound, that will happen asynchronously,
        // although it is triggered by this call.
        if (waveOutWrite(waveOutHandle, &header, sizeof(header)) == MMSYSERR_NOERROR) {
            // Wait for playback to finish so we can safely release the buffer.
            while (soundEnabled && (header.dwFlags & WHDR_DONE) == 0u) {
                Sleep(1u);
            }
        }

        // https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/nf-mmeapi-waveoutunprepareheader
        // waveOutUnprepareHeader cleans up the buffer after playback.
        // It's important to call this since it releases resources allocated during preparation.
        // You can't just call free on the buffer directly after playback because the audio system
        // may still be using it until unpreparation is done.
        // It takes the following arguments:
        // 1. The HWAVEOUT handle representing the output device.
        // 2. Pointer to the WAVEHDR structure describing the buffer.
        // 3. Size of the WAVEHDR structure.
        waveOutUnprepareHeader(waveOutHandle, &header, sizeof(header));
    }

    // https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/nf-mmeapi-waveoutclose
    // waveOutClose closes the waveform output device.
    // If the output device is not closed (for instance we return out of this function sloppily,
    // the audio system may leak resources or behave unpredictably, perhaps not letting 
    // a future call to waveOutOpen succeed.
    // It takes the following argument:
    // 1. The HWAVEOUT handle representing the output device.

    waveOutClose(waveOutHandle);
    free(samples);

    // Release any owned step data after playback finishes.
    if (playback->ownsSteps) {
        free(playback->steps);
    }
    free(playback);

    return 0u;
}

/**
 * Starts playback for a set of steps and handles thread ownership.
 * @param steps Pointer to the tone step array.
 * @param stepCount Number of tone steps.
 * @param ownsSteps True if the array should be freed after playback.
 */
static void SoundManagerStartPlayback(SoundToneStep *steps, size_t stepCount, bool ownsSteps) {
    SoundTonePlayback *playback = (SoundTonePlayback *)malloc(sizeof(SoundTonePlayback));
    if (playback == NULL) {
        if (ownsSteps) {
            free(steps);
        }
        return;
    }

    playback->steps = steps;
    playback->stepCount = stepCount;
    playback->ownsSteps = ownsSteps;

    // Use a detached thread so the main loop never blocks on audio timing.
    // https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createthread
    // Compared to Posix threads, Windows threads are a bit different.
    // The equivalent pthread call here would be something like:
    // pthread_t thread;
    // pthread_create(&thread, NULL, SoundManagerPlaySequenceThread, playback);
    // pthread_detach(thread);
    // But Windows uses HANDLEs to represent threads,
    // and we can simply close the handle immediately after creation to detach it.
    // This allows the thread to run independently without requiring a join later.
    // Moreover, it takes more arguments than pthread_create:
    // Specifically the arguments are:
    // 1. Security attributes (NULL means default).
    // 2. Stack size (0 means default).
    // 3. Pointer to the thread function.
    // 4. Pointer to the parameter to pass to the thread function.
    // 5. Creation flags (0 means the thread runs immediately).
    // 6. Pointer to receive the thread identifier (NULL means we don't care).

    HANDLE threadHandle = CreateThread(NULL, 0u, SoundManagerPlaySequenceThread, playback, 0u, NULL);
    if (threadHandle != NULL) {
        CloseHandle(threadHandle);
    } else {
        // If we cannot spawn the thread, we clean up immediately to avoid leaks.
        if (ownsSteps) {
            free(steps);
        }
        free(playback);
    }
}

/**
 * Attempts to play a sequence while honoring its cooldown.
 * @param sequence Pointer to the sequence to play.
 */
static void SoundManagerTryPlaySequence(SoundToneSequence *sequence) {
    if (!soundEnabled || sequence == NULL) {
        return;
    }

    DWORD now = GetTickCount();
    bool shouldPlay = false;

    SoundManagerEnsureLock();
    EnterCriticalSection(&soundLock);
    // Cooldown exists so rapid collisions do not overwhelm the player with sound.
    if (sequence->lastPlayedMs == 0u || (now - sequence->lastPlayedMs) >= sequence->cooldownMs) {
        sequence->lastPlayedMs = now;
        shouldPlay = true;
    }
    LeaveCriticalSection(&soundLock);

    if (!shouldPlay) {
        return;
    }

    // We pass static steps without ownership because the sequence owns them.
    SoundManagerStartPlayback((SoundToneStep *)sequence->steps, sequence->stepCount, false);
}

/**
 * Initializes the sound manager and enables playback.
 * This also prepares a synchronization primitive used for throttling.
 */
void SoundManagerInitialize(void) {
    SoundManagerEnsureLock();

    // We only need to flip the flag because the lock is already ensured.
    soundEnabled = true;
}

/**
 * Shuts down the sound manager and disables playback.
 * Keeping this explicit simplifies swapping to a different backend later.
 */
void SoundManagerShutdown(void) {
    soundEnabled = false;

    if (soundLockInitialized) {
        DeleteCriticalSection(&soundLock);
        soundLockInitialized = false;
    }
}

/**
 * Plays the cure for when a starship collides with a planet.
 * This uses a cooldown to avoid audio overload during big battles.
 */
void SoundManagerPlayShipImpact(void) {
    // We reuse the impact cooldown but generate new pitches per impact.
    DWORD now = GetTickCount();
    bool shouldPlay = false;

    SoundManagerEnsureLock();
    EnterCriticalSection(&soundLock);
    // Cooldown avoids a wall of tones during large-scale collisions.
    if (shipImpactSequence.lastPlayedMs == 0u || (now - shipImpactSequence.lastPlayedMs) >= shipImpactSequence.cooldownMs) {
        shipImpactSequence.lastPlayedMs = now;
        shouldPlay = true;
    }
    LeaveCriticalSection(&soundLock);

    if (!shouldPlay || !soundEnabled) {
        return;
    }

    // Build a tiny melodic fragment from a chromatic scale while keeping steps small.
    SoundToneStep *steps = (SoundToneStep *)malloc(2u * sizeof(SoundToneStep));
    if (steps == NULL) {
        return;
    }

    int startIndex = SoundManagerRandomRange(0, (int)SOUND_CHROMATIC_COUNT - 1);
    // Smaller step offsets keep the chromatic palette from feeling too harsh.
    int offset = SoundManagerRandomRange(-1, 1);
    int nextIndex = startIndex + offset;
    if (nextIndex < 0) {
        nextIndex = 0;
    } else if (nextIndex >= (int)SOUND_CHROMATIC_COUNT) {
        nextIndex = (int)SOUND_CHROMATIC_COUNT - 1;
    }

    // Small timing variation keeps the sequence lively without sounding chaotic.
    DWORD firstDuration = (DWORD)SoundManagerRandomRange(28, 44);
    DWORD secondDuration = (DWORD)SoundManagerRandomRange(36, 60);

    steps[0].frequency = SOUND_CHROMATIC_FREQS[startIndex];
    steps[0].durationMs = firstDuration;
    steps[0].pauseMs = 6u;
    steps[1].frequency = SOUND_CHROMATIC_FREQS[nextIndex];
    steps[1].durationMs = secondDuration;
    steps[1].pauseMs = 0u;

    SoundManagerStartPlayback(steps, 2u, true);
}

/**
 * Plays the cue for when a planet changes ownership.
 * This is kept separate for easy swapping of celebratory audio.
 */
void SoundManagerPlayPlanetCaptured(void) {
    SoundManagerTryPlaySequence(&planetCapturedSequence);
}

/**
 * Sets the master volume for generated tones.
 * Clamping is used so callers cannot push the mixer into clipping.
 * @param volume Normalized volume scalar.
 */
void SoundManagerSetVolume(float volume) {
    // We guard with the sound lock so concurrent reads stay consistent.
    SoundManagerEnsureLock();
    EnterCriticalSection(&soundLock);
    if (volume < 0.0f) {
        volume = 0.0f;
    } else if (volume > 1.0f) {
        volume = 1.0f;
    }
    // Lower volume keeps tones soft without requiring asset rebalancing.
    soundMasterVolume = volume;
    LeaveCriticalSection(&soundLock);
}

/**
 * Configures a lightweight reverb (echo) applied to generated tones.
 * We clamp values to keep the effect stable and prevent runaway gain.
 * @param enabled True to enable reverb, false to disable.
 * @param delayMs Delay in milliseconds before the echo is mixed in.
 * @param decay Linear decay factor in the range [0.0, 1.0].
 */
void SoundManagerSetReverb(bool enabled, float delayMs, float decay) {
    SoundManagerEnsureLock();
    EnterCriticalSection(&soundLock);

    // The delay must be positive so the echo can be heard distinctly.
    if (delayMs < 1.0f) {
        delayMs = 1.0f;
    }

    // Clamping decay keeps the effect from amplifying over time.
    if (decay < 0.0f) {
        decay = 0.0f;
    } else if (decay > 1.0f) {
        decay = 1.0f;
    }

    reverbEnabled = enabled;
    reverbDelayMs = delayMs;
    reverbDecay = decay;

    LeaveCriticalSection(&soundLock);
}
