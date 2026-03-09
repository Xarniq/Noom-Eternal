/**
 * @file game_logic.c
 * @brief Implementation of game rules and board state management.
 *
 * Core gameplay engine for a two-player coin-sliding game. Implements random
 * board generation, move validation against strict game rules, and terminal
 * state detection.
 */

#include "game_logic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * @brief Initialize the random number generator once per process.
 *
 * Uses static flag to ensure srand() is called exactly once, preventing
 * re-seeding with the same value if random_board_generator() is called
 * multiple times within the same second.
 */
static void seed_rng_once(void) {
	static int seeded = 0;
	if (!seeded) {
		srand((unsigned int)time(NULL));
		seeded = 1;
	}
}

/**
 * @brief Implementation of random_board_generator.
 *
 * Allocates a 12-element array and places exactly 5 coins at randomly selected
 * positions. Ensures no two coins occupy the same position through rejection
 * sampling.
 *
 * Algorithm:
 * 1. Initialize all cells to 0 (empty)
 * 2. While fewer than 5 coins placed:
 *    - Pick random position in [0, 11]
 *    - If empty, place coin and increment count
 *
 * @return Newly allocated array (must be freed by caller).
 */
int* random_board_generator(void) {
	seed_rng_once();
	int* game_board = (int*)malloc(sizeof(int) * 12);

	// Initialize board to 0
	for (int i = 0; i < 12; i++) {
		game_board[i] = 0;
	}

	// Place 5 coins randomly
	int coins_placed = 0;
	while (coins_placed < 5) {
		int pos = rand() % 12;
		if (game_board[pos] == 0) {
			game_board[pos] = 1;
			coins_placed++;
		}
	}

	return game_board;
}

/**
 * @brief Implementation of game_init_board.
 *
 * Initializes the board with a guaranteed playable configuration. Uses random
 * generation with validation, falling back to default if generation attempts
 * consistently fail.
 *
 * Strategy:
 * 1. Attempt up to 32 random generations
 * 2. If any configuration has valid moves, use it
 * 3. If all fail, use hardcoded default: coins at even positions [2, 4, 6, 8,
 * 10]
 *
 * The default configuration has 5 coins with empty spaces on the left for
 * movement.
 *
 * @param board Pre-allocated 12-element array to initialize.
 */
void game_init_board(uint8_t* board) {
	if (!board) {
		return;
	}

	int attempts = 0;
	do {
		int* random_board = random_board_generator();
		for (int i = 0; i < 12; i++) {
			board[i] = (uint8_t)random_board[i];
		}
		free(random_board);
		attempts++;
	} while (game_is_finished(board) && attempts < 32);

	if (game_is_finished(board)) {
		memset(board, 0, 12);
		for (int pos = 2; pos <= 10; pos += 2) {
			board[pos] = 1;
		}
	}
}

/**
 * @brief Implementation of game_is_move_valid.
 *
 * Validates all five aspects of a legal move:
 * 1. Index bounds check (0 ≤ indices < 12)
 * 2. Direction check (coin_pos < coin_index, strictly leftward)
 * 3. Source check (board[coin_index] == 1, source has coin)
 * 4. Destination check (board[coin_pos] == 0, destination empty)
 * 5. Blocking check (destination not blocked by leftmost coin)
 *
 * The blocking check finds the nearest coin to the left (lower indices) and
 * ensures the destination is not beyond it.
 *
 * @param board Board state array.
 * @param coin_index Current position of coin.
 * @param coin_pos Target position.
 * @return 1 if valid, 0 otherwise.
 */
int game_is_move_valid(const uint8_t* board, uint8_t coin_index,
					   uint8_t coin_pos) {
	if (!board) {
		return 0;
	}

	// 1. Bounds
	if (coin_index >= 12 || coin_pos >= 12) {
		return 0;
	}

	// 2. Direction + distance (must move strictly left)
	if (coin_pos >= coin_index) {
		return 0;
	}

	// 3. Source must be a coin
	if (board[coin_index] != 1) {
		return 0;
	}

	// 4. Destination must be empty
	if (board[coin_pos] != 0) {
		return 0;
	}

	// 5. Cannot pass the previous coin (closest one on the left)
	int prev_coin = -1;
	for (int i = (int)coin_index - 1; i >= 0; --i) {
		if (board[i] == 1) {
			prev_coin = i;
			break;
		}
	}

	if (prev_coin >= 0 && (int)coin_pos <= prev_coin) {
		return 0;
	}

	return 1;
}

/**
 * @brief Implementation of game_apply_move.
 *
 * Directly modifies the board by removing coin from source and placing at
 * destination. Assumes the move has been validated.
 *
 * @param board Board state array to modify.
 * @param coin_index Source position.
 * @param coin_pos Destination position.
 */
void game_apply_move(uint8_t* board, uint8_t coin_index, uint8_t coin_pos) {
	board[coin_index] = 0;
	board[coin_pos] = 1;
}

/**
 * @brief Implementation of game_is_finished.
 *
 * Determines if the game has reached terminal state by checking if any coin
 * can move left.
 *
 * Algorithm:
 * - Scan left-to-right tracking if empty space has been seen
 * - When we find a coin after seeing empty space, movement is possible
 * - If we finish scan without finding such a coin, game is finished
 *
 * This works because:
 * - Coins can only move left (to lower indices)
 * - If all empty spaces are to the right of all coins, no moves are possible
 * - Conversely, if an empty space exists left of any coin, that coin can move
 *
 * @param board Board state array.
 * @return 1 if finished (no legal moves), 0 if playable.
 */
int game_is_finished(const uint8_t* board) {
	int empty_seen = 0;

	for (int i = 0; i < 12; ++i) {
		if (board[i] == 0) {
			empty_seen = 1;
		} else if (board[i] == 1 && empty_seen) {
			return 0; // At least one coin can still move left
		}
	}

	// No coin has an empty slot to its left, so no moves remain.
	return 1;
}
