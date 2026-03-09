// Feature test macro for POSIX functions on Linux
#define _POSIX_C_SOURCE 200112L

#include "network_client.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <libemap.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define RECV_TIMEOUT_MS 100

/**
 * @brief Configure a socket file descriptor to non-blocking mode.
 *
 * Sets the O_NONBLOCK flag on the socket using fcntl, allowing read/write
 * operations to return immediately rather than blocking.
 *
 * @param fd File descriptor of the socket to configure.
 * @return 0 on success, -1 on error.
 */
static int set_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * @brief Update local player profile data from server payload.
 *
 * Synchronizes player statistics (level, XP, currency) and skin ownership
 * from a server payload. Ensures selected skin is valid; falls back to first
 * owned skin if necessary. Marks profile as ready.
 *
 * @param player Pointer to PlayerData to update.
 * @param payload Pointer to server payload containing updated player info.
 */
static void client_update_profile(PlayerData* player,
								  const EMAP_PayloadPlayerDataUpdate* payload) {
	if (!player || !payload) {
		return;
	}

	player->level = payload->level;
	player->xp = payload->progression;
	player->souls = payload->money;

	size_t skin_slots =
		sizeof(player->ownedSkins) / sizeof(player->ownedSkins[0]);
	for (size_t i = 0; i < skin_slots; ++i) {
		bool owned = false;
		if (i < TOTAL_SKIN_AMOUNT) {
			owned = payload->possessed_skins[i] != 0;
		}
		player->ownedSkins[i] = owned;
	}

	uint8_t desired = payload->selected_skin;
	if (desired >= skin_slots || !player->ownedSkins[desired]) {
		desired = 0;
		for (size_t i = 0; i < skin_slots; ++i) {
			if (player->ownedSkins[i]) {
				desired = (uint8_t)i;
				break;
			}
		}
	}

	player->selectedCoin = desired;
	player->profileReady = true;
}

/**
 * @brief Receive and decode an EMAP message from a non-blocking socket.
 *
 * Peeks at socket buffer to verify a complete packet is available before
 * reading. Decodes the EMAP message and optionally stores raw payload bytes.
 * Handles incomplete packets gracefully.
 *
 * @param sock Non-blocking socket to read from.
 * @param msg_out Pointer to EMAP_Message to populate with decoded data.
 * @param raw_payload Optional buffer to store raw payload bytes.
 * @param raw_len Pointer to store actual payload size (may be NULL).
 * @param raw_capacity Maximum size of raw_payload buffer.
 * @return 0 on success, -1 on error, -2 if no complete packet available.
 */
static int recv_message_nonblocking(int sock, EMAP_Message* msg_out,
									uint8_t* raw_payload, uint16_t* raw_len,
									size_t raw_capacity) {
	// 1. Peek header to check if we have at least a full header (6 bytes)
	uint8_t header_buf[6];
	ssize_t peek_len = recv(sock, header_buf, sizeof(header_buf), MSG_PEEK);

	if (peek_len < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return -2; // No data available
		}
		return -1; // Error
	}

	if (peek_len < (ssize_t)sizeof(header_buf)) {
		return -2; // Partial header, wait for more
	}

	// 2. Decode payload length from header
	// Header layout: magic[2], version, code, size(uint16_t)
	// size is at offset 4
	uint16_t payload_len_net;
	memcpy(&payload_len_net, &header_buf[4], sizeof(uint16_t));
	uint16_t payload_len = ntohs(payload_len_net);

	// 3. Check if full packet (header + payload) is available
	size_t total_len = sizeof(header_buf) + payload_len;

	// We need to verify that 'total_len' bytes are available.
	// We can use MSG_PEEK with a larger buffer.
	// Max payload is 1024, so 2048 is plenty safe.
	uint8_t peek_full[2048];
	if (total_len > sizeof(peek_full)) {
		// Should not happen given EMAP_MAX_PAYLOAD_SIZE, but handle safety
		return -1;
	}

	ssize_t full_peek = recv(sock, peek_full, total_len, MSG_PEEK);

	if (full_peek < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return -2;
		return -1;
	}

	if ((size_t)full_peek < total_len) {
		return -2; // Full packet not yet arrived
	}

	// 4. Full packet is available, safe to call emap_recv_packet
	// emap_recv_packet calls emap_recv_all which loops until it gets
	// everything. Since we verified it's in the kernel buffer, it should
	// succeed without blocking/EAGAIN.

	uint8_t msg_type;
	void* payload = NULL;
	uint16_t decoded_len = 0;

	int result = emap_recv_packet(sock, &msg_type, &payload, &decoded_len);
	if (result < 0) {
		// This might happen if emap_recv_packet fails validation (magic bytes,
		// etc) In that case, it might have consumed bytes or not. If it returns
		// -1, we assume fatal error or protocol mismatch.
		return -1;
	}

	if (raw_payload && raw_len && raw_capacity > 0 && payload &&
		decoded_len > 0) {
		size_t copy_len = decoded_len;
		if (copy_len > raw_capacity) {
			copy_len = raw_capacity;
		}
		memcpy(raw_payload, payload, copy_len);
		*raw_len = (uint16_t)copy_len;
	} else if (raw_len) {
		*raw_len = decoded_len;
	}

	result = emap_decode_msg(msg_type, payload, decoded_len, msg_out);
	if (result < 0 && msg_type == EMAP_MSG_GAME_STATE) {
		// The base decoder expects a zero-length payload, but we forward the
		// raw snapshot.
		msg_out->type = EMAP_MSG_GAME_STATE;
		result = 0;
	}

	if (payload) {
		free(payload);
	}

	return result;
}

