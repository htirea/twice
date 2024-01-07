#ifndef TWICE_DISPLAY_H
#define TWICE_DISPLAY_H

#include <mutex>

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>

#include "libtwice/nds/defs.h"
#include "libtwice/types.h"

namespace twice {

class Display : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
      public:
	Display();
	~Display();
	void render();

      protected:
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;

      private:
	GLuint compile_shader(const char *src, GLenum type);
	GLuint link_shaders(std::initializer_list<GLuint> shaders);

      public:
	std::array<u32, NDS_FB_SZ> fb{};
	std::mutex fb_mtx;

      private:
	GLuint vbo{};
	GLuint vao{};
	GLuint ebo{};
	GLuint vtx_shader{};
	GLuint fragment_shader{};
	GLuint shader_program{};
	GLuint texture{};
	int w{};
	int h{};
};

} // namespace twice

#endif // TWICE_DISPLAY_H
