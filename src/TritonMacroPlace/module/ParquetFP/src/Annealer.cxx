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


#include "ABKCommon/infolines.h"
#include "FPcommon.h"
#include "PlToSP.h"
#include "Annealer.h"
#include "CommandLine.h"
#include <deque>
#include <cmath>

using namespace parquetfp;
using std::numeric_limits;
using std::cout;
using std::endl;
using std::vector;

#ifndef expf
#define expf exp
#endif

#define VANILLA

Annealer::Annealer(const Command_Line *const params,
                   DB *const db, MaxMem *maxMem)
   : BaseAnnealer(params, db),
     _sp(new SeqPair(_db->getNumNodes())),
     _spEval(new SPeval(_db->getNodes()->getNodeHeights(),
                        _db->getNodes()->getNodeWidths(),
                        _params->useFastSP)),
     _maxMem(maxMem)
{}

Annealer::~Annealer()
{
   if(_sp != NULL)  delete _sp;
   if(_spEval != NULL) delete _spEval;
}

void Annealer::compactSoln(bool minWL, bool fixedOutline, float reqdH, float reqdW)
{
   vector<float> lastXLocs, lastYLocs;

   float lastArea = numeric_limits<float>::max();
   float lastHPWL = numeric_limits<float>::max();
   float currArea = numeric_limits<float>::max();
   float currHPWL = numeric_limits<float>::max();
   bool useWts = true;
   bool whichDir = false;
   bool continueCompaction = false;
   bool lastFitsOutline = false;
   bool currFitsOutline = false;

   eval();

   if(minWL)
   {
#ifdef USEFLUTE
     currHPWL = _db->evalHPWL(useWts,_params->scaleTerms,_params->useSteiner);
#else
     currHPWL = _db->evalHPWL(useWts,_params->scaleTerms);
#endif
   }
   if(fixedOutline)
   {
     currFitsOutline = lessOrEqualFloat(getXSize(),reqdW) &&
                       lessOrEqualFloat(getYSize(),reqdH);
   }
   currArea = getXSize() * getYSize();

   do
   {
     lastArea = currArea;
     lastHPWL = currHPWL;
     lastFitsOutline = currFitsOutline;
     lastXLocs = _db->getXLocs();
     lastYLocs = _db->getYLocs();

     _spEval->evaluateCompact(_sp->getX(), _sp->getY(), whichDir, _params->packleft, _params->packbot);
     _db->updatePlacement(_spEval->xloc, _spEval->yloc);

     whichDir = !whichDir;

     if(minWL)
     {
#ifdef USEFLUTE
       currHPWL = _db->evalHPWL(useWts,_params->scaleTerms,_params->useSteiner);
#else
       currHPWL = _db->evalHPWL(useWts,_params->scaleTerms);
#endif
     }
     if(fixedOutline)
     {
       currFitsOutline = lessOrEqualFloat(getXSize(),reqdW) &&
                         lessOrEqualFloat(getYSize(),reqdH);
     }
     currArea = getXSize() * getYSize();

     if(fixedOutline)
     {
       if(lastFitsOutline)
       {
         if(currFitsOutline)
         {
           if(minWL)
           {
             continueCompaction = lessThanFloat(currHPWL,lastHPWL) &&
                                  lessThanFloat(currArea,lastArea);
           }
           else
           {
             continueCompaction = lessThanFloat(currArea,lastArea);
           }
         }
         else
         {
           continueCompaction = false;
         }
       }
       else
       {
         if(currFitsOutline)
         {
           continueCompaction = true;
         }
         else
         {
           if(minWL)
           {
             continueCompaction = lessThanFloat(currHPWL,lastHPWL) &&
                                  lessThanFloat(currArea,lastArea);
           }
           else
           {
             continueCompaction = lessThanFloat(currArea,lastArea);
           }
         }
       }
     }
     else
     {
       if(minWL)
       {
         continueCompaction = lessThanFloat(currHPWL,lastHPWL) &&
                              lessThanFloat(currArea,lastArea);
       }
       else
       {
         continueCompaction = lessThanFloat(currArea,lastArea);
       }
     }

     if(continueCompaction)
     {
       if(_params->verb.getForMajStats() > 0)
       {
         if(minWL)
         {
           cout << "HPWL: " << lastHPWL << " -> " << currHPWL << " ";
         }
         cout << "area: " << lastArea << " -> " << currArea << endl;
       }
     }
     else
     {
       _db->updatePlacement(lastXLocs, lastYLocs);
     }

     takeSPfromDB();
   
   } while(continueCompaction);
}

void Annealer::takePlfromDB()
{
   takeSPfromDB();
   eval();
}

void Annealer::takeSPfromDB()  //converts the present placement to a sequence pair
{
   vector<float> heights = _db->getNodeHeights();
   vector<float> widths = _db->getNodeWidths();
   vector<float> xloc = _db->getXLocs();
   vector<float> yloc = _db->getYLocs();

   PL2SP_ALGO useAlgo = TCG_ALGO;
   Pl2SP pl2sp(xloc, yloc, widths, heights, useAlgo);
   _sp->putX(pl2sp.getXSP());
   _sp->putY(pl2sp.getYSP());
   _spEval->changeWidths(widths);
   _spEval->changeHeights(heights);
}

