#pragma once

#include <windows.h>

class Timestamp
{
public:
    Timestamp()
    {
        QueryPerformanceFrequency(&_frequency);
        QueryPerformanceCounter(&_startCount);
    }
    ~Timestamp()
    {}

    void    update()
    {
        QueryPerformanceCounter(&_startCount);
    }
    /**
    *   ��ȡ��ǰ��
    */
    double getElapsedSecond()
    {
        return  getElapsedTimeInMicroSec() * 0.000001;
    }
    /**
    *   ��ȡ����
    */
    double getElapsedTimeInMilliSec()
    {
        return this->getElapsedTimeInMicroSec() * 0.001;
    }
    /**
    *   ��ȡ΢��
    */
    double getElapsedTimeInMicroSec()
    {
        LARGE_INTEGER endCount;
        QueryPerformanceCounter(&endCount);

        double  startTimeInMicroSec = _startCount.QuadPart * (1000000.0 / _frequency.QuadPart);
        double  endTimeInMicroSec = endCount.QuadPart * (1000000.0 / _frequency.QuadPart);

        return  endTimeInMicroSec - startTimeInMicroSec;
    }
protected:
    LARGE_INTEGER   _frequency;
    LARGE_INTEGER   _startCount;
};