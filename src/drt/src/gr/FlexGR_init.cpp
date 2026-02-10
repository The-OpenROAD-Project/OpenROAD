// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "db/grObj/grBlockObject.h"
#include "db/grObj/grFig.h"
#include "db/grObj/grNet.h"
#include "db/grObj/grNode.h"
#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frGCellPattern.h"
#include "db/obj/frVia.h"
#include "db/tech/frViaDef.h"
#include "dr/FlexMazeTypes.h"
#include "frBaseTypes.h"
#include "gr/FlexGR.h"
#include "gr/FlexGRCMap.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

using odb::dbTechLayerDir;
using odb::dbTechLayerType;

namespace drt {

void FlexGR::init()
{
  initGCell();
  initLayerPitch();
  initCMap();
}

void FlexGR::initLayerPitch()
{
  int numRoutingLayer = 0;
  for (unsigned lNum = 0; lNum < design_->getTech()->getLayers().size();
       lNum++) {
    if (design_->getTech()->getLayer(lNum)->getType()
        != dbTechLayerType::ROUTING) {
      continue;
    }
    numRoutingLayer++;
  }

  // init pitches
  trackPitches_.resize(numRoutingLayer, -1);
  line2ViaPitches_.resize(numRoutingLayer, -1);
  layerPitches_.resize(numRoutingLayer, -1);
  zHeights_.resize(numRoutingLayer, 0);

  // compute pitches
  for (int lNum = 0; lNum < (int) design_->getTech()->getLayers().size();
       lNum++) {
    if (design_->getTech()->getLayer(lNum)->getType()
        != dbTechLayerType::ROUTING) {
      continue;
    }
    // zIdx  always equal to lNum / 2 - 1
    int zIdx = lNum / 2 - 1;
    auto layer = design_->getTech()->getLayer(lNum);
    bool isLayerHorz = (layer->getDir() == dbTechLayerDir::HORIZONTAL);
    // get track pitch
    for (auto& tp : design_->getTopBlock()->getTrackPatterns(lNum)) {
      if ((isLayerHorz && !tp->isHorizontal())
          || (!isLayerHorz && tp->isHorizontal())) {
        if (trackPitches_[zIdx] == -1
            || (int) tp->getTrackSpacing() < trackPitches_[zIdx]) {
          trackPitches_[zIdx] = tp->getTrackSpacing();
        }
      }
    }

    // calculate line-2-via pitch
    const frViaDef* downVia = nullptr;
    const frViaDef* upVia = nullptr;
    if (getDesign()->getTech()->getBottomLayerNum() <= lNum - 1) {
      downVia = getDesign()->getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (getDesign()->getTech()->getTopLayerNum() >= lNum + 1) {
      upVia = getDesign()->getTech()->getLayer(lNum + 1)->getDefaultViaDef();
    }

    frCoord defaultWidth = layer->getWidth();
    frCoord line2ViaPitchDown = -1, line2ViaPitchUp = -1;
    frCoord minLine2ViaPitch = -1;

    if (downVia) {
      frVia via(downVia);
      odb::Rect viaBox = via.getLayer2BBox();
      frCoord enclosureWidth = viaBox.minDXDY();
      frCoord prl = isLayerHorz ? (viaBox.xMax() - viaBox.xMin())
                                : (viaBox.yMax() - viaBox.yMin());
      // calculate minNonOvlpDist
      frCoord minNonOvlpDist = defaultWidth / 2;
      if (isLayerHorz) {
        minNonOvlpDist += (viaBox.yMax() - viaBox.yMin()) / 2;
      } else {
        minNonOvlpDist += (viaBox.xMax() - viaBox.xMin()) / 2;
      }
      // calculate minSpc required val
      frCoord minReqDist = INT_MIN;

      auto con = layer->getMinSpacing();
      if (con) {
        if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
          minReqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
        } else if (con->typeId()
                   == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
          minReqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(
              std::max(enclosureWidth, defaultWidth), prl);
        } else if (con->typeId()
                   == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
          minReqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(
              enclosureWidth, defaultWidth, prl);
        }
      }
      if (minReqDist != INT_MIN) {
        minReqDist += minNonOvlpDist;
        line2ViaPitchDown = minReqDist;
        if (line2ViaPitches_[zIdx] == -1
            || minReqDist > line2ViaPitches_[zIdx]) {
          line2ViaPitches_[zIdx] = minReqDist;
        }
        if (minLine2ViaPitch == -1 || minReqDist < minLine2ViaPitch) {
          minLine2ViaPitch = minReqDist;
        }
      }
    }

    if (upVia) {
      frVia via(upVia);
      odb::Rect viaBox = via.getLayer1BBox();
      frCoord enclosureWidth = viaBox.minDXDY();
      frCoord prl = isLayerHorz ? (viaBox.xMax() - viaBox.xMin())
                                : (viaBox.yMax() - viaBox.yMin());
      // calculate minNonOvlpDist
      frCoord minNonOvlpDist = defaultWidth / 2;
      if (isLayerHorz) {
        minNonOvlpDist += (viaBox.yMax() - viaBox.yMin()) / 2;
      } else {
        minNonOvlpDist += (viaBox.xMax() - viaBox.xMin()) / 2;
      }
      // calculate minSpc required val
      frCoord minReqDist = INT_MIN;

      auto con = layer->getMinSpacing();
      if (con) {
        if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
          minReqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
        } else if (con->typeId()
                   == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
          minReqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(
              std::max(enclosureWidth, defaultWidth), prl);
        } else if (con->typeId()
                   == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
          minReqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(
              enclosureWidth, defaultWidth, prl);
        }
      }
      if (minReqDist != INT_MIN) {
        minReqDist += minNonOvlpDist;
        line2ViaPitchUp = minReqDist;
        if (line2ViaPitches_[zIdx] == -1
            || minReqDist > line2ViaPitches_[zIdx]) {
          line2ViaPitches_[zIdx] = minReqDist;
        }
        if (minLine2ViaPitch == -1 || minReqDist < minLine2ViaPitch) {
          minLine2ViaPitch = minReqDist;
        }
      }
    }

    if (minLine2ViaPitch > trackPitches_[zIdx]) {
      layerPitches_[zIdx] = std::max(line2ViaPitchDown, line2ViaPitchUp);
    } else {
      layerPitches_[zIdx] = trackPitches_[zIdx];
    }

    // output
    std::cout << layer->getName();
    if (isLayerHorz) {
      std::cout << " H ";
    } else {
      std::cout << " V ";
    }
    std::cout << "Track-Pitch = " << std::fixed << std::setprecision(5)
              << trackPitches_[zIdx]
                     / (double) (design_->getTopBlock()->getDBUPerUU())
              << "  line-2-Via Pitch = " << std::fixed << std::setprecision(5)
              << minLine2ViaPitch
                     / (double) (design_->getTopBlock()->getDBUPerUU())
              << '\n';
    if (trackPitches_[zIdx] < minLine2ViaPitch) {
      std::cout << "Warning: Track pitch is too small compared with line-2-via "
                   "pitch\n";
    }

    if (zIdx == 0) {
      zHeights_[zIdx] = layerPitches_[zIdx];
    } else {
      zHeights_[zIdx] = zHeights_[zIdx - 1] + layerPitches_[zIdx];
    }
  }
}

