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


#include "ABKCommon/infolines.h"
#include "FPcommon.h"
#include "Annealer.h"
#include "ClusterDB.h"
#include "CommandLine.h"
#include "SolveMulti.h"
#include "mixedpacking.h"
#include "baseannealer.h"
#include "btreeanneal.h"

using namespace parquetfp;
using std::cout;
using std::endl;
using std::numeric_limits;
using std::vector;

SolveMulti::SolveMulti(DB * db, Command_Line* params, MaxMem *maxMem):_newDB(0),_maxMem(maxMem)
{
   _db = db;
   _params = params;
   //_newDB = new DB(_params->minWL);
   clusterTime = 0.0;
   annealTime = 0.0;
}

SolveMulti::~SolveMulti()
{
   if(_newDB) delete _newDB;
}

DB* SolveMulti::clusterOnly() const
{
   ClusterDB multiCluster(_db, _params);

   DB* clusteredDB=0;
   Timer T;
   if(_params->clusterPhysical)
      multiCluster.clusterMultiPhysical(clusteredDB);
   else
      multiCluster.clusterMulti(clusteredDB);
   
   T.stop();
   _maxMem->update("Parquet after Clustering");
   clusterTime +=  T.getUserTime();

   return clusteredDB; // Transfer ownership of the DB
}

