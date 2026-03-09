/**
 * @file game_screen.h
 * @brief Main gameplay screen rendering.
 *
 * Renders the game board, player HUD, opponent state, and handles in-game
 * interactions.
 */

#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include "types.h"

/**
 * @brief Render the main gameplay screen.
 *
 * Draws the game board with all active coins, player HUD elements (level, XP,
 * souls), opponent status, and processes player input for move selection and
 * execution. Updates game state based on player interactions.
 *
 * @param assets Shared asset collection for textures, fonts, and audio.
 * @param player Player data including level and currency for HUD display.
 * @param currentScreen Pointer to current screen state for potential
 * transitions.
 * @param game Core game state containing board, coins, and turn information.
 * @param settings Game-wide settings for audio and visual preferences.
 */
void DrawGameScreen(Assets* assets, PlayerData* player,
					GameScreen* currentScreen, GameState* game,
					GameSettings* settings);

#endif // GAME_SCREEN_H
