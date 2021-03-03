#pragma once

#include <windows.h>
#include <tchar.h>

#include "FFVideoReader.h"
#include "Thread.h"

DecodeThread    g_decode;

HBITMAP         g_hBmp = 0;
HDC             g_hMem = 0;
BYTE* g_imageBuf = 0;
FFVideoReader   g_reader;

LRESULT CALLBACK    windowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC         hdc;
        hdc = BeginPaint(hWnd, &ps);
        BitBlt(hdc, 0, 0, g_reader._screenW, g_reader._screenH, g_hMem, 0, 0, SRCCOPY);
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_SIZE:
        break;
    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_TIMER:
    {
        BYTE* data = (BYTE*)g_reader.readFrame();
        if (!data) break;
        for (int i = 0; i < g_reader._screenW * g_reader._screenH * 3; i += 3)
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
    // ffmpeg 
    char    szPath[1024];
    char    szPathName[1024];

    getResourcePath(hInstance, szPath);

    sprintf(szPathName, "1.mp4", szPath);
    g_reader.setup();
    g_reader.load(szPathName);

    //  1   ע�ᴰ����
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

    //  2 ��������
    HWND    hWnd = CreateWindowEx(
        NULL,
        L"FFVideoPlayer",
        L"FFVideoPlayer",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0,
        0,
        g_reader._screenW,
        g_reader._screenH,
        0,
        0,
        hInstance,
        0
    );

    UpdateWindow(hWnd);
    ShowWindow(hWnd, SW_SHOW);

    HDC     hDC = GetDC(hWnd);
    g_hMem = ::CreateCompatibleDC(hDC);

    BITMAPINFO	bmpInfor = { 0 };
    bmpInfor.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfor.bmiHeader.biWidth = g_reader._screenW;
    bmpInfor.bmiHeader.biHeight = -g_reader._screenH;
    bmpInfor.bmiHeader.biPlanes = 1;
    bmpInfor.bmiHeader.biBitCount = 24;
    bmpInfor.bmiHeader.biCompression = BI_RGB;
    bmpInfor.bmiHeader.biSizeImage = 0;
    bmpInfor.bmiHeader.biXPelsPerMeter = 0;
    bmpInfor.bmiHeader.biYPelsPerMeter = 0;
    bmpInfor.bmiHeader.biClrUsed = 0;
    bmpInfor.bmiHeader.biClrImportant = 0;

    g_hBmp = CreateDIBSection(hDC, &bmpInfor, DIB_RGB_COLORS, (void**)&g_imageBuf, 0, 0);
    SelectObject(g_hMem, g_hBmp);

    SetTimer(hWnd, 1, 40, 0);

    MSG     msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return  0;
}