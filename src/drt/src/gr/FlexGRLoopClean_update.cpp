/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/types.h>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/interval_set.hpp>
#include <cstdint>
#include <memory>
#include <vector>

#include "FlexGRCMap.h"
#include "db/grObj/grNet.h"
#include "frDesign.h"
#include "frRTree.h"
#include "gr/FlexGRGridGraph.h"
#include "FlexGR.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <unordered_map>

namespace drt {


// Define the function for loop detection
// Get the bounding box of the net
void FlexGR::getBBoxNet(frNet* net,
  Rect2D& bbox) {        
  for (auto& node : net->getNodes()) {
    if (node->getType() != frNodeTypeEnum::frcSteiner) {
      continue;
    }

    Point loc = design_->getTopBlock()->getGCellIdx(node->getLoc());
    bbox.lower.x = std::min(bbox.lower.x, loc.x());
    bbox.lower.y = std::min(bbox.lower.y, loc.y());
    bbox.upper.x = std::max(bbox.upper.x, loc.x());
    bbox.upper.y = std::max(bbox.upper.y, loc.y());
  }
}


// Define the function for loop detection
bool FlexGR::netLoopDetection_update(
  frNet* net,
  std::vector<frDirEnum>& pathMask)
{
  auto getIdx = [&](int x, int y) {
    return y * xDim_ + x;
  };

  // Get the Bounding Box of the net
  Rect2D bbox = Rect2D(Point2D(std::numeric_limits<int>::max(), std::numeric_limits<int>::max()), 
                       Point2D(std::numeric_limits<int>::min(), std::numeric_limits<int>::min()));
  getBBoxNet(net, bbox);

  // Initialize the 2D vector for the pathMask
  for (int y = bbox.lower.y; y <= bbox.upper.y; y++) {
    for (int x = bbox.lower.x; x <= bbox.upper.x; x++) {
      pathMask[getIdx(x, y)] = frDirEnum::UNKNOWN;
    }
  }

  // Traverse the net and check for loops
  for (auto& node : net->getNodes()) {
    frNode* parentNode = node->getParent();
    if (parentNode == nullptr) {
      continue;
    }

    if (node->getType() != frNodeTypeEnum::frcSteiner
        || parentNode->getType() != frNodeTypeEnum::frcSteiner) {
      continue;
    }

    Point childLocIdx = design_->getTopBlock()->getGCellIdx(node->getLoc());
    Point parentLocIdx = design_->getTopBlock()->getGCellIdx(parentNode->getLoc());

    // check if the parent node is visited
    // In the 2D Grid graph, there is no frDirEnum::U. 
    // So we utilize this for marking the root node
    if (pathMask[getIdx(parentLocIdx.x(), parentLocIdx.y())] != frDirEnum::UNKNOWN) {
      pathMask[getIdx(parentLocIdx.x(), parentLocIdx.y())] = frDirEnum::U;
    }

    if (childLocIdx.y() == parentLocIdx.y()) {
      const int fixedY = childLocIdx.y();
      const frDirEnum dir = (childLocIdx.x() < parentLocIdx.x()) ? frDirEnum::E : frDirEnum::W;
      const int incr = (childLocIdx.x() < parentLocIdx.x()) ? -1 : 1;
      const int endIdx = childLocIdx.x() + incr;
      int xIdx = parentLocIdx.x() + incr;
      while (xIdx != endIdx) {
        const auto idx = getIdx(xIdx, fixedY);        
        if (pathMask[idx] != frDirEnum::UNKNOWN) {
          std::cout << "Loop detected " << xIdx << " " << fixedY << std::endl;
          std::cout << "childLocIdx: " << childLocIdx.x() << " " << childLocIdx.y() << std::endl;
          std::cout << "parentLocIdx: " << parentLocIdx.x() << " " << parentLocIdx.y() << std::endl; 
          return true; // loop detected
        }       

        pathMask[idx] = dir;
        xIdx += incr;
      }
    } else if (childLocIdx.x() == parentLocIdx.x()) {
      const int fixedX = childLocIdx.x();
      const frDirEnum dir = (childLocIdx.y() < parentLocIdx.y()) ? frDirEnum::N : frDirEnum::S;
      const int incr = (childLocIdx.y() < parentLocIdx.y()) ? -1 : 1;
      const int endIdx = childLocIdx.y() + incr;
      int yIdx = parentLocIdx.y() + incr;
      while (yIdx != endIdx) {
        const auto idx = getIdx(fixedX, yIdx);
        if (pathMask[idx] != frDirEnum::UNKNOWN) {
          std::cout << "Loop detected " << fixedX << " " << yIdx << std::endl;
          std::cout << "childLocIdx: " << childLocIdx.x() << " " << childLocIdx.y() << std::endl;
          std::cout << "parentLocIdx: " << parentLocIdx.x() << " " << parentLocIdx.y() << std::endl;
          return true; // loop detected
        }

        pathMask[idx] = dir;
        yIdx += incr;
      }
    } else {
      logger_->error(DRT, 289, "current node and parent node are are not aligned collinearly\n");
    }
  }

  return false;
}


// This function is for testing the loop detection and removal algorithm
// Define the function for the loop remove
bool FlexGR::netLoopRemoveVisual_update(
  frNet* net,
  std::vector<frDirEnum>& pathMask)
{
  // Traverse the net and check for loops
  for (auto& node : net->getNodes()) {
    frNode* parentNode = node->getParent();
    if (parentNode == nullptr) {
      continue;
    }

    if (node->getType() != frNodeTypeEnum::frcSteiner
        || parentNode->getType() != frNodeTypeEnum::frcSteiner) {
      continue;
    }

    Point childLocIdx = design_->getTopBlock()->getGCellIdx(node->getLoc());
    Point parentLocIdx = design_->getTopBlock()->getGCellIdx(parentNode->getLoc());

    logger_->report("[INFO][FlexGR] Child Node: ({}, {}), Parent Node: ({}, {})\n", 
      childLocIdx.x(), childLocIdx.y(), parentLocIdx.x(), parentLocIdx.y());  
  }


  // print the pin gcell nodes
  for (auto& node : net->getNodes()) {
    if (node->getType() != frNodeTypeEnum::frcSteiner) {
      continue;
    }

    if (node->isDontMove()) {
      Point locIdx = design_->getTopBlock()->getGCellIdx(node->getLoc());
      logger_->report("[INFO][FlexGR] DontMove Pin Node: ( {} , {} )\n", locIdx.x(), locIdx.y());

    }
  }

  exit(1);
  return true;
}


void FlexGR::removeLoop(std::vector<frNet*>& net2RR) 
{ 
  for (auto& net: net2RR) {
    std::vector<frNode*> nodes;
    for (auto& node : net->getNodes()) {
      if (node->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }
      //node->setIntProp(nodes.size());
      nodes.push_back(node.get());
    }

    for (auto& node : nodes) {      
      frNode* parentNode = node->getParent();
      if (parentNode == nullptr || parentNode->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }

      Point nodeLocIdx = design_->getTopBlock()->getGCellIdx(node->getLoc());
      Point parentLocIdx = design_->getTopBlock()->getGCellIdx(parentNode->getLoc());
      //int nodeIdx = node->getIntProp();
      //int parentIdx = parentNode->getIntProp();

      // Check if the child node is the same as the parent node
      if (nodeLocIdx == parentLocIdx) {
        //std::cout << "Error: child node is the same as the parent node\n";
        //std::cout << "Child node: " << nodeLocIdx.x() << " " << nodeLocIdx.y() << std::endl;
        //std::cout << "Parent node: " << parentLocIdx.x() << " " << parentLocIdx.y() << std::endl;
        // Merge the child node to the parent node
        if (node->isDontMove()) {
          parentNode->setDontMove();
        }

        for (auto& child : node->getChildren()) {
          parentNode->addChild(child);
          child->setParent(parentNode);
        }

        parentNode->removeChild(node);
        // Remove the child node
        net->removeNode(node);
      }
    }    
  }


  int netId = 0;
  for (auto& net: net2RR) {
    //std::cout << "Net Id: " << netId << std::endl;
    for (auto& node : net->getNodes()) {
      frNode* parentNode = node->getParent();
      if (node->getType() != frNodeTypeEnum::frcSteiner ||
        parentNode == nullptr || 
        parentNode->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }

      Point nodeLocIdx = design_->getTopBlock()->getGCellIdx(node->getLoc());
      Point parentLocIdx = design_->getTopBlock()->getGCellIdx(parentNode->getLoc());
      // std::cout << "Parent Node: " << parentLocIdx.x() << " " << parentLocIdx.y() <<  "  " << " isDontMove =  " << parentNode->isDontMove() << "  "
      //          << "Child Node: " << nodeLocIdx.x() << " " << nodeLocIdx.y() << "  " << " isDontMove =  " << node->isDontMove() << std::endl;
    }
    
    std::vector<int> nodeLevel(net->getNodes().size(), -1);
    int depth = 0;
    std::queue<frNode*> nodeQ;
    nodeQ.push(net->getRootGCellNode());
    int rootId = distance(net->getFirstNonRPinNode()->getIter(), net->getRootGCellNode()->getIter());
    nodeLevel[rootId] = depth;
    while (!nodeQ.empty()) {
      auto node = nodeQ.front();
      nodeQ.pop();
      
      if (node->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }  

      int id = distance(net->getFirstNonRPinNode()->getIter(), node->getIter());
      depth = nodeLevel[id];

      for (auto& child : node->getChildren()) {
        if (child->getType() != frNodeTypeEnum::frcSteiner) {
          continue;
        }

        Point childLoc = design_->getTopBlock()->getGCellIdx(child->getLoc());
        Point nodeLoc = design_->getTopBlock()->getGCellIdx(node->getLoc());

        if (childLoc.x() == nodeLoc.x() && childLoc.y() == nodeLoc.y()) {
          logger_->error(DRT, 364, "Error: two steiner nodes are at the same location\n");
        }

        nodeQ.push(child);
        int nodeId = distance(net->getFirstNonRPinNode()->getIter(), child->getIter());
        if (nodeLevel[nodeId] != -1) {
          logger_->error(DRT, 365, "Error: node {} is visited twice in net {}\n", nodeId, net->getName());
        }
        nodeLevel[nodeId] = depth + 1;
      }
    }

    netId++;
  }
}



void FlexGR::removeLoop()
{
  int numLoopedNets = 0;
  std::vector<frDirEnum> pathMask(xDim_ * yDim_, frDirEnum::UNKNOWN);
  for (auto& net_tuple : sortedNets_) {
    auto& net = std::get<0>(net_tuple);
    if (netLoopDetection_update(net, pathMask)) {
      netLoopRemoveVisual_update(net, pathMask);
      numLoopedNets++;
    }
  }

  logger_->report("[INFO][FlexGR] Number of looped nets: {} ({}%)\n", numLoopedNets, numLoopedNets * 100.0 / sortedNets_.size());

  exit(1);

}













}  // namespace drt
