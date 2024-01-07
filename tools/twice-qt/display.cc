#include "display.h"

#include "libtwice/exception.h"

namespace twice {

static const char *vtx_shader_src = R"___(
#version 330 core
layout (location = 0) in vec3 aPos;

void main()
{
	gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)___";

static const char *fragment_shader_src = R"___(
#version 330 core
out vec4 FragColor;

void main()
{
	FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
)___";

Display::Display() {}

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

	float vertices[] = { -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.5f,
		0.0f };
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices,
			GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
			(void *)0);
	glEnableVertexAttribArray(0);
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
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(shader_program);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
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
