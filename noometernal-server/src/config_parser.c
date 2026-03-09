/**
 * @file config_parser.c
 * @brief Implementation of INI configuration file parser.
 *
 * Provides a simple INI parser that reads server configuration from a file.
 * Supports comments (lines starting with ';' or '#'), sections ([section]),
 * and key-value pairs (key = value).
 */

#include "config_parser.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** @brief Maximum line length for INI file parsing. */
#define MAX_LINE_LEN 256

/**
 * @brief Trim leading and trailing whitespace from a string in-place.
 *
 * Modifies the string by moving content and adding null terminator.
 *
 * @param[in,out] str String to trim.
 * @return Pointer to the trimmed string (same as input).
 */
static char* trim_whitespace(char* str) {
	if (str == NULL) {
		return NULL;
	}

	/* Trim leading whitespace */
	while (isspace((unsigned char)*str)) {
		str++;
	}

	if (*str == '\0') {
		return str;
	}

	/* Trim trailing whitespace */
	char* end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end)) {
		end--;
	}
	*(end + 1) = '\0';

	return str;
}

/**
 * @brief Check if a line is a comment or empty.
 *
 * @param[in] line The line to check.
 * @return 1 if line is a comment or empty, 0 otherwise.
 */
static int is_comment_or_empty(const char* line) {
	if (line == NULL || *line == '\0') {
		return 1;
	}
	if (*line == ';' || *line == '#') {
		return 1;
	}
	return 0;
}

/**
 * @brief Check if a line is a section header.
 *
 * @param[in] line The line to check.
 * @return 1 if line is a section header [section], 0 otherwise.
 */
static int is_section(const char* line) {
	if (line == NULL) {
		return 0;
	}
	return (*line == '[');
}

/**
 * @brief Parse a key-value pair from a line.
 *
 * Expects format: key = value
 *
 * @param[in]  line  The line to parse.
 * @param[out] key   Buffer to store the key (must be at least MAX_LINE_LEN).
 * @param[out] value Buffer to store the value (must be at least MAX_LINE_LEN).
 * @return 0 on success, -1 if no '=' found.
 */
static int parse_key_value(const char* line, char* key, char* value) {
	const char* eq = strchr(line, '=');
	if (eq == NULL) {
		return -1;
	}

	/* Copy key part */
	size_t key_len = (size_t)(eq - line);
	if (key_len >= MAX_LINE_LEN) {
		key_len = MAX_LINE_LEN - 1;
	}
	strncpy(key, line, key_len);
	key[key_len] = '\0';

	/* Copy value part */
	strncpy(value, eq + 1, MAX_LINE_LEN - 1);
	value[MAX_LINE_LEN - 1] = '\0';

	/* Trim both */
	char* trimmed_key = trim_whitespace(key);
	char* trimmed_value = trim_whitespace(value);

	/* Move trimmed content to beginning if needed */
	if (trimmed_key != key) {
		memmove(key, trimmed_key, strlen(trimmed_key) + 1);
	}
	if (trimmed_value != value) {
		memmove(value, trimmed_value, strlen(trimmed_value) + 1);
	}

	return 0;
}

/**
 * @brief Validate and copy IP address string.
 *
 * @param[out] dest Destination buffer.
 * @param[in]  src  Source IP string.
 * @param[in]  size Size of destination buffer.
 */
static void set_ip(char* dest, const char* src, size_t size) {
	if (strlen(src) > 0 && strlen(src) < size) {
		strncpy(dest, src, size - 1);
		dest[size - 1] = '\0';
	}
}

/**
 * @brief Validate and set port number.
 *
 * @param[out] dest Destination buffer.
 * @param[in]  src  Source port string.
 * @param[in]  size Size of destination buffer.
 */
static void set_port(char* dest, const char* src, size_t size) {
	long port = strtol(src, NULL, 10);
	if (port > 0 && port <= 65535) {
		strncpy(dest, src, size - 1);
		dest[size - 1] = '\0';
	}
}

/**
 * @brief Validate and set backlog value.
 *
 * @param[out] dest Pointer to backlog integer.
 * @param[in]  src  Source backlog string.
 */
static void set_backlog(int* dest, const char* src) {
	long backlog = strtol(src, NULL, 10);
	if (backlog > 0 && backlog <= 128) {
		*dest = (int)backlog;
	}
}

void config_init_defaults(ServerConfig* config) {
	if (config == NULL) {
		return;
	}

	strncpy(config->ip, CONFIG_DEFAULT_IP, CONFIG_IP_MAX_LEN - 1);
	config->ip[CONFIG_IP_MAX_LEN - 1] = '\0';

	strncpy(config->port, CONFIG_DEFAULT_PORT, CONFIG_PORT_MAX_LEN - 1);
	config->port[CONFIG_PORT_MAX_LEN - 1] = '\0';

	config->backlog = CONFIG_DEFAULT_BACKLOG;
}

int config_load(const char* filepath, ServerConfig* config) {
	if (filepath == NULL || config == NULL) {
		return -1;
	}

	/* Initialize with defaults first */
	config_init_defaults(config);

	FILE* fp = fopen(filepath, "r");
	if (fp == NULL) {
		/* File not found is not an error - use defaults */
		fprintf(stderr, "[CONFIG] Warning: '%s' not found, using defaults\n",
				filepath);
		return 0;
	}

	char line[MAX_LINE_LEN];
	char key[MAX_LINE_LEN];
	char value[MAX_LINE_LEN];
	int in_server_section = 0;

	while (fgets(line, sizeof(line), fp) != NULL) {
		char* trimmed = trim_whitespace(line);

		if (is_comment_or_empty(trimmed)) {
			continue;
		}

		if (is_section(trimmed)) {
			/* Check if entering [server] section */
			in_server_section = (strncmp(trimmed, "[server]", 8) == 0);
			continue;
		}

		if (!in_server_section) {
			continue;
		}

		if (parse_key_value(trimmed, key, value) == 0) {
			if (strcmp(key, "ip") == 0) {
				set_ip(config->ip, value, CONFIG_IP_MAX_LEN);
			} else if (strcmp(key, "port") == 0) {
				set_port(config->port, value, CONFIG_PORT_MAX_LEN);
			} else if (strcmp(key, "backlog") == 0) {
				set_backlog(&config->backlog, value);
			}
		}
	}

	fclose(fp);
	return 0;
}

void config_print(const ServerConfig* config) {
	if (config == NULL) {
		return;
	}

	printf("=== Server Configuration ===\n");
	printf("  IP Address : %s\n", config->ip);
	printf("  Port       : %s\n", config->port);
	printf("  Backlog    : %d\n", config->backlog);
	printf("============================\n");
}
