#ifndef TWICE_DISPLAY_H
#define TWICE_DISPLAY_H

#include <QOpenGLFunctions>
#include <QOpenGLWidget>

namespace twice {

class Display : public QOpenGLWidget, protected QOpenGLFunctions {
      public:
	Display();

      protected:
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;
};

} // namespace twice

#endif // TWICE_DISPLAY_H
