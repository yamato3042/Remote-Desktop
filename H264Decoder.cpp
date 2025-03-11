#include "H264Decoder.h"

#include <cassert>

H264Decoder::~H264Decoder()
{
    stop();
}

bool H264Decoder::start(unsigned int width, unsigned int height)
{
    assert(width > 0 && height > 0);

    if (started)
        return false;

    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    
    if (!codec)
        return false;

    codecCtx = avcodec_alloc_context3(codec);

    if (!codecCtx)
        return false;

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

    pkt = av_packet_alloc();

    if (!pkt)
    {
        avcodec_free_context(&codecCtx);
        codecCtx = nullptr;

        av_frame_free(&frame);
        frame = nullptr;

        return false;
    }

    swsCtx = sws_getContext(width, height, AV_PIX_FMT_YUV420P,
        width, height, AV_PIX_FMT_BGR24,
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

void H264Decoder::stop() 
{
    if (!started)
        return;

    av_packet_free(&pkt);
    pkt = nullptr;

    av_frame_free(&frame);
    frame = nullptr;

    avcodec_free_context(&codecCtx);
    codecCtx = nullptr;

    sws_freeContext(swsCtx);
    swsCtx = nullptr;
}

bool H264Decoder::decode(std::span<char> encodedData, std::vector<char>& frameDecoded) const
{
    pkt->data = reinterpret_cast<uint8_t*>(encodedData.data());
    pkt->size = static_cast<int>(encodedData.size());

    if (avcodec_send_packet(codecCtx, pkt) < 0)
        return false;

    if (avcodec_receive_frame(codecCtx, frame) < 0)
        return false;

    frameDecoded.resize(frame->width * frame->height * 3);

    uint8_t* dst[1] = { reinterpret_cast<uint8_t*>(frameDecoded.data()) };
    int linesize[1] = { frame->width * 3 };

    sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height, dst, linesize);

    av_frame_unref(frame);

    return true;
}
