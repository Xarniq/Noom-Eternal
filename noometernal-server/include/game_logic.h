/**
 * @file game_logic.h
 * @brief Core game rules and board state management.
 *
 * This module implements the gameplay mechanics of a two-player coin-sliding
 * game. The board consists of 12 positions where players alternate moving coins
 * strictly leftward. The game ends when no legal moves remain (all coins are
 * blocked from further movement).
 *
 * Game Rules:
 * - Board size: 12 cells
 * - Each player has turns to move one coin
 * - Coins can only move left (strictly decreasing position index)
 * - A coin cannot jump over another coin (cannot pass previous coin)
 * - A move to an empty cell must be to the right of the nearest left coin
 * - Game ends when no coins can move left
 * - Last player to move wins
 *
 * @author NoomEternal Development Team
 * @version 1.0
 * @date 2026-01-08
 */

#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <stdint.h>

/**
 * @brief Generate a random game board configuration.
 *
 * Creates a new board with 5 coins placed at random positions among 12 cells.
 * The returned array is dynamically allocated and must be freed by the caller.
 *
 * @return Pointer to a newly allocated array of 12 integers where:
 *         - 1 represents a coin
 *         - 0 represents an empty space
 *         The array will contain exactly 5 coins at random positions.
 *
 * @note Memory allocation: Returns malloc'd memory. Caller must free().
 * @note Randomness: Uses srand()/rand() from stdlib. Seed once per process.
 *
 * @see game_init_board()
 */
int* random_board_generator(void);

/**
 * @brief Initialize the game board with a valid starting configuration.
 *
 * Populates a 12-cell board with coins in a configuration that guarantees
 * at least one legal move is available. The board is typically initialized
 * once per match before play begins.
 *
 * Algorithm:
 * 1. Attempts to generate random board configurations
 * 2. Validates that at least one move is legal
 * 3. Falls back to a default (evenly-spaced coins) if generation fails
 *
 * @param[out] board Pointer to an array of uint8_t with size 12.
 *                   Will be populated with 1 (coin) or 0 (empty).
 *                   Must be non-NULL.
 *
 * @return void. The board is modified in place.
 *
 * @note The caller must provide a pre-allocated 12-cell array.
 * @note Ensures the game is always playable from the start.
 *
 * @see random_board_generator(), game_is_finished()
 */
void game_init_board(uint8_t* board);

/**
 * @brief Validate a move according to game rules.
 *
 * Checks if moving a coin from position coin_index to coin_pos is legal.
 * The validation includes boundary checks, source/destination state, movement
 * direction, and blocking coin constraints.
 *
 * Move is valid if and only if ALL conditions are met:
 * 1. Both indices are in range [0, 11]
 * 2. Movement is strictly leftward: coin_pos < coin_index
 * 3. Source position contains a coin (board[coin_index] == 1)
 * 4. Destination position is empty (board[coin_pos] == 0)
 * 5. No coin blocks the path: destination >= nearest coin to the left
 *
 * @param[in] board Pointer to the board array of size 12. Must be non-NULL.
 *                  board[i] == 1 indicates a coin; board[i] == 0 indicates
 * empty.
 * @param[in] coin_index Position of the source coin (0-11).
 * @param[in] coin_pos Target position for the move (0-11).
 *
 * @return 1 if the move is legal.
 * @return 0 if the move is illegal or board is NULL.
 *
 * @note This function does NOT modify the board; use game_apply_move() for
 * that.
 * @note Blocking rule: If coin_index > 0 and there's a coin at position
 * prev_coin, then coin_pos must be > prev_coin.
 *
 * @see game_apply_move(), game_is_finished()
 */
int game_is_move_valid(const uint8_t* board, uint8_t coin_index,
					   uint8_t coin_pos);

/**
 * @brief Apply a validated move to the board state.
 *
 * Executes a move by removing the coin from coin_index and placing it at
 * coin_pos. This function assumes the move has been validated and does NOT
 * perform any checks. Use game_is_move_valid() before calling this function.
 *
 * @param[in,out] board Pointer to the board array of size 12. Must be non-NULL.
 *                      Modified in place.
 * @param[in] coin_index Source position of the coin (0-11).
 * @param[in] coin_pos Target position for the move (0-11).
 *
 * @return void. The board is modified directly.
 *
 * @warning This function does NOT validate the move. Caller must ensure
 *          game_is_move_valid() returns true before calling this.
 * @warning Behavior is undefined if preconditions are violated.
 *
 * @see game_is_move_valid()
 */
void game_apply_move(uint8_t* board, uint8_t coin_index, uint8_t coin_pos);

/**
 * @brief Check if the game has reached a terminal state (no legal moves
 * remain).
 *
 * Scans the board to determine if any player can make a legal move. A game is
 * finished when all remaining coins are to the right of all empty spaces,
 * making leftward movement impossible.
 *
 * Algorithm:
 * - Finds the rightmost empty cell
 * - Checks if any coin exists to the right of that cell
 * - If yes: at least one coin can move left → not finished
 * - If no: all coins are blocked → finished
 *
 * @param[in] board Pointer to the board array of size 12. Must be non-NULL.
 *                  board[i] == 1 indicates a coin; board[i] == 0 indicates
 * empty.
 *
 * @return 1 if the board is in a finished state (no legal moves available).
 * @return 0 if at least one legal move exists.
 *
 * @note This function is called after every move to detect game termination.
 * @note A board with all coins at positions [0..n-1] and empty cells at [n..11]
 *       is finished (all coins are leftmost).
 *
 * @see game_is_move_valid()
 */
int game_is_finished(const uint8_t* board);

#endif /* GAME_LOGIC_H */
