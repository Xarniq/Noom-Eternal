/**
 * @file game_screen.c
 * @brief Implementation of the main gameplay screen rendering.
 *
 * Renders the game board with 12 slots, player coins, opponent coins, HUD
 * elements, and handles all input for move selection and execution. Includes
 * visual feedback for slot hover, selection, and move validation.
 */

/**
 * @brief Render the game screen with board, pieces, and HUD.
 *
 * Draws the game board with all interactive slots, coins, player status
 * information, turn indicators, and game control buttons. Processes mouse input
 * for move selection and sends valid moves to the network client.
 *
 * @param assets Pointer to loaded assets (textures, fonts).
 * @param player Pointer to player data for HUD display.
 * @param currentScreen Pointer to current screen for navigation.
 * @param game Pointer to game state containing board and turn information.
 * @param settings Pointer to game settings.
 */
#include "ui/game_screen.h"
#include "game_logic.h"
#include "ui/ui_utils.h"

#include "network_client.h"
#include <raylib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================================
   ========================= GAME SCREEN =======================
   ============================================================ */

/**
 * @brief Validate a move on the game board from client-side perspective.
 *
 * Checks that the source position contains a coin, destination is empty,
 * destination index is less than source (moving toward position 0), and the
 * destination is not blocked by a coin to its left. Used for client-side
 * validation before sending to server.
 *
 * @param game Pointer to GameState containing the board.
 * @param fromIndex Index of the coin to move (0-11).
 * @param toIndex Destination index (0-11).
 * @return true if the move is valid, false otherwise.
 */
static bool ClientMoveIsValid(const GameState* game, int fromIndex,
							  int toIndex) {
	if (!game) {
		return false;
	}

	if (fromIndex < 0 || fromIndex >= 12 || toIndex < 0 || toIndex >= 12) {
		return false;
	}

	if (game->board[fromIndex] == 0 || game->board[toIndex] != 0) {
		return false;
	}

	if (toIndex >= fromIndex) {
		return false;
	}

	int prevCoin = -1;
	for (int pos = fromIndex - 1; pos >= 0; --pos) {
		if (game->board[pos] != 0) {
			prevCoin = pos;
			break;
		}
	}

	if (prevCoin >= 0 && toIndex <= prevCoin) {
		return false;
	}

	return true;
}

