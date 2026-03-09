/**
 * @file main.c
 * @brief Entry point and main event loop for the NoomEternal client
 * application.
 *
 * Initializes all subsystems (window, audio, configuration, assets, network),
 * manages the main game loop with screen transitions, and handles cleanup on
 * exit. The main loop processes network events asynchronously, updates game
 * state, renders the current screen, and manages audio playback.
 */

/**
 * @brief Main entry point for the NoomEternal client application.
 *
 * Performs complete application initialization:
 * - Initializes raylib window (1600x800, 60 FPS target)
 * - Sets up audio device (gracefully handles unavailability)
 * - Loads client configuration from INI file
 * - Initializes player data and game state
 * - Creates network client context
 * - Loads all game assets
 *
 * Main loop:
 * - Processes non-blocking network messages
 * - Handles screen transitions with fade effects
 * - Updates game logic and input handling123
 * - Renders current screen
 * - Updates background music
 *
 * Cleanup:
 * - Saves configuration to disk
 * - Unloads all assets
 * - Closes audio device
 * - Closes window
 *
 * @return Exit status code (0 on success, non-zero on error).
 */
#include "audio_manager.h"
#include "config.h"
#include "game_logic.h"
#include "network_client.h"
#include "raylib.h"
#include "settings_ui.h"
#include "types.h"
#include "ui.h"
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 800
#define CONFIG_FILE "config/client.ini"

