#include "audio_out.h"

#include "libtwice/exception.h"

#include <SDL.h>

using namespace twice;

struct AudioOut::impl {
	impl(SharedBuffers::audio_buffer *ab) : ab(ab) {}

	SharedBuffers::audio_buffer *ab;
	stopwatch tmr;
	SDL_AudioDeviceID dev;
	SDL_AudioSpec spec;
};

AudioOut::AudioOut(SharedBuffers::audio_buffer *ab, QObject *parent)
	: QObject(parent), m(std::make_unique<impl>(ab))
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
}

AudioOut::~AudioOut()
{
	if (m->dev) {
		SDL_CloseAudioDevice(m->dev);
	}
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	SDL_Quit();
}

void
AudioOut::push_audio(size_t len)
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

	SDL_QueueAudio(m->dev, m->ab->get_read_buffer().data(), len);
}
