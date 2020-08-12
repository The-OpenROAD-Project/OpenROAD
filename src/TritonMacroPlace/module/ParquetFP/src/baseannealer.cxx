/**************************************************************************
***
*** Copyright (c) 2000-2006 Regents of the University of Michigan,
***               Saurabh N. Adya, Hayward Chan, Jarrod A. Roy
***               and Igor L. Markov
***
***  Contact author(s): sadya@umich.edu, imarkov@umich.edu
***  Original Affiliation:   University of Michigan, EECS Dept.
***                          Ann Arbor, MI 48109-2122 USA
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


#include "ABKCommon/abkMD5.h"
#include "baseannealer.h"
#include "basepacking.h"
#include "CommandLine.h"
#include "DB.h"
#include "AnalytSolve.h"

#ifdef _MSC_VER
#ifndef srand48
#define srand48 srand
#endif
#endif

#ifdef WIN32
#define _X86_
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#endif

using std::cout;
using std::endl;

const int BaseAnnealer::UNINITIALIZED=-1;
const unsigned int BaseAnnealer::UNSIGNED_UNINITIALIZED=0;
const int BaseAnnealer::FREE_OUTLINE=-9999;
const int BaseAnnealer::NOT_FOUND=-1;
   
// --------------------------------------------------------
BaseAnnealer::BaseAnnealer(const parquetfp::Command_Line *const params,
                           parquetfp::DB *const db)
   : _db(db),
     _params(params),
     _analSolve(new parquetfp::AnalytSolve(
                   const_cast<parquetfp::Command_Line*>(params),
                   const_cast<parquetfp::DB*>(db))),
     
     _isFixedOutline(_params->reqdAR != FREE_OUTLINE),
     _outlineDeadspaceRatio((_isFixedOutline)
                            ? (1 + (_params->maxWS/100.0))
                            : basepacking_h::Dimension::Infty),
     
     _outlineArea((_isFixedOutline)
                  ? (_db->getNodesArea() * _outlineDeadspaceRatio)
                  : basepacking_h::Dimension::Infty),
     
     _outlineWidth((_isFixedOutline)
                   ? sqrt(_outlineArea * _params->reqdAR)
                   : basepacking_h::Dimension::Infty),
     
     _outlineHeight((_isFixedOutline)
                    ? sqrt(_outlineArea / _params->reqdAR)
                    : basepacking_h::Dimension::Infty)
{
   //compilerCheck();

   // set the random seed for each invokation of the Annealer
   unsigned rseed;
   if(_params->getSeed)
   {
      //rseed = int(time((time_t *)NULL));
      Timer seedtm;
      char buf[255];
      #if defined(WIN32)
        int procID=_getpid();
        LARGE_INTEGER hiPrecTime;
        ::QueryPerformanceCounter(&hiPrecTime);
        LONGLONG hiP1=hiPrecTime.QuadPart;
        sprintf(buf,"%g %d %I64d",seedtm.getUnixTime(),procID,hiP1);
      #else
        unsigned procID=getpid();
        unsigned rndbuf;
        FILE *rnd=fopen("/dev/urandom","r");
        if(rnd)
          {
           if (fread(&rndbuf,sizeof(rndbuf),1,rnd) != 1) {
             rndbuf = rand();
           }
           fclose(rnd);
           sprintf(buf,"%g %d %d",seedtm.getUnixTime(),procID,rndbuf);
          }
        else
          sprintf(buf,"%g %d",seedtm.getUnixTime(),procID);
      #endif
      MD5 hash(buf);
      rseed = hash;
   }
   else
      rseed = _params->seed;
   
   srand(rseed);        //seed for rand function
   srand48(rseed);      //seed for random_shuffle function
   if(_params->verb.getForMajStats() > 0)
      cout << "The random seed for this run is: " << rseed << endl;
   
   _baseFileName = _params->inFileName;
   annealTime = 0.0;
}
// --------------------------------------------------------
BaseAnnealer::~BaseAnnealer()
{
   if (_analSolve != NULL)
      delete _analSolve;
}
// --------------------------------------------------------
void BaseAnnealer::solveQP()  
{
   _analSolve->solveSOR();
   _db->updatePlacement(_analSolve->getXLocs(), _analSolve->getYLocs());
}
// --------------------------------------------------------
void BaseAnnealer::postHPWLOpt()
{
//    parquetfp::Point offset = _analSolve->getDesignOptLoc();
//    cout << "offset.x " << offset.x
//         << " offset.y " << offset.y << endl;
   
//    float initHPWL = _db->evalHPWL();
//    cout << "initHPWL: " << initHPWL << endl;
//    _db->shiftDesign(offset);
   
//    float afterHPWL = _db->evalHPWL();
//    cout << "afterHPWL: " << afterHPWL << endl;      
//    if(afterHPWL > initHPWL)
//    {
//       cout << "shifting not done." << endl;
//       offset.x *= -1;
//       offset.y *= -1;
//       _db->shiftDesign(offset);
//    }
//    else
//       cout << "shifting is done." << endl;

   // only shift in "fixed-outline" mode
   if (_params->reqdAR != FREE_OUTLINE)
   {
      float blocksArea = _db->getNodesArea();
      float reqdAR = _params->reqdAR;
      float reqdArea = blocksArea * (1 + _params->maxWS/100.0);
      float reqdWidth = sqrt(reqdArea * reqdAR);
      float reqdHeight = sqrt(reqdArea / reqdAR);
      
//       printf("blocksArea: %.2f reqdWidth: %.2f reqdHeight: %.2f\n",
//              blocksArea, reqdWidth, reqdHeight);
#ifdef USEFLUTE
      _db->shiftOptimizeDesign(reqdWidth, reqdHeight, _params->scaleTerms, _params->useSteiner, _params->verb);
#else
      _db->shiftOptimizeDesign(reqdWidth, reqdHeight, _params->scaleTerms, _params->verb);
#endif
   }
}
// --------------------------------------------------------
void BaseAnnealer::printResults(const Timer& tm,
                                const SolutionInfo& curr) const
{
   float timeReqd = tm.getUserTime();
   float realTime = tm.getRealTime();

   float blocksArea = _db->getNodesArea();
   float currArea = curr.area;
   float currWidth = curr.width;
   float currHeight = curr.height;
   float currAR = currWidth / currHeight; 
   float whiteSpace = 100 * (currArea - blocksArea) / blocksArea;
   float HPWL = curr.HPWL;
   float reqdAR = _params->reqdAR;
   float reqdArea = blocksArea * (1 + _params->maxWS/100.0);
   float reqdWidth = sqrt(reqdArea * reqdAR);
   float reqdHeight = sqrt(reqdArea / reqdAR);

   cout.precision(6);

   if(_params->verb.getForSysRes() > 0)
      cout << "realTime:" << realTime << "\tuserTime:" << timeReqd << endl;

   if(_params->verb.getForMajStats() > 0)
      cout << "Final Area: " << currArea << " WhiteSpace " << whiteSpace
           << "%" << " AR " << currAR << " HPWL " << HPWL << endl;

   if(_params->verb.getForMajStats() > 0)
   {
      if (_params->dontClusterMacros && _params->solveTop)
         cout << "width w/ macros only: "
              << _db->getXMaxWMacroOnly() << " ";
      else
         cout << "width:  " << currWidth << " ";

      if (_params->reqdAR != FREE_OUTLINE)
         cout << "(outline width:  " << reqdWidth << ") ";
      cout << endl;
      
      if (_params->dontClusterMacros && _params->solveTop)
         cout << "height w/ macros only: "
              << _db->getYMaxWMacroOnly() << " ";
      else
         cout << "height: " << currHeight << " ";

      if (_params->reqdAR != FREE_OUTLINE)
          cout << "(outline height: " << reqdHeight << ") ";
      cout << endl;
      
      cout << "area utilization (wrt. total current area): "
           << (blocksArea / currArea) * 100 << "%" << endl;
      cout << "whitespace       (wrt. total current area): " 
           << (1 - (blocksArea/currArea)) * 100 << "%" << endl;
   }
}
// --------------------------------------------------------

