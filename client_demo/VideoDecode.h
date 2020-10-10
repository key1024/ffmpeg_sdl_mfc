#pragma once

#include "UdpServer.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "SDL2/SDL.h"
}

#define BUFFER_SIZE 10

class VideoDecode
{
public:
	VideoDecode();
	~VideoDecode();

	int Init(void *dlg);

	void ParseThread();
	void DecodeThread();
	void DisplayThread();

	AVFrame* GetFrame();

private:
	UdpServer* m_server;
	AVPacket* m_pkts[BUFFER_SIZE];
	AVFrame* m_frame;
	AVCodec* m_codec;
	AVCodecContext* m_codec_ctx;
	AVCodecParserContext* m_codec_parser_ctx;
	int m_pkt_index_parse;
	int m_pkt_index_codec;

	SDL_Window* m_screen;
	SDL_Renderer* m_sdlRenderer;
	SDL_Texture* m_sdlTexture;
};