bool Annealer::go()
{
   _maxMem->update("Parquet before annealing");

   Timer T;

   //turn off rotation for hard square macros
   //with pins in the center
   Nodes* theNodes = _db->getNodes();
   unsigned numNodes = _db->getNumNodes();
   for(unsigned i = 0; i < numNodes; ++i)
   {
      Node &node = theNodes->getNode(i);
      if(_params->noRotation ||
         (!node.isOrientFixed() && node.isHard() &&
          equalFloat(node.getminAR(), 1.f) && node.allPinsAtCenter))
      {
        node.putIsOrientFixed(true);
      }
   }

   anneal();
   T.stop();

   annealTime += T.getUserTime();

   _spEval->evaluate(_sp->getX(), _sp->getY(), _params->packleft, _params->packbot);
   _db->updatePlacement(_spEval->xloc, _spEval->yloc);

   if(_params->softBlocks)
   {
      //cout<<"Before area is "<<_spEval->xSize*_spEval->ySize<<endl;
      packSoftBlocks(100);
      _spEval->evaluate(_sp->getX(), _sp->getY(), _params->packleft, _params->packbot);
      _db->updatePlacement(_spEval->xloc, _spEval->yloc);
   }
   _spEval->evalSlacks(_sp->getX(), _sp->getY());
   
   /*
   int blocknum = _sp->getX().size();
   vector<float> xSlacks(blocknum); // absolute x-slacks
   vector<float> ySlacks(blocknum); // absolute y-slacks
   for (int i = 0; i < blocknum; i++)
   {
      xSlacks[i] = (_spEval->xSlacks[i]/100) * _spEval->xSize;
      ySlacks[i] = (_spEval->ySlacks[i]/100) * _spEval->ySize;
   }      
   _db->updateSlacks(xSlacks, ySlacks);
   */
   _db->updateSlacks(_spEval->xSlacks, _spEval->ySlacks);
   
   SolutionInfo currSoln;
   currSoln.area =  _spEval->xSize*_spEval->ySize;
   currSoln.width = _spEval->xSize;
   currSoln.height = _spEval->ySize;
   bool useWts = true;
#ifdef USEFLUTE
   currSoln.HPWL = _db->evalHPWL(useWts,_params->scaleTerms,_params->useSteiner);
#else
   currSoln.HPWL = _db->evalHPWL(useWts,_params->scaleTerms);
#endif
   printResults(T, currSoln);
   
   return true;
}

void Annealer::updatePlacement(void)
{
   _db->updatePlacement(_spEval->xloc, _spEval->yloc);
}

void Annealer::eval(void)
{
   _spEval->evaluate(_sp->getX(), _sp->getY(), _params->packleft, _params->packbot);
   _db->updatePlacement(_spEval->xloc, _spEval->yloc);
}

void Annealer::evalSlacks(void)
{
   _spEval->evalSlacks(_sp->getX(), _sp->getY());
   _db->updateSlacks(_spEval->xSlacks, _spEval->ySlacks);
}

void Annealer::evalCompact(bool whichDir)
{
   _spEval->evaluateCompact(_sp->getX(), _sp->getY(), whichDir, _params->packleft, _params->packbot);
   _db->updatePlacement(_spEval->xloc, _spEval->yloc);
}