int main(void) {
	// Ignore SIGPIPE to prevent crashes when writing to closed sockets
	signal(SIGPIPE, SIG_IGN);

	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "NoomEternal");
	SetTargetFPS(60);

	// Try to initialize audio
	InitAudioDevice();
	bool audioAvailable = IsAudioDeviceReady();

	if (!audioAvailable) {
		printf(
			"WARNING: Audio device not available. Music will be disabled.\n");
	}

	// Load configuration
	ClientConfig config;
	LoadConfig(&config, CONFIG_FILE);

	GameScreen currentScreen = SCREEN_INTRO;
	float transitionAlpha = 0.0f;

	// Initialize player from config/defaults (live data comes from server once
	// connected)
	PlayerData player = {0};
	player.level = 1;
	player.maxXP = 1000;
	player.souls = 0;
	player.selectedCoin = 0;
	player.serverPort = config.serverPort;
	strncpy(player.serverIP, config.serverIP, sizeof(player.serverIP) - 1);

	const char* envUser = getenv("USER");
	strncpy(player.username, envUser ? envUser : "SLAYER",
			sizeof(player.username) - 1);
	player.username[sizeof(player.username) - 1] = '\0';
	player.ownedSkins[0] = true;
	player.profileReady = false;

	// Initialize settings from config
	GameSettings settings = {.musicVolume = config.musicVolume,
							 .sfxVolume = config.sfxVolume,
							 .showFPS = config.showFPS,
							 .musicEnabled = config.musicEnabled,
							 .pageTransitionAlpha = 0.0f,
							 .isTransitioning = false,
							 .targetScreen = SCREEN_INTRO,
							 .musicFadeTimer = 0.0f,
							 .netClient = NULL,
							 .onlineState = ONLINE_STATE_IDLE,
							 .onlineMatchRequested = false,
							 .onlineMatchActive = false,
							 .onlineMatchReady = false,
							 .onlineMatchCancelRequested = false,
							 .onlineStatus = {0},
							 .onlineError = {0},
							 .onlineLastMatch = false};

	Assets assets = {0};
	LoadAssets(&assets);

	NetworkClient netClient;
	NetworkClient_Init(&netClient);
	settings.netClient = &netClient;

	bool showSettings = false;

	float loadingProgress = 0.0f;
	int coinPrices[6] = {0, 100, 150, 200, 250, 300};
	int currentShopCoin = 0;

	GameState game;
	InitGameBoard(&game);

	while (!WindowShouldClose()) {
		NetworkClient* activeClient = settings.netClient;

		// --- Network Logic ---
		if (settings.onlineMatchCancelRequested) {
			settings.onlineMatchCancelRequested = false;
			settings.onlineMatchRequested = false;
			settings.onlineMatchActive = false;
			settings.onlineMatchReady = false;
			settings.onlineState = ONLINE_STATE_IDLE;
			settings.onlineStatus[0] = '\0';
			settings.onlineLastMatch = false;
			if (activeClient) {
				NetworkClient_Disconnect(activeClient);
				NetworkClient_Init(activeClient);
			}
		}

		if (settings.onlineMatchRequested) {
			settings.onlineMatchRequested = false;
			settings.onlineError[0] = '\0';

			if (activeClient) {
				NetworkClient_Disconnect(activeClient);
				NetworkClient_Init(activeClient);
			}

			memset(game.board, 0, sizeof(game.board));
			game.selectedSlot = -1;
			game.gameOver = false;
			game.winner = 0;
			game.currentPlayer = -1;

			if (!activeClient ||
				!NetworkClient_Connect(activeClient, player.serverIP,
									   player.serverPort, player.username,
									   &player)) {
				settings.onlineState = ONLINE_STATE_ERROR;
				snprintf(settings.onlineError, sizeof(settings.onlineError),
						 "Connection failed: %s",
						 activeClient ? NetworkClient_GetError(activeClient)
									  : "client missing");
				settings.onlineLastMatch = false;
				settings.targetScreen = SCREEN_HOME;
				settings.isTransitioning = true;
			} else if (settings.onlineConnectOnly) {
				// Just connected to fetch info
				settings.onlineState = ONLINE_STATE_IDLE;
				settings.onlineConnectOnly = false;
				snprintf(settings.onlineStatus, sizeof(settings.onlineStatus),
						 "Connected!");
			} else if (!NetworkClient_JoinGame(activeClient)) {
				settings.onlineState = ONLINE_STATE_ERROR;
				snprintf(settings.onlineError, sizeof(settings.onlineError),
						 "Matchmaking refused: %s",
						 NetworkClient_GetError(activeClient));
				settings.onlineLastMatch = false;
				settings.targetScreen = SCREEN_HOME;
				settings.isTransitioning = true;
			} else {
				settings.onlineState = ONLINE_STATE_WAITING;
				snprintf(settings.onlineStatus, sizeof(settings.onlineStatus),
						 "Queueing on %s:%d...", player.serverIP,
						 player.serverPort);
			}
		}

		if (settings.onlineState == ONLINE_STATE_WAITING && activeClient) {
			bool stillWaiting = NetworkClient_WaitForGame(activeClient, &game);
			if (!stillWaiting) {
				NetworkState st = NetworkClient_GetState(activeClient);
				if (st == NET_IN_GAME) {
					settings.onlineState = ONLINE_STATE_IN_GAME;
					if (NetworkClient_IsOurTurn(activeClient)) {
						snprintf(settings.onlineStatus,
								 sizeof(settings.onlineStatus), "Your turn");
					} else {
						snprintf(settings.onlineStatus,
								 sizeof(settings.onlineStatus),
								 "Waiting for opponent to play...");
					}
					settings.onlineMatchReady = true;
					settings.onlineMatchActive = true;
				} else if (st == NET_ERROR) {
					settings.onlineState = ONLINE_STATE_ERROR;
					snprintf(settings.onlineError, sizeof(settings.onlineError),
							 "Lobby error: %s",
							 NetworkClient_GetError(activeClient));
					settings.onlineLastMatch = false;
					settings.targetScreen = SCREEN_HOME;
					settings.isTransitioning = true;
				}
			}
		}

		if (activeClient) {
			NetworkState currentState = NetworkClient_GetState(activeClient);
			bool shouldPoll =
				(currentState == NET_CONNECTED || currentState == NET_IN_GAME ||
				 currentState == NET_GAME_ENDED);
			if (shouldPoll) {
				if (!NetworkClient_ProcessMessages(activeClient, &game,
												   &player)) {
					settings.onlineState = ONLINE_STATE_ERROR;
					snprintf(settings.onlineError, sizeof(settings.onlineError),
							 "Network error: %s",
							 NetworkClient_GetError(activeClient));
					settings.onlineMatchActive = false;
					settings.onlineLastMatch = false;
					settings.targetScreen = SCREEN_HOME;
					settings.isTransitioning = true;
				}
			}
		}

		if (settings.onlineMatchActive &&
			settings.onlineState == ONLINE_STATE_IN_GAME && activeClient &&
			settings.onlineError[0] == '\0') {
			const char* status = NetworkClient_IsOurTurn(activeClient)
									 ? "Your turn"
									 : "Waiting for opponent to play...";
			snprintf(settings.onlineStatus, sizeof(settings.onlineStatus), "%s",
					 status);
		}

		if (settings.onlineMatchActive && game.gameOver) {
			settings.onlineMatchActive = false;
			settings.onlineState = ONLINE_STATE_IDLE;
			snprintf(settings.onlineStatus, sizeof(settings.onlineStatus),
					 "Game finished");
		}

		// --- Page Transitions ---
		if (settings.isTransitioning) {
			settings.pageTransitionAlpha +=
				GetFrameTime() * 5.0f; // Fast fade out
			if (settings.pageTransitionAlpha >= 1.0f) {
				settings.pageTransitionAlpha = 1.0f;
				currentScreen = settings.targetScreen;
				settings.isTransitioning = false;
				settings.musicFadeTimer = 0.0f; // Reset music fade timer
			}
		} else if (settings.pageTransitionAlpha > 0.0f) {
			settings.pageTransitionAlpha -=
				GetFrameTime() * 5.0f; // Fast fade in
			if (settings.pageTransitionAlpha < 0.0f) {
				settings.pageTransitionAlpha = 0.0f;
			}
		}

		// --- Audio Management ---
		UpdateBackgroundMusic(&assets, &settings, currentScreen,
							  audioAvailable);

		BeginDrawing();

		// Draw current screen
		if (currentScreen == SCREEN_INTRO) {
			// Intro screen - video playback
			ClearBackground(BLACK);

			if (assets.introLoaded) {
				// Update video with deltaTime for better sync
				UpdateMediaEx(&assets.introVideo, GetFrameTime());

				// Get video properties
				MediaProperties props = GetMediaProperties(assets.introVideo);
				double currentPos = GetMediaPosition(assets.introVideo);

				// Draw video fullscreen
				Texture2D videoTexture = assets.introVideo.videoTexture;

				if (videoTexture.id > 0) {
					// Calculate scaling to fill screen (cover mode)
					float scaleX = (float)SCREEN_WIDTH / videoTexture.width;
					float scaleY = (float)SCREEN_HEIGHT / videoTexture.height;
					float scale = (scaleX > scaleY) ? scaleX : scaleY;

					int drawWidth = (int)(videoTexture.width * scale);
					int drawHeight = (int)(videoTexture.height * scale);
					int drawX = (SCREEN_WIDTH - drawWidth) / 2;
					int drawY = (SCREEN_HEIGHT - drawHeight) / 2;

					// Calculate alpha for fade in/out (2 seconds)
					float videoAlpha = 1.0f;
					float fadeDuration = 2.0f;

					// Fade in at start
					if (currentPos < fadeDuration) {
						videoAlpha = (float)(currentPos / fadeDuration);
					}
					// Fade out at end
					else if (currentPos > props.durationSec - fadeDuration) {
						videoAlpha = (float)((props.durationSec - currentPos) /
											 fadeDuration);
					}

					if (videoAlpha < 0.0f) {
						videoAlpha = 0.0f;
					}
					if (videoAlpha > 1.0f) {
						videoAlpha = 1.0f;
					}

					DrawTexturePro(videoTexture,
								   (Rectangle){0, 0, (float)videoTexture.width,
											   (float)videoTexture.height},
								   (Rectangle){(float)drawX, (float)drawY,
											   (float)drawWidth,
											   (float)drawHeight},
								   (Vector2){0, 0}, 0.0f,
								   (Color){255, 255, 255,
										   (unsigned char)(255 * videoAlpha)});
				}

				// Go to home screen if video is finished
				if (currentPos >= props.durationSec - 0.1f) {
					currentScreen = SCREEN_HOME;
					transitionAlpha = 0.0f;
				}
			} else {
				// Fallback if video not loaded
				DrawDoomText(assets.doomFont, assets.fontLoaded, "NOOMETERNAL",
							 1600 / 2 - 300, 800 / 2 - 50, 96, ORANGE);
			}

			// Allow skipping intro
			if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) ||
				IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
				currentScreen = SCREEN_HOME;
				transitionAlpha = 0.0f;
			}
		} else {
			switch (currentScreen) {
			case SCREEN_INTRO:
				// Already handled by if above, but needed to avoid -Wswitch
				// warning
				break;

			case SCREEN_HOME:
				// Handle fade in (4 seconds for smoother transition)
				if (transitionAlpha < 1.0f) {
					transitionAlpha +=
						GetFrameTime() * 0.25f; // Fade in over 4 seconds
					if (transitionAlpha > 1.0f) {
						transitionAlpha = 1.0f;
					}
				}

				// Draw background for home screen
				if (assets.backgroundLoaded) {
					float scaleX =
						(float)SCREEN_WIDTH / assets.background.width;
					float scaleY =
						(float)SCREEN_HEIGHT / assets.background.height;
					float scale = (scaleX > scaleY) ? scaleX : scaleY;

					int drawWidth = (int)(assets.background.width * scale);
					int drawHeight = (int)(assets.background.height * scale);
					int drawX = (SCREEN_WIDTH - drawWidth) / 2;
					int drawY = (SCREEN_HEIGHT - drawHeight) / 2;

					DrawTexturePro(
						assets.background,
						(Rectangle){0, 0, (float)assets.background.width,
									(float)assets.background.height},
						(Rectangle){(float)drawX, (float)drawY,
									(float)drawWidth, (float)drawHeight},
						(Vector2){0, 0}, 0.0f,
						(Color){255, 255, 255,
								(unsigned char)(255 * transitionAlpha)});

					DrawRectangle(
						0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
						(Color){0, 0, 0,
								(unsigned char)(150 * transitionAlpha)});
				} else {
					ClearBackground((Color){20, 20, 30, 255});
				}

				DrawHomeScreen(&assets, &player, &currentScreen,
							   &loadingProgress, transitionAlpha, &settings);

				// Black overlay fading out for smoother fade in
				DrawRectangle(
					0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
					(Color){0, 0, 0,
							(unsigned char)(255 * (1.0f - transitionAlpha))});

				break;

			case SCREEN_SHOP:
				// Draw background for shop screen
				if (assets.backgroundLoaded) {
					float scaleX =
						(float)SCREEN_WIDTH / assets.background.width;
					float scaleY =
						(float)SCREEN_HEIGHT / assets.background.height;
					float scale = (scaleX > scaleY) ? scaleX : scaleY;

					int drawWidth = (int)(assets.background.width * scale);
					int drawHeight = (int)(assets.background.height * scale);
					int drawX = (SCREEN_WIDTH - drawWidth) / 2;
					int drawY = (SCREEN_HEIGHT - drawHeight) / 2;

					DrawTexturePro(
						assets.background,
						(Rectangle){0, 0, (float)assets.background.width,
									(float)assets.background.height},
						(Rectangle){(float)drawX, (float)drawY,
									(float)drawWidth, (float)drawHeight},
						(Vector2){0, 0}, 0.0f, WHITE);

					DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
								  (Color){0, 0, 0, 150});
				} else {
					ClearBackground((Color){20, 20, 30, 255});
				}

				DrawShopScreen(&assets, &player, &currentScreen, coinPrices,
							   &currentShopCoin, &settings);
				break;

			case SCREEN_LOADING:
				ClearBackground((Color){20, 20, 30, 255});
				DrawLoadingScreen(&assets, &player, &currentScreen,
								  &loadingProgress, &settings);
				break;

			case SCREEN_GAME: {
				DrawGameScreen(&assets, &player, &currentScreen, &game,
							   &settings);

				bool onlineSession = settings.onlineLastMatch;

				if (game.gameOver) {
					if (onlineSession) {
						currentScreen = (game.winner == 1) ? SCREEN_VICTORY
														   : SCREEN_GAMEOVER;
						settings.onlineLastMatch = false;
					} else if (game.winner == 1) {
						// Score increment removed as per request (server
						// handled)
						currentScreen = SCREEN_VICTORY;
					} else if (game.winner == -1) {
						currentScreen = SCREEN_GAMEOVER;
					}
				}
			} break;

			case SCREEN_VICTORY:
				DrawVictoryScreen(&assets, &player, &currentScreen,
								  &loadingProgress, &settings);
				break;

			case SCREEN_GAMEOVER:
				DrawGameOverScreen(&assets, &player, &currentScreen,
								   &loadingProgress, &settings);
				break;
			}
		}

		// --- Settings UI ---
		DrawSettingsButton(&assets, transitionAlpha, &showSettings);

		if (showSettings) {
			DrawSettingsPanel(&assets, &settings, audioAvailable,
							  &showSettings);
		}

		// Show FPS if enabled
		if (settings.showFPS) {
			DrawDoomText(assets.doomFont, assets.fontLoaded,
						 TextFormat("FPS: %d", GetFPS()), 10, 10, 24, GREEN);
		}

		// Page transition overlay
		if (settings.pageTransitionAlpha > 0.0f) {
			DrawRectangle(
				0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
				(Color){0, 0, 0,
						(unsigned char)(255 * settings.pageTransitionAlpha)});
		}

		EndDrawing();
	}

	if (settings.netClient) {
		NetworkClient_Disconnect(settings.netClient);
	}

	// Save configuration before exit
	config.musicVolume = settings.musicVolume;
	config.sfxVolume = settings.sfxVolume;
	config.showFPS = settings.showFPS;
	config.musicEnabled = settings.musicEnabled;

	strncpy(config.serverIP, player.serverIP, sizeof(config.serverIP) - 1);
	config.serverPort = player.serverPort;

	SaveConfig(&config, CONFIG_FILE);

	UnloadAssets(&assets);
	if (audioAvailable) {
		CloseAudioDevice();
	}
	CloseWindow();

	return 0;
}
