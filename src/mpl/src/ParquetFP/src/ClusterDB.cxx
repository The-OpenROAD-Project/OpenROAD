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


#include "CommandLine.h"
#include "FPcommon.h"
#include "DB.h"
#include "ClusterDB.h"

using namespace parquetfp;
using std::map;
using std::cout;
using std::endl;
using std::vector;

ClusterDB::ClusterDB(DB* db, Command_Line *params) :
  _params(params),_db(db),_newDB(0),_oldDB(0),_nodesSeenBB(0),_numConnections(0,0)
{
   //_params = params;
   //_db = db;
   //_oldDB = new DB(_params->minWL);
   //*(_oldDB) = *(_db);
}

void ClusterDB::clusterMulti(DB*& newDB)
{
   unsigned numNewNodes;
   unsigned numOldNodes;
   unsigned layerNum = 1;
   
   bool noCompress=false;
   _oldDB = new DB(*_db, noCompress);
   if(_params->dontClusterMacros)
   {
      float avgNodeHeight = 5*_oldDB->getAvgHeight();
      _oldDB->markTallNodesAsMacros(avgNodeHeight);
   }
 
   unsigned maxTopLevelNodes;
   if(_params->maxTopLevelNodes == -9999) // # top-level nodes
   {
      maxTopLevelNodes = unsigned(2*sqrt(float(_db->getNumNodes())));
      if(maxTopLevelNodes < 50)
         maxTopLevelNodes = 50;
   }
   else
   {
      maxTopLevelNodes = _params->maxTopLevelNodes;
      if(maxTopLevelNodes <= 0)
      {
         cout<<"maxTopLevelNodes has to be > 0"<<endl;
         exit(0);
      }
   }

   if (_params->lookAheadFP)
   {
      // <aaronnn> cluster hard
      maxTopLevelNodes = std::min(maxTopLevelNodes, 20u);
   }
   
   if(_db->getNumNodes() > maxTopLevelNodes)
   {
      unsigned maxConnId = 1; // ???
      do
      {
         _newDB  = new DB();

         // <aaronnn> don't forget your obstacles when clustering (icky code flow)
         float obstacleFrame[2];
         obstacleFrame[0] = _db->getObstacleFrame()[0];
         obstacleFrame[1] = _db->getObstacleFrame()[1];
         _newDB->addObstacles(_db->getObstacles(), obstacleFrame); 

         numOldNodes  = _oldDB->getNodes()->getNumNodes();
         cluster1Layer(layerNum, maxConnId, maxTopLevelNodes);
	  
         numNewNodes = _newDB->getNodes()->getNumNodes();
        
         delete _oldDB;
         _oldDB = _newDB; 
	  
         ++layerNum;
         if(numOldNodes != numNewNodes && maxConnId > 1)
          {
            if(maxConnId > 10)
             maxConnId -= 4;
            else 
             --maxConnId;
          }
         if(numOldNodes == numNewNodes)
            ++maxConnId;
         if(maxConnId >= unsigned(numOldNodes/4) || maxConnId > 100)
            break;
      }
      while(numNewNodes > maxTopLevelNodes/* && numOldNodes != numNewNodes */);
   }

   _newDB = _oldDB; //next function expects to change _newDB
   addWSPerNode(); //add WS per node

   //compress the created DB now
   bool compressDB = true;
   _newDB = new DB(*_oldDB, compressDB);

   //Transfer ownership of the compressed DB to the caller
   newDB = _newDB; 
   _newDB = 0;
}

ClusterDB::~ClusterDB()
{
   if(_oldDB) delete _oldDB;
}

static bool haveMoreNodesToCluster(bool direction, itNode& nodeIt, Nodes* nodes)
{
	if (direction) {
		if (nodeIt == nodes->nodesEnd())
			return false;
	} else {
		if (nodeIt == nodes->nodesBegin())
			return false;
	}

	return true;
}