void FlexGR::initGCell()
{
  auto& gcellPatterns = design_->getTopBlock()->getGCellPatterns();
  if (gcellPatterns.empty()) {
    auto layer = design_->getTech()->getLayer(2);
    auto pitch = layer->getPitch();
    std::cout << '\n'
              << "Generating GCell with size = 15 tracks, using layer "
              << layer->getName() << " pitch  = "
              << pitch / (double) (design_->getTopBlock()->getDBUPerUU())
              << "\n";
    odb::Rect dieBox = design_->getTopBlock()->getDieBox();

    frGCellPattern xgp, ygp;
    // set xgp
    xgp.setHorizontal(false);
    frCoord startCoordX
        = dieBox.xMin() - design_->getTech()->getManufacturingGrid();
    xgp.setStartCoord(startCoordX);
    frCoord GCELLGRIDX = pitch * 15;
    xgp.setSpacing(GCELLGRIDX);
    if (GCELLGRIDX != 0) {
      xgp.setCount((dieBox.xMax() - startCoordX) / GCELLGRIDX);
    }
    // set ygp
    ygp.setHorizontal(true);
    frCoord startCoordY
        = dieBox.yMin() - design_->getTech()->getManufacturingGrid();
    ygp.setStartCoord(startCoordY);
    frCoord GCELLGRIDY = pitch * 15;
    ygp.setSpacing(GCELLGRIDY);
    if (GCELLGRIDY == 0) {
      std::cout << "Pitch is zero for " << layer->getName() << "\n";
      exit(1);
    }
    ygp.setCount((dieBox.yMax() - startCoordY) / GCELLGRIDY);

    design_->getTopBlock()->setGCellPatterns({xgp, ygp});
  }
}