/**
 * @brief Initialize a NetworkClient structure.
 *
 * @param client Pointer to the NetworkClient to initialize.
 */
void NetworkClient_Init(NetworkClient* client) {
	client->socket_fd = -1;
	client->state = NET_DISCONNECTED;
	client->is_our_turn = false;
	client->waiting_for_ack = false;
	client->error_message[0] = '\0';
	client->player_id = 0;
}

/**
 * @brief Establish a connection to the game server and perform the initial
 * handshake.
 *
 * @param client Pointer to the NetworkClient to configure.
 * @param server_ip The IP address of the server to connect to.
 * @param port The port number to use for the connection.
 * @param username The username to present to the server.
 * @param player Pointer to the PlayerData structure to populate from the server
 * response.
 * @return true on successful connection and handshake, false on failure
 * (error_message is set).
 */
bool NetworkClient_Connect(NetworkClient* client, const char* server_ip,
						   int port, const char* username, PlayerData* player) {
	printf("[NETWORK] Starting connection to %s:%d as '%s'\n", server_ip, port,
		   username);

	// Create socket
	client->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (client->socket_fd < 0) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Failed to create socket: %s", strerror(errno));
		printf("[NETWORK] ERROR: %s\n", client->error_message);
		client->state = NET_ERROR;
		return false;
	}
	printf("[NETWORK] Socket created (fd: %d)\n", client->socket_fd);

	// macOS/BSD specific: Set SO_REUSEADDR and SO_REUSEPORT
	int yes = 1;
	if (setsockopt(client->socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
				   sizeof(yes)) == -1) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Failed to set SO_REUSEADDR: %s", strerror(errno));
		printf("[NETWORK] ERROR: %s\n", client->error_message);
		close(client->socket_fd);
		client->socket_fd = -1;
		client->state = NET_ERROR;
		return false;
	}
	printf("[NETWORK] SO_REUSEADDR set\n");

#ifdef SO_REUSEPORT
	if (setsockopt(client->socket_fd, SOL_SOCKET, SO_REUSEPORT, &yes,
				   sizeof(yes)) == -1) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Failed to set SO_REUSEPORT: %s", strerror(errno));
		printf("[NETWORK] ERROR: %s\n", client->error_message);
		close(client->socket_fd);
		client->socket_fd = -1;
		client->state = NET_ERROR;
		return false;
	}
	printf("[NETWORK] SO_REUSEPORT set\n");
#else
	printf("[NETWORK] SO_REUSEPORT not available on this platform, "
		   "continuing...\n");
