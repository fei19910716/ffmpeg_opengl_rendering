#pragma once

#include <windows.h>
#include <tchar.h>
#include <iostream>

#include <mutex>

#include "glew/glew.h"
#include "FFVideoReader.h"
#include "Thread.h"
#include "Timestamp.h"
#include "GLContext.h"
#include "CELLShader.h"
#include "CELLMath.h"



HBITMAP         g_hBmp = 0;
HDC             g_hMem = 0;
BYTE*           g_imageBuf = 0;
FFVideoReader   g_reader;
std::mutex      g_lock;

class   DecodeThread :public Thread
{
public:
    FFVideoReader   _ffReader;
    HWND            _hWnd;

    bool            _exitFlag;
    Timestamp       _timestamp;
    GLContext       _glContext;
    unsigned        _textureYUV;
    PROGRAM_YUV_SingleTexture     _shaderTex;
    unsigned        _pbo[2];
    int             _DMA;
    int             _WRITE;
    void*           _dmaPtr;
public:
    DecodeThread()
    {
        _exitFlag = false;
        _hWnd = 0;
        _DMA = 0;
        _WRITE = 1;
        _dmaPtr = 0;
    }

    virtual void    setup(HWND hwnd, const char* fileName = "11.flv")
    {
        _hWnd = hwnd;
        
        _glContext.setup(hwnd, GetDC(hwnd));

        glewInit();

        glEnable(GL_TEXTURE_2D);
        // 创建单通道纹理，由于UV长宽是原图的一半，因此拼接起来的纹理尺寸为w，h+h/2
        _textureYUV = createTexture(_ffReader._screenW, _ffReader._screenH + _ffReader._screenH / 2); 

        _pbo[0] = createPBuffer(_ffReader._screenW, _ffReader._screenH + _ffReader._screenH / 2);
        _pbo[1] = createPBuffer(_ffReader._screenW, _ffReader._screenH + _ffReader._screenH / 2);

        _shaderTex.initialize();

    }
    /**
    *   �����ļ�
    */
    virtual void    load(const char* fileName)
    {
        _ffReader.setup();
        _ffReader.load(fileName);
    }
    virtual void    shutdown()
    {
        _exitFlag = true;
        Thread::join();
        _glContext.shutdown();
    }
    /**
    *   �߳�ִ�к���
    */
    virtual bool    run()
    {
        _timestamp.update();
        while (!_exitFlag)
        {
            FrameInfor* infor = new FrameInfor();
            {
                std::lock_guard<std::mutex> locker(g_lock); 
                if (!_ffReader.readFrame(*infor))
                {
                    break;
                }
            }
            double      tims = infor->_pts * infor->_timeBase * 1000;
            //! ������Ҫ֪ͨ���ڽ����ػ��Ƹ��£���ʾ��������

            if (infor->_data->data[0] == 0
                || infor->_data->data[1] == 0
                || infor->_data->data[2] == 0)
            {
                continue;
            }

            PostMessage(_hWnd, WM_UPDATE_VIDEO, (WPARAM)infor, 0);


            double      elsped = _timestamp.getElapsedTimeInMilliSec();
            double      sleeps = (tims - elsped);
            if (sleeps > 1)
            {
                Sleep((DWORD)sleeps);
            }
        }

        return  true;
    }

    void    updateImage(GLubyte* dst, int x, int y, int w, int h, void* data)
    {
        int         pitch = _ffReader._screenW;
        GLubyte*    dst1 = dst + y * pitch + x;
        GLubyte*     src = (GLubyte*)(data);

        int         size = w;

        for (int i = 0; i < h; ++i)
        {
            memcpy(dst1, src, w);
            dst1 += pitch;
            src += w;
        }
    }

