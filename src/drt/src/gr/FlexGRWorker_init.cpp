/* Initial Authors: Lutong Wang and Bangqi Xu */
/* Updated Version: Zhiang Wang*/
/*
 * Copyright (c) 2024, The Regents of the University of California
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

#include <deque>
#include <iostream>

#include "FlexGR.h"
#include "FlexGR_util.h"
#include "FlexGRCMap.h"
#include "utl/Logger.h"

namespace drt {


// ----------------------------------------------------------------------------------------------
// Step 1: initialize the boundary connections
// ----------------------------------------------------------------------------------------------
void FlexGRWorker::initBoundary()
{
  std::vector<grBlockObject*> result;
  getRegionQuery()->queryGRObj(extBox_, result);
  for (auto rptr : result) {
    if (rptr->typeId() == grcPathSeg) {
      auto cptr = static_cast<grPathSeg*>(rptr);
      if (cptr->hasNet()) {
        initBoundary_splitPathSeg(cptr);
      } else {
        logger_->error(utl::DRT, 230, "FlexGRWorker::initBoundary grcPathSeg hasNet() empty");
      }
    }
  }
}


// split pathSeg at boudnary
void FlexGRWorker::initBoundary_splitPathSeg(grPathSeg* pathSeg)
{
  auto [bp, ep] = pathSeg->getPoints();
  // Case 1: skip if both endpoints are inside extBox (i.e., no intersection)
  if (extBox_.intersects(bp) && extBox_.intersects(ep)) {
    return;
  }
  Point breakPt1, breakPt2;
  initBoundary_splitPathSeg_getBreakPts(bp, ep, breakPt1, breakPt2);
  auto parent = pathSeg->getParent();
  auto child = pathSeg->getChild();

  if (breakPt1 == breakPt2) {
    // Case 2:  break on one side
    initBoundary_splitPathSeg_split(child, parent, breakPt1);
  } else {
    // Case 3: break on both sides
    Point childLoc = child->getLoc();
    if (childLoc == bp) {
      auto breakNode1
          = initBoundary_splitPathSeg_split(child, parent, breakPt1);
      initBoundary_splitPathSeg_split(breakNode1, parent, breakPt2);
    } else {
      auto breakNode2
          = initBoundary_splitPathSeg_split(child, parent, breakPt2);
      initBoundary_splitPathSeg_split(breakNode2, parent, breakPt1);
    }
  }
}

// breakPt1 <= breakPt2 always
void FlexGRWorker::initBoundary_splitPathSeg_getBreakPts(const Point& bp,
                                                         const Point& ep,
                                                         Point& breakPt1,
                                                         Point& breakPt2)
{
  bool hasBreakPt1 = false;
  bool hasBreakPt2 = false;

  if (bp.x() == ep.x()) { // V: break point at bottom
    if (bp.y() < extBox_.yMin()) {
      breakPt1 = {bp.x(), extBox_.yMin()};
      hasBreakPt1 = true;
    }
    if (ep.y() > extBox_.yMax()) {
      breakPt2 = {ep.x(), extBox_.yMax()};
      hasBreakPt2 = true;
    }
  } else if (bp.y() == ep.y()) { // H
    if (bp.x() < extBox_.xMin()) {
      breakPt1 = {extBox_.xMin(), bp.y()};
      hasBreakPt1 = true;
    }
    if (ep.x() > extBox_.xMax()) {
      breakPt2 = {extBox_.xMax(), ep.y()};
      hasBreakPt2 = true;
    }
  } else {
    logger_->error(utl::DRT, 251, 
      "FlexGRWorker::initBoundary_splitPathSeg_getBreakPts non-colinear pathSeg");
  }

  if (hasBreakPt1 && !hasBreakPt2) {
    breakPt2 = breakPt1;
  } else if (!hasBreakPt1 && hasBreakPt2) {
    breakPt1 = breakPt2;
  } else if (hasBreakPt1 && hasBreakPt2) { 
    ;
  } else {
    logger_->error(utl::DRT, 252, 
      "FlexGRWorker::initBoundary_splitPathSeg_getBreakPts no breakPt created");
  }
}


inline frNode* FlexGRWorker::addNodeToNet(
  frNode* child, frNode* parent, frNet* net,
  const Point& breakPt,
  const frLayerNum& lNum,
  frNodeTypeEnum type)
{  
  auto uBreakNode = std::make_unique<frNode>();
  auto breakNode = uBreakNode.get();
  breakNode->addToNet(net);
  breakNode->setLoc(breakPt);
  breakNode->setLayerNum(lNum);
  breakNode->setType(type);
  // update connectivity
  parent->removeChild(child);
  child->setParent(breakNode);
  breakNode->addChild(child);
  breakNode->setParent(parent);
  parent->addChild(breakNode);
  net->addNode(uBreakNode);
  return breakNode;
}
 

inline void FlexGRWorker::addSegmentToNet(
  frNode* child, frNode* parent, frNet* net,
  const Point& bp, const Point& ep,
  const frLayerNum& lNum)
{
  // create new pathSegs
  auto uPathSeg = std::make_unique<grPathSeg>();
  auto pathSeg = uPathSeg.get();
  pathSeg->setChild(child);
  child->setConnFig(pathSeg);
  pathSeg->setParent(parent);
  pathSeg->addToNet(net);
  pathSeg->setPoints(bp, ep);
  pathSeg->setLayerNum(lNum);
  getRegionQuery()->addGRObj(pathSeg);
  // update net ownership
  std::unique_ptr<grShape> uGRConnFig(std::move(uPathSeg));
  net->addGRShape(uGRConnFig);
}


frNode* FlexGRWorker::initBoundary_splitPathSeg_split(frNode* child,
                                                      frNode* parent,
                                                      const Point& breakPt)
{
  // frNet
  auto pathSeg = static_cast<grPathSeg*>(child->getConnFig());
  auto net = pathSeg->getNet();
  auto lNum = pathSeg->getLayerNum();
  auto [bp, ep] = pathSeg->getPoints();
  bool isChildBP = (child->getLoc() == bp);
  auto breakNode = addNodeToNet(
    child, parent, net, breakPt, lNum, frNodeTypeEnum::frcBoundaryPin);

  // create new pathSegs
  // child - breakPt
  isChildBP == true 
            ? addSegmentToNet(child, breakNode, net, bp, breakPt, lNum) 
            : addSegmentToNet(child, breakNode, net, breakPt, ep, lNum);
  
  // breakPt - parent
  isChildBP == true 
            ? addSegmentToNet(breakNode, parent, net, breakPt, ep, lNum) 
            : addSegmentToNet(breakNode, parent, net, bp, breakPt, lNum);


  // update region query
  getRegionQuery()->removeGRObj(pathSeg);
  net->removeGRShape(pathSeg);

  return breakNode;
}


// ----------------------------------------------------------------------------------------------
// Step 2: initialize the grid graph and nets
// ----------------------------------------------------------------------------------------------
void FlexGRWorker::init()
{
  // init gridGraph first because in case there are overlapping nodes, we need
  // to remove all existing route objs (subtract from cmap) and merge
  // overlapping nodes and recreate route objs and add back to cmap to make sure
  // cmap is accurate
  initGridGraph();
  initNets();
}


// ----------------------------------------------------------------------------------------------
// Step 2.1:  initialize the grid graph
// ----------------------------------------------------------------------------------------------
void FlexGRWorker::initGridGraph()
{
  gridGraph_.setCost(workerCongCost_, workerHistCost_);
  gridGraph_.set2D(is2DRouting_);
  gridGraph_.init();

  initGridGraph_fromCMap();
}


// copy from global cmap
void FlexGRWorker::initGridGraph_fromCMap()
{
  auto cmap = getCMap();
  const Point& gcellIdxLL = getRouteGCellIdxLL();
  const Point& gcellIdxUR = getRouteGCellIdxUR();
  const int idxLLX = gcellIdxLL.x();
  const int idxLLY = gcellIdxLL.y();
  const int idxURX = gcellIdxUR.x();
  const int idxURY = gcellIdxUR.y();

  for (int zIdx = 0; zIdx < (int) cmap->getZMap().size(); zIdx++) {
    for (int xIdx = 0; xIdx <= (idxURX - idxLLX); xIdx++) {
      const int cmapXIdx = xIdx + idxLLX;
      for (int yIdx = 0; yIdx <= (idxURY - idxLLY); yIdx++) {
        const int cmapYIdx = yIdx + idxLLY;
        // copy block
        if (cmap->hasBlock(cmapXIdx, cmapYIdx, zIdx, frDirEnum::E)) {
          gridGraph_.setBlock(xIdx, yIdx, zIdx, frDirEnum::E);
        } else {
          gridGraph_.resetBlock(xIdx, yIdx, zIdx, frDirEnum::E);
        }
        if (cmap->hasBlock(cmapXIdx, cmapYIdx, zIdx, frDirEnum::N)) {
          gridGraph_.setBlock(xIdx, yIdx, zIdx, frDirEnum::N);
        } else {
          gridGraph_.resetBlock(xIdx, yIdx, zIdx, frDirEnum::N);
        }
        if (cmap->hasBlock(cmapXIdx, cmapYIdx, zIdx, frDirEnum::U)) {
          gridGraph_.setBlock(xIdx, yIdx, zIdx, frDirEnum::U);
        } else {
          gridGraph_.resetBlock(xIdx, yIdx, zIdx, frDirEnum::U);
        }
        // copy history cost
        gridGraph_.setHistoryCost(
            xIdx, yIdx, zIdx, cmap->getHistoryCost(cmapXIdx, cmapYIdx, zIdx));

        // copy raw demand
        gridGraph_.setRawDemand(
            xIdx,
            yIdx,
            zIdx,
            frDirEnum::E,
            cmap->getRawDemand(cmapXIdx, cmapYIdx, zIdx, frDirEnum::E));
        gridGraph_.setRawDemand(
            xIdx,
            yIdx,
            zIdx,
            frDirEnum::N,
            cmap->getRawDemand(cmapXIdx, cmapYIdx, zIdx, frDirEnum::N));

        // copy supply (raw supply is only used when comparing to raw demand and
        // supply is always integer)
        gridGraph_.setSupply(
            xIdx,
            yIdx,
            zIdx,
            frDirEnum::E,
            cmap->getSupply(cmapXIdx, cmapYIdx, zIdx, frDirEnum::E));
        gridGraph_.setSupply(
            xIdx,
            yIdx,
            zIdx,
            frDirEnum::N,
            cmap->getSupply(cmapXIdx, cmapYIdx, zIdx, frDirEnum::N));
      }
    }
  }
}



// copy back to global cmap
void FlexGRWorker::initGridGraph_back2CMap()
{
  auto cmap = getCMap();
  const Point& gcellIdxLL = getRouteGCellIdxLL();
  const Point& gcellIdxUR = getRouteGCellIdxUR();
  const int idxLLX = gcellIdxLL.x();
  const int idxLLY = gcellIdxLL.y();
  const int idxURX = gcellIdxUR.x();
  const int idxURY = gcellIdxUR.y();

  for (int zIdx = 0; zIdx < (int) cmap->getZMap().size(); zIdx++) {
    for (int xIdx = 0; xIdx <= (idxURX - idxLLX); xIdx++) {
      const int cmapXIdx = xIdx + idxLLX;
      for (int yIdx = 0; yIdx <= (idxURY - idxLLY); yIdx++) {
        const int cmapYIdx = yIdx + idxLLY;
        cmap->setHistoryCost(
          cmapXIdx, cmapYIdx, zIdx, gridGraph_.getHistoryCost(xIdx, yIdx, zIdx));
        // copy raw demand
        cmap->setRawDemand(
          cmapXIdx,
          cmapYIdx,
          zIdx, 
          frDirEnum::E, 
          gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::E));
        cmap->setRawDemand(
          cmapXIdx,
          cmapYIdx,
          zIdx, 
          frDirEnum::N, 
          gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::N));
        // copy supply
        cmap->setSupply(
          cmapXIdx,
          cmapYIdx,
          zIdx, 
          frDirEnum::E, 
          gridGraph_.getSupply(xIdx, yIdx, zIdx, frDirEnum::E));
        cmap->setSupply(
          cmapXIdx,
          cmapYIdx,
          zIdx, 
          frDirEnum::N, 
          gridGraph_.getSupply(xIdx, yIdx, zIdx, frDirEnum::N));
      }
    }
  }
}



// ----------------------------------------------------------------------------------------------
// Step 2.2:  initialize nets and their objects
// ----------------------------------------------------------------------------------------------
// Note that you need to update the parent/child relationship mannully for flexibility
inline grNode* FlexGRWorker::addNodeToGrNet(
  grNet* net,
  const Point& loc,
  const frLayerNum& lNum,
  frNodeTypeEnum type)
{  
  auto uGCellNode = std::make_unique<grNode>();
  auto gcellNode = uGCellNode.get();
  net->addNode(uGCellNode);
  gcellNode->addToNet(net);
  gcellNode->setLoc(loc);
  gcellNode->setLayerNum(lNum);
  gcellNode->setType(type);
  return gcellNode;
} 


inline void FlexGRWorker::addSegmentToGrNet(
  grNet* net,
  grNode* child, grNode* parent,
  const Point& bp, const Point& ep,
  const frLayerNum& lNum,
  bool isExt)
{
  // create new pathSegs
  auto uPathSeg = std::make_unique<grPathSeg>();
  uPathSeg->setChild(child);
  uPathSeg->setParent(parent);
  uPathSeg->addToNet(net);
  uPathSeg->setPoints(bp, ep);
  uPathSeg->setLayerNum(lNum);
  std::unique_ptr<grConnFig> uGRConnFig(std::move(uPathSeg));
  if (isExt) {
    net->addExtConnFig(uGRConnFig);
  } else {
    net->addRouteConnFig(uGRConnFig);
  }
}

inline void FlexGRWorker::addViaToGrNet(
  grNet* net,
  grNode* child, grNode* parent,
  const Point& loc,
  const frLayerNum& beginLayerNum,
  const frLayerNum& endLayerNum,
  bool isExt)
{
  auto uVia = std::make_unique<grVia>();
  uVia->setChild(child);
  uVia->setParent(parent);
  uVia->addToNet(net);
  uVia->setOrigin(loc);
  uVia->setViaDef(design_->getTech()
                         ->getLayer((beginLayerNum + endLayerNum) / 2)
                         ->getDefaultViaDef());
  std::unique_ptr<grConnFig> uGRConnFig(std::move(uVia));
  if (isExt) {
    net->addExtConnFig(uGRConnFig);
  } else {
    net->addRouteConnFig(uGRConnFig);
  }
}


void FlexGRWorker::initNets()
{
  std::set<frNet*, frBlockObjectComp> nets;
  // store the root node for each net
  std::map<frNet*, std::vector<frNode*>, frBlockObjectComp> netRoots;
  // identify the roots for all the nets (especially for feedthrough nets)  
  initNets_roots(nets, netRoots);
  // create the subnets and their objects
  initNets_searchRepair(nets, netRoots);
  initNets_regionQuery();
}


// get all roots of subnets
void FlexGRWorker::initNets_roots(
  std::set<frNet*, frBlockObjectComp>& nets,
  std::map<frNet*, std::vector<frNode*>, frBlockObjectComp>& netRoots)
{
  std::vector<grBlockObject*> result;
  getRegionQuery()->queryGRObj(routeBox_, result);
  for (auto rptr : result) {
    if (rptr->typeId() == grcPathSeg) {
      auto cptr = static_cast<grPathSeg*>(rptr);
      if (cptr->hasNet()) {
        initNetObjs_roots_pathSeg(cptr, nets, netRoots);
      } else {
        logger_->error(utl::DRT, 231, "FlexGRWorker::initNets_roots grcPathSeg hasNet() empty");
      }
    } else if (rptr->typeId() == grcVia) {
      auto cptr = static_cast<grVia*>(rptr);
      if (cptr->hasNet()) {
        initNetObjs_roots_via(cptr, nets, netRoots);
      } else {
        logger_->error(utl::DRT, 232, "FlexGRWorker::initNets_roots grcVia hasNet() empty");
      }
    } else {
      logger_->error(utl::DRT, 240, "FlexGRWorker::initNets_roots unsupported type: {}", rptr->typeId());
    }
  }
}


// if parent of a pathSeg is at the boundary of extBox, the parent is a subnet
// root (i.e., outgoing edge)
void FlexGRWorker::initNetObjs_roots_pathSeg(
  grPathSeg* pathSeg,
  std::set<frNet*, frBlockObjectComp>& nets,
  std::map<frNet*, std::vector<frNode*>, frBlockObjectComp>& netRoots)
{
  auto net = pathSeg->getNet();
  nets.insert(net);

  auto parent = pathSeg->getParent();
  auto parentLoc = parent->getLoc();

  // boundary pin root
  if (parentLoc.x() == extBox_.xMin() || parentLoc.x() == extBox_.xMax()
      || parentLoc.y() == extBox_.yMin() || parentLoc.y() == extBox_.yMax()) {
    netRoots[net].push_back(parent);
  }

  // real root (not a steiner node)
  if (parent->getParent() == net->getRoot()) {
    netRoots[net].push_back(parent->getParent());
  }
}


// all vias from rq must be inside routeBox
// if the parent of the via parent is the same node as frNet root, then the
// grandparent is a subnet root
void FlexGRWorker::initNetObjs_roots_via(
  grVia* via,
  std::set<frNet*, frBlockObjectComp>& nets,
  std::map<frNet*, std::vector<frNode*>, frBlockObjectComp>& netRoots)
{
  auto net = via->getNet();
  nets.insert(net);

  // real root
  auto parent = via->getParent();
  if (parent->getParent() == net->getRoot()) {
    netRoots[net].push_back(parent->getParent());
  }
}


void FlexGRWorker::initNets_searchRepair(
    std::set<frNet*, frBlockObjectComp>& nets,
    std::map<frNet*, std::vector<frNode*>, frBlockObjectComp>& netRoots)
{
  for (auto net : nets) {
    initNet(net, netRoots[net]);
  }
}


// Note that we may update the ripup status of the net
void FlexGRWorker::initNet(frNet* net, const std::vector<frNode*>& netRoots)
{
  // We need to ensure that we do not create duplicate nets
  std::set<frNode*, frBlockObjectComp> uniqueRoots;
  for (auto fRoot : netRoots) {
    if (uniqueRoots.find(fRoot) != uniqueRoots.end()) {
      continue;
    }
    uniqueRoots.insert(fRoot);

    auto uGRNet = std::make_unique<grNet>();
    auto gNet = uGRNet.get();
    gNet->setFrNet(net);

    // deep copy nodes to create subnet (grNet)
    initNet_initNodes(gNet, fRoot);
    initNet_initRoot(gNet);
    initNet_updateCMap(gNet, false);
    initNet_initPinGCellNodes(gNet);
    initNet_updateCMap(gNet, true);
    initNet_initObjs(gNet);
    gNet->setId(nets_.size());
    initNet_addNet(uGRNet);
  }
}

// bfs from root to deep copy nodes and find all pin nodes (either pin or
// boundary pin)
// Be carefully about this function: create gcell center nodes for boundary pins
// and mark them as "dontMove" so that they will not be moved during ripup
// Otherwise, you may have alignment issues when stitching subnets
void FlexGRWorker::initNet_initNodes(grNet* net, frNode* fRoot)
{
  // map from loc to gcell node
  std::vector<std::pair<frNode*, grNode*> > pinNodePairs;
  std::map<grNode*, frNode*, frBlockObjectComp> gr2FrPinNode; // 1-to-1 mapping
  // parent grNode to children frNode
  std::queue<std::pair<grNode*, frNode*> > nodeQ; // parent grNode to children frNode (auxiliary queue)
  nodeQ.emplace(nullptr, fRoot);  
  
  // Helper: create gcell cneter node for a boundary pin
  auto createGCellCenterNode = [&](grNode* parent, const Point& loc, int layerNum) -> grNode* {
    auto node = addNodeToGrNet(net, loc, layerNum, frNodeTypeEnum::frcSteiner);
    node->setDontMove(true);
    node->setParent(parent);
    if (parent) { parent->addChild(node); }
    return node;
  };

  // bfs from root to deep copy nodes (and create gcell center nodes as needed for boundary pins)
  // note that the created gcell center nodes need to be marked as "dontMove"
  // so that they will not be moved during ripup reroute to ensure alignment
  // Even we do not need to create a Steiner node, we still mark the existing node as "dontMove"
  while (!nodeQ.empty()) {
    auto [parent, frChild] = nodeQ.front(); 
    nodeQ.pop();
    const bool isRoot = (parent == nullptr);
    // Get locations
    const auto& parentLoc = isRoot ? Point() : parent->getLoc();
    const auto& frChildLoc = frChild->getLoc();
    // if parent is boudnary pin and there is no immediate child at the gcell
    // center location, need to create a gcell center node
    bool needGenParentGCellNode = false;
    // if child is boudnary pin and its parent is not at the gcell center
    // location, need to create a gcell center node
    bool needGenChildGCellNode = false;
    // root at boundary
    if (!isRoot && parent->getType() == frNodeTypeEnum::frcBoundaryPin) {
      if (getBoundaryPinGCellNodeLoc(parentLoc) != frChildLoc) {
        needGenParentGCellNode = true;
      }
    }
     
    // leaf at boundary
    // Make sure thagt needGenParentGCellNode is false
    if (!needGenParentGCellNode && !isRoot &&
        frChild->getType() == frNodeTypeEnum::frcBoundaryPin) {
      if (getBoundaryPinGCellNodeLoc(frChildLoc) != parentLoc) {
        needGenChildGCellNode = true;
      }
    }

    grNode* newNode = nullptr;
    // deep copy node -- if need to generate (either parent or child), gen new
    // node instead of copy
    if (needGenParentGCellNode) {
      newNode = createGCellCenterNode(parent, getBoundaryPinGCellNodeLoc(parentLoc), parent->getLayerNum());
      nodeQ.emplace(newNode, frChild);
      continue;
    }

    if (needGenChildGCellNode) {
      newNode = createGCellCenterNode(parent, getBoundaryPinGCellNodeLoc(frChildLoc), frChild->getLayerNum());
      nodeQ.emplace(newNode, frChild);
      continue;
    }
    
    // loc, layerNum and type are set by copy constructor
    auto uChildNode = std::make_unique<grNode>(*frChild);
    auto childNode = uChildNode.get();
    net->addNode(uChildNode);
    childNode->addToNet(net);
    if (!isRoot) {
      childNode->setParent(parent);
      parent->addChild(childNode);
    } else {
      childNode->setParent(nullptr); // root node does not have parent
      childNode->setDontMove(true); // root is a boundary pin, so it should not be moved  
    }

    if (!isRoot && (parent->getType() == frNodeTypeEnum::frcBoundaryPin || frChild->getType() == frNodeTypeEnum::frcBoundaryPin)) {
      // if parent or child is boundary pin, mark the node as dontMove
      childNode->setDontMove(true);
    }
     
    newNode = childNode;
    // Handle pins and roots
    // add to pinNodePairs
    if (frChild->getType() == frNodeTypeEnum::frcBoundaryPin
      || frChild->getType() == frNodeTypeEnum::frcPin) {
      pinNodePairs.emplace_back(frChild, childNode);
      gr2FrPinNode[childNode] = frChild;
      if (isRoot) {  net->setFrRoot(frChild);  }
    }
  
    // We need to continue to traverse even if the frChild is a boundary pin
    if (isRoot || frChild->getType() == frNodeTypeEnum::frcSteiner) {
      for (auto grandChild : frChild->getChildren()) {
        nodeQ.emplace(newNode, grandChild);
      }
    }
  }
  net->setPinNodePairs(pinNodePairs);
  net->setGR2FrPinNode(gr2FrPinNode);
}

Point FlexGRWorker::getBoundaryPinGCellNodeLoc(const Point& boundaryPinLoc)
{
  Point gcellNodeLoc;
  if (boundaryPinLoc.x() == extBox_.xMin()) {
    gcellNodeLoc = {routeBox_.xMin(), boundaryPinLoc.y()};
  } else if (boundaryPinLoc.x() == extBox_.xMax()) {
    gcellNodeLoc = {routeBox_.xMax(), boundaryPinLoc.y()};
  } else if (boundaryPinLoc.y() == extBox_.yMin()) {
    gcellNodeLoc = {boundaryPinLoc.x(), routeBox_.yMin()};
  } else if (boundaryPinLoc.y() == extBox_.yMax()) {
    gcellNodeLoc = {boundaryPinLoc.x(), routeBox_.yMax()};
  } else {
    logger_->error(utl::DRT, 241, "FlexGRWorker::getBoundaryPinGCellNodeLoc non-boundary pin loc");
  }
  return gcellNodeLoc;
}

void FlexGRWorker::initNet_initRoot(grNet* net)
{
  // set root
  auto rootNode = net->getNodes().front().get();
  net->setRoot(rootNode);
  // sanity check
  if (rootNode->getType() == frNodeTypeEnum::frcBoundaryPin) {
    Point rootLoc = rootNode->getLoc();
    if (extBox_.intersects(rootLoc) && routeBox_.intersects(rootLoc)) {
      logger_->error(utl::DRT, 244, 
        "FlexGRWorker::initNet_initRoot root should be on an outgoing edge");
    }
  } else if (rootNode->getType() == frNodeTypeEnum::frcPin) {
    if (rootNode->getLoc() != rootNode->getNet()->getFrNet()->getRoot()->getLoc()) {
      logger_->error(utl::DRT, 356, 
        "FlexGRWorker::initNet_initRoot local root and global root location mismatch");
    }
  } else {
    logger_->error(utl::DRT, 357, "FlexGRWorker::initNet_initRoot root should not be steiner");
  }
}

// based on topology, add / remove congestion map (in gridGraph)
void FlexGRWorker::initNet_updateCMap(grNet* net, bool isAdd)
{
  std::queue<grNode*> nodeQ;
  nodeQ.push(net->getRoot());
  // Lambda to handle demand update for a segment
  auto updateDemand = [&](int x, int y, int z, frDirEnum dir, bool add) {
    if (add) {
      gridGraph_.addRawDemand(x, y, z, dir);
    } else {
      gridGraph_.subRawDemand(x, y, z, dir);
    }
  };

  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop();    
    // push steiner child to nodeQ
    for (auto child : node->getChildren()) {
      if (child->getType() == frNodeTypeEnum::frcSteiner) {
        nodeQ.push(child);
      }
    }

    if (node->getParent() == nullptr ||
        node->getParent()->getType() != frNodeTypeEnum::frcSteiner
        || node->getType() != frNodeTypeEnum::frcSteiner) {
      continue; // only update for connections between steiner nodes
    }

    // currently only update for pathSeg
    auto parent = node->getParent();
    // To do: we need to add the demand for vias later
    if (parent->getLayerNum() != node->getLayerNum()) {
      continue; // to do: add via demand
    }    

    const auto& parentLoc = parent->getLoc();
    const auto& nodeLoc = node->getLoc();
    const frLayerNum lNum = parent->getLayerNum();
    const Point& bp = (parentLoc < nodeLoc) ? parentLoc : nodeLoc;
    const Point& ep = (parentLoc < nodeLoc) ? nodeLoc   : parentLoc;
    FlexMazeIdx bi, ei;
    gridGraph_.getMazeIdx(bp, lNum, bi);
    gridGraph_.getMazeIdx(ep, lNum, ei);  
    if (bi.x() == ei.x()) { // vertical pathSeg
      for (auto yIdx = bi.y(); yIdx < ei.y(); yIdx++) {
        updateDemand(bi.x(), yIdx,     bi.z(), frDirEnum::N, isAdd);
        updateDemand(bi.x(), yIdx + 1, bi.z(), frDirEnum::N, isAdd);
      }
    } else if (bi.y() == ei.y()) {
      // horizontal pathSeg
      for (auto xIdx = bi.x(); xIdx < ei.x(); ++xIdx) {
        updateDemand(xIdx,     bi.y(), bi.z(), frDirEnum::E, isAdd);
        updateDemand(xIdx + 1, bi.y(), bi.z(), frDirEnum::E, isAdd);
      }
    } else {
      logger_->error(utl::DRT, 360, "FlexGRWorker::initNet_updateCMap non-colinear pathSeg");
    }
  }
}

// initialize pinGCellNodes as well as removing overlapping pinGCellNodes
// In this function, we may mark nets with overlapping pinGCellNodes as ripup
// so that they can be rerouted later
// They may be some loops in the routing tree, which may lead to pinGCellNodes overlap
// We just break the loop by connecting the child node to "unique" parent pinGCellNode
// After this function, there may still be some loops in the routing tree
// We may have some dangling segments, which will be removed during ripup and reroute
void FlexGRWorker::initNet_initPinGCellNodes(grNet* net)
{
  std::vector<std::pair<grNode*, grNode*> > pinGCellNodePairs;
  std::map<grNode*, std::vector<grNode*>, frBlockObjectComp> gcell2PinNodes;
  std::vector<grNode*> pinGCellNodes;
  std::map<FlexMazeIdx, grNode*> midx2PinGCellNode;
  grNode* rootGCellNode = nullptr;

  std::queue<grNode*> nodeQ;
  nodeQ.push(net->getRoot());
  
  while (!nodeQ.empty()) {
    auto node = nodeQ.front();    
    nodeQ.pop();
    // push children to queue
    for (auto child : node->getChildren()) {
      nodeQ.push(child);
    }

    grNode* gcellNode = nullptr;
    if (node == net->getRoot()) {
      gcellNode = node->getChildren().front();
    } else if (!node->hasChildren()) {
      gcellNode = node->getParent();
    } else {
      continue;
    }

    if (!rootGCellNode) {
      rootGCellNode = gcellNode;
    }

    Point gcellNodeLoc = gcellNode->getLoc();
    frLayerNum gcellNodeLNum = gcellNode->getLayerNum();
    FlexMazeIdx gcellNodeMIdx;
    gridGraph_.getMazeIdx(gcellNodeLoc, gcellNodeLNum, gcellNodeMIdx);

    // Handle possible overlap of pinGCellNode 
    auto [it, inserted] = midx2PinGCellNode.try_emplace(gcellNodeMIdx, gcellNode);
    if (!inserted && gcellNode != it->second) {
      // disjoint pinGCellNode overlap
      std::string warn_msg = 
        std::string("FlexGRWorker::initNet_initPinGCellNodes overlapping disjoint pinGCellNodes detected of ") 
        + net->getFrNet()->getName();
      // ripup to make sure congestion map is up-to-date
      net->setRipup(true);
      if (it->second == rootGCellNode) {
        // loop back to rootGCellNode, need to directly connect child node to root
        node->getParent()->removeChild(node);
        if (node->getConnFig()) {
          auto pathSeg = static_cast<grPathSeg*>(node->getConnFig());
          pathSeg->setParent(rootGCellNode);
        }
        node->setParent(rootGCellNode);
        rootGCellNode->addChild(node);
        gcellNode = rootGCellNode;
        warn_msg += "  between rootGCellNode and non-rootGCellNode";
      } else {
        // merge the gcellNode to existing leaf node (directly connect all
        // children of the gcellNode to existing leaf) do not care about their
        // parents because it will be rerouted anyway
        node->getParent()->removeChild(node);
        if (node->getConnFig()) {
          auto pathSeg = static_cast<grPathSeg*>(node->getConnFig());
          pathSeg->setParent(it->second);
        }
        node->setParent(it->second);
        it->second->addChild(node);
        gcellNode = it->second;
        warn_msg += "  between non-rootGCellNode and non-rootGCellNode";
      }
      logger_->warn(utl::DRT, 238, warn_msg);        
    }

    // Track pinGCellNode pairs
    pinGCellNodePairs.emplace_back(node, gcellNode);
    if (gcell2PinNodes.find(gcellNode) == gcell2PinNodes.end()) {
      pinGCellNodes.push_back(gcellNode);
    }
    gcell2PinNodes[gcellNode].push_back(node);
  }

  net->setPinGCellNodePairs(pinGCellNodePairs);
  net->setGCell2PinNodes(gcell2PinNodes);
  net->setPinGCellNodes(pinGCellNodes);
  if (pinGCellNodes.size() == 1) {
    net->setTrivial(true);
  } else if (pinGCellNodes.size() > 1) {
    net->setTrivial(false);
  } else {
    logger_->error(utl::DRT, 361, 
       "FlexGRWorker::initNet_initPinGCellNodes pinGCellNodes should contain at least one element");
  }
}


// generate route and ext objs based on subnet tree node information
void FlexGRWorker::initNet_initObjs(grNet* net)
{
  std::queue<grNode*> nodeQ;
  nodeQ.push(net->getRoot());
  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop();
    // push children to queue
    for (auto child : node->getChildren()) {
      nodeQ.push(child);
    }

    // generate and add objects
    // root does not have connFig to parent
    if (node->getParent() == nullptr) {
      continue;
    }
    
    // no pathSeg will be created between pin node and gcell that contains the
    // pin node
    if (node->getType() == frNodeTypeEnum::frcPin
      || node->getParent()->getType() == frNodeTypeEnum::frcPin) {
      continue;
    }
    
    bool isExt = false;
    // short pathSeg between boundary pin and gcell that contains boundary pin
    // are extConnFig and will not be touched during ripup reroute
    if (node->getType() == frNodeTypeEnum::frcBoundaryPin
      || node->getParent()->getType() == frNodeTypeEnum::frcBoundaryPin) {
      isExt = true;
    }


    auto parent = node->getParent();
    const int parentLayerNum = parent->getLayerNum();
    const int childLayerNum = node->getLayerNum();
    const Point& childLoc = node->getLoc();
    const Point& parentLoc = parent->getLoc();
    bool isChildBP = (childLoc < parentLoc);
    if (childLayerNum == parentLayerNum) {
      isChildBP == true
                ? addSegmentToGrNet(net, node, parent, childLoc, parentLoc, childLayerNum, isExt)
                : addSegmentToGrNet(net, node, parent, parentLoc, childLoc, childLayerNum, isExt);
    } else {
      addViaToGrNet(net, node, parent, childLoc, childLayerNum, parentLayerNum, isExt);
    }
  }
}

void FlexGRWorker::initNet_addNet(std::unique_ptr<grNet>& in)
{
  owner2nets_[in->getFrNet()].push_back(in.get());
  nets_.push_back(std::move(in));
}

void FlexGRWorker::initNets_regionQuery()
{
  auto& workerRegionQuery = getWorkerRegionQuery();
  workerRegionQuery.init(false );
}


}  // namespace drt
