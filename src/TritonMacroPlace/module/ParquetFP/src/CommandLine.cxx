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

#include "ABKCommon/abkcommon.h"
#include "ABKCommon/verbosity.h"
#include "CommandLine.h"
#include "baseannealer.h" 

#include <iostream>
#include <string>
#include <sstream>

using namespace parquetfp;
using std::cout;
using std::cerr;
using std::endl;

#ifdef _MSC_VER
#ifndef srand48
#define srand48 srand
#endif
#endif



Command_Line::Command_Line ()
   : getSeed(false),
     budgetTime(0), softBlocks(0), initQP(0),
     inFileName(""), outPlFile(""), capoPlFile(""),
     capoBaseFile(""), baseFile(""), FPrep("Best"),
     seed(0), iterations(1), maxIterHier(10),
     seconds(0.0f), plot(0), savePl(0), saveCapoPl(0), saveCapo(0), 
     save(0), takePl(0), solveMulti(0), clusterPhysical(0), 
     solveTop(0), maxWSHier(15), usePhyLocHier(0),
     dontClusterMacros(0), maxTopLevelNodes(-9999),
     timeInit(30000.0f), timeCool(0.01f), 
     startTime(30000.0f), reqdAR(-9999.0f), maxWS(15.0f), minWL(0), 
#ifdef USEFLUTE
     useSteiner(false), printSteiner(false),
#endif
     areaWeight(0.4f), wireWeight(0.4f),
     useFastSP(false), lookAheadFP(false), initCompact(0), compact(0), 
     verb("1 1 1"), packleft(true), packbot(true), scaleTerms(true),
     shrinkToSize(-1.f), noRotation(false)
{
   setSeed();
}

Command_Line::Command_Line (int argc, const char *argv[])
   : getSeed(0),
     budgetTime(0), softBlocks(0), initQP(0),
     inFileName(""), outPlFile(""), capoPlFile(""),
     capoBaseFile(""), baseFile(""), FPrep("Best"),
     seed(0), iterations(0), maxIterHier(10),
     seconds(0.0f), plot(0), savePl(0), saveCapoPl(0), saveCapo(0), 
     save(0), takePl(0), solveMulti(0), clusterPhysical(0), 
     solveTop(0), maxWSHier(15), usePhyLocHier(0),
     dontClusterMacros(0), maxTopLevelNodes(-9999), 
     timeInit(30000.0f), timeCool(0.01f), 
     reqdAR(-9999.0f), maxWS(15.0f), minWL(0),
#ifdef USEFLUTE
     useSteiner(false), printSteiner(false),
