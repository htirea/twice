#include "audio_io.h"

#include "libtwice/exception.h"

#include <SDL.h>

using namespace twice;

void
audio_in_cb(void *userdata, Uint8 *stream, int len)
{
	auto mb = (SharedBuffers::mic_buffer *)userdata;
	mb->write_locked((const char *)stream, len);
}

struct AudioIO::impl {
	impl(SharedBuffers *bufs) : bufs(bufs) {}

	SharedBuffers *bufs{};
	stopwatch tmr;
	SDL_AudioDeviceID dev;
	SDL_AudioSpec spec;
	SDL_AudioDeviceID mic_dev;
	SDL_AudioSpec mic_spec;
};

AudioIO::AudioIO(SharedBuffers *bufs, QObject *parent)
	: QObject(parent), m(std::make_unique<impl>(bufs))
{
	if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		throw twice_error("SDL: failed to initialize audio.");
	}

	SDL_AudioSpec want;
	SDL_memset(&want, 0, sizeof(want));
	want.freq = NDS_AUDIO_SAMPLE_RATE;
	want.format = AUDIO_S16SYS;
	want.channels = 2;
	want.samples = 1024;
	want.callback = NULL;
	m->dev = SDL_OpenAudioDevice(NULL, 0, &want, &m->spec, 0);
	if (!m->dev) {
		throw twice_error("SDL: failed to create audio device.");
	}
	SDL_PauseAudioDevice(m->dev, 0);

	SDL_AudioSpec want_mic;
	SDL_memset(&want_mic, 0, sizeof(want_mic));
	want_mic.freq = NDS_AUDIO_SAMPLE_RATE;
	want_mic.format = AUDIO_S16SYS;
	want_mic.channels = 1;
	want_mic.samples = 1024;
	want_mic.callback = audio_in_cb;
	want_mic.userdata = &bufs->mb;
	m->mic_dev = SDL_OpenAudioDevice(NULL, 1, &want_mic, &m->mic_spec, 0);
	if (m->mic_dev) {
		SDL_PauseAudioDevice(m->mic_dev, 0);
	}
}

AudioIO::~AudioIO()
{
	if (m->dev) {
		SDL_CloseAudioDevice(m->dev);
	}

	if (m->mic_dev) {
		SDL_CloseAudioDevice(m->mic_dev);
	}

	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	SDL_Quit();
}

void
AudioIO::push_audio(size_t len)
{
	static double acc = 0;
	double early_threshold = -NDS_AVG_SAMPLES_PER_FRAME;
	double late_threshold = NDS_AVG_SAMPLES_PER_FRAME;

	auto elapsed = m->tmr.start();

	if (len == 0) {
		return;
	}

	double samples = NDS_AVG_SAMPLES_PER_FRAME;
	double samples_max =
			NDS_AUDIO_SAMPLE_RATE * to_seconds<double>(elapsed);
	acc += samples - samples_max;

	if (acc < early_threshold) {
		acc = early_threshold;
	}

	if (acc > late_threshold) {
		acc -= late_threshold;
		return;
	}

	SDL_QueueAudio(m->dev, m->bufs->ab.get_read_buffer().data(), len);
}
