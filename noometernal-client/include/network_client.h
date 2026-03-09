/**
 * @file network_client.h
 * @brief EMAP protocol client for network communication.
 *
 * Manages network connections to the game server using the EMAP protocol.
 * Handles non-blocking socket I/O, handshakes, matchmaking, gameplay sync,
 * and graceful error recovery.
 */

#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include "types.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Network connection lifecycle states.
 *
 * Tracks the progression of a client's connection from initial connection
 * through handshake, lobby wait, active gameplay, and game completion or error.
 */
typedef enum {
	NET_DISCONNECTED, /**< No active connection. */
	NET_CONNECTING,	  /**< Attempting to establish TCP connection. */
	NET_CONNECTED,	  /**< Connected; awaiting handshake response. */
	NET_IN_LOBBY,	  /**< Handshake complete; queued for matchmaking. */
	NET_IN_GAME,	  /**< Match started; gameplay in progress. */
	NET_GAME_ENDED,	  /**< Match completed; ready for next cycle. */
	NET_ERROR		  /**< Fatal error; connection cannot be recovered. */
} NetworkState;

/**
 * @brief Client-side network context for EMAP communication.
 *
 * Maintains the state of a connection to the EMAP server, including socket,
 * connection state, turn information, and error messages.
 */
typedef struct NetworkClient {
	int socket_fd;			 /**< TCP socket file descriptor. */
	NetworkState state;		 /**< Current connection lifecycle state. */
	bool is_our_turn;		 /**< true if the client is the active player. */
	bool waiting_for_ack;	 /**< true if awaiting move acknowledgment. */
	char error_message[256]; /**< Description of last error (if any). */
	uint8_t player_id;		 /**< Server-assigned player ID for this session. */
} NetworkClient;

/**
 * @brief Initialize the network client structure.
 *
 * Resets all fields to their default/invalid state. Called before connecting
 * or when resetting after a failed connection.
 *
 * @param client Pointer to NetworkClient to initialize.
 */
void NetworkClient_Init(NetworkClient* client);

/**
 * @brief Establish connection to EMAP server and perform handshake.
 *
 * Initiates a TCP connection to the server and exchanges handshake messages
 * to identify the player and sync initial profile data. Non-blocking on socket
 * operations (with timeout). Updates player profile on success.
 *
 * @param client Pointer to NetworkClient structure.
 * @param server_ip Server IP address or hostname.
 * @param port Server TCP port.
 * @param username Player username to send in handshake.
 * @param player Pointer to PlayerData; populated from server response.
 * @return true if connection and handshake successful, false on failure
 *         (error_message set).
 */
bool NetworkClient_Connect(NetworkClient* client, const char* server_ip,
						   int port, const char* username, PlayerData* player);

/**
 * @brief Request entry into matchmaking queue.
 *
 * Sends a join request message to the server. Must be called after successful
 * connection to progress from NET_CONNECTED to NET_IN_LOBBY state.
 *
 * @param client Pointer to NetworkClient in NET_CONNECTED state.
 * @return true if message sent successfully, false on send error.
 */
bool NetworkClient_JoinGame(NetworkClient* client);

/**
 * @brief Poll the server while waiting for game start.
 *
 * Non-blocking call that checks for game-start messages from the server.
 * Returns true while still in the lobby, false when game starts or on error.
 * Updates game state when a game-start message is received.
 *
 * @param client Pointer to NetworkClient in NET_IN_LOBBY state.
 * @param game Pointer to GameState; initialized when game starts.
 * @return true if still waiting or on recoverable notification, false when
 *         game has started or fatal error occurs.
 */
bool NetworkClient_WaitForGame(NetworkClient* client, GameState* game);

/**
 * @brief Send a coin move to the server during gameplay.
 *
 * Encodes and sends a move message. Move is only valid if it's the client's
 * turn (is_our_turn true). Server will send acknowledgment or rejection.
 *
 * @param client Pointer to NetworkClient in NET_IN_GAME state.
 * @param coin_index Index of the coin being moved (0-11 on board).
 * @param target_pos Target board position (0-11).
 * @return true if move message sent successfully, false on error.
 */
bool NetworkClient_SendMove(NetworkClient* client, uint8_t coin_index,
							uint8_t target_pos);

/**
 * @brief Send a shop/cosmetic action to the server.
 *
 * Requests a purchase or equip action (skin selection, coin purchase).
 * The action code and target skin determine the operation on the server.
 *
 * @param client Pointer to active NetworkClient.
 * @param action EMAP shop action code.
 * @param skin_index Target skin index (for equip actions).
 * @return true if request message sent, false on error.
 */
bool NetworkClient_SendShopAction(NetworkClient* client, uint8_t action,
								  uint8_t skin_index);

/**
 * @brief Process all pending incoming messages from the server.
 *
 * Non-blocking read of available data on the socket. Parses EMAP messages
 * and updates game state, player profile, and turn information accordingly.
 * Handles various message types: turn notifications, move results, player
 * updates.
 *
 * @param client Pointer to NetworkClient whose messages will be processed.
 * @param game Pointer to GameState to update with server board/turn changes.
 * @param player Pointer to PlayerData to update with server profile changes.
 * @return true if processing succeeded, false if fatal network error occurs.
 */
bool NetworkClient_ProcessMessages(NetworkClient* client, GameState* game,
								   PlayerData* player);

/**
 * @brief Disconnect from the server and reset state.
 *
 * Closes the socket, sends a disconnect notification if applicable, and
 * resets all state fields. Safe to call multiple times.
 *
 * @param client Pointer to NetworkClient to disconnect.
 */
void NetworkClient_Disconnect(NetworkClient* client);

/**
 * @brief Query whether it is currently the client's turn.
 *
 * @param client Pointer to NetworkClient.
 * @return true if is_our_turn is set and client is in NET_IN_GAME state.
 */
bool NetworkClient_IsOurTurn(NetworkClient* client);

/**
 * @brief Retrieve the current network connection state.
 *
 * @param client Pointer to NetworkClient.
 * @return Current NetworkState enumeration value.
 */
NetworkState NetworkClient_GetState(NetworkClient* client);

/**
 * @brief Retrieve the most recent error message.
 *
 * @param client Pointer to NetworkClient.
 * @return Pointer to a null-terminated error string (empty if no error).
 */
const char* NetworkClient_GetError(NetworkClient* client);

#endif // NETWORK_CLIENT_H
