/**
 * @file db_manager.c
 * @brief Implementation of player profile persistence and management.
 *
 * Manages a binary database file storing player profiles indexed by username.
 * Implements record normalization to ensure data consistency, particularly for
 * cosmetic skin ownership and selection.
 *
 * Database Format:
 * - File: db/player_db.bin (binary append-only file)
 * - Records: Fixed-size PlayerRecord structures
 * - Index: Linear scan by username (no indexing)
 * - Upsert: In-place update if found, append if new
 *
 * Reward Configuration:
 * - XP_PER_LEVEL: 1000 (progression wraps at this)
 * - XP_REWARD_WIN: 200
 * - XP_REWARD_LOSE: 50
 * - SOUL_REWARD_WIN: 250
 * - SOUL_REWARD_LOSE: 100
 * - DEFAULT_SOULS: 100 (starting currency for new players)
 */

#include "db_manager.h"
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PLAYER_DB_PATH "db/player_db.bin"
#define XP_PER_LEVEL 1000
#define XP_REWARD_WIN 200
#define XP_REWARD_LOSE 50
#define SOUL_REWARD_WIN 250
#define SOUL_REWARD_LOSE 100
#define DEFAULT_SOULS 100

/**
 * @brief Internal representation of a player record.
 *
 * Mirrors EMAP_PayloadPlayerInfo but uses different field names and
 * owns skin tracking (boolean array vs. bit flags).
 *
 * @internal
 */
typedef struct {
	char username[MAX_PLAYER_USERNAME_SIZE];
	uint8_t level;
	uint16_t progression;
	uint16_t money;
	uint8_t owned_skins[TOTAL_SKIN_AMOUNT];
	uint8_t selected_skin;
} PlayerRecord;

/**
 * @brief Normalize a player record to ensure consistency.
 *
 * Performs consistency checks and corrections:
 * 1. If no skins exist, ensures skin 0 is owned
 * 2. Ensures at least one skin is marked owned
 * 3. Corrects selected_skin if invalid or not owned
 *
 * This function is called after every profile load/update to maintain
 * invariants that the client code depends on.
 *
 * @param record Player record to normalize.
 *
 * @internal
 */
static void normalize_record(PlayerRecord* record) {
	if (TOTAL_SKIN_AMOUNT == 0) {
		record->selected_skin = 0;
		return;
	}

	uint8_t fallback = 0;
	bool has_owned = false;

	for (uint8_t i = 0; i < TOTAL_SKIN_AMOUNT; ++i) {
		if (record->owned_skins[i]) {
			fallback = i;
			has_owned = true;
			break;
		}
	}

	if (!has_owned) {
		record->owned_skins[0] = 1;
		fallback = 0;
	}

	if (record->selected_skin >= TOTAL_SKIN_AMOUNT ||
		record->owned_skins[record->selected_skin] == 0) {
		record->selected_skin = fallback;
	}
}

/**
 * @brief Convert internal PlayerRecord to external EMAP payload format.
 *
 * Transforms the internal boolean array skin ownership to EMAP's bit-flag
 * format:
 * - 0: not owned
 * - 1: owned (not selected)
 * - 2: owned and selected
 *
 * @param record Internal player record.
 * @param out_info Output payload structure (must be allocated).
 *
 * @internal
 */
static void record_to_payload(const PlayerRecord* record,
							  EMAP_PayloadPlayerInfo* out_info) {
	memset(out_info, 0, sizeof(*out_info));
	out_info->level = record->level;
	out_info->progression = record->progression;
	out_info->money = record->money;
	out_info->selected_skin = record->selected_skin;

	for (uint8_t i = 0; i < TOTAL_SKIN_AMOUNT; ++i) {
		if (!record->owned_skins[i]) {
			out_info->possessed_skins[i] = 0;
			continue;
		}
		out_info->possessed_skins[i] = (record->selected_skin == i) ? 2 : 1;
	}
}

/**
 * @brief Convert external EMAP payload to internal PlayerRecord format.
 *
 * Inverse of record_to_payload(). Extracts skin ownership from EMAP's
 * bit-flag format and tracks the selected skin.
 *
 * @param username Player username to store.
 * @param info Payload structure from client.
 * @param record Output internal record (must be allocated).
 *
 * @internal
 */
