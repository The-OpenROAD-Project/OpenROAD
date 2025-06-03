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
#include "stt/SteinerTreeBuilder.h"
#include "gr/FlexGR_cost.h"

namespace drt {


void FlexGR::initRoute_gpu()
{
  // generate topology using FLUTE
  initRoute_genTopology();
  // check how many nets are valid for GR routing
  unsigned validNets = 0;
  for (auto& net : design_->getTopBlock()->getNets()) {
    if (net->isGRValid()) {
      validNets++;
    }
  }
  logger_->report("[INFO] {} ( {:.2f} %) nets are valid for GR routing.", 
                  validNets, 
                  static_cast<float>(validNets) / design_->getTopBlock()->getNets().size() * 100.0);  
  logger_->report("[INFO] Initial congestion map after FLUTE topology generation ...");
  reportCong2D();

  // No need to use GPU-accelerated approach
  // Initialize Z-Shape pattern routing for non-clinear segments  
  initRoute_patternRoute_Z_shape();
 
  logger_->report("[INFO] Initial congestion map after Z-shape pattern routing ...");
  reportCong2D();

  // convert topological connection to physical connection
  initGR_initPhysicalObj();
}






// To do list:  provide two options: 
// 1. generate topology using FLUTE [Done]
// 2. generate topology using PD-tree
void FlexGR::initRoute_genTopology()
{
  logger_->report("[INFO] Generating net topology...");
  for (auto& net : design_->getTopBlock()->getNets()) {
    // We assume clk nets has been handled by the clock tree synthesis
    // To do list: it seems that clock net sometimes cannot be identified by the isSpecial() function
    std::string netName = net->getName();  
    if (netName.find("clk") != std::string::npos || netName.find("CLK") != std::string::npos) {
      net->setGRValid(false);
      logger_->report("[INFO] initRoute_genTopology: skipping clk net ({})", netName);
      continue;
    }  

    initRoute_rpinMap(net.get());
    initRoute_createPinGCellNodes(net.get());
    initRoute_genTopology_net(net.get());
    // update the congestion map for the aligned segment in the Steiner trees
    // Then we will use pattern routing for the unaligned segments
    initRoute_updateCongestion2D_net(net.get(), false);
  }
  
  logger_->report("[INFO] Net topology generation done ...");
}



// Map rpin to nodes
void FlexGR::initRoute_rpinMap(frNet* net)
{
  if (net->getNodes().empty()) {
    net->setGRValid(false);
    return;
  }

  if (net->getNodes().size() == 1) {
    net->setRoot(net->getNodes().front().get());
    net->setGRValid(false);  // single node net is not valid for GR routing
    return;
  }


  auto& nodes = net->getPinNodes();
  nodes.resize(net->getNodes().size(), nullptr);  // 0 is source
  
  // Each pin may multiple access points (see Via Pillar as an example)
  // Map real pins to nodes
  std::map<frBlockObject*, std::vector<frNode*> > pin2Nodes;  // vector order needs to align with map below
  // Map real pins to rpins
  std::map<frBlockObject*, std::vector<frRPin*> > pin2RPins;
   
  // Note that not all the nets have driver pin and sink pins.
  // (1) a net may have multiple driver pins, in this case, we will set the first one as root
  // (2) a net may have no driver pin, in this case, we will set the first node as root

  // ------------------------------------------------------------------------------------------------
  // Step 1: Map pins to nodes
  // ------------------------------------------------------------------------------------------------
  unsigned sinkIdx = 1;
  auto& netNodes = net->getNodes();
  // init nodes and populate pin2Nodes
  for (auto& node : netNodes) {
    auto pin = node->getPin();
    if (pin == nullptr) {
      logger_->report("[INFO] initRoute_genTopology_net: "
                      "node has no pin in net ({}), skipping.",
                      net->getName());
      continue;
    }

    if (!(pin->typeId() == frcInstTerm || pin->typeId() == frcBTerm)) {
      logger_->report("[INFO] initRoute_genTopology_net: "
                      "unkown pin type in net ({}), skipping.", net->getName());
      continue;
    }
          
    // Determine if the pin is a driver or a sink
    bool isDriver = 
      pin->typeId() == frcInstTerm && 
      static_cast<frInstTerm*>(pin)->getTerm()->getDirection() == dbIoType::OUTPUT;    
    // In the legacy code, the driver is defined as: 
    // Not sure why input frcMTerm is defined as driver
    // So temporarily remove this condition
    // Please do not remove the following commented lines
    //  isDriver |= 
    //    (pin->typeId() == frcBTerm || pin->typeId() == frcMTerm) &&
    //    static_cast<frTerm*>(pin)->getDirection() == dbIoType::INPUT;
    isDriver |=
      pin->typeId() == frcBTerm && 
      static_cast<frBTerm*>(pin)->getDirection() == dbIoType::INPUT;

    if (isDriver && nodes[0] != nullptr) {
      logger_->report("[INFO] initRoute_genTopology_net: "
                      "net ({}) has multiple driver pins, skipping.",
                      net->getName());
    }

    if (isDriver && nodes[0] == nullptr) {
      nodes[0] = node.get();  // set driver pin as root
    } else {
      if (sinkIdx >= nodes.size()) {
        logger_->report("[INFO] initRoute_genTopology_net: "
                        "sinkIdx ({}) is larger than nodes size ({}), wrapping around.\n"
                        " This may happen if the net ({}) has NO driver pin.",
                        sinkIdx, nodes.size(), net->getName());
        sinkIdx %= nodes.size();
      }
      nodes[sinkIdx] = node.get();
      sinkIdx++;
    }

    pin2Nodes[node->getPin()].push_back(node.get());
  }
  
  net->setRoot(nodes[0]);
  
 
  // ------------------------------------------------------------------------------------------------
  // Step 2:  populate pin2RPins
  // ------------------------------------------------------------------------------------------------
  // Comments from Zhiang:
  // Each term consists of one or more physical pins. Each pin consists of one or more
  // physical shapes across one or more metal and cut layers.
  // A term including more than one pin with “MUSTJOIN” keyword indicates
  // that the two pins should be physically connected in detailed routing. 
  // But at this point, we assume that each term holds one physical pin.
  // To do list: support multiple pins in a term  
  // Map rpin to node
  for (auto& rpin : net->getRPins()) {
    if (rpin->getFrTerm()) {
      pin2RPins[rpin->getFrTerm()].push_back(rpin.get());
    }
  }
  
  std::string errMsg;
  // update nodes location based on rpin
  for (auto& [pin, nodes] : pin2Nodes) {
    if (pin2RPins.find(pin) == pin2RPins.end()) {
      errMsg = "Error: pin not found in pin2RPins for net " + net->getName();
      break;
    }
    
    if (pin2RPins[pin].size() != nodes.size()) {
      errMsg = "Error: mismatch in nodes and ripins size for net " + net->getName();
      break;
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
      if (node->getType() != frNodeTypeEnum::frcPin) {
        std::cout << "Error: node is not rpin" << std::endl;
      }
      node->setDontMove(); // do not move the pinNode during routing
    }
  }
  
  if (!errMsg.empty()) {
    logger_->error(utl::DRT, 137, "initRoute_genTopology_net: {}", errMsg);
    return;
  }
}


// Create pinGCellNodes
void FlexGR::initRoute_createPinGCellNodes(frNet* net)
{
  if (net->isGRValid() == false) {
    return;  // net is not valid for GR routing
  }
  
  // auto nodes = net->getNodes();
  auto& gcellIdx2Nodes = net->getGCellIdx2Nodes();
  auto& gcellNodes = net->getPinGCellNodes();
  auto& gcellNode2RPinNodes = net->getGCellNode2RPinNodes();
  auto& pinNodes = net->getPinNodes();
  auto& rootPinNode = pinNodes[0];

  // prep for 2D topology generation in case two nodes are more than one rpin in
  // same gcell topology genration works on gcell (center-to-center) level
  for (auto node : pinNodes) {
    const odb::Point apLoc = node->getLoc();
    const odb::Point apGCellIdx = design_->getTopBlock()->getGCellIdx(apLoc);
    gcellIdx2Nodes[apGCellIdx].push_back(node);
  }

  // generate gcell-level node
  gcellNodes.resize(gcellIdx2Nodes.size(), nullptr);
  std::vector<std::unique_ptr<frNode> > tmpGCellNodes;
  int sinkIdx = 1;
  int rootIdx = -1;
  int idx = 0;
  
  for (auto& [gcellIdx, localNodes] : gcellIdx2Nodes) {
    // check of nodes[0] is in localNodes
    bool hasRoot = (rootIdx == -1) && 
      (std::find(localNodes.begin(), localNodes.end(), rootPinNode) != localNodes.end());
    auto gcellNode = std::make_unique<frNode>();
    gcellNode->setType(frNodeTypeEnum::frcSteiner);
    gcellNode->setDontMove();  // do not move the gcell node during routing
    const odb::Rect gcellBox = design_->getTopBlock()->getGCellBox(
        odb::Point(gcellIdx.x(), gcellIdx.y()));
    const odb::Point loc((gcellBox.xMin() + gcellBox.xMax()) / 2,
              (gcellBox.yMin() + gcellBox.yMax()) / 2);
    gcellNode->setLayerNum(2);
    gcellNode->setLoc(loc);
    if (!hasRoot) {
      gcellNode->setId(net->getNodes().back()->getId() + sinkIdx + 1);
      gcellNodes[sinkIdx++] = gcellNode.get();
    } else {
      gcellNode->setId(net->getNodes().back()->getId() + 1);
      gcellNodes[0] = gcellNode.get();
      rootIdx = idx;
    }
    gcellNode2RPinNodes[gcellNode.get()] = localNodes;
    tmpGCellNodes.push_back(std::move(gcellNode));
    idx++;
  }
  
 
  if (rootIdx == -1) {
    logger_->error(utl::DRT, 145, "initRoute_createPinGCellNodes: "
                   "rootIdx is -1 for net {}, this should not happen.",
                   net->getName());
    return;
  }

  net->setRootGCellNode(gcellNodes[0]);
  net->setFirstNonRPinNode(gcellNodes[0]);
  // All these unique_ptrs will be moved to net->addNode()
  net->addNode(tmpGCellNodes[rootIdx]); 
  for (unsigned i = 0; i < tmpGCellNodes.size(); i++) {
    if (i != rootIdx) {
      net->addNode(tmpGCellNodes[i]);
    }
  }

  // connect rpin node to gcell center node
  for (auto& [gcellNode, localNodes] : gcellNode2RPinNodes) {
    for (auto localNode : localNodes) {
      if (localNode == rootPinNode) {
        gcellNode->setParent(localNode);
        localNode->addChild(gcellNode);
      } else {
        gcellNode->addChild(localNode);
        localNode->setParent(gcellNode);
      }
    }
  }

  if (gcellNodes.size() <= 1) {
    net->setGRValid(false);  // single node net is not valid for GR routing
  } else {
    net->setGRValid(true);  // valid for GR routing
  }
}



// generate 2D topology, rpin node always connect to center of gcell
// to be followed by layer assignment
// In this function, we generate initial topology for the net
// Also,  we will check if the net is valid for GR routing (i.e., the net spanning multiple GCells).
void FlexGR::initRoute_genTopology_net(frNet* net)
{
  if (net->isGRValid() == false) {
    return;  // net is not valid for GR routing
  }

  auto& pinGCellNodes = net->getPinGCellNodes();  
  std::vector<frNode*> steinerNodes;
  auto root = pinGCellNodes[0];

  // prep for flute
  int degree = pinGCellNodes.size();
  std::vector<int> xs(degree);
  std::vector<int> ys(degree);
  for (int i = 0; i < (int) pinGCellNodes.size(); i++) {
    auto gcellNode = pinGCellNodes[i];
    Point loc = gcellNode->getLoc();
    xs[i] = loc.x();
    ys[i] = loc.y();
  }
 
  
  
  // temporary to keep using flute here
  stt_builder_->setAlpha(0);
  auto fluteTree = stt_builder_->makeSteinerTree(xs, ys, 0);

  std::map<odb::Point, frNode*> pinGCell2Nodes, steinerGCell2Nodes;
  std::map<frNode*, std::set<frNode*, frBlockObjectComp>, frBlockObjectComp>
      adjacencyList;

  for (auto pinNode : pinGCellNodes) {
    pinGCell2Nodes[pinNode->getLoc()] = pinNode;
  }

  // iterate over branches, create new nodes and build connectivity
  for (int i = 0; i < degree * 2 - 2; i++) {
    auto& branch1 = fluteTree.branch[i];
    auto& branch2 = fluteTree.branch[branch1.n];

    Point bp(branch1.x, branch1.y);
    Point ep(branch2.x, branch2.y);

    if (bp == ep) {
      continue;
    }

    frNode* bpNode = nullptr;
    frNode* epNode = nullptr;

    // get bp node
    if (pinGCell2Nodes.find(bp) == pinGCell2Nodes.end()) {
      if (steinerGCell2Nodes.find(bp) == steinerGCell2Nodes.end()) {
        // add steiner
        auto steinerNode = std::make_unique<frNode>();
        bpNode = steinerNode.get();
        steinerNode->setType(frNodeTypeEnum::frcSteiner);
        steinerNode->setLoc(bp);
        steinerNode->setLayerNum(2);
        steinerGCell2Nodes[bp] = steinerNode.get();
        steinerNodes.push_back(steinerNode.get());
        net->addNode(steinerNode);
      } else {
        bpNode = steinerGCell2Nodes[bp];
      }
    } else {
      bpNode = pinGCell2Nodes[bp];
    }
    
    // get ep node
    if (pinGCell2Nodes.find(ep) == pinGCell2Nodes.end()) {
      if (steinerGCell2Nodes.find(ep) == steinerGCell2Nodes.end()) {
        // add steiner
        auto steinerNode = std::make_unique<frNode>();
        epNode = steinerNode.get();
        steinerNode->setType(frNodeTypeEnum::frcSteiner);
        steinerNode->setLoc(ep);
        steinerNode->setLayerNum(2);
        steinerGCell2Nodes[ep] = steinerNode.get();
        steinerNodes.push_back(steinerNode.get());
        net->addNode(steinerNode);
      } else {
        epNode = steinerGCell2Nodes[ep];
      }
    } else {
      epNode = pinGCell2Nodes[ep];
    }

    if (bpNode == nullptr || epNode == nullptr) {
      logger_->error(utl::DRT, 138, 
        "initRoute_genTopology_net: bpNode or epNode is void for net {}",
        net->getName());
    }

    adjacencyList[bpNode].insert(epNode);
    adjacencyList[epNode].insert(bpNode);
  }
  
  // reset nodes
  // disconnect pinGCell2Nodes to ripins
  for (auto& [loc, node] : pinGCell2Nodes) {
    node->reset();
  }

  
  // build tree
  std::set<frNode*> visitedNodes;
  std::queue<frNode*> nodeQueue;
  visitedNodes.insert(root);
  nodeQueue.push(root);
  while (!nodeQueue.empty()) {
    // pop from front
    auto currNode = nodeQueue.front();
    nodeQueue.pop();
    for (auto adjNode : adjacencyList[currNode]) {
      if (visitedNodes.find(adjNode) == visitedNodes.end()) {
        currNode->addChild(adjNode);
        adjNode->setParent(currNode);
        nodeQueue.push(adjNode);
        visitedNodes.insert(adjNode);
      }
    }
  }

  auto& gcellNode2RPinNodes = net->getGCellNode2RPinNodes();
  auto rootPinNode = net->getRoot();
  // reconnect rpin node to gcell center node
  for (auto& [gcellNode, localNodes] : gcellNode2RPinNodes) {
    for (auto localNode : localNodes) {
      if (localNode == rootPinNode) {
        gcellNode->setParent(localNode);
      } else {
        gcellNode->addChild(localNode);
      }
    }
  }    
}


void FlexGR::initRoute_patternRoute_Z_shape() 
{
  // avoid frequent memory allocation
  std::vector<frNode*> stack;
  stack.reserve(1000);  // reserve some space for the stack
  
  for (auto& net : design_->getTopBlock()->getNets()) {
    if (net->isGRValid() == false) {
      continue;  // net is not valid for GR routing
    }

    stack.push_back(net->getRootGCellNode());
    while (!stack.empty()) {
      auto currNode = stack.back();
      stack.pop_back();

      // push children to the stack
      for (auto child : currNode->getChildren()) {
        stack.push_back(child);
      }

      // check if the current node has a parent
      auto parent = currNode->getParent();
      if (parent == nullptr) {
        continue;  // root node, no routing needed
      }

      if (parent->getType() != frNodeTypeEnum::frcSteiner ||
          currNode->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }

      const auto& cLoc = currNode->getLoc();
      const auto& pLoc = parent->getLoc();
      if (cLoc.x() == pLoc.x() && cLoc.y() == pLoc.y()) {
        continue;  // colinear, no routing needed
      }

      // non-colinear segment, route it using Z-shape pattern
      initRoute_patternRoute_Z_shape(currNode, parent);
    }
  }
}



// This function is used to route non-clinear segments in the routing tree.
// Just for initial routing tree construction.
// Later, we will use GPU-accelerated segment shifting and steiner point ajustment for RRR
void FlexGR::initRoute_patternRoute_Z_shape(frNode* child, frNode* parent)
{
  const Point childGCellIdx = design_->getTopBlock()->getGCellIdx(child->getLoc());
  const Point parentGCellIdx = design_->getTopBlock()->getGCellIdx(parent->getLoc());
  // check the all the combinations of child and parent gcell indices
  // first horizontal, then vertical
  const int xDistMax = std::abs(parentGCellIdx.x() - childGCellIdx.x());
  const int yDistMax = std::abs(parentGCellIdx.y() - childGCellIdx.y());
  
  if (xDistMax == 0 && yDistMax == 0) {
    //logger_->report("[INFO] initRoute_patternRoute_Z_shape: "
    //                  "child and parent are colinear, no routing needed.");
    return;
  }
  
  const int xDelta = (childGCellIdx.x() < parentGCellIdx.x()) ? 1 : -1;
  const int yDelta = (childGCellIdx.y() < parentGCellIdx.y()) ? 1 : -1;

  double bestCost = std::numeric_limits<double>::max();
  int bestXDisp = 0;
  int bestYDisp = 0;
  int xDisp = 0;
  int yDisp = 0;


  // check over horizontal and vertical displacements
  for (int idx = 0; idx < xDistMax; idx++) {
    xDisp += xDelta;   
    const Point cornerGCellIdx1(childGCellIdx.x() + xDisp, childGCellIdx.y());
    const Point cornerGCellIdx2(childGCellIdx.x() + xDisp, parentGCellIdx.y());
    double cost = initRoute_getSegmentCost2D(childGCellIdx, cornerGCellIdx1);
    cost += initRoute_getSegmentCost2D(cornerGCellIdx1, cornerGCellIdx2);
    cost += initRoute_getSegmentCost2D(cornerGCellIdx2, parentGCellIdx);
    if (cost < bestCost) {
      bestCost = cost;
      bestXDisp = xDisp;
      bestYDisp = 0;  // no vertical displacement
    }
  }  

  // check vertical displacements
  for (int idx = 0; idx < yDistMax; idx++) {
    yDisp += yDelta;   
    const Point cornerGCellIdx1(childGCellIdx.x(), childGCellIdx.y() + yDisp);
    const Point cornerGCellIdx2(parentGCellIdx.x(), childGCellIdx.y() + yDisp);
    double cost = initRoute_getSegmentCost2D(childGCellIdx, cornerGCellIdx1);
    cost += initRoute_getSegmentCost2D(cornerGCellIdx1, cornerGCellIdx2);
    cost += initRoute_getSegmentCost2D(cornerGCellIdx2, parentGCellIdx);
    if (cost < bestCost) {
      bestCost = cost;
      bestXDisp = 0;  // no horizontal displacement
      bestYDisp = yDisp;
    }
  }
  
  // remove the connection information
  parent->removeChild(child);
  child->setParent(nullptr);  
  frNode* cornerNode1 = nullptr;
  frNode* cornerNode2 = nullptr;

  // commit the best displacement
  if (bestXDisp == 0) {
    // vertical displacement
    cornerNode1 = addSteinerNodeToNet(child, Point(childGCellIdx.x(), childGCellIdx.y() + bestYDisp), 2);
    cornerNode2 = addSteinerNodeToNet(cornerNode1, Point(parentGCellIdx.x(), childGCellIdx.y() + bestYDisp), 2);
  } else if (bestYDisp == 0) {
    // horizontal displacement
    cornerNode1 = addSteinerNodeToNet(child, Point(childGCellIdx.x() + bestXDisp, childGCellIdx.y()), 2);
    cornerNode2 = addSteinerNodeToNet(cornerNode1, Point(childGCellIdx.x() + bestXDisp, parentGCellIdx.y()), 2);
  } else {
    logger_->error(utl::DRT, 188, 
      "initRoute_patternRoute_Z_shape: "
      "bestXDisp ({}) and bestYDisp ({}) are not zero, this should not happen.",
      bestXDisp, bestYDisp);
  }    

  parent->addChild(cornerNode2);
  cornerNode2->setParent(parent);
}



// ----------------------------------------------------------------------------
// Utility functions for updating congestion
// ----------------------------------------------------------------------------
// The segment connecting the node and its parent should be aligned with the GCell grid.
void FlexGR::initRoute_checkValid(frNet* net, bool printFlag)
{
  if (net->isGRValid() == false) {
    return;  // net is not valid for GR routing
  }
  
  if (printFlag) {
    logger_->report("[INFO] initRoute_checkValid: checking net {} ...", net->getName());
  }

  auto node = net->getRootGCellNode();
  std::queue<frNode*> nodeQueue;
  nodeQueue.push(node);

  while (!nodeQueue.empty()) {
    auto currNode = nodeQueue.front();
    nodeQueue.pop();

    // Get the segment attached to the node
    Point bpIdx, epIdx;
    initRoute_getNodeSegment2D(currNode, bpIdx, epIdx);
    
    // Traverse children
    for (auto& child : currNode->getChildren()) {
      nodeQueue.push(child);
    }
  }

  if (printFlag) {
    logger_->report("[INFO] initRoute_checkValid: the routing tree is valid.");      
  }
}


// Get the segment attached to the node (segment connecting the node and its parent).
bool FlexGR::initRoute_getNodeSegment2D(frNode* node, Point& bpIdx, Point& epIdx, bool errFlag)
{
  // Initialize
  bpIdx.setX(-1);
  bpIdx.setY(-1);
  epIdx.setX(-1);
  epIdx.setY(-1);

  if (node->getParent() == nullptr) return false;
  
  if (node->getType() != frNodeTypeEnum::frcSteiner
    || node->getParent()->getType() != frNodeTypeEnum::frcSteiner) {
    return false;
  }
  
  auto bpLoc = node->getLoc();
  auto epLoc = node->getParent()->getLoc();
  if (epLoc < bpLoc) {
    std::swap(bpLoc, epLoc);
  }


  // At initial routing stage, not all the parent-child pair are aligned
  if (bpLoc.x() != epLoc.x() && bpLoc.y() != epLoc.y()) {
    if (errFlag) {
      logger_->error(utl::DRT, 121, 
        "initRoute_getNodeSegment2D: Node segment is not aligned!"
        " bpLoc: ({}, {}), epLoc: ({}, {}).",
        bpLoc.x(), bpLoc.y(), epLoc.x(), epLoc.y());   
    }
    return false;
  }

  // Transform to GCell indices
  bpIdx = design_->getTopBlock()->getGCellIdx(bpLoc);
  epIdx = design_->getTopBlock()->getGCellIdx(epLoc);

  return true;
}


// bpIdx <= epIdx
void FlexGR::initRoute_updateCongestion2D_Segment(
  const Point& bpIdx, 
  const Point& epIdx)
{
  if (bpIdx.x() < 0 || bpIdx.y() < 0 || 
      epIdx.x() < 0 || epIdx.y() < 0) {
    return;
  }
  
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
    logger_->error(utl::DRT, 111, 
      "initRoute_updateCongestion2D_Segment: Segment is not aligned!");
  } 
}


