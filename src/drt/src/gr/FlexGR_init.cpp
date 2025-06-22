/* Authors: Lutong Wang and Bangqi Xu */
/* Updated version:  Zhiang Wang*/
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

#include <deque>
#include <iostream>
#include <vector>

#include "FlexGR.h"
#include "FlexGRCMap.h"

namespace drt {

void FlexGR::init()
{
  initGCell();
  initLayerPitch();
  initCMap();
}


void FlexGR::initGCell()
{
  auto& gcellPatterns = block_->getGCellPatterns();
  if (!gcellPatterns.empty()) { return; } // already initialized
  // Originallly, we use M1 as the default layer for GCell
  // We use M2 as the default layer for GCell 
  const auto layer = design_->getTech()->getLayer(4); // 4 for M2, 2 for M1
  const auto pitch = layer->getPitch();
  logger_->info(utl::DRT, 83,
      "\nGenerating GCell with size = 15 tracks, using layer {} pitch = {}",
      layer->getName(),
      pitch / static_cast<double>(block_->getDBUPerUU()));
  
  const Rect dieBox = block_->getDieBox();
  frCoord startCoordX = dieBox.xMin() - tech_->getManufacturingGrid();
  frCoord startCoordY = dieBox.yMin() - tech_->getManufacturingGrid();
  frCoord gcellGridSize = pitch * 15; // 15 tracks per GCell
  if (gcellGridSize == 0) {
    logger_->error(utl::DRT, 224,
        "Error: GCell grid size is zero for layer {} with pitch {}",
        layer->getName(),
        pitch / static_cast<double>(block_->getDBUPerUU()));
  }

  frGCellPattern xgp, ygp;
  // set xgp
  xgp.setHorizontal(false);
  xgp.setStartCoord(startCoordX);
  xgp.setSpacing(gcellGridSize);
  xgp.setCount((dieBox.xMax() - startCoordX) / gcellGridSize);
  // set ygp
  ygp.setHorizontal(true);
  ygp.setStartCoord(startCoordY);
  ygp.setSpacing(gcellGridSize);
  ygp.setCount((dieBox.yMax() - startCoordY) / gcellGridSize);
  block_->setGCellPatterns({xgp, ygp});
}


void FlexGR::initLayerPitch()
{
  int numRoutingLayer = 0;
  auto& layers = tech_->getLayers();
  for (auto& layer : layers) {
    if (layer->getType() == dbTechLayerType::ROUTING) {
      numRoutingLayer++;
    }
  }

  // init pitches
  trackPitches_.resize(numRoutingLayer, -1);
  line2ViaPitches_.resize(numRoutingLayer, -1);
  layerPitches_.resize(numRoutingLayer, -1);
  zHeights_.resize(numRoutingLayer, 0);

  int bottomRoutingLayerNum = tech_->getBottomLayerNum();
  int topRoutingLayerNum = tech_->getTopLayerNum();
  // Layer 0: active layer,  Layer 1: Fr_VIA layer
  // M1 is layer 2, M2 is layer 4, etc.
  int zIdx = 0;
  for (auto& layer : layers) {
    if (layer->getType() != dbTechLayerType::ROUTING) {
      continue;
    }

    const int lNum = (zIdx + 1) * 2;
    bool isLayerHorz = (layer->getDir() == dbTechLayerDir::HORIZONTAL);
    // get track pitch (pick the smallest one from all the track patterns at current layer)
    for (auto& tp : block_->getTrackPatterns(lNum)) {
      if ((isLayerHorz && !tp->isHorizontal())
          || (!isLayerHorz && tp->isHorizontal())) {
        if (trackPitches_[zIdx] == -1
            || (int) tp->getTrackSpacing() < trackPitches_[zIdx]) {
          trackPitches_[zIdx] = tp->getTrackSpacing();
        }
      }
    }

    // calculate line-2-via pitch
    frCoord defaultWidth = layer->getWidth();
    frCoord minLine2ViaPitch = -1;

    // Make sure that you are using C++17 or later
    auto processVia = [&](const frViaDef* viaDef, bool isUpVia) -> void{
      if (viaDef == nullptr) { return; }
      frVia via(viaDef);
      Rect viaBox = isUpVia ? via.getLayer1BBox() : via.getLayer2BBox();
      frCoord enclosureWidth = viaBox.minDXDY();
      frCoord prl = isLayerHorz ? (viaBox.xMax() - viaBox.xMin())
                                : (viaBox.yMax() - viaBox.yMin());
      // calculate minNonOvlpDist
      frCoord minNonOvlpDist = defaultWidth / 2;
      minNonOvlpDist += isLayerHorz ? (viaBox.yMax() - viaBox.yMin()) / 2
                                    : (viaBox.xMax() - viaBox.xMin()) / 2;
      
      // calculate minSpc required val
      frCoord minReqDist = std::numeric_limits<int>::min();
      auto con = layer->getMinSpacing();
      if (con) {
        switch (con->typeId()) {
          case frConstraintTypeEnum::frcSpacingConstraint:
            minReqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
            break;
          case frConstraintTypeEnum::frcSpacingTablePrlConstraint:
            minReqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(
                std::max(enclosureWidth, defaultWidth), prl);
            break;
          case frConstraintTypeEnum::frcSpacingTableTwConstraint:
            minReqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(
                enclosureWidth, defaultWidth, prl);
            break;
          default:
            break;
        }
      }

      if (minReqDist != std::numeric_limits<int>::min()) {
        minReqDist += minNonOvlpDist;
        if (line2ViaPitches_[zIdx] == -1 || minReqDist > line2ViaPitches_[zIdx]) {
          line2ViaPitches_[zIdx] = minReqDist;
        }

        if (minLine2ViaPitch == -1 || minReqDist < minLine2ViaPitch) {
          minLine2ViaPitch = minReqDist;
        }
      }
    };

    // calculate line-2-via pitch
    if (bottomRoutingLayerNum <= lNum - 1) {
      processVia(tech_->getLayer(lNum - 1)->getDefaultViaDef(), false); // down via
    }
    
    if (topRoutingLayerNum >= lNum + 1) {
      processVia(tech_->getLayer(lNum + 1)->getDefaultViaDef(), true); // up via
    }

    // Zhiang: I do not understand the logic here
    // Should we use line2ViaPitches_[zIdx] instead of minLine2ViaPitch
    // I think we should use the maximum of line2ViaPitches_[zIdx] and trackPitches_[zIdx]
    // Original implementation:
    // if (minLine2ViaPitch > trackPitches_[zIdx]) {
    //  layerPitches_[zIdx] = line2ViaPitches_[zIdx];
    // } else {
    //  layerPitches_[zIdx] = trackPitches_[zIdx];
    //}
    // For the NG45, I notice that the line2ViaPitches_[zIdx] is smaller than trackPitches_[zIdx]
    // Updated by Zhiang:
    layerPitches_[zIdx] = std::max(line2ViaPitches_[zIdx], trackPitches_[zIdx]);
    // Zhiang: we need to understand how the zHeights_ are used
    zHeights_[zIdx] = zIdx == 0 ? layerPitches_[zIdx]
                                : zHeights_[zIdx - 1] + layerPitches_[zIdx];
    
    // print the layer information
    std::string orientation = isLayerHorz ? "H" : "V";
    double layerPitchUU = layer->getPitch() / static_cast<double>(block_->getDBUPerUU());
    double trackPitchUU = trackPitches_[zIdx] / static_cast<double>(block_->getDBUPerUU());
    double line2ViaPitchUU = line2ViaPitches_[zIdx] / static_cast<double>(block_->getDBUPerUU());
    std::string warningMsg = "";
    if (trackPitches_[zIdx] < line2ViaPitches_[zIdx]) {
      warningMsg = "(Warning: Track pitch is smaller than line-2-via pitch!)";
    } 
    logger_->info(utl::DRT, 77,
                  "Layer {}: layerNum = {}, zIdx = {}, Preferred dirction = {}, "
                  "Layer Pitch = {:0.5f}, Track Pitch = {:0.5f}, Line-2-Via Pitch = {:0.5f} "
                  "{}",
                  layer->getName(), lNum, zIdx, orientation, 
                  layerPitchUU, trackPitchUU, line2ViaPitchUU, warningMsg);
    // Move to the next layer
    zIdx++;
  }
}

void FlexGR::initCMap()
{
  logger_->info(utl::DRT, 225, "Initializing congestion map...");
  auto cmap = std::make_unique<FlexGRCMap>(design_, router_cfg_, logger_);
  cmap->setLayerTrackPitches(trackPitches_);
  cmap->setLayerLine2ViaPitches(line2ViaPitches_);
  cmap->setLayerPitches(layerPitches_);
  cmap->init();
  auto cmap2D = std::make_unique<FlexGRCMap>(design_, router_cfg_, logger_);
  cmap2D->initFrom3D(cmap.get());
  setCMap(cmap);
  setCMap2D(cmap2D);
}

// FlexGRWorker related
/*
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
        std::cout << "Error: initNetObjs hasNet() empty" << std::endl;
      }
    }
  }
}

// split pathSeg at boudnary
void FlexGRWorker::initBoundary_splitPathSeg(grPathSeg* pathSeg)
{
  auto [bp, ep] = pathSeg->getPoints();
  // skip if both endpoints are inside extBox (i.e., no intersection)
  if (extBox_.intersects(bp) && extBox_.intersects(ep)) {
    return;
  }
  Point breakPt1, breakPt2;
  initBoundary_splitPathSeg_getBreakPts(bp, ep, breakPt1, breakPt2);
  auto parent = pathSeg->getParent();
  auto child = pathSeg->getChild();

  if (breakPt1 == breakPt2) {
    // break on one side
    initBoundary_splitPathSeg_split(child, parent, breakPt1);
  } else {
    // break on both sides

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

  if (bp.x() == ep.x()) {
    // V
    // break point at bottom
    if (bp.y() < extBox_.yMin()) {
      breakPt1 = {bp.x(), extBox_.yMin()};
      hasBreakPt1 = true;
    }
    if (ep.y() > extBox_.yMax()) {
      breakPt2 = {ep.x(), extBox_.yMax()};
      hasBreakPt2 = true;
    }
  } else if (bp.y() == ep.y()) {
    // H
    if (bp.x() < extBox_.xMin()) {
      breakPt1 = {extBox_.xMin(), bp.y()};
      hasBreakPt1 = true;
    }
    if (ep.x() > extBox_.xMax()) {
      breakPt2 = {extBox_.xMax(), ep.y()};
      hasBreakPt2 = true;
    }
  } else {
    std::cout << "Error: jackpot in "
                 "FlexGRWorker::initBoundary_splitPathSeg_getBreakPts\n";
  }

  if (hasBreakPt1 && !hasBreakPt2) {
    breakPt2 = breakPt1;
  } else if (!hasBreakPt1 && hasBreakPt2) {
    breakPt1 = breakPt2;
  } else if (hasBreakPt1 && hasBreakPt2) {
    ;
  } else {
    std::cout << "Error: at least one breakPt should be created in "
                 "FlexGRWorker::initBoundary_splitPathSeg_getBreakPts\n";
  }
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
  Point childLoc = child->getLoc();
  bool isChildBP = (childLoc == bp);

  auto uBreakNode = std::make_unique<frNode>();
  auto breakNode = uBreakNode.get();
  breakNode->addToNet(net);
  breakNode->setLoc(breakPt);
  breakNode->setLayerNum(lNum);
  breakNode->setType(frNodeTypeEnum::frcBoundaryPin);

  // update connectivity
  parent->removeChild(child);
  child->setParent(breakNode);
  breakNode->addChild(child);
  breakNode->setParent(parent);
  parent->addChild(breakNode);

  net->addNode(uBreakNode);

  // create new pathSegs
  // child - breakPt
  auto uPathSeg1 = std::make_unique<grPathSeg>();
  auto pathSeg1 = uPathSeg1.get();
  pathSeg1->setChild(child);
  child->setConnFig(pathSeg1);
  pathSeg1->setParent(breakNode);
  pathSeg1->addToNet(net);
  if (isChildBP) {
    pathSeg1->setPoints(bp, breakPt);
  } else {
    pathSeg1->setPoints(breakPt, ep);
  }
  pathSeg1->setLayerNum(lNum);

  // breakPt - parent
  auto uPathSeg2 = std::make_unique<grPathSeg>();
  auto pathSeg2 = uPathSeg2.get();
  pathSeg2->setChild(breakNode);
  breakNode->setConnFig(pathSeg2);
  pathSeg2->setParent(parent);
  if (isChildBP) {
    pathSeg2->setPoints(breakPt, ep);
  } else {
    pathSeg2->setPoints(bp, breakPt);
  }
  pathSeg2->setLayerNum(lNum);

  // update region query
  getRegionQuery()->removeGRObj(pathSeg);
  getRegionQuery()->addGRObj(pathSeg1);
  getRegionQuery()->addGRObj(pathSeg2);

  // update net ownership
  std::unique_ptr<grShape> uGRConnFig1(std::move(uPathSeg1));
  std::unique_ptr<grShape> uGRConnFig2(std::move(uPathSeg2));

  net->addGRShape(uGRConnFig1);
  net->addGRShape(uGRConnFig2);

  net->removeGRShape(pathSeg);

  return breakNode;
}


void FlexGRWorker::init()
{
  // init gridGraph first because in case there are overlapping nodes, we need
  // to remove all existing route objs (subtract from cmap) and merge
  // overlapping nodes and recreate route objs and add back to cmap to make sure
  // cmap is accurate
  initGridGraph();

  initNets();
}
*/



void FlexGRWorker::initNets()
{
  std::set<frNet*, frBlockObjectComp> nets;
  std::map<frNet*, std::vector<frNode*>, frBlockObjectComp> netRoots;

  initNets_roots(nets, netRoots);
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
        std::cout << "Error: initNetObjs hasNet() empty" << std::endl;
      }
    } else if (rptr->typeId() == grcVia) {
      auto cptr = static_cast<grVia*>(rptr);
      if (cptr->hasNet()) {
        initNetObjs_roots_via(cptr, nets, netRoots);
      } else {
        std::cout << "Error: initNetObjs hasNet() empty" << std::endl;
      }
    } else {
      std::cout << rptr->typeId() << std::endl;
      std::cout << "Error: initNetObjs unsupported type" << std::endl;
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

  // real root
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

void FlexGRWorker::initNet(frNet* net, const std::vector<frNode*>& netRoots)
{
  std::set<frNode*, frBlockObjectComp> uniqueRoots;
  for (auto fRoot : netRoots) {
    if (uniqueRoots.find(fRoot) != uniqueRoots.end()) {
      continue;
    }
    uniqueRoots.insert(fRoot);

    auto uGRNet = std::make_unique<grNet>();
    auto gNet = uGRNet.get();
    gNet->setFrNet(net);

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
void FlexGRWorker::initNet_initNodes(grNet* net, frNode* fRoot)
{
  // map from loc to gcell node
  // std::map<std::pair<Point, frlayerNum>, grNode*> loc2GCellNode;
  std::vector<std::pair<frNode*, grNode*>> pinNodePairs;
  std::map<grNode*, frNode*, frBlockObjectComp> gr2FrPinNode;
  // parent grNode to children frNode
  std::deque<std::pair<grNode*, frNode*>> nodeQ;
  nodeQ.emplace_back(nullptr, fRoot);

  grNode* parent = nullptr;
  frNode* child = nullptr;

  // bfs from root to deep copy nodes (and create gcell center nodes as needed)
  while (!nodeQ.empty()) {
    parent = nodeQ.front().first;
    child = nodeQ.front().second;
    nodeQ.pop_front();

    bool isRoot = (parent == nullptr);
    Point parentLoc;
    if (!isRoot) {
      parentLoc = parent->getLoc();
    }
    auto childLoc = child->getLoc();

    // if parent is boudnary pin and there is no immediate child at the gcell
    // center location, need to create a gcell center node
    bool needGenParentGCellNode = false;
    // if child is boudnary pin and its parent is not at the gcell center
    // location, need to create a gcell center node
    bool needGenChildGCellNode = false;

    // root at boundary
    if (!isRoot && parent->getType() == frNodeTypeEnum::frcBoundaryPin) {
      Point gcellLoc = getBoundaryPinGCellNodeLoc(parentLoc);
      // if parent is boundary pin, child must be on the same layer
      if (gcellLoc != childLoc) {
        needGenParentGCellNode = true;
      }
    }
    // leaf at boundary
    if (!needGenParentGCellNode && !isRoot
        && child->getType() == frNodeTypeEnum::frcBoundaryPin) {
      Point gcellLoc = getBoundaryPinGCellNodeLoc(childLoc);
      // no need to gen gcell node if immediate parent is the gcell node we need
      if (gcellLoc != parentLoc) {
        needGenChildGCellNode = true;
      }
    }

    grNode* newNode = nullptr;
    // deep copy node -- if need to generate (either parent or child), gen new
    // node instead of copy
    if (needGenParentGCellNode) {
      auto uGCellNode = std::make_unique<grNode>();
      auto gcellNode = uGCellNode.get();
      net->addNode(uGCellNode);

      gcellNode->addToNet(net);
      Point gcellNodeLoc = getBoundaryPinGCellNodeLoc(parentLoc);

      gcellNode->setLoc(gcellNodeLoc);
      gcellNode->setLayerNum(parent->getLayerNum());
      gcellNode->setType(frNodeTypeEnum::frcSteiner);
      // update connections
      gcellNode->setParent(parent);
      parent->addChild(gcellNode);

      newNode = gcellNode;
    } else if (needGenChildGCellNode) {
      auto uGCellNode = std::make_unique<grNode>();
      auto gcellNode = uGCellNode.get();
      net->addNode(uGCellNode);

      gcellNode->addToNet(net);
      Point gcellNodeLoc = getBoundaryPinGCellNodeLoc(childLoc);

      gcellNode->setLoc(gcellNodeLoc);
      gcellNode->setLayerNum(child->getLayerNum());
      gcellNode->setType(frNodeTypeEnum::frcSteiner);
      // update connectons
      gcellNode->setParent(parent);
      parent->addChild(gcellNode);

      newNode = gcellNode;
    } else {
      auto uChildNode = std::make_unique<grNode>(*child);
      auto childNode = uChildNode.get();
      net->addNode(uChildNode);

      childNode->addToNet(net);
      // loc, layerNum, type and pin are set by copy constructor
      // root does not have parent node
      if (!isRoot) {
        childNode->setParent(parent);
        parent->addChild(childNode);
      }

      newNode = childNode;

      // add to pinNodePairs
      if (child->getType() == frNodeTypeEnum::frcBoundaryPin
          || child->getType() == frNodeTypeEnum::frcPin) {
        pinNodePairs.emplace_back(child, childNode);
        gr2FrPinNode[childNode] = child;

        if (isRoot) {
          net->setFrRoot(child);
        }
      }
    }

    // push edge to queue
    if (needGenParentGCellNode) {
      nodeQ.emplace_back(newNode, child);
    } else if (needGenChildGCellNode) {
      nodeQ.emplace_back(newNode, child);
    } else {
      if (isRoot || child->getType() == frNodeTypeEnum::frcSteiner) {
        for (auto grandChild : child->getChildren()) {
          nodeQ.emplace_back(newNode, grandChild);
        }
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
    std::cout << "Error: non-boundary pin loc in "
                 "FlexGRWorker::getBoundaryPinGCellNodeLoc\n";
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
      std::cout << "Error: root should be on an outgoing edge\n";
    }
  } else if (rootNode->getType() == frNodeTypeEnum::frcPin) {
    Point rootLoc = rootNode->getLoc();
    Point globalRootLoc = rootNode->getNet()->getFrNet()->getRoot()->getLoc();
    if (rootLoc != globalRootLoc) {
      std::cout << "Error: local root and global root location mismatch\n";
    }
  } else {
    std::cout << "Error: root should not be steiner\n";
  }
}

// based on topology, add / remove congestion map (in gridGraph)
void FlexGRWorker::initNet_updateCMap(grNet* net, bool isAdd)
{
  std::deque<grNode*> nodeQ;
  nodeQ.push_back(net->getRoot());

  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop_front();

    // push steiner child to nodeQ
    for (auto child : node->getChildren()) {
      if (child->getType() == frNodeTypeEnum::frcSteiner) {
        nodeQ.push_back(child);
      }
    }

    // update cmap for connections between steiner nodes
    if (node->getParent()
        && node->getParent()->getType() == frNodeTypeEnum::frcSteiner
        && node->getType() == frNodeTypeEnum::frcSteiner) {
      // currently only update for pathSeg
      auto parent = node->getParent();
      if (parent->getLayerNum() != node->getLayerNum()) {
        auto parentLoc = parent->getLoc();
        auto nodeLoc = node->getLoc();
        FlexMazeIdx bi, ei;
        Point bp, ep;
        frLayerNum lNum = parent->getLayerNum();
        if (parentLoc < nodeLoc) {
          bp = parentLoc;
          ep = nodeLoc;
        } else {
          bp = nodeLoc;
          ep = parentLoc;
        }
        gridGraph_.getMazeIdx(bp, lNum, bi);
        gridGraph_.getMazeIdx(ep, lNum, ei);

        if (bi.x() == ei.x()) {
          // vertical pathSeg
          for (auto yIdx = bi.y(); yIdx < ei.y(); yIdx++) {
            if (isAdd) {
              gridGraph_.addRawDemand(bi.x(), yIdx, bi.z(), frDirEnum::N);
              gridGraph_.addRawDemand(bi.x(), yIdx + 1, bi.z(), frDirEnum::N);
            } else {
              gridGraph_.subRawDemand(bi.x(), yIdx, bi.z(), frDirEnum::N);
              gridGraph_.subRawDemand(bi.x(), yIdx + 1, bi.z(), frDirEnum::N);
            }
          }
        } else if (bi.y() == ei.y()) {
          // horizontal pathSeg
          for (auto xIdx = bi.x(); xIdx < ei.x(); xIdx++) {
            if (isAdd) {
              gridGraph_.addRawDemand(xIdx, bi.y(), bi.z(), frDirEnum::E);
              gridGraph_.addRawDemand(xIdx + 1, bi.y(), bi.z(), frDirEnum::E);
            } else {
              gridGraph_.subRawDemand(xIdx, bi.y(), bi.z(), frDirEnum::E);
              gridGraph_.subRawDemand(xIdx + 1, bi.y(), bi.z(), frDirEnum::E);
            }
          }
        } else {
          std::cout << "Error: non-colinear pathSeg in updateCMap_net"
                    << std::endl;
        }
      }
    }
  }
}

// initialize pinGCellNodes as well as removing overlapping pinGCellNodes
void FlexGRWorker::initNet_initPinGCellNodes(grNet* net)
{
  std::vector<std::pair<grNode*, grNode*>> pinGCellNodePairs;
  std::map<grNode*, std::vector<grNode*>, frBlockObjectComp> gcell2PinNodes;
  std::vector<grNode*> pinGCellNodes;
  std::map<FlexMazeIdx, grNode*> midx2PinGCellNode;
  grNode* rootGCellNode = nullptr;

  std::deque<grNode*> nodeQ;
  nodeQ.push_back(net->getRoot());
  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop_front();
    // push children to queue
    for (auto child : node->getChildren()) {
      nodeQ.push_back(child);
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

    if (midx2PinGCellNode.find(gcellNodeMIdx) == midx2PinGCellNode.end()) {
      midx2PinGCellNode[gcellNodeMIdx] = gcellNode;
    } else {
      if (gcellNode != midx2PinGCellNode[gcellNodeMIdx]) {
        // disjoint pinGCellNode overlap
        std::cout << "Warning: overlapping disjoint pinGCellNodes detected of "
                  << net->getFrNet()->getName() << "\n";
        // ripup to make sure congestion map is up-to-date
        net->setRipup(true);
        if (midx2PinGCellNode[gcellNodeMIdx] == rootGCellNode) {
          // loop back to rootGCellNode, need to directly connect child node to
          // root
          std::cout << "  between rootGCellNode and non-rootGCellNode\n";
          node->getParent()->removeChild(node);
          if (node->getConnFig()) {
            auto pathSeg = static_cast<grPathSeg*>(node->getConnFig());
            pathSeg->setParent(rootGCellNode);
          }
          node->setParent(rootGCellNode);
          rootGCellNode->addChild(node);
          gcellNode = rootGCellNode;
        } else {
          // merge the gcellNode to existing leaf node (directly connect all
          // children of the gcellNode to existing leaf) do not care about their
          // parents because it will be rerouted anyway
          std::cout << "  between non-rootGCellNode and non-rootGCellNode\n";
          node->getParent()->removeChild(node);
          if (node->getConnFig()) {
            auto pathSeg = static_cast<grPathSeg*>(node->getConnFig());
            pathSeg->setParent(midx2PinGCellNode[gcellNodeMIdx]);
          }
          node->setParent(midx2PinGCellNode[gcellNodeMIdx]);
          midx2PinGCellNode[gcellNodeMIdx]->addChild(node);
          gcellNode = midx2PinGCellNode[gcellNodeMIdx];
        }
      }
    }

    if (gcellNode) {
      pinGCellNodePairs.emplace_back(node, gcellNode);
      if (gcell2PinNodes.find(gcellNode) == gcell2PinNodes.end()) {
        pinGCellNodes.push_back(gcellNode);
      }
      gcell2PinNodes[gcellNode].push_back(node);
    }
  }

  net->setPinGCellNodePairs(pinGCellNodePairs);
  net->setGCell2PinNodes(gcell2PinNodes);
  net->setPinGCellNodes(pinGCellNodes);
  if (pinGCellNodes.size() == 1) {
    net->setTrivial(true);
  } else if (pinGCellNodes.size() > 1) {
    net->setTrivial(false);
  } else {
    std::cout << "Error: pinGCellNodes should contain at least one element\n";
  }
}

// generate route and ext objs based on subnet tree node information
void FlexGRWorker::initNet_initObjs(grNet* net)
{
  std::deque<grNode*> nodeQ;
  nodeQ.push_back(net->getRoot());
  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop_front();
    // push children to queue
    for (auto child : node->getChildren()) {
      nodeQ.push_back(child);
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

    if (node->getLayerNum() == node->getParent()->getLayerNum()) {
      // pathSeg
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
      uPathSeg->setLayerNum(node->getLayerNum());

      std::unique_ptr<grConnFig> uGRConnFig(std::move(uPathSeg));
      if (isExt) {
        net->addExtConnFig(uGRConnFig);
      } else {
        net->addRouteConnFig(uGRConnFig);
      }
    } else {
      // via
      auto parent = node->getParent();
      frLayerNum beginLayerNum, endLayerNum;
      Point loc = node->getLoc();
      beginLayerNum = node->getLayerNum();
      endLayerNum = parent->getLayerNum();

      auto uVia = std::make_unique<grVia>();
      uVia->setChild(node);
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
  workerRegionQuery.init(false /*not include ext*/);
}

void FlexGRWorker::initNets_printNets()
{
  std::cout << std::endl << "printing grNets\n";
  for (auto& net : nets_) {
    initNets_printNet(net.get());
  }
}

void FlexGRWorker::initNets_printNet(grNet* net)
{
  auto root = net->getRoot();
  std::deque<grNode*> nodeQ;
  nodeQ.push_back(root);

  std::cout << "start traversing subnet of " << net->getFrNet()->getName()
            << std::endl;

  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop_front();

    std::cout << "  ";
    if (node->getType() == frNodeTypeEnum::frcSteiner) {
      std::cout << "steiner node ";
    } else if (node->getType() == frNodeTypeEnum::frcPin) {
      std::cout << "pin node ";
    } else if (node->getType() == frNodeTypeEnum::frcBoundaryPin) {
      std::cout << "boundary pin node ";
    }
    std::cout << "at ";
    Point loc = node->getLoc();
    frLayerNum lNum = node->getLayerNum();
    std::cout << "(" << loc.x() << ", " << loc.y() << ") on layerNum " << lNum
              << std::endl;

    for (auto child : node->getChildren()) {
      nodeQ.push_back(child);
    }
  }
}

void FlexGRWorker::initNets_printFNets(
    std::map<frNet*, std::vector<frNode*>, frBlockObjectComp>& netRoots)
{
  std::cout << std::endl << "printing frNets\n";
  for (auto& [net, roots] : netRoots) {
    for (auto root : roots) {
      initNets_printFNet(root);
    }
  }
}

void FlexGRWorker::initNets_printFNet(frNode* root)
{
  std::deque<frNode*> nodeQ;
  nodeQ.push_back(root);

  std::cout << "start traversing subnet of " << root->getNet()->getName()
            << std::endl;

  bool isRoot = true;

  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop_front();

    std::cout << "  ";
    if (node->getType() == frNodeTypeEnum::frcSteiner) {
      std::cout << "steiner node ";
    } else if (node->getType() == frNodeTypeEnum::frcPin) {
      std::cout << "pin node ";
    } else if (node->getType() == frNodeTypeEnum::frcBoundaryPin) {
      std::cout << "boundary pin node ";
    }
    std::cout << "at ";
    Point loc = node->getLoc();
    frLayerNum lNum = node->getLayerNum();
    std::cout << "(" << loc.x() << ", " << loc.y() << ") on layerNum " << lNum
              << std::endl;

    if (isRoot || node->getType() == frNodeTypeEnum::frcSteiner) {
      for (auto child : node->getChildren()) {
        nodeQ.push_back(child);
      }
    }

    isRoot = false;
  }
}

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
  Point gcellIdxLL = getRouteGCellIdxLL();
  Point gcellIdxUR = getRouteGCellIdxUR();
  int idxLLX = gcellIdxLL.x();
  int idxLLY = gcellIdxLL.y();
  int idxURX = gcellIdxUR.x();
  int idxURY = gcellIdxUR.y();

  for (int zIdx = 0; zIdx < (int) cmap->getZMap().size(); zIdx++) {
    for (int xIdx = 0; xIdx <= (idxURX - idxLLX); xIdx++) {
      int cmapXIdx = xIdx + idxLLX;
      for (int yIdx = 0; yIdx <= (idxURY - idxLLY); yIdx++) {
        int cmapYIdx = yIdx + idxLLY;
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
  Point gcellIdxLL = getRouteGCellIdxLL();
  Point gcellIdxUR = getRouteGCellIdxUR();
  int idxLLX = gcellIdxLL.x();
  int idxLLY = gcellIdxLL.y();
  int idxURX = gcellIdxUR.x();
  int idxURY = gcellIdxUR.y();

  for (int zIdx = 0; zIdx < (int) cmap->getZMap().size(); zIdx++) {
    for (int xIdx = 0; xIdx <= (idxURX - idxLLX); xIdx++) {
      int cmapXIdx = xIdx + idxLLX;
      for (int yIdx = 0; yIdx <= (idxURY - idxLLY); yIdx++) {
        int cmapYIdx = yIdx + idxLLY;
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


}  // namespace drt
