#include <stdio.h>
#include <unistd.h>

// #define USE_SDL
#define USE_OPENCV

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#ifdef USE_SDL
#include <SDL2/SDL.h>
#endif
};

#ifdef USE_OPENCV
#include <opencv2/opencv.hpp>
using namespace cv;
using namespace std;
#endif

#ifdef USE_OPENCV

#define COLOR_BLOCK_SIZE 256

#define HISTOGRAM_IMAGE_WIDTH 256
#define HISTOGRAM_IMAGE_HEIGHT 256

#define LINE_WIDTH (HISTOGRAM_IMAGE_WIDTH / COLOR_BLOCK_SIZE)

void showColorRatioByChannel(int channel, cv::Mat &rgb_mat)
{
    int histsize[] = {COLOR_BLOCK_SIZE};
    float midranges[] = {0, 256 - 1};
    const float *ranges[] = {midranges};
    MatND dsthist;

    cv::Mat drawImage = Mat::zeros(Size(HISTOGRAM_IMAGE_WIDTH, HISTOGRAM_IMAGE_HEIGHT), CV_8UC3);

    calcHist(&rgb_mat, 1, &channel, Mat(), dsthist, 1, histsize, ranges, true, false);

    double g_dhistmaxvalue;
    minMaxLoc(dsthist, 0, &g_dhistmaxvalue, 0, 0);
    for (int i = 0; i < COLOR_BLOCK_SIZE; i++)
    {
        int value = cvRound(HISTOGRAM_IMAGE_HEIGHT * (dsthist.at<float>(i) / g_dhistmaxvalue));
        switch (channel)
        {
        case 0:
            rectangle(drawImage, Point(i * LINE_WIDTH, drawImage.rows - 1), Point(i * LINE_WIDTH, drawImage.rows - 1 - value), Scalar(255, 0, 0), LINE_WIDTH);
            break;
        case 1:
            rectangle(drawImage, Point(i * LINE_WIDTH, drawImage.rows - 1), Point(i * LINE_WIDTH, drawImage.rows - 1 - value), Scalar(0, 255, 0), LINE_WIDTH);
            break;
        case 2:
            rectangle(drawImage, Point(i * LINE_WIDTH, drawImage.rows - 1), Point(i * LINE_WIDTH, drawImage.rows - 1 - value), Scalar(0, 0, 255), LINE_WIDTH);
            break;
        default:
            rectangle(drawImage, Point(i * LINE_WIDTH, drawImage.rows - 1), Point(i * LINE_WIDTH, drawImage.rows - 1 - value), Scalar(255, 255, 255), LINE_WIDTH);
            break;
        }
    }

    switch (channel)
    {
    case 0:
        imshow("B", drawImage);
        break;
    case 1:
        imshow("G", drawImage);
        break;
    case 2:
        imshow("R", drawImage);
        break;
    default:
        imshow("U", drawImage);
        break;
    }
}

void showColorRatioBinaryzation(long sn, cv::Mat &rgb_mat)
{
    int c_count = 0;
    static cv::Mat vdrawImage = Mat::zeros(Size(HISTOGRAM_IMAGE_WIDTH * 3, HISTOGRAM_IMAGE_HEIGHT), CV_8UC3);

    cvtColor(rgb_mat, rgb_mat, COLOR_BGR2GRAY);
    threshold(rgb_mat, rgb_mat, 100, 255, THRESH_BINARY);
    cv::imshow("Video_Bin", rgb_mat);

    Mat_<uchar>::iterator it = rgb_mat.begin<uchar>();
    Mat_<uchar>::iterator itend = rgb_mat.end<uchar>();
    for (; it != itend; ++it)
    {
        if ((*it) > 0)
            c_count++;
    }

    int value = cvRound(HISTOGRAM_IMAGE_HEIGHT * (c_count * 1.0 / (rgb_mat.rows * rgb_mat.cols)));
    rectangle(vdrawImage, Point(sn * LINE_WIDTH, vdrawImage.rows - 2 - value), Point(sn * LINE_WIDTH, vdrawImage.rows - 1 - value), Scalar(255, 0, 0), LINE_WIDTH);
    imshow("Binaryzation", vdrawImage);
}

