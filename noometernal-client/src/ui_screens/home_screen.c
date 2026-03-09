/**
 * @file home_screen.c
 * @brief Implementation of the home/main menu screen.
 *
 * Renders the main menu interface with player profile display, available game
 * modes, shop access, and online/offline matchmaking options.
 */

/**
 * @brief Render the home menu screen.
 *
 * Displays player statistics, level progress, currency balance, and interactive
 * buttons for starting a game, accessing the shop, or initiating online play.
 * Handles all menu navigation input.
 *
 * @param assets Pointer to loaded assets for rendering.
 * @param player Pointer to player data for profile display.
 * @param currentScreen Pointer to current screen for navigation.
 * @param loadingProgress Pointer to loading progress value.
 * @param transitionAlpha Current transition fade alpha.
 * @param settings Pointer to game settings for navigation and online state.
 */
#include "ui/home_screen.h"
#include "ui/ui_utils.h"
#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
   ====================== HOME SCREEN FIX ======================
   ============================================================ */

void DrawHomeScreen(Assets* assets, PlayerData* player,
					GameScreen* currentScreen, float* loadingProgress,
					float transitionAlpha, GameSettings* settings) {
	// --- Input State Management ---
	static bool showConnectionPopup = false;
	static int activeInput = -1; // 0: Username, 1: IP, 2: Port
	static char tempUsername[32] = "";
	static char tempIP[64] = "";
	static char tempPort[16] = "";
	static bool inputsInitialized = false;

	if (!inputsInitialized) {
		strncpy(tempUsername, player->username, sizeof(tempUsername) - 1);
		strncpy(tempIP, player->serverIP, sizeof(tempIP) - 1);
		snprintf(tempPort, sizeof(tempPort), "%d", player->serverPort);
		inputsInitialized = true;
	}

	// Handle Text Input (Only when popup is open)
	if (showConnectionPopup && activeInput >= 0) {
		int key = GetCharPressed();
		while (key > 0) {
			if ((key >= 32) && (key <= 125)) {
				char* target = (activeInput == 0)	? tempUsername
							   : (activeInput == 1) ? tempIP
													: tempPort;
				int maxLen = (activeInput == 0)	  ? 31
							 : (activeInput == 1) ? 63
												  : 15;
				int len = strlen(target);
				if (len < maxLen) {
					target[len] = (char)key;
					target[len + 1] = '\0';
				}
			}
			key = GetCharPressed();
		}
		if (IsKeyPressed(KEY_BACKSPACE)) {
			char* target = (activeInput == 0)	? tempUsername
						   : (activeInput == 1) ? tempIP
												: tempPort;
			int len = strlen(target);
			if (len > 0)
				target[len - 1] = '\0';
		}
	}

	DrawRectangle(0, 0, 1600, 800,
				  (Color){0, 0, 0, (unsigned char)(100 * transitionAlpha)});

	// === PLAYER STATS CARD (DOOM theme) ===
	int cardW = 420;
	int cardH = 240;
	int cardX = 30;
	int cardY = 30;

	DrawRectangle(cardX, cardY, cardW, cardH,
				  (Color){15, 5, 5, (unsigned char)(230 * transitionAlpha)});
	DrawRectangleGradientV(
		cardX, cardY, cardW, cardH,
		(Color){40, 10, 0, (unsigned char)(180 * transitionAlpha)},
		(Color){10, 5, 5, (unsigned char)(230 * transitionAlpha)});

	DrawRectangleLinesEx(
		(Rectangle){cardX, cardY, cardW, cardH}, 4,
		(Color){255, 69, 0, (unsigned char)(255 * transitionAlpha)});
	DrawRectangleLinesEx(
		(Rectangle){cardX + 4, cardY + 4, cardW - 8, cardH - 8}, 2,
		(Color){180, 40, 0, (unsigned char)(200 * transitionAlpha)});

	DrawDoomText(assets->doomFont, assets->fontLoaded, player->username,
				 cardX + 18, cardY + 18, 48,
				 (Color){60, 0, 0, (unsigned char)(255 * transitionAlpha)});
	DrawDoomText(assets->doomFont, assets->fontLoaded, player->username,
				 cardX + 15, cardY + 15, 48,
				 (Color){255, 69, 0, (unsigned char)(255 * transitionAlpha)});

	// Username and server address shown beneath the title
	if (assets->eternalFontLoaded) {
		DrawTextEx(
			assets->eternalFont,
			TextFormat("%s @ %s", player->username, player->serverIP),
			(Vector2){cardX + 15, cardY + 50}, 18, 0.9f,
			(Color){180, 180, 180, (unsigned char)(255 * transitionAlpha)});
	} else {
		DrawDoomText(
			assets->doomFont, assets->fontLoaded,
			TextFormat("%s @ %s", player->username, player->serverIP),
			cardX + 15, cardY + 50, 18,
			(Color){180, 180, 180, (unsigned char)(255 * transitionAlpha)});
	}

	// XP Logic (Modulo System)
	int xpPerLevel = 1000;
	int currentLevel = player->level;
	if (currentLevel < 1)
		currentLevel = 1;
	int currentXP = player->xp;
	int maxXP = xpPerLevel;

	// Level indicator
	if (assets->eternalFontLoaded) {
		DrawTextEx(
			assets->eternalFont, "LVL", (Vector2){cardX + 15, cardY + 75}, 24,
			1.2f,
			(Color){150, 150, 150, (unsigned char)(255 * transitionAlpha)});
		DrawTextEx(
			assets->eternalFont, TextFormat("%d", currentLevel),
			(Vector2){cardX + 75, cardY + 70}, 32, 1.6f,
			(Color){255, 200, 0, (unsigned char)(255 * transitionAlpha)});
	} else {
		DrawDoomText(
			assets->doomFont, assets->fontLoaded, "LVL", cardX + 15, cardY + 75,
			24, (Color){150, 150, 150, (unsigned char)(255 * transitionAlpha)});
		DrawDoomText(
			assets->doomFont, assets->fontLoaded,
			TextFormat("%d", currentLevel), cardX + 75, cardY + 70, 32,
			(Color){255, 200, 0, (unsigned char)(255 * transitionAlpha)});
	}

	// XP Bar
	int xpBarX = cardX + 15;
	int xpBarY = cardY + 120;
	int xpBarW = cardW - 30;
	int xpBarH = 35;

	DrawRectangle(xpBarX + 2, xpBarY + 2, xpBarW, xpBarH,
				  (Color){0, 0, 0, (unsigned char)(100 * transitionAlpha)});
	DrawRectangle(xpBarX, xpBarY, xpBarW, xpBarH,
				  (Color){60, 10, 0, (unsigned char)(255 * transitionAlpha)});

	float xpPercent = (float)currentXP / maxXP;
	if (xpPercent > 1.0f)
		xpPercent = 1.0f;
	int xpFillWidth = (int)(xpBarW * xpPercent);

	DrawRectangleGradientH(
		xpBarX, xpBarY, xpFillWidth, xpBarH,
		(Color){255, 140, 0, (unsigned char)(255 * transitionAlpha)},
		(Color){255, 69, 0, (unsigned char)(255 * transitionAlpha)});

	DrawRectangleLinesEx(
		(Rectangle){xpBarX, xpBarY, xpBarW, xpBarH}, 3,
		(Color){255, 69, 0, (unsigned char)(255 * transitionAlpha)});
	DrawRectangleLinesEx(
		(Rectangle){xpBarX + 2, xpBarY + 2, xpBarW - 4, xpBarH - 4}, 1,
		(Color){100, 30, 0, (unsigned char)(200 * transitionAlpha)});

	DrawDoomText(
		assets->doomFont, assets->fontLoaded,
		TextFormat("%d / %d XP", currentXP, maxXP), xpBarX + xpBarW / 2 - 70,
		xpBarY + 7, 22,
		(Color){255, 255, 255, (unsigned char)(255 * transitionAlpha)});

	DrawRectangle(cardX + 15, cardY + 170, cardW - 30, 2,
				  (Color){255, 69, 0, (unsigned char)(150 * transitionAlpha)});

	// SOULS
	if (assets->eternalFontLoaded) {
		DrawTextEx(
			assets->eternalFont, "SOULS", (Vector2){cardX + 15, cardY + 185},
			26, 1.3f,
			(Color){150, 120, 0, (unsigned char)(255 * transitionAlpha)});
		DrawTextEx(
			assets->eternalFont, TextFormat("%d", player->souls),
			(Vector2){cardX + 140, cardY + 180}, 36, 1.8f,
			(Color){255, 215, 0, (unsigned char)(255 * transitionAlpha)});
	} else {
		DrawDoomText(
			assets->doomFont, assets->fontLoaded, "SOULS", cardX + 15,
			cardY + 185, 26,
			(Color){150, 120, 0, (unsigned char)(255 * transitionAlpha)});
		DrawDoomText(
			assets->doomFont, assets->fontLoaded,
			TextFormat("%d", player->souls), cardX + 140, cardY + 180, 36,
			(Color){255, 215, 0, (unsigned char)(255 * transitionAlpha)});
	}

	for (int i = 0; i < 3; i++) {
		int px = cardX + cardW - 30 - i * 15;
		int py = cardY + 195 + (int)(sinf(GetTime() * 2 + i) * 5);
		DrawCircle(px, py, 3,
				   (Color){255, 215, 0,
						   (unsigned char)((150 - i * 30) * transitionAlpha)});
	}

	// === WIFI BUTTON (Left of Settings) ===
	Vector2 mp = GetMousePosition();
	int wifiBtnSize = 60;
	int wifiBtnX = (1600 - 110) - wifiBtnSize -
				   20; // anchor relative to global settings button
	int wifiBtnY = 30;
	Rectangle wifiBtnRect = {wifiBtnX, wifiBtnY, wifiBtnSize, wifiBtnSize};
	bool wifiHover = CheckCollisionPointRec(mp, wifiBtnRect);

	// Draw WiFi Icon Button
	DrawRectangleRec(wifiBtnRect, wifiHover ? (Color){60, 20, 0, 200}
											: (Color){40, 10, 0, 200});
	DrawRectangleLinesEx(wifiBtnRect, 2, ORANGE);

	// Draw WiFi Symbol (Arcs)
	Vector2 wifiCenter = {wifiBtnX + wifiBtnSize / 2,
						  wifiBtnY + wifiBtnSize - 15};
	DrawCircleV(wifiCenter, 4, WHITE);				  // Dot
	DrawRing(wifiCenter, 10, 13, 225, 315, 0, WHITE); // Small arc
	DrawRing(wifiCenter, 18, 21, 225, 315, 0, WHITE); // Medium arc
	DrawRing(wifiCenter, 26, 29, 225, 315, 0, WHITE); // Large arc

	if (wifiHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		showConnectionPopup = true;
	}

	// === SELECTED COIN DISPLAY ===
	int centerX = 1600 / 2;
	int centerY = 800 / 2;
	int coinDisplaySize = 500;

	if (assets->coinTextures[player->selectedCoin].id > 0) {
		Rectangle src = {0, 0, assets->coinTextures[player->selectedCoin].width,
						 assets->coinTextures[player->selectedCoin].height};

		Rectangle dest = {centerX - coinDisplaySize / 2,
						  centerY - coinDisplaySize / 2, coinDisplaySize,
						  coinDisplaySize};

		DrawTexturePro(
			assets->coinTextures[player->selectedCoin], src, dest,
			(Vector2){0, 0}, 0,
			(Color){255, 255, 255, (unsigned char)(255 * transitionAlpha)});
	}

	/* === Uniform Buttons === */

	int btnWidth = 500;
	int btnHeight = 350;
	int padding = 40;

	// SHOP bottom-left
	Rectangle shopBtn = {padding, 800 - btnHeight - padding, btnWidth,
						 btnHeight};

	// PLAY bottom-right
	Rectangle playBtn = {1600 - btnWidth - padding, 800 - btnHeight - padding,
						 btnWidth, btnHeight};

	// Reduced center hitboxes (40% of the button size)
	Rectangle shopHitbox = GetCenteredHitbox(shopBtn, 0.4f);
	Rectangle playHitbox = GetCenteredHitbox(playBtn, 0.4f);

	// DRAW SHOP
	if (assets->btnShop.id) {
		Color shopTint =
			CheckCollisionPointRec(mp, shopHitbox)
				? (Color){255, 200, 100, (unsigned char)(255 * transitionAlpha)}
				: (Color){255, 255, 255,
						  (unsigned char)(255 * transitionAlpha)};
		DrawTexturePro(
			assets->btnShop,
			(Rectangle){0, 0, assets->btnShop.width, assets->btnShop.height},
			shopBtn, (Vector2){0, 0}, 0, shopTint);
	} else {
		DrawRectangleLinesEx(
			shopBtn, 3,
			(Color){255, 203, 0, (unsigned char)(255 * transitionAlpha)});
		DrawDoomText(
			assets->doomFont, assets->fontLoaded, "SHOP",
			shopBtn.x + btnWidth / 2 - 60, shopBtn.y + btnHeight / 2 - 25, 50,
			(Color){255, 203, 0, (unsigned char)(255 * transitionAlpha)});
	}

	// DRAW PLAY
	bool canLaunchOnline =
		settings && settings->onlineState == ONLINE_STATE_IDLE;

	if (assets->btnPlay.id) {
		bool hovered = CheckCollisionPointRec(mp, playHitbox);
		Color playTint =
			hovered && canLaunchOnline
				? (Color){255, 200, 100, (unsigned char)(255 * transitionAlpha)}
				: (Color){180, 180, 180,
						  (unsigned char)(220 * transitionAlpha)};
		DrawTexturePro(
			assets->btnPlay,
			(Rectangle){0, 0, assets->btnPlay.width, assets->btnPlay.height},
			playBtn, (Vector2){0, 0}, 0, playTint);
	} else {
		Color border =
			canLaunchOnline
				? (Color){255, 165, 0, (unsigned char)(255 * transitionAlpha)}
				: (Color){120, 120, 120,
						  (unsigned char)(255 * transitionAlpha)};
		DrawRectangleLinesEx(playBtn, 4, border);
		DrawDoomText(assets->doomFont, assets->fontLoaded,
					 canLaunchOnline ? "PLAY" : "MATCHMAKING",
					 playBtn.x + btnWidth / 2 - 110,
					 playBtn.y + btnHeight / 2 - 30, 60, border);
	}

	// Button interactions using the reduced hitboxes
	if (canLaunchOnline && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
		CheckCollisionPointRec(mp, playHitbox)) {
		settings->targetScreen = SCREEN_LOADING;
		settings->isTransitioning = true;
		*loadingProgress = 0.0f;
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

	const char* statusText = settings->onlineError[0] ? settings->onlineError
													  : settings->onlineStatus;
	if (statusText[0] != '\0') {
		DrawDoomText(assets->doomFont, assets->fontLoaded, statusText,
					 1600 / 2 - 350, 800 - 60, 28,
					 settings->onlineError[0] ? RED : ORANGE);
	}

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
		CheckCollisionPointRec(mp, shopHitbox)) {
		settings->targetScreen = SCREEN_SHOP;
		settings->isTransitioning = true;
	}

	// === CONNECTION POPUP (Drawn Last for Z-Order) ===
	if (showConnectionPopup) {
		// Dim background
		DrawRectangle(0, 0, 1600, 800, (Color){0, 0, 0, 200});

		int connW = 420;
		int connH = 280; // Increased height for close button
		int connX = (1600 - connW) / 2;
		int connY = (800 - connH) / 2;

		DrawRectangle(connX, connY, connW, connH, (Color){15, 5, 5, 255});
		DrawRectangleLinesEx((Rectangle){connX, connY, connW, connH}, 3,
							 (Color){255, 69, 0, 255});

		// Close Button (X)
		Rectangle closeBtn = {connX + connW - 30, connY + 5, 25, 25};
		bool closeHover = CheckCollisionPointRec(mp, closeBtn);
		DrawRectangleLinesEx(closeBtn, 2, closeHover ? RED : GRAY);
		DrawText("X", closeBtn.x + 5, closeBtn.y + 2, 20,
				 closeHover ? RED : GRAY);
		if (closeHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			showConnectionPopup = false;
		}

		// Input Fields
		const char* labels[] = {"Pseudo:", "IP:", "Port:"};
		char* buffers[] = {tempUsername, tempIP, tempPort};
		int fieldY = connY + 40;

		for (int i = 0; i < 3; i++) {
			DrawDoomText(assets->eternalFont, assets->fontLoaded, labels[i],
						 connX + 20, fieldY, 24, WHITE);

			Rectangle fieldRect = {connX + 120, fieldY - 5, 280, 30};
			bool isFocused = (activeInput == i);

			DrawRectangleRec(fieldRect, (Color){30, 10, 10, 255});
			DrawRectangleLinesEx(fieldRect, 2, isFocused ? YELLOW : DARKGRAY);

			if (CheckCollisionPointRec(mp, fieldRect) &&
				IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
				activeInput = i;
			}

			DrawText(buffers[i], fieldRect.x + 5, fieldRect.y + 5, 20, WHITE);

			if (isFocused && (int)(GetTime() * 2) % 2 == 0) {
				DrawText("_", fieldRect.x + 5 + MeasureText(buffers[i], 20),
						 fieldRect.y + 5, 20, YELLOW);
			}

			fieldY += 50;
		}

		// Connect Button
		Rectangle connectBtn = {connX + 20, connY + 210, connW - 40, 50};
		bool btnHover = CheckCollisionPointRec(mp, connectBtn);

		DrawRectangleRec(connectBtn, btnHover ? (Color){200, 60, 0, 255}
											  : (Color){150, 40, 0, 255});
		DrawRectangleLinesEx(connectBtn, 2, ORANGE);
		DrawDoomText(assets->eternalFont, assets->fontLoaded, "SE CONNECTER",
					 connectBtn.x + (connectBtn.width - 180) / 2,
					 connectBtn.y + 10, 30, WHITE);

		if (btnHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			// Save inputs to player struct
			strncpy(player->username, tempUsername,
					sizeof(player->username) - 1);
			strncpy(player->serverIP, tempIP, sizeof(player->serverIP) - 1);
			player->serverPort = atoi(tempPort);

			// Trigger connection
			settings->onlineConnectOnly = true;
			settings->onlineMatchRequested = true;
			settings->onlineState = ONLINE_STATE_CONNECTING;
			settings->onlineMatchActive = false;
			settings->onlineMatchReady = false;
			settings->onlineMatchCancelRequested = false;
			settings->onlineError[0] = '\0';
			snprintf(settings->onlineStatus, sizeof(settings->onlineStatus),
					 "Connecting...");

			showConnectionPopup = false;
		}
	}
}
