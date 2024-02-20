#include "audio_io.h"

#include "libtwice/exception.h"

#include <SDL.h>

#define SDL_MAIN_HANDLED

using namespace twice;

void
audio_in_cb(void *userdata, Uint8 *stream, int len)
{
	auto mb = (SharedBuffers::mic_buffer *)userdata;
	mb->write_locked((const char *)stream, len);
}

void
audio_out_cb(void *userdata, Uint8 *stream, int len)
{
	auto ab = (SharedBuffers::audio_buffer *)userdata;
	ab->read_fill_zero_locked((char *)stream, len);
}

struct AudioIO::impl {
	impl(SharedBuffers *bufs) : bufs(bufs) {}

	SharedBuffers *bufs{};
	stopwatch tmr;
	SDL_AudioDeviceID dev;
	SDL_AudioDeviceID mic_dev;
};

AudioIO::AudioIO(SharedBuffers *bufs, QObject *parent)
	: QObject(parent), m(std::make_unique<impl>(bufs))
{
	if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		throw twice_error("SDL: failed to initialize audio.");
	}

	SDL_AudioSpec obtained;

	SDL_AudioSpec want;
	SDL_memset(&want, 0, sizeof(want));
	want.freq = NDS_AUDIO_SAMPLE_RATE;
	want.format = AUDIO_S16SYS;
	want.channels = 2;
	want.samples = 1024;
	want.callback = audio_out_cb;
	want.userdata = &bufs->ab;
	m->dev = SDL_OpenAudioDevice(NULL, 0, &want, &obtained, 0);
	if (m->dev) {
		SDL_PauseAudioDevice(m->dev, 0);
	}

	want.freq = NDS_AUDIO_SAMPLE_RATE;
	want.format = AUDIO_S16SYS;
	want.channels = 1;
	want.samples = 1024;
	want.callback = audio_in_cb;
	want.userdata = &bufs->mb;
	m->mic_dev = SDL_OpenAudioDevice(NULL, 1, &want, &obtained, 0);
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
