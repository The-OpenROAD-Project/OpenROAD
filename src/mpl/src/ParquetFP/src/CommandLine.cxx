/**************************************************************************
***
*** Copyright (c) 2000-2007 Regents of the University of Michigan,
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

// 040608 hhchan param printout up to v2.1 are placed in the "classic" fcn

#include "CommandLine.h"

#include <iostream>
#include <sstream>
#include <string>

#include "baseannealer.h"

using namespace parquetfp;
using std::cerr;
using std::cout;
using std::endl;

#ifdef _MSC_VER
#ifndef srand48
#define srand48 srand
#endif
#endif

Command_Line::Command_Line()
    : getSeed(false),
      budgetTime(0),
      softBlocks(0),
      initQP(0),
      inFileName(""),
      outPlFile(""),
      capoPlFile(""),
      capoBaseFile(""),
      baseFile(""),
      FPrep("Best"),
      seed(0),
      iterations(1),
      maxIterHier(10),
      seconds(0.0f),
      plot(0),
      plotNoNets(false),
      plotNoSlacks(false),
      plotNoNames(false),
      savePl(0),
      saveCapoPl(0),
      saveCapo(0),
      save(0),
      takePl(0),
      solveMulti(0),
      clusterPhysical(0),
      solveTop(0),
      maxWSHier(15),
      usePhyLocHier(0),
      dontClusterMacros(0),
      maxTopLevelNodes(-9999),
      timeInit(30000.0f),
      timeCool(0.01f),
      startTime(30000.0f),
      reqdAR(-9999.0f),
      maxWS(15.0f),
      minWL(0),
#ifdef USEFLUTE
      useSteiner(false),
      printSteiner(false),
#endif
      areaWeight(0.4f),
      wireWeight(0.4f),
      useFastSP(false),
      lookAheadFP(false),
      initCompact(0),
      compact(0),
      verb(0),
      packleft(true),
      packbot(true),
      scaleTerms(true),
      shrinkToSize(-1.f),
      noRotation(false)
{
  setSeed();
}

void Command_Line::setSeed()
{
  std::srand(seed);  // seed for rand function
}
