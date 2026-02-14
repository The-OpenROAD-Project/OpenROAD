// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <deque>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "db/grObj/grFig.h"
#include "db/grObj/grNet.h"
#include "db/grObj/grNode.h"
#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include "db/obj/frBlockObject.h"
#include "dr/FlexMazeTypes.h"
#include "frBaseTypes.h"
#include "gr/FlexGR.h"
#include "odb/geom.h"

namespace drt {

void FlexGRWorker::route()
{
  std::vector<grNet*> rerouteNets;
  for (int i = 0; i < mazeEndIter_; i++) {
    if (ripupMode_ == RipUpMode::DRC) {
      route_addHistCost();
    }
    route_mazeIterInit();
    route_getRerouteNets(rerouteNets);
    for (auto net : rerouteNets) {
      if (ripupMode_ == RipUpMode::DRC
          || (ripupMode_ == RipUpMode::ALL && mazeNetHasCong(net))) {
        mazeNetInit(net);
        routeNet(net);
      }
    }
    if (ripupMode_ == RipUpMode::ALL) {
      route_decayHistCost();
    }
  }
}

void FlexGRWorker::route_addHistCost()
{
  frMIdx xDim = 0;
  frMIdx yDim = 0;
  frMIdx zDim = 0;

  gridGraph_.getDim(xDim, yDim, zDim);

  for (int zIdx = 0; zIdx < zDim; zIdx++) {
    // frLayerNum lNum = (zIdx + 1) * 2;
    for (int xIdx = 0; xIdx < xDim; xIdx++) {
      for (int yIdx = 0; yIdx < yDim; yIdx++) {
        if (gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::E)
                > congThresh_
                      * gridGraph_.getRawSupply(xIdx, yIdx, zIdx, frDirEnum::E)
            || gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::N)
                   > congThresh_
                         * gridGraph_.getRawSupply(
                             xIdx, yIdx, zIdx, frDirEnum::N)) {
          // has congestion, need to add history cost
          int overflowH = std::max(
              0,
              int(gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::E)
                  - congThresh_
                        * gridGraph_.getRawSupply(
                            xIdx, yIdx, zIdx, frDirEnum::E)));
          int overflowV = std::max(
              0,
              int(gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::N)
                  - congThresh_
                        * gridGraph_.getRawSupply(
                            xIdx, yIdx, zIdx, frDirEnum::N)));
          gridGraph_.addHistoryCost(
              xIdx, yIdx, zIdx, (overflowH + overflowV) / 2 + 1);
        }
      }
    }
  }
}

void FlexGRWorker::route_mazeIterInit()
{
  // reset ripup
  for (auto& net : nets_) {
    net->setRipup(false);
  }
  if (ripupMode_ == RipUpMode::DRC) {
    // set ripup based on congestion
    auto& workerRegionQuery = getWorkerRegionQuery();

    frMIdx xDim = 0;
    frMIdx yDim = 0;
    frMIdx zDim = 0;

    gridGraph_.getDim(xDim, yDim, zDim);

    std::vector<grConnFig*> result;

    for (int zIdx = 0; zIdx < zDim; zIdx++) {
      frLayerNum lNum = (zIdx + 1) * 2;
      for (int xIdx = 0; xIdx < xDim; xIdx++) {
        for (int yIdx = 0; yIdx < yDim; yIdx++) {
          if (gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::E)
                  > congThresh_
                        * gridGraph_.getRawSupply(
                            xIdx, yIdx, zIdx, frDirEnum::E)
              || gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::N)
                     > congThresh_
                           * gridGraph_.getRawSupply(
                               xIdx, yIdx, zIdx, frDirEnum::N)) {
            // has congestion, need to region query to find what nets are using
            // this gcell
            odb::Point gcellCenter;
            gridGraph_.getPoint(xIdx, yIdx, gcellCenter);
            odb::Rect gcellCenterBox(gcellCenter, gcellCenter);
            result.clear();
            workerRegionQuery.query(gcellCenterBox, lNum, result);
            for (auto rptr : result) {
              if (rptr->typeId() == grcPathSeg) {
                auto cptr = static_cast<grPathSeg*>(rptr);
                if (cptr->hasGrNet()) {
                  cptr->getGrNet()->setRipup(true);
                } else {
                  std::cout << "Error: route_mazeIterInit hasGrNet() empty"
                            << '\n';
                }
              }
            }
          }
        }
      }
    }
  }
}

