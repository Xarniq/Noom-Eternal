/**
 * @file ui.c
 * @brief Implementation of asset loading and unloading for UI rendering.
 *
 * Loads textures, fonts, audio streams, and video content from disk into
 * memory. Gracefully handles missing files and provides fallback rendering
 * capabilities.
 */

/**
 * @brief Load all game assets from disk.
 *
 * Attempts to load all textures, fonts, music, and video files from the assets
 * directory. Sets appropriate flags in the Assets structure for each
 * successfully loaded resource. Missing files are logged but do not prevent
 * application startup. Video queue buffers are configured for smooth playback.
 *
 * @param assets Pointer to Assets structure to populate with loaded resources.
 */

/**
 * @brief Unload and release all previously loaded assets.
 *
 * Frees all textures, fonts, music streams, and video media that were
 * successfully loaded. Must be called during shutdown. Safe to call multiple
 * times.
 *
 * @param assets Pointer to Assets structure containing resources to release.
 */
#include "ui.h"
#include <stdio.h>

void LoadAssets(Assets* assets) {
	assets->backgroundLoaded = false;
	assets->fontLoaded = false;
	assets->musicLoaded = false;
	assets->introLoaded = false;

	if (FileExists("assets/doom_eternal_tower.png")) {
		assets->background = LoadTexture("assets/doom_eternal_tower.png");
		assets->backgroundLoaded = true;
	}

	if (FileExists("assets/b2.jpg")) {
		assets->gameBackground = LoadTexture("assets/b2.jpg");
	}

	if (FileExists("assets/lobby.mp3")) {
		assets->lobbyMusic = LoadMusicStream("assets/lobby.mp3");
		assets->musicLoaded = true;
	}

	if (FileExists("assets/song.mp3")) {
		assets->songMusic = LoadMusicStream("assets/song.mp3");
	}

	// Configurer les buffers vidéo pour une meilleure fluidité
	SetMediaFlag(MEDIA_VIDEO_QUEUE,
				 512); // Augmenter la capacité de la queue vidéo
	SetMediaFlag(MEDIA_AUDIO_QUEUE,
				 256); // Augmenter la capacité de la queue audio
	SetMediaFlag(MEDIA_VIDEO_MAX_DELAY, 100); // Réduire le délai max vidéo
	SetMediaFlag(MEDIA_AUDIO_DECODED_BUFFER, 192000); // Buffer audio plus large

	// Charger la vidéo d'intro
	if (FileExists("assets/Intro.mp4")) {
		assets->introVideo = LoadMedia("assets/Intro.mp4");
		if (assets->introVideo.ctx != NULL) {
			assets->introLoaded = true;
		}
	}

	if (FileExists("assets/SHOP.png"))
		assets->btnShop = LoadTexture("assets/SHOP.png");
	if (FileExists("assets/PLAY.png"))
		assets->btnPlay = LoadTexture("assets/PLAY.png");
	if (FileExists("assets/BACK.png"))
		assets->btnBack = LoadTexture("assets/BACK.png");
	if (FileExists("assets/BUY.png"))
		assets->btnBuy = LoadTexture("assets/BUY.png");
	if (FileExists("assets/SKIN.png"))
		assets->btnSkin = LoadTexture("assets/SKIN.png");
	if (FileExists("assets/CANCEL.png"))
		assets->btnCancel = LoadTexture("assets/CANCEL.png");
	if (FileExists("assets/CONTINUE.png"))
		assets->btnContinue = LoadTexture("assets/CONTINUE.png");
	if (FileExists("assets/RETRY.png"))
		assets->btnRetry = LoadTexture("assets/RETRY.png");
	if (FileExists("assets/MENU.png"))
		assets->btnMenu = LoadTexture("assets/MENU.png");
	if (FileExists("assets/RESTART.png"))
		assets->btnRestart = LoadTexture("assets/RESTART.png");
	if (FileExists("assets/settings.png"))
		assets->btnSettings = LoadTexture("assets/settings.png");
	if (FileExists("assets/carres.png"))
		assets->slotTexture = LoadTexture("assets/carres.png");

	for (int i = 0; i < 6; i++) {
		char path[64];
		snprintf(path, sizeof(path), "assets/Coin%d.png", i + 1);
		if (FileExists(path))
			assets->coinTextures[i] = LoadTexture(path);
	}

	if (FileExists("assets/AmazDooMLeft.ttf")) {
		assets->doomFont = LoadFontEx("assets/AmazDooMLeft.ttf", 128, 0, 0);
		assets->fontLoaded = true;
		SetTextureFilter(assets->doomFont.texture, TEXTURE_FILTER_BILINEAR);
	}

	assets->eternalFontLoaded = false;
	if (FileExists("assets/EternalUiRegular-BWZGd.ttf")) {
		assets->eternalFont =
			LoadFontEx("assets/EternalUiRegular-BWZGd.ttf", 64, 0, 0);
		assets->eternalFontLoaded = true;
		SetTextureFilter(assets->eternalFont.texture, TEXTURE_FILTER_BILINEAR);
	}
}

void UnloadAssets(Assets* assets) {
	if (assets->backgroundLoaded)
		UnloadTexture(assets->background);
	if (assets->gameBackground.id > 0)
		UnloadTexture(assets->gameBackground);
	if (assets->fontLoaded)
		UnloadFont(assets->doomFont);
	if (assets->eternalFontLoaded)
		UnloadFont(assets->eternalFont);
	if (assets->musicLoaded) {
		UnloadMusicStream(assets->lobbyMusic);
		UnloadMusicStream(assets->songMusic);
	}

	// Décharger la vidéo d'intro
	if (assets->introLoaded) {
		UnloadMedia(&assets->introVideo);
	}

	if (assets->btnShop.id > 0)
		UnloadTexture(assets->btnShop);
	if (assets->btnPlay.id > 0)
		UnloadTexture(assets->btnPlay);
	if (assets->btnBack.id > 0)
		UnloadTexture(assets->btnBack);
	if (assets->btnBuy.id > 0)
		UnloadTexture(assets->btnBuy);
	if (assets->btnSkin.id > 0)
		UnloadTexture(assets->btnSkin);
	if (assets->btnCancel.id > 0)
		UnloadTexture(assets->btnCancel);
	if (assets->btnContinue.id > 0)
		UnloadTexture(assets->btnContinue);
	if (assets->btnRetry.id > 0)
		UnloadTexture(assets->btnRetry);
	if (assets->btnMenu.id > 0)
		UnloadTexture(assets->btnMenu);
	if (assets->btnRestart.id > 0)
		UnloadTexture(assets->btnRestart);
	if (assets->btnSettings.id > 0)
		UnloadTexture(assets->btnSettings);
	if (assets->slotTexture.id > 0)
		UnloadTexture(assets->slotTexture);

	for (int i = 0; i < 6; i++)
		if (assets->coinTextures[i].id > 0)
			UnloadTexture(assets->coinTextures[i]);
}