#endif
     areaWeight(0.4f), wireWeight(0.4f), useFastSP(false), 
     lookAheadFP(false), initCompact(0), compact(0),
     verb(argc,argv), packleft(true), packbot(true), scaleTerms(true),
     shrinkToSize(-1.f), noRotation(false)
{
   StringParam argInfile ("f", argc, argv);
   StringParam plOutFile ("savePl", argc, argv);
   StringParam saveCapoPlFile ("saveCapoPl", argc, argv);
   StringParam saveCapoFile ("saveCapo", argc, argv);
   StringParam saveFile ("save", argc, argv);

   StringParam FPrep_("FPrep", argc, argv);

   BoolParam help1 ("h", argc, argv);
   BoolParam help2 ("help", argc, argv);
   NoParams  noParams(argc,argv);  // this acts as a flag
   UnsignedParam  fixSeed ("s",argc,argv);
   IntParam  numberOfRuns("n",argc,argv);	
   DoubleParam timeReq("t",argc,argv);
   IntParam  maxIterHier_("maxIterHier",argc,argv);	

   DoubleParam timeInit_("timeInit",argc,argv);
   DoubleParam timeCool_("timeCool",argc,argv);
   DoubleParam startTime_("startTime",argc,argv);
   DoubleParam reqdAR_("AR",argc,argv);
   DoubleParam maxWS_("maxWS",argc,argv);
   BoolParam   minWL_("minWL",argc,argv);
   DoubleParam wireWeight_("wireWeight",argc,argv);
   DoubleParam areaWeight_("areaWeight",argc,argv);
   BoolParam softBlocks_("soft", argc, argv);
   BoolParam initQP_("initQP", argc, argv);
   BoolParam fastSP_("fastSP", argc, argv);

   BoolParam plot_("plot",argc,argv);
   BoolParam plotNoNets_("plotNoNets", argc, argv);
   BoolParam plotNoSlacks_("plotNoSlacks", argc, argv);
   BoolParam plotNoNames_("plotNoNames",argc,argv);
   BoolParam takePl_("takePl",argc,argv);
   BoolParam solveMulti_("solveMulti",argc,argv);
   BoolParam clusterPhysical_("clusterPhysical",argc,argv);
   BoolParam solveTop_("solveTop",argc,argv);
   DoubleParam maxWSHier_("maxWSHier",argc,argv);
   BoolParam usePhyLocHier_("usePhyLocHier",argc,argv);
   BoolParam dontClusterMacros_("dontClusterMacros",argc,argv);
   IntParam maxTopLevelNodes_("maxTopLevelNodes",argc,argv);
   BoolParam compact_("compact",argc,argv);
   BoolParam initCompact_("initCompact", argc, argv);

   // now set up member vars
   if(argInfile.found())
   {
      inFileName = argInfile;
   }

   if(plOutFile.found())
   {
      outPlFile = plOutFile;
      savePl = true;
   }

   if(saveCapoPlFile.found())
   {
      capoPlFile = saveCapoPlFile;
      saveCapoPl = true;
   }

   if(saveCapoFile.found())
   {
      capoBaseFile = saveCapoFile;
      saveCapo = true;
   }

   if(saveFile.found())
   {
      baseFile = saveFile;
      save = true;
   }

   if (FPrep_.found())
   {
      FPrep = FPrep_;
   }
		
   if (fixSeed.found())
   {
      getSeed = false;
      seed = fixSeed;
   }
   else
      getSeed = true;//get sys time as seed

   if (numberOfRuns.found())
      iterations = numberOfRuns;
   else
      iterations = 1;
	
   if (maxIterHier_.found())
      maxIterHier = maxIterHier_;

   if (timeReq.found())
   {
      budgetTime=true;//limit number of runs
      seconds = timeReq;
   }
   else
      budgetTime=false;

	  
   if(timeInit_.found())
      timeInit = timeInit_;

   if(startTime_.found())
      startTime = startTime_;
   else
      startTime = timeInit;

	  
   if(timeCool_.found())
      timeCool = timeCool_;
	  
   if(reqdAR_.found())
   {
      reqdAR = reqdAR_;   //default -9999 means no fixed outline desired
      scaleTerms = false;
   }
	
   if(maxWS_.found())
      maxWS = maxWS_;

   if(maxWSHier_.found())
      maxWSHier = maxWSHier_;

   if(usePhyLocHier_.found())
      usePhyLocHier = true;

   if(maxTopLevelNodes_.found())
      maxTopLevelNodes = maxTopLevelNodes_;

   if(minWL_.found())
   {
      minWL = true;
   }

   if(areaWeight_.found())
   {
      areaWeight = areaWeight_;
      if(areaWeight > 1 || areaWeight < 0)
      {
         cout<<"areaWeight should be : 0 <= areaWeight <= 1"<<endl;  
         exit(0);
      }
   }

   if(wireWeight_.found())
   {
      wireWeight = wireWeight_;
      if(wireWeight > 1 || wireWeight < 0)
      {
         cout<<"wireWeight should be : 0 <= wireWeight <= 1"<<endl;  
         exit(0);
      }

      if(wireWeight == 0) //turn off minWL if wireWeight is 0
         minWL = false;
   }

   if(takePl_.found())
      takePl = true;

   if(solveMulti_.found())
      solveMulti = true;

   if(clusterPhysical_.found())
      clusterPhysical = true;

   if(solveTop_.found())
      solveTop = true;

   if(dontClusterMacros_.found())
      dontClusterMacros = true;

   if(softBlocks_.found())
      softBlocks = true;
	  
   if(initQP_.found())
      initQP = true;

   if(fastSP_.found())
      useFastSP = true;

   if(compact_.found())
      compact = true;

   if(initCompact_.found())
      initCompact = true;

   if(plot_.found() || plotNoNets_.found() || plotNoSlacks_.found() ||
      plotNoNames_.found())
      plot = true;
	
   plotNoNets = plotNoNets_;
   plotNoSlacks = plotNoSlacks_;
   plotNoNames = plotNoNames_;

   BoolParam noScaleTerms("noScaleTerms", argc, argv);

   if(noScaleTerms.found())
     scaleTerms = false;

   StringParam outline("outline", argc, argv);
   if(outline.found())
   {
     std::stringstream strstream;
     strstream << outline;
     strstream >> nonTrivialOutline;
     scaleTerms = false;     
     reqdAR = nonTrivialOutline.getXSize()/nonTrivialOutline.getYSize();
     maxWS = 0; // this cannot be known at this point
   }

#ifdef USEFLUTE
   BoolParam useSteiner_("useSteiner", argc, argv);
   if(useSteiner_.found())
   {
     minWL = true;
     useSteiner = true;
   }
   BoolParam printSteiner_("printSteiner", argc, argv);
   if(printSteiner_.found())
   {
     printSteiner = true;
   }
#endif

   BoolParam noRotation_("noRotation", argc, argv);
   if(noRotation_.found())
   {
     noRotation = true;
   }

   setSeed();
}

