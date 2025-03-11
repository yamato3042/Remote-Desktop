#ifndef H264DECODER_H
#define H264DECODER_H

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
}

#include <span>
#include <vector>

class H264Decoder
{
public:
	H264Decoder() = default;
	H264Decoder(const H264Decoder& other) = delete;
	H264Decoder(H264Decoder&& other) = default;

	H264Decoder& operator=(const H264Decoder& other) = delete;
	H264Decoder& operator=(H264Decoder&& other) = default;

	~H264Decoder();

	bool start(unsigned int width, unsigned int height);
	void stop();

	bool decode(std::span<char> encodedData, std::vector<char>& frameDecoder) const;

private:
	bool started = false;

	AVCodecContext* codecCtx = nullptr;
	AVFrame* frame = nullptr;
	AVPacket* pkt = nullptr;
	SwsContext* swsCtx = nullptr;
};

#endif
