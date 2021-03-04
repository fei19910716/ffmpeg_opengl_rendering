#pragma once

#include <windows.h>
#include <tchar.h>
#include <iostream>

#include "FFVideoReader.h"
#include "Thread.h"
#include "Timestamp.h"
#include "GLContext.h"


HBITMAP         g_hBmp = 0;
HDC             g_hMem = 0;
BYTE* g_imageBuf = 0;
FFVideoReader   g_reader;

class   DecodeThread :public Thread
{
public:
    FFVideoReader   _ffReader;
    HWND            _hWnd;
    BYTE* _imageBuf;
    HDC             _hMem;
    HBITMAP	        _hBmp;
    bool            _exitFlag;
    Timestamp       _timestamp;
public:
    DecodeThread()
    {
        _exitFlag = false;
        _hWnd = 0;
    }

    virtual void    setup(HWND hwnd, const char* fileName)
    {
        _hWnd = hwnd;

        HDC     hDC = GetDC(hwnd);
        _hMem = ::CreateCompatibleDC(hDC);


        BITMAPINFO	bmpInfor;
        bmpInfor.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmpInfor.bmiHeader.biWidth = _ffReader._screenW;
        bmpInfor.bmiHeader.biHeight = -_ffReader._screenH;
        bmpInfor.bmiHeader.biPlanes = 1;
        bmpInfor.bmiHeader.biBitCount = 24;
        bmpInfor.bmiHeader.biCompression = BI_RGB;
        bmpInfor.bmiHeader.biSizeImage = 0;
        bmpInfor.bmiHeader.biXPelsPerMeter = 0;
        bmpInfor.bmiHeader.biYPelsPerMeter = 0;
        bmpInfor.bmiHeader.biClrUsed = 0;
        bmpInfor.bmiHeader.biClrImportant = 0;

        _hBmp = CreateDIBSection(hDC, &bmpInfor, DIB_RGB_COLORS, (void**)&_imageBuf, 0, 0);
        SelectObject(_hMem, _hBmp);
    }
    /**
    *   �����ļ�
    */
    virtual void    load(const char* fileName)
    {
        _ffReader.setup();
        _ffReader.load(fileName);
    }
    virtual void    join()
    {
        _exitFlag = true;
        Thread::join();
    }
    /**
    *   �߳�ִ�к���
    */
    virtual bool    run()
    {
        _timestamp.update();
        while (!_exitFlag)
        {
            FrameInfor  infor;
            if (!_ffReader.readFrame(infor))
            {
                break;
            }
            double      tims = infor._pts * infor._timeBase * 1000; //获取数据帧中的播放时间点
            BYTE* data = (BYTE*)infor._data;
            for (int i = 0; i < infor._dataSize; i += 3)
            {
                _imageBuf[i + 0] = data[i + 2];
                _imageBuf[i + 1] = data[i + 1];
                _imageBuf[i + 2] = data[i + 0];
            }
            InvalidateRect(_hWnd, 0, 0);
            double      elsped = _timestamp.getElapsedTimeInMilliSec(); // 流逝的时间
            double      sleeps = (tims - elsped); // 如果流逝的时间还没到播放时间点，则应该等待
            if (sleeps > 1)
            {
                Sleep((DWORD)sleeps);
            }
        }

        return  true;
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
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(1, 0, 0, 1);

        g_glContext.swapBuffer();
    }
    break;
    case WM_SIZE:
        break;
    case WM_CLOSE:
    case WM_DESTROY:
        g_decode.join();
        PostQuitMessage(0);
        break;
    case WM_UPDATE_VIDEO: // message from the decode thread
    {
        // 非常重要，这里不能用智能指针，否则会crash
        //std::shared_ptr<FrameInfor> infor((FrameInfor*)wParam);
        FrameInfor* infor = (FrameInfor*)wParam;
        memcpy(g_imageBuf, infor->_data, infor->_dataSize); // get the frame data

        BYTE* data = (BYTE*)infor->_data;
        for (int i = 0; i < infor->_dataSize; i += 3)
        {
            g_imageBuf[i + 0] = data[i + 2];
            g_imageBuf[i + 1] = data[i + 1];
            g_imageBuf[i + 2] = data[i + 0];
        }
        InvalidateRect(hWnd, 0, 0);
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
    g_glContext.shutdown();
    return  0;
}