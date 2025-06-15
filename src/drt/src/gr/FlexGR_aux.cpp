/* Authors: Zhiang Wang */
/*
 * Copyright (c) 2025, The Regents of the University of California
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

#include "FlexGR.h"

#include <omp.h>

#include <cmath>
#include <fstream>
#include <iostream>

#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include "db/infra/frTime.h"
#include "db/obj/frGuide.h"
#include "odb/db.h"
#include "utl/exception.h"

namespace drt {

using utl::ThreadException;

// Auxiliary functions for Routing

// Pattern Routing related functions

inline double FlexGR::initRoute_getGCellCost2D(int xIdx, int yIdx, frDirEnum dir) const
{
  const auto rawSupply = cmap2D_->getRawSupply(xIdx, yIdx, 0, dir);
  const auto rawDemand = cmap2D_->getRawDemand(xIdx, yIdx, 0, dir);
  // congestion cost
  double cost = getCongCost_PattenRoute(rawSupply, rawDemand);
  // overflow cost
  if (rawDemand >= rawSupply) {
    cost += router_cfg_->BLOCKCOST;
  }
    
  // block cost
  if (cmap2D_->hasBlock(xIdx, yIdx, 0, dir)) {
    cost += router_cfg_->BLOCKCOST * 100;
  }

  return cost;
}



// BpIdx and EpIdx are the gcell indices of the begin and end point
double FlexGR::initRoute_getSegmentCost2D(const Point& bpIdx, const Point& epIdx) const
{
  frDirEnum dir = frDirEnum::UNKNOWN;
  if (bpIdx.x() == epIdx.x()) { // vertical segment
    dir = frDirEnum::N;
  } else if (bpIdx.y() == epIdx.y()) { // horizontal segment
    dir = frDirEnum::E;
  } else {
    logger_->error(utl::DRT, 159, "initRoute_getSegmentCost2D: Segment is not aligned!");
  }

  const int from = (dir == frDirEnum::N) ? std::min(bpIdx.y(), epIdx.y()) : std::min(bpIdx.x(), epIdx.x());
  const int to = (dir == frDirEnum::N) ? std::max(bpIdx.y(), epIdx.y()) : std::max(bpIdx.x(), epIdx.x());
  double segmentCost = 0.0;
  if (dir == frDirEnum::N) {
    // vertical segment
    const int xIdx = bpIdx.x();
    // To do list: should we use < or <= here?
    for (int yIdx = from; yIdx < to; yIdx++) {
      segmentCost += initRoute_getGCellCost2D(xIdx, yIdx, dir);
    }
  } else {
    // horizontal segment
    const int yIdx = bpIdx.y();
    for (int xIdx = from; xIdx < to; xIdx++) {
      segmentCost += initRoute_getGCellCost2D(xIdx, yIdx, dir);
    }
  }
  
  return segmentCost;
}


frNode* FlexGR::addSteinerNodeToNet(frNode* child, const Point& gcellIdx, int layerNum)
{
  // Perform check if location is valid
  const auto gcellLoc = design_->getTopBlock()->getGCellCenter(gcellIdx);
  const auto childLoc = child->getLoc();
  const auto childGCellIdx = design_->getTopBlock()->getGCellIdx(childLoc);
  if (gcellLoc == childLoc) {
  //  logger_->report("[Warnning] addNodeToNet: "
  //                  "The node location is already at the gcell center. "
  //                  "No new node will be added.");
    return child;
  }   

  // Create a new node
  auto uNode = std::make_unique<frNode>();
  uNode->setType(frNodeTypeEnum::frcSteiner);
  uNode->setLoc(gcellLoc);
  uNode->setLayerNum(layerNum); // default layer num
  auto newNode = uNode.get();
  // Add the new node to the net 
  auto net = child->getNet();
  net->addNode(uNode);
  // Maintain connectivity
  child->setParent(newNode);
  newNode->addChild(child);
  
  // Update congestion map
  initRoute_updateCongestion2D_Segment(childGCellIdx, gcellIdx);
    
  return newNode;
}


// The segment is added to the child node
void FlexGR::addSegmentToNet(frNode* child, int layerNum)
{
  auto parent = child->getParent();
  // Check if the child and parent are colinear
  if (child->getType() != frNodeTypeEnum::frcSteiner
      || parent->getType() != frNodeTypeEnum::frcSteiner) {
    logger_->error(utl::DRT, 197, "addSegmentToNet: Child or parent is not a steiner node.");
    return;
  }

  // Get the begin and end point of the segment
  Point bpLoc = child->getLoc();
  Point epLoc = parent->getLoc();
  
  if (epLoc.x() != bpLoc.x() && epLoc.y() != bpLoc.y()) {
    logger_->error(utl::DRT, 205, 
      "addSegmentToNet: Child and parent are not colinear! "
      "bpLoc: ({}, {}), epLoc: ({}, {}).",
      bpLoc.x(), bpLoc.y(), epLoc.x(), epLoc.y());
    return;
  }  
  
  if (epLoc < bpLoc) {
    std::swap(bpLoc, epLoc);
  }

  auto net = child->getNet();
  // Create a new path segment
  auto uPathSeg = std::make_unique<grPathSeg>();
  uPathSeg->setChild(child);
  child->setConnFig(uPathSeg.get());  
  uPathSeg->setParent(parent);
  uPathSeg->addToNet(net);
  uPathSeg->setPoints(bpLoc, epLoc);
  uPathSeg->setLayerNum(layerNum); // assuming all segments are on layerNum == 2
  
  // transfer the ownership of the path segment to the net
  std::unique_ptr<grShape> uShape(std::move(uPathSeg));
  net->addGRShape(uShape);
}


void FlexGR::checkNetNodeMatch()
{
  for (auto& net : design_->getTopBlock()->getNets()) {
    if (net->isGRValid() == false) {
      continue; // net is not valid for GR routing
    }
    
    const int nodeCnt = net->getNodes().size() - net->getRPins().size(); // exclude the rpin nodes          
    int nodeIdx = 0;
    std::queue<frNode*> nodeQ;
    nodeQ.push(net->getRootGCellNode());

    while (!nodeQ.empty()) {
      auto currNode = nodeQ.front();
      nodeQ.pop();      
      nodeIdx++;
      for (auto& child : currNode->getChildren()) {
        if (child->getType() == frNodeTypeEnum::frcSteiner) {
          nodeQ.push(child);
        }
      }
    }
     
    if (nodeIdx != nodeCnt) {
      logger_->error(utl::DRT, 208, 
        "checkNetNodeMatch: Node count mismatch for net {}: "
        "expected {}, found {}.", net->getName(), nodeCnt, nodeIdx);
    }
  }
}



}  // namespace drt
