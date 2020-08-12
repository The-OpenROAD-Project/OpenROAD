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
#include "FPcommon.h"
#include "Annealer.h"
#include "CommandLine.h"
#include "ABKCommon/abkcommon.h"
#include "ABKCommon/infolines.h"
#include "ABKCommon/abkmessagebuf.h"
#ifdef USEFLUTE
#include "Flute/flute.h"
#endif

using namespace parquetfp;
using std::cout;
using std::cerr;
using std::endl;

int main(int argc, const char *argv[])
{
           float currXSize;
	   float currYSize;
	   float currArea = -1e100;
	   float lastArea;
	   float currWS;
	   float currWL;
	   float currWLnoWts;
	   float currAR=0;
#ifdef USEFLUTE
           float currSteinerWL;
           readLUT();
#endif

           cout<<getABKMessageBuf()<<endl;

	   BoolParam help1 ("h", argc, argv);
	   BoolParam help2 ("help", argc, argv);
	   NoParams  noParams(argc,argv);  // this acts as a flag
	   Command_Line* params = new Command_Line(argc, argv);
	   params->printAnnealerParams();
	   if (noParams.found() || help1.found() || help2.found())
	     {
	       cerr<<"This test case test different functionalities\n"
		   <<"Options \n"
		   <<"FPEvalTest2.exe\n"
		   <<"-f baseFileName\n"
		   <<"-takePl   (construct SP from existing floorplan)\n"
		   <<"-initCompact (compact the initial solution)\n"
		   <<"-soft     (soft blocks in design. pack.)\n"
		   <<"-plot     (plot floorplan)\n"
		   <<"-save     (save in bookshelf floorplan format)\n"
		   <<"-savePl   (save only .pl file)\n"
		   <<"-saveCapoPl (save .pl in Capo bookshelf format)\n"
		   <<"-saveCapo  (save design in Capo bookshelf format)\n";
	       exit (0);
	     }	
	
	   DB* db = new DB(params->inFileName);
	   float blocksArea = db->getNodesArea();
           MaxMem maxMem;
	   
	   Annealer annealer(params, db, &maxMem);
	   if(params->takePl)
	     {
	       //convert placement to sequence pair
	       annealer.takeSPfromDB();
	     }

	   Timer T;
	   T.stop();
	   float totalTime=0;
           T.start(0.0);
	   for(unsigned iter=0; iter<10; ++iter)
	     annealer.eval();
           T.stop();
           totalTime += T.getUserTime();

	   currArea = annealer.getXSize()*annealer.getYSize();
	   annealer.evalSlacks();

	   if(params->initCompact)
	     {
	       bool whichDir = 0;
	       annealer.evalCompact(whichDir);
	       do
		 {
		   whichDir = !whichDir;
		   lastArea = currArea;
		   annealer.takeSPfromDB();
		   annealer.evalCompact(whichDir);
		   currArea = annealer.getXSize()*annealer.getYSize();
		   cout<<currArea<<"\t"<<lastArea<<endl;
		 }
	       while(int(currArea) < int(lastArea));
	     }

	   if(params->softBlocks)
	     {
	       annealer.packSoftBlocks(100);
	       annealer.updatePlacement();	       
	     }

	   currXSize = annealer.getXSize();
	   currYSize = annealer.getYSize();
	   currArea = currXSize*currYSize;
	   currWS = 100*(currArea - blocksArea)/blocksArea;
           bool useWts = true;
#ifdef USEFLUTE
           bool useSteiner = true;
           currWL = db->evalHPWL(useWts,params->scaleTerms,!useSteiner);
           currWLnoWts = db->evalHPWL(!useWts,params->scaleTerms,!useSteiner);
           currSteinerWL = db->evalHPWL(!useWts,params->scaleTerms,useSteiner);
#else
           currWL = db->evalHPWL(useWts,params->scaleTerms);
           currWLnoWts = db->evalHPWL(!useWts,params->scaleTerms);
#endif
	   currAR = currXSize/currYSize;

	   if(params->plot)
	    {
	      currAR = currXSize/currYSize;
              bool plotSlacks = !params->plotNoSlacks;
              bool plotNets = !params->plotNoNets;
	      bool plotNames = !params->plotNoNames;
              db->plot("out.plt", currArea, currWS, currAR, 0, 
	      		currWL, plotSlacks, plotNets, plotNames);
	    }
	    
	   if(params->save)
	     db->save(params->baseFile.c_str());

           if(params->savePl)
	     db->getNodes()->savePl(params->outPlFile.c_str());
			      
	   if(params->saveCapoPl)
	     db->getNodes()->saveCapoPl(params->capoPlFile.c_str());

	   if(params->saveCapo)
	     db->saveCapo(params->capoBaseFile.c_str(), params->nonTrivialOutline, params->reqdAR);
	     
           cout<<"Final Area: "<<currArea<<" WhiteSpace "<<currWS<<"%"
	       <<" currAR "<<currAR<<" HPWL "<<currWL<<" Unweighted HPWL "<<currWLnoWts
#ifdef USEFLUTE
               <<" Steiner WL "<<currSteinerWL
#endif
               <<endl;
	   cout<<"Time Taken  "<<totalTime<<endl;
	  
	return 0;
}
