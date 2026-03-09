/**
 * @file types.h
 * @brief Core data type definitions for the NoomEternal client.
 *
 * Defines the primary game structures, enumerations, and asset management types
 * used throughout the client. These types maintain game state, player
 * progression, network connection state, and all rendering resources.
 */

#ifndef TYPES_H
#define TYPES_H

#include "../lib/raylib-media/src/raymedia.h"
#include "raylib.h"
#include <stdbool.h>

struct NetworkClient; /**< Forward declaration of the network client context. */

/**
 * @brief Runtime state for online matchmaking and network connection lifecycle.
 *
 * Tracks the progression of the client through network initialization, lobby
 * waiting, active gameplay, and error conditions. Used to coordinate UI updates
 * and game flow.
 */
typedef enum {
	ONLINE_STATE_IDLE = 0, /**< No online activity in progress; offline mode or
							  initial state. */
	ONLINE_STATE_CONNECTING, /**< Actively establishing the EMAP connection to
								the server. */
	ONLINE_STATE_WAITING, /**< Connected and queued in matchmaking, waiting for
							 an opponent. */
	ONLINE_STATE_IN_GAME, /**< A match has started and gameplay is in progress.
						   */
	ONLINE_STATE_ERROR /**< Fatal network error; connection cannot be recovered.
						*/
} OnlineState;

/**
 * @brief Active game screen enumeration for UI and state management.
 *
 * Defines the set of possible screens the application can display. Used to
 * control which rendering and input functions are invoked during the main loop.
 */
typedef enum {
	SCREEN_INTRO,	/**< Initial video introduction screen. */
	SCREEN_HOME,	/**< Main menu with game mode selection. */
	SCREEN_SHOP,	/**< Coin/skin marketplace interface. */
	SCREEN_LOADING, /**< Loading screen with progress indicator and matchmaking
					   status. */
	SCREEN_GAME,	/**< Main gameplay with board rendering and move input. */
	SCREEN_VICTORY, /**< Victory screen with rewards display. */
	SCREEN_GAMEOVER /**< Defeat screen with end-game statistics. */
} GameScreen;

/**
 * @brief Encapsulates the player's progression and profile information.
 *
 * Maintains persistent and transient player data such as level, experience
 * points, currency, and owned cosmetics. Updated by network sync operations
 * with the server.
 */
typedef struct {
	int level;		   /**< Current player level. */
	int xp;			   /**< Current experience points (progression). */
	int maxXP;		   /**< XP threshold required for the next level. */
	int souls;		   /**< Game currency (earned through victories). */
	int selectedCoin;  /**< Index of the active coin/skin (0-5). */
	char serverIP[64]; /**< Server address for online connections. */
	char username[32]; /**< Player's username displayed in lobby and matches. */
	int serverPort;	   /**< Server port for establishing EMAP connection. */
	bool ownedSkins[6]; /**< Bitfield tracking which skins have been purchased.
						 */
	bool profileReady; /**< Flag indicating server has sent complete profile. */
} PlayerData;

/**
 * @brief Maintains the state of an active or in-progress game session.
 *
 * Contains the board layout, current player, move information, and game-over
 * conditions. Updated in real-time by game logic and network synchronization.
 */
typedef struct {
	int board[12];	   /**< Board positions: 0=empty, 1-5=player coins, -1 to
						  -5=opponent coins. */
	int currentPlayer; /**< Active player: 1=client, -1=opponent, 0=waiting for
						  server. */
	bool gameOver;	   /**< Flag indicating the game has ended (win/loss). */
	int winner; /**< Outcome: 1=player victory, -1=player defeat, 0=in progress.
				 */
	int selectedSlot; /**< Slot selected by player input (-1 if none). */
} GameState;

/**
 * @brief Aggregates game-wide settings and transient configuration state.
 *
 * Stores audio preferences, transition effects, network state, and UI
 * messaging. Persisted to disk for audio settings, while network and UI state
 * are reset on exit.
 */
