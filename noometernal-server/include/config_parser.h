/**
 * @file config_parser.h
 * @brief INI configuration file parser for NoomEternal server.
 *
 * This module provides functionality to parse server configuration from
 * an INI-style configuration file (server.ini). It handles IP address,
 * port, and backlog settings with sensible defaults.
 *
 * @author NoomEternal Development Team
 * @version 1.0
 * @date 2026-01-12
 */

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <stdint.h>

/** @brief Maximum length for IP address string (including null terminator). */
#define CONFIG_IP_MAX_LEN 46

/** @brief Maximum length for port string (including null terminator). */
#define CONFIG_PORT_MAX_LEN 6

/** @brief Default configuration file path. */
#define CONFIG_DEFAULT_PATH "config/server.ini"

/** @brief Default IP address to bind to. */
#define CONFIG_DEFAULT_IP "0.0.0.0"

/** @brief Default port number. */
#define CONFIG_DEFAULT_PORT "5001"

/** @brief Default connection backlog. */
#define CONFIG_DEFAULT_BACKLOG 2

/**
 * @brief Server configuration structure.
 *
 * Holds all configurable server parameters loaded from the INI file.
 */
typedef struct {
	/** @brief IP address to bind to (e.g., "0.0.0.0", "127.0.0.1"). */
	char ip[CONFIG_IP_MAX_LEN];

	/** @brief Port number as string (e.g., "5001"). */
	char port[CONFIG_PORT_MAX_LEN];

	/** @brief Maximum pending connections queue length. */
	int backlog;
} ServerConfig;

/**
 * @brief Initialize a ServerConfig structure with default values.
 *
 * Sets the configuration to safe defaults:
 * - IP: "0.0.0.0" (all interfaces)
 * - Port: "5001"
 * - Backlog: 2
 *
 * @param[out] config Pointer to the configuration structure to initialize.
 */
void config_init_defaults(ServerConfig* config);

/**
 * @brief Load server configuration from an INI file.
 *
 * Parses the specified INI file and populates the ServerConfig structure.
 * If the file cannot be opened or a key is missing, default values are used.
 *
 * Supported INI format:
 * @code
 * [server]
 * ip = 0.0.0.0
 * port = 5001
 * backlog = 2
 * @endcode
 *
 * @param[in]  filepath Path to the INI configuration file.
 * @param[out] config   Pointer to the configuration structure to populate.
 *
 * @return 0 on success (file parsed, even if some defaults were used).
 * @return -1 if filepath or config is NULL.
 *
 * @note If the file does not exist, defaults are used and 0 is returned.
 * @note Comments starting with ';' or '#' are ignored.
 */
int config_load(const char* filepath, ServerConfig* config);

/**
 * @brief Print the current configuration to stdout.
 *
 * Displays all configuration values in a human-readable format.
 * Useful for debugging and startup logging.
 *
 * @param[in] config Pointer to the configuration structure to print.
 */
void config_print(const ServerConfig* config);

#endif /* CONFIG_PARSER_H */
