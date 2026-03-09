/**
 * @file server_connection.c
 * @brief Implementation of TCP socket connection management.
 *
 * Provides low-level socket operations for establishing listening sockets and
 * accepting client connections. Implements proper error handling and resource
 * cleanup with getaddrinfo for portable address resolution.
 */

#include "server_connection.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * @brief Implementation of srv_set_reuseaddr.
 *
 * Sets the SO_REUSEADDR socket option to allow binding to addresses in
 * TIME_WAIT state. This prevents "Address already in use" errors when
 * restarting the server.
 *
 * @param fd Socket file descriptor.
 * @return 0 on success, -1 on failure.
 */
int srv_set_reuseaddr(int fd) {
	int yes = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
		return -1;
	}
	return 0;
}

/**
 * @brief Implementation of srv_create_listener.
 *
 * Creates a listening TCP socket by:
 * 1. Resolving the host:port using getaddrinfo()
 * 2. Creating a socket with the resolved address family
 * 3. Setting SO_REUSEADDR to avoid bind errors
 * 4. Binding the socket to the specified address
 * 5. Marking the socket as listening
 *
 * Cleanup is performed on all error paths.
 *
 * @param host IP address or hostname to bind.
 * @param port Service name or numeric port.
 * @param backlog Connection queue length.
 * @return Socket file descriptor on success, -1 on failure.
 */
int srv_create_listener(const char* host, const char* port, int backlog) {
	struct addrinfo hints, *result = NULL;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;

	int status = getaddrinfo(host, port, &hints, &result);
	if (status != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return -1;
	}

	int sock_fd =
		socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (sock_fd == -1) {
		perror("socket");
		freeaddrinfo(result);
		return -1;
	}

	if (srv_set_reuseaddr(sock_fd) == -1) {
		perror("setsockopt SO_REUSEADDR");
		freeaddrinfo(result);
		close(sock_fd);
		return -1;
	}

	if (bind(sock_fd, (struct sockaddr*)result->ai_addr, result->ai_addrlen) ==
		-1) {
		perror("bind");
		freeaddrinfo(result);
		close(sock_fd);
		return -1;
	}

	freeaddrinfo(result);

	if (listen(sock_fd, backlog) == -1) {
		perror("listen");
		close(sock_fd);
		return -1;
	}

	return sock_fd;
}

/**
 * @brief Implementation of srv_accept_client.
 *
 * Accepts an incoming connection on the listening socket. Optionally returns
 * the client's address information if addr and addrlen pointers are provided.
 *
 * Uses temporary local storage if caller does not provide addr/addrlen
 * pointers, allowing this function to be called with NULL parameters.
 *
 * @param listen_fd Listening socket file descriptor.
 * @param addr Pointer to sockaddr_storage or NULL.
 * @param addrlen Pointer to address length or NULL.
 * @return Connected client socket on success, -1 on error.
 */
int srv_accept_client(int listen_fd, struct sockaddr_storage* addr,
					  socklen_t* addrlen) {
	struct sockaddr_storage tmp_addr;
	socklen_t tmp_len = sizeof(tmp_addr);
	if (!addr)
		addr = &tmp_addr;
	if (!addrlen)
		addrlen = &tmp_len;
	int fd = accept(listen_fd, (struct sockaddr*)addr, addrlen);

	if (fd >= 0) {
		int yes = 1;
		// Enable TCP keepalive to detect dead connections
		if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1) {
			perror("setsockopt SO_KEEPALIVE");
		}
		// Disable Nagle's algorithm for lower latency
		if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) == -1) {
			perror("setsockopt TCP_NODELAY");
		}
	}

	return fd; /* -1 on error, caller handles perror if desired */
}

/**
 * @brief Implementation of srv_close_fd.
 *
 * Safely closes a socket file descriptor. Silently ignores negative descriptors
 * and does not report close() errors to allow idempotent calls.
 *
 * @param fd Socket file descriptor.
 */
void srv_close_fd(int fd) {
	if (fd >= 0) {
		close(fd);
	}
}