static void payload_to_record(const char* username,
							  const EMAP_PayloadPlayerInfo* info,
							  PlayerRecord* record) {
	memset(record, 0, sizeof(*record));
	strncpy(record->username, username, sizeof(record->username) - 1);
	record->level = info->level;
	record->progression = info->progression;
	record->money = info->money;
	record->selected_skin = info->selected_skin;

	uint8_t fallback = 0;
	bool has_owned = false;
	for (uint8_t i = 0; i < TOTAL_SKIN_AMOUNT; ++i) {
		uint8_t raw = info->possessed_skins[i];
		if (raw) {
			record->owned_skins[i] = 1;
			if (!has_owned) {
				fallback = i;
				has_owned = true;
			}
		}
	}

	if (record->selected_skin >= TOTAL_SKIN_AMOUNT ||
		!record->owned_skins[record->selected_skin]) {
		record->selected_skin = has_owned ? fallback : 0;
	}
	normalize_record(record);
}

/**
 * @brief Initialize a new player record with default values.
 *
 * Used when a player connects for the first time. Sets:
 * - Level: 1
 * - Progression: 0
 * - Money: DEFAULT_SOULS
 * - Skin ownership: skin 0 only
 * - Selected skin: skin 0
 *
 * @param record Output record to initialize.
 * @param username Player username to assign.
 *
 * @internal
 */
static void init_default_record(PlayerRecord* record, const char* username) {
	memset(record, 0, sizeof(*record));
	strncpy(record->username, username, sizeof(record->username) - 1);
	record->level = 1;
	record->progression = 0;
	record->money = DEFAULT_SOULS;
	if (TOTAL_SKIN_AMOUNT > 0) {
		record->owned_skins[0] = 1;
		record->selected_skin = 0;
	}
}

/**
 * @brief Load a player record from disk by username.
 *
 * Performs linear scan of the database file, looking for a record with
 * matching username. Applies normalization to the loaded record.
 *
 * @param username Username to search for.
 * @param out_record Output record (must be allocated).
 * @return true if found, false otherwise.
 *
 * @internal
 */
static bool fetch_player_record(const char* username,
								PlayerRecord* out_record) {
	FILE* file = fopen(PLAYER_DB_PATH, "rb");
	if (!file) {
		return false;
	}

	PlayerRecord record;
	while (fread(&record, sizeof(record), 1, file) == 1) {
		if (strncmp(record.username, username, sizeof(record.username)) == 0) {
			normalize_record(&record);
			fclose(file);
			*out_record = record;
			return true;
		}
	}

	fclose(file);
	return false;
}

/**
 * @brief Insert or update a player record in the database.
 *
 * If a record with matching username exists, it is updated in-place.
 * Otherwise, the record is appended to the file.
 *
 * Files are created if they don't exist (r+b → w+b fallback).
 *
 * @param record Record to insert or update.
 * @return true on success, false on error.
 *
 * @internal
 */
static bool upsert_player_record(const PlayerRecord* record) {
	FILE* file = fopen(PLAYER_DB_PATH, "r+b");
	if (!file) {
		file = fopen(PLAYER_DB_PATH, "w+b");
		if (!file) {
			perror("[DB] Failed to open database file");
			return false;
		}
	}

	PlayerRecord existing;
	while (fread(&existing, sizeof(existing), 1, file) == 1) {
		if (strncmp(existing.username, record->username,
					sizeof(existing.username)) == 0) {
			if (fseek(file, -(long)sizeof(existing), SEEK_CUR) != 0) {
				fclose(file);
				return false;
			}
			size_t written = fwrite(record, sizeof(*record), 1, file);
			fclose(file);
			return written == 1;
		}
	}

	size_t appended = fwrite(record, sizeof(*record), 1, file);
	fclose(file);
	return appended == 1;
}

/**
 * @brief Load a player record, creating a default if not found.
 *
 * Combines fetch_player_record and init_default_record logic.
 * If the player exists, returns their record. Otherwise, creates
 * a new default record and persists it.
 *
 * @param username Username to load or create.
 * @param record Output record (must be allocated).
 * @return 0 on success, -1 on error.
 *
 * @internal
 */
static int load_or_create_record(const char* username, PlayerRecord* record) {
	if (fetch_player_record(username, record)) {
		return 0;
	}

	init_default_record(record, username);
	if (!upsert_player_record(record)) {
		return -1;
	}
	return 0;
}

/**
 * @brief Apply match outcome rewards to a player record.
 *
 * Adds experience and souls based on the match result. Experience that
 * reaches or exceeds XP_PER_LEVEL triggers level advancement and progression
 * wrap. Soul accumulation is capped to prevent overflow.
 *
 * @param record Record to update.
 * @param has_won true for win rewards, false for loss rewards.
 *
 * @internal
 */
