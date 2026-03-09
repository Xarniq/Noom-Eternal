/**
 * @file victory_screen.c
 * @brief Implementation of the victory/win screen.
 *
 * Displays victory message, earned rewards (XP, coins/souls), selected coin
 * preview, and buttons for continuing or retrying.
 */

/**
 * @brief Render the victory screen with rewards and navigation.
 *
 * Shows victory message, player rewards from winning the match, and buttons
 * to continue to next match or return to menu.
 *
 * @param assets Pointer to loaded assets (background, fonts).
 * @param player Pointer to player data for reward display.
 * @param currentScreen Pointer to current screen.
 * @param loadingProgress Pointer to loading progress.
 * @param settings Pointer to game settings for navigation.
 */
#include "ui/victory_screen.h"
#include "ui/ui_utils.h"
#include <raylib.h>
#include <stdio.h>

/* ============================================================
   ======================== VICTORY SCREEN =====================
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

void DrawVictoryScreen(Assets* assets, PlayerData* player,
					   GameScreen* currentScreen, float* loadingProgress,
					   GameSettings* settings) {
	const float screenWidth = 1600.0f;
	const float screenHeight = 800.0f;

	// Background uses the same scaling strategy as the game screen
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
					  (Color){255, 100, 0, 30});
	}

	const char* victoryText = "VICTORY";
	const int victoryFontSize = 100;
	float victoryX =
		GetCenteredTextX(assets->doomFont, assets->fontLoaded, victoryText,
						 (float)victoryFontSize, screenWidth);
	DrawDoomText(assets->doomFont, assets->fontLoaded, victoryText,
				 (int)victoryX, 80, victoryFontSize, GOLD);

	int centerX = (int)(screenWidth / 2.0f);
	int centerY = 300;

	// Display the selected coin in the center of the screen
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

	const char* soulsText = "SOULS EARNED: +250";
	const int infoFontSize = 28;
	float soulsX =
		GetCenteredTextX(assets->eternalFont, assets->fontLoaded, soulsText,
						 (float)infoFontSize, screenWidth);
	DrawDoomText(assets->eternalFont, assets->fontLoaded, soulsText,
				 (int)soulsX, 420, infoFontSize, GOLD);

	const char* xpText = "XP GAINED: +200";
	float xpX = GetCenteredTextX(assets->eternalFont, assets->fontLoaded,
								 xpText, (float)infoFontSize, screenWidth);
	DrawDoomText(assets->eternalFont, assets->fontLoaded, xpText, (int)xpX, 455,
				 infoFontSize, ORANGE);

	Vector2 mousePos = GetMousePosition();

	int btnWidth = 500;
	int btnHeight = 350;
	int padding = 40;

	Rectangle continueBtnRect = {screenWidth - btnWidth - padding,
								 screenHeight - btnHeight - padding, btnWidth,
								 btnHeight};
	Rectangle continueHitbox = GetCenteredHitbox(continueBtnRect, 0.4f);

	if (assets->btnContinue.id > 0) {
		Color continueTint = CheckCollisionPointRec(mousePos, continueHitbox)
								 ? (Color){255, 200, 100, 255}
								 : WHITE;
		DrawTexturePro(assets->btnContinue,
					   (Rectangle){0, 0, assets->btnContinue.width,
								   assets->btnContinue.height},
					   continueBtnRect, (Vector2){0, 0}, 0.0f, continueTint);

		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
			CheckCollisionPointRec(mousePos, continueHitbox)) {
			settings->targetScreen = SCREEN_HOME;
			settings->isTransitioning = true;
		}
	}

	Rectangle retryBtnRect = {padding, screenHeight - btnHeight - padding,
							  btnWidth, btnHeight};
	Rectangle retryHitbox = GetCenteredHitbox(retryBtnRect, 0.4f);

	if (assets->btnRetry.id > 0) {
		Color retryTint = CheckCollisionPointRec(mousePos, retryHitbox)
							  ? (Color){255, 200, 100, 255}
							  : WHITE;
		DrawTexturePro(
			assets->btnRetry,
			(Rectangle){0, 0, assets->btnRetry.width, assets->btnRetry.height},
			retryBtnRect, (Vector2){0, 0}, 0.0f, retryTint);

		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
			CheckCollisionPointRec(mousePos, retryHitbox)) {
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
}
