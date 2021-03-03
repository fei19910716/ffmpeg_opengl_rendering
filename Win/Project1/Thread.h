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
    *   �߳���ں���
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
    *   �����̺߳���
    */
    virtual bool    start()
    {
        if (_thread != 0)
        {
            return  false;
        }
        else
        {
            //HIGH_PRIORITY_CLASS
            _thread = CreateThread(0, 0, &Thread::threadEnter, this, 0, &_threadId);
            return  true;
        }
    }
    /**
    *   �ȴ��˳�����
    */
    virtual void    join()
    {
        if (_thread)
        {
            WaitForSingleObject(_thread, 0xFFFFFFFF);
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
    *   �����ļ�
    */
    virtual void    load(const char* fileName)
    {
        _ffReader.load(fileName);
    }
    /**
    *   �߳�ִ�к���
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
            //! ������Ҫ֪ͨ���ڽ����ػ��Ƹ��£���ʾ��������
            SendMessage(_hWnd, WM_UPDATE_VIDEO, (WPARAM)&infor, 0);
        }

        return  true;
    }
};