#include <stdio.h>

#define __STDC_CONSTANT_MACROS

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include <SDL2/SDL.h>
};

int WinMain()
{
    av_register_all();
    avformat_network_init();
    return 0;
}