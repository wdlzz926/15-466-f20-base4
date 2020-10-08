#include "ColorTextureProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Load< ColorTextureProgram > color_texture_program(LoadTagEarly, []() -> ColorTextureProgram const * {
	ColorTextureProgram *ret = new ColorTextureProgram();
	return ret;
});

ColorTextureProgram::ColorTextureProgram() {
	//Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
	program = gl_compile_program(
		//vertex shader:
		"#version 330\n"
		"uniform mat4 OBJECT_TO_CLIP;\n"
		"in vec4 vertex;\n"
		// "in vec4 Color;\n"
		// "in vec2 TexCoord;\n"
		// "out vec4 color;\n"
		"out vec2 texCoord;\n"
		"void main() {\n"
		"	gl_Position = OBJECT_TO_CLIP * vec4(vertex.xy,0.0,1.0);\n"
		"	texCoord = vertex.zw;\n"
		"}\n"
	,
		//fragment shader:
		"#version 330\n"
		"uniform sampler2D TEX;\n"
		"uniform vec3 textColor;\n"
		// "in vec4 color;\n"
		"in vec2 texCoord;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"	vec4 sampled = vec4(1.0,1.0,1.0, texture(TEX, texCoord).r);\n"
		"	fragColor = vec4(textColor, 1.0)*sampled;\n"
		"}\n"
	);
	//As you can see above, adjacent strings in C/C++ are concatenated.
	// this is very useful for writing long shader programs inline.

	//look up the locations of vertex attributes:
	Position_vec4 = glGetAttribLocation(program, "Position");
	// Color_vec4 = glGetAttribLocation(program, "Color");
	TexCoord_vec2 = glGetAttribLocation(program, "TexCoord");

	//look up the locations of uniforms:
	Color_vec4 = glGetUniformLocation(program, "textColor");
	OBJECT_TO_CLIP_mat4 = glGetUniformLocation(program, "OBJECT_TO_CLIP");
	GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");

	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now

	glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0

	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now
}

ColorTextureProgram::~ColorTextureProgram() {
	glDeleteProgram(program);
	program = 0;
}