void Annealer::anneal()
{ 
   bool budgetTime = _params->budgetTime;
   float seconds = _params->seconds;
  
   vector<unsigned> tempX, tempY;

   bool minWL = _params->minWL;
   float wireWeight = _params->wireWeight;
   float areaWeight = _params->areaWeight;
   float ARWeight = 1 - areaWeight - wireWeight;
   if(ARWeight < 0)
      ARWeight = 0;

   float tempArea = 0;
   float tempHeight = 0;
   float tempWidth = 0;
   float tempAR=0;
   float bestHPWL = numeric_limits<float>::max();
   float bestArea = numeric_limits<float>::max();
   float currArea = numeric_limits<float>::max();
   float deltaArea = 0, deltaAR, delta;
   float blocksArea = _db->getNodesArea();
   float currHeight = 1;
   float currWidth = 1;
   float currTime = _params->startTime;
   float size = _sp->getX().size();
   float alpha = 0, r;
   float reqdAR = _params->reqdAR, currAR;
   int move = 0, count=0, ran;
   unsigned timeChangeCtr = 0;
   unsigned indexOrient = 0, moveSelect, iter, masterMoveSel=0; 
   parquetfp::ORIENT newOrient = N; 
   parquetfp::ORIENT oldOrient = N;
   float oldWidth=numeric_limits<float>::max(),
         oldHeight=numeric_limits<float>::max(),
         newWidth=numeric_limits<float>::max(),
         newHeight=numeric_limits<float>::max();
   bool moveAccepted=0;
 
   const float reqdArea = blocksArea * (1+(_params->maxWS/100.0));
   const float reqdWidth = sqrt(reqdArea*reqdAR);
   const float reqdHeight = reqdWidth/reqdAR;

   bool brokeFromLoop;

   float tempHPWL=std::numeric_limits<float>::max();
   bool useWts = true;
#ifdef USEFLUTE
   float currHPWL=_db->evalHPWL(useWts,_params->scaleTerms,_params->useSteiner);
#else
   float currHPWL=_db->evalHPWL(useWts,_params->scaleTerms);
#endif
   float deltaHPWL=0;
  
   unsigned numNodes = _db->getNumNodes();
   bool updatedBestSoln=false;
   Point dummy;
   dummy.x=0;
   dummy.y=0;
   vector<Point> bestSolnPl(numNodes, dummy);
   vector<parquetfp::ORIENT> bestSolnOrient(numNodes, N);
   vector<float> bestSolnWidth(numNodes, 0);
   vector<float> bestSolnHeight(numNodes, 0);
   vector<unsigned> bestXSP(numNodes, 0);
   vector<unsigned> bestYSP(numNodes, 0);

   Timer looptm;
   looptm.stop();
   float unit=0, total=seconds, percent=1;
   unsigned moves=2000;

   while(currTime>_params->timeCool || budgetTime)
   {
      brokeFromLoop = 0;
      iter=0;
      do
      {
         if (budgetTime)
         {
	    if (count==0)
	    {
               looptm.start(0.0);
	    }
	    else if (count==1000)
	    {  
               looptm.stop();
               unit=looptm.getUserTime()/1000;
               if(unit == 0)
               {
                  unit = 10e-6f;
               }
               seconds-=looptm.getUserTime();
               if(_params->verb.getForMajStats() > 0)
                  cout<<int(seconds/unit)<<" moves left with "
                      <<unit*1000000<<" micro seconds per move."<<endl;
               moves=unsigned(seconds/unit/125.0);//moves every .08 degree
	    }
	    else if (count > 1000)
	    {
               seconds-=unit;
               if (seconds <= 0)
               {
                  brokeFromLoop=1;
                  break;
                  //return;
               }
	    }
         }
         else
         {
            if (count==0)
	    {
               looptm.start(0.0);
	    }
	    else if (count==1000)
	    {  
               looptm.stop();
               unit=looptm.getUserTime()/1000;
               if(unit == 0)
               {
                  unit = 10e-6f;
               }
               if(_params->verb.getForMajStats() > 0)
                  cout << (unit*1000000) << " micro seconds per move." << endl;
	    }
         }
	  
         ++count; 
         ++iter;
	  
         tempX = _sp->getX();
         tempY = _sp->getY();

         //select the types of moves here
         if(_params->softBlocks && currTime < 30)
	    masterMoveSel = rand()%1000;
         moveSelect = rand()%1000;

         currAR = currWidth/currHeight;
         move = -1;

#ifdef VANILLA
         if(_params->softBlocks && masterMoveSel > 990)
         {
	    move = packSoftBlocks(2);
         }
         else if(_params->softBlocks && masterMoveSel > 930)
         {
	    move = makeSoftBlMove(tempX, tempY, indexOrient, newWidth, 
				  newHeight);
         }
         else if(((reqdAR-currAR)/reqdAR > 0.00005 || 
                  (currAR-reqdAR)/reqdAR > 0.00005) && 
                 (timeChangeCtr%4)==0 && reqdAR != -9999)
         {
	    move = makeARMove(tempX, tempY, currAR);
         }
         else if(moveSelect < 150 && minWL == 1 && reqdAR != -9999)
         {
	    move = makeARWLMove(tempX, tempY, currAR);
         }
         else if(moveSelect < 300 && minWL == 1)
         {
	    move = makeHPWLMove(tempX, tempY);
         }
         else if(moveSelect < 500)
         {
	    move = makeMoveOrient(indexOrient, oldOrient, newOrient);
         }
         else
         {
	    move = makeMove(tempX, tempY);
         }
#else
         if(_params->softBlocks && masterMoveSel > 470)
         {
	    move = packSoftBlocks(2);
         }
         else if(_params->softBlocks && masterMoveSel > 400)
         {
	    move = makeSoftBlMove(tempX, tempY, indexOrient, newWidth, 
				  newHeight);
         }
         else if(((reqdAR-currAR)/reqdAR > 0.00007 || 
                  (currAR-reqdAR)/reqdAR >0.00005) 
                 && (timeChangeCtr%2)==0 && reqdAR != -9999)
         {
	    move = makeARMove(tempX, tempY, currAR);
         }
         else if(moveSelect < 150 && minWL == 1 && reqdAR != -9999)
         {
	    move = makeARWLMove(tempX, tempY, currAR);
         }
         else if(moveSelect < 300 && minWL == 1)
         {
	    move = makeHPWLMove(tempX, tempY);
         }
         else if(moveSelect < 400)
         {
	    move = makeMoveSlacks(tempX, tempY);
         }
         else if(moveSelect < 600)
         {
	    move = makeMoveSlacksOrient(tempX, tempY, indexOrient, oldOrient, 
	    	                        newOrient);
         }
         else if(moveSelect < 700)
         {
	    move = makeMoveOrient(indexOrient, oldOrient, newOrient);
         }
         else
         {
	    move = makeMove(tempX, tempY);
         }
#endif

         //for orientation moves
         if(move == 10)
         {
            if(oldOrient%2 != newOrient%2)
            {
               _spEval->changeOrient(indexOrient);
            }

            _db->getNodes()->changeOrient(indexOrient, newOrient, *(_db->getNets()));
         }
         else if(move == 11)  //softBlocks move
         {
            oldHeight = _db->getNodes()->getNodeHeight(indexOrient);
            oldWidth = _db->getNodes()->getNodeWidth(indexOrient);
            _spEval->changeNodeWidth(indexOrient, newWidth);
            _spEval->changeNodeHeight(indexOrient, newHeight);
            _db->getNodes()->putNodeWidth(indexOrient, newWidth);
            _db->getNodes()->putNodeHeight(indexOrient, newHeight);
         }

         _spEval->evaluate(tempX, tempY, _params->packleft, _params->packbot);
         _db->updatePlacement(_spEval->xloc, _spEval->yloc);
         tempHeight = _spEval->ySize;
         tempWidth = _spEval->xSize;
         tempArea = tempHeight*tempWidth;
         tempAR = tempWidth/tempHeight;

         /*area objective*/
	  
         if(currTime>30)
	    deltaArea=((tempArea-currArea)*1.2*_params->timeInit)/blocksArea;
         else
	    deltaArea=((tempArea-currArea)*1.5*_params->timeInit)/blocksArea;
	    
         /* x-viol + y-viol objective
            deltaArea=0.5*1.3*_params->timeInit*((tempHeight-currHeight)/(reqdHeight)+(tempWidth-currWidth)/(reqdWidth));
         */

         /*max(x-viol, y-viol) objective
	   if((tempHeight-currHeight)>(tempWidth-currWidth))
           deltaArea=((tempHeight-currHeight)*1.3*_params->timeInit)/(reqdHeight);
	   else
           deltaArea=((tempWidth-currWidth)*1.3*_params->timeInit)/(reqdWidth);
         */
	  
         delta = deltaArea;

         if(minWL)
         {
#ifdef USEFLUTE
            tempHPWL = _db->evalHPWL(useWts,_params->scaleTerms,_params->useSteiner);
#else
            tempHPWL = _db->evalHPWL(useWts,_params->scaleTerms);
#endif

            if(currHPWL == 0)
               deltaHPWL = 0;
            else
             {
               if(currTime>30)
                 deltaHPWL=((tempHPWL-currHPWL)*1.2*_params->timeInit)/currHPWL;
               else
                 deltaHPWL=((tempHPWL-currHPWL)*1.5*_params->timeInit)/currHPWL;
             }
         }

         if(reqdAR != -9999)
         {
            if(currTime>30)
               deltaAR = ((tempAR - reqdAR)*(tempAR - reqdAR) - (currAR - reqdAR)*(currAR - reqdAR))*10*_params->timeInit;
            else
               deltaAR = ((tempAR - reqdAR)*(tempAR - reqdAR) - (currAR - reqdAR)*(currAR - reqdAR))*10*_params->timeInit;

            //deltaAR = 0;
            if(minWL)
               delta = areaWeight*deltaArea + wireWeight*deltaHPWL +
		  ARWeight*deltaAR;
            else
               delta = (areaWeight+wireWeight*0.5)*deltaArea + 
		  (ARWeight+wireWeight*0.5)*deltaAR;
         }
         else if(minWL)
         {
            delta = (areaWeight+ARWeight*0.5)*deltaArea + 
               (wireWeight+ARWeight*0.5)*deltaHPWL;
         }
         else
	    delta = deltaArea;

         if(delta<0 || move == -1)
	    moveAccepted = 1;
         else if(currTime>_params->timeCool) 
	    //become greedy below time>timeCool
         {
            ran=rand()%10000;
            r=static_cast<float>(ran)/9999.0f;
            if(lessThanFloat(r,expf(-delta/currTime)))
               moveAccepted = 1;
            else
               moveAccepted = 0;
         }
         else
	    moveAccepted = 0;

         if(moveAccepted)
         {
            currHeight=tempHeight;
            currWidth=tempWidth;
            currArea=tempArea;
            currHPWL=tempHPWL;
            _sp->putX(tempX);
            _sp->putY(tempY);
         }
         else
         {
            if(move == 10) //if move not accepted then put back orient
            {
               if(oldOrient%2 != newOrient%2)
               {
                  _spEval->changeOrient(indexOrient);
               }

               _db->getNodes()->changeOrient(indexOrient, oldOrient, *(_db->getNets())); 
            }
            else if(move == 11) //if move not accepted then put back old HW's
            {
               _spEval->changeNodeWidth(indexOrient, oldWidth);
               _spEval->changeNodeHeight(indexOrient, oldHeight);
               _db->getNodes()->putNodeWidth(indexOrient, oldWidth);
               _db->getNodes()->putNodeHeight(indexOrient, oldHeight);
            }
         }

         bool saveSolnInBestCopy=false;
         if(moveAccepted)
         {
            float oldCurrArea = currArea;
            float oldCurrWidth = currWidth;
            float oldCurrHeight = currHeight;
	      
            if(reqdAR != -9999)
            {
               if(_params->solveTop && _params->dontClusterMacros)
               {
                  float tempXSize = _db->getXMaxWMacroOnly();
                  float tempYSize = _db->getYMaxWMacroOnly();
                  if(tempXSize > 1e-5 && tempYSize > 1e-5)
                  {
                     currArea = tempXSize*tempYSize;
                     currHeight = tempYSize;
                     currWidth = tempXSize;
                  }
               }

               if(minWL)
               {
                   if(currArea <= reqdArea && currHeight <= reqdHeight && 
                      currWidth <= reqdWidth && (bestHPWL - currHPWL) > 1e-2)
                   {
                       bestHPWL = currHPWL;
                       bestArea = currArea;
                       updatedBestSoln = true;
                       saveSolnInBestCopy = true;
                   }
               }
               else
               {
                   if(currArea <= reqdArea && currHeight <= reqdHeight && 
                           currWidth <= reqdWidth && bestArea > currArea)
                   {
                       bestHPWL = currHPWL;
                       bestArea = currArea;
                       updatedBestSoln = true;
                       saveSolnInBestCopy = true;
                   }
               }
            }
            else
            {
               if(minWL)
               {
                   if(currArea <= bestArea && (bestHPWL - currHPWL) > 1e-2)
                   {
                       bestHPWL = currHPWL;
                       bestArea = currArea;
                       updatedBestSoln = true;
                       saveSolnInBestCopy = true;
                   }
               }
               else
               {
                   if(currArea <= bestArea)
                   {
                       bestHPWL = currHPWL;
                       bestArea = currArea;
                       updatedBestSoln = true;
                       saveSolnInBestCopy = true;
                   }
               }
            }
            if(saveSolnInBestCopy)
            {
               itNode node;
               unsigned nodeId=0;
               for(node = _db->getNodes()->nodesBegin(); node != _db->getNodes()->nodesEnd(); ++node)
               {
                  bestSolnPl[nodeId].x = node->getX();
                  bestSolnPl[nodeId].y = node->getY();
                  bestSolnOrient[nodeId] = node->getOrient();
                  bestSolnWidth[nodeId] = node->getWidth();
                  bestSolnHeight[nodeId] = node->getHeight();
                  bestXSP[nodeId] = (_sp->getX())[nodeId]; 
                  bestYSP[nodeId] = (_sp->getY())[nodeId];
                  ++nodeId;
               }
            }
	  
            if(reqdAR != -9999 && _params->solveTop && _params->dontClusterMacros)
            {
               currArea = oldCurrArea;
               currWidth = oldCurrWidth;
               currHeight = oldCurrHeight;
            }
         }

         if (iter>=moves && budgetTime) break;
      }
      while(iter<size*4 || budgetTime);

      ++timeChangeCtr;

      if (budgetTime)
      {
         percent=seconds/total;
	
         if(percent<.066666 && percent>.033333)
            alpha=0.9f;
         else if(percent<.033333 && percent>.016666)
            alpha=0.95f;
         else if(percent<.016666 && percent>.006666)
            alpha=0.96f;
         else if(percent<.006666 && percent>.000333)
            alpha=0.8f;
         else if(percent<.000333 && percent>.000003)
            alpha=0.98f;
         else
            alpha=0.85f;
      }
      else
      {
         if(currTime<2000 && currTime>1000)
            alpha=0.95f;
         else if(currTime<1000 && currTime>500)
            alpha=0.95f;
         else if(currTime<500 && currTime>200)
            alpha=0.96f;
         else if(currTime<200 && currTime>10)
            alpha=0.96f;
         else if(currTime<15 && currTime>0.1)
            alpha=0.98f;
         else
            alpha=0.95f;
      }
      currTime = alpha*currTime;

      if(brokeFromLoop == 1)
         break;
   }

   if(updatedBestSoln)
   {
      itNode node;
      unsigned nodeId=0;
      for(node = _db->getNodes()->nodesBegin(); node != _db->getNodes()->nodesEnd(); ++node)
      {
         node->putX(bestSolnPl[nodeId].x);
         node->putY(bestSolnPl[nodeId].y);
         node->changeOrient(bestSolnOrient[nodeId], *(_db->getNets()));
         node->putWidth(bestSolnWidth[nodeId]);
         node->putHeight(bestSolnHeight[nodeId]);
         
         _spEval->changeNodeWidth(nodeId, bestSolnWidth[nodeId]);
         _spEval->changeNodeHeight(nodeId, bestSolnHeight[nodeId]);
         ++nodeId;
      }
      // update the sequence-pair here
      _sp->putX(bestXSP);
      _sp->putY(bestYSP);
   }

   if(_params->verb.getForActions() > 0)
      cout << "NumMoves attempted: " << count << endl;
}


