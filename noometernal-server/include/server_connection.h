/**
 * @file server_connection.h
 * @brief Low-level TCP socket connection management for the NoomEternal game
 * server.
 *
 * This module provides fundamental socket operations for establishing,
 * accepting, and managing TCP connections. It abstracts platform-specific
 * socket API calls and ensures proper resource cleanup.
 *
 * @author NoomEternal Development Team
 * @version 1.0
 * @date 2026-01-08
 */

#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

/**
 * @brief Create a listening TCP socket on the specified host and port.
 *
 * Initializes a passive TCP socket (listener) that can accept incoming client
 * connections. The socket is configured with SO_REUSEADDR to allow immediate
 * rebinding after server restart.
 *
 * @param[in] host The IP address to bind to (e.g., "0.0.0.0" for all
 * interfaces, "127.0.0.1" for localhost).
 * @param[in] port The port number as a string (e.g., "5001"). Must be a valid
 *                  numeric port within the range 1-65535.
 * @param[in] backlog The maximum number of pending connections that can be
 * queued. Typical values range from 1 to 128.
 *
 * @return On success: a valid file descriptor (≥0) representing the listening
 * socket.
 * @return On failure: -1 (errno is set to indicate the specific error).
 *
 * @retval -1 Errors from getaddrinfo(), socket(), setsockopt(), bind(), or
 * listen().
 *
 * @note The caller is responsible for closing the returned socket descriptor
 *       using srv_close_fd() or close().
 *
 * @see srv_accept_client(), srv_close_fd(), srv_set_reuseaddr()
 */
int srv_create_listener(const char* host, const char* port, int backlog);

/**
 * @brief Accept an incoming client connection on a listening socket.
 *
 * Blocks until a client connection is available or an error occurs. When a
 * connection is accepted, the client's address information is optionally
 * returned.
 *
 * @param[in] listen_fd The listening socket file descriptor obtained from
 *                       srv_create_listener().
 * @param[out] addr Optional pointer to a sockaddr_storage structure. If
 * provided, the client's address will be stored here. Can be NULL if the client
 * address is not needed.
 * @param[in,out] addrlen Optional pointer to the size of the addr buffer.
 *                        Must be set to sizeof(struct sockaddr_storage) before
 * call. Upon return, contains the actual address length. Can be NULL if addr is
 * NULL.
 *
 * @return On success: a valid file descriptor (≥0) representing the connected
 * client.
 * @return On failure: -1 (errno is set).
 *
 * @retval -1 Error from accept() system call.
 *
 * @note The caller is responsible for closing the returned client socket.
 * @note This function is blocking and will not return until a connection
 * arrives.
 *
 * @see srv_create_listener(), srv_close_fd()
 */
int srv_accept_client(int listen_fd, struct sockaddr_storage* addr,
					  socklen_t* addrlen);

/**
 * @brief Enable socket address reuse for a given socket descriptor.
 *
 * Sets the SO_REUSEADDR socket option, allowing the socket to bind to an
 * address in the TIME_WAIT state. This is essential for server restart
 * scenarios where immediate rebinding is required.
 *
 * @param[in] fd A valid socket file descriptor.
 *
 * @return 0 on success.
 * @return -1 on failure (errno is set).
 *
 * @note This function should be called before bind() for it to have effect.
 * @note Particularly useful in development and testing environments.
 *
 * @see srv_create_listener()
 */
int srv_set_reuseaddr(int fd);

/**
 * @brief Close a socket descriptor safely.
 *
 * Closes the socket associated with the file descriptor. If the descriptor is
 * invalid (< 0), this function silently returns without attempting close().
 *
 * @param[in] fd A socket file descriptor. Can be any value; negative values are
 * ignored.
 *
 * @return void. Errors from close() are ignored.
 *
 * @note Safe to call multiple times with the same descriptor due to error
 * suppression.
 * @note This function does NOT check if close() succeeded, so subsequent
 * operations on the same descriptor should not be attempted.
 *
 * @see srv_create_listener(), srv_accept_client()
 */
void srv_close_fd(int fd);

#endif /* SERVER_CONNECTION_H */