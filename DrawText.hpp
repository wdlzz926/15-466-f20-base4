#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <iostream>

#include <hb.h>
#include <hb-ft.h>

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

struct DrawText
{
    DrawText();
    ~DrawText();
    static FT_Library ft;
    FT_Face face;
    unsigned int glyph_count;
	hb_font_t *hb_font{nullptr};
	hb_buffer_t *hb_buf{nullptr};
	hb_glyph_info_t *glyph_info{nullptr};
	hb_glyph_position_t *glyph_pos{nullptr};
    std::vector<std::vector<Character>> texts;
    unsigned int VAO, VBO;

    void createText(std::string text);
    void renderText(float x, float y, float scale, glm::vec3 color);
    void clearText();
};
