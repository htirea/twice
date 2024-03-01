#include "display_widget.h"

#include "config_manager.h"
#include "shaders/lcd1x_nds.h"
#include "shaders/nds_color.h"
#include "shaders/none.h"

#include "libtwice/exception.h"
#include "libtwice/nds/defs.h"
#include "libtwice/types.h"
#include "libtwice/util/matrix.h"

using namespace twice;

DisplayWidget::DisplayWidget(SharedBuffers::video_buffer *fb,
		ConfigManager *cfg, QWidget *parent)
	: QOpenGLWidget(parent), fb(fb)
{
	connect(cfg, &ConfigManager::display_key_set, this,
			&DisplayWidget::display_var_set);
	mtx_set_identity<float, 4>(proj_mtx);
}

DisplayWidget::~DisplayWidget()
{
	makeCurrent();

	glDeleteSamplers(1, &sampler);
	glDeleteTextures(1, &texture);
	glDeleteBuffers(1, &ebo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	for (GLuint shader_program : shader_programs) {
		glDeleteProgram(shader_program);
	}
}

void
DisplayWidget::initializeGL()
{
	initializeOpenGLFunctions();

	shader_programs[Shader::NONE] =
			make_program(vtx_shader_src, fragment_shader_src);
	shader_programs[Shader::LCD1X_NDS_COLOR] =
			make_program(vtx_shader_src, lcd1x_nds_frag_src);
	shader_programs[Shader::NDS_COLOR] =
			make_program(vtx_shader_src, nds_color_frag_src);

	float vertices[] = {
		1.0f, 1.0f, 0.0f, 1.0f, 0.0f,   // top right
		1.0f, -1.0f, 0.0f, 1.0f, 1.0f,  // bottom right
		-1.0f, -1.0f, 0.0f, 0.0f, 1.0f, // bottom left
		-1.0f, 1.0f, 0.0f, 0.0f, 0.0f   // top left
	};

	unsigned int indices[] = {
		0, 1, 3, // top right, bottom right, top left
		1, 2, 3, // bottom right, bottom left, top left
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
			GL_UNSIGNED_BYTE, fb->get_read_buffer().data());
	GLuint program = shader_programs[shader];
	glUseProgram(program);
	GLuint proj_mtx_loc = glGetUniformLocation(program, "proj_mtx");
	glUniformMatrix4fv(proj_mtx_loc, 1, GL_FALSE, proj_mtx.data());
	glUniform1i(glGetUniformLocation(program, "texture0"), 0);

	glGenSamplers(1, &sampler);
	glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (glGetError() != GL_NO_ERROR) {
		throw twice_error(
				"An error occurred while initializing OpenGL.");
	}
}

void
DisplayWidget::resizeGL(int new_w, int new_h)
{
	w = new_w;
	h = new_h;
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
			GL_UNSIGNED_BYTE, fb->get_read_buffer().data());

	GLuint program = shader_programs[shader];
	glUseProgram(program);
	GLuint proj_mtx_loc = glGetUniformLocation(program, "proj_mtx");
	glUniformMatrix4fv(proj_mtx_loc, 1, GL_FALSE, proj_mtx.data());
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void
DisplayWidget::update_projection_mtx()
{
	float sx = 1.0;
	float sy = 1.0;

	if (lock_aspect_ratio) {
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
	GLint success = 0;

	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success) {
		char msg[512]{};
		glGetShaderInfoLog(shader, sizeof msg, NULL, msg);
		glDeleteShader(shader);
		throw twice_error(std::format(
				"Error while compiling shader: {}", msg));
	}

	return shader;
}

GLuint
DisplayWidget::link_shaders(std::initializer_list<GLuint> shaders)
{
	GLint success = 0;
	GLuint program = glCreateProgram();

	for (auto shader : shaders) {
		glAttachShader(program, shader);
	}

	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success) {
		char msg[512]{};
		glGetProgramInfoLog(program, sizeof msg, NULL, msg);
		glDeleteProgram(program);
		throw twice_error(std::format(
				"Error while linking shader program: {}",
				msg));
	}

	return program;
}

GLuint
DisplayWidget::make_program(const char *vtx_src, const char *frag_src)
{
	auto vtx_shader = compile_shader(vtx_src, GL_VERTEX_SHADER);
	auto fragment_shader = compile_shader(frag_src, GL_FRAGMENT_SHADER);
	auto program = link_shaders({ vtx_shader, fragment_shader });

	glDeleteShader(vtx_shader);
	glDeleteShader(fragment_shader);

	return program;
}

void
DisplayWidget::display_var_set(int key, const QVariant& v)
{
	if (!v.isValid())
		return;

	using namespace ConfigVariable;

	switch (key) {
	case ORIENTATION:
		orientation = v.toInt();
		break;
	case LOCK_ASPECT_RATIO:
		lock_aspect_ratio = v.toBool();
		break;
	case LINEAR_FILTERING:
		linear_filtering = v.toBool();
		break;
	case SHADER:
		shader = v.toInt();
	}
}
