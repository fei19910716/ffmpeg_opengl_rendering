﻿#include <stdio.h>
//#include <libavformat/avformat.h>

#define __STDC_CONSTANT_MACROS
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>     //裁剪
#include "libavutil/imgutils.h"
}

//输入和输出的文件名，解码前的数据放H264文件中，解码后放YUV文件中
#define FILENAME            "C://Users//fordchen//Desktop//demo.mp4"
#define FILENAME_YUV        "cuc_ieschool_512x288.yuv"
#define FILENAME_H264       "cuc_ieschool_512x288.H264"
#define FILENAME_INFO       "cuc_ieschool.info"

void print_AVFormatContext_info(AVFormatContext* pFormatCtx, FILE* file_stream);

int main(int argc, char* argv[])
{

    AVFormatContext* pFormatCtx;        //统筹结构,保存封装格式相关信息  
    AVCodec* pCodec;            //每种编解码器对应一个结构体
    AVFrame* pFrame;            //解码后的结构
    AVFrame* pFrameYUV;
    AVPacket* pPacket;           //解码前
    struct SwsContext* img_convert_ctx;   //头文件只有一行，但是实际上这个结构体十分复杂
    int                i = 0;
    int                video_index = 0;     //为了检查哪个流是视频流，保存流数组下标
    int                y_size = 0;         //
    int                ret = 0;            //
    int                got_picture = 0;    //
    char               filename[256] = FILENAME;  //
    int                frame_cnt;          //帧数计算
    uint8_t* out_buffer;        //???
    FILE* fp = NULL;
    FILE* YUVfp = NULL;

    if (argc == 2)
    {
        memcpy(filename, argv[1], strlen(argv[1]) + 1);
    }
    else if (argc > 2)
    {
        printf("Usage: ./*.out video_filename");
    }

    //av_register_all();         //注册所有组件

    //avformat_network_init();   //初始化网络，貌似暂时没用到，可以试试删除

    pFormatCtx = avformat_alloc_context(); //分配内存

    //打开文件, 注意第一个参数是指针的指针
    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0)
    {
        printf("Couldn't open input stream.\n");
        return (-1);
    }

    //获取流信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        printf("Couldn't find stream info\n");
        return (-1);
    }

    //找出视频流, nb_streams表示视频中有几种流
    video_index = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_index = i;
            break;
        }
    }
    if (-1 == video_index)
    {
        printf("Couldn't find a video stream\n");
        return (-1);
    }

    //复制编解码的信息到编解码的结构AVCodecContext结构中，
    //一方面为了操作结构中数据方便（不需要每次都从AVFormatContext结构开始一个一个指向）
    //另一方面方便函数的调用
    // pCodecCtx = pFormatCtx->streams[video_index]->codecpar;    //可以查看源码avformat.h中定义的结构
    AVCodecContext* pCodecCtx = avcodec_alloc_context3(NULL); //保存视/音频编解码相关信息
    if (pCodecCtx == NULL)
    {
        printf("Could not allocate AVCodecContext\n");
        return -1;
    }
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[video_index]->codecpar);

    //找到一个和视频流对应的解码器
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (NULL == pCodec)
    {
        printf("Couldn't found a decoder\n");
        return (-1);
    }

    //打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        printf("Couldn't open decoder\n");
        return (-1);
    }

    //输出一些想要输出的信息
    fp = stdout;
    fp = fopen(FILENAME_INFO, "w+");
    if (fp == NULL)
    {
        fprintf(stderr, "fopen error\n");
        return (-1);
    }

    //输出结构中的信息以更了解这些结构。这里代码只示例一个。
    print_AVFormatContext_info(pFormatCtx, fp);

    //输出视频文件信息
    printf("------------------------file infomation-----------------------------\n");
    av_dump_format(pFormatCtx, 0, filename, 0);
    printf("--------------------------------------------------------------------\n");

    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();
    out_buffer = (uint8_t*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height,1));
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
        AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height,1);

    //???
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,  pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
        AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    pPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
    frame_cnt = 0;         //帧数计算吗？

    //打开文件
    fp = fopen(FILENAME_H264, "w+");
    if (fp == NULL)
    {
        fprintf(stderr, "h264 fopen error\n");
        return (-1);
    }

    YUVfp = fopen(FILENAME_YUV, "w+");
    if (NULL == YUVfp)
    {
        fprintf(stderr, "YUV fopen error\n");
        return (-1);
    }

    while (av_read_frame(pFormatCtx, pPacket) >= 0)
    {
        if (pPacket->stream_index == video_index)
        {
            //这里可以输出H264马流信息，这里是解码前
            fwrite(pPacket->data, pPacket->size, 1, fp);

            //解码
            ret = avcodec_send_packet(pCodecCtx, pPacket);
            if (ret < 0)
            {
                printf("Decode fail\n");
                return (-1);
            }

            if (got_picture == 0)
            {
                printf("get picture error\n");
            }
            else
            {
                //调整解码出来的图像，解码出来的可能含有填充的信息。
                sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data,
                    pFrame->linesize, 0, pCodecCtx->height,
                    pFrameYUV->data, pFrameYUV->linesize);
                //                  printf("Decoded frame index: %d\n", frame_cnt);

                                  // 已经解码，可以输出YUV的信息
                                  // 这个data是一个数组，需要注意！一般3个：代表Y、U、V
                                  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                fwrite(pFrameYUV->data[0], 1, pCodecCtx->width * pCodecCtx->height, YUVfp);
                fwrite(pFrameYUV->data[1], 1, pCodecCtx->width * pCodecCtx->height / 4, YUVfp);
                fwrite(pFrameYUV->data[2], 1, pCodecCtx->width * pCodecCtx->height / 4, YUVfp);
                frame_cnt++;
            }
        }
        av_packet_unref(pPacket);
    }

    printf("Decoded frame count: %d\n", frame_cnt);

    fclose(fp);
    fclose(YUVfp);
    sws_freeContext(img_convert_ctx);
    av_frame_free(&pFrameYUV);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    return 0;
}

void print_AVFormatContext_info(AVFormatContext* pFormatCtx, FILE* file_stream)
{
    fprintf(file_stream, "-------------------------AVFormatContext--------------------------\n");
    fprintf(file_stream, "ctx_flags = %d\n"
        "nb_streams = %u\n"
        //"file_name = %d\n"
        "start_time = %d\n"
        "duration = %d\n"
        "bit_rate = %d\n"
        "packet_size = %u\n"
        "max_delay = %d\n"
        "flags = %d\n"
        "probesize = %d\n"
        "key = %d\n"
        "keylen = %d\n"
        "nb_programs = %d\n",
        pFormatCtx->ctx_flags, pFormatCtx->nb_streams,
        /*pFormatCtx->filename,*/ pFormatCtx->start_time,
        pFormatCtx->duration, pFormatCtx->bit_rate,
        pFormatCtx->packet_size, pFormatCtx->max_delay,
        pFormatCtx->flags, pFormatCtx->probesize,
        pFormatCtx->key, pFormatCtx->keylen, pFormatCtx->nb_programs);
    fprintf(file_stream, "------------------------------------------------------------------\n");
}