void SolveMulti::go(void)
{
   Timer T;

   ClusterDB multiCluster(_db, _params);

   if(_params->clusterPhysical)
      multiCluster.clusterMultiPhysical(_newDB);
   else
      multiCluster.clusterMulti(_newDB);

   T.stop();
   _maxMem->update("Parquet after Clustering");
   clusterTime +=  T.getUserTime();
   
   if(_params->verb.getForSysRes() > 0)
      cout<<"Clustering took "<<clusterTime<<" seconds "<<endl;
 
   if(_params->verb.getForMajStats() > 0)
      cout<<"Num Nodes: "<<_newDB->getNumNodes()<<"  Num Nets: "
          <<_newDB->getNets()->getNumNets()<<"  Num Pins: "
          <<_newDB->getNets()->getNumPins()<<"  Num Obstacles: "
          <<_newDB->getNumObstacles()<<endl;
  
   /*  _newDB->plot("cluster.plt", _newDB->evalArea(), 
       100*(_newDB->evalArea()-_newDB->getNodesArea())/_newDB->getNodesArea(),
       _newDB->getXMax()/_newDB->getYMax(), 0, _newDB->evalHPWL(), 0, 1, 0);
   */
   float reqdWidth = numeric_limits<float>::max();
   float reqdHeight = numeric_limits<float>::max();
   float oldDBArea = _db->getNodesArea();
   float newDBArea = _newDB->getNodesArea();
   float maxArea = (1.f+_params->maxWS/100)*oldDBArea;
   float newMaxWS = (maxArea/newDBArea-1.f)*100.f;
   float oldMaxWS = _params->maxWS;
   float inverseRatio = 1.f;

   if(lessThanFloat(newMaxWS, 100.f*_params->shrinkToSize))
   {
     // we don't have enough whitespace, so lets shrink the
     // hard movable objects to make more room

     float targetDBArea = maxArea/(1.f+ _params->shrinkToSize);
     float ratio = targetDBArea/newDBArea;
     inverseRatio = newDBArea/targetDBArea;

     _newDB->resizeHardBlocks(ratio);

     newDBArea = _newDB->getNodesArea();
     newMaxWS = (maxArea/newDBArea-1.f)*100.f;
   }
   else
   {
     _params->shrinkToSize = -1.f;
   }

   _params->maxWS = newMaxWS;
   bool fixedOutline = _params->reqdAR != -9999;
   if(fixedOutline && _params->verb.getForMajStats() > 0)
      cout<<"newMaxWS is "<<_params->maxWS<<endl;

   if(fixedOutline)
   {
      reqdHeight = sqrt(maxArea/_params->reqdAR);
      reqdWidth = maxArea/reqdHeight;
   }
   float currXSize, currYSize;
   int maxIter = 0;

   if (_db->getNumObstacles() > 0 && _params->FPrep != "BTree") {
      if(_params->verb.getForMajStats() > 0)
         cout<<"Obstacles detected. Changing annealer to B*Tree"<<endl;
      _params->FPrep = "BTree";
   }

   BaseAnnealer *annealer = NULL;
   if (_params->FPrep == "BTree")
   {
      annealer =
         new BTreeAreaWireAnnealer(_params, _newDB);
   }
   else if (_params->FPrep == "SeqPair")
   {
      annealer = new Annealer(_params, _newDB, _maxMem);
   }
   else if (_params->FPrep == "Best")
   {
      if(_newDB->getNumNodes() < 100 && _params->maxWS > 10.)
      {
        _params->FPrep = "SeqPair";
        annealer = new Annealer(_params, _newDB, _maxMem);
      }
      else
      {
        _params->FPrep = "BTree";
        annealer = new BTreeAreaWireAnnealer(_params, _newDB);
      }
   }
   else
   {
      abkfatal(false, "Invalid floorplan representation specified");
      exit(1);
   }


   if(_params->takePl)
   {
      //convert placement to sequence pair
      annealer->takePlfromDB();
   }

   if(_params->initQP)
   {
      annealer->solveQP();
      annealer->takePlfromDB();
   }

   if(_params->initCompact)
   {
      bool minimizeWL = false;
      annealer->compactSoln(minimizeWL,fixedOutline,reqdHeight,reqdWidth);
      //_newDB->plot("out.plt", currArea, 0, 0, 0, 0, 0, 0, 0);
   }

   //annealer->eval();
   float startArea = _newDB->getXMax()*_newDB->getYMax();
   if(_params->verb.getForMajStats() > 0)
      cout<<"Starting whitespace "<<100*(startArea-newDBArea)/newDBArea<<"%. Starting AR "<<_newDB->getXMax()/_newDB->getYMax()<<endl;

   //_newDB->save("temp");

   unsigned numNodes = _newDB->getNumNodes();
   Point dummy(0,0);
   vector<Point> bestSolnPl(numNodes, dummy);
   vector<ORIENT> bestSolnOrient(numNodes, N);
   vector<float> bestSolnWidth(numNodes, 0);
   vector<float> bestSolnHeight(numNodes, 0);
   Nodes* newNodes = _newDB->getNodes();

   float minViol = std::numeric_limits<float>::max();
   bool satisfied=true;
   do
   {
       annealer->go();

       // do the shifting HPWL optimization at the very end
       //       if (_params->minWL)
       //          annealer->postHPWLOpt();

       currXSize = _newDB->getXMax();
       currYSize = _newDB->getYMax();

       if(_params->solveTop && _params->dontClusterMacros)
       {
           float tempXSize = _newDB->getXMaxWMacroOnly();
           float tempYSize = _newDB->getYMaxWMacroOnly();
           if(tempXSize > 1e-5 && tempYSize > 1e-5 )
           {
               currXSize = tempXSize;
               currYSize = tempYSize;
           }
       }

       bool legal = lessOrEqualFloat(currXSize,reqdWidth) && lessOrEqualFloat(currYSize,reqdHeight);

       if(_params->compact || !legal) {
           annealer->compactSoln(_params->minWL,fixedOutline,reqdHeight,reqdWidth);

           currXSize = _newDB->getXMax();
           currYSize = _newDB->getYMax();

           if(_params->solveTop && _params->dontClusterMacros)
           {
               cout << "checking size with macros only" << endl;

               float tempXSize = _newDB->getXMaxWMacroOnly();
               float tempYSize = _newDB->getYMaxWMacroOnly();
               if(tempXSize > 1e-5 && tempYSize > 1e-5 )
               {
                   currXSize = tempXSize;
                   currYSize = tempYSize;
               }
           }

           legal = lessOrEqualFloat(currXSize,reqdWidth) && lessOrEqualFloat(currYSize,reqdHeight);
       }

       if(fixedOutline && legal && _params->verb.getForMajStats() > 0)
       {
         cout<<"Fixed-outline FPing SUCCESS"<<endl;
       }

       if(fixedOutline && !legal && _params->verb.getForMajStats() > 0)
       {
         cout<<"Fixed-outline FPing FAILURE"<<endl;
       }

       if(legal) break;

       //if not satisfied. then save best solution
       float viol=0;
       if(currXSize > reqdWidth)
           viol += (currXSize - reqdWidth);
       if(currYSize > reqdHeight)
           viol += (currYSize - reqdHeight);

       if(minViol > viol)
       {
           minViol = viol;
           itNode node;
           unsigned nodeId=0;
           for(node = newNodes->nodesBegin(); node != newNodes->nodesEnd(); ++node)
           {
               bestSolnPl[nodeId].x = node->getX();
               bestSolnPl[nodeId].y = node->getY();
               bestSolnOrient[nodeId] = node->getOrient();
               bestSolnWidth[nodeId] = node->getWidth();
               bestSolnHeight[nodeId] = node->getHeight();
               ++nodeId;
           }
       }

       maxIter++;
       if(maxIter == _params->maxIterHier)
       {
           if(_params->verb.getForMajStats() > 0)
               cout<<"FAILED to satisfy fixed outline constraints" <<
                   " for clustered hypergraph" <<endl;
           satisfied = false;
           break;
       }

       //change the annealer to BTree if 1'st 2 iterations fail
       if(maxIter == 2 && _params->FPrep == "SeqPair") 
       {
           if(_params->verb.getForMajStats() > 0)
               cout<<"Failed 1st iteration. Changing annealer to B*Tree"<<endl;

           annealTime += annealer->annealTime;
           delete annealer;

           _params->FPrep = "BTree";
           annealer = new BTreeAreaWireAnnealer(_params, _newDB);
       }
   }
   while(1);

   annealTime += annealer->annealTime;
   delete annealer;

   if(!satisfied)//failed to satisfy constraints. save best soln
   {
      itNode node;
      unsigned nodeId=0;
      for(node = newNodes->nodesBegin(); node != newNodes->nodesEnd(); ++node)
      {
         node->putX(bestSolnPl[nodeId].x);
         node->putY(bestSolnPl[nodeId].y);
         node->changeOrient(bestSolnOrient[nodeId], *(_newDB->getNets()));
         node->putWidth(bestSolnWidth[nodeId]);
         node->putHeight(bestSolnHeight[nodeId]);
         ++nodeId;
      }
   }

   if(!equalFloat(inverseRatio, 1.f))
   {
      _newDB->resizeHardBlocks(inverseRatio);
   }

   updatePlaceUnCluster(_newDB);

   if(!_params->solveTop)
      placeSubBlocks();

   _params->maxWS = oldMaxWS;
 
   /* 
      _newDB->plot("main.plt", _newDB->evalArea(), 
      100*(_newDB->evalArea()-_newDB->getNodesArea())/_newDB->getNodesArea(),
      _newDB->getXMax()/_newDB->getYMax(), 0, _newDB->evalHPWL(), 
      0, 0, 1);

      _db->plot("final.plt", _db->evalArea(), 
      100*(_db->evalArea()-_db->getNodesArea())/_db->getNodesArea(),
      _db->getXMax()/_db->getYMax(), 0, _db->evalHPWL(), 
      0, 0, 0);
   */
}