#endif

	// Enable TCP keepalive to detect dead connections
	if (setsockopt(client->socket_fd, SOL_SOCKET, SO_KEEPALIVE, &yes,
				   sizeof(yes)) == -1) {
		printf("[NETWORK] WARNING: Failed to set SO_KEEPALIVE: %s\n",
			   strerror(errno));
	} else {
		printf("[NETWORK] SO_KEEPALIVE set\n");
	}

	// Disable Nagle's algorithm for lower latency
	if (setsockopt(client->socket_fd, IPPROTO_TCP, TCP_NODELAY, &yes,
				   sizeof(yes)) == -1) {
		printf("[NETWORK] WARNING: Failed to set TCP_NODELAY: %s\n",
			   strerror(errno));
	} else {
		printf("[NETWORK] TCP_NODELAY set\n");
	}

	// Set non-blocking
	if (set_nonblocking(client->socket_fd) < 0) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Failed to set non-blocking: %s", strerror(errno));
		printf("[NETWORK] ERROR: %s\n", client->error_message);
		close(client->socket_fd);
		client->socket_fd = -1;
		client->state = NET_ERROR;
		return false;
	}
	printf("[NETWORK] Socket set to non-blocking\n");

	// Connect to server
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	// Try to parse as IP address first
	if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) > 0) {
		printf("[NETWORK] Server address parsed as IPv4: %s:%d\n", server_ip, port);
	} else {
		// Not a valid IPv4 address, try DNS resolution
		printf("[NETWORK] Not an IPv4 address, attempting DNS resolution for %s\n", server_ip);
		
		struct addrinfo hints, *res, *p;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		
		int gai_result = getaddrinfo(server_ip, NULL, &hints, &res);
		if (gai_result != 0) {
			snprintf(client->error_message, sizeof(client->error_message),
					 "DNS resolution failed for %s: %s", server_ip, gai_strerror(gai_result));
			printf("[NETWORK] ERROR: %s\n", client->error_message);
			close(client->socket_fd);
			client->socket_fd = -1;
			client->state = NET_ERROR;
			return false;
		}
		
		// Use the first result
		p = res;
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
		server_addr.sin_addr = ipv4->sin_addr;
		freeaddrinfo(res);
		
		printf("[NETWORK] DNS resolved %s to %s:%d\n", server_ip,
			   inet_ntoa(server_addr.sin_addr), port);
	}

	client->state = NET_CONNECTING;

	int conn_result = connect(client->socket_fd, (struct sockaddr*)&server_addr,
							  sizeof(server_addr));
	printf("[NETWORK] connect() returned: %d, errno: %d (%s)\n", conn_result,
		   errno, strerror(errno));

	if (conn_result < 0 && errno != EINPROGRESS) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Connection failed: %s", strerror(errno));
		printf("[NETWORK] ERROR: %s\n", client->error_message);
		close(client->socket_fd);
		client->socket_fd = -1;
		client->state = NET_ERROR;
		return false;
	}

	printf("[NETWORK] Connection initiated, waiting for completion...\n");

	// Wait for connection to complete (non-blocking connect needs to be
	// checked) Use select to wait for the socket to be writable
	fd_set writefds;
	FD_ZERO(&writefds);
	FD_SET(client->socket_fd, &writefds);
	struct timeval timeout;
	timeout.tv_sec = 5; // 5 second timeout
	timeout.tv_usec = 0;

	int select_result =
		select(client->socket_fd + 1, NULL, &writefds, NULL, &timeout);
	printf("[NETWORK] select() returned: %d\n", select_result);

	if (select_result <= 0) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Connection timeout or failed");
		printf("[NETWORK] ERROR: %s\n", client->error_message);
		close(client->socket_fd);
		client->socket_fd = -1;
		client->state = NET_ERROR;
		return false;
	}

	// Check if connection was successful
	int so_error = 0;
	socklen_t len = sizeof(so_error);
	if (getsockopt(client->socket_fd, SOL_SOCKET, SO_ERROR, &so_error, &len) <
		0) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Failed to check connection status");
		printf("[NETWORK] ERROR: %s\n", client->error_message);
		close(client->socket_fd);
		client->socket_fd = -1;
		client->state = NET_ERROR;
		return false;
	}

	if (so_error != 0) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Connection failed: %s", strerror(so_error));
		printf("[NETWORK] ERROR: %s\n", client->error_message);
		close(client->socket_fd);
		client->socket_fd = -1;
		client->state = NET_ERROR;
		return false;
	}

	printf("[NETWORK] Connection established successfully\n");

	// Send CONNECT message
	printf("[NETWORK] Sending CONNECT message with username: %s\n", username);
	if (emap_helper_send_connect(client->socket_fd, username) < 0) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Failed to send CONNECT message");
		printf("[NETWORK] ERROR: %s\n", client->error_message);
		close(client->socket_fd);
		client->socket_fd = -1;
		client->state = NET_ERROR;
		return false;
	}
	printf("[NETWORK] CONNECT message sent\n");

	// Set to blocking mode for handshake responses
	printf("[NETWORK] Setting socket to blocking mode for handshake\n");
	fcntl(client->socket_fd, F_SETFL,
		  fcntl(client->socket_fd, F_GETFL, 0) & ~O_NONBLOCK);

	// Expect GENERAL_ACK
	EMAP_Message msg;
	int res;

	// Loop until we get a full message or error
	printf("[NETWORK] Waiting for GENERAL_ACK from server...\n");
	do {
		res = recv_message_nonblocking(client->socket_fd, &msg, NULL, NULL, 0);
		if (res == -2)
			printf("[NETWORK] No data yet, retrying...\n");
	} while (res == -2);

	if (res < 0 || msg.type != EMAP_MSG_GENERAL_ACK ||
		msg.data.general_ack.ack_type != EMAP_MESSAGE_ACK_OK) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Connection refused by server");
		printf("[NETWORK] ERROR: %s (res=%d, msg.type=%d)\n",
			   client->error_message, res, msg.type);
		close(client->socket_fd);
		client->socket_fd = -1;
		client->state = NET_ERROR;
		return false;
	}
	printf("[NETWORK] Received GENERAL_ACK OK\n");

	// Expect PLAYER_INFO from server
	printf("[NETWORK] Waiting for PLAYER_INFO from server...\n");
	do {
		res = recv_message_nonblocking(client->socket_fd, &msg, NULL, NULL, 0);
		if (res == -2)
			printf("[NETWORK] No data yet, retrying...\n");
	} while (res == -2);

	if (res < 0 || msg.type != EMAP_MSG_PLAYER_INFO) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Failed to receive player info");
		printf("[NETWORK] ERROR: %s (res=%d, msg.type=%d)\n",
			   client->error_message, res, msg.type);
		close(client->socket_fd);
		client->socket_fd = -1;
		client->state = NET_ERROR;
		return false;
	}
	printf("[NETWORK] Received PLAYER_INFO: level=%d, xp=%d, money=%d\n",
		   msg.data.player_info.level, msg.data.player_info.progression,
		   msg.data.player_info.money);

	client_update_profile(player, &msg.data.player_info);

	// Send ACK for PLAYER_INFO
	printf("[NETWORK] Sending ACK for PLAYER_INFO\n");
	if (emap_helper_send_ack(client->socket_fd, EMAP_MESSAGE_ACK_OK) < 0) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Failed to send ACK");
		printf("[NETWORK] ERROR: %s\n", client->error_message);
		close(client->socket_fd);
		client->socket_fd = -1;
		client->state = NET_ERROR;
		return false;
	}

	// Set back to non-blocking
	set_nonblocking(client->socket_fd);
	printf("[NETWORK] Socket set back to non-blocking\n");

	client->state = NET_CONNECTED;
	printf("[NETWORK] Connection successful! State: NET_CONNECTED\n");
	return true;
}