void FlexGRWorker::route_getRerouteNets(std::vector<grNet*>& rerouteNets)
{
  rerouteNets.clear();
  if (ripupMode_ == RipUpMode::DRC) {
    for (auto& net : nets_) {
      if (net->isRipup() && !net->isTrivial()) {
        rerouteNets.push_back(net.get());
        net->setModified(true);
        net->getFrNet()->setModified(true);
      } else {
      }
    }
  } else if (ripupMode_ == RipUpMode::ALL) {
    for (auto& net : nets_) {
      if (!net->isTrivial()) {
        rerouteNets.push_back(net.get());
      }
    }
  }
}

bool FlexGRWorker::mazeNetHasCong(grNet* net)
{
  for (auto& uptr : net->getRouteConnFigs()) {
    if (uptr->typeId() == grcPathSeg) {
      auto cptr = static_cast<grPathSeg*>(uptr.get());
      auto [bp, ep] = cptr->getPoints();
      frLayerNum lNum = cptr->getLayerNum();
      FlexMazeIdx bi, ei;
      gridGraph_.getMazeIdx(bp, lNum, bi);
      gridGraph_.getMazeIdx(ep, lNum, ei);
      if (bi.x() == ei.x()) {
        // vert
        for (auto yIdx = bi.y(); yIdx <= ei.y(); yIdx++) {
          if (gridGraph_.getRawDemand(bi.x(), yIdx, bi.z(), frDirEnum::E)
                  > getCongThresh()
                        * gridGraph_.getRawSupply(
                            bi.x(), yIdx, bi.z(), frDirEnum::E)
              || gridGraph_.getRawDemand(bi.x(), yIdx, bi.z(), frDirEnum::N)
                     > getCongThresh()
                           * gridGraph_.getRawSupply(
                               bi.x(), yIdx, bi.z(), frDirEnum::N)) {
            net->setModified(true);
            net->getFrNet()->setModified(true);
            net->setRipup(true);

            return true;
          }
        }
      } else {
        // horz
        for (auto xIdx = bi.x(); xIdx <= ei.x(); xIdx++) {
          if (gridGraph_.getRawDemand(xIdx, bi.y(), bi.z(), frDirEnum::E)
                  > getCongThresh()
                        * gridGraph_.getRawSupply(
                            xIdx, bi.y(), bi.z(), frDirEnum::E)
              || gridGraph_.getRawDemand(xIdx, bi.y(), bi.z(), frDirEnum::N)
                     > getCongThresh()
                           * gridGraph_.getRawSupply(
                               xIdx, bi.y(), bi.z(), frDirEnum::N)) {
            net->setModified(true);
            net->getFrNet()->setModified(true);
            net->setRipup(true);

            return true;
          }
        }
      }
    }
  }
  return false;
}

// reset gridGraph, remove routeObj and remove nodes
void FlexGRWorker::mazeNetInit(grNet* net)
{
  gridGraph_.resetStatus();
  if (ripupMode_ == RipUpMode::DRC) {
    mazeNetInit_decayHistCost(net);
  } else if (ripupMode_ == RipUpMode::ALL) {
    mazeNetInit_addHistCost(net);
  }
  mazeNetInit_removeNetObjs(net);
  mazeNetInit_removeNetNodes(net);
}

void FlexGRWorker::mazeNetInit_addHistCost(grNet* net)
{
  // add history cost for all gcells that this net previously visited
  std::set<FlexMazeIdx> pts;
  for (auto& uptr : net->getRouteConnFigs()) {
    if (uptr->typeId() == grcPathSeg) {
      auto cptr = static_cast<grPathSeg*>(uptr.get());
      auto [bp, ep] = cptr->getPoints();
      frLayerNum lNum = cptr->getLayerNum();
      FlexMazeIdx bi, ei;
      gridGraph_.getMazeIdx(bp, lNum, bi);
      gridGraph_.getMazeIdx(ep, lNum, ei);
      if (bi.x() == ei.x()) {
        // vert
        for (auto yIdx = bi.y(); yIdx <= ei.y(); yIdx++) {
          pts.insert(FlexMazeIdx(bi.x(), yIdx, bi.z()));
        }
      } else {
        // horz
        for (auto xIdx = bi.x(); xIdx <= ei.x(); xIdx++) {
          pts.insert(FlexMazeIdx(xIdx, bi.y(), bi.z()));
        }
      }
    }
  }
  for (const auto& pt : pts) {
    if (gridGraph_.getRawDemand(pt.x(), pt.y(), pt.z(), frDirEnum::E)
            > getCongThresh()
                  * gridGraph_.getRawSupply(
                      pt.x(), pt.y(), pt.z(), frDirEnum::E)
        || gridGraph_.getRawDemand(pt.x(), pt.y(), pt.z(), frDirEnum::N)
               > getCongThresh()
                     * gridGraph_.getRawSupply(
                         pt.x(), pt.y(), pt.z(), frDirEnum::N)) {
      gridGraph_.addHistoryCost(pt.x(), pt.y(), pt.z(), 1);
    }
  }
}

