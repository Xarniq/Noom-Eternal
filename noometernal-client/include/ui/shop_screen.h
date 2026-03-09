/**
 * @file shop_screen.h
 * @brief In-game shop/marketplace screen rendering.
 *
 * Renders the shop interface where players can purchase coins, skins, and
 * cosmetics using earned currency.
 */

#ifndef SHOP_SCREEN_H
#define SHOP_SCREEN_H

#include "types.h"

/**
 * @brief Render the shop screen for purchasing coins and skins.
 *
 * Displays available items for purchase, their prices, and the player's current
 * currency. Handles purchasing logic, inventory updates, and network
 * synchronization of shop actions.
 *
 * @param assets Pointer to loaded assets (item textures, fonts).
 * @param player Pointer to player data (currency, inventory).
 * @param currentScreen Pointer to current screen for navigation.
 * @param coinPrices Array of 6 coin prices (indices 0-5).
 * @param currentCoin Pointer to selected coin index (0-5).
 * @param settings Game settings for UI behavior and network state.
 */
void DrawShopScreen(Assets* assets, PlayerData* player,
					GameScreen* currentScreen, int coinPrices[6],
					int* currentCoin, GameSettings* settings);

#endif // SHOP_SCREEN_H