void showColorRatioByYUVBlack(long sn, AVFrame *frame)
{
    int i,j,c_count = 0;;

    for (i = 0; i < frame->height; i++)
    {
        for (j = 0; j < frame->width; j++)
        {
            uint8_t y = *(frame->data[0] + i * frame->linesize[0] + j);
            uint8_t u = *(frame->data[1] + i / 2 * frame->linesize[1] + j / 2);
            uint8_t v = *(frame->data[2] + i / 2 * frame->linesize[2] + j / 2);

            if (y < 24 && u >= 124 && u <= 132 && v >= 124 && v <= 132)
            {
                c_count++;
            }
        }
    }

    static cv::Mat vdrawImage = Mat::zeros(Size(HISTOGRAM_IMAGE_WIDTH * 3, HISTOGRAM_IMAGE_HEIGHT), CV_8UC3);
    int value = cvRound(HISTOGRAM_IMAGE_HEIGHT * (c_count * 1.0 / (frame->height * frame->width)));
    rectangle(vdrawImage, Point(sn * LINE_WIDTH, vdrawImage.rows - 2 - value), Point(sn * LINE_WIDTH, vdrawImage.rows - 1 - value), Scalar(0, 0, 255), LINE_WIDTH);
    imshow("YUV Black", vdrawImage);
}

void showColorRatioByValue(long sn, unsigned char r, unsigned char g, unsigned b, cv::Mat &rgb_mat)
{
    int c_count = 0;
    int i, j;
    int cPointR, cPointG, cPointB, cPoint;
    static cv::Mat vdrawImage = Mat::zeros(Size(HISTOGRAM_IMAGE_WIDTH * 10, HISTOGRAM_IMAGE_HEIGHT), CV_8UC3);

    for (i = 0; i < rgb_mat.rows; i++)
    {
        unsigned char *data = rgb_mat.data + i * rgb_mat.cols * rgb_mat.channels();

        for (j = 0; j < rgb_mat.cols; j++)
        {
            cPointB = *(data + j * rgb_mat.channels());
            cPointG = *(data + j * rgb_mat.channels() + 1);
            cPointR = *(data + j * rgb_mat.channels() + 2);

            if (cPointB == b && cPointG == g && cPointR == r)
            {
                c_count++;
            }
        }
    }

    int value = cvRound(HISTOGRAM_IMAGE_HEIGHT * (c_count * 1.0 / (rgb_mat.rows * rgb_mat.cols)));
    rectangle(vdrawImage, Point(sn * LINE_WIDTH, vdrawImage.rows - 1), Point(sn * LINE_WIDTH, vdrawImage.rows - 1 - value), Scalar(255, 0, 0), LINE_WIDTH);
    imshow("RGB", vdrawImage);
}

cv::Mat avframe_to_cvmat(AVFrame *frame)
{
    AVFrame dst;
    cv::Mat m;

    memset(&dst, 0, sizeof(dst));

    int w = frame->width, h = frame->height;
    m = cv::Mat(h, w, CV_8UC3);
    dst.data[0] = (uint8_t *)m.data;
    avpicture_fill((AVPicture *)&dst, dst.data[0], AV_PIX_FMT_BGR24, w, h);

    struct SwsContext *convert_ctx = NULL;
    enum AVPixelFormat src_pixfmt = (enum AVPixelFormat)frame->format;
    enum AVPixelFormat dst_pixfmt = AV_PIX_FMT_BGR24;
    convert_ctx = sws_getContext(w, h, src_pixfmt, w, h, dst_pixfmt,
                                 SWS_FAST_BILINEAR, NULL, NULL, NULL);
    sws_scale(convert_ctx, frame->data, frame->linesize, 0, h,
              dst.data, dst.linesize);
    sws_freeContext(convert_ctx);

    return m;
}

#endif

#ifdef USE_SDL
int WinMain()
#else
int main()
#endif
{
    AVFormatContext *format_ctx;
    AVCodecContext *codec_ctx;
    AVCodec *codec;
    AVFrame *frame, *frame_yuv;
    AVPacket *packet;
    unsigned char *out_buffer;
    struct SwsContext *img_convert_ctx;
    char file_path[] = "bigbuckbunny_480x272.h265";//bigbuckbunny_640x480.h265
    int ret, v_index = -1;
    int got_picture;
    long frame_count = 0;
    int screen_w = 0, screen_h = 0;
#ifdef USE_SDL
    SDL_Window *screen;
    SDL_Renderer *sdl_renderer;
    SDL_Texture *sdl_texture;
    SDL_Rect sdl_rect;
#endif

    av_register_all();
    avformat_network_init();

    format_ctx = avformat_alloc_context();

    if (avformat_open_input(&format_ctx, file_path, NULL, NULL) != 0)
    {
        printf("Couldn't open input stream.\n");
        return -1;
    }

    if (avformat_find_stream_info(format_ctx, NULL) < 0)
    {
        printf("Couldn't find stream information.\n");
        return -1;
    }

    for (int i = 0; i < format_ctx->nb_streams; i++)
    {
        if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            v_index = i;
            break;
        }
    }

    if (v_index == -1)
    {
        printf("Didn't find a video stream.\n");
        return -1;
    }

    codec_ctx = format_ctx->streams[v_index]->codec;
    codec = avcodec_find_decoder(codec_ctx->codec_id);
    if (codec == NULL)
    {
        printf("Codec not found.\n");
        return -1;
    }

    if (avcodec_open2(codec_ctx, codec, NULL) < 0)
    {
        printf("Could not open codec.\n");
        return -1;
    }

    frame = av_frame_alloc();