void FlexGR::initCMap()
{
  std::cout << '\n' << "initializing congestion map...\n";
  auto cmap = std::make_unique<FlexGRCMap>(design_, router_cfg_);
  cmap->setLayerTrackPitches(trackPitches_);
  cmap->setLayerLine2ViaPitches(line2ViaPitches_);
  cmap->setLayerPitches(layerPitches_);
  cmap->init();
  auto cmap2D = std::make_unique<FlexGRCMap>(design_, router_cfg_);
  cmap2D->initFrom3D(cmap.get());
  // cmap->print2D(true);
  // cmap->print();
  setCMap(cmap);
  setCMap2D(cmap2D);
}

// FlexGRWorker related

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
        std::cout << "Error: initNetObjs hasNet() empty\n";
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
  odb::Point breakPt1, breakPt2;
  initBoundary_splitPathSeg_getBreakPts(bp, ep, breakPt1, breakPt2);
  auto parent = pathSeg->getParent();
  auto child = pathSeg->getChild();

  if (breakPt1 == breakPt2) {
    // break on one side
    initBoundary_splitPathSeg_split(child, parent, breakPt1);
  } else {
    // break on both sides

    odb::Point childLoc = child->getLoc();
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
void FlexGRWorker::initBoundary_splitPathSeg_getBreakPts(const odb::Point& bp,
                                                         const odb::Point& ep,
                                                         odb::Point& breakPt1,
                                                         odb::Point& breakPt2)
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
                                                      const odb::Point& breakPt)
{
  // frNet
  auto pathSeg = static_cast<grPathSeg*>(child->getConnFig());
  auto net = pathSeg->getNet();
  auto lNum = pathSeg->getLayerNum();
  auto [bp, ep] = pathSeg->getPoints();
  odb::Point childLoc = child->getLoc();
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

void FlexGRWorker::initNets()
{
  frOrderedIdSet<frNet*> nets;
  frOrderedIdMap<frNet*, std::vector<frNode*>> netRoots;

  initNets_roots(nets, netRoots);
  initNets_searchRepair(nets, netRoots);
  initNets_regionQuery();
}

// get all roots of subnets
void FlexGRWorker::initNets_roots(
    frOrderedIdSet<frNet*>& nets,
    frOrderedIdMap<frNet*, std::vector<frNode*>>& netRoots)
{
  std::vector<grBlockObject*> result;
  getRegionQuery()->queryGRObj(routeBox_, result);

  for (auto rptr : result) {
    if (rptr->typeId() == grcPathSeg) {
      auto cptr = static_cast<grPathSeg*>(rptr);
      if (cptr->hasNet()) {
        initNetObjs_roots_pathSeg(cptr, nets, netRoots);
      } else {
        std::cout << "Error: initNetObjs hasNet() empty\n";
      }
    } else if (rptr->typeId() == grcVia) {
      auto cptr = static_cast<grVia*>(rptr);
      if (cptr->hasNet()) {
        initNetObjs_roots_via(cptr, nets, netRoots);
      } else {
        std::cout << "Error: initNetObjs hasNet() empty\n";
      }
    } else {
      std::cout << rptr->typeId() << '\n';
      std::cout << "Error: initNetObjs unsupported type\n";
    }
  }
}

// if parent of a pathSeg is at the boundary of extBox, the parent is a subnet
// root (i.e., outgoing edge)
void FlexGRWorker::initNetObjs_roots_pathSeg(
    grPathSeg* pathSeg,
    frOrderedIdSet<frNet*>& nets,
    frOrderedIdMap<frNet*, std::vector<frNode*>>& netRoots)
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
    frOrderedIdSet<frNet*>& nets,
    frOrderedIdMap<frNet*, std::vector<frNode*>>& netRoots)
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
    frOrderedIdSet<frNet*>& nets,
    frOrderedIdMap<frNet*, std::vector<frNode*>>& netRoots)
{
  for (auto net : nets) {
    initNet(net, netRoots[net]);
  }
}