void Annealer::sortSlacks(vector<Point>& sortedXSlacks, 
			  vector<Point>& sortedYSlacks)
{
   sortedXSlacks.erase(sortedXSlacks.begin(), sortedXSlacks.end());
   sortedYSlacks.erase(sortedYSlacks.begin(), sortedYSlacks.end());
   Point tempPoint;

   for(unsigned i=0;i<_spEval->xSlacks.size();++i)
   {
      tempPoint.x = _spEval->xSlacks[i];
      tempPoint.y = i;
      sortedXSlacks.push_back(tempPoint);
      tempPoint.x = _spEval->ySlacks[i];
      tempPoint.y = i;
      sortedYSlacks.push_back(tempPoint);
   }
   std::sort(sortedXSlacks.begin(),sortedXSlacks.end(),sort_slacks());
   std::sort(sortedYSlacks.begin(),sortedYSlacks.end(),sort_slacks());
}


int Annealer::makeMove(vector<unsigned>& A, vector<unsigned>& B)
{
   int elem1,elem2,i=0;
   int size = A.size();
   if(size == 1)
      return -1;

   unsigned temporary;
   int moverand = rand()%1000;
   int movedir;
   elem1=rand()%size;
   //ensure that elem2 is different from elem1
   while((elem2=rand()%size)==elem1) ;

   vector<unsigned>::iterator itb;

   moverand=rand()%600;

   if(moverand<75)
   {
      temporary=A[elem1];
      A[elem1]=A[elem2];
      A[elem2]=temporary;
      return 1;
   }
   else if(moverand>75 && moverand<150)
   {
      temporary=B[elem1];
      B[elem1]=B[elem2];
      B[elem2]=temporary;
      return 2;
   }
   else if(moverand>150 && moverand<200)
   {
      temporary=A[elem1];
      A[elem1]=A[elem2];
      A[elem2]=temporary;
      elem1=rand()%size;
      elem2=rand()%size;
      temporary=B[elem1];
      B[elem1]=B[elem2];
      B[elem2]=temporary;
      return 3;
   }
   else if(moverand>200 && moverand<400)
   {
      movedir=rand()%100;
      if(movedir<50)
      {
         i=0;
         for(itb=A.begin();i!=elem1;++itb,++i)
         { }
         temporary=*itb;
         A.erase(itb);
         i=0;
         for(itb=A.begin();i!=elem2;++itb,++i)
         { }
         A.insert(itb,1,temporary);
      }
      else
      {
         i=0;
         for(itb=B.begin();i!=elem1;++itb,++i)
         { }
         temporary=*itb;
         B.erase(itb);
         i=0;
         for(itb=B.begin();i!=elem2;++itb,++i)
         {
         }
         B.insert(itb,1,temporary);	
      }
      return 4;
   }
   else if(moverand>400 && moverand<600)
   {
      elem2=rand()%(int(ceil(size/4.0)));
      movedir=rand()%100;
      if(movedir<50)
      {
         if((elem1-elem2)<0)
	    elem2=elem1+elem2;
         else
	    elem2=elem1-elem2;
      }
      else
      {
         if((elem1+elem2)>size-1)
	    elem2=elem1-elem2;
         else
	    elem2=elem1+elem2;
      }
      abkfatal(elem2 < size, "bad element to swap with");
      if(moverand<500)
      {
         temporary=A[elem1];
         A[elem1]=A[elem2];
         A[elem2]=temporary;
      }
      else
      {
         temporary=B[elem1];
         B[elem1]=B[elem2];
         B[elem2]=temporary;
      }
      return 5;
   }
   return -1;
}