static void apply_match_rewards(PlayerRecord* record, bool has_won) {
	uint16_t xp_gain = has_won ? XP_REWARD_WIN : XP_REWARD_LOSE;
	uint16_t soul_gain = has_won ? SOUL_REWARD_WIN : SOUL_REWARD_LOSE;

	uint32_t new_xp = record->progression + xp_gain;
	while (new_xp >= XP_PER_LEVEL) {
		new_xp -= XP_PER_LEVEL;
		if (record->level < UINT8_MAX) {
			record->level++;
		}
	}
	record->progression = (uint16_t)new_xp;

	uint32_t new_money = record->money + soul_gain;
	if (new_money > UINT16_MAX) {
		new_money = UINT16_MAX;
	}
	record->money = (uint16_t)new_money;
}

/**
 * @brief Implementation of db_get_player_info.
 *
 * Loads or creates the player's profile and converts to EMAP payload format.
 */
int db_get_player_info(const char* username, EMAP_PayloadPlayerInfo* out_info) {
	if (!username || !out_info) {
		return -1;
	}

	PlayerRecord record;
	if (load_or_create_record(username, &record) != 0) {
		return -1;
	}

	normalize_record(&record);
	record_to_payload(&record, out_info);
	printf("[DB] Loaded info for %s (level=%u xp=%u souls=%u)\n", username,
		   out_info->level, out_info->progression, out_info->money);
	return 0;
}

/**
 * @brief Implementation of db_update_player_info.
 *
 * Converts EMAP payload to internal record and persists to disk.
 */
int db_update_player_info(const char* username,
						  const EMAP_PayloadPlayerInfo* info) {
	if (!username || !info) {
		return -1;
	}

	PlayerRecord record;
	payload_to_record(username, info, &record);
	if (!upsert_player_record(&record)) {
		return -1;
	}

	printf("[DB] Persisted profile %s (level=%u xp=%u souls=%u)\n", username,
		   info->level, info->progression, info->money);
	return 0;
}

/**
 * @brief Implementation of db_apply_match_result.
 *
 * Loads profile, applies rewards, normalizes, and persists changes.
 */
int db_apply_match_result(const char* username, int has_won,
						  EMAP_PayloadPlayerInfo* out_info) {
	if (!username) {
		return -1;
	}

	PlayerRecord record;
	if (load_or_create_record(username, &record) != 0) {
		return -1;
	}

	apply_match_rewards(&record, has_won != 0);
	normalize_record(&record);

	if (!upsert_player_record(&record)) {
		return -1;
	}

	if (out_info) {
		record_to_payload(&record, out_info);
	}

	printf("[DB] Match result applied for %s (won=%d, level=%u, souls=%u)\n",
		   username, has_won != 0, record.level, record.money);
	return 0;
}

/**
 * @brief Implementation of db_purchase_skin.
 *
 * Checks ownership and funds, deducts souls, marks skin as owned, equips it,
 * and persists changes.
 */
int db_purchase_skin(const char* username, uint8_t skin_index, uint16_t price,
					 EMAP_PayloadPlayerInfo* out_info) {
	if (!username || skin_index >= TOTAL_SKIN_AMOUNT) {
		return -3;
	}

	PlayerRecord record;
	if (load_or_create_record(username, &record) != 0) {
		return -1;
	}

	if (record.owned_skins[skin_index]) {
		if (out_info) {
			record_to_payload(&record, out_info);
		}
		return 0;
	}

	if (record.money < price) {
		return -2;
	}

	record.money -= price;
	record.owned_skins[skin_index] = 1;
	record.selected_skin = skin_index;
	normalize_record(&record);

	if (!upsert_player_record(&record)) {
		return -1;
	}

	if (out_info) {
		record_to_payload(&record, out_info);
	}

	printf("[DB] %s purchased skin %u (souls=%u)\n", username, skin_index,
		   record.money);
	return 0;
}

/**
 * @brief Implementation of db_equip_skin.
 *
 * Changes the selected skin if the player owns it and validates the skin index.
 */
int db_equip_skin(const char* username, uint8_t skin_index,
				  EMAP_PayloadPlayerInfo* out_info) {
	if (!username || skin_index >= TOTAL_SKIN_AMOUNT) {
		return -3;
	}

	PlayerRecord record;
	if (load_or_create_record(username, &record) != 0) {
		return -1;
	}

	if (!record.owned_skins[skin_index]) {
		return -3;
	}

	record.selected_skin = skin_index;
	normalize_record(&record);

	if (!upsert_player_record(&record)) {
		return -1;
	}

	if (out_info) {
		record_to_payload(&record, out_info);
	}

	printf("[DB] %s equipped skin %u\n", username, skin_index);
	return 0;
}
