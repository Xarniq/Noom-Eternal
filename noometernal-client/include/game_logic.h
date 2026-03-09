/**
 * @file game_logic.h
 * @brief Core game logic for board initialization and state management.
 *
 * Provides functions to initialize the game board, reset game state, and
 * maintain the logical representation of an active or completed match.
 */

#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "types.h"

/**
 * @brief Initialize the game board to its starting state.
 *
 * Sets up an empty board with no active pieces and resets player/game state
 * variables. Called at the start of a new match or when the server sends
 * initial game state.
 *
 * @param game Pointer to the GameState structure to initialize.
 */
void InitGameBoard(GameState* game);

/**
 * @brief Reset the game to its initial state.
 *
 * Clears the board and resets all game variables. Equivalent to calling
 * InitGameBoard(); used when transitioning between matches or clearing state
 * between game sessions.
 *
 * @param game Pointer to the GameState structure to reset.
 */
void ResetGame(GameState* game);

#endif // GAME_LOGIC_H
