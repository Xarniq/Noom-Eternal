/**
 * @file audio_manager.c
 * @brief Implementation of dynamic music management with fade transitions.
 *
 * Provides screen-aware music selection, automatic fade-in/fade-out, and volume
 * control. Handles graceful fallback when audio device is unavailable.
 */

#include "audio_manager.h"

/**
 * @brief Update and manage background music playback with transitions.
 *
 * Selects appropriate music based on the current screen:
 * - INTRO: all music stops
 * - HOME/SHOP: lobby.mp3 (fade-in over 9 seconds)
 * - GAME/LOADING/VICTORY/GAMEOVER: song.mp3 (fade-in over 10 seconds)
 *
 * Applies current volume setting and handles stream updates. Returns
 * immediately if audio is unavailable.
 *
 * @param assets Pointer to Assets containing loaded music streams.
 * @param settings Pointer to GameSettings for volume and fade timer.
 * @param currentScreen The active GameScreen determining music selection.
 * @param audioAvailable Boolean indicating audio device readiness.
 */
void UpdateBackgroundMusic(Assets* assets, GameSettings* settings,
						   GameScreen currentScreen, bool audioAvailable) {
	if (!audioAvailable || !assets->musicLoaded || !settings->musicEnabled) {
		return;
	}

	// On HOME and SHOP: play lobby.mp3
	if (currentScreen == SCREEN_HOME || currentScreen == SCREEN_SHOP) {
		if (!IsMusicStreamPlaying(assets->lobbyMusic)) {
			StopMusicStream(assets->songMusic);
			PlayMusicStream(assets->lobbyMusic);
			settings->musicFadeTimer = 0.0f;
		}
		UpdateMusicStream(assets->lobbyMusic);

		// Progressive fade in over 9 seconds
		if (settings->musicFadeTimer < 9.0f) {
			settings->musicFadeTimer += GetFrameTime();
			float fadeVolume =
				(settings->musicFadeTimer / 9.0f) * settings->musicVolume;
			SetMusicVolume(assets->lobbyMusic, fadeVolume);
		} else {
			SetMusicVolume(assets->lobbyMusic, settings->musicVolume);
		}
	}
	// On INTRO: stop all music
	else if (currentScreen == SCREEN_INTRO) {
		StopMusicStream(assets->lobbyMusic);
		StopMusicStream(assets->songMusic);
	}
	// On GAME, LOADING, VICTORY, GAMEOVER: play song.mp3
	else if (currentScreen == SCREEN_GAME || currentScreen == SCREEN_LOADING ||
			 currentScreen == SCREEN_VICTORY ||
			 currentScreen == SCREEN_GAMEOVER) {
		if (!IsMusicStreamPlaying(assets->songMusic)) {
			StopMusicStream(assets->lobbyMusic);
			PlayMusicStream(assets->songMusic);
			settings->musicFadeTimer = 0.0f;
		}
		UpdateMusicStream(assets->songMusic);

		// Progressive fade in over 10 seconds
		if (settings->musicFadeTimer < 10.0f) {
			settings->musicFadeTimer += GetFrameTime();
			float fadeVolume =
				(settings->musicFadeTimer / 10.0f) * settings->musicVolume;
			SetMusicVolume(assets->songMusic, fadeVolume);
		} else {
			SetMusicVolume(assets->songMusic, settings->musicVolume);
		}
	}
	// Fallback for other screens
	else {
		if (IsMusicStreamPlaying(assets->lobbyMusic)) {
			UpdateMusicStream(assets->lobbyMusic);
			if (settings->musicFadeTimer >= 9.0f)
				SetMusicVolume(assets->lobbyMusic, settings->musicVolume);
		}
		if (IsMusicStreamPlaying(assets->songMusic)) {
			UpdateMusicStream(assets->songMusic);
			if (settings->musicFadeTimer >= 10.0f)
				SetMusicVolume(assets->songMusic, settings->musicVolume);
		}
	}
}