    void    updateTexture(FrameInfor* infor)
    {
        int     w = _ffReader._screenW;
        int     h = _ffReader._screenH + _ffReader._screenH / 2;

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbo[_WRITE]); // 内存交换时拷贝到write pbo
        GLubyte* dst = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);

        if (dst != 0)
        {
            memcpy(dst, infor->_data->data[0], _ffReader._screenW * _ffReader._screenH);

            updateImage(dst, 0, _ffReader._screenH, _ffReader._screenW / 2, _ffReader._screenH / 2, infor->_data->data[1]);
            updateImage(dst, _ffReader._screenW / 2, _ffReader._screenH, _ffReader._screenW / 2, _ffReader._screenH / 2, infor->_data->data[2]);
            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        }
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbo[_DMA]); // 显存交换时，拷贝dma pbo
        glBindTexture(GL_TEXTURE_2D, _textureYUV);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        std::swap(_DMA, _WRITE); // 不断交替进行

    }

    void    render()
    {
        // 顶点数据
        struct  Vertex
        {
            CELL::float2  pos; // 位置
            CELL::float2  uvY; // Y的采样坐标
            CELL::float2  uvU;
            CELL::float2  uvV;
        };

        RECT    rt;
        GetClientRect(_hWnd, &rt);
        int     w = rt.right - rt.left;
        int     h = rt.bottom - rt.top;
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(1, 0, 0, 1);

        glViewport(0, 0, w, h);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, w, h, 0, -100, 100);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();


        glBindTexture(GL_TEXTURE_2D, _textureYUV);

        float   yv = (float)_ffReader._screenH / (float)(_ffReader._screenH + _ffReader._screenH / 2);

        float   fw = _ffReader._screenW;
        float   fh = _ffReader._screenH;

        float   x = 0;
        float   y = 0;

        float   Yv = (float)fh / (float)(fh + fh / 2);

        float   Uu0 = 0;
        float   Uv0 = Yv;
        float   Uu1 = 0.5f;
        float   Uv1 = 1.0f;

        float   Vu0 = 0.5f; // V的数据左上角的顶点在整张图中的坐标为（0.5，Yv）
        float   Vv0 = Yv;
        float   Vu1 = 1.0f;
        float   Vv1 = 1.0f;
        // 基于新的尺寸为w,h+h/2的一张纹理计算采样坐标
        Vertex  vertex[] =
        {
            // 第一个点是纹理左上角
            CELL::float2(x,y),/*位置*/           CELL::float2(0,0),/*Y的采样uv*/      CELL::float2(Uu0,Uv0),     CELL::float2(Vu0,Vv0),
            CELL::float2(x + w,y),               CELL::float2(1,0),                   CELL::float2(Uu1,Uv0),     CELL::float2(Vu1,Vv0),
            CELL::float2(x,y + h),               CELL::float2(0,Yv),                  CELL::float2(Uu0,Uv1),     CELL::float2(Vu0,Vv1),
            CELL::float2(x + w, y + h),          CELL::float2(1,Yv),                  CELL::float2(Uu1,Uv1),     CELL::float2(Vu1,Vv1),
        };

        CELL::matrix4   matMVP = CELL::ortho<float>(0, w, h, 0, -100, 100);
        _shaderTex.begin();
        glUniformMatrix4fv(_shaderTex._MVP, 1, GL_FALSE, matMVP.data());

        glUniform1i(_shaderTex._textureYUV, 0);
        glVertexAttribPointer(_shaderTex._position, 2, GL_FLOAT, false, sizeof(Vertex), vertex);
        glVertexAttribPointer(_shaderTex._uvY,      2, GL_FLOAT, false, sizeof(Vertex), &vertex[0].uvY); // 
        glVertexAttribPointer(_shaderTex._uvU,      2, GL_FLOAT, false, sizeof(Vertex), &vertex[0].uvU);
        glVertexAttribPointer(_shaderTex._uvV,      2, GL_FLOAT, false, sizeof(Vertex), &vertex[0].uvV);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        _shaderTex.end();

        _glContext.swapBuffer();
    }

protected:

    unsigned    createTexture(int w, int h)
    {
        unsigned    texId;
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0); // GL_ALPHA单通道的三张纹理

        return  texId;
    }

    unsigned    createPBuffer(int w, int h)
    {
        unsigned    pbuffer = 0;
        glGenBuffers(1, &pbuffer);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbuffer);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, w * h, 0, GL_STREAM_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        return  pbuffer;
    }
};
DecodeThread    g_decode;
GLContext       g_glContext;

LRESULT CALLBACK    windowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:// draw
    {
        
    }
    break;
    case WM_SIZE:
        break;
    case WM_CLOSE:
    case WM_DESTROY:
        g_decode.shutdown();
        PostQuitMessage(0);
        break;
    case WM_UPDATE_VIDEO: // message from the decode thread
    {
        // 非常重要，这里不能用智能指针，否则会crash
        // std::shared_ptr<FrameInfor> infor((FrameInfor*)wParam);
        FrameInfor* infor = (FrameInfor*)wParam;
        {
            std::lock_guard<std::mutex> locker(g_lock);
            g_decode.updateTexture(infor);
        }
        delete  infor;
        g_decode.render();
    }
    break;
    default:
        break;
    }

    return  DefWindowProc(hWnd, msg, wParam, lParam);
}
// get the path of .exe
void  getResourcePath(HINSTANCE hInstance, char pPath[1024])
{
    char    szPathName[1024];
    char    szDriver[64];
    char    szPath[1024];
    GetModuleFileNameA(hInstance, szPathName, sizeof(szPathName));
    _splitpath(szPathName, szDriver, szPath, 0, 0);
    sprintf(pPath, "%s%s", szDriver, szPath);
}

int     WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    AllocConsole(); freopen("CONOUT$", "r+", stdout);
    // ffmpeg 
    char    szPath[1024];
    char    szPathName[1024];

    getResourcePath(hInstance, szPath);

    sprintf(szPathName, "1.mp4", szPath);
    g_decode.load(szPathName); // load the video

    //  register window class
    ::WNDCLASSEXA winClass;
    winClass.lpszClassName = "FFVideoPlayer";
    winClass.cbSize = sizeof(::WNDCLASSEX);
    winClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
    winClass.lpfnWndProc = windowProc;
    winClass.hInstance = hInstance;
    winClass.hIcon = 0;
    winClass.hIconSm = 0;
    winClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    winClass.hbrBackground = (HBRUSH)(BLACK_BRUSH);
    winClass.lpszMenuName = NULL;
    winClass.cbClsExtra = 0;
    winClass.cbWndExtra = 0;
    RegisterClassExA(&winClass);

    //  create window
    HWND    hWnd = CreateWindowEx(
        NULL,
        "FFVideoPlayer",
        "FFVideoPlayer",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0,
        0,
        g_decode._ffReader._screenW,
        g_decode._ffReader._screenH,
        0,
        0,
        hInstance,
        0
    );

    g_glContext.setup(hWnd, GetDC(hWnd));

    UpdateWindow(hWnd);
    ShowWindow(hWnd, SW_SHOW);

    g_decode.setup(hWnd, szPathName);
    g_decode.start();

    MSG     msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    g_decode.shutdown();
    return  0;
}