void Command_Line::printHelp(int argc, const char *argv[]) const
{
   cerr<< argv[0] << endl 
       <<"-f filename\n"
       <<"-s int        (give a fixed seed)\n"
       <<"-n int        (determine number of runs. default 1)\n"
       <<"-t float     (set a time limit on the annealing run)\n"
       <<"-FPrep {SeqPair | BTree | Best} (floorplan representation default: Best)\n"
       <<"-save basefilename       (save design in bookshelf format)\n"
       <<"-savePl baseFilename     (save .pl file of solution)\n"
       <<"-saveCapoPl basefilename (save .pl in Capo format)\n"
       <<"-saveCapo basefilename   (save design in Capo format)\n"
       <<"-plot         (plot the output solution to out.plt file)\n"
       <<"-plotNoNets   (plot without the nets)\n"
       <<"-plotNoSlacks (plot without slacks info)\n"
       <<"-plotNoNames  (plot without name of blocks)\n"
       <<"-timeInit float  (initial normalizing time: default 30000)\n"
       <<"-startTime float (annealing initial time: default timeInit)\n"
       <<"-timeCool float  (annealing cool time: default 0.01\n"
       <<"-AR float  (required Aspect Ratio of fixed outline: default no Fixed Outline)\n"
       <<"-maxWS float (maxWS(%) allowed if fixed outline constraints)\n"
       <<"-outline xMin,yMin,xMax,yMax (specify the fixed box to floorplan in instead of AR and maxWS)\n"
       <<"-maxWSHier float (maxWS(%) for each hierarchical block)\n"
       <<"-usePhyLocHier (use physical locs which updating locs of sub-blocks of clustered blocks)\n"
       <<"-maxTopLevelNodes int (max # top level nodes during clustering)\n"
       <<"-maxIterHier int (max # of iterations in hierarchical mode to satisfy fixed-outline)\n"
       <<"-minWL        (minimize WL default turned off)\n"
       <<"-noRotation   (disable rotation of all blocks default turned off)\n"
#ifdef USEFLUTE
       <<"-useSteiner   (minimize Steiner WL default turned off)\n"
       <<"-printSteiner (print Steiner WL in run summary)\n"
#endif
       <<"-wireWeight float  (default 0.4)\n"
       <<"-areaWeight float  (default 0.4)\n"
       <<"-soft         (soft Blocks present in input default no)\n"
       <<"-initQP       (start the annealing with a QP solution)\n"
       <<"-fastSP       (use O(nlog n) algo for sequence pair evaluation)\n"
       <<"-takePl       (take a placement and convert to sequence pair for use as initial solution)\n"
       <<"-solveMulti   (solve as multiLevel heirarchy)\n"
       <<"-clusterPhysical (use Physical Heirarchy)\n"
       <<"-dontClusterMacros (keep Macros out of Clustering)\n"
       <<"-solveTop     (solve only top level of heirarchy)\n"
       <<"-compact      (compact the final solution)\n"
       <<"-initCompact  (construct initial SP by compaction)\n"
       <<"-snapToGrid   (snap to row and site grid)\n"
       <<endl;
}

