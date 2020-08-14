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


// 040608 hhchan modified how it plots:
//                  plot terminals only when plotNets, "set size ratio -1"

#include "ABKCommon/abkcommon.h"
#include "FPcommon.h"
#include "DB.h"
#include "PlToSP.h"
#include "SPeval.h"
#include <map>
#include <algorithm>
#include <climits>
#include <cfloat>
#include <cmath>
#include <sstream>


#ifdef USEFLUTE
#include "Flute/flute.h"
#endif

using std::min;
using std::max;
using std::stringstream;
using std::numeric_limits;
using std::map;
using std::string;
using std::cout;
using std::endl;
using std::ofstream;
using std::vector;


namespace parquetfp
{

void DB::buildTermBBox(void)
{
   termBBox.clear();
   if(_nodes)
   {
     Point temp(0.f,0.f);
     termBBox.put(temp);
     for(itNode n = _nodes->terminalsBegin(); n != _nodes->terminalsEnd(); ++n)
     {
       temp.x = n->getX();
       temp.y = n->getY();
       termBBox.put(temp);
       temp.x += n->getWidth();
       temp.y += n->getHeight();
       termBBox.put(temp);
     }
   }
}

void DB::scaleTerminals(void)
{
   if(_nodes)
   {
     _scaledLocs.resize(_nodes->getNumTerminals());

     float xScale = getXMax()/termBBox.getXSize();
     float yScale = getYMax()/termBBox.getYSize();     

     for(itNode n = _nodes->terminalsBegin(); n != _nodes->terminalsEnd(); ++n)
     {
       _scaledLocs[n->getIndex()].x = xScale * n->getX();
       _scaledLocs[n->getIndex()].y = yScale * n->getY();
     }     
   }
}

//ctor
DB::DB(const std::string &baseName)
{
   cout << "Reading Nodes (files " << baseName << ".blocks and "
        << baseName << ".pl)" << endl;
   _nodes = new Nodes(baseName);

   cout << "Reading Nets (file " << baseName << ".nets)" 
        << endl; 
   _nets =  new Nets(baseName);
   
   _nets->updateNodeInfo(*_nodes);
   _nodes->updatePinsInfo(*_nets);
   _nodes->initNodesFastPOAccess(*_nets, false);

   cout << "Done creating DB " << endl;

   _nodesBestCopy = new Nodes();
   _obstacles = new Nodes();
   //_nodesBestCopy = new Nodes(baseName);
   //_nodesBestCopy->updatePinsInfo(*_nets);

   _initArea = 0;
   successAR = 0;
   _rowHeight = 0;
   _siteSpacing = 0;

   buildTermBBox();
}

DB::DB(void)
{
   _nodes = new Nodes();
   _nets =  new Nets();
   _nodesBestCopy = new Nodes();
   _obstacles = new Nodes();

   _initArea = 0;
   successAR = 0;
   _rowHeight = 0;
   _siteSpacing = 0;

   buildTermBBox();
}

DB::DB(DB * db, vector<int>& subBlocksIndices, parquetfp::Point& dbLoc, float reqdAR)
{
   _nodes = new Nodes();
   _nets =  new Nets();
   _nodesBestCopy = new Nodes();
   _obstacles = new Nodes();
   _initArea = 0;
   successAR = 0;
   _rowHeight = 0;
   _siteSpacing = 0;

   Nodes* origNodes = db->getNodes();
   Nets* origNets = db->getNets();

   map<unsigned, unsigned> mapping;
   unsigned numOrigNodes = origNodes->getNumNodes();
   vector<bool> seenNodes(numOrigNodes, false);

   float nodesArea = 0;
   for(unsigned i=0; i<subBlocksIndices.size(); ++i)
   {
      Node& origNode = origNodes->getNode(subBlocksIndices[i]);
      nodesArea += origNode.getArea();
      Node tempNode(origNode.getName(), origNode.getArea() , 
                    origNode.getminAR(), origNode.getmaxAR(), i, false);
      tempNode.addSubBlockIndex(subBlocksIndices[i]);
      tempNode.putX(origNode.getX());
      tempNode.putY(origNode.getY());
      _nodes->putNewNode(tempNode);
      seenNodes[subBlocksIndices[i]] = true;
      mapping[subBlocksIndices[i]] = i;
   }

   float reqdHeight = sqrt(nodesArea/reqdAR);
   float reqdWidth = reqdHeight*reqdAR;
   float termXLoc, termYLoc;
   float minXLoc = dbLoc.x;
   float maxXLoc = minXLoc + reqdWidth;
   float minYLoc = dbLoc.y;
   float maxYLoc = minYLoc + reqdHeight;

   unsigned numOrigNets = origNets->getNumNets();
   vector<bool> seenNets(numOrigNets, false);

   unsigned newNetCtr=0;
   unsigned newTermCtr = 0;

   itNodePin origNodePin;
   Net tempEdge;
  
   for(unsigned i=0; i<subBlocksIndices.size(); ++i)
   {
      Node& origNode = origNodes->getNode(subBlocksIndices[i]);
      for(origNodePin = origNode.pinsBegin(); 
          origNodePin != origNode.pinsEnd(); ++origNodePin)
      {
         Net& origNet = origNets->getNet(origNodePin->netIndex);
         if(!seenNets[origNet.getIndex()])
         {
            seenNets[origNet.getIndex()] = true;
            tempEdge.putName(origNet.getName());
            tempEdge.putIndex(newNetCtr);
            for(itPin netPin = origNet.pinsBegin(); 
                netPin != origNet.pinsEnd(); ++netPin)
            {
               float poffsetX, poffsetY;
               unsigned origNodeIdx = netPin->getNodeIndex();
               if(!netPin->getType()) //not a terminal
               {
                  if(seenNodes[origNodeIdx])
                  {
                     unsigned newNodeIdx = mapping[origNodeIdx];
                     poffsetX = netPin->getXOffset();
                     poffsetY = netPin->getYOffset();

                     Node& newNode = _nodes->getNode(newNodeIdx);
                     pin tempPin(newNode.getName(), false, poffsetX, 
                                 poffsetY, newNetCtr);
                     tempPin.putNodeIndex(newNodeIdx);
                     tempEdge.addNode(tempPin);
                  }
                  else //need terminal propagation
                  {
                     Node& origTerm = 
                        origNodes->getNode(netPin->getNodeIndex());
                          
                     Node tempTerm(origTerm.getName(), 0, 1, 1, 
                                   newTermCtr, true);
                     tempTerm.addSubBlockIndex(origTerm.getIndex());

                     //terminal propagation
                     if(origTerm.getX() < minXLoc)
                        termXLoc = 0;
                     else if(origTerm.getX() > maxXLoc)
                        termXLoc = reqdWidth;
                     else
                        termXLoc = origTerm.getX() - minXLoc;

                     if(origTerm.getY() < minYLoc)
                        termYLoc = 0;
                     else if(origTerm.getY() > maxYLoc)
                        termYLoc = reqdHeight;
                     else
                        termYLoc = origTerm.getY() - minYLoc;

                     tempTerm.putX(termXLoc);
                     tempTerm.putY(termYLoc);

                     _nodes->putNewTerm(tempTerm);
                          
                     pin tempPin(origTerm.getName(), true, 0, 0, newNetCtr);
                     tempPin.putNodeIndex(newTermCtr);
                     tempEdge.addNode(tempPin);
                     newTermCtr++;                        
                  }
               }
               else //actual terminal
               {
                  Node& origTerm = 
                     origNodes->getTerminal(netPin->getNodeIndex());

                  Node tempTerm(origTerm.getName(), 0, 1, 1, newTermCtr,
                                true);
                  tempTerm.addSubBlockIndex(origTerm.getIndex());


                  //terminal propagation
                  if(origTerm.getX() < minXLoc)
                     termXLoc = 0;
                  else if(origTerm.getX() > maxXLoc)
                     termXLoc = reqdWidth;
                  else
                     termXLoc = origTerm.getX() - minXLoc;
                      
                  if(origTerm.getY() < minYLoc)
                     termYLoc = 0;
                  else if(origTerm.getY() > maxYLoc)
                     termYLoc = reqdHeight;
                  else
                     termYLoc = origTerm.getY() - minYLoc;
                      
                  tempTerm.putX(termXLoc);
                  tempTerm.putY(termYLoc);
                      
                  _nodes->putNewTerm(tempTerm);

                  pin tempPin(origTerm.getName(), true, 0, 0, newNetCtr);
                  tempPin.putNodeIndex(newTermCtr);
                  tempEdge.addNode(tempPin);
                  newTermCtr++;
               }
            }
            _nets->putNewNet(tempEdge);
            ++newNetCtr;
            tempEdge.clean();
         }
      }
   }
   _nodes->updatePinsInfo(*_nets);
   _nodes->initNodesFastPOAccess(*_nets, false);
   *_nodesBestCopy = *_nodes;
   *_obstacles = *(db->_obstacles);
   _obstacleFrame[0] = db->_obstacleFrame[0];
   _obstacleFrame[1] = db->_obstacleFrame[1];

   buildTermBBox();
}

DB::~DB()
{
   if(_nodes) delete _nodes;
   if(_nets) delete _nets;
   if(_nodesBestCopy) delete _nodesBestCopy;
   if(_obstacles) delete _obstacles;
}

DB& DB::operator=(DB& db2)
{
   if(this != &db2)
   {
      if(_nodes) delete _nodes;
      if(_nets) delete _nets;
      if(_nodesBestCopy) delete _nodesBestCopy;
	  if(_obstacles) delete _obstacles;
      
      _nodes = new Nodes();
      _nets = new Nets();
      _nodesBestCopy = new Nodes();
	  _obstacles = new Nodes();
      
      *(_nodes) = *(db2.getNodes());
      *(_nets) = *(db2.getNets());
      *(_nodesBestCopy) = *(db2._nodesBestCopy);
      *(_obstacles) = *(db2._obstacles);
      _obstacleFrame[0] = db2._obstacleFrame[0];
      _obstacleFrame[1] = db2._obstacleFrame[1];

      _area = db2._area;
      _initArea = db2._initArea;
      _rowHeight = db2._rowHeight;
      _siteSpacing = db2._siteSpacing;

      termBBox = db2.termBBox;
   }
   return *this;
}


struct sortFnPairFirst
{
  bool operator()(const std::pair<unsigned, float> &elem1, 
    const std::pair<unsigned, float> &elem2)
  { return(elem1.first < elem2.first); }
};


DB::DB(DB& db2, bool compressDB)
{
   if(this != &db2)
   {
       /*
          if(_nodes) delete _nodes;
          if(_nets) delete _nets;
          if(_nodesBestCopy) delete _nodesBestCopy;
        */
       _nodes = new Nodes();
       _nets = new Nets();
       _nodesBestCopy = new Nodes();
       _obstacles = new Nodes();

       *(_nodes) = *(db2.getNodes());
       *(_nodesBestCopy) = *(db2._nodesBestCopy);
       *(_obstacles) = *(db2._obstacles);
       _obstacleFrame[0] = db2._obstacleFrame[0];
       _obstacleFrame[1] = db2._obstacleFrame[1];

       _area = db2._area;
       _initArea = db2._initArea;
       _rowHeight = db2._rowHeight;
       _siteSpacing = db2._siteSpacing;

       if(!compressDB)
       {
           *(_nets) = *(db2.getNets());
       }
       else
       {
           //any net connected to a macro or terminal is added as is
           //only nets between normal blocks are compressed
           int netCtr=0;
           Nets* oldNets = db2.getNets();
           Nodes* oldNodes = db2.getNodes();
           vector<bool> seenNets(oldNets->getNumNets(), false);
           for(itNode term=oldNodes->terminalsBegin(); 
                   term != oldNodes->terminalsEnd(); ++term)
           {
               for(itNodePin pin=term->pinsBegin(); pin!=term->pinsEnd(); ++pin)
               {
                   unsigned netId = pin->netIndex;
                   if(!seenNets[netId])
                   {
                       Net& origNet = oldNets->getNet(netId);
                       Net tempEdge;
                       tempEdge.putName(origNet.getName());
                       tempEdge.putIndex(netCtr);
                       tempEdge.putWeight(origNet.getWeight());
                       for(itPin netPin = origNet.pinsBegin(); 
                               netPin != origNet.pinsEnd(); ++netPin)
                       {
                           unsigned currNodeIdx = netPin->getNodeIndex();
                           float poffsetX = 0, poffsetY = 0;
                           poffsetX = netPin->getXOffset();
                           poffsetY = netPin->getYOffset();
                           if(!netPin->getType())
                           {
                               Node& oldNode = oldNodes->getNode(currNodeIdx);
                               parquetfp::pin tempPin(oldNode.getName(), false, poffsetX, 
                                       poffsetY, netCtr);
                               tempPin.putNodeIndex(currNodeIdx);
                               tempEdge.addNode(tempPin);
                           }
                           else
                           {
                               Node& oldTerm = oldNodes->getTerminal(currNodeIdx);
                               parquetfp::pin tempPin(oldTerm.getName(), true, poffsetX, 
                                       poffsetY, netCtr);
                               tempPin.putNodeIndex(currNodeIdx);
                               tempEdge.addNode(tempPin);
                           }
                       }
                       seenNets[netId] = true;
                       _nets->putNewNet(tempEdge);
                       ++netCtr;
                   }
               }
           }

           for(itNode node=oldNodes->nodesBegin(); 
                   node != oldNodes->nodesEnd(); ++node)
           {
               for(itNodePin pin=node->pinsBegin(); pin!=node->pinsEnd(); ++pin)
               {
                   unsigned netId = pin->netIndex;
                   Net& origNet = oldNets->getNet(netId);
                   if(!seenNets[netId] 
                           /*&& ( node->isMacro() || origNet.getDegree() > 2)*/)
                   {
                       Net tempEdge;
                       bool atleast1PinOffsetNotZero=false;
                       tempEdge.putName(origNet.getName());
                       tempEdge.putIndex(netCtr);
                       tempEdge.putWeight(origNet.getWeight());
                       for(itPin netPin = origNet.pinsBegin(); 
                               netPin != origNet.pinsEnd(); ++netPin)
                       {
                           unsigned currNodeIdx = netPin->getNodeIndex();
                           float poffsetX = 0, poffsetY = 0;
                           poffsetX = netPin->getXOffset();
                           poffsetY = netPin->getYOffset();
                           if(!(std::abs(poffsetX-0) < 1e-5 && std::abs(poffsetY-0) < 1e-5))
                               atleast1PinOffsetNotZero = true;

                           if(!netPin->getType())
                           {
                               Node& oldNode = oldNodes->getNode(currNodeIdx);
                               parquetfp::pin tempPin(oldNode.getName(), false, poffsetX, 
                                       poffsetY, netCtr);
                               tempPin.putNodeIndex(currNodeIdx);
                               tempEdge.addNode(tempPin);
                           }
                           else
                           {
                               Node& oldTerm = oldNodes->getTerminal(currNodeIdx);
                               parquetfp::pin tempPin(oldTerm.getName(), true, poffsetX, 
                                       poffsetY, netCtr);
                               tempPin.putNodeIndex(currNodeIdx);
                               tempEdge.addNode(tempPin);
                           }
                       }
                       if(atleast1PinOffsetNotZero || origNet.getDegree()>2)
                       {
                           seenNets[netId] = true;
                           _nets->putNewNet(tempEdge);
                           ++netCtr;
                       }
                   }
               }
           }

           //now add the compressing code
           vector< vector< std::pair<unsigned,float> > > nodeConnections(oldNodes->getNumNodes());

           for(itNode node=oldNodes->nodesBegin(); 
                   node != oldNodes->nodesEnd(); ++node)
           {
               //if(!node->isMacro())
               {
                   unsigned currNodeIdx = node->getIndex();

                   for(itNodePin pin=node->pinsBegin(); pin!=node->pinsEnd(); ++pin)
                   {
                       unsigned netId = pin->netIndex;
                       Net& origNet = oldNets->getNet(netId);
                       if(!seenNets[netId] && origNet.getDegree() == 2)
                       {
                           for(itPin netPin = origNet.pinsBegin(); 
                                   netPin != origNet.pinsEnd(); ++netPin)
                           {
                               unsigned nextNodeIdx = netPin->getNodeIndex();
                               //following cond ensures that we go over each
                               //net only once
                               if(nextNodeIdx > currNodeIdx)
                               {
                                   std::pair<unsigned, float> elem(nextNodeIdx, origNet.getWeight());
                                   nodeConnections[currNodeIdx].push_back(elem);
                               }
                           }
                       }
                   }
               } 
           }

           for(unsigned i=0; i<nodeConnections.size(); ++i)
               std::sort(nodeConnections[i].begin(), nodeConnections[i].end(), sortFnPairFirst());

           char netName [100];
           for(int i=0; i<int(nodeConnections.size()); ++i)
           {
               int j=0;
               while(j<int(nodeConnections[i].size()))
               {
                   unsigned numConn=1;
                   float sumOfWts=nodeConnections[i][j].second;
                   if(j != (int(nodeConnections[i].size())-1))
                   {
                       while(j<(int(nodeConnections[i].size())-1) &&
                               nodeConnections[i][j].first == nodeConnections[i][j+1].first)
                       {
                           ++numConn;
                           ++j;
                           sumOfWts+=nodeConnections[i][j].second;
                       }
                   }

                   Net tempEdge;
                   sprintf(netName, "clusteredNet_%d",netCtr);
                   tempEdge.putName(netName);
                   tempEdge.putIndex(netCtr);
                   float poffsetX = 0, poffsetY = 0;
                   Node& node1 = _nodes->getNode(i);
                   Node& node2 = _nodes->getNode(nodeConnections[i][j].first);

                   pin tempPin1(node1.getName(), false, poffsetX, 
                           poffsetY, netCtr);
                   tempPin1.putNodeIndex(i);
                   tempEdge.addNode(tempPin1);

                   pin tempPin2(node2.getName(), false, poffsetX, 
                           poffsetY, netCtr);
                   tempPin2.putNodeIndex(nodeConnections[i][j].first);
                   tempEdge.addNode(tempPin2);

                   //tempEdge.putWeight(numConn);
                   tempEdge.putWeight(sumOfWts);
                   _nets->putNewNet(tempEdge);

                   ++netCtr;
                   ++j;
               }
           }
           //_nets->updateNodeInfo(*_nodes);
           _nodes->updatePinsInfo(*_nets);
           _nodes->initNodesFastPOAccess(*_nets, false);

           /*
              cout<<"Initial #Nets "<<oldNets->getNumNets()<<" Initial #Pins "
              <<oldNets->getNumPins()<<" Compressed #Nets "<<_nets->getNumNets()
              <<" Compressed #Pins "<<_nets->getNumPins()<<endl;
            */
       }
       buildTermBBox();
   }
}


void DB::clean(void)
{
   if(_nodes) _nodes->clean();
   if(_nets) _nets->clean();
   if(_nodesBestCopy) _nodesBestCopy->clean();
   if(_obstacles) _obstacles->clean();
}

Nodes* DB::getNodes(void)
{ return _nodes; }

Nets* DB::getNets(void) 
{ return _nets; }

Nodes* DB::getObstacles(void)
{ return _obstacles; }

void DB::addObstacles(Nodes *obstacles, float obstacleFrame[2])
{
   if(_obstacles) delete _obstacles;
   _obstacles = new Nodes();
   *(_obstacles) = *obstacles;
   _obstacleFrame[0] = obstacleFrame[0];
   _obstacleFrame[1] = obstacleFrame[1];
}

unsigned DB::getNumObstacles(void) const
{
   return _obstacles->getNumNodes();
}

unsigned DB::getNumNodes(void) const
{
   return _nodes->getNumNodes();
}

vector<float> DB::getNodeWidths(void) const
{
   return _nodes->getNodeWidths();
}

vector<float> DB::getNodeHeights(void) const
{
   return _nodes->getNodeHeights();
}

vector<float> DB::getXLocs(void) const
{
   return _nodes->getXLocs();
}

vector<float> DB::getYLocs(void) const
{
   return _nodes->getYLocs();
}

parquetfp::Point DB::getBottomLeftCorner() const
{
   const vector<float> &nodesXLocs = _nodes->getXLocs();
   const vector<float> &nodesYLocs = _nodes->getYLocs();
   
   vector<float>::const_iterator xMin = min_element(nodesXLocs.begin(),
                                               nodesXLocs.end());
   vector<float>::const_iterator yMin = min_element(nodesYLocs.begin(),
                                               nodesYLocs.end());

   Point bottomLeft;
   bottomLeft.x = *xMin;
   bottomLeft.y = *yMin;
   return bottomLeft;
}

parquetfp::Point DB::getTopRightCorner() const
{
   vector<float> nodesXLocs(_nodes->getXLocs());
   vector<float> nodesYLocs(_nodes->getYLocs());
   for (unsigned int i = 0; i < _nodes->getNumNodes(); i++)
   {
      nodesXLocs[i] += _nodes->getNodeWidth(i);
      nodesYLocs[i] += _nodes->getNodeHeight(i);
   }
   
   vector<float>::iterator xMax = max_element(nodesXLocs.begin(),
                                               nodesXLocs.end());
   vector<float>::iterator yMax = max_element(nodesYLocs.begin(),
                                               nodesYLocs.end());

   Point topRight;
   topRight.x = *xMax;
   topRight.y = *yMax;
   return topRight;
}

void DB::packToCorner(vector< vector<float> >& xlocsAt,
                      vector< vector<float> >& ylocsAt) const
{
   const Point topRightCorner(getTopRightCorner());
   const Point bottomLeftCorner(getBottomLeftCorner());

   const vector<float> xlocs(getXLocs());
   const vector<float> ylocs(getYLocs());
   const vector<float> widths(getNodeWidths());
   const vector<float> heights(getNodeHeights());

   // naive FP representation independent way of determining slacks
   vector<float> yBottomSlacks(getNumNodes());
   vector<float> yTopSlacks(getNumNodes());
   vector<float> xLeftSlacks(getNumNodes());
   vector<float> xRightSlacks(getNumNodes());

   // calculate the bottom, top, left, and right slacks for the current positions
   for (unsigned i = 0; i < getNumNodes(); i++)
   {
      xRightSlacks[i] = max(0.f, topRightCorner.x - xlocs[i] - widths[i]);
      xLeftSlacks[i] = max(0.f, xlocs[i] - bottomLeftCorner.x);

      yTopSlacks[i] = max(0.f, topRightCorner.y - ylocs[i] - heights[i]);
      yBottomSlacks[i] = max(0.f, ylocs[i] - bottomLeftCorner.y);

      for (unsigned j = 0; j < getNumNodes(); j++)
      {
         if (i != j)
         {
            // have x-overlap
            if (lessThanFloat(xlocs[i], xlocs[j] + widths[j]) &&
                lessThanFloat(xlocs[j], xlocs[i] + widths[i]))
            {
               if (ylocs[j] > ylocs[i])
               {
                  // [j] is above [i]
                  yTopSlacks[i] = min(yTopSlacks[i],
                                      max(0.f, ylocs[j] - (ylocs[i] + heights[i])));
               }
               else
               {
                  // [j] is below [i]
                  yBottomSlacks[i] = min(yBottomSlacks[i],
                                         max(0.f, ylocs[i] - (ylocs[j] + heights[j])));
               }
            }

            // have y-overlap
            if (lessThanFloat(ylocs[i], ylocs[j] + heights[j]) &&
                lessThanFloat(ylocs[j], ylocs[i] + heights[i]))
            {
               if (xlocs[j] > xlocs[i])
               {
                  // [j] is right of [i]
                  xRightSlacks[i] = min(xRightSlacks[i],
                                        max(0.f, xlocs[j] - (xlocs[i] + widths[i])));
               }
               else
               {
                  // [j] is left of [i]
                  xLeftSlacks[i] = min(xLeftSlacks[i],
                                       max(0.f, xlocs[i] - (xlocs[j] + widths[j])));
               }
            }
         }
      }
   }

   // set the locations for the easy directions
   for (unsigned i = 0; i < getNumNodes(); i++)
   {
      xlocsAt[BOTTOM][i] = xlocs[i];
      ylocsAt[BOTTOM][i] = ylocs[i] - yBottomSlacks[i];

      xlocsAt[TOP][i] = xlocs[i];
      ylocsAt[TOP][i] = ylocs[i] + yTopSlacks[i];

      xlocsAt[LEFT][i] = xlocs[i] - xLeftSlacks[i];
      ylocsAt[LEFT][i] = ylocs[i] ;

      xlocsAt[RIGHT][i] = xlocs[i] + xRightSlacks[i];
      ylocsAt[RIGHT][i] = ylocs[i];
   }

   // now assuming we packed to the bottom, find the x slacks
   // and assuming we packed to the left, find the y slacks

   for (unsigned i = 0; i < getNumNodes(); i++)
   {
      xRightSlacks[i] = topRightCorner.x - xlocsAt[BOTTOM][i] - widths[i];
      xLeftSlacks[i] = xlocsAt[BOTTOM][i] - bottomLeftCorner.x;

      yTopSlacks[i] = topRightCorner.y - ylocsAt[LEFT][i] - heights[i];
      yBottomSlacks[i] = ylocsAt[LEFT][i] - bottomLeftCorner.y;

      for (unsigned j = 0; j < getNumNodes(); j++)
      {
         if (i != j)
         {
            // have x-overlap
            if (lessThanFloat(xlocsAt[LEFT][i], xlocsAt[LEFT][j] + widths[j]) &&
                lessThanFloat(xlocsAt[LEFT][j], xlocsAt[LEFT][i] + widths[i]))
            {
               if (ylocsAt[LEFT][j] > ylocsAt[LEFT][i])
               {
                  // [j] is above [i]
                  yTopSlacks[i] = min(yTopSlacks[i],
                                      max(0.f, ylocsAt[LEFT][j] - (ylocsAt[LEFT][i] + heights[i])));
               }
               else
               {
                  // [j] is below [i]
                  yBottomSlacks[i] = min(yBottomSlacks[i],
                                         max(0.f, ylocsAt[LEFT][i] - (ylocsAt[LEFT][j] + heights[j])));
               }
            }

            // have y-overlap
            if (lessThanFloat(ylocsAt[BOTTOM][i], ylocsAt[BOTTOM][j] + heights[j]) &&
                lessThanFloat(ylocsAt[BOTTOM][j], ylocsAt[BOTTOM][i] + heights[i]))
            {
               if (xlocsAt[BOTTOM][j] > xlocsAt[BOTTOM][i])
               {
                  // [j] is right of [i]
                  xRightSlacks[i] = min(xRightSlacks[i],
                                        max(0.f, xlocsAt[BOTTOM][j] - (xlocsAt[BOTTOM][i] + widths[i])));
               }
               else
               {
                  // [j] is left of [i]
                  xLeftSlacks[i] = min(xLeftSlacks[i],
                                       max(0.f, xlocsAt[BOTTOM][i] - (xlocsAt[BOTTOM][j] + widths[j])));
               }
            }
         }
      }
   }

   // set the locations for those compound directions
   for (unsigned i = 0; i < getNumNodes(); i++)
   {
      xlocsAt[BOTTOM_LEFT][i] = xlocsAt[BOTTOM][i] - xLeftSlacks[i];
      ylocsAt[BOTTOM_LEFT][i] = ylocsAt[BOTTOM][i];

      xlocsAt[BOTTOM_RIGHT][i] = xlocsAt[BOTTOM][i] + xRightSlacks[i];
      ylocsAt[BOTTOM_RIGHT][i] = ylocsAt[BOTTOM][i];

      xlocsAt[LEFT_BOTTOM][i] = xlocsAt[LEFT][i];
      ylocsAt[LEFT_BOTTOM][i] = ylocsAt[LEFT][i] - yBottomSlacks[i];

      xlocsAt[LEFT_TOP][i] = xlocsAt[LEFT][i];
      ylocsAt[LEFT_TOP][i] = ylocsAt[LEFT][i] + yTopSlacks[i];
   }

   // now assuming we packed to the top, find the x slacks
   // and assuming we packed to the right, find the y slacks

   for (unsigned i = 0; i < getNumNodes(); i++)
   {
      xRightSlacks[i] = topRightCorner.x - xlocsAt[TOP][i] - widths[i];
      xLeftSlacks[i] = xlocsAt[TOP][i] - bottomLeftCorner.x;

      yTopSlacks[i] = topRightCorner.y - ylocsAt[RIGHT][i] - heights[i];
      yBottomSlacks[i] = ylocsAt[RIGHT][i] - bottomLeftCorner.y;

      for (unsigned j = 0; j < getNumNodes(); j++)
      {
         if (i != j)
         {
            // have x-overlap
            if (lessThanFloat(xlocsAt[RIGHT][i], xlocsAt[RIGHT][j] + widths[j]) &&
                lessThanFloat(xlocsAt[RIGHT][j], xlocsAt[RIGHT][i] + widths[i]))
            {
               if (ylocsAt[RIGHT][j] > ylocsAt[RIGHT][i])
               {
                  // [j] is above [i]
                  yTopSlacks[i] = min(yTopSlacks[i],
                                      max(0.f, ylocsAt[RIGHT][j] - (ylocsAt[RIGHT][i] + heights[i])));
               }
               else
               {
                  // [j] is below [i]
                  yBottomSlacks[i] = min(yBottomSlacks[i],
                                         max(0.f, ylocsAt[RIGHT][i] - (ylocsAt[RIGHT][j] + heights[j])));
               }
            }

            // have y-overlap
            if (lessThanFloat(ylocsAt[TOP][i], ylocsAt[TOP][j] + heights[j]) &&
                lessThanFloat(ylocsAt[TOP][j], ylocsAt[TOP][i] + heights[i]))
            {
               if (xlocsAt[TOP][j] > xlocsAt[TOP][i])
               {
                  // [j] is right of [i]
                  xRightSlacks[i] = min(xRightSlacks[i],
                                        max(0.f, xlocsAt[TOP][j] - (xlocsAt[TOP][i] + widths[i])));
               }
               else
               {
                  // [j] is left of [i]
                  xLeftSlacks[i] = min(xLeftSlacks[i],
                                       max(0.f, xlocsAt[TOP][i] - (xlocsAt[TOP][j] + widths[j])));
               }
            }
         }
      }
   }

   // set the locations for those compound directions
   for (unsigned i = 0; i < getNumNodes(); i++)
   {
      xlocsAt[TOP_LEFT][i] = xlocsAt[TOP][i] - xLeftSlacks[i];
      ylocsAt[TOP_LEFT][i] = ylocsAt[TOP][i];

      xlocsAt[TOP_RIGHT][i] = xlocsAt[TOP][i] + xRightSlacks[i];
      ylocsAt[TOP_RIGHT][i] = ylocsAt[TOP][i];

      xlocsAt[RIGHT_BOTTOM][i] = xlocsAt[RIGHT][i];
      ylocsAt[RIGHT_BOTTOM][i] = ylocsAt[RIGHT][i] - yBottomSlacks[i];

      xlocsAt[RIGHT_TOP][i] = xlocsAt[RIGHT][i];
      ylocsAt[RIGHT_TOP][i] = ylocsAt[RIGHT][i] + yTopSlacks[i];
   }
}

#ifdef USEFLUTE
void DB::cornerOptimizeDesign(bool scaleTerms, bool minWL, bool useSteiner)
#else
void DB::cornerOptimizeDesign(bool scaleTerms, bool minWL)
#endif
{
   vector<float> origXLocs;
   vector<float> origYLocs;

   vector< vector<float> > xlocsAt(NUM_CORNERS,
                                    vector<float>(getNumNodes()));
   vector< vector<float> > ylocsAt(NUM_CORNERS,
                                    vector<float>(getNumNodes()));

   int ORIGINAL = -1;
   int minCorner = ORIGINAL;
   bool useWts = true;

   float minObjective = numeric_limits<float>::max();

   if(minWL)
   {
#ifdef USEFLUTE
     minObjective = evalHPWL(useWts,scaleTerms,useSteiner);
#else
     minObjective = evalHPWL(useWts,scaleTerms);
#endif
     cout << "Original HPWL: " << minObjective << endl;
   }
   else
   {
     minObjective = evalArea();
     cout << "Original Area: " << minObjective << endl;
   }

   do
   {
     origXLocs = getXLocs();
     origYLocs = getYLocs();

     minCorner = ORIGINAL;

     packToCorner(xlocsAt, ylocsAt);

     for (unsigned i = 0; i < NUM_CORNERS; i++)
     {
        updatePlacement(xlocsAt[i], ylocsAt[i]);

        float currObjective = numeric_limits<float>::max();
        if(minWL)
        {
#ifdef USEFLUTE
          currObjective = evalHPWL(useWts,scaleTerms,useSteiner);
#else
          currObjective = evalHPWL(useWts,scaleTerms);
#endif
          cout << toString(Corner(i)) << " HPWL: " << currObjective << endl;
        }
        else
        {
          currObjective = evalArea();
          cout << toString(Corner(i)) << " Area: " << currObjective << endl;
        }

        if (lessThanFloat(currObjective, minObjective))
        {
           minObjective = currObjective;
           minCorner = i;
        }
     }
     if(minWL)
     {
       cout << "minimum HPWL: " << minObjective;
     }
     else
     {
       cout << "minimum Area: " << minObjective;
     }

     if (minCorner == ORIGINAL)
        cout << " in the original position." << endl;
     else
        cout << " at " << toString(Corner(minCorner)) << endl;

     cout << "packing design towards "
          << ((minCorner==ORIGINAL)
              ? string("the original position")
              : toString(Corner(minCorner))) << endl;

     if (minCorner == ORIGINAL)
        updatePlacement(origXLocs, origYLocs);
     else
        updatePlacement(xlocsAt[minCorner], ylocsAt[minCorner]);

   } while(minCorner != ORIGINAL);

   if(minWL)
   {
#ifdef USEFLUTE
     cout << "final HPWL: " << evalHPWL(useWts,scaleTerms,useSteiner) << endl;
#else
     cout << "final HPWL: " << evalHPWL(useWts,scaleTerms) << endl;
#endif
   }
   else
   {
     cout << "final Area: " << evalArea() << endl;
   }
}

float DB::getNodesArea(void) const
{
   if(_initArea)
      return _area;
   else
   {
      _area=_nodes->getNodesArea();
      _initArea = 1;
      return _area;
   }
}

float DB::getRowHeight(void) const
{
   return _rowHeight;
}

float DB::getSiteSpacing(void) const
{
   return _siteSpacing;
}

void DB::setRowHeight(float rowHeight)
{
   _rowHeight = rowHeight;
}

void DB::setSiteSpacing(float siteSpacing)
{
   _siteSpacing = siteSpacing;
}

float DB::getAvgHeight(void) const
{
   itNode node;
   float avgHeight = 0;
   for(node = _nodes->nodesBegin(); node != _nodes->nodesEnd(); ++node)
      avgHeight += node->getHeight();
   avgHeight /= getNumNodes();
   return(avgHeight);
}

void DB::updatePlacement(const vector<float>& xloc,
                         const vector<float>& yloc)
{
   unsigned int numNodes = _nodes->getNumNodes();
   unsigned int size = min(unsigned(xloc.size()), numNodes);
   abkwarn(size >= numNodes, "Too few locations specified.");

   for(unsigned i = 0; i < size; i++)
   {
      _nodes->getNode(i).putX(xloc[i]);
      _nodes->getNode(i).putY(yloc[i]);
   }
}

void DB::initPlacement(const parquetfp::Point& loc)
{
   itNode node;
   for(node = _nodes->nodesBegin(); node != _nodes->nodesEnd(); ++node)
   {
      node->putX(loc.x);
      node->putY(loc.y);
   }
}

void DB::updateSlacks(const vector<float>& xSlack,
                      const vector<float>& ySlack)
{
   for(unsigned i=0; i<xSlack.size(); ++i)
   {
      _nodes->getNode(i).putslackX(xSlack[i]);
      _nodes->getNode(i).putslackY(ySlack[i]);
   }  
}

#ifdef USEFLUTE
float DB::evalHPWL(bool useWts, bool scaleTerms, bool useSteiner)
{
  exit(1);
    if(useSteiner) return evalSteiner(useWts, scaleTerms);
#else
float DB::evalHPWL(bool useWts, bool scaleTerms)
{
#endif

    //this function has a lot of code repetition
    //because things have been manually inlined for speed concerns
    //this function accounts for the greatest portion of runtime
    //used by Parquet within Capo (it is the bottleneck)
    float HPWL=0.0f;
    float width=0.0f;
    float height=0.0f;

    if(scaleTerms)
    {
      scaleTerminals();
    }

    for(itNet net = _nets->netsBegin(); net != _nets->netsEnd(); ++net)
    {
        unsigned nDegree = net->getDegree();
        if(nDegree <= 1)
        {
            continue; //one pin has 0 WL
        }
        if(nDegree == 2)
        {  //fast special case for 2 pin nets
            Net& currNet = (*net);
            float nodeLocx=0.0f; 
            float nodeLocy=0.0f; 
            float pinOffsetx=0.0f; 
            float pinOffsety=0.0f; 
           
            //Begin Compute location of pin 0
            pin& pin0 = currNet.getPin(0); 
            unsigned nodeIndex0 = pin0.getNodeIndex();
            Node* node0;
            if(pin0.getType())
            {
              node0 = &_nodes->getTerminal(nodeIndex0);
              if(scaleTerms)
              {
                nodeLocx = getScaledX(*node0);
                nodeLocy = getScaledY(*node0);
              }
              else
              {
                nodeLocx = node0->getX();
                nodeLocy = node0->getY();
              }
            }
            else
            {
              node0 = &_nodes->getNode(nodeIndex0);
              nodeLocx = node0->getX();
              nodeLocy = node0->getY();
            }
            //Begin Compute pin offsets 
            width = node0->getWidth();
            height = node0->getHeight();
            if(node0->allPinsAtCenter)
            {
                pinOffsetx = 0.5f*width;
                pinOffsety = 0.5f*height;
            }
            else
            {
                pinOffsetx = (0.5f + pin0.getXOffset())*width;
                pinOffsety = (0.5f + pin0.getYOffset())*height;
            }
            //End Compute pin offsets  
            float pin0Locx = nodeLocx+pinOffsetx;
            float pin0Locy = nodeLocy+pinOffsety;
            //End Compute location of pin 0

            //Begin Compute location of pin 1
            pin& pin1 = currNet.getPin(1); 
            unsigned nodeIndex1 = pin1.getNodeIndex();
            Node* node1;
            if(pin1.getType())
            {
              node1 = &_nodes->getTerminal(nodeIndex1);
              if(scaleTerms)
              {
                nodeLocx = getScaledX(*node1);
                nodeLocy = getScaledY(*node1);
              }
              else
              {
                nodeLocx = node1->getX();
                nodeLocy = node1->getY();
              }
            }
            else
            {
              node1 = &_nodes->getNode(nodeIndex1);
              nodeLocx = node1->getX();
              nodeLocy = node1->getY();
            }
            //Begin Compute pin offsets 
            width = node1->getWidth();
            height = node1->getHeight();
            if(node1->allPinsAtCenter)
            {
                pinOffsetx = 0.5f*width;
                pinOffsety = 0.5f*height;
            }
            else
            {
                pinOffsetx = (0.5f + pin1.getXOffset())*width;
                pinOffsety = (0.5f + pin1.getYOffset())*height;
            }
            //End Compute pin offsets  
            float pin1Locx = nodeLocx + pinOffsetx;
            float pin1Locy = nodeLocy + pinOffsety;
            //End Compute location of pin 1

            if(useWts)
            {
              HPWL+= net->getWeight()*(std::abs(pin1Locx - pin0Locx) + std::abs(pin1Locy - pin0Locy));
            }
            else
            {
              HPWL+= std::abs(pin1Locx - pin0Locx) + std::abs(pin1Locy - pin0Locy);
            }

//            if(HPWL == numeric_limits<float>::infinity())
//            {
//              const vector<float>& nodexlocs = _nodes->getXLocs();
//              const vector<float>& nodeylocs = _nodes->getYLocs();  

//              abkfatal(nodexlocs.size() == nodeylocs.size(), "What the hell am i doing?");

//              cerr<<"There is a problem with HPWL, dumping node locs"<<endl;
//              for(unsigned i = 0; i < nodexlocs.size(); ++i)
//              {
//                cerr<<" node idx: "<<i<<" Loc: "<<nodexlocs[i]<<","<<nodeylocs[i]<<endl;
//              }

//              abkfatal( HPWL != numeric_limits<float>::infinity(), "Returning inf HPWL! ");
//            }
        }
        else if (nDegree == 3)
        {
            Net& currNet = (*net);
            float nodeLocx=0.0f; 
            float nodeLocy=0.0f; 
            float pinOffsetx=0.0f; 
            float pinOffsety=0.0f; 
           
            //Begin Compute location of pin 0
            pin& pin0 = currNet.getPin(0); 
            unsigned nodeIndex0 = pin0.getNodeIndex();
            Node* node0;
            if(pin0.getType())
            {
              node0 = &_nodes->getTerminal(nodeIndex0);
              if(scaleTerms)
              {
                nodeLocx = getScaledX(*node0);
                nodeLocy = getScaledY(*node0);
              }
              else
              {
                nodeLocx = node0->getX();
                nodeLocy = node0->getY();
              }
            }
            else
            {
              node0 = &_nodes->getNode(nodeIndex0);
              nodeLocx = node0->getX();
              nodeLocy = node0->getY();
            }
            //Begin Compute pin offsets 
            width = node0->getWidth();
            height = node0->getHeight();
            if(node0->allPinsAtCenter)
            {
                pinOffsetx = 0.5f*width;
                pinOffsety = 0.5f*height;
            }
            else
            {
                pinOffsetx = (0.5f + pin0.getXOffset())*width;
                pinOffsety = (0.5f + pin0.getYOffset())*height;
            }
            //End Compute pin offsets  
            float pin0Locx = nodeLocx+pinOffsetx;
            float pin0Locy = nodeLocy+pinOffsety;
            //End Compute location of pin 0

            //Begin Compute location of pin 1
            pin& pin1 = currNet.getPin(1); 
            unsigned nodeIndex1 = pin1.getNodeIndex();
            Node* node1;
            if(pin1.getType())
            {
              node1 = &_nodes->getTerminal(nodeIndex1);
              if(scaleTerms)
              {
                nodeLocx = getScaledX(*node1);
                nodeLocy = getScaledY(*node1);
              }
              else
              {
                nodeLocx = node1->getX();
                nodeLocy = node1->getY();
              }
            }
            else
            {
              node1 = &_nodes->getNode(nodeIndex1);
              nodeLocx = node1->getX();
              nodeLocy = node1->getY();
            }
            //Begin Compute pin offsets 
            width = node1->getWidth();
            height = node1->getHeight();
            if(node1->allPinsAtCenter)
            {
                pinOffsetx = 0.5f*width;
                pinOffsety = 0.5f*height;
            }
            else
            {
                pinOffsetx = (0.5f + pin1.getXOffset())*width;
                pinOffsety = (0.5f + pin1.getYOffset())*height;
            }
            //End Compute pin offsets  
            float pin1Locx = nodeLocx + pinOffsetx;
            float pin1Locy = nodeLocy + pinOffsety;
            //End Compute location of pin 1

            //Begin Compute location of pin 2
            pin& pin2 = currNet.getPin(2); 
            unsigned nodeIndex2 = pin2.getNodeIndex();
            Node* node2;
            if(pin2.getType())
            {
              node2 = &_nodes->getTerminal(nodeIndex2);
              if(scaleTerms)
              {
                nodeLocx = getScaledX(*node2);
                nodeLocy = getScaledY(*node2);
              }
              else
              {
                nodeLocx = node2->getX();
                nodeLocy = node2->getY();
              }
            }
            else
            {
              node2 = &_nodes->getNode(nodeIndex2);
              nodeLocx = node2->getX();
              nodeLocy = node2->getY();
            }
            //Begin Compute pin offsets 
            width = node2->getWidth();
            height = node2->getHeight();
            if(node2->allPinsAtCenter)
            {
                pinOffsetx = 0.5f*width;
                pinOffsety = 0.5f*height;
            }
            else
            {
                pinOffsetx = (0.5f + pin2.getXOffset())*width;
                pinOffsety = (0.5f + pin2.getYOffset())*height;
            }
            //End Compute pin offsets
            float pin2Locx = nodeLocx + pinOffsetx;
            float pin2Locy = nodeLocy + pinOffsety;
            //End Compute location of pin 2

            float maxy=0.0f;
            float miny=0.0f;
            float maxx=0.0f;
            float minx=0.0f;
            if(pin1Locx > pin0Locx)
            {
              if(pin2Locx > pin1Locx)
              {
                maxx=pin2Locx;
                minx=pin0Locx;
              }
              else
              {
                maxx=pin1Locx;
                if(pin0Locx < pin2Locx)
                  minx = pin0Locx;
                else
                  minx = pin2Locx;
              }
            }
            else
            {
              if(pin2Locx > pin0Locx)
              {
                maxx=pin2Locx;
                minx=pin1Locx;
              }
              else
              {
                maxx=pin0Locx;
                if(pin1Locx < pin2Locx)
                  minx = pin1Locx;
                else
                  minx = pin2Locx;
              }
            }

            if(pin1Locy > pin0Locy)
            {
              if(pin2Locy > pin1Locy)
              {
                maxy=pin2Locy;
                miny=pin0Locy;
              }
              else
              {
                maxy=pin1Locy;
                if(pin0Locy < pin2Locy)
                  miny = pin0Locy;
                else
                  miny = pin2Locy;
              }
            }
            else
            {
              if(pin2Locy > pin0Locy)
              {
                maxy=pin2Locy;
                miny=pin1Locy;
              }
              else
              {
                maxy=pin0Locy;
                if(pin1Locy < pin2Locy)
                  miny = pin1Locy;
                else
                  miny = pin2Locy;
              }
            }

            if(useWts)
            {
              HPWL+= net->getWeight()*(maxx-minx + maxy-miny);
            }
            else
            {
              HPWL+= maxx-minx + maxy-miny;
            }

//            if(HPWL == numeric_limits<float>::infinity())
//            {
//              const vector<float>& nodexlocs = _nodes->getXLocs();
//              const vector<float>& nodeylocs = _nodes->getYLocs();  
    
//              abkfatal(nodexlocs.size() == nodeylocs.size(), "What the hell am i doing?");

//              cerr<<"There is a problem with HPWL, dumping node locs"<<endl;
//              for(unsigned i = 0; i < nodexlocs.size(); ++i)
//              {
//                cerr<<" node idx: "<<i<<" Loc: "<<nodexlocs[i]<<","<<nodeylocs[i]<<endl;
//              }
        
//              abkfatal( HPWL != numeric_limits<float>::infinity(), "Returning inf HPWL! ");
//            }
        }
        else //degree >= 4
        {
            float maxx=0.0f;
            float maxy=0.0f;
            float minx=0.0f;
            float miny=0.0f;
            float nodeLocx=0.0f; 
            float nodeLocy=0.0f; 
            float pinOffsetx=0.0f; 
            float pinOffsety=0.0f; 
            float halfPerim=0.0f;

            //Begin by initializing the locations to the first four pins
            //faster because it saves some comparing            
            Net& currNet = (*net);
            //Begin Compute location of pin 0
            pin& pin0 = currNet.getPin(0); 
            unsigned nodeIndex0 = pin0.getNodeIndex();
            Node* node0;
            if(pin0.getType())
            {
              node0 = &_nodes->getTerminal(nodeIndex0);
              if(scaleTerms)
              {
                nodeLocx = getScaledX(*node0);
                nodeLocy = getScaledY(*node0);
              }
              else
              {
                nodeLocx = node0->getX();
                nodeLocy = node0->getY();
              }
            }
            else
            {
              node0 = &_nodes->getNode(nodeIndex0);
              nodeLocx = node0->getX();
              nodeLocy = node0->getY();
            }
            //Begin Compute pin offsets 
            width = node0->getWidth();
            height = node0->getHeight();
            if(node0->allPinsAtCenter)
            {
                pinOffsetx = 0.5f*width;
                pinOffsety = 0.5f*height;
            }
            else
            {
                pinOffsetx = (0.5f + pin0.getXOffset())*width;
                pinOffsety = (0.5f + pin0.getYOffset())*height;
            }
            //End Compute pin offsets  
            float pin0Locx = nodeLocx+pinOffsetx;
            float pin0Locy = nodeLocy+pinOffsety;
            //End Compute location of pin 0

            //Begin Compute location of pin 1
            pin& pin1 = currNet.getPin(1); 
            unsigned nodeIndex1 = pin1.getNodeIndex();
            Node* node1;
            if(pin1.getType())
            {
              node1 = &_nodes->getTerminal(nodeIndex1);
              if(scaleTerms)
              {
                nodeLocx = getScaledX(*node1);
                nodeLocy = getScaledY(*node1);
              }
              else
              {
                nodeLocx = node1->getX();
                nodeLocy = node1->getY();
              }
            }
            else
            {
              node1 = &_nodes->getNode(nodeIndex1);
              nodeLocx = node1->getX();
              nodeLocy = node1->getY();
            }
            //Begin Compute pin offsets 
            width = node1->getWidth();
            height = node1->getHeight();
            if(node1->allPinsAtCenter)
            {
                pinOffsetx = 0.5f*width;
                pinOffsety = 0.5f*height;
            }
            else
            {
                pinOffsetx = (0.5f + pin1.getXOffset())*width;
                pinOffsety = (0.5f + pin1.getYOffset())*height;
            }
            //End Compute pin offsets  
            float pin1Locx = nodeLocx + pinOffsetx;
            float pin1Locy = nodeLocy + pinOffsety;
            //End Compute location of pin 1

            //Begin Compute location of pin 2
            pin& pin2 = currNet.getPin(2); 
            unsigned nodeIndex2 = pin2.getNodeIndex();
            Node* node2;
            if(pin2.getType())
            {
              node2 = &_nodes->getTerminal(nodeIndex2);
              if(scaleTerms)
              {
                nodeLocx = getScaledX(*node2);
                nodeLocy = getScaledY(*node2);
              }
              else
              {
                nodeLocx = node2->getX();
                nodeLocy = node2->getY();
              }
            }
            else
            {
              node2 = &_nodes->getNode(nodeIndex2);
              nodeLocx = node2->getX();
              nodeLocy = node2->getY();
            }
            //Begin Compute pin offsets 
            width = node2->getWidth();
            height = node2->getHeight();
            if(node2->allPinsAtCenter)
            {
                pinOffsetx = 0.5f*width;
                pinOffsety = 0.5f*height;
            }
            else
            {
                pinOffsetx = (0.5f + pin2.getXOffset())*width;
                pinOffsety = (0.5f + pin2.getYOffset())*height;
            }
            //End Compute pin offsets  
            float pin2Locx = nodeLocx+pinOffsetx;
            float pin2Locy = nodeLocy+pinOffsety;
            //End Compute location of pin 2

            //Begin Compute location of pin 3
            pin& pin3 = currNet.getPin(3); 
            unsigned nodeIndex3 = pin3.getNodeIndex();
            Node* node3;
            if(pin3.getType())
            {
              node3 = &_nodes->getTerminal(nodeIndex3);
              if(scaleTerms)
              {
                nodeLocx = getScaledX(*node3);
                nodeLocy = getScaledY(*node3);
              }
              else
              {
                nodeLocx = node3->getX();
                nodeLocy = node3->getY();
              }
            }
            else
            {
              node3 = &_nodes->getNode(nodeIndex3);
              nodeLocx = node3->getX();
              nodeLocy = node3->getY();
            }
            //Begin Compute pin offsets 
            width = node3->getWidth();
            height = node3->getHeight();
            if(node3->allPinsAtCenter)
            {
                pinOffsetx = 0.5f*width;
                pinOffsety = 0.5f*height;
            }
            else
            {
                pinOffsetx = (0.5f + pin3.getXOffset())*width;
                pinOffsety = (0.5f + pin3.getYOffset())*height;
            }
            //End Compute pin offsets  
            float pin3Locx = nodeLocx + pinOffsetx;
            float pin3Locy = nodeLocy + pinOffsety;
            //End Compute location of pin 3

            if(pin1Locx > pin0Locx)
            {
              if(pin3Locx > pin2Locx)
              {
                if(pin3Locx > pin1Locx)
                  maxx = pin3Locx;
                else
                  maxx = pin1Locx;
               
                if(pin2Locx > pin0Locx)
                  minx = pin0Locx;
                else
                  minx = pin2Locx;
              }
              else
              {
                if(pin2Locx > pin1Locx)
                  maxx = pin2Locx;
                else
                  maxx = pin1Locx;
               
                if(pin3Locx > pin0Locx)
                  minx = pin0Locx;
                else
                  minx = pin3Locx;
              }
            }
            else
            {
              if(pin3Locx > pin2Locx)
              {
                if(pin3Locx > pin0Locx)
                  maxx = pin3Locx;
                else
                  maxx = pin0Locx;
               
                if(pin2Locx > pin1Locx)
                  minx = pin1Locx;
                else
                  minx = pin2Locx;
              }
              else
              {
                if(pin2Locx > pin0Locx)
                  maxx = pin2Locx;
                else
                  maxx = pin0Locx;
               
                if(pin3Locx > pin1Locx)
                  minx = pin1Locx;
                else
                  minx = pin3Locx;
              }
            }

            if(pin1Locy > pin0Locy)
            {
              if(pin3Locy > pin2Locy)
              {
                if(pin3Locy > pin1Locy)
                  maxy = pin3Locy;
                else
                  maxy = pin1Locy;
               
                if(pin2Locy > pin0Locy)
                  miny = pin0Locy;
                else
                  miny = pin2Locy;
              }
              else
              {
                if(pin2Locy > pin1Locy)
                  maxy = pin2Locy;
                else
                  maxy = pin1Locy;
               
                if(pin3Locy > pin0Locy)
                  miny = pin0Locy;
                else
                  miny = pin3Locy;
              }
            }
            else
            {
              if(pin3Locy > pin2Locy)
              {
                if(pin3Locy > pin0Locy)
                  maxy = pin3Locy;
                else
                  maxy = pin0Locy;
               
                if(pin2Locy > pin1Locy)
                  miny = pin1Locy;
                else
                  miny = pin2Locy;
              }
              else
              {
                if(pin2Locy > pin0Locy)
                  maxy = pin2Locy;
                else
                  maxy = pin0Locy;
               
                if(pin3Locy > pin1Locy)
                  miny = pin1Locy;
                else
                  miny = pin3Locy;
              }
            }
            //end initialize

            //Loop over the rest of the pins in pairs (A,B)  starting at the fifth and sixth pin
            //compare nodes A and B, compare the greater to the max, and the lesser to the min
            //The loop will execute 0 times if there are exactly 5 pins
            unsigned A=0,nodeIndexA=0;
            unsigned B=0,nodeIndexB=0;
            unsigned numLoopIterations = (nDegree - 4)/2; //integer division floor((nDegree-4)/2.0);
   
            for(unsigned loopCt = 0; loopCt < numLoopIterations; ++loopCt)
            {
                A = loopCt*2+0 + 4;
                B = loopCt*2+1 + 4;

                //Begin Compute location of pin A
                pin& pinA = currNet.getPin(A); 
                nodeIndexA = pinA.getNodeIndex();
                Node* nodeA;
                if(pinA.getType())
                {
                  nodeA = &_nodes->getTerminal(nodeIndexA);
                  if(scaleTerms)
                  {
                    nodeLocx = getScaledX(*nodeA);
                    nodeLocy = getScaledY(*nodeA);
                  }
                  else
                  {
                    nodeLocx = nodeA->getX();
                    nodeLocy = nodeA->getY();
                  }
                }
                else
                {
                  nodeA = &_nodes->getNode(nodeIndexA);
                  nodeLocx = nodeA->getX();
                  nodeLocy = nodeA->getY();
                }
                //Begin Compute pin offsets 
                width = nodeA->getWidth();
                height = nodeA->getHeight();
                if(nodeA->allPinsAtCenter)
                {
                    pinOffsetx = 0.5f*width;
                    pinOffsety = 0.5f*height;
                }
                else
                {
                    pinOffsetx = (0.5f + pinA.getXOffset())*width;
                    pinOffsety = (0.5f + pinA.getYOffset())*height;
                }
                //End Compute pin offsets  
                float pinALocx = nodeLocx + pinOffsetx;
                float pinALocy = nodeLocy + pinOffsety;
                //End Compute location of pin A

                //Begin Compute location of pin B
                pin& pinB = currNet.getPin(B); 
                nodeIndexB = pinB.getNodeIndex();
                Node* nodeB;
                if(pinB.getType())
                {
                  nodeB = &_nodes->getTerminal(nodeIndexB);
                  if(scaleTerms)
                  {
                    nodeLocx = getScaledX(*nodeB);
                    nodeLocy = getScaledY(*nodeB);
                  }
                  else
                  {
                    nodeLocx = nodeB->getX();
                    nodeLocy = nodeB->getY();
                  }
                }
                else
                {
                  nodeB = &_nodes->getNode(nodeIndexB);
                  nodeLocx = nodeB->getX();
                  nodeLocy = nodeB->getY();
                }
                //Begin Compute pin offsets 
                width = nodeB->getWidth();
                height = nodeB->getHeight();
                if(nodeB->allPinsAtCenter)
                {
                    pinOffsetx = 0.5f*width;
                    pinOffsety = 0.5f*height;
                }
                else
                {
                    pinOffsetx = (0.5f + pinB.getXOffset())*width;
                    pinOffsety = (0.5f + pinB.getYOffset())*height;
                }
                //End Compute pin offsets  
                float pinBLocx = nodeLocx + pinOffsetx;
                float pinBLocy = nodeLocy + pinOffsety;
                //End Compute location of pin B

                if(pinBLocx > pinALocx)
                {
                  if(pinBLocx > maxx)
                    maxx = pinBLocx;
                  if(pinALocx < minx)
                    minx = pinALocx;
                }
                else 
                {
                  if(pinALocx > maxx) 
                    maxx = pinALocx;
                  if(pinBLocx < minx)
                    minx = pinBLocx;
                }

                if(pinBLocy > pinALocy)
                {
                  if(pinBLocy > maxy)
                    maxy = pinBLocy;
                  if(pinALocy < miny)
                    miny = pinALocy;
                }
                else 
                {
                  if(pinALocy > maxy)
                    maxy = pinALocy;
                  if(pinBLocy < miny)
                    miny = pinBLocy;
                }
            }

            //finally, if nDegree is odd, pick up the last compare
            if(nDegree % 2 == 1)
            {
                //Begin Compute location of pin N
                unsigned N = nDegree-1;
                pin& pinN = currNet.getPin(N); 
                unsigned nodeIndexN = pinN.getNodeIndex();
                Node* nodeN;
                if(pinN.getType())
                {
                  nodeN = &_nodes->getTerminal(nodeIndexN);
                  if(scaleTerms)
                  {
                    nodeLocx = getScaledX(*nodeN);
                    nodeLocy = getScaledY(*nodeN);
                  }
                  else
                  {
                    nodeLocx = nodeN->getX();
                    nodeLocy = nodeN->getY();
                  }
                }
                else
                {
                  nodeN = &_nodes->getNode(nodeIndexN);
                  nodeLocx = nodeN->getX();
                  nodeLocy = nodeN->getY();
                }
                //Begin Compute pin offsets 
                width = nodeN->getWidth();
                height = nodeN->getHeight();
                if(nodeN->allPinsAtCenter)
                {
                    pinOffsetx = 0.5f*width;
                    pinOffsety = 0.5f*height;
                }
                else
                {
                    pinOffsetx = (0.5f + pinN.getXOffset())*width;
                    pinOffsety = (0.5f + pinN.getYOffset())*height;
                }
                //End Compute pin offsets  
                float pinNLocx = nodeLocx + pinOffsetx;
                float pinNLocy = nodeLocy + pinOffsety;
                //End Compute location of pin N

                if(pinNLocx > maxx)
                  maxx = pinNLocx;
                else if (pinNLocx < minx)
                  minx = pinNLocx;
                if(pinNLocy > maxy)
                  maxy = pinNLocy;
                else if (pinNLocy < miny)
                  miny = pinNLocy;
            }

            halfPerim = maxx-minx+maxy-miny;
            if(useWts)
                HPWL += net->getWeight()*halfPerim;
            else
                HPWL += halfPerim;

//            if(HPWL == numeric_limits<float>::infinity())
//            {
//              const vector<float>& nodexlocs = _nodes->getXLocs();
//              const vector<float>& nodeylocs = _nodes->getYLocs();  

//              abkfatal(nodexlocs.size() == nodeylocs.size(), "What the hell am i doing?");

//              cerr<<"There is a problem with HPWL, dumping node locs"<<endl;
//              for(unsigned i = 0; i < nodexlocs.size(); ++i)
//              {
//                cerr<<" node idx: "<<i<<" Loc: "<<nodexlocs[i]<<","<<nodeylocs[i]<<endl;
//              }

//              abkfatal( HPWL != numeric_limits<float>::infinity(), "Returning inf HPWL! ");
//            }
        }
    }

//    if(HPWL == numeric_limits<float>::infinity())
//    {
//      const vector<float>& nodexlocs = _nodes->getXLocs();
//      const vector<float>& nodeylocs = _nodes->getYLocs();

//      abkfatal(nodexlocs.size() == nodeylocs.size(), "What the hell am i doing?");

//      cerr<<"There is a problem with HPWL, dumping node locs"<<endl;
//      for(unsigned i = 0; i < nodexlocs.size(); ++i)
//      {
//        cerr<<" node idx: "<<i<<" Loc: "<<nodexlocs[i]<<","<<nodeylocs[i]<<endl;
//      }

//      abkfatal( HPWL != numeric_limits<float>::infinity(), "Returning inf HPWL! ");
//    }

    return HPWL;
}

#ifdef USEFLUTE
double flute_x[MAXD], flute_y[MAXD];

float DB::evalSteiner(bool useWts, bool scaleTerms)
{
   if(scaleTerms)
   {
     scaleTerminals();
   }

   float total = 0.f;
   vector<Point> pointsOnNet;

   for(itNet net = _nets->netsBegin(); net != _nets->netsEnd(); ++net)
   {
     pointsOnNet.clear();

     for(unsigned i = 0; i < net->getDegree(); ++i)
     {
       const pin& Pin = net->getPin(i);
       unsigned nodeIndex = Pin.getNodeIndex();
       Node* node;
       float nodeLocx, nodeLocy;
       if(Pin.getType())
       {
         node = &_nodes->getTerminal(nodeIndex);
         if(scaleTerms)
         {
           nodeLocx = getScaledX(*node);
           nodeLocy = getScaledY(*node);
         }
         else
         {
           nodeLocx = node->getX();
           nodeLocy = node->getY();
         }
       }
       else
       {
         node = &_nodes->getNode(nodeIndex);
         nodeLocx = node->getX();
         nodeLocy = node->getY();
       }
       //Begin Compute pin offsets
       float width = node->getWidth();
       float height = node->getHeight();
       float pinOffsetx, pinOffsety;
       if(node->allPinsAtCenter)
       {
         pinOffsetx = 0.5f*width;
         pinOffsety = 0.5f*height;
       }
       else
       {
         pinOffsetx = (0.5f + Pin.getXOffset())*width;
         pinOffsety = (0.5f + Pin.getYOffset())*height;
       }
       //End Compute pin offsets
       float pinLocx = nodeLocx+pinOffsetx;
       float pinLocy = nodeLocy+pinOffsety;

       pointsOnNet.push_back(Point(pinLocx, pinLocy));
     }

     sort(pointsOnNet.begin(), pointsOnNet.end());
     vector<Point>::iterator new_end = unique(pointsOnNet.begin(), pointsOnNet.end());
     pointsOnNet.erase(new_end, pointsOnNet.end());

     if(pointsOnNet.size() <= 1)
     { /* do nothing */ }
     else if(pointsOnNet.size() == 2)
     {
       if(useWts)
       {
         total += net->getWeight()*(std::abs(pointsOnNet[0].x-pointsOnNet[1].x)+
                                    std::abs(pointsOnNet[0].y-pointsOnNet[1].y));
       }
       else
       {
         total += std::abs(pointsOnNet[0].x-pointsOnNet[1].x)+
                  std::abs(pointsOnNet[0].y-pointsOnNet[1].y);
       }
     }
     else if(pointsOnNet.size() == 3)
     {
       float minx, maxx, miny, maxy;

       if(pointsOnNet[0].x < pointsOnNet[1].x)
       {
         minx = pointsOnNet[0].x;
         maxx = pointsOnNet[1].x;
       }
       else
       {
         minx = pointsOnNet[1].x;
         maxx = pointsOnNet[0].x;
       }
       if(pointsOnNet[2].x < minx)
       {
         minx = pointsOnNet[2].x;
       }
       else if(pointsOnNet[2].x > maxx)
       {
         maxx = pointsOnNet[2].x;
       }

       if(pointsOnNet[0].y < pointsOnNet[1].y)
       {
         miny = pointsOnNet[0].y;
         maxy = pointsOnNet[1].y;
       }
       else
       {
         miny = pointsOnNet[1].y;
         maxy = pointsOnNet[0].y;
       }
       if(pointsOnNet[2].y < miny)
       {
         miny = pointsOnNet[2].y;
       }
       else if(pointsOnNet[2].y > maxy)
       {
         maxy = pointsOnNet[2].y;
       }

       if(useWts)
       {
         total += net->getWeight()*((maxx - minx) + (maxy - miny));
       }
       else
       {
         total += (maxx - minx) + (maxy - miny);
       }
     }
     else if(pointsOnNet.size() <= MAXD)
     {
       for(unsigned i = 0; i < pointsOnNet.size(); ++i)
       {
         flute_x[i] = static_cast<double>(pointsOnNet[i].x);
         flute_y[i] = static_cast<double>(pointsOnNet[i].y);
       }
       Tree flutetree = flute(pointsOnNet.size(), flute_x, flute_y, ACCURACY);
       if(useWts)
       {
         total += net->getWeight()*static_cast<float>(flutetree.length);
       } 
       else
       {
         total += static_cast<float>(flutetree.length);
       }
       free(flutetree.branch);
     }
     else
     {
       abkfatal(0,"Net too large to use Flute");
     }
   }

   return total;
}
#endif

float DB::evalArea(void) const
{
   BBox area;
   itNode node;
   Point P;
   for(node = const_cast<DB*>(this)->getNodes()->nodesBegin();
       node != const_cast<DB*>(this)->getNodes()->nodesEnd(); ++node)
   {
      P.x = node->getX();
      P.y = node->getY();
      area.put(P);
      P.x = node->getX()+node->getWidth();
      P.y = node->getY()+node->getHeight();
      area.put(P);
   }
   return(area.getXSize()*area.getYSize());
}

float DB::getXSize(void) const
{
   BBox xSize;
   itNode node;
   Point P;
   for(node = const_cast<DB*>(this)->getNodes()->nodesBegin();
       node != const_cast<DB*>(this)->getNodes()->nodesEnd(); ++node)
   {
      P.x = node->getX();
      P.y = 0;
      xSize.put(P);
      P.x = node->getX()+node->getWidth();
      P.y = 0;
      xSize.put(P);
   }
   return(xSize.getXSize());
}

float DB::getYSize(void) const
{
   BBox ySize;
   itNode node;
   Point P;
   for(node = const_cast<DB*>(this)->getNodes()->nodesBegin();
       node != const_cast<DB*>(this)->getNodes()->nodesEnd(); ++node)
   {
      P.y = node->getY();
      P.x = 0;
      ySize.put(P);
      P.y = node->getY()+node->getHeight();
      P.x = 0;
      ySize.put(P);
   }
   return(ySize.getYSize());
}

void DB::plot(const char* fileName, float area, float whitespace, 
    float aspectRatio, float time, float HPWL, bool plotSlacks,
    bool plotNets, bool plotNames, bool fixedOutline, 
    float ll, float ly, float ux, float uy) const
{
  float x=0;
  float y=0;
  float w=0;
  float h=0;
  float nodesArea = getNodesArea();
  float starDelta = sqrt(nodesArea) / 200;
  itNode it;

  cout<<"OutPut Plot file is "<<fileName<<endl;
  ofstream gpOut(fileName);
  if (!gpOut.good())
  {
    cout << "Warning: output file " << fileName
      << " can't be opened" << endl;
  }
  //   gpOut <<"set terminal png size 1024,768" << endl;

  gpOut<<"#Use this file as a script for gnuplot"<<endl;
  gpOut<<"#(See http://www.gnuplot.info/ for details)"<<endl;
  gpOut << "set nokey"<<endl;

  gpOut << "set size ratio -1" << endl;
  gpOut << "set title ' " << fileName
    << " area= " << area << " WS= " << whitespace << "%" << " AR= " << aspectRatio
    << " time= " << time << "s" << " HPWL= " << HPWL << endl << endl;

  gpOut<<"#   Uncomment these two lines starting with \"set\""<<endl ;
  gpOut<<"#   to save an EPS file for inclusion into a latex document"<<endl;
  gpOut << "# set terminal postscript eps color solid 10"<<endl;
  gpOut << "# set output \"out.eps\""<<endl<<endl<<endl;

  gpOut<<"#   Uncomment these two lines starting with \"set\""<<endl ;
  gpOut<<"#   to save a PS file for printing"<<endl;
  gpOut<<"# set terminal postscript portrait color solid 8"<<endl;
  gpOut << "# set output \"out.ps\""<<endl<<endl<<endl;

  if(fixedOutline) {
    gpOut << "set xrange[" << ll << ":" << ux << "]" << endl;
    gpOut << "set yrange[" << ly << ":" << uy << "]" << endl;
  }

  if(plotNames)
  {
    for(it=_nodes->nodesBegin();it!=_nodes->nodesEnd();++it)
    {
      gpOut<<"set label '"<<it->getName()<<"("<<(it-_nodes->nodesBegin())<<")"
        <<"'noenhanced at "<<it->getX()+it->getWidth()/5<<" , "<<it->getY()+it->getHeight()/4<<endl;
    }

    // plot terminals only when Net is plotted
    if (plotNets)
    {
      for(it=_nodes->terminalsBegin();it!=_nodes->terminalsEnd();++it)
      {
        gpOut<<"set label \""<<it->getName()<<"\" at "<<it->getX()+it->getWidth()/4<<" , "<<it->getY()+it->getHeight()/4<<endl;
      }
    }
  }


  if(plotSlacks)
  {
    for(it=_nodes->nodesBegin();it!=_nodes->nodesEnd();++it)
    {
      float xSlack = it->getslackX();
      float ySlack = it->getslackY();
      if(xSlack < 1e-5)
        xSlack = 0;
      if(ySlack < 1e-5)
        ySlack = 0;

      gpOut.precision(4);
      gpOut<<"set label \"x "<<xSlack<<"\" at "<<it->getX()+it->getWidth()/6<<" , "<<it->getY()+it->getHeight()/2<<endl;
      gpOut<<"set label \"y "<<ySlack<<"\" at "<<it->getX()+it->getWidth()/6<<" , "<<it->getY()+it->getHeight()*3/4<<endl;
    }
  }

  int objCnt = 0; 
  // Blockage drawing
  for(it=_obstacles->nodesBegin();it!=_obstacles->nodesEnd();++it)
  {
    x=it->getX();
    y=it->getY();
    w=it->getWidth();
    h=it->getHeight();

    gpOut << "set object " << ++objCnt
      << " rect from " << x << "," << y << " to " << x+w << "," << y+h 
      << " fc rgb \"gold\"" << endl;
  }

  gpOut.precision(6);

  // HALO drawing
  for(it=_nodes->nodesBegin(); it!=_nodes->nodesEnd(); ++it)
  {
    x = it->getX();
    y = it->getY(); 

    x = (x - it->getHaloX() >= 0)? x - it->getHaloX() : 0;
    y = (y - it->getHaloY() >= 0)? y - it->getHaloY() : 0;

    float ux = it->getWidth() + it->getX() + it->getHaloX();
    float uy = it->getHeight() + it->getY() + it->getHaloY();

    //      gpOut<<x<<" "<<y<<endl;
    //      gpOut<<ux<<" "<<y<<endl;
    //      gpOut<<ux<<" "<<uy<<endl;
    //      gpOut<<x<<" "<<uy<<endl;
    //      gpOut<<x<<" "<<y<<endl<<endl;
    gpOut << "set object " << ++objCnt
      << " rect from " << x << "," << y << " to " << ux << "," << uy 
      << " fc rgb \"#DCDCDCBB\"" << endl;
  }

  // block drawing
  for(it=_nodes->nodesBegin();it!=_nodes->nodesEnd();++it)
  {
    x=it->getX();
    y=it->getY();
    w=it->getWidth();
    h=it->getHeight();

    //      gpOut<<x<<" "<<y<<endl;
    //      gpOut<<x+w<<" "<<y<<endl;
    //      gpOut<<x+w<<" "<<y+h<<endl;
    //      gpOut<<x<<" "<<y+h<<endl;
    //      gpOut<<x<<" "<<y<<endl<<endl;

    gpOut << "set object " << ++objCnt
      << " rect from " << x << "," << y << " to " << x+w << "," << y+h 
      << " fc rgb \"#808080BB\"" << endl;
  }


  gpOut << "plot '-' w l" << endl;

  if(plotNets)
  {
    float width;
    float height;
    float absOffsetX;
    float absOffsetY;

    itNet net;
    itPin netPin;
    for(net = _nets->netsBegin(); net != _nets->netsEnd(); ++net)
    {
      Point starPoint;
      starPoint.x = 0;
      starPoint.y = 0;
      unsigned netDegree = 0;
      for(netPin = net->pinsBegin(); netPin != net->pinsEnd(); netPin++)
      {
        Node* nodep;
        if(!netPin->getType())  //if not terminal
          nodep = &_nodes->getNode(netPin->getNodeIndex());
        else
          nodep = &_nodes->getTerminal(netPin->getNodeIndex());
        Node node = *nodep;

        width = node.getWidth();
        height = node.getHeight();
        absOffsetX = width/2 + (netPin->getXOffset()*width);
        absOffsetY = height/2 + (netPin->getYOffset()*height);
        starPoint.x += node.getX() + absOffsetX;
        starPoint.y += node.getY() + absOffsetY;
        ++netDegree;
      }

      if(netDegree != 0)
      {
        starPoint.x /= netDegree;
        starPoint.y /= netDegree;
        for(netPin = net->pinsBegin(); netPin != net->pinsEnd(); netPin++)
        {
          Node* nodep;
          if(!netPin->getType())
            nodep = &_nodes->getNode(netPin->getNodeIndex());
          else
            nodep = &_nodes->getTerminal(netPin->getNodeIndex());
          Node node = *nodep;

          width = node.getWidth();
          height = node.getHeight();
          absOffsetX = width/2 + (netPin->getXOffset()*width);
          absOffsetY = height/2 + (netPin->getYOffset()*height);
          gpOut<<starPoint.x<<"  "<<starPoint.y<<endl;
          gpOut<<node.getX()+absOffsetX<<"  "<<node.getY()+absOffsetY<<endl;
          gpOut<<starPoint.x<<"  "<<starPoint.y<<endl<<endl;

          gpOut<<starPoint.x-starDelta<<"  "<<starPoint.y<<endl;
          gpOut<<starPoint.x<<"  "<<starPoint.y+starDelta<<endl;
          gpOut<<starPoint.x+starDelta<<"  "<<starPoint.y<<endl;
          gpOut<<starPoint.x<<"  "<<starPoint.y-starDelta<<endl;
          gpOut<<starPoint.x-starDelta<<"  "<<starPoint.y<<endl<<endl;

        }
      }
      else
      {
        cout << "Warning: net with zero degree detected." << endl;
      }
    }
  }
  gpOut << "EOF"<<endl<<endl; 
  gpOut << "pause -1 'Press any key' "<<endl;
  gpOut.close();  
}


void DB::saveCapo(const char* baseFileName, const BBox &nonTrivialBBox,
                  float reqdAR, float reqdWS) const
{
   cout<<"Saving in Capo Format "<<baseFileName<<endl;
   _nodes->saveCapoNodes(baseFileName);
   _nodes->saveCapoPl(baseFileName);
   saveCapoNets(baseFileName);
   _nodes->saveCapoScl(baseFileName, reqdAR, reqdWS, nonTrivialBBox);

   // check for soft blocks
   bool anySoft = false;
   for(unsigned i = 0; i < getNumNodes(); ++i)
   {
     if(isNodeSoft(i)) { anySoft = true; break; }
   }

   if(anySoft) { _nodes->saveNodes(baseFileName); }

   //save the aux file now
   char fileName[1024];
   strcpy(fileName, baseFileName);
   strcat(fileName, ".aux");
   ofstream aux(fileName);
   aux<<"RowBasedPlacement : "<<baseFileName<<".nodes "<<baseFileName<<".nets ";
   if(anySoft) { aux<<baseFileName<<".blocks "; }
   aux<<baseFileName<<".pl "<<baseFileName<<".scl ";
   aux<<endl;
   aux.close();     
}

void DB::save(const char* baseFileName) const
{
   cout<<"Saving in Floorplan Format "<<baseFileName<<endl;
   _nodes->saveNodes(baseFileName);
   _nodes->savePl(baseFileName);
   saveNets(baseFileName);
   saveWts(baseFileName);
}

void DB::saveCapoNets(const char* baseFileName) const
{
   Nets* nets;
   nets = const_cast<DB*>(this)->getNets();
   float absOffsetX;
   float absOffsetY;
   float width;
   float height;
   float temp;
   int nodeIndex;
  
   char fileName[1024];
   strcpy(fileName, baseFileName);
   strcat(fileName, ".nets");
   ofstream file(fileName);

   file<<"UCLA nets   1.0"<<endl<<endl<<endl;
   file<<"NumNets : "<<nets->getNumNets()<<endl;
   file<<"NumPins : "<<nets->getNumPins()<<endl<<endl;
  
   itNet net;
   itPin pin;
   for(net = nets->netsBegin(); net != nets->netsEnd(); ++net)
   {
      file<<"NetDegree : "<<net->_pins.size()<<"\t"<<net->getName()<<endl;
      for(pin = net->pinsBegin(); pin != net->pinsEnd(); ++pin)
      {
         nodeIndex = pin->getNodeIndex();
         if(!pin->getType())        //if not terminal
         {
            Node& node = _nodes->getNode(nodeIndex);
            width = node.getWidth();
            height = node.getHeight();
         }                      
         else
         {
            Node& node = _nodes->getTerminal(nodeIndex);
            width = node.getWidth();
            height = node.getHeight();
         }
         if(int(pin->getOrient())%2 == 1)
         {
            temp = width;
            width = height;
            height = temp;
         }
         absOffsetX = (pin->getOrigXOffset()*width);
         absOffsetY = (pin->getOrigYOffset()*height);
          
         file<<"\t"<<pin->getName()<<" B : \t"<<absOffsetX<<"\t "<<absOffsetY<<endl;
      }
   }
   file.close();
}

void DB::saveNets(const char* baseFileName) const
{
   Nets* nets;
   nets = const_cast<DB*>(this)->getNets();

   char fileName[1024];
   strcpy(fileName, baseFileName);
   strcat(fileName, ".nets");
   ofstream file(fileName);

   file<<"UCLA nets   1.0"<<endl<<endl<<endl;
   file<<"NumNets : "<<nets->getNumNets()<<endl;
   file<<"NumPins : "<<nets->getNumPins()<<endl<<endl;
  
   itNet net;
   itPin pin;
   for(net = nets->netsBegin(); net != nets->netsEnd(); ++net)
   {
     file<<"NetDegree : "<<net->_pins.size()<<"  "<<net->getName()<<endl;
      for(pin = net->pinsBegin(); pin != net->pinsEnd(); ++pin)
      {
         file<<pin->getName()<<" B : \t%"<<pin->getOrigXOffset()*100<<
            "\t %"<<pin->getOrigYOffset()*100<<endl;
      }
   }
   file.close();
}

void DB::saveWts(const char* baseFileName) const
{
   Nets* nets;
   nets = const_cast<DB*>(this)->getNets();

   char fileName[1024];
   strcpy(fileName, baseFileName);
   strcat(fileName, ".wts");
   ofstream file(fileName);

   file<<"UCLA wts   1.0"<<endl<<endl<<endl;
  
   itNet net;
//   itPin pin;
   for(net = nets->netsBegin(); net != nets->netsEnd(); ++net)
     {
       file<<net->getName()<<"\t"<<net->getWeight()<<endl;
     }
   file.close();
}

void DB::shiftDesign(const parquetfp::Point& offset)
{
   itNode node;
   for(node = _nodes->nodesBegin(); node != _nodes->nodesEnd(); ++node)
   {
      node->putX(node->getX() + offset.x);
      node->putY(node->getY() + offset.y);
   }
}

void DB::shiftTerminals(const parquetfp::Point& offset)
{
   for(itNode term = _nodes->terminalsBegin(); term != _nodes->terminalsEnd(); ++term)
   {
      term->putX(term->getX() + offset.x);
      term->putY(term->getY() + offset.y);
   }
}

void DB::expandDesign(float maxWidth, float maxHeight)
{
   float currWidth = getXMax();
   float currHeight = getYMax();
   if(currWidth > maxWidth && currHeight > maxHeight)
      return;

   float xExpRatio=1;
   float yExpRatio=1;

   if(currWidth < maxWidth)
      xExpRatio = maxWidth/currWidth;
   if(currHeight < maxHeight)
      yExpRatio = maxHeight/currHeight;

   itNode node;
   float newLoc;
   for(node = getNodes()->nodesBegin(); node != getNodes()->nodesEnd(); ++node)
   {
      newLoc = node->getX()*xExpRatio;
      node->putX(newLoc);
      newLoc = node->getY()*yExpRatio;
      node->putY(newLoc);
   }
}


void DB::saveInBestCopy(void)
{
   successAR = 1;
   itNode node;
   itNode nodeBest;
   nodeBest = _nodesBestCopy->nodesBegin();
   for(node = _nodes->nodesBegin(); node != _nodes->nodesEnd(); node++)
   {
      nodeBest->putX(node->getX());
      nodeBest->putY(node->getY());
      nodeBest->putWidth(node->getWidth());
      nodeBest->putHeight(node->getHeight());
      nodeBest->putOrient(node->getOrient());
      nodeBest++;
   }
}

void DB::saveBestCopyPl(char* fileName) const
{
   _nodesBestCopy->savePl(fileName);
}

void DB::markTallNodesAsMacros(float maxHeight)
{
   itNode node;
   for(node = _nodes->nodesBegin(); node != _nodes->nodesEnd(); ++node)
   {
      if(node->getHeight() > maxHeight)
         node->updateMacroInfo(true);
   }
}

void DB::reduceCoreCellsArea(float layoutArea, float maxWS)
{
   float currNodesArea = getNodesArea();
   float currWS = (layoutArea - currNodesArea)/currNodesArea;

   if(currWS > maxWS)
      return;

   float macroArea = 0;
   itNode node;
   for(node = _nodes->nodesBegin(); node != _nodes->nodesEnd(); ++node)
   {
      if(node->isMacro())
         macroArea += node->getHeight()*node->getWidth();
   }

   float newCtoOldCRatio = ((layoutArea - (1+maxWS)*macroArea)*(1+currWS))/
      ((layoutArea - (1+currWS)*macroArea)*(1+maxWS));

   for(node = _nodes->nodesBegin(); node != _nodes->nodesEnd(); ++node)
   {
      if(!node->isMacro())
      {
         float nodeWidth = node->getWidth();
         float newNodeWidth = nodeWidth*newCtoOldCRatio;
         node->putWidth(newNodeWidth);
      }
   }
  
   _initArea = false;
   float newNodesArea = getNodesArea();

   currWS = (layoutArea - newNodesArea)/newNodesArea;
}

float DB::getXMax(void)
{
   float xMax = -numeric_limits<float>::max();
   for(itNode node = _nodes->nodesBegin(); node != _nodes->nodesEnd(); ++node)
   {
      xMax = max(xMax, node->getX()+node->getWidth());
   }

   return xMax;
}

float DB::getXMaxWMacroOnly(void)
{
   float xMax = -numeric_limits<float>::max();
   for(itNode node = _nodes->nodesBegin(); node != _nodes->nodesEnd(); ++node)
   {
      if(node->isMacro() && node->needsFP())
      {
         xMax = max(xMax, node->getX()+node->getWidth());
      }
   }

   return xMax;
}

float DB::getXSizeWMacroOnly(void)
{
   BBox xSize;
   itNode node;
   Point P;
   for(node = _nodes->nodesBegin(); node != _nodes->nodesEnd(); ++node)
   {
      if(node->isMacro() && node->needsFP())
      {
         P.x = node->getX();
         P.y = 0;
         xSize.put(P);
         P.x = node->getX()+node->getWidth();
         P.y = 0;
         xSize.put(P);
      }
   }
   if(xSize.isValid())
      return (xSize.getXSize());
   else
      return 0;
}

float DB::getYMax(void)
{
   float yMax = -numeric_limits<float>::max();
   for(itNode node = _nodes->nodesBegin(); node != _nodes->nodesEnd(); ++node)
   {
      yMax = max(yMax, node->getY()+node->getHeight());
   }

   return yMax;
}

float DB::getYMaxWMacroOnly(void)
{
   float yMax = -numeric_limits<float>::max();
   for(itNode node = _nodes->nodesBegin(); node != _nodes->nodesEnd(); ++node)
   {
      if(node->isMacro() && node->needsFP())
      {
         yMax = max(yMax, node->getY()+node->getHeight());
      }
   }

   return yMax;
}


float DB::getYSizeWMacroOnly(void)
{
   BBox ySize;
   itNode node;
   Point P;
   for(node = _nodes->nodesBegin(); node != _nodes->nodesEnd(); ++node)
   {
      if(node->isMacro() && node->needsFP())
      {
         P.y = node->getY();
         P.x = 0;
         ySize.put(P);
         P.y = node->getY()+node->getHeight();
         P.x = 0;
         ySize.put(P);
     }
   }
   if(ySize.isValid())
      return (ySize.getYSize());
   else
      return 0;
}

// --------------------------------------------------------
#ifdef USEFLUTE
void DB::shiftOptimizeDesign(float outlineWidth, float outlineHeight,
                             bool scaleTerms, bool useSteiner,
                             Verbosity verb)
#else
void DB::shiftOptimizeDesign(float outlineWidth, float outlineHeight,
                             bool scaleTerms, Verbosity verb)
#endif
{
   if (_nodes->getNumTerminals() == 0)
      return;   

   // original bottomLeft corner of the floorplan
   Point outlineBottomLeft(getBottomLeftCorner());
   Point outlineTopRight;
   outlineTopRight.x = outlineBottomLeft.x + outlineWidth;
   outlineTopRight.y = outlineBottomLeft.y + outlineHeight;

#ifdef USEFLUTE
   shiftOptimizeDesign(outlineBottomLeft, outlineTopRight, scaleTerms, useSteiner, verb);
#else
   shiftOptimizeDesign(outlineBottomLeft, outlineTopRight, scaleTerms, verb);
#endif
}
// --------------------------------------------------------
#ifdef USEFLUTE
void DB::shiftOptimizeDesign(const parquetfp::Point& outlineBottomLeft,
                             const parquetfp::Point& outlineTopRight,
                             bool scaleTerms, bool useSteiner, Verbosity verb)
#else
void DB::shiftOptimizeDesign(const parquetfp::Point& outlineBottomLeft,
                             const parquetfp::Point& outlineTopRight,
                             bool scaleTerms, Verbosity verb)
#endif

{
   float designWidth = getXMax();
   float designHeight = getYMax();

   float outlineWidth = outlineTopRight.x - outlineBottomLeft.x;
   float outlineHeight = outlineTopRight.y - outlineBottomLeft.y;

   if (designWidth < outlineWidth && designHeight < outlineHeight)
   {
      Point currBottomLeft(getBottomLeftCorner());
      Point minOffset;
      minOffset.x = outlineBottomLeft.x - currBottomLeft.x;
      minOffset.y = outlineBottomLeft.y - currBottomLeft.y;

      Point currTopRight(getTopRightCorner());
      Point maxOffset;
      maxOffset.x = outlineTopRight.x - currTopRight.x;
      maxOffset.y = outlineTopRight.y - currTopRight.y;

      float xRangeStart = getOptimalRangeStart(true);
      xRangeStart = max(xRangeStart, minOffset.x);
      xRangeStart = min(xRangeStart, maxOffset.x);

      float yRangeStart = getOptimalRangeStart(false);
      yRangeStart = max(yRangeStart, minOffset.y);
      yRangeStart = min(yRangeStart, maxOffset.y);

      // the magnitude of shifting (not the final pos)
      Point offset;
      offset.x = xRangeStart;
      offset.y = yRangeStart;

      if(verb.getForMajStats() > 0)
      {
        printf("currBBox: %.2f %.2f %.2f %.2f\n",
               currBottomLeft.x, currBottomLeft.y,
               currTopRight.x, currTopRight.y);
        printf("outBBox: %.2f %.2f %.2f %.2f\n",
               outlineBottomLeft.x, outlineBottomLeft.y,
               outlineTopRight.x, outlineTopRight.y);
        cout << "offset.x: " << offset.x
             << " offset.y: " << offset.y << endl;
      }

      bool useWts = true;
#ifdef USEFLUTE
      float origHPWL = evalHPWL(useWts,scaleTerms,useSteiner);
#else
      float origHPWL = evalHPWL(useWts,scaleTerms);
#endif
      if(verb.getForMajStats() > 0)
        cout << "HPWL before shifting: " << origHPWL << endl;

      shiftDesign(offset);

#ifdef USEFLUTE
      float newHPWL = evalHPWL(useWts,scaleTerms,useSteiner);
#else
      float newHPWL = evalHPWL(useWts,scaleTerms);
#endif
      if(verb.getForMajStats() > 0)
        cout << "HPWL after shifting: " << newHPWL << endl;

      if (origHPWL < newHPWL)
      {
         if(verb.getForMajStats() > 0)
           cout << "Undo-ing the shift..." << endl;
         offset.x = -offset.x;
         offset.y = -offset.y;
         shiftDesign(offset);
      }

#ifdef USEFLUTE
      float finalHPWL = evalHPWL(useWts,scaleTerms,useSteiner);
#else
      float finalHPWL = evalHPWL(useWts,scaleTerms);
#endif
      if(verb.getForMajStats() > 0)
        cout << "Final HPWL: " << finalHPWL << endl;
   }
   else
     {
       /* Saurabh: The below code is not general enough. only valid
	  when relevant nodes are marked as macros. So I am connenting
	  it out. only used for printing out a message anyway */
       /*
       float tempXSize = getXMaxWMacroOnly();
       float tempYSize = getYMaxWMacroOnly();
       if(tempXSize > 1e-5 && tempYSize > 1e-5)
	 {
	   designHeight = tempYSize;
	   designWidth = tempXSize;
	 }
       if (!(designWidth <= outlineWidth &&
	     designHeight <= outlineHeight))
       */
       if(verb.getForMajStats() > 0)
         cout << "No shifting for HPWL minimization is performed. " << endl;
     }
}
// --------------------------------------------------------
float DB::getOptimalRangeStart(bool horizontal)
{
   float center = (horizontal)
      ? getXMax()*0.5f : getYMax()*0.5f;
   
   vector<float> endPoints;
   for (itNode currBlk = _nodes->nodesBegin();
        currBlk != _nodes->nodesEnd(); currBlk++)
   {
      float currNodeLength = (horizontal)
         ? currBlk->getWidth() : currBlk->getHeight();

      float currNodeLocAbs = (horizontal)
         ? currBlk->getX() : currBlk->getY();
      
      for (itNodePin currNodePin = currBlk->pinsBegin();
           currNodePin != currBlk->pinsEnd(); currNodePin++)
      {
         // find the net in which this pin lies
         unsigned currNetIndex = currNodePin->netIndex;
         Net& currNet = _nets->getNet(currNetIndex);

         // index number for the pin in its net
         unsigned currPinOffset = currNodePin->pinOffset;

         // pins offset from the center
         float pinOffset = (horizontal)
            ? currNet.getPin(currPinOffset).getXOffset()
            : currNet.getPin(currPinOffset).getYOffset();

         // absolute location of the pin
         float pinLocationAbs =
            currNodeLocAbs +
            currNodeLength/2 + (currNodeLength * pinOffset);

         // relative location wrt center of the FLOORPLAN
         float pinLocationWrtCenter = pinLocationAbs - center;

         // span of the net in this direction wrt this pin
		 float spanStart = numeric_limits<float>::max();
		 float spanEnd = -numeric_limits<float>::max();
         
         for (itPin currPad = currNet.pinsBegin();
              currPad != currNet.pinsEnd(); currPad++)
         {
            if (currPad->getType()) // true: pad/terminal
            {
               int padIndex = currPad->getNodeIndex();
               const Node& currTerm = _nodes->getTerminal(padIndex);

               float padLoc = (horizontal)
                  ? currTerm.getX() : currTerm.getY();

               float padAdjusted = padLoc - pinLocationWrtCenter;

               spanStart = min(spanStart, padAdjusted);
               spanEnd = max(spanEnd, padAdjusted);
            }
         } // end for each pin in currNet

         if (spanStart <= spanEnd)
         {
            endPoints.push_back(spanStart);
            endPoints.push_back(spanEnd);
         }
      } // end for each pin in currNode
   } // end for each node

   sort(endPoints.begin(), endPoints.end());

   // return the median, assuming endPoints[] has even size
   int endPointNum = endPoints.size();
   if(endPointNum < 2)
      {
        abkwarn(0,"Could not find optimal ranges.");
        return 0;
      }
   abkwarn(endPointNum % 2 == 0, "size of endPoints is not even.");
   return (endPoints[(endPointNum/2) - 1] + endPoints[endPointNum/2]) / 2;
}


void DB::resizeHardBlocks(float ratio)
{
   float factor = sqrt(ratio);

   for (itNode currBlk = _nodes->nodesBegin(); currBlk != _nodes->nodesEnd(); ++currBlk)
   {
     if(!currBlk->getType() && currBlk->isHard()) // examine only hard blocks that are not pads
     {
       currBlk->putArea(ratio*currBlk->getArea());
       currBlk->putHeight(factor*currBlk->getHeight());
       currBlk->putWidth(factor*currBlk->getWidth());
     }
   }

   _area=_nodes->getNodesArea();
}

// --------------------------------------------------------
               
}