typedef struct {
	float musicVolume; /**< Master volume for background music [0.0, 1.0]. */
	float sfxVolume;   /**< Master volume for sound effects [0.0, 1.0]. */
	bool showFPS;	   /**< Enable on-screen frame rate counter. */
	bool musicEnabled; /**< Toggle background music playback on/off. */
	float pageTransitionAlpha; /**< Current opacity for screen fade effects
								  [0.0, 1.0]. */
	bool isTransitioning;	   /**< Flag indicating a screen transition is in
								  progress. */
	GameScreen
		targetScreen;	  /**< Destination screen for the current transition. */
	float musicFadeTimer; /**< Elapsed time for progressive music fade-in. */
	struct NetworkClient*
		netClient; /**< Active EMAP client (NULL if offline mode). */
	OnlineState
		onlineState; /**< High-level matchmaking and connection state. */
	bool onlineMatchRequested; /**< User has requested to join an online match.
								*/
	bool onlineMatchActive;	   /**< A network match is currently running. */
	bool onlineMatchReady; /**< Server has confirmed match start; game screen
							  can begin. */
	bool onlineMatchCancelRequested; /**< User abort signal for
										connection/waiting phase. */
	bool onlineConnectOnly; /**< If true, only establish connection without
							   joining a game. */
	char onlineStatus[128]; /**< Human-readable status text (displayed on
							   loading/game screens). */
	char onlineError[256];	/**< Description of the last fatal network error (if
							   any). */
	bool onlineLastMatch;	/**< Indicates whether the most recent match was
							   online. */
} GameSettings;

/**
 * @brief Container for all game assets: textures, fonts, and audio resources.
 *
 * Manages the lifecycle of loaded resources, including success/failure flags to
 * enable graceful fallback rendering when assets are unavailable. Loaded during
 * initialization and unloaded at shutdown.
 */
typedef struct {
	// === Textures ===
	Texture2D background; /**< Main backdrop texture for all screens. */
	Texture2D
		gameBackground; /**< Specialized background for gameplay rendering. */
	Texture2D btnShop;	/**< Button texture for shop navigation. */
	Texture2D btnPlay;	/**< Button texture for starting matches. */
	Texture2D btnBack;	/**< Button texture for returning to previous screen. */
	Texture2D btnBuy;	/**< Button texture for purchase confirmation. */
	Texture2D btnSkin;	/**< Button texture for skin selection. */
	Texture2D btnCancel;   /**< Button texture for canceling operations. */
	Texture2D btnContinue; /**< Button texture for advancing to next screen. */
	Texture2D btnRetry;	   /**< Button texture for replaying a match. */
	Texture2D btnMenu;	   /**< Button texture for menu access from game. */
	Texture2D btnRestart;  /**< Button texture for session restart. */
	Texture2D btnSettings; /**< Button texture for opening settings panel. */
	Texture2D
		slotTexture; /**< Texture for inventory/selection slot backgrounds. */
	Texture2D coinTextures[6]; /**< Array of textures for coin/skin
								  representations. */

	// === Media ===
	MediaStream introVideo; /**< Video stream for the intro animation. */

	// === Fonts ===
	Font doomFont;	  /**< Primary Doom Eternal-style font. */
	Font eternalFont; /**< Secondary font for thematic UI elements. */

	// === Audio ===
	Music lobbyMusic; /**< Background music for menus and lobbies. */
	Music songMusic;  /**< Background music during gameplay. */

	// === Load Flags ===
	bool backgroundLoaded;	/**< true if background texture successfully loaded.
							 */
	bool fontLoaded;		/**< true if primary font successfully loaded. */
	bool eternalFontLoaded; /**< true if secondary font successfully loaded. */
	bool musicLoaded;		/**< true if music streams successfully loaded. */
	bool introLoaded;		/**< true if intro video successfully loaded. */
} Assets;

#endif // TYPES_H
