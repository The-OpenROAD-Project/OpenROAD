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

#include <vector>

#include "gr/FlexGR.h"

namespace drt {

void FlexGRWorker::route(std::vector<grNet*> &nets)
{
  for (auto net : nets) {
    if (net->getCPUFlag() == false) {
      continue;
    }

    mazeNetInit(net);
    routeNet(net);
  }
}


void FlexGRWorker::route_decayHistCost_update()
{
 if (ripupMode_ == RipUpMode::ALL) {
    route_decayHistCost();
  }
}


// ----------------------------------------------------------------------------
// Original Functions
// ----------------------------------------------------------------------------
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






bool FlexGRWorker::routeNet(grNet* net)
{
  if (net->isTrivial()) {
    std::cout << "Error: trivial net should not be routed\n";
    return true;
  }

  gridGraph_.resetStatus();
  std::set<grNode*, frBlockObjectComp> unConnPinGCellNodes;
  std::map<FlexMazeIdx, grNode*> mazeIdx2unConnPinGCellNode;
  std::map<FlexMazeIdx, grNode*> mazeIdx2endPointNode;
  routeNet_prep(net,
                unConnPinGCellNodes,
                mazeIdx2unConnPinGCellNode,
                mazeIdx2endPointNode);

  FlexMazeIdx ccMazeIdx1, ccMazeIdx2;  // connComps ll, ur FlexMazeIdx
  Point centerPt;
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
  
  routeNet_checkNet(net);

  /*
  // To be deleted, just for testing
  if (net->getPinGCellNodes().size() < 0) {
    net->setPostCost(calcPathCost(net));
    if (net->getPostCost() != net->getPreCost()) {
      std::cout << "Net " << net->getFrNet()->getName() << " ";
      if (net->getPostCost() < net->getPreCost()) {
        std::cout << " Improved: PostCost = " << net->getPostCost() << " < PreCost = " << net->getPreCost() << std::endl;
      } else {
        std::cout << " Deteriorated: PostCost = " << net->getPostCost() << " > PreCost = " << net->getPreCost() << std::endl;
        std::cout << "PinGCellNodes: ";
        for (auto pinGCellNode : net->getPinGCellNodes()) {
          auto loc = pinGCellNode->getLoc();
          auto lNum = pinGCellNode->getLayerNum();
          FlexMazeIdx mi;
          gridGraph_.getMazeIdx(loc, lNum, mi);
          std::cout << "\t(" << mi.x() << ", " << mi.y() << ", " << mi.z() << ")";
        } 
        std::cout << std::endl;
        for (auto& uptr : net->getRouteConnFigs()) {
          if (uptr->typeId() == grcPathSeg) {
            auto cptr = static_cast<grPathSeg*>(uptr.get());
            auto [bp, ep] = cptr->getPoints();
            frLayerNum lNum = cptr->getLayerNum();
            FlexMazeIdx bi, ei;
            gridGraph_.getMazeIdx(bp, lNum, bi);
            gridGraph_.getMazeIdx(ep, lNum, ei);
            frDirEnum dir = (bi.x() == ei.x()) ? frDirEnum::N : frDirEnum::E;
            if (bi.x() == ei.x()) {
              std::cout << "vert: " << " bp = ( " << bi.x() << " " << bi.y() << " ) "
                        << " ep = ( " << ei.x() << " " << ei.y() << " ) " << std::endl;
            } else {
              // horz
              std::cout << "horz: " << " bp = ( " << bi.x() << " " << bi.y() << " ) "
                        << " ep = ( " << ei.x() << " " << ei.y() << " ) " << std::endl;
            }
          }
        } 
      }
    }
  }
  */

  return true;
}

void FlexGRWorker::routeNet_prep(
    grNet* net,
    std::set<grNode*, frBlockObjectComp>& unConnPinGCellNodes,
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


// Add by zhiang 
void FlexGRWorker::routeNet_checkNet(grNet* net)
{
  // Make sure all the pinGCellNodes are visited
  // Check all the pinGCellNodes in the net
  std::map<FlexMazeIdx, bool> pinGCellNodesVisited;
  for (auto pinGCellNode : net->getPinGCellNodes()) {
    auto loc = pinGCellNode->getLoc();
    auto lNum = pinGCellNode->getLayerNum();
    FlexMazeIdx mi;
    gridGraph_.getMazeIdx(loc, lNum, mi);
    //std::cout << "pinGCellNode: x = " << mi.x() << ", y = " << mi.y() << ", z = " << mi.z() << "\n";
    pinGCellNodesVisited[mi] = false;
  }

  auto originalPinGCellNodes = net->getPinGCellIdxs();
  for (auto& idx : originalPinGCellNodes) {
    if (pinGCellNodesVisited.find(idx) == pinGCellNodesVisited.end()) {
      std::cout << "Error: original pinGCellNode not found in the net : ";
      std::cout << "x = " << idx.x() << ", y = " << idx.y() << ", z = " << idx.z() << "\n"; 
      exit(1);
    }
  }

  // Start traversing the net
  auto root = net->getRoot();
  auto ripin2GCellNode = net->getPinGCellNodePairs();
  std::map<FlexMazeIdx, FlexMazeIdx> pin2GCellNode;
  for (auto& [pinNode, gcellNode] : ripin2GCellNode) {
    auto pinLoc = pinNode->getLoc();
    auto pinLNum = pinNode->getLayerNum();
    FlexMazeIdx pinMi;
    pinMi.set(pinLoc.x(), pinLoc.y(), pinLNum);
    
    auto loc = gcellNode->getLoc();
    auto lNum = gcellNode->getLayerNum();
    FlexMazeIdx mi;
    gridGraph_.getMazeIdx(loc, lNum, mi);
    pin2GCellNode[pinMi] = mi;
  }
  

  std::queue<grNode*> nodeQ;  
  nodeQ.push(root);
  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop();

    // Check if the node is a pinGCellNode
    if (node->getType() == frNodeTypeEnum::frcPin || node->getType() == frNodeTypeEnum::frcBoundaryPin) {
      auto loc = node->getLoc();
      auto lNum = node->getLayerNum();
      FlexMazeIdx pinMi;
      pinMi.set(loc.x(), loc.y(), lNum);
      auto mi = pin2GCellNode[pinMi];
      if (pinGCellNodesVisited.find(mi) != pinGCellNodesVisited.end()) {
        pinGCellNodesVisited[mi] = true;
      } else {
        std::cout << "Error: pinGCellNode not found in the net : ";
        std::cout << "x = " << mi.x() << ", y = " << mi.y() << ", z = " << mi.z() << "\n"; 
        exit(1);
      }
    }

    for (auto child : node->getChildren()) {
      nodeQ.push(child);
    }
  }

  
  for (auto& [mi, visited] : pinGCellNodesVisited) {
    if (!visited) {
      int x = mi.x();
      int y = mi.y();
      int z = mi.z();
      std::cout << "Error :  x = " << x << ", y = " << y << ", z = " << z << " Error: pinGCellNode not visited\n";
      routeNet_printNet(net);      
      exit(1);
    }
  }
}


void FlexGRWorker::routeNet_printNet(grNet* net)
{
  auto root = net->getRoot();
  std::deque<grNode*> nodeQ;
  nodeQ.push_back(root);

  std::cout << "start traversing subnet of " << net->getFrNet()->getName()
            << std::endl;

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
    Point loc = node->getLoc();
    frLayerNum lNum = node->getLayerNum();
    std::cout << "(" << loc.x() << ", " << loc.y() << ") on layerNum " << lNum
              << std::endl;

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
      Point loc = child->getLoc();
      frLayerNum lNum = child->getLayerNum();
      std::cout << "(" << loc.x() << ", " << loc.y() << ") on layerNum " << lNum;
      if (child->getType() == frNodeTypeEnum::frcSteiner) {
        FlexMazeIdx mi;
        gridGraph_.getMazeIdx(loc, lNum, mi);
        std::cout << " mazeIdx (" << mi.x() << ", " << mi.y() << ", " << mi.z() << ")";
      }
      std::cout << std::endl;
    }
  }
}

