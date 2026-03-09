/**
 * @file shop_screen.c
 * @brief Implementation of the shop/marketplace screen.
 *
 * Renders the in-game shop interface for purchasing coins and cosmetic skins.
 * Provides visual feedback for item selection, ownership status, and purchase
 * actions.
 */

/**
 * @brief Render the shop screen with coin selection and purchase interface.
 *
 * Displays available coins/skins with pricing, handles currency transactions,
 * and manages cosmetic equipping. Shows player's current balance and item
 * ownership.
 *
 * @param assets Pointer to loaded assets (coin textures, fonts).
 * @param player Pointer to player data (currency, owned items).
 * @param currentScreen Pointer to current screen for navigation.
 * @param coinPrices Array of 6 coin prices.
 * @param currentCoin Pointer to selected coin index.
 * @param settings Pointer to game settings for navigation.
 */
#include "ui/shop_screen.h"
#include "network_client.h"
#include "ui/ui_utils.h"
#include <libemap.h>
#include <raylib.h>
#include <stdio.h>

/* ============================================================
   ========================= SHOP SCREEN =======================
   ============================================================ */

void DrawShopScreen(Assets* assets, PlayerData* player,
					GameScreen* currentScreen, int coinPrices[6],
					int* currentCoin, GameSettings* settings) {
	DrawRectangle(0, 0, 1600, 800, (Color){0, 0, 0, 140});

	// BACK BUTTON displayed at the top-left corner
	Vector2 mp = GetMousePosition();
	NetworkClient* netClient = settings ? settings->netClient : NULL;
	NetworkState netState =
		netClient ? NetworkClient_GetState(netClient) : NET_DISCONNECTED;
	bool shopOnline = (netState == NET_CONNECTED);
	Rectangle backB = {30, 30, 300, 280};
	Rectangle backHitbox = GetCenteredHitbox(backB, 0.4f);
	Color backTint = CheckCollisionPointRec(mp, backHitbox)
						 ? (Color){255, 200, 100, 255}
						 : WHITE;
	DrawTexturePro(
		assets->btnBack,
		(Rectangle){0, 0, assets->btnBack.width, assets->btnBack.height}, backB,
		(Vector2){0, 0}, 0, backTint);

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
		CheckCollisionPointRec(mp, backHitbox)) {
		settings->targetScreen = SCREEN_HOME;
		settings->isTransitioning = true;
	}

	if (settings) {
		const char* statusText = settings->onlineError[0]
									 ? settings->onlineError
									 : settings->onlineStatus;
		if (statusText[0] != '\0') {
			DrawDoomText(assets->eternalFont, assets->fontLoaded, statusText,
						 1600 / 2 - 200, 20, 28,
						 settings->onlineError[0] ? RED : ORANGE);
		}

		if (!shopOnline) {
			DrawDoomText(assets->eternalFont, assets->fontLoaded,
						 "Connect via the WiFi button to access the shop",
						 1600 / 2 - 400, 80, 24, LIGHTGRAY);
		}
	}

	DrawDoomText(assets->doomFont, assets->fontLoaded,
				 TextFormat("SOULS: %d", player->souls), 1600 - 350, 40, 52,
				 GOLD);

	int centerX = 1600 / 2;
	int centerY = 450;

	// === CLICKABLE THUMBNAIL GALLERY ===
	int thumbnailSize = 120;
	int thumbnailSpacing = 20;
	int galleryY = 80;
	int totalGalleryWidth = 6 * thumbnailSize + 5 * thumbnailSpacing;
	int galleryStartX = (1600 - totalGalleryWidth) / 2;

	for (int i = 0; i < 6; i++) {
		int thumbX = galleryStartX + i * (thumbnailSize + thumbnailSpacing);
		Rectangle thumbRect = {thumbX, galleryY, thumbnailSize, thumbnailSize};
		Rectangle thumbHitbox = GetCenteredHitbox(thumbRect, 0.85f);

		bool isSelected = (i == *currentCoin);
		bool isHovered = CheckCollisionPointRec(mp, thumbHitbox);
		bool isOwned = player->ownedSkins[i];

		// Thumbnail frame with hover and selection states
		Color borderColor;
		int borderThickness;
		if (isSelected) {
			borderColor = (Color){255, 69, 0, 255}; // Orange DOOM
			borderThickness = 5;
			// Glow effect to emphasize the active selection
			DrawRectangle(thumbX - 3, galleryY - 3, thumbnailSize + 6,
						  thumbnailSize + 6, (Color){255, 100, 0, 100});
		} else if (isHovered) {
			borderColor = (Color){255, 150, 50, 255};
			borderThickness = 4;
		} else {
			borderColor =
				isOwned ? (Color){180, 140, 0, 200} : (Color){100, 80, 60, 200};
			borderThickness = 3;
		}

		// Thumbnail background
		DrawRectangle(thumbX, galleryY, thumbnailSize, thumbnailSize,
					  (Color){20, 15, 10, 230});
		DrawRectangleGradientV(thumbX, galleryY, thumbnailSize, thumbnailSize,
							   (Color){30, 20, 10, 200},
							   (Color){15, 10, 5, 230});

		// Render the mini coin preview
		if (assets->coinTextures[i].id > 0) {
			Color coinTint = isOwned ? WHITE : (Color){80, 80, 80, 200};
			int coinPadding = 10;
			DrawTexturePro(assets->coinTextures[i],
						   (Rectangle){0, 0, assets->coinTextures[i].width,
									   assets->coinTextures[i].height},
						   (Rectangle){thumbX + coinPadding,
									   galleryY + coinPadding,
									   thumbnailSize - 2 * coinPadding,
									   thumbnailSize - 2 * coinPadding},
						   (Vector2){0, 0}, 0, coinTint);
		}

		// "LOCKED" indicator for unowned coins
		if (!isOwned) {
			DrawRectangle(thumbX, galleryY, thumbnailSize, thumbnailSize,
						  (Color){0, 0, 0, 180});
			DrawDoomText(assets->doomFont, assets->fontLoaded, "LOCKED",
						 thumbX + 17 + (i * 2),
						 galleryY + thumbnailSize / 2 - 15, 30,
						 (Color){200, 50, 50, 255});
		}

		// "EQUIPPED" indicator for the currently equipped coin
		if (player->selectedCoin == i) {
			DrawRectangle(thumbX, galleryY + thumbnailSize - 25, thumbnailSize,
						  25, (Color){0, 150, 0, 200});
			DrawDoomText(assets->doomFont, assets->fontLoaded, "EQUIPPED",
						 thumbX + 15, galleryY + thumbnailSize - 22, 25, WHITE);
		}

		// Border outline
		DrawRectangleLinesEx(thumbRect, borderThickness, borderColor);

		// Click to select
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
			CheckCollisionPointRec(mp, thumbHitbox)) {
			*currentCoin = i;
		}
	}

	// Large center coin preview (500x500)
	int coinSize = 500;
	Rectangle coinRect = {centerX - coinSize / 2, centerY - coinSize / 2,
						  coinSize, coinSize};

	if (assets->coinTextures[*currentCoin].id > 0) {
		DrawTexturePro(assets->coinTextures[*currentCoin],
					   (Rectangle){0, 0,
								   assets->coinTextures[*currentCoin].width,
								   assets->coinTextures[*currentCoin].height},
					   coinRect, (Vector2){0, 0}, 0, WHITE);
	}

	// Buttons and price labels
	int btnWidth = 400;
	int btnHeight = 300;
	int btnY = 800 - btnHeight;

	if (player->ownedSkins[*currentCoin]) {
		if (player->selectedCoin == *currentCoin) {
			// EQUIPPED message shown when the coin is already active
			DrawDoomText(assets->doomFont, assets->fontLoaded, "EQUIPPED",
						 centerX - 70, btnY + 150, 64, GREEN);
		} else {
			// EQUIP button rendering
			Rectangle equipBtn = {centerX - btnWidth / 2, btnY, btnWidth,
								  btnHeight};
			Rectangle equipHitbox = GetCenteredHitbox(equipBtn, 0.4f);
			Color equipTint = CheckCollisionPointRec(mp, equipHitbox)
								  ? (Color){255, 200, 100, 255}
								  : WHITE;

			if (assets->btnSkin.id > 0) {
				DrawTexturePro(assets->btnSkin,
							   (Rectangle){0, 0, assets->btnSkin.width,
										   assets->btnSkin.height},
							   equipBtn, (Vector2){0, 0}, 0, equipTint);
			} else {
				DrawRectangle(equipBtn.x, equipBtn.y, equipBtn.width,
							  equipBtn.height, (Color){0, 100, 0, 200});
				DrawDoomText(assets->doomFont, assets->fontLoaded, "EQUIP",
							 centerX - 80, btnY + 70, 48, WHITE);
			}

			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
				CheckCollisionPointRec(mp, equipHitbox)) {
				if (shopOnline && netClient) {
					if (!NetworkClient_SendShopAction(netClient,
													  EMAP_SHOP_ACTION_EQUIP,
													  (uint8_t)*currentCoin)) {
						if (settings) {
							snprintf(settings->onlineError,
									 sizeof(settings->onlineError), "%s",
									 NetworkClient_GetError(netClient));
							settings->onlineStatus[0] = '\0';
						}
					} else if (settings) {
						settings->onlineError[0] = '\0';
						settings->onlineStatus[0] = '\0';
					}
				} else if (settings) {
					snprintf(settings->onlineError,
							 sizeof(settings->onlineError),
							 "Connect to equip skins");
					settings->onlineStatus[0] = '\0';
				}
			}
		}
	} else {
		// Display the price label above the BUY button
		DrawDoomText(assets->doomFont, assets->fontLoaded,
					 TextFormat("%d SOULS", coinPrices[*currentCoin]),
					 centerX - 70, btnY - 250, 52, GOLD);

		// BUY button rendering
		Rectangle buyBtn = {centerX - btnWidth / 2, btnY, btnWidth, btnHeight};
		Rectangle buyHitbox = GetCenteredHitbox(buyBtn, 0.4f);
		bool canAfford = player->souls >= coinPrices[*currentCoin];
		bool canSendPurchase = shopOnline && netClient && canAfford;
		Color buyTint = (Color){120, 120, 120, 255};

		if (canSendPurchase) {
			buyTint = CheckCollisionPointRec(mp, buyHitbox)
						  ? (Color){255, 200, 100, 255}
						  : WHITE;
		}

		if (assets->btnBuy.id > 0) {
			DrawTexturePro(
				assets->btnBuy,
				(Rectangle){0, 0, assets->btnBuy.width, assets->btnBuy.height},
				buyBtn, (Vector2){0, 0}, 0, buyTint);
		} else {
			DrawRectangle(buyBtn.x, buyBtn.y, buyBtn.width, buyBtn.height,
						  canSendPurchase ? (Color){200, 100, 0, 200}
										  : (Color){60, 60, 60, 200});
			DrawDoomText(assets->doomFont, assets->fontLoaded, "BUY",
						 centerX - 50, btnY + 70, 48,
						 canSendPurchase ? WHITE : (Color){100, 100, 100, 255});
		}

		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
			CheckCollisionPointRec(mp, buyHitbox)) {
			if (canSendPurchase) {
				if (!NetworkClient_SendShopAction(netClient,
												  EMAP_SHOP_ACTION_PURCHASE,
												  (uint8_t)*currentCoin)) {
					if (settings) {
						snprintf(settings->onlineError,
								 sizeof(settings->onlineError), "%s",
								 NetworkClient_GetError(netClient));
						settings->onlineStatus[0] = '\0';
					}
				} else if (settings) {
					settings->onlineError[0] = '\0';
					settings->onlineStatus[0] = '\0';
				}
			} else if (settings) {
				if (!shopOnline) {
					snprintf(settings->onlineError,
							 sizeof(settings->onlineError),
							 "Connect to buy skins");
				} else if (!canAfford) {
					snprintf(settings->onlineError,
							 sizeof(settings->onlineError), "Not enough souls");
				} else {
					snprintf(settings->onlineError,
							 sizeof(settings->onlineError),
							 "Action not available");
				}
				settings->onlineStatus[0] = '\0';
			}
		}
	}
}