void FlexGRWorker::mazeNetInit_decayHistCost(grNet* net)
{
  // decay history cost for all gcells that this net previously visited
  std::set<FlexMazeIdx> pts;
  for (auto& uptr : net->getRouteConnFigs()) {
    if (uptr->typeId() == grcPathSeg) {
      auto cptr = static_cast<grPathSeg*>(uptr.get());
      auto [bp, ep] = cptr->getPoints();
      frLayerNum lNum = cptr->getLayerNum();
      FlexMazeIdx bi, ei;
      gridGraph_.getMazeIdx(bp, lNum, bi);
      gridGraph_.getMazeIdx(ep, lNum, ei);
      if (bi.x() == ei.x()) {
        // vert
        for (auto yIdx = bi.y(); yIdx <= ei.y(); yIdx++) {
          pts.insert(FlexMazeIdx(bi.x(), yIdx, bi.z()));
        }
      } else {
        // horz
        for (auto xIdx = bi.x(); xIdx <= ei.x(); xIdx++) {
          pts.insert(FlexMazeIdx(xIdx, bi.y(), bi.z()));
        }
      }
    }
  }
  for (const auto& pt : pts) {
    gridGraph_.decayHistoryCost(pt.x(), pt.y(), pt.z());
  }
}

// with vector, we can only erase all route objs
// if we need partial ripup, need to change routeConnfigs to list
void FlexGRWorker::mazeNetInit_removeNetObjs(grNet* net)
{
  auto& workerRegionQuery = getWorkerRegionQuery();
  // remove everything from region query
  for (auto& uptr : net->getRouteConnFigs()) {
    workerRegionQuery.remove(uptr.get());
  }
  // update congestion for the to-be-removed objs
  for (auto& uptr : net->getRouteConnFigs()) {
    if (uptr->typeId() == grcPathSeg) {
      auto cptr = static_cast<grPathSeg*>(uptr.get());
      modCong_pathSeg(cptr, /*isAdd*/ false);
    }
  }

  // remove the connFigs themselves
  net->clearRouteConnFigs();
}

void FlexGRWorker::modCong_pathSeg(grPathSeg* pathSeg, bool isAdd)
{
  FlexMazeIdx bi, ei;
  frLayerNum lNum = pathSeg->getLayerNum();
  auto [bp, ep] = pathSeg->getPoints();
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
    std::cout << "Error: non-colinear pathSeg in modCong_pathSeg\n";
  }
}

//
void FlexGRWorker::mazeNetInit_removeNetNodes(grNet* net)
{
  // remove non-terminal gcell nodes
  std::deque<grNode*> nodeQ;
  std::set<grNode*> terminalNodes;
  grNode* root = nullptr;

  for (auto& [pinNode, gcellNode] : net->getPinGCellNodePairs()) {
    if (root == nullptr) {
      root = gcellNode;
    }
    terminalNodes.insert(gcellNode);
  }

  if (root == nullptr) {
    std::cout << "Error: root is nullptr\n";
  }

  nodeQ.push_back(root);
  // bfs to remove non-terminal nodes
  // only steiner node can exist in queue
  bool isRoot = true;
  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    // std::cout << "  " << node << " (" << node->getLoc().x() << ", " <<
    // node->getLoc().y() << ", " << node->getLayerNum() << ")" << std::endl;
    nodeQ.pop_front();

    // push steiner child
    for (auto child : node->getChildren()) {
      if (child->getType() == frNodeTypeEnum::frcSteiner) {
        nodeQ.push_back(child);
      }
    }

    // update connection
    // for root terminal node, clear all steiner children;
    // for non-root terminal nodes, clear all steiner children and reset parent
    // and remove node
    if (terminalNodes.find(node) != terminalNodes.end()) {
      std::vector<grNode*> steinerChildNodes;
      for (auto child : node->getChildren()) {
        if (child->getType() == frNodeTypeEnum::frcSteiner) {
          steinerChildNodes.push_back(child);
        }
      }
      for (auto steinerChildNode : steinerChildNodes) {
        node->removeChild(steinerChildNode);
      }

      if (!isRoot) {
        node->setParent(nullptr);
      }
    } else {
      net->removeNode(node);
    }

    isRoot = false;
  }
}

