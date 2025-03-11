#ifndef H264ENCODER_H
#define H264ENCODER_H

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
}

#include <span>
#include <vector>

class H264Encoder
{
public:
	H264Encoder() = default;
	H264Encoder(const H264Encoder& other) = delete;
	H264Encoder(H264Encoder&& other) = default;

	H264Encoder& operator=(const H264Encoder& other) = delete;
	H264Encoder& operator=(H264Encoder&& other) = default;

	~H264Encoder();

	bool start(unsigned int width, unsigned int height, unsigned int time_base, unsigned int framerate, unsigned int gopsize = 30);
	void stop();

	bool encode(std::span<const char> frameToEncode, std::vector<char>& dataEncoded);

private:
	bool started = false;

	AVCodecContext* codecCtx = nullptr;
	AVFrame* frame = nullptr;
	AVPacket* pkt = nullptr;
	SwsContext* swsCtx = nullptr;
	unsigned int frameIndex = 0;
};

#endif