void FlexGR::initRoute_updateCongestion2D_net(frNet* net, bool errFlag)
{
  // net spanning multiple GCells
  if (net->isGRValid() == false) {
    return;
  }

  Point bpIdx, epIdx;
  // Traverse the entire routing tree of the net  
  for (auto& node : net->getNodes()) {
    // Get the segment attached to the node
    if (initRoute_getNodeSegment2D(node.get(), bpIdx, epIdx, errFlag) == true) {    
      initRoute_updateCongestion2D_Segment(bpIdx, epIdx);
    }
  }
}


// ---------------------------------------------------------------------------------------
// Convert topological connection to physical connection
// ---------------------------------------------------------------------------------------
void FlexGR::initGR_initPhysicalObj()
{
  std::vector<frNode*> stack;
  stack.reserve(1000);  // reserve some space for the stack  
  for (auto& net : design_->getTopBlock()->getNets()) {
    if (net->isGRValid() == false) {
      continue;  // net is not valid for GR routing
    }
 
    stack.clear();
    stack.push_back(net->getRootGCellNode());
    
    while (!stack.empty()) {
      auto currNode = stack.back();
      stack.pop_back();

      // push children to the stack
      for (auto child : currNode->getChildren()) {
        stack.push_back(child);
      }

      // check if the current node has a parent
      auto parent = currNode->getParent();
      if (parent == nullptr) {
        continue;  // root node, no routing needed
      }

      if (parent->getType() != frNodeTypeEnum::frcSteiner ||
          currNode->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }

      if (currNode->getLayerNum() != 2 || parent->getLayerNum() != 2) {
        logger_->error(utl::DRT, 196, 
          "initGR_initPhysicalObj: Node or parent is not on layerNum == 2 , "
          "node layerNum: {}, parent layerNum: {}. "
          "this should not happen.", currNode->getLayerNum(), parent->getLayerNum());
      }
              
      addSegmentToNet(currNode, 2);
    }
  
    // for testing purpose, sanity check
    const int numSteinerNodes = net->getNodes().size() - net->getRPins().size();
    if (numSteinerNodes != 0 && (int) net->getGRShapes().size() != (numSteinerNodes - 1)) {
      logger_->error(utl::DRT, 189, 
        "initGR_initPhysicalObj: Net {} has {} steiner nodes, but {} pathSegs.",
        net->getName(), numSteinerNodes, net->getGRShapes().size());
    }
  }
}




}  // namespace drt
