/**
 * @file main.c
 * @brief NoomEternal Game Server - Main Entry Point
 *
 * This is the main server application that:
 * 1. Loads configuration from server.ini
 * 2. Initializes a TCP listening socket on configured IP:port
 * 3. Accepts incoming client connections
 * 4. Spawns a thread for each client to handle their complete session
 * 5. Maintains a shared two-player lobby for match coordination
 *
 * Architecture:
 * - Single-threaded accept loop in main()
 * - Multi-threaded client handling (one thread per client)
 * - Thread-safe lobby using mutex and condition variables
 * - Graceful error handling with per-client isolation
 *
 * @author NoomEternal Development Team
 * @version 1.0
 * @date 2026-01-08
 */

#include "config_parser.h"
#include "game_logic.h"
#include "libemap.h"
#include "server_connection.h"
#include "server_worker.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/** @brief Maximum pending connection queue length. */
#define BACKLOG 2

/**
 * @brief Main server entry point.
 *
 * Execution flow:
 * 1. Load configuration from server.ini (or use defaults)
 * 2. Configure signal handling (ignore SIGPIPE for client disconnections)
 * 3. Seed the random number generator for game board generation
 * 4. Create a listening socket on configured IP:port
 * 5. Initialize the shared lobby structure with mutex and condition variable
 * 6. Enter accept loop:
 *    - Accept incoming client connections
 *    - Create Thread_args structure
 *    - Spawn detached client handling thread
 *    - Return to listening
 *
 * The server continues indefinitely, accepting connections until interrupted.
 * Clients are isolated; connection/thread failures don't affect other clients.
 *
 * Lobby Details:
 * - Shared by all clients for match coordination
 * - Holds state for two-player matches (board, turn, scores)
 * - Protected by mutex to ensure thread safety
 * - Signaled via condition variable when state changes
 *
 * @return EXIT_FAILURE if listener socket creation fails.
 *         Never returns on success (infinite accept loop).
 */
int main(void) {
	setbuf(stdout, NULL); /* Disable stdout buffering */
	signal(SIGPIPE, SIG_IGN);
	srandom((unsigned)time(NULL));

	/* Load configuration from server.ini */
	ServerConfig config;
	if (config_load(CONFIG_DEFAULT_PATH, &config) != 0) {
		fprintf(stderr, "Failed to load configuration\n");
		config_init_defaults(&config);
	}
	config_print(&config);

	int sock_fd, new_fd;
	struct sockaddr_storage client_addr;
	socklen_t addr_size;
	sock_fd = srv_create_listener(config.ip, config.port, config.backlog);
	if (sock_fd == -1) {
		fprintf(stderr, "Failed to create listener on %s:%s\n", config.ip,
				config.port);
		exit(EXIT_FAILURE);
	}

	printf("[SERVER] Listening on %s:%s\n", config.ip, config.port);

	static ServerLobby lobby;
	pthread_mutex_init(&lobby.mutex, NULL);
	pthread_cond_init(&lobby.cond, NULL);
	lobby.count = 0;
	lobby.game_started = 0;
	lobby.game_over = 0;
	lobby.current_turn = 0;
	lobby.winner_id = -1;

	// Initialize game board using game logic module
	game_init_board(lobby.board);

	for (;;) {
		addr_size = sizeof(client_addr);
		new_fd = srv_accept_client(sock_fd, &client_addr, &addr_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		Thread_args* arg = (Thread_args*)malloc(sizeof(Thread_args));
		if (!arg) {
			srv_close_fd(new_fd);
			continue;
		}
		arg->fd = new_fd;
		arg->lobby = &lobby;
		arg->id = -1; // Will be assigned in thread

		pthread_t th;
		if (pthread_create(&th, NULL, server_client_thread, arg) != 0) {
			srv_close_fd(new_fd);
			free(arg);
			continue;
		}
		pthread_detach(th);
	}

	srv_close_fd(sock_fd);
	return 0;
}