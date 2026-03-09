/**
 * @file ui.h
 * @brief UI resource management and screen rendering interface.
 *
 * Coordinates loading/unloading of all game assets (textures, fonts, audio) and
 * provides the public interface for all screen rendering functions. Acts as the
 * central hub for UI-related operations.
 */

#ifndef UI_H
#define UI_H

#include "types.h"

/**
 * @brief Load all game assets from disk.
 *
 * Attempts to load textures, fonts, audio streams, and video files. Sets
 * appropriate success flags in the Assets structure for each resource.
 * Gracefully handles missing files by skipping them while logging warnings,
 * allowing the application to continue with fallback rendering.
 *
 * @param assets Pointer to the Assets structure to populate with loaded
 * resources.
 */
void LoadAssets(Assets* assets);

/**
 * @brief Unload and release all previously loaded assets.
 *
 * Frees all textures, fonts, music streams, and video media. Must be called
 * exactly once during shutdown before closing the rendering context. Safe to
 * call multiple times if needed, but only on assets that were successfully
 * loaded.
 *
 * @param assets Pointer to the Assets structure containing resources to
 * release.
 */
void UnloadAssets(Assets* assets);

/* Screen rendering headers */
#include "ui/game_screen.h"
#include "ui/gameover_screen.h"
#include "ui/home_screen.h"
#include "ui/loading_screen.h"
#include "ui/shop_screen.h"
#include "ui/ui_utils.h"
#include "ui/victory_screen.h"

#endif // UI_H
