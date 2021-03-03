#pragma once

#include <windows.h>

#define WM_UPDATE_VIDEO WM_USER + 100

class   Thread
{
public:
    DWORD               _threadId;
    HANDLE              _thread;
protected:

    /**
    *   callback of thread
    */
    static  DWORD    CALLBACK threadEnter(void* pVoid)
    {
        Thread* pThis = (Thread*)pVoid;
        if (pThis)
        {
            pThis->run();
        }
        return  0;
    }
public:
    Thread()
    {
        _thread = 0;
        _threadId = 0;
    }
    virtual ~Thread()
    {
        join();
    }

    virtual bool    run()
    {
        return  true;
    }
    /**
    *   start the thread
    */
    virtual bool    start()
    {
        if (_thread != 0)
        {
            return  false;
        }
        else
        {
            //HIGH_PRIORITY_CLASS: create a thread, set the callback function
            _thread = CreateThread(0, 0, &Thread::threadEnter, this, 0, &_threadId);
            return  true;
        }
    }
    /**
    *   stop a thread, block the main thread
    */
    virtual void    join()
    {
        if (_thread)
        {
            WaitForSingleObject(_thread, 0xFFFFFFFF);// waite the thread to stop
            CloseHandle(_thread);
            _thread = 0;
        }
    }
};



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
            FrameInfor  infor;
            if (!_ffReader.readFrame(infor))
            {
                break;
            }
            //! 同步发送消息，等待返回；异步发送消息用：postMessage
            SendMessage(_hWnd, WM_UPDATE_VIDEO, (WPARAM)&infor, 0);
        }

        return  true;
    }
};