void FlexGRWorker::initNet(frNet* net, const std::vector<frNode*>& netRoots)
{
  frOrderedIdSet<frNode*> uniqueRoots;
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
  // std::map<std::pair<odb::Point, frlayerNum>, grNode*> loc2GCellNode;
  std::vector<std::pair<frNode*, grNode*>> pinNodePairs;
  frOrderedIdMap<grNode*, frNode*> gr2FrPinNode;
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
    odb::Point parentLoc;
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
      odb::Point gcellLoc = getBoundaryPinGCellNodeLoc(parentLoc);
      // if parent is boundary pin, child must be on the same layer
      if (gcellLoc != childLoc) {
        needGenParentGCellNode = true;
      }
    }
    // leaf at boundary
    if (!needGenParentGCellNode && !isRoot
        && child->getType() == frNodeTypeEnum::frcBoundaryPin) {
      odb::Point gcellLoc = getBoundaryPinGCellNodeLoc(childLoc);
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
      odb::Point gcellNodeLoc = getBoundaryPinGCellNodeLoc(parentLoc);

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
      odb::Point gcellNodeLoc = getBoundaryPinGCellNodeLoc(childLoc);

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

odb::Point FlexGRWorker::getBoundaryPinGCellNodeLoc(
    const odb::Point& boundaryPinLoc)
{
  odb::Point gcellNodeLoc;
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
    odb::Point rootLoc = rootNode->getLoc();
    if (extBox_.intersects(rootLoc) && routeBox_.intersects(rootLoc)) {
      std::cout << "Error: root should be on an outgoing edge\n";
    }
  } else if (rootNode->getType() == frNodeTypeEnum::frcPin) {
    odb::Point rootLoc = rootNode->getLoc();
    odb::Point globalRootLoc
        = rootNode->getNet()->getFrNet()->getRoot()->getLoc();
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
        odb::Point bp, ep;
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
          std::cout << "Error: non-colinear pathSeg in updateCMap_net\n";
        }
      }
    }
  }
}

// initialize pinGCellNodes as well as removing overlapping pinGCellNodes
void FlexGRWorker::initNet_initPinGCellNodes(grNet* net)
{
  std::vector<std::pair<grNode*, grNode*>> pinGCellNodePairs;
  frOrderedIdMap<grNode*, std::vector<grNode*>> gcell2PinNodes;
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

    odb::Point gcellNodeLoc = gcellNode->getLoc();
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
      odb::Point childLoc = node->getLoc();
      odb::Point parentLoc = parent->getLoc();
      odb::Point bp, ep;
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
      odb::Point loc = node->getLoc();
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
  std::cout << '\n' << "printing grNets\n";
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
            << '\n';

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
    odb::Point loc = node->getLoc();
    frLayerNum lNum = node->getLayerNum();
    std::cout << "(" << loc.x() << ", " << loc.y() << ") on layerNum " << lNum
              << '\n';

    for (auto child : node->getChildren()) {
      nodeQ.push_back(child);
    }
  }
}

void FlexGRWorker::initNets_printFNets(
    frOrderedIdMap<frNet*, std::vector<frNode*>>& netRoots)
{
  std::cout << '\n' << "printing frNets\n";
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
            << '\n';

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
    odb::Point loc = node->getLoc();
    frLayerNum lNum = node->getLayerNum();
    std::cout << "(" << loc.x() << ", " << loc.y() << ") on layerNum " << lNum
              << '\n';

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
  odb::Point gcellIdxLL = getRouteGCellIdxLL();
  odb::Point gcellIdxUR = getRouteGCellIdxUR();
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

}  // namespace drt