int Annealer::makeMoveOrient(unsigned& index, parquetfp::ORIENT& oldOrient,
			     parquetfp::ORIENT& newOrient)
{

   index = rand()%_sp->getSize();

   Node& node = _db->getNodes()->getNode(index);
   if(node.isOrientFixed())
     return -1;

   oldOrient = node.getOrient();
   newOrient = parquetfp::ORIENT(rand()%8);   //one of the 8 orientations

   return 10;
}

int Annealer::makeMoveSlacksOrient(vector<unsigned>& A, vector<unsigned>& B,
                                   unsigned& index, parquetfp::ORIENT& oldOrient, parquetfp::ORIENT& newOrient)
{
   unsigned size = A.size();
   _spEval->evalSlacks(A, B);

   sortSlacks(sortedXSlacks, sortedYSlacks);
   unsigned elem1=0;
   unsigned movedir = rand()%100;
   index = elem1;
   if(movedir<50)
   {
      for(unsigned i=0;i<size;++i)
      {
         elem1 = unsigned(sortedXSlacks[i].y);
         if(_db->getNodes()->getNodeWidth(elem1) >
            _db->getNodes()->getNodeHeight(elem1))
	    break;
      }
      index = elem1;
   }
   else
   {
      for(unsigned i=0;i<size;++i)
      {
         elem1 = unsigned(sortedYSlacks[i].y);
         if(_db->getNodes()->getNodeHeight(elem1) >
            _db->getNodes()->getNodeWidth(elem1))
	    break;
      }
      index = elem1;
   }

   Node& node = _db->getNodes()->getNode(index);
   if(node.isOrientFixed())
     return -1;

   oldOrient = node.getOrient();
   unsigned r = rand()%4;
  
   if(oldOrient%2 == 0)
      newOrient = parquetfp::ORIENT(2*r+1);
   else
      newOrient = parquetfp::ORIENT(2*r);
    
   return 10;
}


