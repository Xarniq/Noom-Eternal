/**
 * @file server_worker.c
 * @brief Implementation of client connection handling and game session
 * coordination.
 *
 * Manages the complete lifecycle of a single client connection:
 * 1. **Authentication**: Receive CONNECT message with username
 * 2. **Profile Loading**: Load player data from database
 * 3. **Lobby & Shop**: Allow clients to wait for matches or browse shop
 * 4. **Match Coordination**: Two-player game session with turn-based play
 * 5. **Reward Application**: Process match outcomes and update profiles
 * 6. **Reconnection Loop**: Clients can play multiple matches in one session
 *
 * Protocol Flow:
 * - Client connects → CONNECT message → PLAYER_INFO response
 * - Client requests join → GAME_JOIN → enters lobby
 * - When 2 players ready → GAME_START with board state
 * - Players alternate MOVE requests → server validates and broadcasts state
 * - Game ends → GAME_END with win/loss, rewards applied
 * - Loop back to lobby
 *
 * Thread Safety:
 * - ServerLobby protected by mutex
 * - Each client thread has local copy of board before playing
 * - State changes broadcast via condition variable
 */

#include "server_worker.h"
#include "db_manager.h"
#include "game_logic.h"
#include "libemap.h"
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * @brief Set receive timeout on a socket.
 *
 * Configures SO_RCVTIMEO to prevent indefinite blocking on recv calls.
 * This is crucial for network play to detect dead connections.
 *
 * @param fd Socket file descriptor.
 * @param timeout_sec Timeout in seconds (0 to disable).
 * @return 0 on success, -1 on failure.
 * @internal
 */
