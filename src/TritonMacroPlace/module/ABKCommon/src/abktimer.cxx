/**************************************************************************
***    
*** Copyright (c) 1995-2000 Regents of the University of California,
***               Andrew E. Caldwell, Andrew B. Kahng and Igor L. Markov
*** Copyright (c) 2000-2010 Regents of the University of Michigan,
***               Saurabh N. Adya, Jarrod A. Roy, David A. Papa and
***               Igor L. Markov
***
***  Contact author(s): abk@cs.ucsd.edu, imarkov@umich.edu
***  Original Affiliation:   UCLA, Computer Science Department,
***                          Los Angeles, CA 90095-1596 USA
***
***  Permission is hereby granted, free of charge, to any person obtaining 
***  a copy of this software and associated documentation files (the
***  "Software"), to deal in the Software without restriction, including
***  without limitation 
***  the rights to use, copy, modify, merge, publish, distribute, sublicense, 
***  and/or sell copies of the Software, and to permit persons to whom the 
***  Software is furnished to do so, subject to the following conditions:
***
***  The above copyright notice and this permission notice shall be included
***  in all copies or substantial portions of the Software.
***
*** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
*** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*** OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
*** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
*** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
*** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
*** THE USE OR OTHER DEALINGS IN THE SOFTWARE.
***
***
***************************************************************************/


/*========================================================================
  =   Portable Timer Class    (SunOS, Solaris, Linux, NT, '95, DOS )     =
  =                                                Igor Markov 06/22/97  =
  = ( with the help of the Regents of the University of California ;-)   =
  ========================================================================
*/

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif


#include <iostream>
#include <iomanip>
#include <cmath>
#include "abkassert.h"
#include "abktimer.h"
#include "abkcpunorm.h"

#ifdef WIN32
#define _X86_
#include <windows.h>
#include <process.h>

class WinTimes
{
    public:
        enum  WinKind {NOTSET,WINNT,WIN95,WIN98PLUS,UNKNOWN};
    private:
        LARGE_INTEGER _freq; //frequency of perf counter
        LARGE_INTEGER _reTime1,_reTime2; //values of perf counter at start,stop
        HANDLE        _process;
        LONGLONG _usTime1,_usTime2; //units of 100 ns (freq irrelevant)
        LONGLONG _keTime1,_keTime2;
        static WinKind _winKind;
        static void _setWinKind();
    public:
        WinTimes();
        ~WinTimes(){::CloseHandle(_process);}
        inline double getFreq() {return double(_freq.QuadPart);}
        inline LONGLONG getRealTime1() {return _reTime1.QuadPart;}
        inline LONGLONG getRealTime2() {return _reTime2.QuadPart;}
        inline LONGLONG getKerTime1() {return _keTime1;}
        inline LONGLONG getKerTime2() {return _keTime2;}
        inline LONGLONG getUserTime1() {return _usTime1;}
        inline LONGLONG getUserTime2() {return _usTime2;}
        void doTimes1();
        void doTimes2();


};

WinTimes::WinKind WinTimes::_winKind=WinTimes::NOTSET;

void WinTimes::_setWinKind()
{
    if (_winKind != NOTSET)
    {
        return;
    }
    OSVERSIONINFO vInfo;
    vInfo.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
    if (!::GetVersionEx(&vInfo))
    {
        _winKind = UNKNOWN;
    }
    else if (vInfo.dwPlatformId==VER_PLATFORM_WIN32_NT)
    {
        _winKind = WINNT;
    }
    else if (vInfo.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS)
    {
        if (vInfo.dwMinorVersion>0)
        {
            _winKind = WIN98PLUS;
        }
        else
        {
            _winKind = WIN95;
        }
    }
    else
        _winKind = UNKNOWN;
}

