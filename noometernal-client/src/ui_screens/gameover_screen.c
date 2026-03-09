/**
 * @file gameover_screen.c
 * @brief Implementation of the defeat/game over screen.
 *
 * Displays game over message, match statistics, selected coin preview, and
 * buttons for retrying or returning to menu.
 */

/**
 * @brief Renders the Game Over screen, including the background, selected coin
 * display, statistics, and interactive buttons.
 *
 * This function draws a tinted background—either the scaled game background or
 * a placeholder color—and overlays the prominent "GAME OVER" title. It displays
 * the currently selected coin at the center, followed by static placeholders
 * for survival time and final score. Two large buttons ("Retry" and "Menu")
 * with hover highlighting and click handling are rendered at the bottom
 * corners. Button interactions update the provided @p settings to trigger
 * screen transitions and reset the loading progress when retrying.
 *
 * @param assets Pointer to the loaded asset set containing textures and fonts
 * used for rendering.
 * @param player Pointer to the player data structure, used here to determine
 * which coin texture to display.
 * @param currentScreen Unused in the current implementation but maintained for
 * interface consistency with other screens.
 * @param loadingProgress Pointer to the loading progress meter, reset when the
 * retry button is clicked.
 * @param settings Pointer to the shared game settings used to initiate screen
 * transitions.
 */
#include "ui/gameover_screen.h"
#include "ui/ui_utils.h"
#include <raylib.h>
#include <stdio.h>

/* ============================================================
   ======================== GAME OVER SCREEN ===================
   ============================================================ */

/**
 * @brief Calculate the X position to center text horizontally on screen.
 *
 * Measures the text width using the provided font and computes the starting
 * X coordinate to center it. Falls back to simple calculation if font is not
 * loaded.
 *
 * @param font The Raylib Font to use for text measurement.
 * @param fontLoaded Whether the font was successfully loaded.
 * @param text The text string to measure.
 * @param screenWidth The width of the screen or target area.
 * @return The X coordinate for centered text placement.
 */
static float GetCenteredTextX(Font font, bool fontLoaded, const char* text,
							  float fontSize, float totalWidth) {
	if (fontLoaded) {
		Vector2 size = MeasureTextEx(font, text, fontSize, fontSize * 0.05f);
		return (totalWidth - size.x) * 0.5f;
	}

	int width = MeasureText(text, (int)fontSize);
	return (totalWidth - (float)width) * 0.5f;
}

