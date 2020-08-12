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
#include "DB.h"
#include "AnalytSolve.h"
#include "CommandLine.h"
#include <cmath>

using namespace parquetfp;
using std::vector;

AnalytSolve::AnalytSolve(Command_Line* params, DB* db)
	   :_params(params), _db(db)
{}

parquetfp::Point AnalytSolve::getDesignOptLoc()
{
  Point finalLoc;
  Nodes *nodes = _db->getNodes();
  Nets *nets = _db->getNets();
  itNode node;
  itNodePin nodePin;
  itPin netPin;
  vector<bool> netsSeen(nets->getNumNets(), 0);
  
  float xSum = 0;
  float ySum = 0;
  float netXSum = 0;
  float netYSum = 0;
  unsigned netDegree = 0;
  unsigned nodeDegree = 0;
  bool pinType=0;
  float xOffset;
  float yOffset;

  for(node = nodes->terminalsBegin(); node != nodes->terminalsEnd(); ++node)
  {
  for(nodePin = node->pinsBegin(); nodePin != node->pinsEnd(); ++nodePin)
   {
     unsigned netIndex = nodePin->netIndex;
     if(!netsSeen[netIndex])
     {
       ++nodeDegree;
       netsSeen[netIndex] = 1;
       Net& net = nets->getNet(netIndex);
     
       netDegree = 0;
       netXSum = 0;
       netYSum = 0;
     
       for(netPin = net.pinsBegin(); netPin != net.pinsEnd(); ++netPin)
         {
           pinType = netPin->getType();	
           int currNodeIndex = netPin->getNodeIndex();
	
	   if(!pinType)  //if not terminal
	    {
	      Node& currNode = nodes->getNode(currNodeIndex);
	      float width = currNode.getWidth();
	      float height = currNode.getHeight();
	      xOffset = netPin->getXOffset()*width + width/2;
	      yOffset = netPin->getYOffset()*height + height/2;     
	      netXSum -= (currNode.getX() +xOffset);
	      netYSum -= (currNode.getY() +yOffset);
	    }
	   else
	    {
	      ++netDegree;
	      Node& terminal = nodes->getTerminal(currNodeIndex);
	      netXSum += terminal.getX();
	      netYSum += terminal.getY();
	    }
         }
       if(netDegree > 0)
        {
          netXSum = netXSum/netDegree;
	  netYSum = netYSum/netDegree;
	  xSum += netXSum;
	  ySum += netYSum;
        }
     }
   }
  }
  if(nodeDegree != 0)
   {
     xSum /= nodeDegree;
     ySum /= nodeDegree;
     finalLoc.x = xSum;
     finalLoc.y = ySum;
   }
  else
   {
     finalLoc.x = 0;
     finalLoc.y = 0;
   }
  /*
   if(finalLoc.x < 0)
    finalLoc.x = 0;
   if(finalLoc.y < 0)
    finalLoc.y = 0;
    */
 return finalLoc;
}



parquetfp::Point AnalytSolve::getOptLoc(int index, vector<float>& xloc, 
	                   vector<float>& yloc)
{
  Point finalLoc;
  Nodes *nodes = _db->getNodes();
  Nets *nets = _db->getNets();
  Node& node = nodes->getNode(index);
  itNodePin nodePin;
  itPin netPin;
  float xSum = 0;
  float ySum = 0;
  float netXSum = 0;
  float netYSum = 0;
  unsigned netDegree = 0;
  unsigned nodeDegree = node.getDegree();
  bool pinType=0;
  float xOffset;
  float yOffset;

  float nodeXOffset;
  float nodeYOffset;
  float nodeWidth = node.getWidth();
  float nodeHeight= node.getHeight();
  
  for(nodePin = node.pinsBegin(); nodePin != node.pinsEnd(); ++nodePin)
   {
     unsigned netIndex = nodePin->netIndex;
     Net& net = nets->getNet(netIndex);
     unsigned pinIndex = nodePin->pinOffset;
     pin& thisPin = net.getPin(pinIndex);
     nodeXOffset = thisPin.getXOffset()*nodeWidth + nodeWidth/2;
     nodeYOffset = thisPin.getYOffset()*nodeHeight + nodeHeight/2;
     
     netDegree = net.getDegree();
     netXSum = 0;
     netYSum = 0;
     
     for(netPin = net.pinsBegin(); netPin != net.pinsEnd(); ++netPin)
      {
        pinType = netPin->getType();	
        int currNodeIndex = netPin->getNodeIndex();
	
        if(!pinType && currNodeIndex == index)
	 { 
	   --netDegree;
	   //cout<<"currNodeIndex "<<currNodeIndex<<" index "<<index<<endl;
	 }
	else
	 {
	   //cout<<"not currNodeIndex "<<currNodeIndex<<" index "<<index<<endl;
	   if(!pinType)  //if not terminal
	    {
	      Node& currNode = nodes->getNode(currNodeIndex);
	      float width = currNode.getWidth();
	      float height = currNode.getHeight();
	      xOffset = netPin->getXOffset()*width + width/2;
	      yOffset = netPin->getYOffset()*height + height/2;     
	      netXSum += (xloc[currNodeIndex] +xOffset-nodeXOffset);
	      netYSum += (yloc[currNodeIndex] +yOffset-nodeYOffset);
	    }
	   else
	    {
	      Node& terminal = nodes->getTerminal(currNodeIndex);
	      netXSum += terminal.getX();
	      netYSum += terminal.getY();
	    }
	 }
      }
     if(netDegree > 0)
      {
        netXSum = netXSum/netDegree;
	netYSum = netYSum/netDegree;
	xSum += netXSum;
	ySum += netYSum;
      }
   }
   
  if(nodeDegree != 0)
   {
     xSum /= nodeDegree;
     ySum /= nodeDegree;
     finalLoc.x = xSum;
     finalLoc.y = ySum;
   }
  else
   {
     finalLoc.x = 0;
     finalLoc.y = 0;
   }
 return finalLoc;

}

void AnalytSolve::solveSOR()
{
  _xloc = _db->getXLocs();
  _yloc = _db->getYLocs();

  float epsilon = sqrt(_db->getNodesArea());
  Point newLoc;
  float change = std::numeric_limits<float>::max();
  float indChange=0.0f;
  unsigned numIter = 0;
  float xchange;
  float ychange;
  float overshoot;
  
  while(change > epsilon && numIter < 1000000)
  {
    numIter++;
    change = 0.0f;
    for(unsigned i=0; i<_xloc.size(); ++i)
     {
       newLoc = getOptLoc(i, _xloc, _yloc);
       xchange = newLoc.x - _xloc[i];
       ychange = newLoc.y - _yloc[i];
       overshoot = xchange*1.7f;
       _xloc[i] += overshoot;
       
       overshoot = ychange*1.7f;
       _yloc[i] += overshoot;
       
       indChange = std::abs(xchange) + std::abs(ychange);
       
       change += indChange;
     }
    //cout<<change<<" ";
  }
}