int Annealer::makeMoveSlacks(vector<unsigned>& A, vector<unsigned>& B)
{
   unsigned size = A.size();
   if(size == 1)
      return -1;

   _spEval->evalSlacks(A, B);

   sortSlacks(sortedXSlacks, sortedYSlacks);

   vector<unsigned>::iterator itb;
   unsigned temporary;
   unsigned elem1, elem2;
   unsigned movedir = rand()%100;
   unsigned choseElem1=rand()%int(ceil(size/5.0));
   unsigned choseElem2=size-rand()%int(ceil(size/5.0))-1;
   if(movedir<50)
   {
      elem1 = unsigned(sortedXSlacks[choseElem1].y);
      for(itb=A.begin();(*itb)!=elem1;++itb)
      { }
      temporary=*itb;
      A.erase(itb);
      
      elem2=unsigned(sortedXSlacks[choseElem2].y);
      for(itb=A.begin();(*itb)!=elem2;++itb)
      { }
      A.insert(itb,1,temporary);
      //for B seqPair
      for(itb=B.begin();(*itb)!=elem1;++itb)
      { }
      temporary=*itb;
      B.erase(itb);
      
      for(itb=B.begin();(*itb)!=elem2;++itb)
      { }
      B.insert(itb,1,temporary);
   }
   else
   {
      elem1 = unsigned(sortedYSlacks[choseElem1].y);
      for(itb=A.begin();(*itb)!=elem1;++itb)
      { }
      temporary=*itb;
      A.erase(itb);
      
      elem2=unsigned(sortedYSlacks[choseElem2].y);
      for(itb=A.begin();(*itb)!=elem2;++itb)
      { }
      A.insert(itb,1,temporary);
      
      //for B seqPair
      for(itb=B.begin();(*itb)!=elem1;++itb)
      { }
      temporary=*itb;
      B.erase(itb);
      
      for(itb=B.begin();(*itb)!=elem2;++itb)
      { }
      B.insert(itb,1,temporary);
   }
   return 6;
}

int Annealer::makeARMove(vector<unsigned>& A, vector<unsigned>& B, 
			 float currAR)
{
   unsigned size = A.size();
   if(size == 1)
      return -1;

   unsigned direction, temp;
   _spEval->evalSlacks(A, B);

   sortSlacks(sortedXSlacks, sortedYSlacks);

   vector<unsigned>::iterator itb;
   unsigned chooseElem1=0;
   unsigned elem1=0;
   unsigned chooseElem2=0;
   unsigned elem2=0;
   unsigned temporary=0;

   bool HVDir=0;
   if(currAR > 1.0*_params->reqdAR)//width is greater than reqd,(a little bias 
      //to increase width for better performance
      HVDir = 0;  // width needs to reduce
   else
      HVDir = 1;  // height needs to reduce

   chooseElem1=rand()%int(ceil(size/5.0));
   chooseElem2=size-rand()%int(ceil(size/5.0))-1;

   if(HVDir == 0)    //horizontal
   { 
      elem1 = unsigned(sortedXSlacks[chooseElem1].y); 
      elem2 = unsigned(sortedXSlacks[chooseElem2].y); 
   }
   else //vertical
   {
      elem1 = unsigned(sortedYSlacks[chooseElem1].y);
      elem2 = unsigned(sortedYSlacks[chooseElem2].y);
   }

   temp = rand() % 2;
   if(HVDir == 0)
   {
      if(temp == 0)
         direction = 0;  //left
      else
         direction = 3;  //right
   }
   else
   {
      if(temp == 0)
         direction = 1;  //top
      else
         direction = 2;  //bottom
   }

   for(itb=A.begin();(*itb)!=unsigned(elem1);++itb)
   { }
   temporary=*itb;
   A.erase(itb);
      
   for(itb=A.begin();(*itb)!=unsigned(elem2);++itb)
   { }
   switch(direction)
   {
   case 0:   //left
   case 1:   //top
      break;
   case 2:   //bottom
   case 3:   //right
      ++itb;
      break;
   }
   A.insert(itb,1,temporary);

   //for B seqPair
   for(itb=B.begin();(*itb)!=unsigned(elem1);++itb)
   { }
   temporary=*itb;
   B.erase(itb);
  
   for(itb=B.begin();(*itb)!=unsigned(elem2);++itb)
   { }
   switch(direction)
   {
   case 0:   //left
   case 2:   //bottom
      break;
   case 1:   //top
   case 3:   //right
      ++itb;
      break;
   }

   B.insert(itb,1,temporary);  

   return 7;
}

int Annealer::makeSoftBlMove(const vector<unsigned>& A,
                             const vector<unsigned>& B,
                             unsigned &index,
                             float &newWidth, float &newHeight)
{
   unsigned size = A.size();
   _spEval->evalSlacks(A, B);

   sortSlacks(sortedXSlacks, sortedYSlacks);
   unsigned elem1=0;
   float minAR, maxAR, currAR;  
   unsigned movedir = rand()%100;
   float maxWidth, maxHeight;
   index = elem1;
   bool brokeFromLoop=0;

   if(movedir<50)
   {
      for(unsigned i=0;i<size;++i)
      {
         elem1 = unsigned(sortedXSlacks[i].y);
         Node& node = _db->getNodes()->getNode(elem1);
         minAR = node.getminAR();
         maxAR = node.getmaxAR();
         currAR = node.getWidth()/node.getHeight();
  
         if(_spEval->ySlacks[elem1] > 0 && minAR != maxAR && currAR > minAR)
         {
            brokeFromLoop = 1;
            break;
         }
      }
      index = elem1;
   }
  
   if(movedir >= 50 || brokeFromLoop == 0)
   {
      for(unsigned i=0;i<size;++i)
      {
         elem1 = unsigned(sortedYSlacks[i].y);
         Node& node = _db->getNodes()->getNode(elem1);
         minAR = node.getminAR();
         maxAR = node.getmaxAR();
         currAR = node.getWidth()/node.getHeight();
	  
         if(_spEval->xSlacks[elem1] > 0 && minAR != maxAR && currAR < maxAR)
	    break;
      }
      index = elem1;
   }

   Node& node = _db->getNodes()->getNode(index);
   float origHeight = node.getHeight();
   float origWidth = node.getWidth();
   currAR = origWidth/origHeight;
   float area = node.getArea();

   if(node.getminAR() > node.getmaxAR())
   {
      minAR = node.getmaxAR();
      maxAR = node.getminAR();
   }
   else
   {
      minAR = node.getminAR();
      maxAR = node.getmaxAR();
   }

   if(node.getOrient()%2 == 0)
   {
      maxHeight = sqrt(area/minAR);
      maxWidth = sqrt(area*maxAR);
   }
   else
   {
      maxHeight = sqrt(area*maxAR);
      maxWidth = sqrt(area/minAR);
   }

   float absSlackX = _spEval->xSlacks[index]*_spEval->xSize/100;
   float absSlackY = _spEval->ySlacks[index]*_spEval->ySize/100;

   newHeight = origHeight;
   newWidth = origWidth;

   if(absSlackX > absSlackY)  //need to elongate in X direction
   {
      if(currAR < maxAR)     //need to satisfy this constraint
      {
         newWidth = origWidth + absSlackX;
         if(newWidth > maxWidth)
	    newWidth = maxWidth;
         newHeight = area/newWidth;
      }
   }
   else                       //need to elongate in Y direction
   {
      if(currAR > minAR)     //need to satisfy this constraint
      {
         newHeight = origHeight + absSlackY;
         if(newHeight > maxHeight)
	    newHeight = maxHeight;
         newWidth = area/newHeight;
      }
   }

   if(minAR == maxAR)
   {
      newHeight = origHeight;
      newWidth = origWidth;
   }

   return 11;
}

