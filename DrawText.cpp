#include "DrawText.hpp"
#include "Load.hpp"
#include "data_path.hpp"
#include "ColorTextureProgram.hpp"

FT_Library DrawText::ft;
Load<void> load_ft_library(LoadTagEarly, []() {
	if (FT_Init_FreeType(&DrawText::ft))
	{
		throw std::runtime_error("FT_Init_FreeType error!");
	}
});

DrawText::DrawText(){
    std::string font_name = data_path("fonts/cour.ttf");
    if (font_name.empty())
    {
        std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
    }

    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    }

    // disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

DrawText::~DrawText(){
    FT_Done_Face(face);
    if (hb_buf){
        hb_buffer_destroy(hb_buf);
		hb_buf = nullptr;
    }
}

void DrawText::createText(std::string text){
    std::cout<<"start create"<<std::endl;
    if (hb_buf){
        hb_buffer_destroy(hb_buf);
		hb_buf = nullptr;
    }
    std::cout<<"passed in text "<<text<<std::endl; 
    FT_Set_Char_Size(face, 0, 48*64, 0,0);
    std::cout<<"passed in text "<<text<<std::endl; 
    if (!face){
        std::cout<<"error creating face"<<std::endl;
    }
	hb_font = hb_ft_font_create(face, nullptr);
    std::cout<<"passed in text "<<text<<std::endl; 
    const char* t = text.c_str();
	hb_buf = hb_buffer_create();
	hb_buffer_add_utf8(hb_buf, t, -1, 0, -1);
	hb_buffer_set_direction(hb_buf, HB_DIRECTION_LTR);
	hb_buffer_set_script(hb_buf, HB_SCRIPT_LATIN);
	hb_buffer_set_language(hb_buf, hb_language_from_string("en", -1));
	hb_shape(hb_font, hb_buf, nullptr, 0);
	glyph_info = hb_buffer_get_glyph_infos(hb_buf, &glyph_count);
	glyph_pos = hb_buffer_get_glyph_positions(hb_buf, &glyph_count);
    std::vector<Character> char_vec;
    std::cout<<"hb initialize, text: "<<t<<std::endl;
    for (unsigned int i = 0; i < glyph_count; ++i){
        if (FT_Load_Char(face, t[i], FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = {
                texture,
                // glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left+glyph_pos[i].x_offset/64.0f, face->glyph->bitmap_top+glyph_pos[i].y_offset/64.0f),
                // static_cast<unsigned int>(face->glyph->advance.x)
                static_cast<unsigned int>(glyph_pos[i].x_advance)
            };
            char_vec.push_back(character);
            glBindTexture(GL_TEXTURE_2D, 0);
    }
    texts.push_back(char_vec);
}

void DrawText::clearText(){

}

void DrawText::renderText(float x, float y, float scale, glm::vec3 color){
    // activate corresponding render state	
    glUniform3f(glGetUniformLocation(color_texture_program->program, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // iterate through all characters
    for (auto char_vec:texts){
        for (unsigned int i = 0; i < char_vec.size();i++){
            Character ch = char_vec[i];
            float xpos = x + ch.Bearing.x * scale;
            float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

            float w = ch.Size.x * scale;
            float h = ch.Size.y * scale;
            // update VBO for each character
            float vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 0.0f },            
                { xpos,     ypos,       0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 1.0f },

                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 0.0f }           
            };
            // render glyph texture over quad
            glBindTexture(GL_TEXTURE_2D, ch.TextureID);
            // update content of VBO memory
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            // render quad
            glDrawArrays(GL_TRIANGLES, 0, 6);
            // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
            x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
        }
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}