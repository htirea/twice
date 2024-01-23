#ifndef TWICE_QT_AUDIO_BUFFER_DEVICE_H
#define TWICE_QT_AUDIO_BUFFER_DEVICE_H

#include <QIODevice>
#include <iostream>

class AudioBufferDevice : public QIODevice {
	Q_OBJECT

	static constexpr qint64 N = 16384;

      public:
	AudioBufferDevice(QObject *parent = nullptr) : QIODevice(parent) {}

	void start()
	{
		open(QIODeviceBase::ReadWrite | QIODeviceBase::Unbuffered);
	}

	void stop() { close(); }

	void readLast(char *data, qint64 count)
	{
		auto old_count = count;

		if (count > size) {
			count = size;
		}

		auto idx = (write_idx + N - count) % N;
		auto count1 = std::min(count, N - idx);
		auto count2 = count - count1;

		std::memcpy(data, buffer + idx, count1);
		std::memcpy(data + count1, buffer, count2);

		if (old_count != count) {
			std::memset(data + count1 + count2, 0,
					old_count - count);
		}
	}

      protected:
	qint64 readData(char *data, qint64 count) override
	{
		auto old_count = count;

		if (count > size) {
			count = size;
		}

		auto count1 = std::min(count, N - read_idx);
		auto count2 = count - count1;

		std::memcpy(data, buffer + read_idx, count1);
		std::memcpy(data + count1, buffer, count2);

		if (old_count != count) {
			std::memset(data + count1 + count2, 0,
					old_count - count);
		}

		read_idx = (read_idx + count) % N;
		size -= count;

		return count;
	}

	qint64 writeData(const char *data, qint64 count) override
	{
		if (count > N) {
			auto skip = count - N;
			data += skip;
			write_idx = (write_idx + skip) % N;
			count = N;
		}

		auto count1 = std::min(count, N - write_idx);
		auto count2 = count - count1;

		std::memcpy(buffer + write_idx, data, count1);
		std::memcpy(buffer, data + count1, count2);

		write_idx = (write_idx + count) % N;
		size = std::min(size + count, N);

		return count;
	}

      private:
	qint64 size{};
	qint64 read_idx{};
	qint64 write_idx{};
	char buffer[N]{};
};

using AudioInputBufferDevice = AudioBufferDevice;

#endif
