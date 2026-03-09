/**
 * @file ui_utils.c
 * @brief Implementation of UI utility functions.
 *
 * Provides helper functions for hitbox calculations and custom font text
 * rendering.
 */

/**
 * @brief Creates a hitbox centered inside the provided button rectangle, scaled
 * by a given ratio.
 *
 * @param btn The original button rectangle to derive the new hitbox from.
 * @param ratio The scale factor for the hitbox (0.0 to 1.0, where 0.5 produces
 * a half-size hitbox).
 * @return Rectangle A new Rectangle centered within the original and scaled by
 * the ratio.
 */

/**
 * @brief Returns the provided button rectangle as-is to allow full-area
 * interaction.
 *
 * @param btn The rectangle representing the button area.
 * @return Rectangle The same rectangle supplied as input.
 */

/**
 * @brief Draws text using the supplied font if loaded; otherwise falls back to
 * default drawing.
 *
 * @param font The font to draw the text with.
 * @param fontLoaded Indicates whether the custom font has been loaded
 * successfully.
 * @param text The null-terminated string to be drawn.
 * @param x The x-coordinate of the text's starting position.
 * @param y The y-coordinate of the text's baseline.
 * @param fontSize The desired font size for drawing.
 * @param color The color to render the text in.
 */
#include "ui/ui_utils.h"
#include <raylib.h>

/**
 * @brief Create a scaled hitbox centered within a button rectangle.
 *
 * Reduces the button size by the specified ratio and centers the resulting
 * hitbox within the original button bounds. Useful for creating smaller
 * clickable areas than the visual button size.
 *
 * @param btn The original button rectangle.
 * @param ratio Scaling factor (0.0 to 1.0), where 0.5 creates a hitbox half
 *             the original size.
 * @return Rectangle containing the centered, scaled hitbox.
 */
Rectangle GetCenteredHitbox(Rectangle btn, float ratio) {
	float newWidth = btn.width * ratio;
	float newHeight = btn.height * ratio;
	float offsetX = (btn.width - newWidth) / 2.0f;
	float offsetY = (btn.height - newHeight) / 2.0f;

	return (Rectangle){btn.x + offsetX, btn.y + offsetY, newWidth, newHeight};
}

/**
 * @brief Return the full button rectangle as the hitbox.
 *
 * Passthrough function that returns the button bounds unchanged, useful for
 * consistency with GetCenteredHitbox when no scaling is desired.
 *
 * @param btn The button rectangle to use as hitbox.
 * @return The button rectangle unchanged.
 */
Rectangle GetFullHitbox(Rectangle btn) { return btn; }

/**
 * @brief Draw styled text with font fallback support.
 *
 * Draws text using the provided custom font if loaded; otherwise falls back
 * to the default Raylib font. Automatically applies letter spacing based on
 * font size.
 *
 * @param font The Raylib Font to use (ignored if fontLoaded is false).
 * @param fontLoaded Boolean indicating whether the font is ready to use.
 * @param text The text string to draw.
 * @param x Screen X coordinate for text position.
 * @param y Screen Y coordinate for text position.
 * @param fontSize The font size in pixels.
 * @param color The color to draw the text in.
 */
void DrawDoomText(Font font, bool fontLoaded, const char* text, int x, int y,
				  int fontSize, Color color) {
	if (fontLoaded) {
		DrawTextEx(font, text, (Vector2){x, y}, fontSize, fontSize * 0.05f,
				   color);
	} else {
		DrawText(text, x, y, fontSize, color);
	}
}
