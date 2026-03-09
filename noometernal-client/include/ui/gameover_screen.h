/**
 * @file gameover_screen.h
 * @brief Defeat/game over screen with statistics.
 *
 * Displays match results when the player loses, showing statistics and
 * options to retry or return to menu.
 */

#ifndef GAMEOVER_SCREEN_H
#define GAMEOVER_SCREEN_H

#include "types.h"

/**
 * @brief Render the game over (defeat) screen.
 *
 * Shows defeat message, match statistics, and buttons to retry or
 * return to main menu.
 *
 * @param assets Pointer to assets (game over textures, fonts).
 * @param player Pointer to player data for stats display.
 * @param currentScreen Pointer to current screen for navigation.
 * @param loadingProgress Pointer to loading progress (unused).
 * @param settings Game settings for UI and audio preferences.
 */
void DrawGameOverScreen(Assets* assets, PlayerData* player,
						GameScreen* currentScreen, float* loadingProgress,
						GameSettings* settings);

#endif // GAMEOVER_SCREEN_H
