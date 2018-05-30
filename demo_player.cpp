#include <stdio.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include <SDL2/SDL.h>
};

int WinMain()
{
    AVFormatContext* format_ctx;
    AVCodecContext* codec_ctx;
    AVCodec* codec;
    AVFrame *frame, *frame_yuv;
    AVPacket* packet;
    unsigned char* out_buffer;
    struct SwsContext* img_convert_ctx;
    char file_path[] = "bigbuckbunny_480x272.h265";
    int ret, v_index = -1;
    int got_picture;

    int screen_w = 0, screen_h = 0;
    SDL_Window* screen;
    SDL_Renderer* sdl_renderer;
    SDL_Texture* sdl_texture;
    SDL_Rect sdl_rect;

    av_register_all();
    avformat_network_init();

    format_ctx = avformat_alloc_context();

    if (avformat_open_input(&format_ctx, file_path, NULL, NULL) != 0) {
        printf("Couldn't open input stream.\n");
        return -1;
    }

    if (avformat_find_stream_info(format_ctx, NULL) < 0) {
        printf("Couldn't find stream information.\n");
        return -1;
    }

    for (int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            v_index = i;
            break;
        }
    }

    if (v_index == -1) {
        printf("Didn't find a video stream.\n");
        return -1;
    }

    codec_ctx = format_ctx->streams[v_index]->codec;
    codec = avcodec_find_decoder(codec_ctx->codec_id);
    if (codec == NULL) {
        printf("Codec not found.\n");
        return -1;
    }

    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        printf("Could not open codec.\n");
        return -1;
    }

    frame = av_frame_alloc();
    frame_yuv = av_frame_alloc();

    screen_w = codec_ctx->width * 2;
    screen_h = codec_ctx->height * 2;

    out_buffer = (unsigned char*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, screen_w, screen_h, 1));
    av_image_fill_arrays(frame_yuv->data, frame_yuv->linesize, out_buffer, AV_PIX_FMT_YUV420P, screen_w, screen_h, 1);

    packet = (AVPacket*)av_malloc(sizeof(AVPacket));
    printf("--------------- File Information ----------------\n");
    av_dump_format(format_ctx, 0, file_path, 0);
    printf("-------------------------------------------------\n");

    img_convert_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt, screen_w, screen_h,
        AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    screen = SDL_CreateWindow("SDL Window demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_OPENGL);
    if (screen == 0) {
        printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
        return -1;
    }

    sdl_renderer = SDL_CreateRenderer(screen, -1, 0);
    sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, screen_w, screen_h);

    sdl_rect.x = 0;
    sdl_rect.y = 0;
    sdl_rect.w = screen_w;
    sdl_rect.h = screen_h;

    while (av_read_frame(format_ctx, packet) == 0) {
        if (packet->stream_index != v_index) {
            av_free_packet(packet);
            continue;
        }
        
        ret = avcodec_decode_video2(codec_ctx, frame, &got_picture, packet);
        if (ret < 0) {
            printf("Decode Error.\n");
            av_free_packet(packet);
            return -1;
        }

        if (got_picture == 0) {
            av_free_packet(packet);
            continue;
        }

        sws_scale(img_convert_ctx, frame->data, frame->linesize, 0, codec_ctx->height, frame_yuv->data, frame_yuv->linesize);
        SDL_UpdateYUVTexture(sdl_texture, &sdl_rect,
            frame_yuv->data[0], frame_yuv->linesize[0],
            frame_yuv->data[1], frame_yuv->linesize[1],
            frame_yuv->data[2], frame_yuv->linesize[2]);

        SDL_RenderClear(sdl_renderer);
        SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, &sdl_rect);
        SDL_RenderPresent(sdl_renderer);

        av_free_packet(packet);
        
        SDL_Delay(40);
    }

    while (true) {
        ret = avcodec_decode_video2(codec_ctx, frame, &got_picture, packet);
        if (ret < 0 || got_picture == 0) {
            break;
        }

        sws_scale(img_convert_ctx, frame->data, frame->linesize, 0, codec_ctx->height, frame_yuv->data, frame_yuv->linesize);
        SDL_UpdateYUVTexture(sdl_texture, &sdl_rect,
            frame_yuv->data[0], frame_yuv->linesize[0],
            frame_yuv->data[1], frame_yuv->linesize[1],
            frame_yuv->data[2], frame_yuv->linesize[2]);
        SDL_RenderClear(sdl_renderer);
        SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, &sdl_rect);
        SDL_RenderPresent(sdl_renderer);

        SDL_Delay(40);
    }

    sws_freeContext(img_convert_ctx);

    SDL_Quit();

    av_frame_free(&frame_yuv);
    av_frame_free(&frame);
    avcodec_close(codec_ctx);
    avformat_close_input(&format_ctx);

    return 0;
}