bool FlexGRWorker::routeNet(grNet* net)
{
  if (net->isTrivial()) {
    std::cout << "Error: trivial net should not be routed\n";
    return true;
  }

  frOrderedIdSet<grNode*> unConnPinGCellNodes;
  std::map<FlexMazeIdx, grNode*> mazeIdx2unConnPinGCellNode;
  std::map<FlexMazeIdx, grNode*> mazeIdx2endPointNode;
  routeNet_prep(net,
                unConnPinGCellNodes,
                mazeIdx2unConnPinGCellNode,
                mazeIdx2endPointNode);

  FlexMazeIdx ccMazeIdx1, ccMazeIdx2;  // connComps ll, ur FlexMazeIdx
  odb::Point centerPt;
  std::vector<FlexMazeIdx> connComps;

  routeNet_setSrc(net,
                  unConnPinGCellNodes,
                  mazeIdx2unConnPinGCellNode,
                  connComps,
                  ccMazeIdx1,
                  ccMazeIdx2,
                  centerPt);

  std::vector<FlexMazeIdx> path;  // astar must return with more than one idx

  while (!unConnPinGCellNodes.empty()) {
    // reset prev dirs before routing to a new dst
    gridGraph_.resetPrevNodeDir();
    path.clear();
    auto nextPinGCellNode = routeNet_getNextDst(
        ccMazeIdx1, ccMazeIdx2, mazeIdx2unConnPinGCellNode);
    if (gridGraph_.search(connComps,
                          nextPinGCellNode,
                          path,
                          ccMazeIdx1,
                          ccMazeIdx2,
                          centerPt)) {
      auto leaf = routeNet_postAstarUpdate(
          path, connComps, unConnPinGCellNodes, mazeIdx2unConnPinGCellNode);
      routeNet_postAstarWritePath(net, path, leaf, mazeIdx2endPointNode);
    } else {
      return false;
    }
  }

  routeNet_postRouteAddCong(net);
  return true;
}

void FlexGRWorker::routeNet_prep(
    grNet* net,
    frOrderedIdSet<grNode*>& unConnPinGCellNodes,
    std::map<FlexMazeIdx, grNode*>& mazeIdx2unConnPinGCellNode,
    std::map<FlexMazeIdx, grNode*>& mazeIdx2endPointNode)
{
  for (auto pinGCellNode : net->getPinGCellNodes()) {
    auto loc = pinGCellNode->getLoc();
    auto lNum = pinGCellNode->getLayerNum();
    FlexMazeIdx mi;
    gridGraph_.getMazeIdx(loc, lNum, mi);
    gridGraph_.setDst(mi);

    unConnPinGCellNodes.insert(pinGCellNode);
    if (mazeIdx2unConnPinGCellNode.find(mi)
        != mazeIdx2unConnPinGCellNode.end()) {
      if (mazeIdx2unConnPinGCellNode[mi] == net->getPinGCellNodes()[0]) {
        std::cout
            << "Warning: unConnPinGCellNode overlaps with root pinGCellNode\n";
      } else {
        std::cout << "Warning: overlapping leaf pinGCellNode\n";
      }
    }
    mazeIdx2unConnPinGCellNode[mi] = pinGCellNode;
    mazeIdx2endPointNode[mi] = pinGCellNode;
  }
}

void FlexGRWorker::routeNet_printNet(grNet* net)
{
  auto root = net->getRoot();
  std::deque<grNode*> nodeQ;
  nodeQ.push_back(root);

  std::cout << "start traversing subnet of " << net->getFrNet()->getName()
            << '\n';

  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop_front();

    std::cout << "  parent ";
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
      std::cout << "    child ";
      if (child->getType() == frNodeTypeEnum::frcSteiner) {
        std::cout << "steiner node ";
      } else if (child->getType() == frNodeTypeEnum::frcPin) {
        std::cout << "pin node ";
      } else if (child->getType() == frNodeTypeEnum::frcBoundaryPin) {
        std::cout << "boundary pin node ";
      }
      std::cout << "at ";
      odb::Point loc = child->getLoc();
      frLayerNum lNum = child->getLayerNum();
      std::cout << "(" << loc.x() << ", " << loc.y() << ") on layerNum " << lNum
                << '\n';
    }
  }
}