int Annealer::makeIndexSoftBlMove(const vector<unsigned>& A,
                                  const vector<unsigned>& B,
                                  unsigned index,
                                  float &newWidth, float &newHeight)
{
   _spEval->evalSlacks(A, B);

   //sortSlacks(sortedXSlacks, sortedYSlacks);
   float minAR, maxAR, currAR;  
   float maxWidth, maxHeight;

   Node& node = _db->getNodes()->getNode(index);
   float origHeight = node.getHeight();
   float origWidth = node.getWidth();
   currAR = origWidth/origHeight;
   float area = node.getArea();
  
   if(node.getminAR() > node.getmaxAR())
   {
      minAR = node.getmaxAR();
      maxAR = node.getminAR();
   }
   else
   {
      minAR = node.getminAR();
      maxAR = node.getmaxAR();
   }
  
   if(node.getOrient()%2 == 0)  
   {
      maxHeight = sqrt(area/minAR);
      maxWidth = sqrt(area*maxAR);
   }
   else
   {
      maxHeight = sqrt(area*maxAR);
      maxWidth = sqrt(area/minAR);
   }

   float absSlackX = _spEval->xSlacks[index]*_spEval->xSize/100;
   float absSlackY = _spEval->ySlacks[index]*_spEval->ySize/100;

   newHeight = origHeight;
   newWidth = origWidth;

   if(absSlackX > absSlackY)  //need to elongate in X direction
   {
      newWidth = origWidth + absSlackX;
      if(newWidth > maxWidth)
         newWidth = maxWidth;
      newHeight = area/newWidth;

   }
   else                       //need to elongate in Y direction
   {
      newHeight = origHeight + absSlackY;
      if(newHeight > maxHeight)
         newHeight = maxHeight;
      newWidth = area/newHeight;
   }

   if(std::abs(minAR-maxAR) < 0.0000001)
   {
      newHeight = origHeight;
      newWidth = origWidth;
   }

   return 11;
}

int Annealer::packSoftBlocks(unsigned numIter)
{
   unsigned index;
   float origHeight, origWidth;
   float newHeight, newWidth;
   float oldArea;
   float newArea;
   float change=-1;

   unsigned size = (_sp->getX()).size();
  
   _spEval->evaluate(_sp->getX(), _sp->getY(), _params->packleft, _params->packbot);
   oldArea = _spEval->ySize * _spEval->xSize;
   unsigned iter = 0;
   while(iter < numIter)
   {
      ++iter;
     
      _spEval->evalSlacks(_sp->getX(), _sp->getY());
      sortSlacks(sortedXSlacks, sortedYSlacks);
     
      for(unsigned i = 0; i<size; ++i)
      {
         if(numIter%2 == 0)
            index = unsigned(sortedXSlacks[i].y);
         else
            index = unsigned(sortedYSlacks[i].y);
      
         origHeight = _db->getNodes()->getNodeHeight(index);
         origWidth = _db->getNodes()->getNodeWidth(index);
	      
         makeIndexSoftBlMove(_sp->getX(), _sp->getY(), index, newWidth, newHeight);
         _spEval->changeNodeWidth(index, newWidth);
         _spEval->changeNodeHeight(index, newHeight);
         _spEval->evaluate(_sp->getX(), _sp->getY(), _params->packleft, _params->packbot);
       
         newArea = _spEval->ySize * _spEval->xSize;
         change = newArea-oldArea;
         if(change > 1)  //do not accept
         {
            _spEval->changeNodeWidth(index, origWidth);
            _spEval->changeNodeHeight(index, origHeight);
         }
         else
         {
            _db->getNodes()->putNodeWidth(index, newWidth);
            _db->getNodes()->putNodeHeight(index, newHeight);
            oldArea = newArea;
         }
      }
   }
   return -1;
}

int Annealer::makeHPWLMove(vector<unsigned>& A, vector<unsigned>& B)
{

   int size = A.size();

   if(size == 1)
      return -1;

   int i, j, temp, direction;
   int elem1,elem2;

   elem1=rand()%size;
   int searchRadiusNum = int(ceil(size/5.0));
   float searchRadius;
   float distance;
   float xDist;
   float yDist;

   vector<unsigned>::iterator itb;
   int temporary;

   vector<bool> seenBlocks;
   seenBlocks.resize(size);

   float unitRadiusSize;

   vector<int> searchBlocks;

   vector<float>& xloc = _spEval->xloc;
   vector<float>& yloc = _spEval->yloc;

  
   //evaluate SP to determine locs, may be redundant
   _spEval->evaluate(A, B, _params->packleft, _params->packbot);
   //can use db locs instead

   if(_spEval->xSize > _spEval->ySize)
      unitRadiusSize = _spEval->xSize;
   else
      unitRadiusSize = _spEval->ySize;

   unitRadiusSize /= sqrt(float(size));

   Point idealLoc = _analSolve->getOptLoc(elem1, _spEval->xloc, _spEval->yloc);
   //get optimum location of elem1

   fill(seenBlocks.begin(), seenBlocks.end(), 0);
   searchRadius = 0;
   for(i=0; i<searchRadiusNum; ++i)
   {
      searchRadius += unitRadiusSize;
      for(j = 0; j<size; ++j)
      {
         if(seenBlocks[j] == 0 && j != elem1)
         {
            xDist = xloc[j]-idealLoc.x;
            yDist = yloc[j]-idealLoc.y;
            distance = sqrt(xDist*xDist + yDist*yDist);
            if(distance < searchRadius)
            {
               seenBlocks[j] = 1;
               searchBlocks.push_back(j);
               if(searchBlocks.size() >= unsigned(searchRadiusNum))
                  break;
               continue;
            }
         }
      }
      if(searchBlocks.size() >= unsigned(searchRadiusNum))
         break;
   }

   if(searchBlocks.size() != 0)
   {
      temp = rand() % searchBlocks.size();
      elem2 = searchBlocks[temp];
   }
   else
   {
      do
         elem2 = rand() % size;
      while(elem2 == elem1);
   }

   if(elem1 == elem2)
      return -1;

   direction = rand() % 4;


   for(itb=A.begin();(*itb)!=unsigned(elem1);++itb)
   { }
   temporary=*itb;
   A.erase(itb);
      
   for(itb=A.begin();(*itb)!=unsigned(elem2);++itb)
   { }
   switch(direction)
   {
   case 0:   //left
   case 1:   //top
      break;
   case 2:   //bottom
   case 3:   //right
      ++itb;
      break;
   }
   A.insert(itb,1,temporary);

   //for B seqPair
   for(itb=B.begin();(*itb)!=unsigned(elem1);++itb)
   { }
   temporary=*itb;
   B.erase(itb);
  
   for(itb=B.begin();(*itb)!=unsigned(elem2);++itb)
   { }
   switch(direction)
   {
   case 0:   //left
   case 2:   //bottom
      break;
   case 1:   //top
   case 3:   //right
      ++itb;
      break;
   }

   B.insert(itb,1,temporary);  
   return 12;

   return -1;
}