WinTimes::WinTimes()
{
    _process = ::OpenProcess(PROCESS_QUERY_INFORMATION,
                                       FALSE,
                                       ::_getpid());
    int success=::QueryPerformanceFrequency(&_freq);
    abkfatal(success,"Unable to find hi-res timer");

}
void WinTimes::doTimes1()
{
    FILETIME CreationTime,ExitTime,KernelTime,UserTime;

    int success=::QueryPerformanceCounter(&_reTime1);
    abkfatal(success,"Can't get time");

    _setWinKind();
    if (_winKind==WINNT)
        {

        success=::GetProcessTimes(_process,
                                  &CreationTime,
                                  &ExitTime,
                                  &KernelTime,
                                  &UserTime);
        abkfatal(success,"Can't get time");
        _usTime1 =          ((__int64)UserTime.dwHighDateTime)<<32 
                                        | 
                            (__int64)UserTime.dwLowDateTime;

        _keTime1 =          ((__int64)KernelTime.dwHighDateTime)<<32 
                                        | 
                            (__int64)KernelTime.dwLowDateTime;
        }

}
void WinTimes::doTimes2()
{
    FILETIME CreationTime,ExitTime,KernelTime,UserTime;

    int success=::QueryPerformanceCounter(&_reTime2);
    abkfatal(success,"Can't get time");

    _setWinKind();

    if (_winKind==WINNT)
        {
        success=::GetProcessTimes(_process,
                                  &CreationTime,
                                  &ExitTime,
                                  &KernelTime,
                                  &UserTime);
        abkwarn(success,"Can't get time");
        _usTime2 =          ((__int64)UserTime.dwHighDateTime)<<32 
                                        | 
                            (__int64)UserTime.dwLowDateTime;

        _keTime2 =          ((__int64)KernelTime.dwHighDateTime)<<32 
                                        | 
                            (__int64)KernelTime.dwLowDateTime;
        }

}
#endif

Timer::Timer(double limitInSec):timeLimit(limitInSec)
{
#ifdef WIN32
    _winTimes = new WinTimes;
#endif
    status=TIMER_OFF;
    start(limitInSec);
}

#ifdef WIN32
Timer::~Timer()
{
    delete _winTimes;
}
#endif

void Timer::start(double limitInSec)
{
    abkfatal(status == TIMER_OFF, "Can't start timer twice");
    status = TIMER_ON;

    timeLimit=limitInSec;
 
#ifndef WIN32
    abkfatal(time(&realTime1)!=-1,"Can't get time");
#endif
#if defined (linux)
    abkfatal(getrusage(RUSAGE_SELF,&_ru)!=-1," Can't get time");
    UserTime1 = _ru.ru_utime.tv_sec+1e-6*_ru.ru_utime.tv_usec;
    SysTime1  = _ru.ru_stime.tv_sec+1e-6*_ru.ru_stime.tv_usec;
#elif defined(WIN32)
    _winTimes->doTimes1();
#else
    UserTime1=clock();
#endif
}
 
void Timer::stop()
{
    abkfatal(status == TIMER_ON, "Can't stop timer twice!\n");
    status=TIMER_OFF;

#ifndef WIN32
    abkfatal(time(&realTime2)!=-1,"Can't get time\n");
#endif

#if defined(linux)
    abkfatal(getrusage(RUSAGE_SELF,&_ru)!=-1," Can't get time");
    UserTime2 = _ru.ru_utime.tv_sec+1e-6*_ru.ru_utime.tv_usec;
    SysTime2  = _ru.ru_stime.tv_sec+1e-6*_ru.ru_stime.tv_usec;
#elif defined(WIN32)
    _winTimes->doTimes2();
#else
    UserTime2=clock();
#endif
}

void Timer::split()
{
    stop();
    status=TIMER_SPLIT;
}

void Timer::resume()
{
    abkfatal(status==TIMER_SPLIT,"Can''t resume timer unless split!\n");
    status=TIMER_ON;
}