/**
 * @brief Send a request to join a game session on the server.
 *
 * @param client Pointer to the connected NetworkClient.
 * @return true when the game join request is acknowledged, false otherwise.
 */
bool NetworkClient_JoinGame(NetworkClient* client) {
	if (client->state != NET_CONNECTED) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Not connected to server");
		printf("[NETWORK] ERROR: Cannot join game - %s (state=%d)\n",
			   client->error_message, client->state);
		return false;
	}

	printf("[NETWORK] Sending GAME_JOIN request\n");
	// Send GAME_JOIN
	if (emap_helper_send_game_join(client->socket_fd) < 0) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Failed to send GAME_JOIN");
		printf("[NETWORK] ERROR: %s\n", client->error_message);
		client->state = NET_ERROR;
		return false;
	}

	printf("[NETWORK] Switching to blocking mode for GAME_JOIN ACK\n");
	// Expect GENERAL_ACK (blocking for this critical step)
	fcntl(client->socket_fd, F_SETFL,
		  fcntl(client->socket_fd, F_GETFL, 0) & ~O_NONBLOCK);

	EMAP_Message msg;
	int res;
	printf("[NETWORK] Waiting for GENERAL_ACK for GAME_JOIN...\n");
	do {
		res = recv_message_nonblocking(client->socket_fd, &msg, NULL, NULL, 0);
		if (res == -2)
			printf("[NETWORK] No data yet, retrying...\n");
	} while (res == -2);

	if (res < 0 || msg.type != EMAP_MSG_GENERAL_ACK ||
		msg.data.general_ack.ack_type != EMAP_MESSAGE_ACK_OK) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Game join refused");
		printf("[NETWORK] ERROR: %s (res=%d, msg.type=%d)\n",
			   client->error_message, res, msg.type);
		set_nonblocking(client->socket_fd);
		client->state = NET_ERROR;
		return false;
	}
	printf("[NETWORK] Received GENERAL_ACK for GAME_JOIN\n");

	set_nonblocking(client->socket_fd);
	client->state = NET_IN_LOBBY;
	printf("[NETWORK] Joined lobby successfully! State: NET_IN_LOBBY\n");
	return true;
}

