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
#include <string>

#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include "db/infra/frTime.h"
#include "db/obj/frGuide.h"
#include "odb/db.h"
#include "utl/exception.h"

namespace drt {


void FlexGR::initRoute_gpu()
{
  // generate topology using FLUTE
  initRoute_genTopology();

  // initRoute_patternRoute();

  // initRoute_initObj();
}












// To do list:  provide two options: 
// 1. generate topology using FLUTE [Done]
// 2. generate topology using PD-tree
void FlexGR::initRoute_genTopology()
{
  logger_->report("[INFO] Generating net topology...");
  for (auto& net : design_->getTopBlock()->getNets()) {
    initRoute_genTopology_net(net.get());
    initRoute_updateCongestion2D_net(net.get());
  }
  
  logger_->report("[INFO] Net topology generation done ...");
}





// generate 2D topology, rpin node always connect to center of gcell
// to be followed by layer assignment
void FlexGR::initRoute_genTopology_net(frNet* net)
{
  if (net->getNodes().empty()) {
    return;
  }

  if (net->getNodes().size() == 1) {
    net->setRoot(net->getNodes().front().get());
    return;
  }


  /*
  std::vector<frNode*> nodes(net->getNodes().size(), nullptr);  // 0 is source
  std::map<frBlockObject*, std::vector<frNode*>>
      pin2Nodes;  // vector order needs to align with map below
  std::map<frBlockObject*, std::vector<frRPin*>> pin2RPins;
  unsigned sinkIdx = 1;

  auto& netNodes = net->getNodes();
  // init nodes and populate pin2Nodes
  for (auto& node : netNodes) {
    if (node->getPin()) {
      if (node->getPin()->typeId() == frcInstTerm) {
        auto ioType = static_cast<frInstTerm*>(node->getPin())
                          ->getTerm()
                          ->getDirection();
        // for instTerm, direction OUTPUT is driver
        if (ioType == dbIoType::OUTPUT && nodes[0] == nullptr) {
          nodes[0] = node.get();
        } else {
          if (sinkIdx >= nodes.size()) {
            sinkIdx %= nodes.size();
          }
          nodes[sinkIdx] = node.get();
          sinkIdx++;
        }
        pin2Nodes[node->getPin()].push_back(node.get());
      } else if (node->getPin()->typeId() == frcBTerm
                 || node->getPin()->typeId() == frcMTerm) {
        auto ioType = static_cast<frTerm*>(node->getPin())->getDirection();
        // for IO term, direction INPUT is driver
        if (ioType == dbIoType::INPUT && nodes[0] == nullptr) {
          nodes[0] = node.get();
        } else {
          if (sinkIdx >= nodes.size()) {
            sinkIdx %= nodes.size();
          }
          nodes[sinkIdx] = node.get();
          sinkIdx++;
        }
        pin2Nodes[node->getPin()].push_back(node.get());
      } else {
        std::cout << "Error: unknown pin type in initGR_genTopology_net\n";
      }
    }
  }

  net->setRoot(nodes[0]);
  // populate pin2RPins
  for (auto& rpin : net->getRPins()) {
    if (rpin->getFrTerm()) {
      pin2RPins[rpin->getFrTerm()].push_back(rpin.get());
    }
  }
  // update nodes location based on rpin
  for (auto& [pin, nodes] : pin2Nodes) {
    if (pin2RPins.find(pin) == pin2RPins.end()) {
      std::cout << "Error: pin not found in pin2RPins\n";
      exit(1);
    }
    if (pin2RPins[pin].size() != nodes.size()) {
      std::cout << "Error: mismatch in nodes and ripins size\n";
      exit(1);
    }
    auto& rpins = pin2RPins[pin];
    for (int i = 0; i < (int) nodes.size(); i++) {
      auto rpin = rpins[i];
      auto node = nodes[i];
      Point pt;
      if (rpin->getFrTerm()->typeId() == frcInstTerm) {
        auto inst = static_cast<frInstTerm*>(rpin->getFrTerm())->getInst();
        dbTransform shiftXform = inst->getNoRotationTransform();
        pt = rpin->getAccessPoint()->getPoint();
        shiftXform.apply(pt);
      } else {
        pt = rpin->getAccessPoint()->getPoint();
      }
      node->setLoc(pt);
      node->setLayerNum(rpin->getAccessPoint()->getLayerNum());
      // added by Zhiang   
      node->setRPin(rpin);
    }
  }

  // std::map<std::pair<int, int>, std::vector<frNode*> > gcellIdx2Nodes;
  auto& gcellIdx2Nodes = net2GCellIdx2Nodes_[net];
  // std::map<frNode*, std::vector<frNode*> > gcellNode2RPinNodes;
  auto& gcellNode2RPinNodes = net2GCellNode2RPinNodes_[net];

  // prep for 2D topology generation in case two nodes are more than one rpin in
  // same gcell topology genration works on gcell (center-to-center) level
  for (auto node : nodes) {
    Point apLoc = node->getLoc();
    Point apGCellIdx = design_->getTopBlock()->getGCellIdx(apLoc);
    gcellIdx2Nodes[std::make_pair(apGCellIdx.x(), apGCellIdx.y())].push_back(
        node);
  }

  // generate gcell-level node
  // std::vector<frNode*> gcellNodes(gcellIdx2Nodes.size(), nullptr);
  auto& gcellNodes = net2GCellNodes_[net];
  gcellNodes.resize(gcellIdx2Nodes.size(), nullptr);

  std::vector<std::unique_ptr<frNode>> tmpGCellNodes;
  sinkIdx = 1;
  unsigned rootIdx = 0;
  unsigned rootIdxCnt = 0;
  for (auto& [gcellIdx, localNodes] : gcellIdx2Nodes) {
    bool hasRoot = false;
    for (auto localNode : localNodes) {
      if (localNode == nodes[0]) {
        hasRoot = true;
      }
    }

    auto gcellNode = std::make_unique<frNode>();
    gcellNode->setType(frNodeTypeEnum::frcSteiner);
    Rect gcellBox = design_->getTopBlock()->getGCellBox(
        Point(gcellIdx.first, gcellIdx.second));
    Point loc((gcellBox.xMin() + gcellBox.xMax()) / 2,
              (gcellBox.yMin() + gcellBox.yMax()) / 2);
    gcellNode->setLayerNum(2);
    gcellNode->setLoc(loc);
    if (!hasRoot) {
      gcellNode->setId(net->getNodes().back()->getId() + sinkIdx + 1);
      gcellNodes[sinkIdx] = gcellNode.get();
      sinkIdx++;
    } else {
      gcellNode->setId(net->getNodes().back()->getId() + 1);
      gcellNodes[0] = gcellNode.get();
      rootIdx = rootIdxCnt;
    }
    gcellNode2RPinNodes[gcellNode.get()] = localNodes;
    tmpGCellNodes.push_back(std::move(gcellNode));
    rootIdxCnt++;
  }
  net->setFirstNonRPinNode(gcellNodes[0]);

  net->addNode(tmpGCellNodes[rootIdx]);
  for (unsigned i = 0; i < tmpGCellNodes.size(); i++) {
    if (i != rootIdx) {
      net->addNode(tmpGCellNodes[i]);
    }
  }

  for (unsigned i = 0; i < gcellNodes.size(); i++) {
    auto node = gcellNodes[i];
    if (!node) {
      std::cout << "Error: gcell node " << i << " is 0x0\n";
    }
  }

  if (gcellNodes.size() <= 1) {
    return;
  }

  net->setRootGCellNode(gcellNodes[0]);

  auto& steinerNodes = net2SteinerNodes_[net];
  // if (gcellNodes.size() >= 150) {
  // TODO: remove connFig instantiation to match FLUTE behavior
  if (false) {
    // generate mst topology
    genMSTTopology(gcellNodes);

    // sanity check
    for (unsigned i = 1; i < gcellNodes.size(); i++) {
      if (gcellNodes[i]->getParent() == nullptr) {
        std::cout << "Error: non-root gcell node does not have parent\n";
      }
    }

    // generate steiner tree from MST
    genSTTopology_HVW(gcellNodes, steinerNodes);
    // generate shapes and update congestion map
    for (auto node : gcellNodes) {
      // add shape from child to parent
      if (node->getParent()) {
        auto parent = node->getParent();
        Point childLoc = node->getLoc();
        Point parentLoc = parent->getLoc();
        Point bp, ep;
        if (childLoc < parentLoc) {
          bp = childLoc;
          ep = parentLoc;
        } else {
          bp = parentLoc;
          ep = childLoc;
        }

        auto uPathSeg = std::make_unique<grPathSeg>();
        uPathSeg->setChild(node);
        uPathSeg->setParent(parent);
        uPathSeg->addToNet(net);
        uPathSeg->setPoints(bp, ep);
        // 2D shapes are all on layerNum == 2
        // assuming (layerNum / - 1) == congestion map idx
        uPathSeg->setLayerNum(2);

        Point bpIdx = design_->getTopBlock()->getGCellIdx(bp);
        Point epIdx = design_->getTopBlock()->getGCellIdx(ep);

        // update congestion map
        // horizontal
        unsigned zIdx = 0;
        if (bpIdx.y() == epIdx.y()) {
          for (int xIdx = bpIdx.x(); xIdx < epIdx.x(); xIdx++) {
            cmap_->addDemand(xIdx, bpIdx.y(), zIdx, frDirEnum::E);
          }
        } else {
          for (int yIdx = bpIdx.y(); yIdx < epIdx.y(); yIdx++) {
            cmap_->addDemand(bpIdx.x(), yIdx, zIdx, frDirEnum::N);
          }
        }

        std::unique_ptr<grShape> uShape(std::move(uPathSeg));
        net->addGRShape(uShape);
      }
    }

    for (auto node : steinerNodes) {
      // add shape from child to parent
      if (node->getParent()) {
        auto parent = node->getParent();
        Point childLoc = node->getLoc();
        Point parentLoc = parent->getLoc();
        Point bp, ep;
        if (childLoc < parentLoc) {
          bp = childLoc;
          ep = parentLoc;
        } else {
          bp = parentLoc;
          ep = childLoc;
        }

        auto uPathSeg = std::make_unique<grPathSeg>();
        uPathSeg->setChild(node);
        uPathSeg->setParent(parent);
        uPathSeg->addToNet(net);
        uPathSeg->setPoints(bp, ep);
        // 2D shapes are all on layerNum == 2
        // assuming (layerNum / - 1) == congestion map idx
        uPathSeg->setLayerNum(2);

        Point bpIdx = design_->getTopBlock()->getGCellIdx(bp);
        Point epIdx = design_->getTopBlock()->getGCellIdx(ep);

        // update congestion map
        // horizontal
        unsigned zIdx = 0;
        if (bpIdx.y() == epIdx.y()) {
          for (int xIdx = bpIdx.x(); xIdx < epIdx.x(); xIdx++) {
            cmap_->addDemand(xIdx, bpIdx.y(), zIdx, frDirEnum::E);
          }
        } else {
          for (int yIdx = bpIdx.y(); yIdx < epIdx.y(); yIdx++) {
            cmap_->addDemand(bpIdx.x(), yIdx, zIdx, frDirEnum::N);
          }
        }

        std::unique_ptr<grShape> uShape(std::move(uPathSeg));
        net->addGRShape(uShape);
      }
    }
  } else {
    genSTTopology_FLUTE(gcellNodes, steinerNodes);
  }

  // connect rpin node to gcell center node
  for (auto& [gcellNode, localNodes] : gcellNode2RPinNodes) {
    for (auto localNode : localNodes) {
      if (localNode == nodes[0]) {
        gcellNode->setParent(localNode);
        localNode->addChild(gcellNode);
      } else {
        gcellNode->addChild(localNode);
        localNode->setParent(gcellNode);
      }
    }
  }

  // sanity check
  for (size_t i = 1; i < nodes.size(); i++) {
    if (nodes[i]->getParent() == nullptr) {
      std::cout << "Error: non-root node does not have parent in "
                << net->getName() << '\n';
    }
  }
  if (nodes.size() > 1 && nodes[0]->getChildren().empty()) {
    std::cout << "Error: root does not have any children\n";
  }
    */
}





// ----------------------------------------------------------------------------
// Utility functions for updating congestion
// ----------------------------------------------------------------------------


// Get the segment attached to the node (segment connecting the node and its parent).
void FlexGR::initRoute_getNodeSegment2D(frNode* node, Point& bpIdx, Point& epIdx)
{
  // Initialize
  bpIdx.set(-1, -1);
  epIdx.set(-1, -1);

  if (node->getParent() == nullptr) return;
  
  if (node->getType() != frNodeTypeEnum::frcSteiner
    || node->getParent()->getType() != frNodeTypeEnum::frcSteiner) {
    return;
  }
    
  bpIdx = node->getLoc();
  epIdx = node->getParent()->getLoc();
  if (epIdx < bpIdx) {
    std::swap(bpIdx, epIdx);
  }

  if (bpIdx.x() != epIdx.x() && bpIdx.y() != epIdx.y()) {
    logger_->error(utl::DRT, 100, 
      "initRoute_getNodeSegment2D: Node segment is not aligned!");   
    return;
  }

  // Transform to GCell indices
  bpIdx = design_->getTopBlock()->getGCellIdx(bpIdx);
  epIdx = design_->getTopBlock()->getGCellIdx(epIdx);
}


// bpIdx <= epIdx
void FlexGR::initRoute_updateCongestion2D_Segment(
  const Point& bpIdx, 
  const Point& epIdx)
{
  // Update 2D congestion map
  unsigned zIdx = 0;
  if (bpIdx.y() == epIdx.y()) {
    for (int xIdx = bpIdx.x(); xIdx < epIdx.x(); xIdx++) {
      cmap2D_->addRawDemand(xIdx, bpIdx.y(), zIdx, frDirEnum::E);
      cmap2D_->addRawDemand(xIdx + 1, bpIdx.y(), zIdx, frDirEnum::E);
    }
  } else if (bpIdx.x() == epIdx.x()) {
    for (int yIdx = bpIdx.y(); yIdx < epIdx.y(); yIdx++) {
      cmap2D_->addRawDemand(bpIdx.x(), yIdx, zIdx, frDirEnum::N);
      cmap2D_->addRawDemand(bpIdx.x(), yIdx + 1, zIdx, frDirEnum::N);
    }
  } else {
    logger_->error(utl::DRT, 101, 
      "initRoute_updateCongestion2D_Segment: Segment is not aligned!");
  } 
}


void FlexGR::initRoute_updateCongestion2D_net(frNet* net)
{
  // net spanning multiple GCells
  if (net->isGRValid() == false) {
    return;
  }

  Point bpIdx, epIdx;
  // Traverse the entire routing tree of the net  
  for (auto& node : net->getNodes()) {
    // Get the segment attached to the node
    initRoute_getNodeSegment2D(node.get(), bpIdx, epIdx);    
    initRoute_updateCongestion2D_Segment(bpIdx, epIdx);
  }
}











}  // namespace drt
