/**
 * @file settings_ui.c
 * @brief Implementation of settings UI panel and button rendering.
 *
 * Provides functions to render an overlay settings button and a modal settings
 * panel with audio volume, SFX volume, music toggle, and FPS display controls.
 */

#include "settings_ui.h"
#include "ui.h"

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 800

/**
 * @brief Render settings button in top-right corner with interaction handling.
 *
 * Draws a small clickable settings icon. Toggles panel visibility on click.
 * Uses transition alpha for fade effects during screen transitions.
 *
 * @param assets Pointer to Assets (for button texture).
 * @param transitionAlpha Screen transition fade alpha [0.0, 1.0].
 * @param showSettings Pointer to panel visibility flag (toggled on click).
 */
void DrawSettingsButton(Assets* assets, float transitionAlpha,
						bool* showSettings) {
	Vector2 mousePos = GetMousePosition();
	Rectangle settingsBtn = {1600 - 110, 15, 90, 90};

	if (assets->btnSettings.id > 0) {
		Color tint =
			CheckCollisionPointRec(mousePos, settingsBtn)
				? (Color){255, 255, 255, (unsigned char)(255 * transitionAlpha)}
				: (Color){255, 255, 255,
						  (unsigned char)(200 * transitionAlpha)};
		DrawTexturePro(assets->btnSettings,
					   (Rectangle){0, 0, assets->btnSettings.width,
								   assets->btnSettings.height},
					   settingsBtn, (Vector2){0, 0}, 0.0f, tint);
	} else {
		// Fallback if image doesn't exist
		Color settingsColor =
			CheckCollisionPointRec(mousePos, settingsBtn)
				? (Color){255, 165, 0, (unsigned char)(255 * transitionAlpha)}
				: (Color){255, 200, 0, (unsigned char)(255 * transitionAlpha)};
		DrawCircle(settingsBtn.x + 45, settingsBtn.y + 45, 40,
				   (Color){20, 20, 30, (unsigned char)(230 * transitionAlpha)});
		DrawCircle(settingsBtn.x + 45, settingsBtn.y + 45, 30, settingsColor);
	}

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
		CheckCollisionPointRec(mousePos, settingsBtn)) {
		*showSettings = !(*showSettings);
	}
}

/**
 * @brief Render the settings panel overlay with configuration controls.
 *
 * Displays a modal panel with sliders for music volume, SFX volume, toggles for
 * music enable/disable and FPS display, and a close button. Updates GameSettings
 * based on user interaction.
 *
 * @param assets Pointer to Assets for rendering.
 * @param settings Pointer to GameSettings to read and update.
 * @param transitionAlpha Screen transition fade alpha [0.0, 1.0].
 * @param showSettings Pointer to panel visibility flag (toggled by close button).
 */
