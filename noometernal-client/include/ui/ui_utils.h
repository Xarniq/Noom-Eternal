/**
 * @file ui_utils.h
 * @brief Utility functions for UI rendering and interaction handling.
 *
 * Provides helper functions for text rendering with custom fonts and hitbox
 * calculations for interactive UI elements.
 */

#ifndef UI_UTILS_H
#define UI_UTILS_H

#include "raylib.h"
#include "types.h"

/**
 * @brief Calculate a centered, reduced clickable hitbox for a button.
 *
 * Shrinks the button hitbox by a specified ratio, keeping it centered.
 * Useful for more forgiving click detection when buttons overlap.
 *
 * @param btn The base button rectangle.
 * @param ratio Shrinkage ratio (0.5 = 50% size, centered).
 * @return A new Rectangle representing the reduced hitbox.
 */
Rectangle GetCenteredHitbox(Rectangle btn, float ratio);

/**
 * @brief Return the full button rectangle as the hitbox.
 *
 * No reduction applied; the entire button area is clickable.
 *
 * @param btn The button rectangle.
 * @return The same rectangle as the hitbox.
 */
Rectangle GetFullHitbox(Rectangle btn);

/**
 * @brief Render text using a custom font with Doom-style appearance.
 *
 * Draws text at the specified position using the provided font. If the font
 * failed to load, falls back to raylib's default font for readability.
 *
 * @param font The Font object to use for rendering.
 * @param fontLoaded Boolean indicating if the font was successfully loaded.
 * @param text Null-terminated string to render.
 * @param x X-coordinate of the text origin.
 * @param y Y-coordinate of the text origin.
 * @param fontSize Size in pixels to render the text.
 * @param color Color to tint the rendered text.
 */
void DrawDoomText(Font font, bool fontLoaded, const char* text, int x, int y,
				  int fontSize, Color color);

#endif // UI_UTILS_H