// current set subnet root to be src with the absence of reroot
void FlexGRWorker::routeNet_setSrc(
    grNet* net,
    std::set<grNode*, frBlockObjectComp>& unConnPinGCellNodes,
    std::map<FlexMazeIdx, grNode*>& mazeIdx2unConnPinGCellNode,
    std::vector<FlexMazeIdx>& connComps,
    FlexMazeIdx& ccMazeIdx1,
    FlexMazeIdx& ccMazeIdx2,
    Point& centerPt)
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
  Point pt;
  Point ll, ur;
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
      frCoord dx = std::max(std::max(ll.x() - pt.x(), pt.x() - ur.x()), 0);
      frCoord dy = std::max(std::max(ll.y() - pt.y(), pt.y() - ur.y()), 0);
      frCoord dz
          = std::max(std::max(gridGraph_.getZHeight(ccMazeIdx1.z())
                                  - gridGraph_.getZHeight(mazeIdx.z()),
                              gridGraph_.getZHeight(mazeIdx.z())
                                  - gridGraph_.getZHeight(ccMazeIdx2.z())),
                     0);
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
    std::cout << nextDst->getNet()->getFrNet()->getName() << std::endl;
  }
  return nextDst;
}

grNode* FlexGRWorker::routeNet_postAstarUpdate(
    std::vector<FlexMazeIdx>& path,
    std::vector<FlexMazeIdx>& connComps,
    std::set<grNode*, frBlockObjectComp>& unConnPinGCellNodes,
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
    std::cout << "Error: routeNet_postAstarUpdate path is empty" << std::endl;
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
  if (points.size() < 2) {  return; }   
    
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
    if (i == 0) {  child = leaf; }

    // need to check whether source node already exist
    // if not, need to create new node (and break existing pathSeg if needed)
    if (i == (int) points.size() - 2) {
      if (mazeIdx2endPointNode.find(points.back()) == mazeIdx2endPointNode.end()) {
        needGenParent = true;
      } else {
        needGenParent = false;
        parent = mazeIdx2endPointNode[points.back()];
      }
      // std::cout << "Need Gen Parent: " << needGenParent << std::endl;
    }

  
    // if need to genParent, need to break pathSeg in case the source is not a
    // pin (i.e., branch from pathSeg)
    if (needGenParent && i == (int) points.size() - 2) {
      Point srcLoc;
      gridGraph_.getPoint(points.back().x(), points.back().y(), srcLoc);
      frLayerNum srcLNum = (points.back().z() + 1) * 2;
      Rect srcBox(srcLoc, srcLoc);
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
      // Check if the parent node already exists
      if (mazeIdx2endPointNode.find(points[i + 1]) != mazeIdx2endPointNode.end()) {
        parent = mazeIdx2endPointNode[points[i + 1]];
      } else {
        // create parent node
        Point parentLoc;
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
    }

      
    // child and parent are all set
    if (startX != endX && startY == endY && startZ == endZ) {
      // horz pathSeg
      Point startLoc, endLoc;
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
      Point startLoc, endLoc;
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
          Point loc;
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
          Point loc;
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




/*
void FlexGRWorker::routeNet_postAstarWritePath_old(
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
      Point srcLoc;
      gridGraph_.getPoint(points.back().x(), points.back().y(), srcLoc);
      frLayerNum srcLNum = (points.back().z() + 1) * 2;
      Rect srcBox(srcLoc, srcLoc);
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
      Point parentLoc;
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
      Point startLoc, endLoc;
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
      Point startLoc, endLoc;
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
          Point loc;
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
          Point loc;
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
*/


grNode* FlexGRWorker::routeNet_postAstarWritePath_splitPathSeg(
    grNode* child,
    grNode* parent,
    const Point& breakPt)
{
  auto& workerRegionQuery = getWorkerRegionQuery();

  auto pathSeg = static_cast<grPathSeg*>(child->getConnFig());
  // remove from rq first
  workerRegionQuery.remove(pathSeg);

  auto net = pathSeg->getGrNet();
  auto lNum = pathSeg->getLayerNum();
  auto [bp, ep] = pathSeg->getPoints();
  Point childLoc = child->getLoc();
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