static void advanceClusterIt(bool direction, itNode& nodeIt) 
{
	if (direction)
		++nodeIt;
	else 
		--nodeIt;
}

void ClusterDB::cluster1Layer(unsigned layerNum, unsigned maxConnId, unsigned maxTopLevelNodes)
{
	map<unsigned, unsigned> mapping; // actual index -> top-level index

	char blockName[1024];
	unsigned blkCtr;

	Nodes* nodes = _oldDB->getNodes();
	Nets* nets = _oldDB->getNets();
	Nodes* newNodes = _newDB->getNodes();
	Nets* newNets = _newDB->getNets();

	float totalArea = _oldDB->getNodesArea();
	float threshold = 0.2*totalArea;  // upper bd in area for a cluster
	unsigned numNodes = nodes->getNumNodes();
	unsigned newNumNodes = numNodes; // <aaronnn> the # of nodes in the new (clustered) DB
	unsigned currNodeIdx, nextNodeIdx;

	itNode node, nodeBegin;
	static bool direction=false;

	blkCtr = 0;
	vector<bool> seenNodes;
	seenNodes.resize(numNodes);
	fill(seenNodes.begin(), seenNodes.end(), false);

	_nodesSeenBB.reset(numNodes);
	_numConnections.resize(numNodes,0);
	fill(_numConnections.begin(), _numConnections.end(), false);

	// set up the nodes (top-level nodes)
	if(direction || numNodes == 1)
		nodeBegin = nodes->nodesBegin();
	else
		nodeBegin = nodes->nodesEnd()-1;

	for (node = nodeBegin; 
		newNumNodes > maxTopLevelNodes // <aaronnn> don't cluster more than we need to
		 && haveMoreNodesToCluster(direction, node, nodes);
		advanceClusterIt(direction, node))
	{
		bool clusterEverything = !_params->dontClusterMacros;

		Node& currNode = *node;
		currNodeIdx = currNode.getIndex();

		// if the node hasn't been seen, create it
		// otherwise, update existing info
		//if(!seenNodes[currNode.getIndex()] && !currNode.isMacro())
		if (!seenNodes[currNodeIdx] 
				&& (clusterEverything || !currNode.isMacro())
				&& (!currNode.isMacro() || !currNode.needsFP()))
		{
			float currNodeArea = currNode.getArea();

			Node& nextNode = getClosestNode(currNode, nodes, nets, seenNodes,
					maxConnId, direction);
			// both "currNode" and "nextNode" haven't been seen
			if(!seenNodes[nextNode.getIndex()] && 
					(currNodeIdx != (unsigned)nextNode.getIndex()))
			{
				float nextNodeArea = nextNode.getArea();
				float newNodeArea = currNodeArea + nextNodeArea;
				nextNodeIdx = nextNode.getIndex();

				//if(newNodeArea < threshold && !currNode.isMacro() && 
				//		!nextNode.isMacro()) // cluster these nodes
				if(newNodeArea < threshold
						&& (clusterEverything || !nextNode.isMacro())
						&& (!nextNode.isMacro() || !nextNode.needsFP())) // cluster these nodes
				{
					seenNodes[nextNodeIdx] = true;
					seenNodes[currNodeIdx] = true;

					float largeNodeArea = nextNodeArea;
					float largeNodeMinAR = nextNode.getminAR();
					float largeNodeMaxAR = nextNode.getmaxAR();
					float smallNodeArea = currNodeArea;
					float smallNodeMinAR = currNode.getminAR();
					float smallNodeMaxAR = currNode.getmaxAR();
					if (currNodeArea > nextNodeArea) {
						largeNodeArea = currNodeArea;
						largeNodeMinAR = currNode.getminAR();
						largeNodeMaxAR = currNode.getmaxAR();
						smallNodeArea = nextNodeArea;
						smallNodeMinAR = nextNode.getminAR();
						smallNodeMaxAR = nextNode.getmaxAR();
					}

					//float newNodeMinAR = 0.75; 
					//float newNodeMaxAR = 1.5;
					float newNodeMinAR = 0.25f; 
					float newNodeMaxAR = 4.0f;

					// temporary AR calculations until we figure out something smarter
					if (currNode.isMacro() && !nextNode.isMacro()) {
						newNodeMinAR = currNode.getminAR();	
						newNodeMaxAR = currNode.getmaxAR();	
					} else if (!currNode.isMacro() && nextNode.isMacro()) {
						newNodeMinAR = nextNode.getminAR();	
						newNodeMaxAR = nextNode.getmaxAR();	
					} else if (largeNodeArea > smallNodeArea * 36) {
						// clustering something large with something small - assume AR of large
						newNodeMinAR = largeNodeMinAR;	
						newNodeMaxAR = largeNodeMaxAR;	
					} else {//if (largeNodeArea < smallNodeArea * 1.2) {
						// make it squarish
						newNodeMinAR = 0.67f;
						newNodeMaxAR = 1.5f;
					}
					//else {
					//	// get weighted average of ARs
					//	newNodeMinAR = ((largeNodeArea*largeNodeMinAR) + (smallNodeArea*smallNodeMinAR)) 
					//		/ (largeNodeArea + smallNodeArea);
					//	newNodeMaxAR = ((largeNodeArea*largeNodeMaxAR) + (largeNodeArea*smallNodeMaxAR)) 
					//		/ (largeNodeArea + smallNodeArea);
					//}

					sprintf(blockName, "Block_%d_%d",layerNum,blkCtr);
					Node tempNode(blockName, newNodeArea, newNodeMinAR, newNodeMaxAR, blkCtr, false);

					// add both the sub-blocks of "currNode" and "nextNode"
					for(vector<int>::iterator i = currNode.subBlocksBegin();
							i!= currNode.subBlocksEnd(); ++i)
						tempNode.addSubBlockIndex(*i);

					// tempNode.putNeedsFP(false); // don't have to propagate 'needFP' 
					// flag coz only nodes that don't needFP will be
					// clustered, and 'needFP' is false by default

					for(vector<int>::iterator i = nextNode.subBlocksBegin();
							i!= nextNode.subBlocksEnd(); ++i)
						tempNode.addSubBlockIndex(*i);

					newNumNodes--; // <aaronnn> clustering each pair reduces numNodes by 1
				}
				else
				{
					seenNodes[currNodeIdx] = true;
					Node tempNode(currNode.getName(), currNode.getArea(), 
							currNode.getminAR(), currNode.getmaxAR(), 
							blkCtr, false);
					if(currNode.isMacro())
						tempNode.updateMacroInfo(true);
					if(currNode.isOrientFixed())
						tempNode.putIsOrientFixed(true);
					tempNode.putNeedsFP(currNode.needsFP()); // propagate 'needFP'

					for(vector<int>::iterator i = currNode.subBlocksBegin();
							i!= currNode.subBlocksEnd(); ++i)
						tempNode.addSubBlockIndex(*i);

					mapping[currNodeIdx] = blkCtr;
					newNodes->putNewNode(tempNode);
					++blkCtr;
				}
			}
		}
		else if ((!clusterEverything && currNode.isMacro())
				|| (currNode.isMacro() && currNode.needsFP()))
		{
			seenNodes[currNodeIdx] = true;
			Node tempNode(currNode.getName(), currNode.getArea(), 
					currNode.getminAR(), currNode.getmaxAR(), 
					blkCtr, false);
			if(currNode.isMacro())
				tempNode.updateMacroInfo(true);
			if(currNode.isOrientFixed())
				tempNode.putIsOrientFixed(true);
			tempNode.putNeedsFP(currNode.needsFP()); // propagate 'needFP'

			for(vector<int>::iterator i = currNode.subBlocksBegin();
					i!= currNode.subBlocksEnd(); ++i)
				tempNode.addSubBlockIndex(*i);

			mapping[currNodeIdx] = blkCtr;
			newNodes->putNewNode(tempNode);
			++blkCtr;
		}
	}

	direction = !direction;

	//put any remaining nodes in new DB 
	for(node = nodes->nodesBegin(); node != nodes->nodesEnd(); ++node)
	{
		Node& currNode = *node;
		currNodeIdx = currNode.getIndex();
		if(!seenNodes[currNodeIdx])
		{
			seenNodes[currNodeIdx] = true;
			Node tempNode(currNode.getName(), currNode.getArea(),
					currNode.getminAR(), currNode.getmaxAR(),
					blkCtr, false);
			if(currNode.isMacro())
				tempNode.updateMacroInfo(true);
			if(currNode.isOrientFixed())
				tempNode.putIsOrientFixed(true);
			tempNode.putNeedsFP(currNode.needsFP()); // propagate 'needFP'

			for(vector<int>::iterator i = currNode.subBlocksBegin();
					i!= currNode.subBlocksEnd(); ++i)
				tempNode.addSubBlockIndex(*i);
			mapping[currNodeIdx] = blkCtr;
			newNodes->putNewNode(tempNode);
			++blkCtr;
		}
	}

	for(node = nodes->terminalsBegin(); node != nodes->terminalsEnd(); ++node)
		newNodes->putNewTerm(*node);

	//set up the nets now
	addNetsToNewDB(nets, newNets, nodes, newNodes, mapping);
}

