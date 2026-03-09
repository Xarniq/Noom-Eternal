/**
 * @file victory_screen.h
 * @brief Victory screen displaying match results and rewards.
 *
 * Shows player victory, rewards earned (XP, souls), and options to return
 * to menu or play again.
 */

#ifndef VICTORY_SCREEN_H
#define VICTORY_SCREEN_H

#include "types.h"

/**
 * @brief Render the victory screen.
 *
 * Displays victory message, earned rewards (XP, souls, progression),
 * and buttons for continuing or returning to menu.
 *
 * @param assets Pointer to assets (victory textures, fonts).
 * @param player Pointer to player data for reward display.
 * @param currentScreen Pointer to current screen for navigation.
 * @param loadingProgress Pointer to loading progress (unused).
 * @param settings Game settings for UI and audio preferences.
 */
void DrawVictoryScreen(Assets* assets, PlayerData* player,
					   GameScreen* currentScreen, float* loadingProgress,
					   GameSettings* settings);

#endif // VICTORY_SCREEN_H