/**
 * @file config.c
 * @brief Implementation of client configuration persistence.
 *
 * Provides loading, saving, and initialization of client-side settings from
 * INI files. Supports audio preferences, network endpoint configuration, and
 * display settings.
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Initialize the client configuration with default values.
 *
 * Sets all configuration fields to reasonable defaults. Called when no config
 * file exists or cannot be read.
 *
 * Defaults:
 * - Music volume: 50%, enabled
 * - SFX volume: 70%
 * - Server: localhost:5001
 * - FPS display: off
 *
 * @param config Pointer to ClientConfig to initialize.
 */
void InitDefaultConfig(ClientConfig* config) {
	config->musicVolume = 0.5f;
	config->sfxVolume = 0.7f;
	config->musicEnabled = true;

	strncpy(config->serverIP, "127.0.0.1", sizeof(config->serverIP) - 1);
	config->serverPort = 5001;

	config->showFPS = false;
}

/**
 * @brief Load the client configuration from an INI file.
 *
 * Reads configuration from an INI-format file with sections [Audio], [Network],
 * and [Display]. If the file does not exist, falls back to defaults.
 * Parses key=value pairs and populates the ClientConfig structure.
 *
 * @param config Pointer to ClientConfig to populate.
 * @param filename Path to the INI file.
 * @return true if file loaded successfully, false if defaults were used.
 */
bool LoadConfig(ClientConfig* config, const char* filename) {
	FILE* file = fopen(filename, "r");
	if (!file) {
		printf("Config file not found, using defaults: %s\n", filename);
		InitDefaultConfig(config);
		return false;
	}

	char line[256];
	char section[64] = "";

	while (fgets(line, sizeof(line), file)) {
		// Enlever le retour à la ligne
		line[strcspn(line, "\r\n")] = 0;

		// Ignorer les lignes vides et les commentaires
		if (line[0] == '\0' || line[0] == ';' || line[0] == '#')
			continue;

		// Détecter une section [Section]
		if (line[0] == '[') {
			char* end = strchr(line, ']');
			if (end) {
				*end = '\0';
				strncpy(section, line + 1, sizeof(section) - 1);
			}
			continue;
		}

		// Parser une ligne clé=valeur
		char* equals = strchr(line, '=');
		if (!equals)
			continue;

		*equals = '\0';
		char* key = line;
		char* value = equals + 1;

		// Enlever les espaces au début/fin
		while (*key == ' ' || *key == '\t')
			key++;
		while (*value == ' ' || *value == '\t')
			value++;

		// Parser selon la section
		if (strcmp(section, "Audio") == 0) {
			if (strcmp(key, "MusicVolume") == 0)
				config->musicVolume = atof(value);
			else if (strcmp(key, "SFXVolume") == 0)
				config->sfxVolume = atof(value);
			else if (strcmp(key, "MusicEnabled") == 0)
				config->musicEnabled =
					(strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
		} else if (strcmp(section, "Network") == 0) {
			if (strcmp(key, "ServerIP") == 0)
				strncpy(config->serverIP, value, sizeof(config->serverIP) - 1);
			else if (strcmp(key, "ServerPort") == 0)
				config->serverPort = atoi(value);
		} else if (strcmp(section, "Game") == 0 ||
				   strcmp(section, "Display") == 0) {
			if (strcmp(key, "ShowFPS") == 0)
				config->showFPS = (strcmp(value, "true") == 0 ||
								  strcmp(value, "1") == 0);
		}
	}

	fclose(file);
	printf("Config loaded from: %s\n", filename);
	return true;
}

/**
 * @brief Save the client configuration to an INI file.
 *
 * Writes the current ClientConfig to an INI-format file organized by
 * sections. Creates the file if it does not exist. Outputs human-readable
 * key=value pairs.
 *
 * @param config Pointer to ClientConfig to save.
 * @param filename Path where the INI file will be written.
 * @return true if file saved successfully, false on I/O error.
 */
bool SaveConfig(const ClientConfig* config, const char* filename) {
	FILE* file = fopen(filename, "w");
	if (!file) {
		printf("ERROR: Could not save config to: %s\n", filename);
		return false;
	}

	// Section Audio
	fprintf(file, "[Audio]\n");
	fprintf(file, "MusicVolume=%.2f\n", config->musicVolume);
	fprintf(file, "SFXVolume=%.2f\n", config->sfxVolume);
	fprintf(file, "MusicEnabled=%s\n\n",
			config->musicEnabled ? "true" : "false");

	// Section Network
	fprintf(file, "[Network]\n");
	fprintf(file, "ServerIP=%s\n", config->serverIP);
	fprintf(file, "ServerPort=%d\n\n", config->serverPort);

	// Section Display
	fprintf(file, "[Display]\n");
	fprintf(file, "ShowFPS=%s\n", config->showFPS ? "true" : "false");

	fclose(file);
	printf("Config saved to: %s\n", filename);
	return true;
}
