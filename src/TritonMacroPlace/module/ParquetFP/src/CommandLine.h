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
#include "ABKCommon/abkcommon.h"
#include "FPcommon.h"

namespace parquetfp
{
   class Command_Line
   {
   public:
      Command_Line (int argc, const char *argv[]);
      Command_Line ();
                
      // -----public member variables to store params-----
      bool getSeed, budgetTime;
      bool softBlocks;
      bool initQP;     // initialize a QP soln
      std::string inFileName;
      std::string outPlFile;
      std::string capoPlFile;
      std::string capoBaseFile;
      std::string baseFile;

      std::string FPrep;
      
      unsigned seed;   // fixed seed
      int iterations;  // number of runs
      int maxIterHier; // max # iterations during hierarchical flow

      float seconds;
      bool plot;             // plot to out.plt
      bool plotNoNets;       // do not plot nets 
      bool plotNoSlacks;     // do not plot slacks
      bool plotNoNames;      // do not plot names
      bool savePl;           // save .pl file as output
      bool saveCapoPl;       // save .pl file in Capo format
      bool saveCapo;         // save files in Capo format
      bool save;             // save files in bookshelf format

      bool takePl;           // takes a Placement and converts it 
                             // to sequence pair for initial soln

      // -----Parameters for 2-level hierarchy-----
      bool solveMulti;       // use multilevel heirarchy
      bool clusterPhysical;  // use physical hierarchy instead of 
                             // closest node
      bool solveTop;         // solve only top level of heirarchy
      float maxWSHier;      // maximum WS for heirarchical blocks
                             // default: 15%
      bool usePhyLocHier;    // use physical locs which updating 
      // locs of sub-blocks of clustered blocks, if using physical 
      // clustering. usefull for eco purposes
      bool dontClusterMacros; // keep macros out of clustering
                              // default: false
      int maxTopLevelNodes; // number of top-level nodes required during
                            // clustering. if -9999 then use sqrt(#nodes)

      // -----Annealer performance parameters next-----
      float timeInit;     // initial temperature default 30000
      float timeCool;     // cooling temperature default 0.01
      float startTime;    // default to timeInit
                          // (this is the time the area & WL are normalized to)
      float reqdAR;       // required Aspect Ratio of fixed outline
                          // default -9999(means no fixed outline
                          // desired)
      float maxWS;        // if fixed-outline then maximum whitespace
                          // acceptable
      bool minWL;         // whether HPWL minimization desired
#ifdef USEFLUTE
      bool useSteiner;    // use Steiner WL instead of HPWL
      bool printSteiner;
#endif
      float areaWeight;   // weight for area minimization
      float wireWeight;   // weight for WL minimization

      bool useFastSP;     // whether to use fast SP(O(nlog n)) algo 
                          // for SPEval

      bool lookAheadFP;   // <aaronnn> run FP in lookahead mode (fast)

      // -----pre/post-processing-----
      bool initCompact;   // whether to use compaction to generate
                          // initial SP (default: false)
      bool compact;       // compact final soln (default: false)

      Verbosity verb;     // by royj to control the verbosity of Parquet

      bool packleft;      // directions the SPannealer should use
      bool packbot;

      bool scaleTerms;

      BBox nonTrivialOutline;

      float shrinkToSize;

      bool noRotation;

      // print usage info. to cerr
      void printHelp(int argc, const char *argv[]) const;
      
      // print the Annealer params
      void printAnnealerParams() const;
      void printAnnealerParamsClassic() const;
      void setSeed();
   };
}
//using namespace parquetfp;
#endif // COMMAND_LINE_H