// current set subnet root to be src with the absence of reroot
void FlexGRWorker::routeNet_setSrc(
    grNet* net,
    frOrderedIdSet<grNode*>& unConnPinGCellNodes,
    std::map<FlexMazeIdx, grNode*>& mazeIdx2unConnPinGCellNode,
    std::vector<FlexMazeIdx>& connComps,
    FlexMazeIdx& ccMazeIdx1,
    FlexMazeIdx& ccMazeIdx2,
    odb::Point& centerPt)
{
  frMIdx xDim, yDim, zDim;
  gridGraph_.getDim(xDim, yDim, zDim);
  ccMazeIdx1.set(xDim - 1, yDim - 1, zDim - 1);
  ccMazeIdx2.set(0, 0, 0);

  centerPt = {0, 0};
  int totPinCnt = unConnPinGCellNodes.size();
  for (auto pinGCellNode : unConnPinGCellNodes) {
    auto loc = pinGCellNode->getLoc();
    auto lNum = pinGCellNode->getLayerNum();
    FlexMazeIdx mi;
    gridGraph_.getMazeIdx(loc, lNum, mi);

    centerPt = {centerPt.x() + loc.x(), centerPt.y() + loc.y()};
  }
  centerPt = {centerPt.x() / totPinCnt, centerPt.y() / totPinCnt};

  // currently use root gcell
  auto rootPinGCellNode = net->getPinGCellNodes()[0];
  unConnPinGCellNodes.erase(rootPinGCellNode);

  auto loc = rootPinGCellNode->getLoc();
  auto lNum = rootPinGCellNode->getLayerNum();
  FlexMazeIdx mi;
  gridGraph_.getMazeIdx(loc, lNum, mi);

  mazeIdx2unConnPinGCellNode.erase(mi);
  gridGraph_.setSrc(mi);
  gridGraph_.resetDst(mi);

  connComps.push_back(mi);
  ccMazeIdx1.set(std::min(ccMazeIdx1.x(), mi.x()),
                 std::min(ccMazeIdx1.y(), mi.y()),
                 std::min(ccMazeIdx1.z(), mi.z()));
  ccMazeIdx2.set(std::max(ccMazeIdx2.x(), mi.x()),
                 std::max(ccMazeIdx2.y(), mi.y()),
                 std::max(ccMazeIdx2.z(), mi.z()));
}

grNode* FlexGRWorker::routeNet_getNextDst(
    FlexMazeIdx& ccMazeIdx1,
    FlexMazeIdx& ccMazeIdx2,
    std::map<FlexMazeIdx, grNode*>& mazeIdx2unConnPinGCellNode)
{
  odb::Point pt;
  odb::Point ll, ur;
  gridGraph_.getPoint(ccMazeIdx1.x(), ccMazeIdx1.y(), ll);
  gridGraph_.getPoint(ccMazeIdx2.x(), ccMazeIdx2.y(), ur);
  frCoord currDist = std::numeric_limits<frCoord>::max();
  grNode* nextDst = nullptr;

  if (mazeIdx2unConnPinGCellNode.empty()) {
    std::cout << "Error: no available nextDst\n" << std::flush;
  }

  if (!nextDst) {
    for (auto& [mazeIdx, node] : mazeIdx2unConnPinGCellNode) {
      if (node == nullptr) {
        std::cout << "Error: nullptr node in mazeIdx2unConnPinGCellNode\n"
                  << std::flush;
      }
      gridGraph_.getPoint(mazeIdx.x(), mazeIdx.y(), pt);
      frCoord dx = std::max({ll.x() - pt.x(), pt.x() - ur.x(), 0});
      frCoord dy = std::max({ll.y() - pt.y(), pt.y() - ur.y(), 0});
      frCoord dz = std::max({gridGraph_.getZHeight(ccMazeIdx1.z())
                                 - gridGraph_.getZHeight(mazeIdx.z()),
                             gridGraph_.getZHeight(mazeIdx.z())
                                 - gridGraph_.getZHeight(ccMazeIdx2.z()),
                             0});
      if (dx + dy + dz < currDist) {
        currDist = dx + dy + dz;
        nextDst = node;
      }
      if (currDist == 0) {
        break;
      }
    }
  }

  if (nextDst == nullptr) {
    std::cout << "Error: nextDst node is nullptr\n" << std::flush;
    std::cout << nextDst->getNet()->getFrNet()->getName() << '\n';
  }
  return nextDst;
}

