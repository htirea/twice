#ifndef TWICE_QT_DISPLAY_WIDGET_H
#define TWICE_QT_DISPLAY_WIDGET_H

#include <QAction>
#include <QActionGroup>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>

#include <array>

#include "buffers.h"

class ConfigManager;

class DisplayWidget : public QOpenGLWidget,
		      protected QOpenGLFunctions_3_3_Core {
	Q_OBJECT

	friend class MainWindow;

      public:
	DisplayWidget(SharedBuffers::video_buffer *fb, ConfigManager *cfg,
			QWidget *parent);
	~DisplayWidget();

      protected:
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;

      private:
	void update_projection_mtx();
	GLuint compile_shader(const char *src, GLenum type);
	GLuint link_shaders(std::initializer_list<GLuint> shaders);
	GLuint make_program(const char *vtx_src, const char *frag_src);

      private slots:
	void display_var_set(int key, const QVariant& v);

      private:
	int w{};
	int h{};
	int orientation{};
	bool lock_aspect_ratio{};
	bool linear_filtering{};
	std::array<float, 16> proj_mtx{};
	SharedBuffers::video_buffer *fb{};

	/* OpenGL stuff */
	GLuint shader_program{};
	GLuint vao{};
	GLuint vbo{};
	GLuint ebo{};
	GLuint texture{};
	GLuint sampler{};
};

#endif