/**
 * @brief Poll the server while waiting for the game to start from the lobby.
 *
 * @param client Pointer to the client in the lobby state.
 * @param game Pointer to the GameState to initialize when the game starts.
 * @return true while still waiting or on recoverable notifications, false when
 * the game is ready (or on fatal error).
 */
bool NetworkClient_WaitForGame(NetworkClient* client, GameState* game) {
	if (client->state != NET_IN_LOBBY) {
		return false;
	}

	EMAP_Message msg;
	int result;
	static int waitPingCounter = 0;

	// Drain any queued lobby messages so we do not miss GAME_START
	do {
		result =
			recv_message_nonblocking(client->socket_fd, &msg, NULL, NULL, 0);

		if (result == -2) {
			return true; // Still waiting (no more data)
		}

		if (result < 0) {
			snprintf(client->error_message, sizeof(client->error_message),
					 "Error while waiting for game");
			client->state = NET_ERROR;
			return false;
		}

		switch (msg.type) {
		case EMAP_MSG_GAME_WAIT:
			if ((waitPingCounter++ % 60) == 0) {
				printf("[CLIENT] Still waiting for opponent...\n");
			}
			emap_helper_send_ack(client->socket_fd, EMAP_MESSAGE_ACK_OK);
			break;

		case EMAP_MSG_GAME_START:
			client->is_our_turn =
				(msg.data.game_start.is_starting == EMAP_PLAY_YOUR_TURN);
			client->state = NET_IN_GAME;
			client->waiting_for_ack = false;

			memset(game->board, 0, sizeof(game->board));
			game->currentPlayer = client->is_our_turn ? 1 : -1;
			game->gameOver = false;
			game->winner = 0;
			game->selectedSlot = -1;
			waitPingCounter = 0;

			printf("[CLIENT] Received GAME_START. Starting: %s\n",
				   client->is_our_turn ? "YES" : "NO");

			emap_helper_send_ack(client->socket_fd, EMAP_MESSAGE_ACK_OK);
			return false; // Game started!

		default:
			// Ignore other messages while waiting
			break;
		}
	} while (result >= 0);

	return true;
}

/**
 * @brief Send a move to the server when it is the client's turn.
 *
 * @param client Pointer to the active NetworkClient in a game.
 * @param coin_index Index of the coin being moved.
 * @param target_pos Position to move the coin to.
 * @return true when the move request was successfully sent, false on validation
 * or send errors.
 */
