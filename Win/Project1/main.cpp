#pragma once

#include <windows.h>
#include <tchar.h>
#include <iostream>

#include "FFVideoReader.h"
#include "Thread.h"


HBITMAP         g_hBmp = 0;
HDC             g_hMem = 0;
BYTE* g_imageBuf = 0;
FFVideoReader   g_reader;

class   DecodeThread :public Thread
{
public:
    FFVideoReader   _ffReader;
    HWND            _hWnd;
    bool            _exitFlag;
public:
    DecodeThread()
    {
        _ffReader.setup();
        _hWnd = 0;
        _exitFlag = false;
    }

    void    exitThread()
    {
        _exitFlag = true;
        join();
    }
    /**
    *   load the video
    */
    virtual void    load(const char* fileName)
    {
        _ffReader.load(fileName);
    }
    /**
    *   actual task of decode thread
    */
    virtual bool    run()
    {
        while (!_exitFlag)
        {
            // 非常重要，这里不能用智能指针，否则会crash
            //std::shared_ptr<FrameInfor> infor = std::make_shared<FrameInfor>();
            FrameInfor infor;
            if (!_ffReader.readFrame(infor))
            {
                break;
            }
            // 先进行耗时操作：数据的拷贝，尽可能避免主线程读到脏数据
            BYTE* data = (BYTE*)infor._data;
            for (int i = 0; i < infor._dataSize; i += 3)
            {
                g_imageBuf[i + 0] = data[i + 2];
                g_imageBuf[i + 1] = data[i + 1];
                g_imageBuf[i + 2] = data[i + 0];
            }
            InvalidateRect(_hWnd, 0, 0);

            // 方案三：new g_imageBuf存储数据，则不涉及多线程操作同一块内存
        }

        return  true;
    }
};
DecodeThread    g_decode;

LRESULT CALLBACK    windowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:// draw
    {
        PAINTSTRUCT ps;
        HDC         hdc;
        hdc = BeginPaint(hWnd, &ps);
        if (g_hMem)
        {
            BITMAPINFO  bmpInfor;
            GetObject(g_hBmp, sizeof(bmpInfor), &bmpInfor);
            BitBlt(hdc, 0, 0, bmpInfor.bmiHeader.biWidth, bmpInfor.bmiHeader.biHeight, g_hMem, 0, 0, SRCCOPY);
        }
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_SIZE:
        break;
    case WM_CLOSE:
    case WM_DESTROY:
        g_decode.exitThread();
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
        L"FFVideoPlayer",
        L"FFVideoPlayer",
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
    g_decode._hWnd = hWnd; // set the window to send message

    UpdateWindow(hWnd);
    ShowWindow(hWnd, SW_SHOW);

    HDC     hDC = GetDC(hWnd); 
    g_hMem = ::CreateCompatibleDC(hDC); // 创建画板

    BITMAPINFO	bmpInfor = { 0 }; 
    bmpInfor.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfor.bmiHeader.biWidth = g_decode._ffReader._screenW;
    bmpInfor.bmiHeader.biHeight = -g_decode._ffReader._screenH;
    bmpInfor.bmiHeader.biPlanes = 1;
    bmpInfor.bmiHeader.biBitCount = 24;
    bmpInfor.bmiHeader.biCompression = BI_RGB;
    bmpInfor.bmiHeader.biSizeImage = 0;
    bmpInfor.bmiHeader.biXPelsPerMeter = 0;
    bmpInfor.bmiHeader.biYPelsPerMeter = 0;
    bmpInfor.bmiHeader.biClrUsed = 0;
    bmpInfor.bmiHeader.biClrImportant = 0;

    g_hBmp = CreateDIBSection(hDC, &bmpInfor, DIB_RGB_COLORS, (void**)&g_imageBuf, 0, 0); // 创建画布，并指定数据源
    SelectObject(g_hMem, g_hBmp);

    g_decode.start(); // start the thread

    MSG     msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return  0;
}