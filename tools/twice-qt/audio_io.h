#ifndef TWICE_QT_AUDIO_OUT_H
#define TWICE_QT_AUDIO_OUT_H

#include <QObject>

#include "libtwice/util/stopwatch.h"

#include "buffers.h"

class AudioIO : QObject {
	Q_OBJECT

      public:
	AudioIO(SharedBuffers *bufs, QObject *parent);
	~AudioIO();

      private:
	struct impl;
	std::unique_ptr<impl> m;
};

#endif
