#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"
#include <stdbool.h>

// Structure pour stocker la configuration du client
/**
 * @brief Configuration settings persisted between game sessions for the client.
 *
 * Only local preferences that outlive a session are stored here: audio levels,
 * network endpoint hints, and lightweight display toggles. Player progression
 * now lives entirely on the server.
 *
 * @param musicVolume Master volume level for music playback.
 * @param sfxVolume Master volume level for sound effects.
 * @param musicEnabled Toggles music playback on or off.
 * @param serverIP Null-terminated string containing the target server’s IP
 * address.
 * @param serverPort TCP/UDP port used to connect to the server.
 * @param showFPS Enables or disables the on-screen frames-per-second counter.
 */
typedef struct {
	float musicVolume;
	float sfxVolume;
	bool musicEnabled;

	char serverIP[64];
	int serverPort;

	bool showFPS;
} ClientConfig;

/**
 * @brief Initialize configuration with default values.
 *
 * Sets sensible defaults for all configuration fields. Called when no
 * configuration file exists or the user requests a reset.
 *
 * @param config Pointer to ClientConfig to initialize.
 */
void InitDefaultConfig(ClientConfig* config);

/**
 * @brief Load configuration from INI file.
 *
 * Reads client settings from disk. Falls back to defaults if file not found.
 * Supports [Audio], [Network], and [Display] sections.
 *
 * @param config Pointer to ClientConfig to populate.
 * @param filename Path to the INI file.
 * @return true if loaded successfully, false if defaults were used.
 */
bool LoadConfig(ClientConfig* config, const char* filename);

/**
 * @brief Save configuration to INI file.
 *
 * Persists the current configuration to disk in INI format.
 *
 * @param config Pointer to ClientConfig to save.
 * @param filename Path where the INI file will be written.
 * @return true if saved successfully, false on I/O error.
 */
bool SaveConfig(const ClientConfig* config, const char* filename);

#endif // CONFIG_H