static int set_recv_timeout(int fd, int timeout_sec) {
	struct timeval tv;
	tv.tv_sec = timeout_sec;
	tv.tv_usec = 0;
	return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

/**
 * @brief Skin price lookup table.
 *
 * Indexed by skin_index. Skin 0 is free (default), others have cost in souls.
 *
 * @internal
 */
static const uint16_t kCoinPrices[] = {0, 100, 150, 200, 250, 300};

/**
 * @brief Safe free that nullifies the pointer.
 *
 * @param p Pointer to pointer to free.
 * @internal
 */
static void safe_free(void** p) {
	if (p && *p) {
		free(*p);
		*p = NULL;
	}
}

/**
 * @brief Resolve the price of a skin given its index.
 *
 * Looks up the price in kCoinPrices array. Returns 0 if index is out of bounds.
 *
 * @param skin_index Skin to query.
 * @return Price in souls, or 0 if invalid.
 * @internal
 */
static uint16_t resolve_skin_price(uint8_t skin_index) {
	size_t count = sizeof(kCoinPrices) / sizeof(kCoinPrices[0]);
	if (skin_index < count) {
		return kCoinPrices[skin_index];
	}
	return 0;
}

/**
 * @brief Wait for GENERAL_ACK from client.
 *
 * Receives the next packet and validates it's a GENERAL_ACK with OK status.
 * Frees payload regardless of outcome.
 *
 * @param fd Client socket.
 * @return 0 if ACK_OK received, -1 otherwise.
 * @internal
 */
static int wait_for_general_ack(int fd) {
	uint8_t type = 0;
	void* payload = NULL;
	uint16_t payload_len = 0;

	if (emap_recv_packet(fd, &type, &payload, &payload_len) == -1) {
		safe_free(&payload);
		return -1;
	}

	int status = -1;
	if (type == EMAP_MSG_GENERAL_ACK && payload_len >= 1) {
		EMAP_PayloadGeneralACK ack = {0};
		memcpy(&ack, payload,
			   sizeof(ack) <= payload_len ? sizeof(ack) : payload_len);
		if (ack.ack_type == EMAP_MESSAGE_ACK_OK) {
			status = 0;
		}
	}

	safe_free(&payload);
	return status;
}

/**
 * @brief Send PLAYER_INFO and wait for ACK.
 *
 * Combines PLAYER_INFO send with ACK wait for simplified protocol handling.
 *
 * @param fd Client socket.
 * @param info Player info to send.
 * @return 0 on success, -1 on failure.
 * @internal
 */
static int send_player_info_and_wait_ack(int fd,
										 const EMAP_PayloadPlayerInfo* info) {
	if (emap_helper_send_player_info(fd, info->level, info->progression,
									 info->money, info->selected_skin,
									 info->possessed_skins,
									 TOTAL_SKIN_AMOUNT) == -1) {
		return -1;
	}
	return wait_for_general_ack(fd);
}

/**
 * @brief Send PLAYER_DATA_UPDATE and wait for ACK.
 *
 * Similar to send_player_info_and_wait_ack but uses PLAYER_DATA_UPDATE message
 * type.
 *
 * @param fd Client socket.
 * @param info Updated player data.
 * @return 0 on success, -1 on failure.
 * @internal
 */
static int
send_player_update_data_and_wait_ack(int fd,
									 const EMAP_PayloadPlayerDataUpdate* info) {
	if (emap_helper_send_player_data_update(fd, info->level, info->progression,
											info->money, info->selected_skin,
											info->possessed_skins,
											TOTAL_SKIN_AMOUNT) == -1) {
		return -1;
	}
	return wait_for_general_ack(fd);
}

/**
 * @brief Handle SHOP_ACTION message (purchase or equip skin).
 *
 * Processes skin shop transactions: PURCHASE (charges souls) or EQUIP (free).
 * Sends updated PLAYER_INFO if successful, ACK_NOK if failed.
 *
 * @param fd Client socket.
 * @param username Player username for database operations.
 * @param action Shop action details.
 * @return 0 on success, -1 on communication error.
 * @internal
 */
static int handle_shop_action(int fd, const char* username,
							  const EMAP_PayloadShopAction* action) {
	if (!action) {
		return -1;
	}

	EMAP_PayloadPlayerInfo updated = {0};
	int rc = 0;

	switch (action->action) {
	case EMAP_SHOP_ACTION_PURCHASE:
		rc = db_purchase_skin(username, action->skin_index,
							  resolve_skin_price(action->skin_index), &updated);
		break;
	case EMAP_SHOP_ACTION_EQUIP:
		rc = db_equip_skin(username, action->skin_index, &updated);
		break;
	default:
		rc = -3;
		break;
	}

	if (rc != 0) {
		emap_helper_send_ack(fd, EMAP_MESSAGE_ACK_NOK);
		return 0;
	}

	if (emap_helper_send_ack(fd, EMAP_MESSAGE_ACK_OK) == -1) {
		return -1;
	}

	return send_player_info_and_wait_ack(fd, &updated);
}

// Forward declarations of static functions
static bool wait_for_game_join(int fd, const char* username);
static bool wait_for_lobby_start(ServerLobby* lobby, int fd);
static bool run_match_session(int fd, ServerLobby* lobby, const char* username,
							  int my_id);
static void lobby_release_slot(ServerLobby* lobby, int* my_id);
static bool play_single_match(int fd, ServerLobby* lobby, const char* username);

/**
 * @brief Handle client disconnection during a match.
 *
 * Called when a client disconnects mid-game. Immediately terminates the match
 * and declares the other player as winner.
 *
 * @param lobby Shared lobby structure.
 * @param my_id Disconnecting client's ID.
 * @internal
 */
static void handle_disconnect_win(ServerLobby* lobby, int my_id) {
	pthread_mutex_lock(&lobby->mutex);
	// Only declare a winner if the game has actually started and is not already
	// over
	if (lobby->game_started && !lobby->game_over) {
		lobby->game_over = 1;
		// If I leave, the other player wins
		lobby->winner_id = (my_id + 1) % 2;
		pthread_cond_broadcast(&lobby->cond);
	}
	pthread_mutex_unlock(&lobby->mutex);
}

/**
 * @brief Implementation of server_client_thread.
 *
 * Complete client session orchestration:
 * 1. Connection phase: CONNECT → ACK + PLAYER_INFO
 * 2. Lobby loop:
 *    - wait_for_game_join(): Handle GAME_JOIN or SHOP_ACTION
 *    - play_single_match(): Enter match lobby and play
 *
 * Exits when client disconnects or communication error occurs.
 */
void* server_client_thread(void* arg) {
	Thread_args* args = (Thread_args*)arg;
	int fd = args->fd;
	ServerLobby* lobby = args->lobby;
	free(args);

	// Set a generous receive timeout (60 seconds) to detect dead connections
	// This prevents the server from blocking forever on recv when a client
	// disconnects unexpectedly (common in network play due to NAT/firewall)
	if (set_recv_timeout(fd, 60) == -1) {
		perror("[SERVER] Warning: Failed to set recv timeout");
	}

	uint8_t type_out = 0;
	void* payload_out = NULL;
	uint16_t payload_len_out = 0;
	EMAP_Message out;
	int connection_active = 1;
	char username[MAX_PLAYER_USERNAME_SIZE] = {0};

	// 1. Connection phase
	do {
		// Client -> Server: CONNECT
		if (emap_recv_packet(fd, &type_out, &payload_out, &payload_len_out) ==
				-1 ||
			type_out != EMAP_MSG_CONNECT) {
			printf("[SERVER] Couldn't receive packet\n");
			safe_free(&payload_out);
			connection_active = 0;
			break;
		}
		// Decode CONNECT to get username
		if (emap_decode_msg(type_out, payload_out, payload_len_out, &out) ==
			-1) {
			printf("[SERVER] Couldn't decode msg\n");
			safe_free(&payload_out);
			connection_active = 0;
			break;
		}
		// Copy username (ensure null termination)
		memcpy(username, out.data.connect.username, MAX_PLAYER_USERNAME_SIZE);
		username[MAX_PLAYER_USERNAME_SIZE - 1] = '\0'; // Safety

		printf("[SERVER] Received CONNECT from %s\n", username);

		safe_free(&payload_out);

		// Server -> Client: GENERAL_ACK
		emap_helper_send_ack(fd, EMAP_MESSAGE_ACK_OK);
		printf("[SERVER] Sent ACK (CONNECT)\n");

		printf("Client connected: %s\n", username);

		// Server -> Client: PLAYER_INFO (Server sends initial data)
		EMAP_PayloadPlayerInfo p_info;
		printf("[SERVER] Sending PLAYER_INFO for %s\n", username);
		int profile_rc = db_get_player_info(username, &p_info);
		if (profile_rc != 0) {
			memset(&p_info, 0, sizeof(p_info));
			p_info.level = 1;
			p_info.progression = 0;
			p_info.money = 0;
			if (TOTAL_SKIN_AMOUNT > 0) {
				p_info.selected_skin = 0;
				p_info.possessed_skins[0] = 2; // default skin owned + equipped
			}
		}

		if (send_player_info_and_wait_ack(fd, &p_info) != 0) {
			connection_active = 0;
			break;
		}

	} while (0);

	bool keep_running = true;
	while (keep_running) {
		if (!wait_for_game_join(fd, username)) {
			keep_running = false;
			break;
		}

		if (!play_single_match(fd, lobby, username)) {
			keep_running = false;
		}
	}

	printf("Client disconnected: %s\n", username);
	close(fd);
	return NULL;
}

/**
 * @brief Wait for client to request game join or shop action.
 *
 * Accepts GAME_JOIN (enters match lobby) or SHOP_ACTION (process
 * purchase/equip). Loops until GAME_JOIN is received.
 *
 * @param fd Client socket.
 * @param username Player name for shop operations.
 * @return true if GAME_JOIN received, false on error.
 * @internal
 */
static bool wait_for_game_join(int fd, const char* username) {
	while (true) {
		uint8_t type = 0;
		void* payload = NULL;
		uint16_t payload_len = 0;

		if (emap_recv_packet(fd, &type, &payload, &payload_len) == -1) {
			safe_free(&payload);
			return false;
		}

		if (type == EMAP_MSG_GAME_JOIN) {
			printf("[SERVER] Received GAME_JOIN from %s\n", username);
			safe_free(&payload);
			emap_helper_send_ack(fd, EMAP_MESSAGE_ACK_OK);
			printf("[SERVER] Sent ACK (GAME_JOIN)\n");
			return true;
		}

		if (type == EMAP_MSG_SHOP_ACTION) {
			if (payload_len < sizeof(EMAP_PayloadShopAction)) {
				safe_free(&payload);
				emap_helper_send_ack(fd, EMAP_MESSAGE_ACK_NOK);
				continue;
			}
			EMAP_PayloadShopAction shop_action;
			memcpy(&shop_action, payload, sizeof(shop_action));
			safe_free(&payload);
			if (handle_shop_action(fd, username, &shop_action) < 0) {
				return false;
			}
			continue;
		}

		safe_free(&payload);
	}
}

/**
 * @brief Wait until two players are in the lobby.
 *
 * Polls lobby count while periodically sending GAME_WAIT to keep client alive.
 * Uses select() with 50ms timeout to balance responsiveness and CPU usage.
 *
 * @param lobby Shared lobby structure.
 * @param fd Client socket (to send keep-alive packets).
 * @return true when count reaches 2, false on error.
 * @internal
 */
static bool wait_for_lobby_start(ServerLobby* lobby, int fd) {
	while (true) {
		pthread_mutex_lock(&lobby->mutex);
		int player_count = lobby->count;
		pthread_mutex_unlock(&lobby->mutex);

		if (player_count >= 2) {
			return true;
		}

		if (emap_helper_send_game_wait(fd) == -1) {
			return false;
		}

		fd_set readfds;
		struct timeval tv;
		FD_ZERO(&readfds);
		FD_SET(fd, &readfds);
		tv.tv_sec = 0;
		tv.tv_usec = 50000;

		int ret = select(fd + 1, &readfds, NULL, NULL, &tv);
		if (ret > 0) {
			uint8_t type = 0;
			void* payload = NULL;
			uint16_t payload_len = 0;
			if (emap_recv_packet(fd, &type, &payload, &payload_len) == -1) {
				safe_free(&payload);
				return false;
			}
			safe_free(&payload);
		} else if (ret < 0) {
			perror("select");
			return false;
		}

		usleep(50000);
	}
}

/**
 * @brief Execute a complete match session for one client.
 *
 * Match flow:
 * 1. Send GAME_START with initial board and turn assignment
 * 2. Send initial board state
 * 3. Turn loop:
 *    - Wait for client's turn using condition variable
 *    - Send current board state
 *    - Send PLAY (YOUR_TURN)
 *    - Receive MOVE, validate, apply, broadcast new state
 *    - Alternate turns or end if board is finished
 * 4. Send GAME_END with win/loss
 * 5. Return to lobby
 *
 * @param fd Client socket.
 * @param lobby Shared lobby.
 * @param username Player name (for logging).
 * @param my_id Client ID in lobby (0 or 1).
 * @return true if match completed successfully, false on error.
 * @internal
 */
static bool run_match_session(int fd, ServerLobby* lobby, const char* username,
							  int my_id) {
	uint8_t type_out = 0;
	void* payload_out = NULL;
	uint16_t payload_len_out = 0;

	pthread_mutex_lock(&lobby->mutex);
	if (!lobby->game_started) {
		lobby->game_started = 1;
		lobby->game_over = 0;
		lobby->winner_id = -1;
		game_init_board(lobby->board);
		lobby->current_turn = random() % lobby->count;
	}
	int starting_player = lobby->current_turn;
	pthread_cond_broadcast(&lobby->cond);
	pthread_mutex_unlock(&lobby->mutex);

	int is_starting = (my_id == starting_player);
	if (emap_helper_send_game_start(fd, is_starting
											? EMAP_PLAY_YOUR_TURN
											: EMAP_PLAY_NOT_YOUR_TURN) == -1) {
		return false;
	}
	printf("[SERVER] Sent GAME_START to %s (Starting: %s)\n", username,
		   is_starting ? "YES" : "NO");

	if (emap_recv_packet(fd, &type_out, &payload_out, &payload_len_out) == -1 ||
		type_out != EMAP_MSG_GENERAL_ACK) {
		safe_free(&payload_out);
		handle_disconnect_win(lobby, my_id);
		return false;
	}
	printf("[SERVER] Received ACK (GAME_START) from %s\n", username);
	safe_free(&payload_out);

	if (emap_helper_send_game_state(fd, lobby->board, 12) == -1) {
		handle_disconnect_win(lobby, my_id);
		return false;
	}
	printf("[SERVER] Sent initial GAME_STATE to %s\n", username);

	if (emap_recv_packet(fd, &type_out, &payload_out, &payload_len_out) == -1 ||
		type_out != EMAP_MSG_GENERAL_ACK) {
		safe_free(&payload_out);
		handle_disconnect_win(lobby, my_id);
		return false;
	}
	printf("[SERVER] Received ACK (GAME_STATE) from %s\n", username);
	safe_free(&payload_out);

	int game_running = 1;
	EMAP_Message out;

	while (game_running) {
		pthread_mutex_lock(&lobby->mutex);
		while (lobby->current_turn != my_id && !lobby->game_over &&
			   game_running) {
			pthread_cond_wait(&lobby->cond, &lobby->mutex);
		}

		if (lobby->game_over) {
			game_running = 0;
			pthread_mutex_unlock(&lobby->mutex);
			break;
		}

		uint8_t board_snapshot[12];
		memcpy(board_snapshot, lobby->board, sizeof(board_snapshot));
		pthread_mutex_unlock(&lobby->mutex);

		if (emap_helper_send_game_state(fd, board_snapshot, 12) == -1) {
			handle_disconnect_win(lobby, my_id);
			return false;
		}
		printf("[SERVER] Sent GAME_STATE to %s before PLAY\n", username);

		if (emap_recv_packet(fd, &type_out, &payload_out, &payload_len_out) ==
				-1 ||
			type_out != EMAP_MSG_GENERAL_ACK) {
			safe_free(&payload_out);
			handle_disconnect_win(lobby, my_id);
			return false;
		}
		safe_free(&payload_out);

		if (emap_helper_send_play(fd, EMAP_PLAY_YOUR_TURN) == -1) {
			handle_disconnect_win(lobby, my_id);
			return false;
		}
		printf("[SERVER] Sent PLAY (YOUR_TURN) to %s\n", username);

		if (emap_recv_packet(fd, &type_out, &payload_out, &payload_len_out) ==
				-1 ||
			type_out != EMAP_MSG_GENERAL_ACK) {
			safe_free(&payload_out);
			handle_disconnect_win(lobby, my_id);
			return false;
		}
		printf("[SERVER] Received ACK (PLAY) from %s\n", username);
		safe_free(&payload_out);

		int move_valid = 0;
		while (!move_valid) {
			if (emap_recv_packet(fd, &type_out, &payload_out,
								 &payload_len_out) == -1 ||
				type_out != EMAP_MSG_MOVE) {
				safe_free(&payload_out);
				handle_disconnect_win(lobby, my_id);
				return false;
			}

			if (emap_decode_msg(type_out, payload_out, payload_len_out, &out) ==
				-1) {
				safe_free(&payload_out);
				handle_disconnect_win(lobby, my_id);
				return false;
			}

			uint8_t coin_idx = out.data.move.coin_index;
			uint8_t coin_pos = out.data.move.coin_pos;
			safe_free(&payload_out);

			printf("[SERVER] Received MOVE from %s: Coin %d -> Pos %d\n",
				   username, coin_idx, coin_pos);

			pthread_mutex_lock(&lobby->mutex);
			if (game_is_move_valid(lobby->board, coin_idx, coin_pos)) {
				game_apply_move(lobby->board, coin_idx, coin_pos);
				move_valid = 1;

				if (game_is_finished(lobby->board)) {
					lobby->game_over = 1;
					lobby->winner_id = my_id;
					game_running = 0;
				} else {
					lobby->current_turn =
						(lobby->current_turn + 1) % lobby->count;
				}

				uint8_t new_snapshot[12];
				memcpy(new_snapshot, lobby->board, sizeof(new_snapshot));
				pthread_cond_broadcast(&lobby->cond);
				pthread_mutex_unlock(&lobby->mutex);

				emap_helper_send_ack(fd, EMAP_MESSAGE_ACK_OK);
				printf("[SERVER] Move Valid. Sent ACK OK.\n");

				if (emap_helper_send_game_state(fd, new_snapshot, 12) == -1) {
					handle_disconnect_win(lobby, my_id);
					return false;
				}
				printf("[SERVER] Sent updated GAME_STATE to %s after move\n",
					   username);

				if (emap_recv_packet(fd, &type_out, &payload_out,
									 &payload_len_out) == -1 ||
					type_out != EMAP_MSG_GENERAL_ACK) {
					safe_free(&payload_out);
					handle_disconnect_win(lobby, my_id);
					return false;
				}
				safe_free(&payload_out);
			} else {
				pthread_mutex_unlock(&lobby->mutex);
				emap_helper_send_illegal(fd);
				printf("[SERVER] Move Invalid. Sent ILLEGAL_PLAY.\n");

				if (emap_recv_packet(fd, &type_out, &payload_out,
									 &payload_len_out) == -1 ||
					type_out != EMAP_MSG_GENERAL_ACK) {
					safe_free(&payload_out);
					handle_disconnect_win(lobby, my_id);
					return false;
				}
				safe_free(&payload_out);
			}
		}
	}

	pthread_mutex_lock(&lobby->mutex);
	int won = (lobby->winner_id == my_id);
	pthread_mutex_unlock(&lobby->mutex);

	EMAP_PayloadPlayerDataUpdate updated_info;
	if (db_apply_match_result(username, won, &updated_info) == 0) {
		if (send_player_update_data_and_wait_ack(fd, &updated_info) != 0) {
			return false;
		}
	}

	if (emap_helper_send_game_end(fd, won ? EMAP_END_GAME_WIN
										  : EMAP_END_GAME_LOSE) == -1) {
		return false;
	}
	printf("[SERVER] Sent GAME_END (%s) to %s\n", won ? "WIN" : "LOSE",
		   username);

	if (emap_recv_packet(fd, &type_out, &payload_out, &payload_len_out) != -1) {
		safe_free(&payload_out);
	}

	return true;
}

/**
 * @brief Release a client's slot in the lobby.
 *
 * Removes client from lobby and resets lobby state if it becomes empty.
 * Used when client completes a match or disconnects.
 *
 * @param lobby Shared lobby.
 * @param my_id Client ID (pointer, set to -1 on exit).
 * @internal
 */
static void lobby_release_slot(ServerLobby* lobby, int* my_id) {
	pthread_mutex_lock(&lobby->mutex);
	if (*my_id >= 0 && lobby->count > 0) {
		lobby->client_fds[*my_id] = -1;
		lobby->count--;
	}
	if (lobby->count <= 0) {
		lobby->count = 0;
		lobby->game_started = 0;
		lobby->game_over = 0;
		lobby->winner_id = -1;
		lobby->current_turn = 0;
		game_init_board(lobby->board);
		printf("[SERVER] Lobby reset (empty)\n");
		pthread_cond_broadcast(&lobby->cond);
	}
	pthread_mutex_unlock(&lobby->mutex);
	*my_id = -1;
}

/**
 * @brief Orchestrate a complete match cycle.
 *
 * 1. Acquire slot in lobby
 * 2. Wait until 2 players present
 * 3. Run match session
 * 4. Release slot
 * 5. Return to lobby for potential next match
 *
 * @param fd Client socket.
 * @param lobby Shared lobby.
 * @param username Player name.
 * @return true if match completed (can play again), false on error.
 * @internal
 */
static bool play_single_match(int fd, ServerLobby* lobby,
							  const char* username) {
	pthread_mutex_lock(&lobby->mutex);
	while (lobby->game_started) {
		pthread_cond_wait(&lobby->cond, &lobby->mutex);
	}
	if (lobby->count >= 2) {
		pthread_mutex_unlock(&lobby->mutex);
		return false;
	}

	int my_id = lobby->count;
	lobby->client_fds[my_id] = fd;
	lobby->count++;
	pthread_mutex_unlock(&lobby->mutex);

	if (!wait_for_lobby_start(lobby, fd)) {
		lobby_release_slot(lobby, &my_id);
		return false;
	}

	bool session_ok = run_match_session(fd, lobby, username, my_id);
	lobby_release_slot(lobby, &my_id);

	if (session_ok) {
		printf("[SERVER] %s returned to lobby, keeping connection open.\n",
			   username);
	}

	return session_ok;
}
