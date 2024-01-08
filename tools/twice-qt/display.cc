#include "display.h"

#include <iostream>

#include "libtwice/exception.h"

namespace twice {

static const char *vtx_shader_src = R"___(
#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv_in;

out vec2 uv;

void main()
{
	gl_Position = vec4(pos, 1.0);
	uv = uv_in;
}
)___";

static const char *fragment_shader_src = R"___(
#version 330 core
out vec4 FragColor;

in vec2 uv;

uniform sampler2D texture1;

void main()
{
	FragColor = texture(texture1, uv);
}
)___";

Display::Display(triple_buffer<std::array<u32, NDS_FB_SZ>> *tbuffer)
	: tbuffer(tbuffer)
{
}

Display::~Display()
{
	makeCurrent();
	glDeleteShader(vtx_shader);
	glDeleteShader(fragment_shader);
}

void
Display::render()
{
	update();
}

void
Display::initializeGL()
{
	initializeOpenGLFunctions();

	vtx_shader = compile_shader(vtx_shader_src, GL_VERTEX_SHADER);
	fragment_shader = compile_shader(
			fragment_shader_src, GL_FRAGMENT_SHADER);
	shader_program = link_shaders({ vtx_shader, fragment_shader });

	float vertices[] = {
		1.0f, 1.0f, 0.0f, 1.0f, 0.0f,   // top right
		1.0f, -1.0f, 0.0f, 1.0f, 1.0f,  // bottom right
		-1.0f, -1.0f, 0.0f, 0.0f, 1.0f, // bottom left
		-1.0f, 1.0f, 0.0f, 0.0f, 0.0f   // top left
	};

	unsigned int indices[] = {
		0, 1, 3, //
		1, 2, 3, //
	};

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices,
			GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof indices, indices,
			GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
			(void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
			(void *)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NDS_FB_W, NDS_FB_H, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, tbuffer->get_read_buffer().data());
	glGenerateMipmap(GL_TEXTURE_2D);
	glUseProgram(shader_program);
}

void
Display::resizeGL(int w, int h)
{
	this->w = w;
	this->h = h;
}

void
Display::paintGL()
{
	glViewport(0, 0, w, h);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, NDS_FB_W, NDS_FB_H, GL_RGBA,
			GL_UNSIGNED_BYTE, tbuffer->get_read_buffer().data());
	glUseProgram(shader_program);
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

GLuint
Display::compile_shader(const char *src, GLenum type)
{
	int success = 0;
	GLuint shader = glCreateShader(type);

	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success) {
		char msg[512]{};
		glGetShaderInfoLog(shader, sizeof msg, NULL, msg);
		throw twice_error(std::format(
				"failed to compile shader: {}, {}", src, msg));
	}

	return shader;
}

GLuint
Display::link_shaders(std::initializer_list<GLuint> shaders)
{
	int success = 0;
	GLuint program = glCreateProgram();

	for (GLuint shader : shaders) {
		glAttachShader(program, shader);
	}

	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success) {
		char msg[512]{};
		glGetProgramInfoLog(program, sizeof msg, NULL, msg);
		throw twice_error(std::format(
				"failed to link shaders: {}", msg));
	}

	return program;
}

} // namespace twice