void DrawGameOverScreen(Assets* assets, PlayerData* player,
						GameScreen* currentScreen, float* loadingProgress,
						GameSettings* settings) {
	const float screenWidth = 1600.0f;
	const float screenHeight = 800.0f;

	// Background texture with the same scaling strategy as the game screen
	if (assets->gameBackground.id > 0) {
		float scaleX = screenWidth / assets->gameBackground.width;
		float scaleY = screenHeight / assets->gameBackground.height;
		float scale = (scaleX > scaleY) ? scaleX : scaleY;

		int drawWidth = (int)(assets->gameBackground.width * scale);
		int drawHeight = (int)(assets->gameBackground.height * scale);
		int drawX = (int)((screenWidth - drawWidth) / 2.0f);
		int drawY = (int)((screenHeight - drawHeight) / 2.0f);

		DrawTexturePro(assets->gameBackground,
					   (Rectangle){0, 0, (float)assets->gameBackground.width,
								   (float)assets->gameBackground.height},
					   (Rectangle){(float)drawX, (float)drawY, (float)drawWidth,
								   (float)drawHeight},
					   (Vector2){0, 0}, 0.0f, WHITE);

		DrawRectangle(0, 0, (int)screenWidth, (int)screenHeight,
					  (Color){0, 0, 0, 150});
	} else {
		DrawRectangle(0, 0, (int)screenWidth, (int)screenHeight,
					  (Color){30, 0, 0, 100});
	}

	const char* title = "GAME OVER";
	const int titleFontSize = 90;
	float titleX = GetCenteredTextX(assets->doomFont, assets->fontLoaded, title,
									(float)titleFontSize, screenWidth);
	DrawDoomText(assets->doomFont, assets->fontLoaded, title, (int)titleX, 100,
				 titleFontSize, (Color){150, 0, 0, 255});

	int centerX = (int)(screenWidth / 2.0f);
	int centerY = 320;

	// Display the currently selected coin in the middle of the screen
	int coinDisplaySize = 350;
	if (assets->coinTextures[player->selectedCoin].id > 0) {
		DrawTexturePro(
			assets->coinTextures[player->selectedCoin],
			(Rectangle){0, 0, assets->coinTextures[player->selectedCoin].width,
						assets->coinTextures[player->selectedCoin].height},
			(Rectangle){centerX - coinDisplaySize / 2,
						centerY - coinDisplaySize / 2, coinDisplaySize,
						coinDisplaySize},
			(Vector2){0, 0}, 0.0f, WHITE);
	}

	const int statsTopY =
		centerY + (coinDisplaySize / 2) - 35; // padding below coin

	const char* soulsText = "SOULS EARNED: +100";
	const int infoFontSize = 28;
	float soulsX =
		GetCenteredTextX(assets->eternalFont, assets->fontLoaded, soulsText,
						 (float)infoFontSize, screenWidth);
	DrawDoomText(assets->eternalFont, assets->fontLoaded, soulsText,
				 (int)soulsX, statsTopY, infoFontSize,
				 (Color){200, 100, 100, 255});

	const char* xpText = "XP GAINED: +50";
	float xpX = GetCenteredTextX(assets->eternalFont, assets->fontLoaded,
								 xpText, (float)infoFontSize, screenWidth);
	DrawDoomText(assets->eternalFont, assets->fontLoaded, xpText, (int)xpX,
				 statsTopY + 35, infoFontSize, (Color){200, 100, 100, 255});

	Vector2 mousePos = GetMousePosition();

	int btnWidth = 500;
	int btnHeight = 350;
	int padding = 40;

	Rectangle retryBtnRect = {screenWidth - btnWidth - padding,
							  screenHeight - btnHeight - padding, btnWidth,
							  btnHeight};
	Rectangle retryHitbox2 = GetCenteredHitbox(retryBtnRect, 0.4f);

	if (assets->btnRetry.id > 0) {
		Color retryTint = CheckCollisionPointRec(mousePos, retryHitbox2)
							  ? (Color){255, 200, 100, 255}
							  : WHITE;
		DrawTexturePro(
			assets->btnRetry,
			(Rectangle){0, 0, assets->btnRetry.width, assets->btnRetry.height},
			retryBtnRect, (Vector2){0, 0}, 0.0f, retryTint);

		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
			CheckCollisionPointRec(mousePos, retryHitbox2)) {
			settings->targetScreen = SCREEN_LOADING;
			settings->isTransitioning = true;
			if (loadingProgress) {
				*loadingProgress = 0.0f;
			}
			if (settings) {
				settings->onlineMatchRequested = true;
				settings->onlineState = ONLINE_STATE_CONNECTING;
				settings->onlineMatchActive = false;
				settings->onlineMatchReady = false;
				settings->onlineMatchCancelRequested = false;
				settings->onlineError[0] = '\0';
				settings->onlineLastMatch = true;
				snprintf(settings->onlineStatus, sizeof(settings->onlineStatus),
						 "Connexion au serveur %s:%d...", player->serverIP,
						 player->serverPort);
			}
		}
	}

	Rectangle menuBtnRect = {padding, screenHeight - btnHeight - padding,
							 btnWidth, btnHeight};
	Rectangle menuHitbox2 = GetCenteredHitbox(menuBtnRect, 0.4f);

	if (assets->btnMenu.id > 0) {
		Color menuTint = CheckCollisionPointRec(mousePos, menuHitbox2)
							 ? (Color){255, 200, 100, 255}
							 : WHITE;
		DrawTexturePro(
			assets->btnMenu,
			(Rectangle){0, 0, assets->btnMenu.width, assets->btnMenu.height},
			menuBtnRect, (Vector2){0, 0}, 0.0f, menuTint);

		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
			CheckCollisionPointRec(mousePos, menuHitbox2)) {
			settings->targetScreen = SCREEN_HOME;
			settings->isTransitioning = true;
		}
	}
}
