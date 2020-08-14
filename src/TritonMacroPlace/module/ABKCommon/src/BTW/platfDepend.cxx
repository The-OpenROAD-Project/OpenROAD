/**************************************************************************
***    
***    Copyright (c) 1995-1997 by ABKGroup, UCLA VLSI CAD Laboratory,
***    UCLA Computer Science Department, Los Angeles, CA 90095-1596 USA, 
***    and by the Regents of the University of California.    
***    All rights reserved.   
***
***    No portion of this file may be used, copied, or transmitted for
***    any purpose, including any research or commercial purpose,
***    without the express written consent of Professor Andrew B. Kahng,
***    UCLA Computer Science Department, 3713 Boelter Hall, Los Angeles,
***    CA 90095-1596 USA.   Any copy made of this file must include this
***    notice.    ABKGroup and the Regents of the University of California
***    retain title to all copies of this file and/or its compiled
***    derivatives.
***
***************************************************************************/

// August 22, 1997   Igor Markov  VLSI CAD UCLA ABKGROUP

// This file to be included into all projects in the group
// it contains platform-specific code portability of which relies
// on symbols defined by compilers

// 970825 mro made corrections to conditional compilation in ctors for
//            Platform and User:
//            i)   _MSC_VER not __MSC_VER (starts with single underscore)
//            ii)  allocated more space for _infoLines
//            iii) changed nested #ifdefs to #if ... #elif ... #else

#include <stdio.h>
#include <string.h>
#include "infolines.h"
#include "abktypes.h"

#ifdef __SUNPRO_CC
 #include <sys/systeminfo.h>
 #include <sys/stat.h>
 #include <unistd.h>
 #include <netdb.h>
//#include <sys/resource.h>
//extern "C" int getrusage(int, struct rusage *);
//extern "C" int getpagesize(void);
#endif


/* ======================== IMPLEMENTATIONS ======================== */

MemUsage::MemUsage()
{
#ifdef __SUNPRO_CC
  char procFileName[20];
  sprintf(procFileName,"/proc/%d",getpid());
  struct stat buf;
  if (stat(procFileName,&buf))
  {
    _memUsage=-1.0;
    return;
  }
  _memUsage=(1.0/2048.0)*buf.st_blocks;
#else
  _memUsage=-1.0;
#endif
}

Platform::Platform()
{
#if defined(__SUNPRO_CC)
  Str31 sys, rel, arch, platf;
  sysinfo(SI_SYSNAME, sys, 31);
  sysinfo(SI_RELEASE, rel, 31);
  sysinfo(SI_ARCHITECTURE,arch, 31);
  sysinfo(SI_PLATFORM,platf, 31);
  _infoLine= new char[strlen(sys)+strlen(rel)+strlen(arch)+strlen(platf)+30];
  sprintf(_infoLine,"# Platform   : %s %s %s %s \n",sys,rel,arch,platf);
#elif defined(_MSC_VER)
  _infoLine= new char[40];
  strcpy(_infoLine, "# Platform   : MS Windows \n");
#else
  _infoLine= new char[40];
  strcpy(_infoLine, "# Platform   : unknown \n");
#endif
}

User::User()
{
#if defined(__SUNPRO_CC)
//Str31 host, dom, user;
  Str31            user;
  const char *host=gethostent()->h_aliases[0];
//sysinfo(SI_HOSTNAME, host, 31);
//sysinfo(SI_SRPC_DOMAIN, dom, 31);
  cuserid(user);
//_infoLine= new char[strlen(host)+strlen(dom)+strlen(user)+30];
  _infoLine= new char[strlen(host)+            strlen(user)+30];
//sprintf(_infoLine,"# User       : %s@%s (%s) \n",user,host,dom);
  sprintf(_infoLine,"# User       : %s@%s \n"     ,user,host    );
#elif defined(_MSC_VER)
  _infoLine= new char[40];
  strcpy(_infoLine, "# User       : unknown \n");
#else
  _infoLine= new char[40];
  strcpy(_infoLine, "# User       : unknown \n");
#endif
}
/* 
ResourceUsage::ResourceUsage()
 Solaris notes for getrusage

      User time     The total amount of time spent  executing  in
                    user  mode.

      System time   The total amount of time spent  executing  in
                    system  mode.   Time  is given in seconds and
                    microseconds.

      Max memory    The way resident set size is calculated is an
         used       approximation, and could misrepresent the true
                    resident set size.
{ 
#ifdef __SUNPRO_CC
   char buf[100];
   char format[100];
   struct rusage usage;
   if (getrusage(RUSAGE_SELF, &usage))
   {
     _infoLine=new char[80];
     strcpy(_infoLine," Error in getrusage() "); 
     return;
   }
 
   sprintf(format," Time elapsed, sec : %s %s \n %s", " %6.3f (user) ",
                  " %6.3f (sys)", "Max memory used (Mb) : %6.3f ");

   sprintf(buf,format, usage.ru_utime.tv_sec+0.001*usage.ru_utime.tv_usec,
                       usage.ru_stime.tv_sec+0.001*usage.ru_stime.tv_usec,
                       1e-6*usage.ru_maxrss*getpagesize());

   _infoLine = new char[strlen(buf)];
   strcpy(_infoLine,buf);
#else
   _infoLine = new char[80];
   strcpy(_infoLine," System resource usage unknown (unsupported platform) ");
#endif
*/