Node& ClusterDB::getClosestNode(Node& currNode, Nodes* nodes, Nets* nets,
                                vector<bool>& seenNodes, unsigned maxConnId,
				bool direction)
{
   unsigned numNodes = nodes->getNumNodes();
   Point tempPoint;
   tempPoint.x = 0; // x: node index
   tempPoint.y = 0; // y: incidence w/ "currNode"
   vector<Point> numConnections;
   bool iamDesperate=false;

   unsigned currNodeIdx = currNode.getIndex();
   itNodePin nodePin;

   for(nodePin = currNode.pinsBegin(); nodePin != currNode.pinsEnd(); ++nodePin)
   {
      Net& net = nets->getNet(nodePin->netIndex);
      //if(net.getDegree() < 50)
	{
	  itPin netPin;
	  for(netPin = net.pinsBegin(); netPin != net.pinsEnd(); ++netPin)
	    {
	      if(!netPin->getType()) //not terminal
		{
		  if(unsigned(netPin->getNodeIndex()) != currNodeIdx)
		    {
		      unsigned nodeId = netPin->getNodeIndex();
		      _numConnections[nodeId] += 
			1.0/net.getDegree();
		      _nodesSeenBB.setBit(nodeId);
		    }
		}
	    }
	}
   }

   if(maxConnId < 11)
    {
     const vector<unsigned>& bitsSet = _nodesSeenBB.getIndicesOfSetBits();
     for(unsigned i=0; i<bitsSet.size(); ++i)
      {
       tempPoint.x = float(bitsSet[i]);
       tempPoint.y = _numConnections[bitsSet[i]];
       numConnections.push_back(tempPoint);

       _numConnections[bitsSet[i]] = 0;
      }
    _nodesSeenBB.clear();
    }
   else
    {
     for(unsigned i=0; i<_numConnections.size(); ++i)
      {
       tempPoint.x = float(i);
       tempPoint.y = _numConnections[i];
       numConnections.push_back(tempPoint);
      }
     _nodesSeenBB.clear();
     fill(_numConnections.begin(), _numConnections.end(), 0);
    }

   //sort
   std::sort(numConnections.begin(), numConnections.end(), 
             sortNumConnections());

   unsigned maxConnectionsIdx = 0;
   unsigned maxConnectedNodes = numConnections.size();
   int startingId=0;
   if(maxConnId > maxConnectedNodes)
     startingId = 0;
   else
     startingId = maxConnectedNodes-maxConnId;

   if(maxConnectedNodes > 0)
     {
       for(int i=startingId; i>=0; --i)
	 {
	   maxConnectionsIdx = unsigned(numConnections[i].x);
	   if(seenNodes[maxConnectionsIdx] != 1)
	     {
	       numConnections.clear(); // why need to clear()
	       return nodes->getNode(maxConnectionsIdx);
	     }
	 }
       
       if(maxConnId <= maxConnectedNodes)
	 maxConnectionsIdx = unsigned(numConnections[maxConnectedNodes-maxConnId].x);
       else if(maxConnectedNodes > 0)
	 maxConnectionsIdx = unsigned(numConnections[maxConnectedNodes-1].x);
       else //desperate attempt. return something
	 {
	   iamDesperate = true;
	   if(direction)
	     maxConnectionsIdx = 0;
	   else
	     maxConnectionsIdx = numNodes-1;
	 }
     }
   else
     {
       //desperate attempt. return something
       iamDesperate = true;
       if(direction)
	 maxConnectionsIdx = 0;
       else
	 maxConnectionsIdx = numNodes-1;
     }
   if(iamDesperate && maxConnId > 15)
     {
       /*
	 Node& nextClosestNode = getClosestNodeBFS(currNode, nodes, nets, 
	 seenNodes, maxConnId, direction);
       */
       maxConnectionsIdx = rand()%numNodes;
     }
   numConnections.clear();
   return nodes->getNode(maxConnectionsIdx);
}

