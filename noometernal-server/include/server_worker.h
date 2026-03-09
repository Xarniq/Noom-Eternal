/**
 * @file server_worker.h
 * @brief Client connection management and game session coordination.
 *
 * This module handles individual client threads, lobby management, and the
 * complete lifecycle of game sessions including authentication, shop
 * interactions, and match play. It implements thread-safe lobbies using mutexes
 * and condition variables for synchronizing two-player matches.
 *
 * @author NoomEternal Development Team
 * @version 1.0
 * @date 2026-01-08
 */

#ifndef SERVER_WORKER_H
#define SERVER_WORKER_H

#include <pthread.h>
#include <stdint.h>

/**
 * @brief Forward declaration of ServerLobby structure.
 * @see ServerLobby
 */
struct ServerLobby;

/**
 * @brief Thread entry point for handling a single client connection.
 *
 * This function serves as the entry point for each client handling thread. It
 * orchestrates the complete client lifecycle:
 * - Authentication via CONNECT message
 * - Loading player profile from database
 * - Lobby waiting and shop interaction
 * - Match sessions with win/loss reward application
 * - Connection teardown
 *
 * The function runs in a loop, allowing clients to play multiple matches and
 * access the shop between games.
 *
 * @param[in] arg Pointer to a Thread_args structure containing the client
 * socket descriptor, client ID, and reference to the game lobby. The function
 * takes ownership and will free the Thread_args pointer.
 *
 * @return NULL on thread completion or disconnection.
 *
 * @note Each client connection spawns a detached thread running this function.
 * @note The function closes the client socket descriptor before returning.
 * @note Thread-safe access to ServerLobby is ensured via mutex operations.
 *
 * @see Thread_args, ServerLobby, server_connection.h
 */
void* server_client_thread(void* arg);

/**
 * @brief Argument structure passed to client handling threads.
 *
 * Contains all necessary information for a thread to manage a single client
 * connection, including socket communication and shared game lobby state.
 */
typedef struct {
	/** @brief Socket file descriptor for client communication. */
	int fd;
	/** @brief Unique client/player ID within the lobby (0 or 1). */
	int id;
	/** @brief Pointer to the shared lobby state for match coordination. */
	struct ServerLobby* lobby;
} Thread_args;

/**
 * @brief Shared game lobby state for two-player match coordination.
 *
 * This structure maintains all shared state between two clients participating
 * in a match. Access to all fields must be protected by the mutex to ensure
 * thread safety.
 *
 * @thread_safe All accesses must hold the mutex lock.
 */
typedef struct ServerLobby {
	/** @brief Mutual exclusion lock protecting all lobby state. */
	pthread_mutex_t mutex;
	/** @brief Condition variable for signaling state changes to waiting
	 * threads. */
	pthread_cond_t cond;
	/** @brief Socket file descriptors for the two connected clients. Unused
	 * slots are -1. */
	int client_fds[2];
	/** @brief Number of clients currently in the lobby (0, 1, or 2). */
	int count;
	/** @brief Game board state: 12 cells, 1 = coin, 0 = empty. */
	uint8_t board[12];
	/** @brief Index in client_fds array (0 or 1) indicating whose turn it is.
	 */
	int current_turn;
	/** @brief Client ID of the match winner (-1 = no winner, 0 or 1 = player
	 * index). */
	int winner_id;
	/** @brief Boolean flag indicating whether the match has started. */
	int game_started;
	/** @brief Boolean flag indicating whether the match has finished. */
	int game_over;
} ServerLobby;

#endif /* SERVER_WORKER_H */