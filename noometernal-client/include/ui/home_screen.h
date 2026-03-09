/**
 * @file home_screen.h
 * @brief Main menu and lobby screen rendering.
 *
 * Renders the home screen interface with options for playing, accessing the
 * shop, viewing player stats, and initiating online matchmaking.
 */

#ifndef HOME_SCREEN_H
#define HOME_SCREEN_H

#include "types.h"

/**
 * @brief Render the home/main menu screen.
 *
 * Displays the menu interface with the player's profile information, level,
 * currency, and buttons to start game, visit shop, or enter online matchmaking.
 * Handles all input for menu navigation and selection.
 *
 * @param assets Pointer to shared assets for textures and fonts.
 * @param player Player data for profile display (level, souls, username).
 * @param currentScreen Pointer to current screen for navigation handling.
 * @param loadingProgress Pointer to loading progress value for async
 * operations.
 * @param transitionAlpha Current screen transition alpha for fade effects
 * [0.0, 1.0].
 * @param settings Game settings including online state and navigation flags.
 */
void DrawHomeScreen(Assets* assets, PlayerData* player,
					GameScreen* currentScreen, float* loadingProgress,
					float transitionAlpha, GameSettings* settings);

#endif // HOME_SCREEN_H