/*
Node& ClusterDB getClosestNodeBFS(Node& currNode,
				  Nodes* nodes, Nets* nets,
				  vector<bool>& seenNodes, unsigned maxConnId,
				  bool direction)
{


}
*/

void ClusterDB::addWSPerNode(void)
{
   float multFactor = 1+_params->maxWSHier/100;
   if (!_params->dontClusterMacros)
     multFactor = 0.80f; // <aaronnn> let's make them squishy, but not too squishy
   if (_params->lookAheadFP)
     multFactor = 0.01f; // <aaronnn> squish them hard
   float currArea, newArea, newHeight, newWidth;

   Nodes* newNodes = _newDB->getNodes();
   for(itNode node = newNodes->nodesBegin(); node != newNodes->nodesEnd(); ++node)
   {
      if(node->numSubBlocks() > 1)
      {
         currArea = node->getArea();
         newArea = currArea*multFactor; //add WS
         newWidth = sqrt(newArea*node->getminAR());
         newHeight = newWidth/node->getminAR();
         node->putArea(newArea);
         node->putWidth(newWidth);
         node->putHeight(newHeight);
      }
   }
}

void ClusterDB::addNetsToNewDB(Nets* nets, Nets* newNets, Nodes* nodes, 
                               Nodes* newNodes, map<unsigned, unsigned>& mapping)
{
   int netCtr=0;
   itNet net;

   vector<bool> seenNodes(newNodes->getNumNodes());
   for(net = nets->netsBegin(); net != nets->netsEnd(); ++net)
   {
      Net tempEdge;
      fill(seenNodes.begin(), seenNodes.end(), false);
      tempEdge.putName(net->getName());
      tempEdge.putIndex(netCtr);
      tempEdge.putWeight(net->getWeight());
      for(itPin netPin = net->pinsBegin(); netPin != net->pinsEnd(); ++netPin)
      {
         unsigned currNodeIdx = netPin->getNodeIndex();

         if(!netPin->getType())
         {
            unsigned newNodeIdx = mapping[currNodeIdx];
            Node& newNode = newNodes->getNode(newNodeIdx);
            float poffsetX = 0, poffsetY = 0;
            if(newNode.numSubBlocks() == 1)
            {
               poffsetX = netPin->getXOffset();
               poffsetY = netPin->getYOffset();
            }
            else if(_params->clusterPhysical) //get pinoffsets of subcells
            {
               float xMin = newNode.getX();
               float yMin = newNode.getY();
               float xMax = xMin + sqrt(newNode.getArea()); //assume AR 1
               float yMax = yMin + sqrt(newNode.getArea()); //assume AR 1
               Node& oldNode = nodes->getNode(currNodeIdx);
               float xloc = oldNode.getX();
               float yloc = oldNode.getY();
               if(xloc >= xMax)
                  poffsetX = 0.5;
               else if(xloc <= xMin)
                  poffsetX = -0.5;
               else
                  poffsetX = ((xloc-xMin)/(xMax-xMin)) - 0.5;

               if(yloc >= yMax)
                  poffsetY = 0.5;
               else if(yloc <= yMin)
                  poffsetY = -0.5;
               else
                  poffsetY = ((yloc-yMin)/(yMax-yMin)) - 0.5;
            }
	      
            pin tempPin(newNode.getName(), false, poffsetX, poffsetY, 
                        netCtr);
            tempPin.putNodeIndex(newNodeIdx);
	      
            if(!seenNodes[newNodeIdx])
            {
               tempEdge.addNode(tempPin);
               seenNodes[newNodeIdx] = 1;
            }
         }
         else
         {
            Node& newTerm = newNodes->getTerminal(currNodeIdx);
            float poffsetX = 0, poffsetY = 0;
	      
            pin tempPin(newTerm.getName(), true, poffsetX, poffsetY, 
                        netCtr);
            tempPin.putNodeIndex(currNodeIdx);
	      
            tempEdge.addNode(tempPin);
         }
      }

      bool needNet = false;
      int firstNodeIdx = tempEdge.pinsBegin()->getNodeIndex();
      for(itPin netPin = tempEdge.pinsBegin(); netPin != tempEdge.pinsEnd();
          netPin++)
      {
         if(netPin->getType())
         {
            needNet = true;
            break;
         }
         else if(netPin->getNodeIndex() != firstNodeIdx) //atleast 1 different
         {
            needNet = true;
            break;
         }
      }
      if(needNet)
      {
         newNets->putNewNet(tempEdge);
         ++netCtr;
      }
   }

   //cout<<"Num Nets: "<<newNets->getNumNets()<<"  Num Pins: "
   //<<newNets->getNumPins()<<endl;
   //update the pins info in nodes
   newNodes->updatePinsInfo(*newNets);
}

