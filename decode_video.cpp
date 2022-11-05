#include <stdio.h>
#include <unistd.h>

#include <vector>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
};

#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
using namespace cv;
using namespace std;

#define COLOR_BLOCK_SIZE 256

#define HISTOGRAM_IMAGE_WIDTH 256
#define HISTOGRAM_IMAGE_HEIGHT 256

#define LINE_WIDTH (HISTOGRAM_IMAGE_WIDTH / COLOR_BLOCK_SIZE)

ofstream fout;

void showColorRatioByChannel(int channel, cv::Mat &rgb_mat) {
  int histsize[] = {COLOR_BLOCK_SIZE};
  float midranges[] = {0, 256 - 1};
  const float *ranges[] = {midranges};
  MatND dsthist;

  cv::Mat drawImage =
      Mat::zeros(Size(HISTOGRAM_IMAGE_WIDTH, HISTOGRAM_IMAGE_HEIGHT), CV_8UC3);

  calcHist(&rgb_mat, 1, &channel, Mat(), dsthist, 1, histsize, ranges, true,
           false);

  double g_dhistmaxvalue;
  minMaxLoc(dsthist, 0, &g_dhistmaxvalue, 0, 0);
  for (int i = 0; i < COLOR_BLOCK_SIZE; i++) {
    int value = cvRound(HISTOGRAM_IMAGE_HEIGHT *
                        (dsthist.at<float>(i) / g_dhistmaxvalue));
    switch (channel) {
      case 0:
        rectangle(drawImage, Point(i * LINE_WIDTH, drawImage.rows - 1),
                  Point(i * LINE_WIDTH, drawImage.rows - 1 - value),
                  Scalar(255, 0, 0), LINE_WIDTH);
        break;
      case 1:
        rectangle(drawImage, Point(i * LINE_WIDTH, drawImage.rows - 1),
                  Point(i * LINE_WIDTH, drawImage.rows - 1 - value),
                  Scalar(0, 255, 0), LINE_WIDTH);
        break;
      case 2:
        rectangle(drawImage, Point(i * LINE_WIDTH, drawImage.rows - 1),
                  Point(i * LINE_WIDTH, drawImage.rows - 1 - value),
                  Scalar(0, 0, 255), LINE_WIDTH);
        break;
      default:
        rectangle(drawImage, Point(i * LINE_WIDTH, drawImage.rows - 1),
                  Point(i * LINE_WIDTH, drawImage.rows - 1 - value),
                  Scalar(255, 255, 255), LINE_WIDTH);
        break;
    }
  }

  switch (channel) {
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

void showColorRatioBinaryzation(long sn, cv::Mat &rgb_mat, int thresh) {
  int c_count = 0;
  static cv::Mat vdrawImage = Mat::zeros(
      Size(HISTOGRAM_IMAGE_WIDTH * 3, HISTOGRAM_IMAGE_HEIGHT), CV_8UC3);

  cvtColor(rgb_mat, rgb_mat, COLOR_BGR2GRAY);
  threshold(rgb_mat, rgb_mat, thresh, 255, THRESH_BINARY);
  cv::imshow("Video_Bin", rgb_mat);

  Mat_<uchar>::iterator it = rgb_mat.begin<uchar>();
  Mat_<uchar>::iterator itend = rgb_mat.end<uchar>();
  for (; it != itend; ++it) {
    if ((*it) == 0) c_count++;
  }

  int value = cvRound(HISTOGRAM_IMAGE_HEIGHT *
                      (c_count * 1.0 / (rgb_mat.rows * rgb_mat.cols)));
  rectangle(vdrawImage, Point(sn * LINE_WIDTH, vdrawImage.rows - 2 - value),
            Point(sn * LINE_WIDTH, vdrawImage.rows - 1 - value),
            Scalar(255, 0, 0), LINE_WIDTH);
  imshow("Binaryzation", vdrawImage);
}

void showColorRatioByYUVBlack(long sn, AVFrame *frame) {
  int i, j, c_count = 0;

  for (i = 0; i < frame->height; i++) {
    for (j = 0; j < frame->width; j++) {
      uint8_t y = *(frame->data[0] + i * frame->linesize[0] + j);
      uint8_t u = *(frame->data[1] + i / 2 * frame->linesize[1] + j / 2);
      uint8_t v = *(frame->data[2] + i / 2 * frame->linesize[2] + j / 2);

      if (y < 24 && u >= 124 && u <= 132 && v >= 124 && v <= 132) {
        c_count++;
      }
    }
  }

  static cv::Mat vdrawImage = Mat::zeros(
      Size(HISTOGRAM_IMAGE_WIDTH * 3, HISTOGRAM_IMAGE_HEIGHT), CV_8UC3);
  int value = cvRound(HISTOGRAM_IMAGE_HEIGHT *
                      (c_count * 1.0 / (frame->height * frame->width)));
  rectangle(vdrawImage, Point(sn * LINE_WIDTH, vdrawImage.rows - 2 - value),
            Point(sn * LINE_WIDTH, vdrawImage.rows - 1 - value),
            Scalar(0, 0, 255), LINE_WIDTH);
  imshow("YUV Black", vdrawImage);
}

// Y420P
void showColorRatioByYBlack(long sn, AVFrame *frame, int thresh,
                            int devide = 4) {
  int i, j, c_count = 0;
  int counter[10][10];
  int area_w, area_h;

  if (devide > 10) devide = 10;
  area_w = frame->width / devide;
  area_h = frame->height / devide;
  memset(counter, 0, sizeof(counter));

  for (i = 0; i < area_h * devide; i++) {
    for (j = 0; j < area_w * devide; j++) {
      uint8_t y = *(frame->data[0] + i * frame->linesize[0] + j);

      if (y < thresh) {
        counter[j / area_w][i / area_h]++;
      }
    }
  }

  static cv::Mat vdrawImage = Mat::zeros(
      Size(HISTOGRAM_IMAGE_WIDTH * 3, HISTOGRAM_IMAGE_HEIGHT), CV_8UC3);
  fout << "{";
  if (frame->pict_type == AV_PICTURE_TYPE_I)
    fout << "'I'";
  else if (frame->pict_type == AV_PICTURE_TYPE_P)
    fout << "'P'";
  else if (frame->pict_type == AV_PICTURE_TYPE_B)
    fout << "'B'";
  else
    fout << "'!'";

  for (i = 0; i < devide; i++) {
    for (j = 0; j < devide; j++) {
      float ratio = counter[i][j] * 1.0 / (area_w * area_h);
      // fout << setw(10) << setiosflags(ios::left) << setprecision(2) << ratio;
      fout << " ," << setw(4) << (int)(ratio * 1000);

      int value = cvRound(HISTOGRAM_IMAGE_HEIGHT * ratio);
      Scalar c(255 * (i + 1) / devide, 0, 255 * (j + 1) / devide);
      rectangle(vdrawImage, Point(sn * LINE_WIDTH, vdrawImage.rows - 2 - value),
                Point(sn * LINE_WIDTH, vdrawImage.rows - 1 - value), c,
                LINE_WIDTH);
      imshow("Y Black", vdrawImage);
    }
  }

  fout << "}," << endl;
}

// void siftImage(long sn, cv::Mat &rgb_mat)
// {
//     std::vector<KeyPoint> keyPoints;
//     cv::SiftFeatureDetector feature;

//     feature.detect(rgb_mat, keyPoints);
//     drawKeypoints(rgb_mat, keyPoints, rgb_mat, Scalar::all(255),
//     DrawMatchesFlags::DRAW_OVER_OUTIMG); imshow("SIFT", rgb_mat);
// }

void showColorRatioByValue(long sn, unsigned char r, unsigned char g,
                           unsigned b, cv::Mat &rgb_mat) {
  int c_count = 0;
  int i, j;
  int cPointR, cPointG, cPointB, cPoint;
  static cv::Mat vdrawImage = Mat::zeros(
      Size(HISTOGRAM_IMAGE_WIDTH * 10, HISTOGRAM_IMAGE_HEIGHT), CV_8UC3);

  for (i = 0; i < rgb_mat.rows; i++) {
    unsigned char *data = rgb_mat.data + i * rgb_mat.cols * rgb_mat.channels();

    for (j = 0; j < rgb_mat.cols; j++) {
      cPointB = *(data + j * rgb_mat.channels());
      cPointG = *(data + j * rgb_mat.channels() + 1);
      cPointR = *(data + j * rgb_mat.channels() + 2);

      if (cPointB == b && cPointG == g && cPointR == r) {
        c_count++;
      }
    }
  }

  int value = cvRound(HISTOGRAM_IMAGE_HEIGHT *
                      (c_count * 1.0 / (rgb_mat.rows * rgb_mat.cols)));
  rectangle(vdrawImage, Point(sn * LINE_WIDTH, vdrawImage.rows - 1),
            Point(sn * LINE_WIDTH, vdrawImage.rows - 1 - value),
            Scalar(255, 0, 0), LINE_WIDTH);
  imshow("RGB", vdrawImage);
}

cv::Mat avframe_to_cvmat(AVFrame *frame) {
  AVFrame dst;
  cv::Mat m;

  memset(&dst, 0, sizeof(dst));

  int w = frame->width, h = frame->height;
  m = cv::Mat(h, w, CV_8UC3);
  dst.data[0] = (uint8_t *)m.data;
  av_image_fill_arrays(dst.data, dst.linesize, dst.data[0], AV_PIX_FMT_BGR24, w,
                       h, 1);

  struct SwsContext *convert_ctx = NULL;
  enum AVPixelFormat src_pixfmt = (enum AVPixelFormat)frame->format;
  enum AVPixelFormat dst_pixfmt = AV_PIX_FMT_BGR24;
  convert_ctx = sws_getContext(w, h, src_pixfmt, w, h, dst_pixfmt,
                               SWS_FAST_BILINEAR, NULL, NULL, NULL);
  sws_scale(convert_ctx, frame->data, frame->linesize, 0, h, dst.data,
            dst.linesize);
  sws_freeContext(convert_ctx);

  return m;
}

int main(int argc, char *argv[]) {
  AVFormatContext *format_ctx;
  AVCodecContext *codec_ctx;
  AVCodec *codec;
  AVFrame *frame, *frame_yuv;
  AVPacket *packet;
  unsigned char *out_buffer;
  struct SwsContext *img_convert_ctx;
  const char *file_path = "../bigbuckbunny_640x480.h265";
  int ret, v_index = -1;
  long frame_count = 0;
  int screen_w = 0, screen_h = 0;

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
    if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      v_index = i;
      break;
    }
  }

  if (v_index == -1) {
    printf("Didn't find a video stream.\n");
    return -1;
  }

  codec_ctx = avcodec_alloc_context3(NULL);
  if (codec_ctx == NULL) {
    printf("Could not allocate AVCodecContext \n");
    return -1;
  }
  avcodec_parameters_to_context(codec_ctx,
                                format_ctx->streams[v_index]->codecpar);

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
  if (!frame) {
    fprintf(stderr, "Could not allocate video frame\n");
    exit(1);
  }

  packet = (AVPacket *)av_malloc(sizeof(AVPacket));
  printf("--------------- File Information ----------------\n");
  av_dump_format(format_ctx, 0, file_path, 0);
  printf("-------------------------------------------------\n");

  while (av_read_frame(format_ctx, packet) == 0) {
    if (packet->stream_index != v_index) {
      av_packet_unref(packet);
      continue;
    }

    if ((ret = avcodec_send_packet(codec_ctx, packet)) < 0) {
      av_packet_unref(packet);
      printf("send frame error. \n");
      return -1;
    }

    if ((ret = avcodec_receive_frame(codec_ctx, frame)) != 0) {
      if (ret == AVERROR(EAGAIN)) {
        continue;
      } else if (ret == AVERROR_EOF) {
        break;
      }
      av_packet_unref(packet);
      printf("decode error. (ret=%d) \n", ret);
      return -1;
    }

    showColorRatioByYBlack(frame_count, frame, 50);

    cv::Mat rgbImg;
    rgbImg = avframe_to_cvmat(frame);
    cv::imshow("Video", rgbImg);

    // showColorRatioByChannel(0, rgbImg);
    // showColorRatioByChannel(1, rgbImg);
    // showColorRatioByChannel(2, rgbImg);

    // showColorRatioBinaryzation(frame_count, rgbImg, 50);
    // siftImage(frame_count, rgbImg);

    frame_count++;

    av_packet_unref(packet);

    cv::waitKey(1000 / 25);
  }

  while (true) {
    if ((ret = avcodec_send_packet(codec_ctx, packet)) < 0) {
      av_packet_unref(packet);
      printf("send frame error. \n");
      return -1;
    }

    if ((ret = avcodec_receive_frame(codec_ctx, frame)) != 0) {
      if (ret == AVERROR(EAGAIN)) {
        continue;
      } else if (ret == AVERROR_EOF) {
        break;
      }
      av_packet_unref(packet);
      printf("decode error. (ret=%d) \n", ret);
      return -1;
    }

    showColorRatioByYBlack(frame_count, frame, 150);

    cv::Mat rgbImg;
    rgbImg = avframe_to_cvmat(frame);
    cv::imshow("Video", rgbImg);

    // showColorRatioByChannel(0, rgbImg);
    // showColorRatioByChannel(1, rgbImg);
    // showColorRatioByChannel(2, rgbImg);

    // showColorRatioBinaryzation(frame_count, rgbImg, 100);
    // siftImage(frame_count, rgbImg);

    frame_count++;

    cv::waitKey(1000 / 25);
  }

  av_frame_free(&frame);
  avcodec_close(codec_ctx);
  avformat_close_input(&format_ctx);

  return 0;
}