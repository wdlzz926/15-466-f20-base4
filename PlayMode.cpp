#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "ColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <fstream>

FT_Face face;
unsigned int glyph_count;
hb_font_t *hb_font{nullptr};
hb_buffer_t *hb_buf{nullptr};
hb_glyph_info_t *glyph_info{nullptr};
hb_glyph_position_t *glyph_pos{nullptr};
void RenderText(std::string text, float x, float y, float scale, glm::vec3 color);
unsigned int VAO, VBO;
std::vector<std::vector<Character>> story_c;
std::vector<std::vector<Character>> choice_c;

void createText(std::string text, std::vector<std::vector<Character>> &texts){
    if (hb_buf){
        hb_buffer_destroy(hb_buf);
		hb_buf = nullptr;
    } 
    FT_Set_Char_Size(face, 0, 48*64, 0,0);

    if (!face){
        std::cout<<"error creating face"<<std::endl;
    }
	hb_font = hb_ft_font_create(face, nullptr);
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

void renderText(std::vector<std::vector<Character>> texts, float x, float y, float scale, glm::vec3 color){
    // activate corresponding render state	
    glUniform3f(glGetUniformLocation(color_texture_program->program, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);
	float init_x = x;
	float init_y = y;
    // iterate through all characters
    for (auto char_vec:texts){
        for (unsigned int i = 0; i < char_vec.size();i++){
            Character ch = char_vec[i];
            float xpos = init_x + ch.Bearing.x * scale;
            float ypos = init_y - (ch.Size.y - ch.Bearing.y) * scale;

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
            init_x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
        }
		init_x = x;
		init_y -= 100.0f;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void load_game_text(std::vector<std::vector<std::string>> &container, std::string path){
	std::ifstream file(data_path(path));
	std::string s;
	std::vector<std::string> text;
	while(std::getline(file, s)){
		if(s[0] == '-'){
			std::vector<std::string> tmp(text);
			container.push_back(tmp);
			text.clear();
			continue;
		}
		text.push_back(s);
	}
	file.close();

}

PlayMode::PlayMode(){
	std::cout<<"Play mode initialize"<<std::endl;

	std::string font_name = data_path("fonts/cour.ttf");
    if (font_name.empty())
    {
        std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
    }

    if (FT_New_Face(DrawText::ft, font_name.c_str(), 0, &face)) {
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

	load_game_text(story, "dialog/story.txt");
	load_game_text(choice, "dialog/choice.txt");
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_1) {
			left.pressed = true;
			choice_made = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_2) {
			right.pressed = true;
			choice_made = true;
			return true;
		}
		
	}

	return false;
}

void PlayMode::update(float elapsed) {
	if (choice_made && !end){
		std::string c;
		if (left.pressed){
			c = choice[step][2];
		}else if(right.pressed){
			c = choice[step][3];
		}
		step++;
		change = true;
		story_c.clear();
		choice_c.clear();
	}
	if(step == story.size()){
		//end of game
		load_game_text(story, "dialog/end.txt");
		change = false;
		end = true;
	}
	if(change){
		for(std::string s:story[step]){
			createText(s, story_c);
		}
		for(unsigned int i = 0; i<2;i++){
			createText(choice[step][i], choice_c);
		}
	}
	change = false;
	choice_made = false;
	//reset button press counters:
	left.pressed = false;
	right.pressed = false;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(color_texture_program->program);
	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(drawable_size.x), 0.0f, static_cast<float>(drawable_size.y));
	glUniformMatrix4fv(glGetUniformLocation(color_texture_program->program, "OBJECT_TO_CLIP"), 1, GL_FALSE, glm::value_ptr(projection));

	// glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.
	renderText(story_c,100.0f, 900.0f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f));
	renderText(choice_c,100.0f, 200.0f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f));
	// scene.draw(*camera);
	glUseProgram(0);
	GL_ERRORS();
}
