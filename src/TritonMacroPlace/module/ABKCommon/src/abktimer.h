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







//! author="Igor Markov 06/22/97 "      
/*   Portable Timer Class    (SunOS, Solaris, Linux, NT  , '95  , DOS  )  *
                                                  
   ( with the help of the Regents of the University of California ;-)   *

 970723 ilm    Changed #include <bool.h> to #include <stl-config.h>
               for STL 2.01 compliance  
 CHANGES

 970901 ilm    completely reworked the Timer class to circumvent the
                 wrap-around problem on all platforms and provide separate 
                 user/system time readings on "supported platforms" 
                 (see below)
 970901 ilm    even though class Timer is (expectected to be) completely
                 portable, it has "better supported" platforms (currently 
                 _SUNPRO_CC). This means that platform-specific calls will
                 be used instead of common functions to provide better 
                 granularity of time readings and circumvent the wrap-around 
                 problem
                    However, the effect of the wrap-around problem should
                 be minimal on other platforms too. The only precaution
                 for users of "other" platforms (i.e. not _SUNPRO_CC)
                 is not to use Timer::expired() after the time wrap-around 
                 period of (INT_MAX+0.0)/CLOCKS_PER_SEC sec (can be 36sec)
                 use realTimeExpired() instead

                    Note: on Windows 95 (perhaps, on NT as well),
                 CLOCKS_PER_SEC is not big, therefore the granularity 
                 problem is not as bad as it was on Solaris with older
                 versions of class Timer
*/
#ifndef _ABKTIMER_H_
#define _ABKTIMER_H_

#include <time.h>
#include <iostream>
#include "abkassert.h"

//#ifndef _MSC_VER
//#include <stl_config.h>
//#endif

#ifdef WIN32
class WinTimes;
#endif 

#if defined(linux)
 #include <sys/resource.h>
#endif      

//: Used to record the CPU time of process 
class Timer 
{
#ifndef WIN32
    time_t  realTime1, realTime2;
#endif
    double  UserTime1, UserTime2;
#if defined(linux)
    double  SysTime1,  SysTime2;
#endif
    double  timeLimit;

#ifdef WIN32
    double getRealTimeOnTheFly();
    double getUserTimeOnTheFly();
    double getCombTimeOnTheFly();
#else
    inline double getRealTimeOnTheFly();
    inline double getUserTimeOnTheFly();
#if defined(linux)
    inline double  getSysTimeOnTheFly();
#endif
    inline double getCombTimeOnTheFly();

#endif  //WIN32

#if defined (linux)
   struct rusage _ru;
#endif

#ifdef WIN32
   WinTimes *_winTimes;
#endif

 protected:
    enum {TIMER_ON, TIMER_OFF, TIMER_SPLIT } status;
 public:

    Timer(double limitInSec=0.0);
#ifndef WIN32
   ~Timer() { };
#else
   ~Timer();
#endif
    void start(double limitInSec=0.0);
    void stop();
    void split(); //call to allow taking a reading without interrupting timing
    void resume(); //call after taking a reading after calling split()
    bool isStopped()       const { return status == TIMER_OFF; }
    
    double  getUserTime()  const; // processor time in seconds 
    
    double   getSysTime()  const; // processor time in seconds 
    double  getCombTime()  const;
    // processor time in seconds  (sum of the prev. two)
    
    double  getRealTime() const; /// real time in seconds 
    double  getUnixTime() const;  
    // Unix time (large number for in randseed)
 
    inline bool    expired();
    // class expired and realtimeexpired can be used to check if 
    //  the time is over. The choice of CPU time over
    //  real time for expiration is explained by a much finer granularity of
    //  measurment. The author observed sensitivity of up to 0.001
    //  CPU sec. on Sparc architecture (while real seconds were integers).
    //  DO NOT CALL THIS METHOD ON "OTHER" PLATFORMS  (not __SUNPRO_CC)
    //  AFTER WRAP_AROUND HAPPENED: (INT_MAX+0.0)/CLOCKS_PER_SEC) sec
    //  (can be 36 mins), call realTimeExpired() instead
    inline bool    realTimeExpired();

    friend std::ostream& operator<<(std::ostream& os, const Timer& tm );
};

/* ----------------------      IMPLEMENTATIONS  --------------------*/
#ifndef WIN32
inline double Timer::getRealTimeOnTheFly() 
{ 
  time(&realTime2);
  return difftime(realTime2,realTime1);
}

inline double Timer::getUserTimeOnTheFly() 
{ 
#if defined(gnu)
   abkfatal(getrusage(RUSAGE_SELF,&_ru)!=-1,"Can't get time");
   return _ru.ru_utime.tv_sec+1e-6*_ru.ru_utime.tv_usec - UserTime1;
#else
  return (clock()-UserTime1+0.0)/CLOCKS_PER_SEC;
#endif
}

#if defined(linux)
inline double Timer::getSysTimeOnTheFly() 
{ 
  abkfatal(getrusage(RUSAGE_SELF,&_ru)!=-1,"Can't get time");
  return  _ru.ru_stime.tv_sec+1e-6*_ru.ru_stime.tv_usec - SysTime1;
}
#endif

inline double Timer::getCombTimeOnTheFly() 
{ 
#if defined(linux)
   abkfatal(getrusage(RUSAGE_SELF,&_ru)!=-1,"Can't get time");
   return   _ru.ru_utime.tv_sec+1e-6*_ru.ru_utime.tv_usec - UserTime1
           +_ru.ru_stime.tv_sec+1e-6*_ru.ru_stime.tv_usec - SysTime1;
#else
  return getUserTimeOnTheFly();
#endif
}
#endif //WIN32

inline bool Timer::expired() 
{ 
   return (timeLimit<getCombTimeOnTheFly()) && (timeLimit!=0.0); 
}

inline bool Timer::realTimeExpired() 
{ 
   return (timeLimit<getRealTimeOnTheFly()) && (timeLimit!=0.0); 
}

#endif
