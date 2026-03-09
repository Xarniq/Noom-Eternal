/**
 * @file loading_screen.c
 * @brief Implementation of the loading/matchmaking screen.
 *
 * Renders a loading screen with animated coin, progress bar, decorative
 * particles, and options to cancel matchmaking or loading operations.
 */

/**
 * @brief Render the loading screen with progress indicator.
 *
 * Displays animated loading visualization with rotating coin, progress bar,
 * and status messages for asset loading or matchmaking. Provides cancel button
 * for user interruption.
 *
 * @param assets Pointer to loaded assets (coin textures, fonts).
 * @param player Pointer to player data for selected coin display.
 * @param currentScreen Pointer to current screen.
 * @param loadingProgress Pointer to loading progress [0.0, 1.0].
 * @param settings Pointer to game settings including online status.
 */
#include "ui/loading_screen.h"
#include "ui/ui_utils.h"
#include <math.h>
#include <raylib.h>
#include <stdio.h>

/* ============================================================
   ======================== LOADING SCREEN =====================
   ============================================================ */

void DrawLoadingScreen(Assets* assets, PlayerData* player,
					   GameScreen* currentScreen, float* loadingProgress,
					   GameSettings* settings) {

	DrawRectangle(0, 0, 1600, 800, (Color){0, 0, 0, 180});

	int centerX = 1600 / 2;
	int centerY = 800 / 2 - 60;

	// --- COIN DISPLAY WITH FIXED CENTER ROTATION ---
	if (assets->coinTextures[player->selectedCoin].id > 0) {
		int coinDisplaySize = 645;
		float scale = (float)coinDisplaySize /
					  fmaxf(assets->coinTextures[player->selectedCoin].width,
							assets->coinTextures[player->selectedCoin].height);

		int drawWidth =
			(int)(assets->coinTextures[player->selectedCoin].width * scale);
		int drawHeight =
			(int)(assets->coinTextures[player->selectedCoin].height * scale);

		float rotation = GetTime() * 75.0f; // rotation douce

		// Rectangles
		Rectangle src =
			(Rectangle){0, 0, assets->coinTextures[player->selectedCoin].width,
						assets->coinTextures[player->selectedCoin].height};

		// Destination rectangle stores the center position directly
		Rectangle dest = (Rectangle){centerX, centerY, drawWidth, drawHeight};

		// Pivot at the exact center to guarantee zero drift while rotating
		Vector2 origin = (Vector2){drawWidth / 2, drawHeight / 2};

		DrawTexturePro(assets->coinTextures[player->selectedCoin], src, dest,
					   origin, rotation, WHITE);
	}

	// --- LOADING PROGRESS (visual only) ---
	*loadingProgress += 0.35f * GetFrameTime();
	if (*loadingProgress > 1.0f)
		*loadingProgress -= 1.0f;

	int barY = centerY + 220;
	DrawDoomText(assets->doomFont, assets->fontLoaded, "", (1600 - 200) / 2,
				 barY - 50, 32, ORANGE);

	// Decorative particle effect rendered below the coin
	for (int i = 0; i < 3; i++) {
		int px = centerX - 30 + i * 30;
		int py = centerY + 280 + (int)(sinf(GetTime() * 2 + i) * 5);
		DrawCircle(px, py, 6,
				   (Color){255, 69, 0, (unsigned char)(200 - i * 40)});
	}

	if (settings->onlineMatchReady) {
		settings->onlineMatchReady = false;
		settings->onlineMatchActive = true;
		settings->targetScreen = SCREEN_GAME;
		settings->isTransitioning = true;
	}

	const char* statusText = settings->onlineError[0] ? settings->onlineError
													  : settings->onlineStatus;
	if (statusText[0] != '\0') {
		DrawDoomText(assets->doomFont, assets->fontLoaded, statusText,
					 (1600 / 2) - 350, centerY + 300, 28,
					 settings->onlineError[0] ? RED : ORANGE);
	}

	// --- CANCEL BUTTON ---
	Vector2 mousePos = GetMousePosition();
	Rectangle cancelBtnRect = (Rectangle){40, 800 - 245 - 40, 350, 245};
	Rectangle cancelHitbox = GetCenteredHitbox(cancelBtnRect, 0.4f);

	if (assets->btnCancel.id > 0) {
		Color cancelTint = CheckCollisionPointRec(mousePos, cancelHitbox)
							   ? (Color){255, 200, 100, 255}
							   : WHITE;
		DrawTexturePro(assets->btnCancel,
					   (Rectangle){0, 0, assets->btnCancel.width,
								   assets->btnCancel.height},
					   cancelBtnRect, (Vector2){0, 0}, 0.0f, cancelTint);

		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
			CheckCollisionPointRec(mousePos, cancelHitbox)) {
			settings->onlineMatchCancelRequested = true;
			settings->onlineStatus[0] = '\0';
			snprintf(settings->onlineError, sizeof(settings->onlineError),
					 "Connection canceled by the user");
			settings->targetScreen = SCREEN_HOME;
			settings->isTransitioning = true;
		}
	}
}