bool NetworkClient_SendMove(NetworkClient* client, uint8_t coin_index,
							uint8_t target_pos) {
	if (client->state != NET_IN_GAME) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Not in game");
		return false;
	}

	if (!client->is_our_turn) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Not our turn");
		return false;
	}

	if (client->waiting_for_ack) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Awaiting previous move result");
		return false;
	}

	// Validation: coin_index must be > target_pos
	if (coin_index <= target_pos) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Invalid move: coin_index must be > target_pos");
		return false;
	}

	// Send MOVE
	if (emap_helper_send_move(client->socket_fd, coin_index, target_pos) < 0) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Failed to send move");
		client->state = NET_ERROR;
		return false;
	}

	printf("[CLIENT] Sent MOVE: Coin %d -> Pos %d\n", coin_index, target_pos);

	client->waiting_for_ack = true;
	return true;
}
/**
 * @brief Send a shop action request to the server.
 *
 * @param client Pointer to the connected NetworkClient.
 * @param action The shop action to perform.
 * @param skin_index The index of the skin to act upon.
 * @return true on successful send, false on error.
 */
bool NetworkClient_SendShopAction(NetworkClient* client, uint8_t action,
								  uint8_t skin_index) {
	if (!client) {
		return false;
	}

	if (client->state != NET_CONNECTED) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Shop unavailable (connect first)");
		return false;
	}

	if (client->socket_fd < 0) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Socket closed");
		return false;
	}

	if (client->waiting_for_ack) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Action déjà en attente");
		return false;
	}

	if (skin_index >= TOTAL_SKIN_AMOUNT) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Skin invalide");
		return false;
	}

	if (emap_helper_send_shop_action(client->socket_fd, action, skin_index) <
		0) {
		snprintf(client->error_message, sizeof(client->error_message),
				 "Failed to send shop action");
		client->state = NET_ERROR;
		return false;
	}

	printf("[CLIENT] Sent SHOP_ACTION (action=%u, skin=%u)\n", action,
		   skin_index);
	client->error_message[0] = '\0';
	return true;
}

/**
 * @brief Process incoming EMAP messages from the server during a game session.
 *
 * @param client Pointer to the client whose state will be updated.
 * @param game Pointer to the game state to keep in sync with the server.
 * @param player Pointer to the player data used when sending final updates.
 * @return true on success, false if a fatal network error occurs.
 */