grNode* FlexGRWorker::routeNet_postAstarUpdate(
    std::vector<FlexMazeIdx>& path,
    std::vector<FlexMazeIdx>& connComps,
    frOrderedIdSet<grNode*>& unConnPinGCellNodes,
    std::map<FlexMazeIdx, grNode*>& mazeIdx2unConnPinGCellNode)
{
  std::set<FlexMazeIdx> localConnComps;
  grNode* leaf = nullptr;
  if (!path.empty()) {
    auto mi = path[0];
    if (mazeIdx2unConnPinGCellNode.find(mi)
        == mazeIdx2unConnPinGCellNode.end()) {
      std::cout << "Error: destination pin gcellNode not found\n";
    }
    leaf = mazeIdx2unConnPinGCellNode[mi];
    mazeIdx2unConnPinGCellNode.erase(mi);
    unConnPinGCellNodes.erase(leaf);
    gridGraph_.resetDst(mi);
    gridGraph_.setSrc(mi);
    localConnComps.insert(mi);
  } else {
    std::cout << "Error: routeNet_postAstarUpdate path is empty\n";
  }

  for (int i = 0; i < (int) path.size() - 1; i++) {
    auto start = path[i];
    auto end = path[i + 1];
    auto startX = start.x();
    auto startY = start.y();
    auto startZ = start.z();
    auto endX = end.x();
    auto endY = end.y();
    auto endZ = end.z();
    // horizontal wire
    if (startX != endX && startY == endY && startZ == endZ) {
      for (auto currX = std::min(startX, endX); currX <= std::max(startX, endX);
           ++currX) {
        localConnComps.insert(FlexMazeIdx(currX, startY, startZ));
        gridGraph_.setSrc(currX, startY, startZ);
      }
      // vertical wire
    } else if (startX == endX && startY != endY && startZ == endZ) {
      for (auto currY = std::min(startY, endY); currY <= std::max(startY, endY);
           ++currY) {
        localConnComps.insert(FlexMazeIdx(startX, currY, startZ));
        gridGraph_.setSrc(startX, currY, startZ);
      }
      // via
    } else if (startX == endX && startY == endY && startZ != endZ) {
      for (auto currZ = std::min(startZ, endZ); currZ <= std::max(startZ, endZ);
           ++currZ) {
        localConnComps.insert(FlexMazeIdx(startX, startY, currZ));
        gridGraph_.setSrc(startX, startY, currZ);
      }
      // zero length
    } else if (startX == endX && startY == endY && startZ == endZ) {
      // root.addPinGrid(startX, startY, startZ);
      std::cout << "Warning: zero-length path in updateFlexPin\n";
    } else {
      std::cout << "Error: non-colinear path in updateFlexPin\n";
    }
  }
  for (auto& mi : localConnComps) {
    if (!(mi == *(path.cbegin()))) {
      connComps.push_back(mi);
    }
  }

  return leaf;
}