void ClusterDB::clusterMultiPhysical(DB*& newDB)
{
   const unsigned numNew = 6;
   unsigned numOldNodes = _db->getNumNodes();

   unsigned blkCtr = 0;
   if(numOldNodes <= 50)
   {
      //*newDB = *_oldDB;
      //Dont change the DB, create a copy of the original
      //and transfer ownership
      bool noCompress = false;
      newDB = new DB(*_db, noCompress);
      return;
   }

   bool noCompress = false;
   _oldDB = new DB(*_db, noCompress);
   if(_params->dontClusterMacros)
   {
      float avgNodeHeight = _oldDB->getAvgHeight();
      _oldDB->markTallNodesAsMacros(avgNodeHeight);
   }
  
   map<unsigned, unsigned> mapping;

   _newDB = new DB(*_oldDB, noCompress) ;

   Nodes* nodes = _oldDB->getNodes();
   Nets* nets = _oldDB->getNets();
   Nodes* newNodes = _newDB->getNodes();
   Nets* newNets = _newDB->getNets();

   itNode node;


   float layOutXSize = _oldDB->getXMax();
   float layOutYSize = _oldDB->getYMax();

   //put nodes outside layout region into layout region
   for(node = nodes->nodesBegin(); node != nodes->nodesEnd(); ++node)
   {
      Node& currNode = *node;
      if(currNode.getX() > layOutXSize)
         currNode.putX(layOutXSize);
      if(currNode.getX() < 0.0)
         currNode.putX(0.0);
      if(currNode.getY() > layOutYSize)
         currNode.putY(layOutYSize);
      if(currNode.getY() < 0.0)
         currNode.putY(0.0);
   }

   float xStep = layOutXSize/numNew;
   float yStep = layOutYSize/numNew;
   float xMax, yMax, xMin, yMin;

   vector<bool> seenNodes(numOldNodes, false);

   char blockName[1024];

   for(unsigned i=0; i<numNew; ++i)
   {
      yMin = i*yStep;
      yMax = (i+1)*yStep;
      for(unsigned j=0; j<numNew; ++j)
      {
         xMin = j*xStep;
         xMax = (j+1)*xStep;

         sprintf(blockName, "Block_%d_%d",i,j);
         float newNodeArea = 0;
         vector<int> newNodesIdxs;

         for(node = nodes->nodesBegin(); node != nodes->nodesEnd(); ++node)
         {
            Node& currNode = *node;
            unsigned currNodeIdx = currNode.getIndex();
            if(!seenNodes[currNodeIdx])
            {
               if(currNode.getX() >= xMin && currNode.getX() <= xMax &&
                  currNode.getY() >= yMin && currNode.getY() <= yMax)
               {
                  if(!currNode.isMacro())
                  {
                     newNodeArea += currNode.getArea();
                     newNodesIdxs.push_back(currNode.getIndex());
                     seenNodes[currNode.getIndex()] = 1;
                  }
                  else //macro needs to stored alone
                  {
                     Node tempNode(currNode.getName(), currNode.getArea(),
                                   currNode.getminAR(), 
                                   currNode.getmaxAR(), blkCtr, false);
                     mapping[currNode.getIndex()] = blkCtr;
                     tempNode.addSubBlockIndex(currNode.getIndex());
                     tempNode.putX(currNode.getX());
                     tempNode.putY(currNode.getY());
                     tempNode.putOrient(currNode.getOrient());
                     newNodes->putNewNode(tempNode);
                     ++blkCtr;
                     seenNodes[currNode.getIndex()] = 1;
                  }
               }
            }
         }
         if(newNodesIdxs.size() != 0)
         {
            Node tempNode(blockName, newNodeArea, 0.5, 2.0, blkCtr, false);
            for(unsigned k=0; k<newNodesIdxs.size(); ++k)
            {
               tempNode.addSubBlockIndex(newNodesIdxs[k]);
               mapping[newNodesIdxs[k]] = blkCtr;
            }
            tempNode.putX(xMin);
            tempNode.putY(yMin);
            newNodes->putNewNode(tempNode);
            ++blkCtr;
         }
      }
   }
   for(unsigned i=0; i<seenNodes.size(); ++i)
      if(seenNodes[i] == 0)
      {
         Node& temp = nodes->getNode(i);
         Node tempNode(temp.getName(), temp.getArea(), temp.getminAR(), 
                       temp.getmaxAR(), blkCtr, false);
	
         tempNode.addSubBlockIndex(temp.getIndex());
         float xLoc = temp.getX() > 0 ? temp.getX() : 0;
         float yLoc = temp.getY() > 0 ? temp.getY() : 0;
         tempNode.putX(xLoc);
         tempNode.putY(yLoc);
         tempNode.putOrient(temp.getOrient());
         newNodes->putNewNode(tempNode);
         ++blkCtr;
         cout<<"Warning in ClusterDB.cxx "<<temp.getName()<<"("<<temp.getX()
             <<", "<<temp.getY()<<") out of layout region "<<endl;
      }

   for(node = nodes->terminalsBegin(); node != nodes->terminalsEnd(); ++node)
      newNodes->putNewTerm(*node);

   addWSPerNode(); //add WS per node needs to be before addNetsToNewDB

   addNetsToNewDB(nets, newNets, nodes, newNodes, mapping);

   //Transfer ownership of the new DB
   newDB = _newDB;
   _newDB = 0;
}