bool NetworkClient_ProcessMessages(NetworkClient* client, GameState* game,
								   PlayerData* player) {
	bool handleGameMessages =
		(client->state == NET_IN_GAME || client->state == NET_GAME_ENDED);
	bool allowPassivePoll = (client->state == NET_CONNECTED);

	if (!handleGameMessages && !allowPassivePoll) {
		return true;
	}

	EMAP_Message msg;
	uint8_t raw_payload[64];
	uint16_t raw_len = 0;

	while (true) {
		raw_len = 0;
		int result =
			recv_message_nonblocking(client->socket_fd, &msg, raw_payload,
									 &raw_len, sizeof(raw_payload));

		if (result == -2) {
			break; // No more data available right now
		}

		if (result < 0) {
			snprintf(client->error_message, sizeof(client->error_message),
					 "Error receiving message");
			client->state = NET_ERROR;
			return false;
		}

		switch (msg.type) {
		case EMAP_MSG_GENERAL_ACK:
			if (client->waiting_for_ack) {
				if (msg.data.general_ack.ack_type == EMAP_MESSAGE_ACK_OK) {
					client->waiting_for_ack = false;
					client->is_our_turn =
						false; // Turn is over after a successful move
					printf("[CLIENT] Move ACKed OK. Turn ended.\n");
				} else {
					snprintf(client->error_message,
							 sizeof(client->error_message),
							 "Move rejected by server");
					client->waiting_for_ack = false;
					printf("[CLIENT] Move REJECTED\n");
				}
			} else {
				printf("[CLIENT] Received ACK (%u)\n",
					   msg.data.general_ack.ack_type);
			}
			break;

		case EMAP_MSG_ILLEGAL_PLAY:
			if (!handleGameMessages) {
				emap_helper_send_ack(client->socket_fd, EMAP_MESSAGE_ACK_OK);
				break;
			}
			snprintf(client->error_message, sizeof(client->error_message),
					 "Illegal move - try again");
			client->waiting_for_ack = false;
			emap_helper_send_ack(client->socket_fd, EMAP_MESSAGE_ACK_OK);
			printf("[CLIENT] Received ILLEGAL_PLAY\n");
			break;

		case EMAP_MSG_GAME_STATE:
			if (handleGameMessages && raw_len >= 12) {
				printf("[CLIENT] Received GAME_STATE: ");
				for (int i = 0; i < 12 && i < raw_len; ++i) {
					game->board[i] = raw_payload[i];
					printf("%d ", game->board[i]);
				}
				printf("\n");
				game->selectedSlot = -1;
			} else if (!handleGameMessages) {
				printf("[CLIENT] Ignoring GAME_STATE outside a match\n");
			} else {
				printf("[CLIENT] Received partial GAME_STATE (%u bytes)\n",
					   raw_len);
			}
			emap_helper_send_ack(client->socket_fd, EMAP_MESSAGE_ACK_OK);
			break;

		case EMAP_MSG_PLAY:
			if (!handleGameMessages) {
				emap_helper_send_ack(client->socket_fd, EMAP_MESSAGE_ACK_OK);
				break;
			}
			client->is_our_turn =
				(msg.data.play.play_type == EMAP_PLAY_YOUR_TURN);
			game->currentPlayer = client->is_our_turn ? 1 : -1;
			emap_helper_send_ack(client->socket_fd, EMAP_MESSAGE_ACK_OK);
			printf("[CLIENT] Received PLAY. My turn: %s\n",
				   client->is_our_turn ? "YES" : "NO");
			break;

		case EMAP_MSG_GAME_END:
			if (!handleGameMessages) {
				break;
			}
			game->gameOver = true;
			game->winner =
				(msg.data.game_end.has_won == EMAP_END_GAME_WIN) ? 1 : -1;
			client->state = NET_CONNECTED;
			client->waiting_for_ack = false;
			client->is_our_turn = false;
			emap_helper_send_ack(client->socket_fd, EMAP_MESSAGE_ACK_OK);
			break;

		case EMAP_MSG_PLAYER_INFO:
			client_update_profile(player, &msg.data.player_info);
			emap_helper_send_ack(client->socket_fd, EMAP_MESSAGE_ACK_OK);
			break;

		case EMAP_MSG_PLAYER_DATA_UPDATE:
			client_update_profile(player, &msg.data.player_update);
			emap_helper_send_ack(client->socket_fd, EMAP_MESSAGE_ACK_OK);
			break;

		default:
			// Ignore unsupported message types for now
			break;
		}
	}

	return true;
}

/**
 * @brief Cleanly disconnect from the server, sending any final player updates
 * if necessary.
 *
 * @param client Pointer to the NetworkClient to disconnect.
 */
void NetworkClient_Disconnect(NetworkClient* client) {
	if (!client) {
		return;
	}

	if (client->socket_fd >= 0) {
		close(client->socket_fd);
		client->socket_fd = -1;
	}

	client->state = NET_DISCONNECTED;
	client->is_our_turn = false;
	client->waiting_for_ack = false;
	client->error_message[0] = '\0';
}

/**
 * @brief Query whether it is currently the client's turn to play.
 *
 * @param client Pointer to the active NetworkClient.
 * @return true if it is the client's turn and the client is in a game, false
 * otherwise.
 */
bool NetworkClient_IsOurTurn(NetworkClient* client) {
	return client->is_our_turn && client->state == NET_IN_GAME;
}

/**
 * @brief Retrieve the current network state of the client.
 *
 * @param client Pointer to the NetworkClient whose state is requested.
 * @return The current NetworkState enumeration value.
 */
NetworkState NetworkClient_GetState(NetworkClient* client) {
	return client->state;
}

/**
 * @brief Obtain the most recent network error message.
 *
 * @param client Pointer to the NetworkClient containing the error message.
 * @return A string describing the last network error, or an empty string if
 * none.
 */
const char* NetworkClient_GetError(NetworkClient* client) {
	return client->error_message;
}