void FlexGRWorker::routeNet_postAstarWritePath(
    grNet* net,
    std::vector<FlexMazeIdx>& points,
    grNode* leaf,
    std::map<FlexMazeIdx, grNode*>& mazeIdx2endPointNode)
{
  if (points.empty()) {
  }

  auto& workerRegionQuery = getWorkerRegionQuery();
  grNode* child = nullptr;
  grNode* parent = nullptr;

  for (int i = 0; i < (int) points.size() - 1; i++) {
    FlexMazeIdx start, end;
    if (points[i + 1] < points[i]) {
      start = points[i + 1];
      end = points[i];
    } else {
      start = points[i];
      end = points[i + 1];
    }
    auto startX = start.x();
    auto startY = start.y();
    auto startZ = start.z();
    auto endX = end.x();
    auto endY = end.y();
    auto endZ = end.z();

    bool needGenParent = false;
    // destination must already have existing
    if (i == 0) {
      child = leaf;
    }

    // need to check whether source node already exist
    // if not, need to create new node (and break existing pathSeg if needed)
    if (i == (int) points.size() - 2) {
      if (mazeIdx2endPointNode.find(points.back())
          == mazeIdx2endPointNode.end()) {
        needGenParent = true;
      } else {
        needGenParent = false;
        parent = mazeIdx2endPointNode[points.back()];
      }
    }
    // if need to genParent, need to break pathSeg in case the source is not a
    // pin (i.e., branch from pathSeg)
    if (needGenParent && i == (int) points.size() - 2) {
      odb::Point srcLoc;
      gridGraph_.getPoint(points.back().x(), points.back().y(), srcLoc);
      frLayerNum srcLNum = (points.back().z() + 1) * 2;
      odb::Rect srcBox(srcLoc, srcLoc);
      std::vector<grConnFig*> result;
      workerRegionQuery.query(srcBox, srcLNum, result);
      for (auto rptr : result) {
        if (rptr->typeId() == grcPathSeg) {
          auto cptr = static_cast<grPathSeg*>(rptr);
          if (cptr->hasGrNet() && cptr->getGrNet() == net) {
            parent = routeNet_postAstarWritePath_splitPathSeg(
                cptr->getGrChild(), cptr->getGrParent(), srcLoc);
            mazeIdx2endPointNode[points.back()] = parent;
            break;
          }
        }
      }
    }

    // create parent node if no existing done
    if (parent == nullptr) {
      odb::Point parentLoc;
      frLayerNum parentLNum = (points[i + 1].z() + 1) * 2;
      gridGraph_.getPoint(points[i + 1].x(), points[i + 1].y(), parentLoc);

      auto uParent = std::make_unique<grNode>();
      parent = uParent.get();
      parent->addToNet(net);
      parent->setLoc(parentLoc);
      parent->setLayerNum(parentLNum);
      parent->setType(frNodeTypeEnum::frcSteiner);

      net->addNode(uParent);
      mazeIdx2endPointNode[points[i + 1]] = parent;
    }

    // child and parent are all set
    if (startX != endX && startY == endY && startZ == endZ) {
      // horz pathSeg
      odb::Point startLoc, endLoc;
      frLayerNum lNum = gridGraph_.getLayerNum(startZ);
      gridGraph_.getPoint(startX, startY, startLoc);
      gridGraph_.getPoint(endX, endY, endLoc);
      auto uPathSeg = std::make_unique<grPathSeg>();
      auto pathSeg = uPathSeg.get();
      pathSeg->setPoints(startLoc, endLoc);
      pathSeg->setLayerNum(lNum);
      pathSeg->addToNet(net);

      // update connectivity
      pathSeg->setParent(parent);
      pathSeg->setChild(child);
      child->setConnFig(pathSeg);
      child->setParent(parent);
      parent->addChild(child);

      // update rq
      workerRegionQuery.add(pathSeg);

      // update ownership
      std::unique_ptr<grConnFig> uConnFig(std::move(uPathSeg));
      net->addRouteConnFig(uConnFig);

    } else if (startX == endX && startY != endY && startZ == endZ) {
      // vert pathSeg
      odb::Point startLoc, endLoc;
      frLayerNum lNum = gridGraph_.getLayerNum(startZ);
      gridGraph_.getPoint(startX, startY, startLoc);
      gridGraph_.getPoint(endX, endY, endLoc);
      auto uPathSeg = std::make_unique<grPathSeg>();
      auto pathSeg = uPathSeg.get();
      pathSeg->setPoints(startLoc, endLoc);
      pathSeg->setLayerNum(lNum);
      pathSeg->addToNet(net);

      // update connectivity
      pathSeg->setParent(parent);
      pathSeg->setChild(child);
      child->setConnFig(pathSeg);
      child->setParent(parent);
      parent->addChild(child);

      // update rq
      workerRegionQuery.add(pathSeg);

      // update ownership
      std::unique_ptr<grConnFig> uConnFig(std::move(uPathSeg));
      net->addRouteConnFig(uConnFig);

    } else if (startX == endX && startY == endY && startZ != endZ) {
      // via(s)
      bool isChildBP = (startZ == points[i].z());
      // currZ is the bottom enclosure layer
      if (isChildBP) {
        // currZ is child node idx and parent idx is currZ + 1
        for (auto currZ = startZ; currZ < endZ; currZ++) {
          odb::Point loc;
          gridGraph_.getPoint(startX, startY, loc);
          grNode* tmpParent = nullptr;
          if (currZ == endZ - 1) {
            tmpParent = parent;
          } else {
            // create temporary parent node
            auto uNode = std::make_unique<grNode>();
            tmpParent = uNode.get();
            tmpParent->addToNet(net);
            tmpParent->setLoc(loc);
            tmpParent->setLayerNum(((currZ + 1) + 1) * 2);
            tmpParent->setType(frNodeTypeEnum::frcSteiner);

            net->addNode(uNode);

            // add to endPoint map
            mazeIdx2endPointNode[FlexMazeIdx(startX, startY, currZ + 1)]
                = tmpParent;
          }
          // create via
          auto uVia = std::make_unique<grVia>();
          auto via = uVia.get();
          via->setChild(child);
          via->setParent(tmpParent);
          via->addToNet(net);
          via->setOrigin(loc);
          via->setViaDef(getDesign()
                             ->getTech()
                             ->getLayer((currZ + 1) * 2 + 1)
                             ->getDefaultViaDef());

          // update connectivity
          child->setConnFig(via);
          child->setParent(tmpParent);
          tmpParent->addChild(child);

          // update rq
          workerRegionQuery.add(via);

          // update ownership
          std::unique_ptr<grConnFig> uConnFig(std::move(uVia));
          net->addRouteConnFig(uConnFig);

          child = tmpParent;
        }
      } else {
        // currZ is child idx and parent idx is currZ - 1
        for (auto currZ = endZ; currZ > startZ; currZ--) {
          odb::Point loc;
          gridGraph_.getPoint(startX, startY, loc);
          grNode* tmpParent = nullptr;
          if (currZ == startZ + 1) {
            tmpParent = parent;
          } else {
            // create temporary parent node
            auto uNode = std::make_unique<grNode>();
            tmpParent = uNode.get();
            tmpParent->addToNet(net);
            tmpParent->setLoc(loc);
            tmpParent->setLayerNum(((currZ - 1) + 1) * 2);
            tmpParent->setType(frNodeTypeEnum::frcSteiner);

            net->addNode(uNode);

            // add to endPoint map
            mazeIdx2endPointNode[FlexMazeIdx(startX, startY, currZ - 1)]
                = tmpParent;
          }
          // create via
          auto uVia = std::make_unique<grVia>();
          auto via = uVia.get();
          via->setChild(child);
          via->setParent(tmpParent);
          via->addToNet(net);
          via->setOrigin(loc);
          via->setViaDef(getDesign()
                             ->getTech()
                             ->getLayer(currZ * 2 + 1)
                             ->getDefaultViaDef());

          // update connectivty
          child->setConnFig(via);
          child->setParent(tmpParent);
          tmpParent->addChild(child);

          // update rq
          workerRegionQuery.add(via);

          // update ownership
          std::unique_ptr<grConnFig> uConnFig(std::move(uVia));
          net->addRouteConnFig(uConnFig);

          child = tmpParent;
        }
      }
    } else if (startX == endX && startY == endY && startZ == endZ) {
      std::cout << "Warning: zero-length path in routeNet_postAstarWritePath\n";
    } else {
      std::cout << "Error: non-colinear path in routeNet_postAstarWritePath\n";
    }

    // set child to parent and parent to null for next iteration
    child = parent;
    parent = nullptr;
  }
}

