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
using std::endl;
using std::cerr;

int main(int argc, const char *argv[])
{
           float currXSize;
	   float currYSize;
	   float currArea;
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
	       cerr<<"This test case test analytical techniques\n"
		   <<"Options \n"
		   <<"FPEvalTest3.exe\n"
		   <<"-f baseFileName\n"
		   <<"-initQP   (initial SP is constructed from quadratic min WL)\n"
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
	   annealer.solveQP();
	   if(params->initQP)
	    {
	     annealer.takeSPfromDB();
	     annealer.eval();
	     
	     currXSize = annealer.getXSize();
	     currYSize = annealer.getYSize();
	     currArea = currXSize*currYSize;
	    }
	   else
	    {
              currXSize = db->getXSize();
	      currYSize = db->getYSize();
	      currArea = db->evalArea();
	    }
						       
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

	   if(params->plot)
	    {
	      currAR = currXSize/currYSize;
              bool plotSlacks = !params->plotNoSlacks;
              bool plotNets = !params->plotNoNets;
	      bool plotNames = !params->plotNoNames;
              db->plot("out.plt", currArea, currWS, currAR, 0, 
	      		currWL, plotSlacks, plotNets, plotNames);
	    }
	    
           if(params->savePl)
	     db->getNodes()->savePl(params->outPlFile.c_str());
			      
	   if(params->saveCapoPl)
	     db->getNodes()->saveCapoPl(params->capoPlFile.c_str());

	   if(params->saveCapo)
	     db->saveCapo(params->capoBaseFile.c_str(), params->nonTrivialOutline, params->reqdAR);

	   if(params->save)
	     db->save(params->baseFile.c_str());

           cout<<"Final Area: "<<currArea<<" WhiteSpace "<<currWS<<"%"
	       <<" currAR "<<currAR<<" HPWL "<<currWL<<" Unweighted HPWL "<<currWLnoWts
#ifdef USEFLUTE
               <<" Steiner WL "<<currSteinerWL
#endif
               <<endl;
	  
	return 0;
}