void SolveMulti::placeSubBlocks(void)
{
   Nodes* nodes = _newDB->getNodes();
   Nodes* origNodes = _db->getNodes();
   
   //itNode node;

   Command_Line* params = new Command_Line(*_params);
   params->budgetTime = 0; // (false)

   // for each node at top-level
   for(itNode node=nodes->nodesBegin(); node!=nodes->nodesEnd(); ++node)
   {
      Point dbLoc; // location of a top-level block
      dbLoc.x = node->getX(); 
      dbLoc.y = node->getY();
      params->reqdAR = node->getWidth()/node->getHeight();

      DB *tempDB = new DB(_db,
                          node->getSubBlocks(),
                          dbLoc,
                          params->reqdAR);

      if(_params->verb.getForMajStats() > 0)
         cout << node->getName() << "  numSubBlks : " << node->numSubBlocks()
              << "reqdAR " << params->reqdAR << endl;

      BaseAnnealer *annealer = NULL;
      if (params->FPrep == "BTree")
      {
         annealer =
            new BTreeAreaWireAnnealer(params, tempDB);
      }
      else if (params->FPrep == "SeqPair")
      {
         annealer = new Annealer(params, tempDB, _maxMem);
      }
      else if (_params->FPrep == "Best")
      {
         if(tempDB->getNumNodes() < 100 && _params->maxWS > 10.)
         {
           annealer = new Annealer(_params, tempDB, _maxMem);
         }
         else
         {
           annealer = new BTreeAreaWireAnnealer(_params, tempDB);
         }
      }
      else
      {
         abkfatal(false, "Invalid floorplan representation specified");
         exit(1);
      }

      float currXSize, currYSize;
      float reqdWidth = node->getWidth();
      float reqdHeight = node->getHeight();

      int maxIter = 0;
      if(node->numSubBlocks() > 1)
      {
         unsigned numNodes = tempDB->getNumNodes();
         Point dummy;
         dummy.x=0;
         dummy.y=0;
         
         vector<Point> bestSolnPl(numNodes, dummy);
         vector<ORIENT> bestSolnOrient(numNodes, N);
         vector<float> bestSolnWidth(numNodes, 0);
         vector<float> bestSolnHeight(numNodes, 0);
         Nodes *tempNodes = tempDB->getNodes();
	  
         float minViol = std::numeric_limits<float>::max();
         bool satisfied=true;
         do
         {
            annealer->go();

            // do the shifting HPWL optimization after packing
//             if (_params->minWL)
//                annealer->postHPWLOpt();
            
            currXSize = tempDB->getXMax();
            currYSize = tempDB->getYMax();
            if(currXSize<=reqdWidth && currYSize<=reqdHeight)
               break;
            
            //if not satisfied. then save best solution
            float viol = 0;
            if(currXSize > reqdWidth)
               viol += (currXSize - reqdWidth);
            if(currYSize > reqdHeight)
               viol += (currYSize - reqdHeight);
	      
            if(minViol > viol)
            {
               minViol = viol;
               unsigned nodeId=0;

               for(itNode tempNode = tempNodes->nodesBegin();
                   tempNode != tempNodes->nodesEnd(); ++tempNode)
               {
                  bestSolnPl[nodeId].x = tempNode->getX();
                  bestSolnPl[nodeId].y = tempNode->getY();
                  bestSolnOrient[nodeId] = tempNode->getOrient();
                  bestSolnWidth[nodeId] = tempNode->getWidth();
                  bestSolnHeight[nodeId] = tempNode->getHeight();
                  ++nodeId;
               }
            }
	      
            maxIter++;
            if(maxIter == _params->maxIterHier)
            {
               if(_params->verb.getForMajStats() > 0)
                  cout<<"FAILED to satisfy fixed outline constraints for "
                      <<node->getName()<<endl;
               satisfied=false;
               break;
            }
         }
         while(1);

         delete annealer;

         if(!satisfied)//failed to satisfy constraints. save best soln
         {
            unsigned nodeId=0;
            for(itNode tempNode = tempNodes->nodesBegin(); tempNode != tempNodes->nodesEnd(); ++tempNode)
            {
               tempNode->putX(bestSolnPl[nodeId].x);
               tempNode->putY(bestSolnPl[nodeId].y);
               tempNode->changeOrient(bestSolnOrient[nodeId], *(tempDB->getNets()));
               // *(_newDB->getNets()));
               tempNode->putWidth(bestSolnWidth[nodeId]);
               tempNode->putHeight(bestSolnHeight[nodeId]);
               ++nodeId;
            }
         }
      }
      else
      {
         Point loc;
         loc.x = 0.0;
         loc.y = 0.0;
         tempDB->initPlacement(loc);
      }
      
      Point offset;
      offset.x = node->getX();
      offset.y = node->getY();
      
      tempDB->shiftDesign(offset);
 
      Nodes * tempNodes = tempDB->getNodes();

      if(node->numSubBlocks() > 1)
      {
         for(itNode tempNode = tempNodes->nodesBegin(); 
             tempNode != tempNodes->nodesEnd(); ++tempNode)
         {
            for(vector<int>::iterator tempIdx = tempNode->subBlocksBegin();
                tempIdx != tempNode->subBlocksEnd(); ++tempIdx)
            {
               Node& origNode = origNodes->getNode(*tempIdx);
               origNode.putX(tempNode->getX());
               origNode.putY(tempNode->getY());
               origNode.changeOrient(tempNode->getOrient(), *(_db->getNets()));
               origNode.putHeight(tempNode->getHeight());
               origNode.putWidth(tempNode->getWidth());
            }
         }
      }
      else if(node->numSubBlocks() == 1)
      {
         vector<int>::iterator tempIdx = node->subBlocksBegin();
         Node& origNode = origNodes->getNode(*tempIdx);
         origNode.putX(node->getX());
         origNode.putY(node->getY());
         origNode.changeOrient(node->getOrient(), *(_db->getNets()));
         origNode.putHeight(node->getHeight());
         origNode.putWidth(node->getWidth());
      }
      delete tempDB;
   } // end for each node
}

