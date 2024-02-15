#ifndef TWICE_QT_AUDIO_OUT_H
#define TWICE_QT_AUDIO_OUT_H

#include <QObject>

#include "libtwice/util/stopwatch.h"

#include "buffers.h"

class AudioOut : QObject {
	Q_OBJECT

      public:
	AudioOut(SharedBuffers::audio_buffer *ab, QObject *parent);
	~AudioOut();
	void push_audio(size_t len);

      private:
	struct impl;
	std::unique_ptr<impl> m;
};

#endif
