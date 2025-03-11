#include "H264Encoder.h"

#include <cassert>

extern "C"
{
#include <libavutil/opt.h>
}

H264Encoder::~H264Encoder()
{
	stop();
}

bool H264Encoder::start(unsigned int width, unsigned int height, unsigned int time_base, unsigned int framerate, unsigned int gopsize)
{
	assert(width > 0 && height > 0 && time_base > 0 && framerate > 0 && gopsize > 0);

	if (started)
		return false;

	const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);

	if (!codec)
		return false;

	codecCtx = avcodec_alloc_context3(codec);

	if (!codecCtx)
		return false;

	codecCtx->width = width;
	codecCtx->height = height;
	codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	codecCtx->time_base = { 1, static_cast<int>(time_base) };
	codecCtx->bit_rate = framerate;
	codecCtx->gop_size = gopsize;
	codecCtx->max_b_frames = 0;

	av_opt_set(codecCtx->priv_data, "preset", "ultrafast", 0);
	av_opt_set(codecCtx->priv_data, "tune", "zerolatency", 0);

	if (avcodec_open2(codecCtx, codec, nullptr) < 0) 
	{
		avcodec_free_context(&codecCtx);
		codecCtx = nullptr;

		return false;
	}

	frame = av_frame_alloc();

	if (!frame)
	{
		avcodec_free_context(&codecCtx);
		codecCtx = nullptr;

		return false;
	}

	frame->format = codecCtx->pix_fmt;
	frame->width = codecCtx->width;
	frame->height = codecCtx->height;

	av_frame_get_buffer(frame, 32);

	pkt = av_packet_alloc();

	if (!pkt)
	{
		avcodec_free_context(&codecCtx);
		codecCtx = nullptr;

		av_frame_free(&frame);
		frame = nullptr;

		return false;
	}

	swsCtx = sws_getContext(width, height, AV_PIX_FMT_BGR24,
		width, height, AV_PIX_FMT_YUV420P,
		SWS_BICUBIC, nullptr, nullptr, nullptr);

	if (!swsCtx)
	{
		avcodec_free_context(&codecCtx);
		codecCtx = nullptr;

		av_frame_free(&frame);
		frame = nullptr;

		av_packet_free(&pkt);
		pkt = nullptr;

		return false;
	}

	started = true;

	return true;
}

void H264Encoder::stop()
{
	if (!started)
		return;

	started = false;

	frameIndex = 0;

	avcodec_send_frame(codecCtx, nullptr);

	while (avcodec_receive_packet(codecCtx, pkt) == 0) 
		av_packet_unref(pkt);

	av_packet_free(&pkt);
	pkt = nullptr;

	av_frame_free(&frame);
	frame = nullptr;

	avcodec_free_context(&codecCtx);
	codecCtx = nullptr;

	sws_freeContext(swsCtx);
	swsCtx = nullptr;
}

bool H264Encoder::encode(std::span<const char> frameToEncode, std::vector<char>& dataEncoded)
{
	const uint8_t* inData[1] = { reinterpret_cast<const uint8_t*>(frameToEncode.data()) };
	int inLinesize[1] = { codecCtx->width * 3 };

	sws_scale(swsCtx, inData, inLinesize, 0, codecCtx->height, frame->data, frame->linesize);

	frame->pts = frameIndex++;

	if (avcodec_send_frame(codecCtx, frame) < 0)
		return false;

	if (avcodec_receive_packet(codecCtx, pkt) < 0)
		return false;

	dataEncoded.resize(pkt->size);
	std::memcpy(dataEncoded.data(), pkt->data, pkt->size);
		
	av_packet_unref(pkt);

	return true;
}