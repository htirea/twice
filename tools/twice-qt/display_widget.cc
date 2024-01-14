#include "display_widget.h"

#include <iostream>

#include <QMouseEvent>

#include "libtwice/exception.h"
#include "libtwice/nds/display.h"
#include "libtwice/util/matrix.h"

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

uniform sampler2D texture0;

void main()
{
	FragColor = texture(texture0, uv);
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
	glDeleteSamplers(1, &sampler);
	glDeleteTextures(1, &texture);
	glDeleteProgram(shader_program);
	glDeleteShader(fragment_shader);
	glDeleteShader(vtx_shader);
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
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
	float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NDS_FB_W, NDS_FB_H, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, tbuffer->get_read_buffer().data());
	glUseProgram(shader_program);
	GLuint proj_mtx_loc = glGetUniformLocation(shader_program, "proj_mtx");
	glUniformMatrix4fv(proj_mtx_loc, 1, GL_FALSE, proj_mtx.data());
	glUniform1i(glGetUniformLocation(shader_program, "texture0"), 0);

	glGenSamplers(1, &sampler);
	glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
	update_projection_mtx();
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	if (linear_filtering) {
		glBindSampler(0, sampler);
	} else {
		glBindSampler(0, 0);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, NDS_FB_W, NDS_FB_H, GL_RGBA,
			GL_UNSIGNED_BYTE, tbuffer->get_read_buffer().data());

	glUseProgram(shader_program);
	GLuint proj_mtx_loc = glGetUniformLocation(shader_program, "proj_mtx");
	glUniformMatrix4fv(proj_mtx_loc, 1, GL_FALSE, proj_mtx.data());
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void
DisplayWidget::mousePressEvent(QMouseEvent *e)
{
	auto button = e->button();
	if (button != Qt::LeftButton && button != Qt::RightButton) {
		e->ignore();
		return;
	}

	if (button == Qt::LeftButton) {
		mouse1_down = true;
	}

	if (button == Qt::RightButton) {
		mouse2_down = true;
	}

	auto pos = e->position();
	if (auto coords = window_coords_to_screen_coords(w, h, pos.x(),
			    pos.y(), letterboxed, orientation, 0)) {
		event_q->push(TouchEvent{ .x = coords->first,
				.y = coords->second,
				.down = true,
				.quicktap = button == Qt::RightButton,
				.move = false });
	}
}

void
DisplayWidget::mouseReleaseEvent(QMouseEvent *e)
{
	if (!mouse1_down && !mouse2_down) {
		e->ignore();
		return;
	}

	auto button = e->button();
	if (button == Qt::LeftButton) {
		mouse1_down = false;
	}

	if (button == Qt::RightButton) {
		mouse2_down = false;
	}

	event_q->push(TouchEvent{ .x = 0,
			.y = 0,
			.down = false,
			.quicktap = false,
			.move = false });
}

void
DisplayWidget::mouseMoveEvent(QMouseEvent *e)
{
	if (!mouse1_down) {
		e->ignore();
		return;
	}

	auto pos = e->position();
	if (auto coords = window_coords_to_screen_coords(w, h, pos.x(),
			    pos.y(), letterboxed, orientation, 0)) {
		event_q->push(TouchEvent{ .x = coords->first,
				.y = coords->second,
				.down = true,
				.quicktap = false,
				.move = true });
	} else {
		event_q->push(TouchEvent{ .x = 0,
				.y = 0,
				.down = false,
				.quicktap = false,
				.move = false });
	}
}

void
DisplayWidget::update_projection_mtx()
{
	float sx = 1.0;
	float sy = 1.0;

	if (letterboxed) {
		double ratio = (double)w / h;
		double target_ratio;
		if (orientation & 1) {
			target_ratio = NDS_FB_ASPECT_RATIO_RECIP;
		} else {
			target_ratio = NDS_FB_ASPECT_RATIO;
		}

		if (ratio < target_ratio) {
			sy = w / target_ratio / h;
		} else if (ratio > target_ratio) {
			sx = h * target_ratio / w;
		}

		if (orientation & 1) {
			std::swap(sx, sy);
		}
	}

	proj_mtx.fill(0);
	proj_mtx[10] = 1.0;
	proj_mtx[15] = 1.0;

	switch (orientation) {
	case 0:
		proj_mtx[0] = sx;
		proj_mtx[5] = sy;
		break;
	case 1:
		proj_mtx[1] = -sx;
		proj_mtx[4] = sy;
		break;
	case 2:
		proj_mtx[0] = -sx;
		proj_mtx[5] = -sy;
		break;
	case 3:
		proj_mtx[1] = sx;
		proj_mtx[4] = -sy;
		break;
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
