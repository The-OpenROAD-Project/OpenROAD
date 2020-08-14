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




// June 15, 1997   Igor Markov  VLSI CAD UCLA ABKGROUP

// This file to be included into all projects in the group

#include "infolines.h"

#include <cstdlib>
#include <time.h>
#include <iostream>
#include <iomanip>
#include <cfloat>
#include <cstring>

using std::ostream;
using std::setw;
using std::cout;
using std::endl;

/* ======================== IMPLEMENTATIONS ======================== */
TimeStamp::TimeStamp(bool justTime)
{ 
   char * date;
   time_t tp;   

   char expl[]="# Created      : ";
   if (time(&tp)==-1)
   {
     _infoLine = new char[80]; 
     strcpy(_infoLine," Error in time() "); 
      return;
   }
   date=asctime(localtime(&tp));
   if(justTime) {
     _infoLine = new char[strlen(date)+1];
     strcpy(_infoLine,date);
   }
   else {
     _infoLine = new char[strlen(expl)+strlen(date)+1];
     strcpy(_infoLine,expl);
     strcat(_infoLine,date);
   }
}

CmdLine::CmdLine(int argc, const char *argv[])
{
  char expl[]="# Command line :";
  int len=strlen(expl), n=argc;
  while (n--) len+=(1+strlen(argv[n])); 
  if (len<255) len=255;

  _infoLine=new char[len+3]; 

  char * infoPtr=_infoLine;
  strcpy(infoPtr,expl);
  infoPtr += strlen(expl);
  infoPtr[0]=' ';
  infoPtr++;

  n=-1;
  while (++n<argc)
  {
      strcpy(infoPtr,argv[n]);
      infoPtr += strlen(argv[n]);
      infoPtr[0]=' '; 
      infoPtr++;
  }
  infoPtr[0]='\n';
  infoPtr[1]='\0';
}

ostream& operator<<(ostream& out, const MemUsage& memu)
{
  out << "# Virtual Memory usage : " << setw(7) << memu.getEstimate() 
      << "MB (estimate)";
  return out;
}

MaxMem::MaxMem(void): _printExtra(false), _noticedThrashing(false),
_peak(0.), _initialMinFaults(-DBL_MAX), _initialMajFaults(-DBL_MAX),
_message("No measurements taken.") 
{}

void MaxMem::update(const char* message)
{
  double currmem = MemUsage();
  if(currmem > _peak)
  {
     _peak = currmem;
     _message = message;
  }
  if(_initialMinFaults == -DBL_MAX)
  {
     _initialMinFaults = VMemUsage::measureMinPageFaults();
     _initialMajFaults = VMemUsage::measureMajPageFaults();
     _noticedThrashing = _initialMinFaults != -1. && _initialMajFaults != -1. && _initialMajFaults/_initialMinFaults > 0.02;
     if(_noticedThrashing)
     {
       cout << "# Minor page faults: " << _initialMinFaults << " Major page faults: " << _initialMajFaults << endl
            << "# Thrashing Detected!" << endl
            << "# Process may be trying to use more memory than is available" << endl
            << "# (memory might be consumed by other processes or file cache)." << endl;
     }
     _noticedThrashing = _noticedThrashing || _initialMinFaults == -1. || _initialMajFaults == -1.;
   }
   else
   {
     if(!_noticedThrashing)
     {
       const double &majfaults = VMemUsage::measureMajPageFaults();
       _noticedThrashing = majfaults > 2.*_initialMajFaults && majfaults/_initialMinFaults > 0.04;
       if(_noticedThrashing)
       {
         cout << "# Minor page faults: " << VMemUsage::measureMinPageFaults() << " Major page faults: " << majfaults << endl
              << "# Thrashing Detected!" << endl
              << "# Process may be trying to use more memory than is available" << endl
              << "# (memory might be consumed by other processes or file cache)." << endl;
       }
     }
   }
}

bool MaxMem::noticedThrashing() const
{
  return _noticedThrashing;
}

bool MaxMem::printExtra() const
{
  return _printExtra;
}

void MaxMem::setPrintExtra(bool print)
{
  _printExtra = print;
}

double MaxMem::getPeak() const
{
  return _peak;
}

const std::string &MaxMem::getMessage() const
{
  return _message;
}

std::ostream& operator<<(std::ostream& out, const MaxMem& maxm)
{
  const double majfaults = VMemUsage::measureMajPageFaults();
  if((maxm.printExtra() && majfaults != -1) || majfaults > 0.) {
    out <<"Minor page faults: " << VMemUsage::measureMinPageFaults()
        <<" Major page faults: " << majfaults << endl;
  }
  const double rss = VMemUsage::measureRSS();
  if(rss != -1) out <<"Current resident memory: " << rss << "MB ";
  out <<"Peak process memory thus far: "  << maxm.getPeak() << "MB" << endl
      <<"Peak process memory observed in \"" << maxm.getMessage().c_str() << "\"" << endl;
  return out;
}

ostream& operator<<(ostream& out, const SysInfo& si)
{
  out << si.tm
      << si.pl
      << si.us
      << si.mu
      << si.cpunorm << endl;
  return out;
}

