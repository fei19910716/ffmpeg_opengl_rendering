#pragma once

#define __STD_CONSTANT_MACROS
extern "C" {
#include "libavutil/opt.h"
#include "libavutil/pixfmt.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}
#include <string>

class VideoDecoder {
 public:
  VideoDecoder(std::string path);

  ~VideoDecoder();

  int getFrameWidth() { return width; }
  int getFrameHeight() { return height; }
  bool decodeNextFrame(bool flip = false);
  uint8_t* getFrameData() { return frameToGL->data[0]; }

 private:
  bool inited = false;
  std::string videoPath;
  uint8_t* buffer = nullptr;
  int width = 0;
  int height = 0;
  int index_video = -1;
  AVFormatContext* formatContext = nullptr;
  AVCodecContext* codecContext = nullptr;
  int frame_count = 0;
  int got_frame = 0;
  AVFrame* frame = nullptr;
  AVFrame* frameToGL = nullptr;
  AVPacket* avpkt = nullptr;
  SwsContext * img_convert_ctx = nullptr;
};