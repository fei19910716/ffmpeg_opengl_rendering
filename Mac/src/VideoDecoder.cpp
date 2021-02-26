#include "VideoDecoder.h"
#include <QDebug>

VideoDecoder::VideoDecoder(std::string path) : videoPath(std::move(path)) {
  avcodec_register_all();

  formatContext = avformat_alloc_context();
  if (avformat_open_input(&formatContext, videoPath.c_str(), NULL, NULL) < 0) {
    qDebug() << "Can't open the input stream.";
    return;
  }

  if (avformat_find_stream_info(formatContext, NULL) < 0) {
      qDebug() << "Can't find the stream information!";
    return;
  }

  av_dump_format(formatContext, 0, videoPath.c_str(), 0);

  int i = -1;
  for (i = 0; i < formatContext->nb_streams; i++) {
    if (formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
      index_video = i;
      break;
    }
  }
  if (index_video == -1) {
      qDebug() << "Can't find a video stream";
    return;
  }

  codecContext = formatContext->streams[index_video]->codec;
  if (!codecContext) {
      qDebug() << "Could not allocate video codec context";
    inited = false;
    return;
  }
  AVCodec *codec = avcodec_find_decoder(codecContext->codec_id);
  if (codec == NULL) {
      qDebug() << "Can't find a decoder!";
    return;
  }

  /* open it */
  if (avcodec_open2(codecContext, codec, NULL) < 0) {
      qDebug() << "Could not open codec";
    return;
  }

  frame = av_frame_alloc();
  frameToGL = av_frame_alloc();
  int size = avpicture_get_size(AV_PIX_FMT_RGB24, codecContext->width, codecContext->height);
  buffer = (uint8_t *)av_malloc(size * sizeof(uint8_t));
  avpicture_fill((AVPicture *)frameToGL, buffer, AV_PIX_FMT_RGB24, codecContext->width,
                 codecContext->height);

  avpkt = (AVPacket *)av_malloc(sizeof(AVPacket));
  av_init_packet(avpkt);

  width = codecContext->width;
  height = codecContext->height;

  img_convert_ctx = sws_getContext(codecContext->width, codecContext->height, codecContext->pix_fmt,
                                   codecContext->width, codecContext->height, AV_PIX_FMT_RGB24,
                                   SWS_BICUBIC, NULL, NULL, NULL);

  frame_count = 0;
  inited = true;
}

VideoDecoder::~VideoDecoder() {
  av_free_packet(avpkt);
  sws_freeContext(img_convert_ctx);
  av_frame_free(&frameToGL);
  av_frame_free(&frame);
  avcodec_close(codecContext);
  avformat_close_input(&formatContext);
  avformat_free_context(formatContext);
  inited = false;
}

static inline void flipFrame(AVFrame *pFrame) {
  pFrame->data[0] += pFrame->linesize[0] * (pFrame->height - 1);
  pFrame->linesize[0] *= -1;
  pFrame->data[1] += pFrame->linesize[1] * (pFrame->height / 2 - 1);
  pFrame->linesize[1] *= -1;
  pFrame->data[2] += pFrame->linesize[2] * (pFrame->height / 2 - 1);
  pFrame->linesize[2] *= -1;
}

bool VideoDecoder::decodeNextFrame(bool flip) {
  if (av_read_frame(formatContext, avpkt) >= 0) {
    if (avpkt->stream_index == index_video) {
      if (avcodec_decode_video2(codecContext, frame, &got_frame, avpkt) < 0) {
          qDebug() << "Decode Error!";
        return false;
      }
      if (got_frame) {
//        LOG(ERROR) << "Decoded frame index: " << frame_count;
        // 需要翻转一下
        if (flip) {
          flipFrame(frame);
        }
        sws_scale(img_convert_ctx, frame->data, frame->linesize, 0, codecContext->height,
                  frameToGL->data, frameToGL->linesize);
        frame_count++;
        return true;
      }
    }
    av_free_packet(avpkt);
  }
  // 播放结束之后，seek到开头
  av_seek_frame(formatContext, -1, (double)formatContext->start_time, AVSEEK_FLAG_BACKWARD);
  return frame_count == 0; // 第0帧av_read_frame会失败，忽略第0帧
}