void Command_Line::printAnnealerParams() const
{
   cout << "Annealer Parameters: " << endl;
   cout << "   normalizing Time:  " << timeInit << endl;
   cout << "   start Time:        " << startTime << endl;
   cout << "   cooling Time:      " << timeCool << endl;
   cout << "   fixed-outline?     "
        << ((reqdAR != BaseAnnealer::FREE_OUTLINE)? "Yes" : "No") << endl;

   if (reqdAR != BaseAnnealer::FREE_OUTLINE)
   {
      cout << "    -reqd Aspect Ratio:  " << reqdAR << endl;
      cout << "    -maximum WS:         " << maxWS << "%" << endl;
   }      
      
   cout << "   minimize WL?       " << ((minWL)? "Yes" : "No") << endl;
   if (minWL)
   {
#ifdef USEFLUTE
      cout << "    minimize Steiner WL?  " << ((useSteiner)? "Yes" : "No") << endl;
#endif
      cout << "    -wireWeight =  " << wireWeight << endl;
      cout << "    -areaWeight =  " << areaWeight << endl;
   }

   cout << "   scale terminals?        " << ((scaleTerms)? "Yes" : "No") << endl;
   cout << "   soft blks exist?        " << ((softBlocks)? "Yes" : "No") << endl;
   cout << "   use initial placement?  " << ((takePl)? "Yes" : "No") << endl;
   cout << "   compact init plmt?      " << ((initCompact)? "Yes" : "No") << endl;
   cout << "   compact final plmt?     " << ((compact)? "Yes" : "No") << endl;
   cout << "   use quad-pl as initial? " << ((initQP)? "Yes" : "No") << endl;
   cout << endl;
}

void Command_Line::printAnnealerParamsClassic() const
{
   cout << "Annealer Params: "<<endl;
   cout << "\tnormalizing Time  " << timeInit << endl;
   cout << "\tstart Time        " << startTime << endl;
   cout << "\tcooling Time      " << timeCool << endl;
   cout<<"\treqd Aspect Ratio "<<reqdAR<<" (-9999 means no fixed shape)"<<endl;
   cout<<"\tminimize WL       "<<minWL;
   if(minWL == 1)
   {
#ifdef USEFLUTE
      cout<<"   :  minimizeSteiner WL  "<<useSteiner; 
#endif
      cout<<"   :  wireWeight = "<<wireWeight;
   }
   cout<<endl;
   cout<<"\tmaximum WS        "<<maxWS<<"% (only for fixed-outline)"<<endl<<endl;
}

void Command_Line::setSeed()
{
   int rseed;
   if(getSeed)
      rseed = int(time((time_t *)NULL));
   else
      rseed = seed;
  
   srand(rseed);        //seed for rand function
   srand48(rseed);      //seed for random_shuffle function
//   if(verb.forMajStats > 0)
//      cout<<"The random seed for this run is: "<<rseed<<endl;
}

