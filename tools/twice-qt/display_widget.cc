#include "display_widget.h"

#include <iostream>

#include <QMouseEvent>

#include "libtwice/exception.h"
#include "libtwice/nds/display.h"

namespace twice {

static const char *vtx_shader_src = R"___(
#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv_in;

uniform mat4 proj_mtx;

out vec2 uv;

void main()
{
	gl_Position = proj_mtx * vec4(pos, 1.0);
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

DisplayWidget::DisplayWidget(
		triple_buffer<std::array<u32, NDS_FB_SZ>> *tbuffer,
		threaded_queue<Event> *event_q)
	: tbuffer(tbuffer), event_q(event_q)
{
	mtx_set_identity<float, 4>(proj_mtx);
}

DisplayWidget::~DisplayWidget()
{
	makeCurrent();
	glDeleteShader(vtx_shader);
	glDeleteShader(fragment_shader);
}

void
DisplayWidget::render()
{
	update();
}

void
DisplayWidget::initializeGL()
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
	GLuint proj_mtx_loc = glGetUniformLocation(shader_program, "proj_mtx");
	glUniformMatrix4fv(proj_mtx_loc, 1, GL_FALSE, proj_mtx);
}

void
DisplayWidget::resizeGL(int w, int h)
{
	this->w = w;
	this->h = h;
}

void
DisplayWidget::paintGL()
{
	glViewport(0, 0, w, h);
	update_projection_mtx();
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, NDS_FB_W, NDS_FB_H, GL_RGBA,
			GL_UNSIGNED_BYTE, tbuffer->get_read_buffer().data());
	glUseProgram(shader_program);
	GLuint proj_mtx_loc = glGetUniformLocation(shader_program, "proj_mtx");
	glUniformMatrix4fv(proj_mtx_loc, 1, GL_FALSE, proj_mtx);
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void
DisplayWidget::mousePressEvent(QMouseEvent *e)
{
	auto pos = e->position();
	if (auto coords = window_coords_to_screen_coords(
			    w, h, pos.x(), pos.y(), letterboxed, 0, 0)) {
		event_q->push(TouchEvent{ .x = coords->first,
				.y = coords->second,
				.down = true });
	}
}

void
DisplayWidget::mouseReleaseEvent(QMouseEvent *)
{
	event_q->push(TouchEvent{ .x = 0, .y = 0, .down = false });
}

void
DisplayWidget::mouseMoveEvent(QMouseEvent *e)
{
	auto pos = e->position();
	if (auto coords = window_coords_to_screen_coords(
			    w, h, pos.x(), pos.y(), letterboxed, 0, 0)) {
		event_q->push(TouchEvent{ .x = coords->first,
				.y = coords->second,
				.down = true });
	} else {
		event_q->push(TouchEvent{ .x = 0, .y = 0, .down = false });
	}
}

void
DisplayWidget::update_projection_mtx()
{
	mtx_set_identity<float, 4>(proj_mtx);

	if (letterboxed) {
		double ratio = w / h;
		if (ratio < NDS_FB_ASPECT_RATIO) {
			proj_mtx[5] = w / NDS_FB_ASPECT_RATIO / h;
		} else if (ratio > NDS_FB_ASPECT_RATIO) {
			proj_mtx[0] = h * NDS_FB_ASPECT_RATIO / w;
		}
	}
}

GLuint
DisplayWidget::compile_shader(const char *src, GLenum type)
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
DisplayWidget::link_shaders(std::initializer_list<GLuint> shaders)
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