grNode* FlexGRWorker::routeNet_postAstarWritePath_splitPathSeg(
    grNode* child,
    grNode* parent,
    const odb::Point& breakPt)
{
  auto& workerRegionQuery = getWorkerRegionQuery();

  auto pathSeg = static_cast<grPathSeg*>(child->getConnFig());
  // remove from rq first
  workerRegionQuery.remove(pathSeg);

  auto net = pathSeg->getGrNet();
  auto lNum = pathSeg->getLayerNum();
  auto [bp, ep] = pathSeg->getPoints();
  odb::Point childLoc = child->getLoc();
  bool isChildBP = (childLoc == bp);

  auto uBreakNode = std::make_unique<grNode>();
  auto breakNode = uBreakNode.get();
  breakNode->addToNet(net);
  breakNode->setLoc(breakPt);
  breakNode->setLayerNum(lNum);
  breakNode->setType(frNodeTypeEnum::frcSteiner);

  net->addNode(uBreakNode);

  // update connectivity
  parent->removeChild(child);
  child->setParent(breakNode);
  breakNode->addChild(child);
  breakNode->setParent(parent);
  parent->addChild(breakNode);

  // shrink child - parent pathSeg to child - breakPt (modified from
  // initBoundary_splitPathSeg_split)
  auto pathSeg1 = pathSeg;
  pathSeg1->setChild(child);
  child->setConnFig(pathSeg1);
  pathSeg1->setParent(breakNode);
  pathSeg->addToNet(net);
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
  // workerRegionQuery.remove(pathSeg);
  workerRegionQuery.add(pathSeg1);
  workerRegionQuery.add(pathSeg2);

  // update net ownershiop (no need to touch pathSeg1 because it is shrinked
  // from original one)
  std::unique_ptr<grConnFig> uConnFig(std::move(uPathSeg2));
  net->addRouteConnFig(uConnFig);

  return breakNode;
}

void FlexGRWorker::routeNet_postRouteAddCong(grNet* net)
{
  for (auto& uptr : net->getRouteConnFigs()) {
    if (uptr->typeId() == grcPathSeg) {
      auto cptr = static_cast<grPathSeg*>(uptr.get());
      modCong_pathSeg(cptr, /*isAdd*/ true);
    }
  }
}

void FlexGRWorker::route_decayHistCost()
{
  frMIdx xDim = 0;
  frMIdx yDim = 0;
  frMIdx zDim = 0;

  gridGraph_.getDim(xDim, yDim, zDim);

  std::vector<grConnFig*> result;

  for (int zIdx = 0; zIdx < zDim; zIdx++) {
    for (int xIdx = 0; xIdx < xDim; xIdx++) {
      for (int yIdx = 0; yIdx < yDim; yIdx++) {
        // decay history cost
        gridGraph_.decayHistoryCost(xIdx, yIdx, zIdx, 0.8);
      }
    }
  }
}

}  // namespace drt
