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
#include "ClusterDB.h"
#include "SolveMulti.h"
#include "ABKCommon/abkcommon.h"
#include "ABKCommon/infolines.h"
#include "ABKCommon/abkmessagebuf.h"
#ifdef USEFLUTE
#include "Flute/flute.h"
#endif

using namespace parquetfp;
using std::cout;
using std::endl;
using std::cerr;

int main(int argc, const char *argv[])
{
           Timer T;
	   T.stop();
   	   float currArea, lastArea;
	   float currWS;
	   float currWL;
	   float currXSize;
	   float currYSize;
	   float currAR;
#ifdef USEFLUTE
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
	       cerr<<"This test case solves heirarchical floorplans\n"
		   <<"Options \n"
		   <<"FPEvalTest4.exe\n"
		   <<"-f baseFileName\n"
		   <<"-compact  (compact the final multilevel solution)\n"
		   <<"-plot     (plot floorplan)\n"
		   <<"-save     (save in bookshelf floorplan format)\n"
		   <<"-savePl   (save only .pl file)\n"
		   <<"-saveCapoPl (save .pl in Capo bookshelf format)\n"
		   <<"-saveCapo  (save design in Capo bookshelf format)\n";
	       exit (0);
	     }	

	   DB* db = new DB(params->inFileName);
           MaxMem maxMem;
	   
	   T.start(0.0);
	   SolveMulti solveMulti(db, params, &maxMem);
	   solveMulti.go();

	   if(params->compact)
	     {
	       //compact the design
	       Annealer annealer(params, db, &maxMem);
	       bool whichDir = 0;
	       annealer.takeSPfromDB();
	       annealer.eval();
	       currArea = annealer.getXSize()*annealer.getYSize();
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
	   T.stop();

	   float blocksArea = db->getNodesArea();

	   currXSize = 0;
	   currYSize = 1;
	   currArea = db->evalArea();
	   currWS = 100*(currArea - blocksArea)/blocksArea;
           bool useWts = true;
#ifdef USEFLUTE
           currWL = db->evalHPWL(useWts,params->scaleTerms,params->useSteiner);
#else
           currWL = db->evalHPWL(useWts,params->scaleTerms);
#endif
	   currAR = currXSize/currYSize;

	   if(params->plot)
	     {
	       bool plotSlacks = !params->plotNoSlacks;
	       bool plotNets = !params->plotNoNets;
	       bool plotNames = !params->plotNoNames;
	       
	       db->plot("out.plt", currArea, currWS, currAR, 
			T.getUserTime(), currWL, plotSlacks, plotNets, 
			plotNames);
	     }
           if(params->savePl)
	     db->getNodes()->savePl(params->outPlFile.c_str());
			      
	   if(params->saveCapoPl)
	     db->getNodes()->saveCapoPl(params->capoPlFile.c_str());

	   if(params->saveCapo)
	     db->saveCapo(params->capoBaseFile.c_str(), params->nonTrivialOutline, params->reqdAR);

	   if(params->save)
	     db->save(params->baseFile.c_str());

	return 0;
}