#ifdef USE_SDL
    frame_yuv = av_frame_alloc();

    screen_w = codec_ctx->width * 2;
    screen_h = codec_ctx->height * 2;

    out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, screen_w, screen_h, 1));
    av_image_fill_arrays(frame_yuv->data, frame_yuv->linesize, out_buffer, AV_PIX_FMT_YUV420P, screen_w, screen_h, 1);
#endif
    packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    printf("--------------- File Information ----------------\n");
    av_dump_format(format_ctx, 0, file_path, 0);
    printf("-------------------------------------------------\n");
#ifdef USE_SDL
    img_convert_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt, screen_w, screen_h,
                                     AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
    {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    screen = SDL_CreateWindow("SDL Window demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_w, screen_h, SDL_WINDOW_OPENGL);
    if (screen == 0)
    {
        printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
        return -1;
    }

    sdl_renderer = SDL_CreateRenderer(screen, -1, 0);
    sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, screen_w, screen_h);

    sdl_rect.x = 0;
    sdl_rect.y = 0;
    sdl_rect.w = screen_w;
    sdl_rect.h = screen_h;
#endif
    while (av_read_frame(format_ctx, packet) == 0)
    {
        if (packet->stream_index != v_index)
        {
            av_free_packet(packet);
            continue;
        }

        ret = avcodec_decode_video2(codec_ctx, frame, &got_picture, packet);
        if (ret < 0)
        {
            printf("Decode Error.\n");
            av_free_packet(packet);
            return -1;
        }

        if (got_picture == 0)
        {
            av_free_packet(packet);
            continue;
        }
#ifdef USE_SDL
        sws_scale(img_convert_ctx, frame->data, frame->linesize, 0, codec_ctx->height, frame_yuv->data, frame_yuv->linesize);
        SDL_UpdateYUVTexture(sdl_texture, &sdl_rect,
                             frame_yuv->data[0], frame_yuv->linesize[0],
                             frame_yuv->data[1], frame_yuv->linesize[1],
                             frame_yuv->data[2], frame_yuv->linesize[2]);

        SDL_RenderClear(sdl_renderer);
        SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, &sdl_rect);
        SDL_RenderPresent(sdl_renderer);
#endif
#ifdef USE_OPENCV
        showColorRatioByYUVBlack(frame_count, frame);

        cv::Mat rgbImg;
        rgbImg = avframe_to_cvmat(frame);
        cv::imshow("Video", rgbImg);

        showColorRatioByChannel(0, rgbImg);
        showColorRatioByChannel(1, rgbImg);
        showColorRatioByChannel(2, rgbImg);

        showColorRatioBinaryzation(frame_count, rgbImg);

        frame_count++;
#endif
        av_free_packet(packet);
#ifdef USE_SDL
        SDL_Delay(40);
#endif
#ifdef USE_OPENCV
        cv::waitKey(1000 / 25);
#endif
    }

    while (true)
    {
        ret = avcodec_decode_video2(codec_ctx, frame, &got_picture, packet);
        if (ret < 0 || got_picture == 0)
        {
            break;
        }
#ifdef USE_SDL
        sws_scale(img_convert_ctx, frame->data, frame->linesize, 0, codec_ctx->height, frame_yuv->data, frame_yuv->linesize);

        SDL_UpdateYUVTexture(sdl_texture, &sdl_rect,
                             frame_yuv->data[0], frame_yuv->linesize[0],
                             frame_yuv->data[1], frame_yuv->linesize[1],
                             frame_yuv->data[2], frame_yuv->linesize[2]);
        SDL_RenderClear(sdl_renderer);
        SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, &sdl_rect);
        SDL_RenderPresent(sdl_renderer);

        SDL_Delay(40);
#endif
#ifdef USE_OPENCV
        showColorRatioByYUVBlack(frame_count, frame);

        cv::Mat rgbImg;
        rgbImg = avframe_to_cvmat(frame);
        cv::imshow("Video", rgbImg);

        showColorRatioByChannel(0, rgbImg);
        showColorRatioByChannel(1, rgbImg);
        showColorRatioByChannel(2, rgbImg);

        showColorRatioBinaryzation(frame_count, rgbImg);

        frame_count++;
        
        cv::waitKey(1000 / 25);
#endif
    }

#ifdef USE_SDL
    sws_freeContext(img_convert_ctx);
    SDL_Quit();
    av_frame_free(&frame_yuv);
#endif
    av_frame_free(&frame);
    avcodec_close(codec_ctx);
    avformat_close_input(&format_ctx);

    return 0;
}