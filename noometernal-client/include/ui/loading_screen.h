/**
 * @file loading_screen.h
 * @brief Loading/matchmaking screen with progress indication.
 *
 * Displays a loading screen with progress bar and status messages for asset
 * loading, matchmaking wait, and online connection establishment.
 */

#ifndef LOADING_SCREEN_H
#define LOADING_SCREEN_H

#include "types.h"

/**
 * @brief Render the loading/matchmaking screen.
 *
 * Shows loading progress and status messages for network initialization and
 * matchmaking. Displays live network status updates and connection state.
 *
 * @param assets Pointer to shared assets (textures, fonts).
 * @param player Pointer to player data for context.
 * @param currentScreen Pointer to current screen for state management.
 * @param loadingProgress Pointer to progress value [0.0, 1.0].
 * @param settings Game settings including online status and error messages.
 */
void DrawLoadingScreen(Assets* assets, PlayerData* player,
					   GameScreen* currentScreen, float* loadingProgress,
					   GameSettings* settings);

#endif // LOADING_SCREEN_H