void DrawSettingsPanel(Assets* assets, GameSettings* settings,
					   bool audioAvailable, bool* showSettings) {
	Vector2 mousePos = GetMousePosition();
	int panelW = 500;
	int panelH = 450;
	int panelX = (SCREEN_WIDTH - panelW) / 2;
	int panelY = (SCREEN_HEIGHT - panelH) / 2;

	// Background overlay
	DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 180});

	// Panel
	DrawRectangle(panelX, panelY, panelW, panelH, (Color){15, 5, 5, 250});
	DrawRectangleGradientV(panelX, panelY, panelW, panelH,
						   (Color){40, 10, 0, 230}, (Color){10, 5, 5, 250});
	DrawRectangleLinesEx((Rectangle){panelX, panelY, panelW, panelH}, 4,
						 (Color){150, 0, 0, 255});

	Rectangle closeBtn = {panelX + panelW - 35, panelY + 5, 30, 30};
	bool closeHover = CheckCollisionPointRec(mousePos, closeBtn);
	Color closeColor = closeHover ? RED : DARKGRAY;
	DrawRectangleLinesEx(closeBtn, 2, closeColor);
	DrawLine(closeBtn.x + 6, closeBtn.y + 6, closeBtn.x + closeBtn.width - 6,
			 closeBtn.y + closeBtn.height - 6, closeColor);
	DrawLine(closeBtn.x + closeBtn.width - 6, closeBtn.y + 6, closeBtn.x + 6,
			 closeBtn.y + closeBtn.height - 6, closeColor);
	if (closeHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		if (showSettings) {
			*showSettings = false;
		}
		return;
	}

	// Title
	if (assets->eternalFontLoaded) {
		DrawTextEx(assets->eternalFont, "SETTINGS",
				   (Vector2){panelX + panelW / 2 - 100, panelY + 20}, 42, 2.1f,
				   (Color){200, 50, 50, 255});
	} else {
		DrawDoomText(assets->doomFont, assets->fontLoaded, "SETTINGS",
					 panelX + panelW / 2 - 100, panelY + 20, 42,
					 (Color){200, 50, 50, 255});
	}

	// Music Volume
	if (assets->eternalFontLoaded) {
		DrawTextEx(assets->eternalFont, "MUSIC VOLUME",
				   (Vector2){panelX + 30, panelY + 100}, 28, 1.4f,
				   (Color){200, 200, 200, 255});
	} else {
		DrawDoomText(assets->doomFont, assets->fontLoaded, "MUSIC VOLUME",
					 panelX + 30, panelY + 100, 28,
					 (Color){200, 200, 200, 255});
	}

	Rectangle musicSlider = {panelX + 30, panelY + 140, panelW - 60, 30};
	DrawRectangle(musicSlider.x, musicSlider.y, musicSlider.width,
				  musicSlider.height, (Color){40, 0, 0, 255});

	// Red gradient based on percentage
	int fillWidth = (int)(musicSlider.width * settings->musicVolume);
	for (int i = 0; i < fillWidth; i++) {
		float ratio = (float)i / musicSlider.width;
		Color gradientColor =
			(Color){(unsigned char)(150 + ratio * 105), 0, 0, 255};
		DrawRectangle(musicSlider.x + i, musicSlider.y, 1, musicSlider.height,
					  gradientColor);
	}

	DrawRectangleLinesEx(musicSlider, 3, (Color){150, 0, 0, 255});

	if (CheckCollisionPointRec(mousePos, musicSlider) &&
		IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
		settings->musicVolume =
			(mousePos.x - musicSlider.x) / musicSlider.width;
		if (settings->musicVolume < 0.0f)
			settings->musicVolume = 0.0f;
		if (settings->musicVolume > 1.0f)
			settings->musicVolume = 1.0f;
	}

	if (assets->eternalFontLoaded) {
		DrawTextEx(assets->eternalFont,
				   TextFormat("%.0f%%", settings->musicVolume * 100),
				   (Vector2){panelX + panelW - 80, panelY + 142}, 26, 1.3f,
				   (Color){255, 100, 100, 255});
	} else {
		DrawDoomText(assets->doomFont, assets->fontLoaded,
					 TextFormat("%.0f%%", settings->musicVolume * 100),
					 panelX + panelW - 80, panelY + 142, 26,
					 (Color){255, 100, 100, 255});
	}

	// SFX Volume
	if (assets->eternalFontLoaded) {
		DrawTextEx(assets->eternalFont, "SFX VOLUME",
				   (Vector2){panelX + 30, panelY + 200}, 28, 1.4f,
				   (Color){200, 200, 200, 255});
	} else {
		DrawDoomText(assets->doomFont, assets->fontLoaded, "SFX VOLUME",
					 panelX + 30, panelY + 200, 28,
					 (Color){200, 200, 200, 255});
	}

	Rectangle sfxSlider = {panelX + 30, panelY + 240, panelW - 60, 30};
	DrawRectangle(sfxSlider.x, sfxSlider.y, sfxSlider.width, sfxSlider.height,
				  (Color){40, 0, 0, 255});

	// Red gradient based on percentage
	int sfxFillWidth = (int)(sfxSlider.width * settings->sfxVolume);
	for (int i = 0; i < sfxFillWidth; i++) {
		float ratio = (float)i / sfxSlider.width;
		Color gradientColor =
			(Color){(unsigned char)(150 + ratio * 105), 0, 0, 255};
		DrawRectangle(sfxSlider.x + i, sfxSlider.y, 1, sfxSlider.height,
					  gradientColor);
	}

	DrawRectangleLinesEx(sfxSlider, 3, (Color){150, 0, 0, 255});

	if (CheckCollisionPointRec(mousePos, sfxSlider) &&
		IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
		settings->sfxVolume = (mousePos.x - sfxSlider.x) / sfxSlider.width;
		if (settings->sfxVolume < 0.0f)
			settings->sfxVolume = 0.0f;
		if (settings->sfxVolume > 1.0f)
			settings->sfxVolume = 1.0f;
	}

	if (assets->eternalFontLoaded) {
		DrawTextEx(assets->eternalFont,
				   TextFormat("%.0f%%", settings->sfxVolume * 100),
				   (Vector2){panelX + panelW - 80, panelY + 242}, 26, 1.3f,
				   (Color){255, 100, 100, 255});
	} else {
		DrawDoomText(assets->doomFont, assets->fontLoaded,
					 TextFormat("%.0f%%", settings->sfxVolume * 100),
					 panelX + panelW - 80, panelY + 242, 26,
					 (Color){255, 100, 100, 255});
	}

	// Music Toggle
	Rectangle musicToggle = {panelX + 30, panelY + 300, 50, 30};
	DrawRectangle(musicToggle.x, musicToggle.y, musicToggle.width,
				  musicToggle.height,
				  settings->musicEnabled ? (Color){0, 150, 0, 255}
										 : (Color){100, 0, 0, 255});
	DrawRectangleLinesEx(musicToggle, 3, (Color){150, 0, 0, 255});

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
		CheckCollisionPointRec(mousePos, musicToggle)) {
		settings->musicEnabled = !settings->musicEnabled;
		if (audioAvailable && assets->musicLoaded) {
			if (settings->musicEnabled) {
				ResumeMusicStream(assets->lobbyMusic);
				ResumeMusicStream(assets->songMusic);
			} else {
				PauseMusicStream(assets->lobbyMusic);
				PauseMusicStream(assets->songMusic);
			}
		}
	}

	if (assets->eternalFontLoaded) {
		DrawTextEx(assets->eternalFont,
				   settings->musicEnabled ? "MUSIC: ON" : "MUSIC: OFF",
				   (Vector2){panelX + 90, panelY + 303}, 26, 1.3f,
				   (Color){200, 200, 200, 255});
	} else {
		DrawDoomText(assets->doomFont, assets->fontLoaded,
					 settings->musicEnabled ? "MUSIC: ON" : "MUSIC: OFF",
					 panelX + 90, panelY + 303, 26,
					 (Color){200, 200, 200, 255});
	}

	// Show FPS Toggle
	Rectangle fpsToggle = {panelX + 30, panelY + 350, 50, 30};
	DrawRectangle(fpsToggle.x, fpsToggle.y, fpsToggle.width, fpsToggle.height,
				  settings->showFPS ? (Color){0, 150, 0, 255}
									: (Color){100, 0, 0, 255});
	DrawRectangleLinesEx(fpsToggle, 3, (Color){150, 0, 0, 255});

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
		CheckCollisionPointRec(mousePos, fpsToggle)) {
		settings->showFPS = !settings->showFPS;
	}

	if (assets->eternalFontLoaded) {
		DrawTextEx(assets->eternalFont,
				   settings->showFPS ? "SHOW FPS: ON" : "SHOW FPS: OFF",
				   (Vector2){panelX + 90, panelY + 353}, 26, 1.3f,
				   (Color){200, 200, 200, 255});
	} else {
		DrawDoomText(assets->doomFont, assets->fontLoaded,
					 settings->showFPS ? "SHOW FPS: ON" : "SHOW FPS: OFF",
					 panelX + 90, panelY + 353, 26,
					 (Color){200, 200, 200, 255});
	}
}