void DrawGameScreen(Assets* assets, PlayerData* player,
					GameScreen* currentScreen, GameState* game,
					GameSettings* settings) {
	// Safety guard to avoid null dereference when the game state is missing
	if (!game)
		return;

	// Background texture scaled the same way as the home screen
	if (assets->gameBackground.id > 0) {
		float scaleX = (float)1600 / assets->gameBackground.width;
		float scaleY = (float)800 / assets->gameBackground.height;
		float scale = (scaleX > scaleY) ? scaleX : scaleY;

		int drawWidth = (int)(assets->gameBackground.width * scale);
		int drawHeight = (int)(assets->gameBackground.height * scale);
		int drawX = (1600 - drawWidth) / 2;
		int drawY = (800 - drawHeight) / 2;

		DrawTexturePro(assets->gameBackground,
					   (Rectangle){0, 0, (float)assets->gameBackground.width,
								   (float)assets->gameBackground.height},
					   (Rectangle){(float)drawX, (float)drawY, (float)drawWidth,
								   (float)drawHeight},
					   (Vector2){0, 0}, 0.0f, WHITE);

		DrawRectangle(0, 0, 1600, 800, (Color){0, 0, 0, 150});
	} else {
		DrawRectangle(0, 0, 1600, 800, (Color){0, 0, 0, 50});
	}

	// === Board without the red frame ===
	int boardY = 150;
	float slotRatio = 1.40f;
	int slotSize = (int)(130 * slotRatio);
	int slotSpacing =
		-50; // Negative spacing to overlap and hide harsh texture borders
	int totalWidth = 12 * slotSize + 11 * slotSpacing;
	int startX = (1600 - totalWidth) / 2;

	Vector2 mp = GetMousePosition();
	bool onlineMatch = settings && settings->onlineMatchActive;
	NetworkClient* client = onlineMatch ? settings->netClient : NULL;
	bool ourTurnOnline =
		onlineMatch && client && NetworkClient_IsOurTurn(client);
	bool canInteract =
		!game->gameOver && ((onlineMatch && ourTurnOnline) ||
							(!onlineMatch && game->currentPlayer == 1));

	for (int i = 0; i < 12; i++) {

		int x = startX + i * (slotSize + slotSpacing);

		// Use carres.png texture for slots when available
		if (assets->slotTexture.id > 0) {
			Color slotTint = WHITE;
			if (game && game->selectedSlot == i) {
				slotTint = (Color){255, 150, 100, 255};
			}

			DrawTexturePro(assets->slotTexture,
						   (Rectangle){0, 0, assets->slotTexture.width,
									   assets->slotTexture.height},
						   (Rectangle){x, boardY, slotSize, slotSize},
						   (Vector2){0, 0}, 0.0f, slotTint);
		} else {
			// Fallback style when the texture is not available
			Color slotColor = (Color){20, 20, 30, 255};
			if (game && game->selectedSlot == i) {
				slotColor = (Color){100, 50, 0, 255};
			}

			// Slot with faux 3D shading
			DrawRectangle(x + 3, boardY + 3, slotSize, slotSize,
						  (Color){0, 0, 0, 150});
			DrawRectangleGradientV(x, boardY, slotSize, slotSize, slotColor,
								   (Color){slotColor.r / 2, slotColor.g / 2,
										   slotColor.b / 2, 255});

			// Inner golden border
			DrawRectangleLinesEx(
				(Rectangle){x + 2, boardY + 2, slotSize - 4, slotSize - 4}, 2,
				(Color){100, 80, 40, 200});
			// Outer orange border
			DrawRectangleLinesEx((Rectangle){x, boardY, slotSize, slotSize}, 3,
								 (Color){180, 80, 20, 255});
		}

		Rectangle slotRect = {x, boardY, slotSize, slotSize};
		bool isHoveredRaw = CheckCollisionPointRec(mp, slotRect);
		bool isHovered = isHoveredRaw && canInteract;

		if (game && isHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
			canInteract) {
			if (game->selectedSlot == -1) {
				if (game->board[i] != 0)
					game->selectedSlot = i;
			} else {
				if (onlineMatch) {
					if (client && game->board[i] == 0) {
						if (ClientMoveIsValid(game, game->selectedSlot, i)) {
							if (!NetworkClient_SendMove(
									client, (uint8_t)game->selectedSlot,
									(uint8_t)i)) {
								if (settings) {
									snprintf(settings->onlineError,
											 sizeof(settings->onlineError),
											 "%s",
											 NetworkClient_GetError(client));
									settings->onlineStatus[0] = '\0';
								}
							} else if (settings) {
								settings->onlineError[0] = '\0';
								settings->onlineStatus[0] = '\0';
							}
						} else if (settings) {
							snprintf(settings->onlineError,
									 sizeof(settings->onlineError),
									 "Impossible Move");
							settings->onlineStatus[0] = '\0';
						}
						game->selectedSlot = -1;
					} else if (game->board[i] != 0) {
						game->selectedSlot = i;
					} else {
						game->selectedSlot = -1;
					}
				} else {
					// Offline mode is deprecated in this branch; logic lives on
					// the server.
					game->selectedSlot = -1;
				}
			}
		}

		if (game && game->board[i] != 0) {

			// Server uses simple 1/0 logic, so map any non-zero value to a
			// default coin index unless we have specific coin IDs. For now, use
			// the player's selected coin style.
			int coinIndex =
				0; // Default to first coin style if board just has '1'

			// If board has specific values (e.g. 1-6), use them.
			if (abs(game->board[i]) > 0) {
				// If the server sends 1 for all coins, we just render coin 0.
				// If the server sent 1..6, we could use abs(game->board[i])
				// - 1. But current server logic sends 1. Let's just use the
				// player's selected coin preference for rendering.
				coinIndex = player->selectedCoin;
			}

			if (coinIndex >= 0 && coinIndex < 6 &&
				assets->coinTextures[coinIndex].id > 0) {
				float coinSize = 90 * slotRatio;

				// Highlight the coin when hovered and the match is not over
				Color coinTint = WHITE;
				if (isHovered && !game->gameOver) {
					coinTint =
						(Color){255, 255, 200,
								255}; // Light golden highlight while hovering
				}

				DrawTexturePro(
					assets->coinTextures[coinIndex],
					(Rectangle){0, 0, assets->coinTextures[coinIndex].width,
								assets->coinTextures[coinIndex].height},
					(Rectangle){x + slotSize / 2 - coinSize / 2,
								boardY + slotSize / 2 - coinSize / 2, coinSize,
								coinSize},
					(Vector2){0, 0}, 0.0f, coinTint);
			}
		}
	}

	// === Status message displayed with Doom styled card ===
	int msgCardY = 340;
	int msgCardW = 1200;
	int msgCardH = 100;
	int msgCardX = (1600 - msgCardW) / 2;

	if (!game->gameOver) {
		DrawRectangle(msgCardX, msgCardY, msgCardW, msgCardH,
					  (Color){15, 5, 5, 230});
		DrawRectangleGradientV(msgCardX, msgCardY, msgCardW, msgCardH,
							   (Color){40, 10, 0, 180}, (Color){10, 5, 5, 230});
		DrawRectangleLinesEx(
			(Rectangle){msgCardX, msgCardY, msgCardW, msgCardH}, 4,
			(Color){255, 69, 0, 255});

		const char* statusText = NULL;
		Color statusColor = ORANGE;

		if (settings && settings->onlineError[0]) {
			statusText = settings->onlineError;
			statusColor = RED;
		} else if (settings && settings->onlineStatus[0]) {
			statusText = settings->onlineStatus;
		} else if (onlineMatch) {
			statusText =
				ourTurnOnline ? "YOUR TURN" : "Waiting for opponent to play...";
		} else if (game->currentPlayer == 1) {
			statusText = "YOUR TURN";
			statusColor = (Color){255, 200, 0, 255};
		} else {
			statusText = "YOUR TURN...";
		}

		DrawDoomText(assets->eternalFont, assets->eternalFontLoaded,
					 statusText ? statusText : "", msgCardX + 40, msgCardY + 30,
					 32, statusColor);
	} else {
		// Game Over message with larger card
		DrawRectangle(msgCardX, msgCardY, msgCardW, msgCardH,
					  (Color){15, 5, 5, 230});
		DrawRectangleGradientV(msgCardX, msgCardY, msgCardW, msgCardH,
							   (Color){40, 10, 0, 180}, (Color){10, 5, 5, 230});
		DrawRectangleLinesEx(
			(Rectangle){msgCardX, msgCardY, msgCardW, msgCardH}, 5,
			(game->winner == 1) ? GREEN : RED);

		if (game->winner == 1) {
			DrawDoomText(assets->doomFont, assets->fontLoaded, "VICTORY!",
						 msgCardX + msgCardW / 2 - 130 + 3, msgCardY + 23, 56,
						 (Color){0, 60, 0, 255});
			DrawDoomText(assets->doomFont, assets->fontLoaded, "VICTORY!",
						 msgCardX + msgCardW / 2 - 130, msgCardY + 20, 56,
						 GREEN);
		} else {
			DrawDoomText(assets->doomFont, assets->fontLoaded, "DEFEAT!",
						 msgCardX + msgCardW / 2 - 110 + 3, msgCardY + 23, 56,
						 (Color){60, 0, 0, 255});
			DrawDoomText(assets->doomFont, assets->fontLoaded, "DEFEAT!",
						 msgCardX + msgCardW / 2 - 110, msgCardY + 20, 56, RED);
		}
	}

	Vector2 mousePos = GetMousePosition();

	float ratio = 0.7f;
	int btnWidth = (int)(375 * ratio);
	int btnHeight = (int)(350 * ratio);
	int padding = 40;

	Rectangle restartBtnRect = {1600 / 2 - btnWidth / 2,
								800 - btnHeight - padding, btnWidth, btnHeight};
	Rectangle restartHitbox = GetCenteredHitbox(restartBtnRect, 0.4f);

	if (!onlineMatch && assets->btnRestart.id > 0) {
		Color restartTint = CheckCollisionPointRec(mousePos, restartHitbox)
								? (Color){255, 200, 100, 255}
								: WHITE;
		DrawTexturePro(assets->btnRestart,
					   (Rectangle){0, 0, assets->btnRestart.width,
								   assets->btnRestart.height},
					   restartBtnRect, (Vector2){0, 0}, 0.0f, restartTint);

		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
			CheckCollisionPointRec(mousePos, restartHitbox)) {
			ResetGame(game);
		}
	}

	Rectangle menuBtnRect = {1600 - btnWidth - padding,
							 800 - btnHeight - padding, btnWidth, btnHeight};
	Rectangle menuHitbox = GetCenteredHitbox(menuBtnRect, 0.4f);

	if (assets->btnMenu.id > 0) {
		Color menuTint = CheckCollisionPointRec(mousePos, menuHitbox)
							 ? (Color){255, 200, 100, 255}
							 : WHITE;
		DrawTexturePro(
			assets->btnMenu,
			(Rectangle){0, 0, assets->btnMenu.width, assets->btnMenu.height},
			menuBtnRect, (Vector2){0, 0}, 0.0f, menuTint);

		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
			CheckCollisionPointRec(mousePos, menuHitbox)) {
			if (onlineMatch) {
				settings->onlineMatchCancelRequested = true;
				snprintf(settings->onlineStatus, sizeof(settings->onlineStatus),
						 "Retour au menu...");
			}
			*currentScreen = SCREEN_HOME;
		}
	}
}
