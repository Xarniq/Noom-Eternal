/**
 * @file settings_ui.h
 * @brief Settings menu UI rendering and interaction.
 *
 * Provides functions to draw an overlay settings button and a settings panel
 * with audio and display controls. The panel is modal and can be dismissed by
 * the user.
 */

#ifndef SETTINGS_UI_H
#define SETTINGS_UI_H

#include "raylib.h"
#include "types.h"

/**
 * @brief Render the settings button and handle button input.
 *
 * Draws a small settings icon button in the top-right corner of the screen.
 * Toggles the settings panel visibility on click. Uses transition alpha for
 * fade effects during screen transitions.
 *
 * @param assets Pointer to the Assets structure (for button texture).
 * @param transitionAlpha Current transition fade alpha [0.0, 1.0].
 * @param showSettings Pointer to boolean flag controlling panel visibility;
 *                     toggled on button click.
 */
void DrawSettingsButton(Assets* assets, float transitionAlpha,
						bool* showSettings);

/**
 * @brief Render the settings panel and process user interactions.
 *
 * Draws a modal panel with audio volume sliders, music toggle, and FPS toggle.
 * Handles mouse input for slider adjustment and panel closure. Updates the
 * GameSettings structure with user-selected values in real-time.
 *
 * @param assets Pointer to the Assets structure (for font rendering).
 * @param settings Pointer to GameSettings to read and update with user input.
 * @param audioAvailable Boolean indicating if audio device is ready; affects
 *                       music volume slider availability.
 * @param showSettings Pointer to visibility flag; set to false when user clicks
 * close.
 */
void DrawSettingsPanel(Assets* assets, GameSettings* settings,
					   bool audioAvailable, bool* showSettings);

#endif // SETTINGS_UI_H
