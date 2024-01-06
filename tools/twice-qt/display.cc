#include "display.h"

namespace twice {

Display::Display() {}

void
Display::initializeGL()
{
	initializeOpenGLFunctions();
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
}

void
Display::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
}

void
Display::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT);
}

} // namespace twice
