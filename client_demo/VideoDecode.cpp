#include "pch.h"
#include "VideoDecode.h"
#include "client_demoDlg.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "SDL2/SDL.h"
}

#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)

static char head[4] = { 0x00, 0x00, 0x00, 0x01 };

static int parse_thread(void* data)
{
	((VideoDecode*)data)->ParseThread();
	return 0;
}

static int decode_thread(void* data)
{
	((VideoDecode*)data)->DecodeThread();
	return 0;
}

static int display_thread(void* data)
{
	((VideoDecode*)data)->DisplayThread();
	return 0;
}

VideoDecode::VideoDecode()
{
	m_server = new UdpServer(23003);
	m_server->Open();

	m_frame = nullptr;
	m_codec = nullptr;
	m_codec_ctx = nullptr;
	m_codec_parser_ctx = nullptr;
	for (int i = 0; i < BUFFER_SIZE; i++)
		m_pkts[i] = nullptr;
	m_pkt_index_parse = 0;
	m_pkt_index_codec = -1;
}

VideoDecode::~VideoDecode()
{
	m_server->Close();
	delete m_server;
}

int VideoDecode::Init(void* dlg)
{
	avcodec_register_all();

	// ����ǰ��һ֡����
	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		m_pkts[i] = av_packet_alloc();
		if (!m_pkts[i])
		{
			printf("av_packet_alloc error.\n");
			return -1;
		}
		m_pkts[i]->data = (uint8_t*)malloc(10 * 1024 * 1024);
	}

	// ������һ֡ͼ��
	m_frame = av_frame_alloc();
	if (!m_frame)
	{
		printf("av_frame_alloc error.\n");
		return -1;
	}

	// �Ȳ��ҽ�����
	// �������Ѿ�֪��Ҫ�������Ƶ��h265��ʽ�ģ�����ֱ��ָ����AV_CODEC_ID_H265���������
	// ��������ʹ�õ���ʲô�������Ļ�������ʹ��avformat_find_stream_info�����������ȡ��Ƶ��Ϣ
	m_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!m_codec)
	{
		printf("codec not found: %d\n", AV_CODEC_ID_H264);
		return -1;
	}

	// ��������������
	m_codec_ctx = avcodec_alloc_context3(m_codec);
	if (!m_codec_ctx)
	{
		printf("avcodec_alloc_context3 error.\n");
		return -1;
	}

	// �򿪽�����
	int ret = avcodec_open2(m_codec_ctx, m_codec, NULL);
	if (ret < 0)
	{
		printf("avcodec_open2 error.\n");
		return -1;
	}

	// ����������������һ֡���ݵĿ�ͷ�ͽ�β
	m_codec_parser_ctx = av_parser_init(m_codec->id);
	if (!m_codec_parser_ctx)
	{
		printf("av_parser_init error\n");
		return -1;
	}

	// SDL��ʼ��
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		printf("could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	// ��������
	m_screen = SDL_CreateWindowFrom(((CclientdemoDlg*)dlg)->GetDlgItem(IDC_STATIC)->GetSafeHwnd());
	if (!m_screen)
	{
		printf("could not create window -exiting: %s\n", SDL_GetError());
		return -1;
	}
	m_sdlRenderer = SDL_CreateRenderer(m_screen, -1, 0);
	m_sdlTexture = SDL_CreateTexture(m_sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
		1920, 1080);

	SDL_CreateThread(parse_thread, NULL, this);
	SDL_CreateThread(decode_thread, NULL, this);
	SDL_CreateThread(display_thread, NULL, this);

	return 0;
}

AVFrame* VideoDecode::GetFrame()
{
	return m_frame;
}

void VideoDecode::ParseThread()
{
	char* inbuf = (char*)malloc(1024 * 1024);
	size_t	buf_size = 1024 * 1024;

	int data_len = m_server->Read(inbuf, 102400);

	while (1)
	{
		char *read_buf = (char*)malloc(102400);
		int read_len = m_server->Read(read_buf, 102400);
		printf("read_len: %d\n", read_len);
		if (memcmp(read_buf, head, 4) != 0)
		{
			memcpy(inbuf + data_len, read_buf, read_len);
			data_len += read_len;
			continue;
		}

		AVPacket* pkt = m_pkts[m_pkt_index_parse];
		memcpy(pkt->data, inbuf, data_len);
		pkt->size = data_len;
		m_pkt_index_codec = m_pkt_index_parse;
		m_pkt_index_parse = (m_pkt_index_parse + 1) % BUFFER_SIZE;

		memcpy(inbuf, read_buf, read_len);
		data_len = read_len;
	}
}
void VideoDecode::DecodeThread()
{
	while (1)
	{
		if (m_pkt_index_codec != -1 && m_pkts[m_pkt_index_codec]->size > 0)
		{
			//printf("����һ֡����: %d\n", pkt->size);
			// ��һ֡���ݷ��͵�������
			//clock_t t1 = clock();
			//printf("t1: %ld\n", t1);
			int ret = avcodec_send_packet(m_codec_ctx, m_pkts[m_pkt_index_codec]);
			if (ret < 0)
			{
				printf("error sending a packet for decoding\n");
				Sleep(1);
				continue;
			}

			m_pkts[m_pkt_index_codec]->size = 0;

			// �ӽ������л�ȡһ֡������ͼ��
			ret = avcodec_receive_frame(m_codec_ctx, m_frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			{
				Sleep(1);
				continue;
			}
			else if (ret < 0)
			{
				printf("error during ecoding\n");
				return;
			}

			SDL_Event event;
			event.type = SFM_REFRESH_EVENT;
			SDL_PushEvent(&event);
			Sleep(2);
		}
	}
}

void VideoDecode::DisplayThread()
{
	clock_t old = 0;
	SDL_Event event;
	while (1)
	{
		SDL_WaitEvent(&event);
		if (event.type == SFM_REFRESH_EVENT)
		{
			SDL_Rect rect;
			rect.x = 0; rect.y = 0; rect.w = m_frame->width; rect.h = m_frame->height;
			SDL_UpdateYUVTexture(m_sdlTexture, NULL, m_frame->data[0], m_frame->linesize[0],
				m_frame->data[1], m_frame->linesize[1],
				m_frame->data[2], m_frame->linesize[2]);
			SDL_RenderClear(m_sdlRenderer);
			SDL_RenderCopy(m_sdlRenderer, m_sdlTexture, NULL, NULL);
			SDL_RenderPresent(m_sdlRenderer);
			clock_t t2 = clock();
			//printf("֡���: %ld\n", t2 - old);
			old = t2;
		}
	}
}