void SolveMulti::updatePlaceUnCluster(DB * clusterDB)
{
   Nodes* nodes = _db->getNodes();
   Nodes* newNodes = clusterDB->getNodes();

   itNode node;

   const unsigned numNew = 6;
   float layOutXSize = _db->getXMax();
   float layOutYSize = _db->getYMax();
   float xStep = layOutXSize/numNew;
   float yStep = layOutYSize/numNew;

   for(node = newNodes->nodesBegin(); node != newNodes->nodesEnd(); ++node)
   {
      if(node->numSubBlocks() > 1)
      {
         for(vector<int>::iterator subBlockIdx = node->subBlocksBegin(); 
             subBlockIdx != node->subBlocksEnd(); ++subBlockIdx)
         {
            Node& tempNode = nodes->getNode(*subBlockIdx);
            if(!_params->usePhyLocHier || node->numSubBlocks() == 1)
            {
               tempNode.putX(node->getX());
               tempNode.putY(node->getY());
               tempNode.changeOrient(node->getOrient(), *(_db->getNets()));
            }
            else
            {
               float xloc = tempNode.getX();
               float yloc = tempNode.getY();
               unsigned xIdx = unsigned(floor(xloc/xStep));
               unsigned yIdx = unsigned(floor(yloc/yStep));
               xloc -= xIdx*xStep;
               yloc -= yIdx*yStep;
		  
               tempNode.putX(xloc+node->getX());
               tempNode.putY(yloc+node->getY());
            }
            //tempNode.changeOrient(node->getOrient(), *(_db->getNets()));
         }
      }
      else if(node->numSubBlocks() == 1)  //the only block
      {
         vector<int>::iterator subBlockIdx = node->subBlocksBegin();
         Node& tempNode = nodes->getNode(*subBlockIdx);
         tempNode.putX(node->getX());
         tempNode.putY(node->getY());
         tempNode.changeOrient(node->getOrient(), *(_db->getNets()));
         tempNode.putHeight(node->getHeight());
         tempNode.putWidth(node->getWidth());
      }
   }
}
