#pragma once


extern "C"
{
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
}
// 封装一帧数据
struct  FrameInfor
{
    AVFrame* _data;
    int     _dataSize;
    int     _width;
    int     _height;
    int64_t _pts;
    double  _timeBase;

};

class   FFVideoReader
{
public:
    AVFormatContext* _formatCtx;
    int             _videoIndex;
    AVCodecContext* _codecCtx;
    AVCodec*        _codec;
    AVFrame*        _frame;
public:
    int             _screenW;
    int             _screenH;

    int             _imageSize;
public:
    FFVideoReader()
    {
        _formatCtx = 0;
        _videoIndex = -1;
        _codecCtx = 0;
        _codec = 0;
        _frame = 0;
        _screenW = 0;
        _screenH = 0;

    }

    ~FFVideoReader()
    {
        av_free(_frame);
        avcodec_close(_codecCtx);
        avformat_close_input(&_formatCtx);
    }

    void    setup()
    {
        av_register_all();
        _formatCtx = avformat_alloc_context();
    }
    int     load(const char* filepath)
    {
        int     ret = 0;

        //! 打开视频文件
        if (avformat_open_input(&_formatCtx, filepath, NULL, NULL) != 0)
        {
            return -1;
        }
        //! 查找数据流（音频、视频等）
        if (avformat_find_stream_info(_formatCtx, NULL) < 0)
        {
            return -1;
        }
        //! 获取视频流的索引
        _videoIndex = -1;
        for (int i = 0; i < _formatCtx->nb_streams; i++)
        {
            if (_formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                _videoIndex = i;
                break;
            }
        }
        /**
        *   没有找到视频流，直接退出
        */
        if (_videoIndex == -1)
        {
            return -1;
        }
        // 获取视频流对应的解码器context
        _codecCtx = _formatCtx->streams[_videoIndex]->codec;
        // 获取解码器
        _codec = avcodec_find_decoder(_codecCtx->codec_id);
        if (_codec == NULL)
        {
            return -1;
        }
        /**
        *   打开解码器
        */
        if (avcodec_open2(_codecCtx, _codec, NULL) < 0)
        {
            return -1;
        }
        // 分配数据帧内存
        _frame = av_frame_alloc();

        _screenW = _codecCtx->width;
        _screenH = _codecCtx->height;

        return  0;
    }
    
    // 读取一帧信息到frameInfo中
    bool    readFrame(FrameInfor& infor)
    {
        AVPacket packet;
        av_init_packet(&packet);
        for (;;)
        {
            if (av_read_frame(_formatCtx, &packet))
            {
                av_free_packet(&packet);
                return false;
            }
            if (packet.stream_index != _videoIndex)
            {
                continue;
            }
            int frame_finished = 0;

            int res = avcodec_decode_video2(_codecCtx, _frame, &frame_finished, &packet);

            if (frame_finished)
            {
                AVStream* streams = _formatCtx->streams[_videoIndex];
                double      tmbase = av_q2d(streams->time_base);
                int64_t     pts = _frame->pts;


                // CPU进行YUV到RGB的转化，可以转到GPU中进行提升性能
                av_packet_unref(&packet);

                infor._data = _frame;
                infor._dataSize = _imageSize;
                infor._width = _screenW;
                infor._height = _screenH;
                infor._pts = _frame->pts;
                infor._timeBase = av_q2d(streams->time_base);

                return  true;
            }
        }
        return  false;
    }
};
