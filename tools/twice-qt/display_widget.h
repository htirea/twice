#ifndef TWICE_QT_DISPLAY_WIDGET_H
#define TWICE_QT_DISPLAY_WIDGET_H

#include <mutex>

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>

#include "libtwice/nds/defs.h"
#include "libtwice/types.h"
#include "libtwice/util/threaded_queue.h"
#include "libtwice/util/triple_buffer.h"

#include "emulator_events.h"

namespace twice {

class DisplayWidget : public QOpenGLWidget,
		      protected QOpenGLFunctions_3_3_Core {
	friend class MainWindow;

      public:
	DisplayWidget(triple_buffer<std::array<u32, NDS_FB_SZ>> *tbuffer,
			threaded_queue<Event> *event_q);
	~DisplayWidget();
	void render();

      protected:
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;
	void mousePressEvent(QMouseEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;

      private:
	void update_projection_mtx();
	GLuint compile_shader(const char *src, GLenum type);
	GLuint link_shaders(std::initializer_list<GLuint> shaders);

      private:
	GLuint vbo{};
	GLuint vao{};
	GLuint ebo{};
	GLuint vtx_shader{};
	GLuint fragment_shader{};
	GLuint shader_program{};
	GLuint texture{};
	std::array<float, 16> proj_mtx;
	double w{};
	double h{};
	int orientation{};
	bool letterboxed{ true };
	bool mouse1_down{ false };
	bool mouse2_down{ false };
	triple_buffer<std::array<u32, NDS_FB_SZ>> *tbuffer;
	threaded_queue<Event> *event_q{};
};

} // namespace twice

#endif // TWICE_DISPLAY_H
