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

#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

// 040608 hhchan added StringParam "FPrep" ("SeqPair" or "BTree")

#include <string>

#include "FPcommon.h"

namespace parquetfp {
class Command_Line
{
 public:
  Command_Line();

  // -----public member variables to store params-----
  bool getSeed{false}, budgetTime{false};
  bool softBlocks{false};
  bool initQP{false};  // initialize a QP soln
  std::string inFileName;
  std::string outPlFile;
  std::string capoPlFile;
  std::string capoBaseFile;
  std::string baseFile;

  std::string FPrep{"Best"};

  unsigned seed{0};     // fixed seed
  int iterations{1};    // number of runs
  int maxIterHier{10};  // max # iterations during hierarchical flow

  float seconds{0.0f};
  bool plot{false};          // plot to out.plt
  bool plotNoNets{false};    // do not plot nets
  bool plotNoSlacks{false};  // do not plot slacks
  bool plotNoNames{false};   // do not plot names
  bool savePl{false};        // save .pl file as output
  bool saveCapoPl{false};    // save .pl file in Capo format
  bool saveCapo{false};      // save files in Capo format
  bool save{false};          // save files in bookshelf format

  bool takePl{false};  // takes a Placement and converts it
                       // to sequence pair for initial soln

  // -----Parameters for 2-level hierarchy-----
  bool solveMulti{false};       // use multilevel heirarchy
  bool clusterPhysical{false};  // use physical hierarchy instead of
                                // closest node
  bool solveTop{false};         // solve only top level of heirarchy
  float maxWSHier{15};          // maximum WS for heirarchical blocks
                                // default: 15%
  bool usePhyLocHier{false};    // use physical locs which updating
  // locs of sub-blocks of clustered blocks, if using physical
  // clustering. usefull for eco purposes
  bool dontClusterMacros{false};  // keep macros out of clustering
                                  // default: false
  int maxTopLevelNodes{-9999};    // number of top-level nodes required during
                                  // clustering. if -9999 then use sqrt(#nodes)

  // -----Annealer performance parameters next-----
  float timeInit{30000.0f};  // initial temperature default 30000
  float timeCool{0.01f};     // cooling temperature default 0.01
  float startTime{
      30000.0f};           // default to timeInit
                           // (this is the time the area & WL are normalized to)
  float reqdAR{-9999.0f};  // required Aspect Ratio of fixed outline
                           // default -9999(means no fixed outline
                           // desired)
  float maxWS{15.0f};      // if fixed-outline then maximum whitespace
                           // acceptable
  bool minWL{false};       // whether HPWL minimization desired
  float areaWeight{0.4f};  // weight for area minimization
  float wireWeight{0.4f};  // weight for WL minimization

  bool useFastSP{false};  // whether to use fast SP(O(nlog n)) algo
                          // for SPEval

  bool lookAheadFP{false};  // <aaronnn> run FP in lookahead mode (fast)

  // -----pre/post-processing-----
  bool initCompact{false};  // whether to use compaction to generate
                            // initial SP (default: false)
  bool compact{false};      // compact final soln (default: false)

  int verb{false};  // by royj to control the verbosity of Parquet

  bool packleft{true};  // directions the SPannealer should use
  bool packbot{true};

  bool scaleTerms{true};

  BBox nonTrivialOutline;

  float shrinkToSize{-1.f};

  bool noRotation{false};

  void setSeed();
};
}  // namespace parquetfp
// using namespace parquetfp;
#endif  // COMMAND_LINE_H