int Annealer::makeARWLMove(vector<unsigned>& A, vector<unsigned>& B, 
			   float currAR)
{
   unsigned size = A.size();

   if(size == 1)
      return -1;
  
   vector<unsigned>::iterator itb;
   unsigned chooseElem1=0;
   unsigned elem1=0;
   unsigned elem2=0;
   unsigned temporary=0;
   unsigned i, j, direction, temp;
   float maxSlack;
   unsigned searchRadiusNum = unsigned(ceil(size/5.0));
   float searchRadius;
   float distance;
   float xDist;
   float yDist;

   vector<bool> seenBlocks;
   seenBlocks.resize(size);
   float unitRadiusSize;

   vector<int> searchBlocks;

   vector<float>& xloc = _spEval->xloc;
   vector<float>& yloc = _spEval->yloc;

   _spEval->evalSlacks(A, B);

   sortSlacks(sortedXSlacks, sortedYSlacks);

   //evaluate SP to determine locs, may be redundant
   _spEval->evaluate(A, B, _params->packleft, _params->packbot);
   //can use db locs instead

   bool HVDir=0;
   if(currAR > _params->reqdAR) //width is greater than reqd,(a little bias 
      //to increase width for better performance
      HVDir = 0;  // width needs to reduce
   else
      HVDir = 1;  // height needs to reduce

   chooseElem1=rand()%int(ceil(size/5.0));

   if(HVDir == 0)    //horizontal
      elem1 = unsigned(sortedXSlacks[chooseElem1].y); 
   else //vertical
      elem1 = unsigned(sortedYSlacks[chooseElem1].y);

   if(_spEval->xSize > _spEval->ySize)
      unitRadiusSize = _spEval->xSize;
   else
      unitRadiusSize = _spEval->ySize;
   unitRadiusSize /= sqrt(float(size));

   Point idealLoc = _analSolve->getOptLoc(elem1, xloc, yloc);
   //get optimum location of elem1

   fill(seenBlocks.begin(), seenBlocks.end(), 0);
   searchRadius = 0;
   for(i=0; i<searchRadiusNum; ++i)
   {
      searchRadius += unitRadiusSize;
      for(j = 0; j<size; ++j)
      {
         if(seenBlocks[j] == 0 && j != elem1)
         {
            xDist = xloc[j]-idealLoc.x;
            yDist = yloc[j]-idealLoc.y;
            distance = sqrt(xDist*xDist + yDist*yDist);
            if(distance < searchRadius)
            {
               seenBlocks[j] = 1;
               searchBlocks.push_back(j);
               if(searchBlocks.size() >= unsigned(searchRadiusNum))
                  break;
               continue;
            }
         }
      }
      if(searchBlocks.size() >= unsigned(searchRadiusNum))
         break;
   }
  
   maxSlack = -std::numeric_limits<float>::max();
   if(HVDir == 0)  //width reduction. find max xSlack block
   {
      for(i=0; i<searchBlocks.size(); ++i)
      {
         if(_spEval->xSlacks[searchBlocks[i]] > maxSlack)
         {
            maxSlack = _spEval->xSlacks[searchBlocks[i]];
            elem2 = searchBlocks[i];
         }
      }
   }
   else  //height reduction. find max yslack block
   {
      for(i=0; i<searchBlocks.size(); ++i)
      {
         if(_spEval->ySlacks[searchBlocks[i]] > maxSlack)
         {
            maxSlack = _spEval->ySlacks[searchBlocks[i]];
            elem2 = searchBlocks[i];
         }
      }
   }

   if(searchBlocks.size() == 0)
      do
         elem2 = rand() % size;
      while(elem2 == elem1);

   temp = rand() % 2;
   if(HVDir == 0)
   {
      if(temp == 0)
         direction = 0;  //left
      else
         direction = 3;  //right
   }
   else
   {
      if(temp == 0)
         direction = 1;  //top
      else
         direction = 2;  //bottom
   }

   for(itb=A.begin();(*itb)!=unsigned(elem1);++itb)
   { }
   temporary=*itb;
   A.erase(itb);
      
   for(itb=A.begin();(*itb)!=unsigned(elem2);++itb)
   { }
   switch(direction)
   {
   case 0:   //left
   case 1:   //top
      break;
   case 2:   //bottom
   case 3:   //right
      ++itb;
      break;
   }
   A.insert(itb,1,temporary);

   //for B seqPair
   for(itb=B.begin();(*itb)!=unsigned(elem1);++itb)
   { }
   temporary=*itb;
   B.erase(itb);
  
   for(itb=B.begin();(*itb)!=unsigned(elem2);++itb)
   { }
   switch(direction)
   {
   case 0:   //left
   case 2:   //bottom
      break;
   case 1:   //top
   case 3:   //right
      ++itb;
      break;
   }

   B.insert(itb,1,temporary);  

   return 13;
}


float Annealer::getXSize(void)
{
   return _spEval->xSize;
}


float Annealer::getYSize(void)
{
   return _spEval->ySize;
}
 