double Timer::getUserTime() const
{
   abkfatal(status == TIMER_OFF || status==TIMER_SPLIT,
       "Have to stop timer to get a reading\n");
#if defined(linux)
    return (UserTime2-UserTime1);
#elif defined(WIN32)
    return (_winTimes->getUserTime2() - _winTimes->getUserTime1())*1.0e-7;
#else
    if (getRealTime()>(INT_MAX+0.0)/CLOCKS_PER_SEC)
       return 0.0; // not to give a suspiciously correct reading
    return ((UserTime2-UserTime1+0.0)/CLOCKS_PER_SEC);
#endif

}

double Timer::getSysTime() const
{
   abkfatal(status == TIMER_OFF || status==TIMER_SPLIT,
       "Have to stop timer to get a reading\n");
#if defined(linux)
    return (SysTime2-SysTime1);
#elif defined(WIN32)
    return (_winTimes->getKerTime2() - _winTimes->getKerTime1())*1.0e-7;
#else
    return 0;
#endif
}

double Timer::getCombTime() const
{
   abkfatal(status == TIMER_OFF || status==TIMER_SPLIT,
       "Have to stop timer to get a reading\n");
#if defined(linux)
    return (UserTime2-UserTime1)+(SysTime2-SysTime1);
#elif defined(WIN32)
    return
        ( (_winTimes->getUserTime2() - _winTimes->getUserTime1()) +
          (_winTimes->getKerTime2() - _winTimes->getKerTime1()) )
          *1.0e-7;
#else
    return ((UserTime2-UserTime1+0.0)/CLOCKS_PER_SEC);
#endif
}

double Timer::getRealTime() const
{
   abkfatal(status == TIMER_OFF || status==TIMER_SPLIT,
       "Have to stop timer to get a reading\n");
#ifdef WIN32
   return (_winTimes->getRealTime2()-_winTimes->getRealTime1())/
       _winTimes->getFreq();
#else
   return difftime(realTime2,realTime1);
#endif
}

double Timer::getUnixTime() const
{
    time_t utime, zero=0;
    abkfatal(time(&utime)!=-1,"Can't get time\n");
    return difftime(utime, zero);
} 

std::ostream& operator<<(std::ostream& os, const Timer& tm)
{
#if defined(WIN32) || defined(linux)
    CPUNormalizer cpunorm;
    double userSec=tm.getUserTime(),
           normSec=tm.getUserTime() * cpunorm.getNormalizingFactor();
    char   buffer[20];
    if (userSec>0.01) 
    {
       double frac =userSec-floor(userSec);
       double delta=frac-0.001*floor(frac*1000.0);
       userSec-=delta;
              frac =normSec-floor(normSec); 
              delta=frac-0.001*floor(frac*1000);
       normSec-=delta;
    }
  

    sprintf(buffer,"%.1e",tm.getSysTime());
    
    os << " " << userSec <<" user,"
       << " " << buffer <<" system," 
       << " " << tm.getRealTime() <<" real," 
       << " " << normSec << " norm'd "
       << "sec ";
#else
    if (tm.getRealTime()>(INT_MAX+0.0)/CLOCKS_PER_SEC)
       os << tm.getRealTime() << " real sec";
    else
       os << " " << tm.getUserTime() << " user sec, "
                 << tm.getRealTime() << " real sec";
#endif
    return os;
}
 
#ifdef WIN32
double Timer::getRealTimeOnTheFly()
{
    _winTimes->doTimes2();
    return (_winTimes->getRealTime2()-_winTimes->getRealTime1())/
            _winTimes->getFreq();
}
double Timer::getUserTimeOnTheFly()
{
    _winTimes->doTimes2();
    return (_winTimes->getUserTime2() - _winTimes->getUserTime1())*1.0e-7;
}
double Timer::getCombTimeOnTheFly()
{
    _winTimes->doTimes2();
    return
        ( (_winTimes->getUserTime2() - _winTimes->getUserTime1()) +
          (_winTimes->getKerTime2() - _winTimes->getKerTime1()) )
          *1.0e-7;
}

#endif // WIN32

