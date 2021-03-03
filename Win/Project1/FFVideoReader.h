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

struct  FrameInfor
{
    void* _data;
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
    AVCodec* _codec;
    AVFrame* _frame;
    AVFrame* _frameRGB;
    SwsContext* _convertCtx;
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
        _frameRGB = 0;
        _convertCtx = 0;
        _screenW = 0;
        _screenH = 0;

    }

    ~FFVideoReader()
    {
        sws_freeContext(_convertCtx);
        av_free(_frameRGB);
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
        _frameRGB = av_frame_alloc();

        _screenW = _codecCtx->width;
        _screenH = _codecCtx->height;

        // 转换context，比如窗口大小改变时，数据帧需要进行转换
        _convertCtx = sws_getContext(
            _codecCtx->width
            , _codecCtx->height
            , _codecCtx->pix_fmt
            , _codecCtx->width // 转换后的width
            , _codecCtx->height
            , AV_PIX_FMT_RGB24 // 转换后的format
            , SWS_BICUBIC
            , NULL
            , NULL
            , NULL
        );
        // 计算当前帧的内存大小
        int     numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, _codecCtx->width, _codecCtx->height);
        uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
        avpicture_fill((AVPicture*)_frameRGB, buffer, AV_PIX_FMT_RGB24, _codecCtx->width, _codecCtx->height);
        _imageSize = numBytes;
        return  0;
    }
    // 读取视频帧
    void* readFrame()
    {
        // 加密的数据包
        AVPacket packet;
        av_init_packet(&packet);
        for (;;)
        {
            // 读取数据包
            if (av_read_frame(_formatCtx, &packet))
            {
                av_free_packet(&packet);
                return 0;
            }
            if (packet.stream_index != _videoIndex)
            {
                continue;
            }
            int frame_finished = 0;
            // 进行数据包解密，存储在_frame中
            int res = avcodec_decode_video2(_codecCtx, _frame, &frame_finished, &packet);

            // 图像帧可能由多帧数据帧构成，当图像帧完整时进行处理
            if (frame_finished)
            {

                char    buf[128];
                sprintf(buf, "pts = %I64d     dts =  %I64d\n", packet.pts, packet.dts);
                // 进行format的转换
                int res = sws_scale(
                    _convertCtx
                    , (const uint8_t* const*)_frame->data
                    , _frame->linesize
                    , 0
                    , _codecCtx->height
                    , _frameRGB->data
                    , _frameRGB->linesize
                );
                av_packet_unref(&packet);

                return  _frameRGB->data[0]; // 返回解密后的图像帧数据
            }
        }
        return  0;
    }

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



                char        buf[128];
                sprintf(buf, "pts = %I64d     dts =  %I64d\n", packet.pts, packet.dts);
                int res = sws_scale(
                    _convertCtx
                    , (const uint8_t* const*)_frame->data
                    , _frame->linesize
                    , 0
                    , _codecCtx->height
                    , _frameRGB->data
                    , _frameRGB->linesize
                );
                av_packet_unref(&packet);

                infor._data = _frameRGB->data[0];
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
