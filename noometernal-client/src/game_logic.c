/**
 * @file game_logic.c
 * @brief Implementation of game board and state initialization.
 *
 * Provides functions to create and reset game boards, initializing the game
 * state for new matches or between rounds.
 */

/**
 * @brief Initialize the game board to its starting state.
 *
 * Sets all board positions to empty (0), resets the current player, clears win
 * condition, and resets the selected slot. Called at match start or after
 * reset.
 *
 * @param game Pointer to the GameState instance to initialize.
 */

/**
 * @brief Reset the game to its initial state.
 *
 * Clears all board state and resets variables. Equivalent to InitGameBoard().
 * Used between matches or when the user requests a fresh start.
 *
 * @param game Pointer to the GameState instance to reset.
 */
#include "game_logic.h"
#include "types.h"

// Initialize the game board
void InitGameBoard(GameState* game) {
	// For client-side only rendering, we might initialize to empty or wait for
	// server state. Setting to 0.
	for (int i = 0; i < 12; i++) {
		game->board[i] = 0;
	}

	game->currentPlayer = 0; // Waiting for server
	game->gameOver = false;
	game->winner = 0;
	game->selectedSlot = -1;
}

// Reset the game
void ResetGame(GameState* game) { InitGameBoard(game); }
