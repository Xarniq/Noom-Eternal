/**
 * @file db_manager.h
 * @brief Player profile database management and persistence.
 *
 * This module provides an abstraction layer for player data persistence,
 * handling profile creation, loading, updates, and match result processing.
 * Player profiles are stored in a binary database file and include progression
 * (level, experience, souls) and cosmetic inventory (owned skins and selected
 * skin).
 *
 * All operations normalize player records to ensure data consistency,
 * particularly regarding skin ownership and selection validity.
 *
 * @author NoomEternal Development Team
 * @version 1.0
 * @date 2026-01-08
 */

#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include "../lib/emap-lib/include/types.h"

/**
 * @brief Retrieve player information from the database.
 *
 * Loads an existing player profile or creates a new default profile if the
 * player doesn't exist. The profile includes level, progression (experience
 * points), souls (in-game currency), and cosmetic skin ownership/selection.
 *
 * @param[in] username The unique username to look up. Must be non-NULL and
 *                      will be stored with a maximum of
 * MAX_PLAYER_USERNAME_SIZE-1 characters.
 * @param[out] out_info Pointer to EMAP_PayloadPlayerInfo structure to populate
 *                      with player data. All numeric fields are in host byte
 * order. Must be non-NULL.
 *
 * @return 0 on success (player found or created).
 * @return -1 on failure (database access error).
 *
 * @note If the player doesn't exist, a new profile is created with:
 *       - Level: 1
 *       - Progression: 0 XP
 *       - Money: DEFAULT_SOULS (100)
 *       - Owned skins: default skin (index 0)
 *       - Selected skin: default skin
 *
 * @see db_update_player_info(), db_apply_match_result()
 */
int db_get_player_info(const char* username, EMAP_PayloadPlayerInfo* out_info);

/**
 * @brief Update or create player information in the database.
 *
 * Persists a complete player profile snapshot. If the player exists, their
 * record is updated in-place. If not, a new record is appended to the database
 * file.
 *
 * @param[in] username The unique username to update. Must be non-NULL.
 * @param[in] info Pointer to EMAP_PayloadPlayerInfo structure containing the
 * new profile data. All numeric fields are expected to be in host byte order.
 *                 Must be non-NULL.
 *
 * @return 0 on success.
 * @return -1 on failure (database write error).
 *
 * @warning This function overwrites the complete profile. Use
 * db_apply_match_result() for match-based updates to ensure reward calculations
 * are correct.
 *
 * @see db_get_player_info(), db_apply_match_result()
 */
int db_update_player_info(const char* username,
						  const EMAP_PayloadPlayerInfo* info);

/**
 * @brief Apply post-match rewards for a player profile.
 *
 * Processes match completion, applying experience and soul rewards based on the
 * match outcome. Experience rewards are applied with automatic level-up when
 * progression exceeds XP_PER_LEVEL. New players are created on demand.
 *
 * Reward table:
 * - Match Win:  +XP_REWARD_WIN XP, +SOUL_REWARD_WIN souls
 * - Match Loss: +XP_REWARD_LOSE XP, +SOUL_REWARD_LOSE souls
 *
 * @param[in] username Player identifier. If not found, a new profile is
 * created. Must be non-NULL.
 * @param[in] has_won Non-zero indicates a win; zero indicates a loss.
 * Determines which reward tier is applied.
 * @param[out] out_info Optional pointer to receive the refreshed profile
 * snapshot. If NULL, the updated profile is not returned. All numeric fields
 * will be in host byte order.
 *
 * @return 0 on success.
 * @return -1 on failure (database access or storage error).
 *
 * @note Experience that exceeds XP_PER_LEVEL triggers automatic level
 * advancement.
 * @note Soul accumulation is capped at UINT16_MAX to prevent overflow.
 *
 * @see db_get_player_info(), db_update_player_info()
 */
int db_apply_match_result(const char* username, int has_won,
						  EMAP_PayloadPlayerInfo* out_info);

/**
 * @brief Purchase a cosmetic skin for a player if they have sufficient funds.
 *
 * Attempts to purchase a skin if the player has enough souls and hasn't already
 * owned the skin. On successful purchase, the newly acquired skin is
 * automatically equipped (selected).
 *
 * @param[in] username Player identifier to charge and update. Must be non-NULL.
 * @param[in] skin_index Target skin slot (0 to TOTAL_SKIN_AMOUNT-1). Determines
 *                        which skin to purchase and its price via
 * resolve_skin_price().
 * @param[in] price Price in souls (host order). Deducted from player's soul
 * balance.
 * @param[out] out_info Optional pointer to receive the refreshed profile
 * snapshot. If NULL, the updated profile is not returned. All numeric fields
 * will be in host byte order.
 *
 * @return 0 on success (skin purchased or already owned).
 * @return -1 on failure (database storage error).
 * @return -2 on insufficient funds (player souls < price).
 * @return -3 on invalid skin index (skin_index >= TOTAL_SKIN_AMOUNT).
 *
 * @note If the skin is already owned, the function returns 0 without deducting
 * souls or changing the selected skin.
 * @note On successful purchase, the new skin is immediately equipped.
 *
 * @see db_equip_skin(), db_get_player_info()
 */
int db_purchase_skin(const char* username, uint8_t skin_index, uint16_t price,
					 EMAP_PayloadPlayerInfo* out_info);

/**
 * @brief Equip an already owned skin for a player profile.
 *
 * Changes the player's active (selected) skin to a different skin they already
 * own. No soul cost is applied; this is purely a cosmetic selection change.
 *
 * @param[in] username Player identifier to update. Must be non-NULL.
 * @param[in] skin_index Target skin slot (0 to TOTAL_SKIN_AMOUNT-1). The player
 *                        must already own this skin.
 * @param[out] out_info Optional pointer to receive the refreshed profile
 * snapshot. If NULL, the updated profile is not returned. All numeric fields
 * will be in host byte order.
 *
 * @return 0 on success (skin equipped or already the selected skin).
 * @return -1 on failure (database storage error).
 * @return -3 on invalid skin index or skin not owned (skin_index >=
 * TOTAL_SKIN_AMOUNT or player does not own the skin).
 *
 * @note If the player doesn't own the requested skin, this returns -3.
 * @note If the skin is already selected, the function succeeds without change.
 *
 * @see db_purchase_skin(), db_get_player_info()
 */
int db_equip_skin(const char* username, uint8_t skin_index,
				  EMAP_PayloadPlayerInfo* out_info);

#endif /* DB_MANAGER_H */
