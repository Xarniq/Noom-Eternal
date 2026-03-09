/**
 * @file audio_manager.h
 * @brief Background music management with screen-aware playlist switching.
 *
 * Handles dynamic music transitions based on the current game screen, with
 * support for fade-in effects and volume control. Gracefully handles cases
 * where audio initialization fails.
 */

#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "raylib.h"
#include "types.h"

/**
 * @brief Update and manage background music playback.
 *
 * Selects appropriate music tracks based on the current screen, manages fade-in
 * timing, and applies the current volume settings. Lobby/menu screens play
 * lobby.mp3, while gameplay screens play song.mp3. Music stops on intro
 * screens.
 *
 * @param assets Pointer to the Assets structure containing loaded music
 * streams.
 * @param settings Pointer to GameSettings for volume and playback preferences.
 * @param currentScreen The currently active GameScreen (determines music
 * selection).
 * @param audioAvailable Boolean flag indicating if the audio device is
 * initialized. If false, function returns immediately without playing audio.
 */
void UpdateBackgroundMusic(Assets* assets, GameSettings* settings,
						   GameScreen currentScreen, bool audioAvailable);

#endif // AUDIO_MANAGER_H
