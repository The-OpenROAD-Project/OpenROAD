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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "gr/FlexGR.h"

using namespace std;
using namespace fr;

void FlexGRWorker::route() {
  bool enableOutput = false;
  if (enableOutput) {
    cout << "start maze route #nets = " << nets_.size() << endl;
  }

  vector<grNet*> rerouteNets;
  for (int i = 0; i < mazeEndIter_; i++) {
    if (enableOutput) {
      cout << "  iter" << i << endl;
    }
    if (ripupMode_ == 0) {
      route_addHistCost();
    }
    route_mazeIterInit();
    route_getRerouteNets(rerouteNets);
    for (auto net: rerouteNets) {
      if (ripupMode_ == 0 || (ripupMode_ == 1 && mazeNetHasCong(net))) {
        mazeNetInit(net);
        routeNet(net);
      }
    }
    if (ripupMode_ == 1) {
      route_decayHistCost();
    }
  }
}

void FlexGRWorker::route_addHistCost() {
  frMIdx xDim = 0;
  frMIdx yDim = 0;
  frMIdx zDim = 0;

  gridGraph_.getDim(xDim, yDim, zDim);

  for (int zIdx = 0; zIdx < zDim; zIdx++) {
    // frLayerNum lNum = (zIdx + 1) * 2;
    for (int xIdx = 0; xIdx <xDim; xIdx++) {
      for (int yIdx = 0; yIdx < yDim; yIdx++) {
        if (gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::E) > congThresh_ * gridGraph_.getRawSupply(xIdx, yIdx, zIdx, frDirEnum::E) ||
            gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::N) > congThresh_ * gridGraph_.getRawSupply(xIdx, yIdx, zIdx, frDirEnum::N)) {
          // has congestion, need to add history cost
          int overflowH = max(0, int(gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::E) - congThresh_ * gridGraph_.getRawSupply(xIdx, yIdx, zIdx, frDirEnum::E)));
          int overflowV = max(0, int(gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::N) - congThresh_ * gridGraph_.getRawSupply(xIdx, yIdx, zIdx, frDirEnum::N)));
          gridGraph_.addHistoryCost(xIdx, yIdx, zIdx, (overflowH + overflowV) / 2 + 1);
        }
      }
    }
  }
}

void FlexGRWorker::route_mazeIterInit() {
  // reset ripup
  for (auto &net: nets_) {
    net->setRipup(false);
  }
  if (ripupMode_ == 0) {
    // set ripup based on congestion
    auto &workerRegionQuery = getWorkerRegionQuery();

    frMIdx xDim = 0;
    frMIdx yDim = 0;
    frMIdx zDim = 0;

    gridGraph_.getDim(xDim, yDim, zDim);

    vector<grConnFig*> result;

    for (int zIdx = 0; zIdx < zDim; zIdx++) {
      frLayerNum lNum = (zIdx + 1) * 2;
      for (int xIdx = 0; xIdx <xDim; xIdx++) {
        for (int yIdx = 0; yIdx < yDim; yIdx++) {
          if (gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::E) > congThresh_ * gridGraph_.getRawSupply(xIdx, yIdx, zIdx, frDirEnum::E) ||
              gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::N) > congThresh_ * gridGraph_.getRawSupply(xIdx, yIdx, zIdx, frDirEnum::N)) {
            // has congestion, need to region query to find what nets are using this gcell
            frPoint gcellCenter;
            gridGraph_.getPoint(xIdx, yIdx, gcellCenter);
            frBox gcellCenterBox(gcellCenter, gcellCenter);
            result.clear();
            workerRegionQuery.query(gcellCenterBox, lNum, result);
            for (auto rptr: result) {
              if (rptr->typeId() == grcPathSeg) {
                auto cptr = static_cast<grPathSeg*>(rptr);
                if (cptr->hasGrNet()) {
                  cptr->getGrNet()->setRipup(true);
                } else {
                  cout << "Error: route_mazeIterInit hasGrNet() empty" <<endl;
                }
              }
            }
          }
        }
      }
    }
  }
}

void FlexGRWorker::route_getRerouteNets(vector<grNet*> &rerouteNets) {
  bool enableOutput = false;
  rerouteNets.clear();
  if (ripupMode_ == 0) {
    for (auto &net: nets_) {
      if (net->isRipup() && !net->isTrivial()) {
        rerouteNets.push_back(net.get());
        net->setModified(true);
        net->getFrNet()->setModified(true);
      } else {
        if (enableOutput) {
          if (!net->isRipup()) {
            cout << "subnet of " << net->getFrNet()->getName() << " is not set for ripup\n";
          }
          if (net->isTrivial()) {
            cout << "subnet of " << net->getFrNet()->getName() << " is trivial\n";
          }
        }
      }
    }
  } else if (ripupMode_ == 1) {
    for (auto &net: nets_) {
      if (!net->isTrivial()) {
        rerouteNets.push_back(net.get());
      }
    }
  }
}

bool FlexGRWorker::mazeNetHasCong(grNet* net) {
  for (auto &uptr: net->getRouteConnFigs()) {
    if (uptr->typeId() == grcPathSeg) {
      auto cptr = static_cast<grPathSeg*>(uptr.get());
      frPoint bp, ep;
      cptr->getPoints(bp, ep);
      frLayerNum lNum = cptr->getLayerNum();
      FlexMazeIdx bi, ei;
      gridGraph_.getMazeIdx(bp, lNum, bi);
      gridGraph_.getMazeIdx(ep, lNum, ei);
      if (bi.x() == ei.x()) {
        // vert
        for (auto yIdx = bi.y(); yIdx <= ei.y(); yIdx++) {
          if (gridGraph_.getRawDemand(bi.x(), yIdx, bi.z(), frDirEnum::E) > getCongThresh() * gridGraph_.getRawSupply(bi.x(), yIdx, bi.z(), frDirEnum::E) ||
              gridGraph_.getRawDemand(bi.x(), yIdx, bi.z(), frDirEnum::N) > getCongThresh() * gridGraph_.getRawSupply(bi.x(), yIdx, bi.z(), frDirEnum::N)) {
            net->setModified(true);
            net->getFrNet()->setModified(true);
            net->setRipup(true);

            return true;
          }
          
        }
      } else {
        // horz
        for (auto xIdx = bi.x(); xIdx <= ei.x(); xIdx++) {
          if (gridGraph_.getRawDemand(xIdx, bi.y(), bi.z(), frDirEnum::E) > getCongThresh() * gridGraph_.getRawSupply(xIdx, bi.y(), bi.z(), frDirEnum::E) ||
              gridGraph_.getRawDemand(xIdx, bi.y(), bi.z(), frDirEnum::N) > getCongThresh() * gridGraph_.getRawSupply(xIdx, bi.y(), bi.z(), frDirEnum::N)) {
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
void FlexGRWorker::mazeNetInit(grNet* net) {
  bool enableOutput = false;
  if (enableOutput) {
    cout << "mazeInit subnet of " << net->getFrNet()->getName() << endl;
  }

  gridGraph_.resetStatus();
  if (ripupMode_ == 0) {
    mazeNetInit_decayHistCost(net);
  } else if (ripupMode_ == 1) {
    mazeNetInit_addHistCost(net);
  }
  mazeNetInit_removeNetObjs(net);
  mazeNetInit_removeNetNodes(net);
}

void FlexGRWorker::mazeNetInit_addHistCost(grNet* net) {
  // add history cost for all gcells that this net previously visited
  set<FlexMazeIdx> pts;
  for (auto &uptr: net->getRouteConnFigs()) {
    if (uptr->typeId() == grcPathSeg) {
      auto cptr = static_cast<grPathSeg*>(uptr.get());
      frPoint bp, ep;
      cptr->getPoints(bp, ep);
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
  for (auto pt: pts) {
    if (gridGraph_.getRawDemand(pt.x(), pt.y(), pt.z(), frDirEnum::E) >  getCongThresh() * gridGraph_.getRawSupply(pt.x(), pt.y(), pt.z(), frDirEnum::E) ||
        gridGraph_.getRawDemand(pt.x(), pt.y(), pt.z(), frDirEnum::N) >  getCongThresh() * gridGraph_.getRawSupply(pt.x(), pt.y(), pt.z(), frDirEnum::N))
      gridGraph_.addHistoryCost(pt.x(), pt.y(), pt.z(), 1);
  }
}

void FlexGRWorker::mazeNetInit_decayHistCost(grNet* net) {
  // decay history cost for all gcells that this net previously visited
  set<FlexMazeIdx> pts;
  for (auto &uptr: net->getRouteConnFigs()) {
    if (uptr->typeId() == grcPathSeg) {
      auto cptr = static_cast<grPathSeg*>(uptr.get());
      frPoint bp, ep;
      cptr->getPoints(bp, ep);
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
  for (auto pt: pts) {
    gridGraph_.decayHistoryCost(pt.x(), pt.y(), pt.z());
  }
}

// with vector, we can only erase all route objs
// if we need partial ripup, need to change routeConnfigs to list
void FlexGRWorker::mazeNetInit_removeNetObjs(grNet* net) {
  auto &workerRegionQuery = getWorkerRegionQuery();
  // remove everything from region query
  for (auto &uptr: net->getRouteConnFigs()) {
    workerRegionQuery.remove(uptr.get());
  }
  // update congestion for the to-be-removed objs
  for (auto &uptr: net->getRouteConnFigs()) {
    if (uptr->typeId() == grcPathSeg) {
      auto cptr = static_cast<grPathSeg*>(uptr.get());
      modCong_pathSeg(cptr, /*isAdd*/false);
    }
  }

  // remove the connFigs themselves
  net->clearRouteConnFigs();
}

void FlexGRWorker::modCong_pathSeg(grPathSeg* pathSeg, bool isAdd) {
  FlexMazeIdx bi, ei;
  frPoint bp, ep;
  frLayerNum lNum = pathSeg->getLayerNum();
  pathSeg->getPoints(bp, ep);
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
    cout << "Error: non-colinear pathSeg in modCong_pathSeg" << endl;
  }
} 

// 
void FlexGRWorker::mazeNetInit_removeNetNodes(grNet* net) {
  // remove non-terminal gcell nodes
  deque<grNode*> nodeQ;
  set<grNode*> terminalNodes;
  grNode* root = nullptr;

  for (auto &[pinNode, gcellNode]: net->getPinGCellNodePairs()) {
    if (root == nullptr) {
      root = gcellNode;
    }
    terminalNodes.insert(gcellNode);
  }

  if (root == nullptr) {
    cout << "Error: root is nullptr\n";
  }

  nodeQ.push_back(root);
  // bfs to remove non-terminal nodes
  // only steiner node can exist in queue
  bool isRoot = true;
  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    // cout << "  " << node << " (" << node->getLoc().x() << ", " << node->getLoc().y() << ", " << node->getLayerNum() << ")" << endl;
    nodeQ.pop_front();

    // push steiner child
    for (auto child: node->getChildren()) {
      if (child->getType() == frNodeTypeEnum::frcSteiner) {
        nodeQ.push_back(child);
      }
    }

    // update connection
    // for root terminal node, clear all steiner children;
    // for non-root terminal nodes, clear all steiner children and reset parent and remove node
    if (terminalNodes.find(node) != terminalNodes.end()) {
      std::vector<grNode*> steinerChildNodes;
      for (auto child: node->getChildren()) {
        if (child->getType() == frNodeTypeEnum::frcSteiner) {
          steinerChildNodes.push_back(child);
        }
      }
      for (auto steinerChildNode: steinerChildNodes) {
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

bool FlexGRWorker::routeNet(grNet* net) {
  bool enableOutput = false;
  if (net->isTrivial()) {
    cout << "Error: trivial net should not be routed\n";
    return true;
  }

  if (enableOutput) {
    cout << "route " << net->getFrNet()->getName() << endl;
  }

  set<grNode*, frBlockObjectComp> unConnPinGCellNodes;
  map<FlexMazeIdx, grNode*> mazeIdx2unConnPinGCellNode;
  map<FlexMazeIdx, grNode*> mazeIdx2endPointNode;
  routeNet_prep(net, unConnPinGCellNodes, mazeIdx2unConnPinGCellNode, mazeIdx2endPointNode);

  FlexMazeIdx ccMazeIdx1, ccMazeIdx2; // connComps ll, ur FlexMazeIdx
  frPoint centerPt;
  vector<FlexMazeIdx> connComps;

  if (enableOutput) {
    cout << "    #pin = " << mazeIdx2unConnPinGCellNode.size() << endl;
  }

  routeNet_setSrc(net, unConnPinGCellNodes, mazeIdx2unConnPinGCellNode, connComps, ccMazeIdx1, ccMazeIdx2, centerPt);
  
  if (enableOutput) {
    cout << "    #dst pin = " << mazeIdx2unConnPinGCellNode.size() << endl;
  }

  vector<FlexMazeIdx> path; // astar must return with more than one idx

  while (!unConnPinGCellNodes.empty()) {
    // reset prev dirs before routing to a new dst
    gridGraph_.resetPrevNodeDir();
    path.clear();
    auto nextPinGCellNode = routeNet_getNextDst(ccMazeIdx1, ccMazeIdx2, mazeIdx2unConnPinGCellNode);
    if (enableOutput) {
      cout << "      dst at (" << nextPinGCellNode->getLoc().x() << ", " << nextPinGCellNode->getLoc().y() << ", " << nextPinGCellNode->getLayerNum() << ")\n";
    }
    if (gridGraph_.search(connComps, nextPinGCellNode, path, ccMazeIdx1, ccMazeIdx2, centerPt)) {
      auto leaf = routeNet_postAstarUpdate(path, connComps, unConnPinGCellNodes, mazeIdx2unConnPinGCellNode);
      routeNet_postAstarWritePath(net, path, leaf, mazeIdx2endPointNode);
    } else {
      return false;
    }
  }

  if (enableOutput) {
    cout << "    done routing\n";
  }

  routeNet_postRouteAddCong(net);
  return true;
}

void FlexGRWorker::routeNet_prep(grNet* net, set<grNode*, frBlockObjectComp> &unConnPinGCellNodes, 
                                 map<FlexMazeIdx, grNode*> &mazeIdx2unConnPinGCellNode,
                                 map<FlexMazeIdx, grNode*> &mazeIdx2endPointNode) {
  bool enableOutput = false;
  for (auto pinGCellNode: net->getPinGCellNodes()) {
    auto loc = pinGCellNode->getLoc();
    auto lNum = pinGCellNode->getLayerNum();
    FlexMazeIdx mi;
    gridGraph_.getMazeIdx(loc, lNum, mi);
    gridGraph_.setDst(mi);

    unConnPinGCellNodes.insert(pinGCellNode);
    if (mazeIdx2unConnPinGCellNode.find(mi) != mazeIdx2unConnPinGCellNode.end()) {
      if (mazeIdx2unConnPinGCellNode[mi] == net->getPinGCellNodes()[0]) {
        cout << "Warning: unConnPinGCellNode overlaps with root pinGCellNode\n";
      } else {
        cout << "Warning: overlapping leaf pinGCellNode\n";
      }
    }
    mazeIdx2unConnPinGCellNode[mi] = pinGCellNode;
    if (enableOutput) {
      if (mazeIdx2endPointNode.find(mi) != mazeIdx2endPointNode.end()) {
        cout << "Error: mi already exists in mazeIdx2endPointNode\n" << flush;
      }
    }
    mazeIdx2endPointNode[mi] = pinGCellNode;
  }
}

void FlexGRWorker::routeNet_printNet(grNet* net) {
  auto root = net->getRoot();
  deque<grNode*> nodeQ;
  nodeQ.push_back(root);

  cout << "start traversing subnet of " << net->getFrNet()->getName() << endl;

  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop_front();

    cout << "  parent ";
    if (node->getType() == frNodeTypeEnum::frcSteiner) {
      cout << "steiner node ";
    } else if (node->getType() == frNodeTypeEnum::frcPin) {
      cout << "pin node ";
    } else if (node->getType() == frNodeTypeEnum::frcBoundaryPin) {
      cout << "boundary pin node ";
    }
    cout << "at ";
    frPoint loc = node->getLoc();
    frLayerNum lNum = node->getLayerNum();
    cout << "(" << loc.x() << ", " << loc.y() << ") on layerNum " << lNum << endl;

    for (auto child: node->getChildren()) {
      nodeQ.push_back(child);
      cout << "    child ";
      if (child->getType() == frNodeTypeEnum::frcSteiner) {
        cout << "steiner node ";
      } else if (child->getType() == frNodeTypeEnum::frcPin) {
        cout << "pin node ";
      } else if (child->getType() == frNodeTypeEnum::frcBoundaryPin) {
        cout << "boundary pin node ";
      }
      cout << "at ";
      frPoint loc = child->getLoc();
      frLayerNum lNum = child->getLayerNum();
      cout << "(" << loc.x() << ", " << loc.y() << ") on layerNum " << lNum << endl;
    }
  }
}

// current set subnet root to be src with the absence of reroot
void FlexGRWorker::routeNet_setSrc(grNet* net, 
                                   set<grNode*, frBlockObjectComp> &unConnPinGCellNodes, 
                                   map<FlexMazeIdx, grNode*> &mazeIdx2unConnPinGCellNode,
                                   vector<FlexMazeIdx> &connComps,
                                   FlexMazeIdx &ccMazeIdx1, 
                                   FlexMazeIdx &ccMazeIdx2, 
                                   frPoint &centerPt) {
  bool enableOutput = false;
  frMIdx xDim, yDim, zDim;
  gridGraph_.getDim(xDim, yDim, zDim);
  ccMazeIdx1.set(xDim - 1, yDim - 1, zDim - 1);
  ccMazeIdx2.set(0, 0, 0);

  centerPt.set(0, 0);
  int totPinCnt = unConnPinGCellNodes.size();
  frCoord totX = 0;
  frCoord totY = 0;
  frCoord totZ = 0;
  for (auto pinGCellNode: unConnPinGCellNodes) {
    auto loc = pinGCellNode->getLoc();
    auto lNum = pinGCellNode->getLayerNum();
    FlexMazeIdx mi;
    gridGraph_.getMazeIdx(loc, lNum, mi);

    totX += loc.x();
    totY += loc.y();
    centerPt.set(centerPt.x() + loc.x(), centerPt.y() + loc.y());
    totZ += gridGraph_.getZHeight(mi.z());
  }
  totX /= totPinCnt;
  totY /= totPinCnt;
  totZ /= totPinCnt;
  centerPt.set(centerPt.x() / totPinCnt, centerPt.y() / totPinCnt);



  // currently use root gcell
  auto rootPinGCellNode = net->getPinGCellNodes()[0];
  unConnPinGCellNodes.erase(rootPinGCellNode);

  auto loc = rootPinGCellNode->getLoc();
  auto lNum = rootPinGCellNode->getLayerNum();
  FlexMazeIdx mi;
  gridGraph_.getMazeIdx(loc, lNum, mi);

  if (enableOutput) {
    cout << "  src at mazeIdx = (" << mi.x() << ", " << mi.y() << ", " << mi.z() << ")\n";
  }

  mazeIdx2unConnPinGCellNode.erase(mi);
  gridGraph_.setSrc(mi);
  gridGraph_.resetDst(mi);

  connComps.push_back(mi);
  ccMazeIdx1.set(min(ccMazeIdx1.x(), mi.x()),
                 min(ccMazeIdx1.y(), mi.y()),
                 min(ccMazeIdx1.z(), mi.z()));
  ccMazeIdx2.set(max(ccMazeIdx2.x(), mi.x()),
                 max(ccMazeIdx2.y(), mi.y()),
                 max(ccMazeIdx2.z(), mi.z()));
}

grNode* FlexGRWorker::routeNet_getNextDst(FlexMazeIdx &ccMazeIdx1, FlexMazeIdx &ccMazeIdx2, 
                                          map<FlexMazeIdx, grNode*> &mazeIdx2unConnPinGCellNode) {
  bool enableOutput = false;
  frPoint pt;
  frPoint ll, ur;
  gridGraph_.getPoint(ccMazeIdx1.x(), ccMazeIdx1.y(), ll);
  gridGraph_.getPoint(ccMazeIdx2.x(), ccMazeIdx2.y(), ur);
  frCoord currDist = std::numeric_limits<frCoord>::max();
  grNode* nextDst = nullptr;

  if (mazeIdx2unConnPinGCellNode.empty()) {
    cout << "Error: no available nextDst\n" << flush;
  }

  if (!nextDst) {
    for (auto &[mazeIdx, node]: mazeIdx2unConnPinGCellNode) {
      if (node == nullptr) {
        cout << "Error: nullptr node in mazeIdx2unConnPinGCellNode\n" << flush;
      }
      gridGraph_.getPoint(mazeIdx.x(), mazeIdx.y(), pt);
      if (enableOutput) {
        cout << "  mazeIdx2unConnPinGCellNode at mazeIdx = (" << mazeIdx.x() << ", " << mazeIdx.y() << ", " << mazeIdx.z() << ")\n";
      }
      frCoord dx = max(max(ll.x() - pt.x(), pt.x() - ur.x()), 0);
      frCoord dy = max(max(ll.y() - pt.y(), pt.y() - ur.y()), 0);
      frCoord dz = max(max(gridGraph_.getZHeight(ccMazeIdx1.z()) - gridGraph_.getZHeight(mazeIdx.z()), 
                           gridGraph_.getZHeight(mazeIdx.z()) - gridGraph_.getZHeight(ccMazeIdx2.z())), 0);
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
    cout << "Error: nextDst node is nullptr\n" << flush;
    cout << nextDst->getNet()->getFrNet()->getName() << endl;
  }
  return nextDst;
}

grNode* FlexGRWorker::routeNet_postAstarUpdate(vector<FlexMazeIdx> &path, vector<FlexMazeIdx> &connComps,
                                               set<grNode*, frBlockObjectComp> &unConnPinGCellNodes, 
                                               map<FlexMazeIdx, grNode*> &mazeIdx2unConnPinGCellNode) {
  set<FlexMazeIdx> localConnComps;
  grNode* leaf = nullptr;
  if (!path.empty()) {
    auto mi = path[0];
    if (mazeIdx2unConnPinGCellNode.find(mi) == mazeIdx2unConnPinGCellNode.end()) {
      cout << "Error: destination pin gcellNode not found\n";
    }
    leaf = mazeIdx2unConnPinGCellNode[mi];
    mazeIdx2unConnPinGCellNode.erase(mi);
    unConnPinGCellNodes.erase(leaf);
    gridGraph_.resetDst(mi);
    gridGraph_.setSrc(mi);
    localConnComps.insert(mi);
  } else {
    cout << "Error: routeNet_postAstarUpdate path is empty" << endl;
  }

  for (int i = 0; i < (int)path.size() - 1; i++) {
    auto start = path[i];
    auto end = path[i+1];
    auto startX = start.x(); 
    auto startY = start.y();
    auto startZ = start.z();
    auto endX = end.x();
    auto endY = end.y();
    auto endZ = end.z();
    // horizontal wire
    if (startX != endX && startY == endY && startZ == endZ) {
      for (auto currX = std::min(startX, endX); currX <= std::max(startX, endX); ++currX) {
        localConnComps.insert(FlexMazeIdx(currX, startY, startZ));
        gridGraph_.setSrc(currX, startY, startZ);
      }
    // vertical wire
    } else if (startX == endX && startY != endY && startZ == endZ) {
      for (auto currY = std::min(startY, endY); currY <= std::max(startY, endY); ++currY) {
        localConnComps.insert(FlexMazeIdx(startX, currY, startZ));
        gridGraph_.setSrc(startX, currY, startZ);
      }
    // via
    } else if (startX == endX && startY == endY && startZ != endZ) {
      for (auto currZ = std::min(startZ, endZ); currZ <= std::max(startZ, endZ); ++currZ) {
        localConnComps.insert(FlexMazeIdx(startX, startY, currZ));
        gridGraph_.setSrc(startX, startY, currZ);
      }
    // zero length
    } else if (startX == endX && startY == endY && startZ == endZ) {
      //root.addPinGrid(startX, startY, startZ);
      std::cout << "Warning: zero-length path in updateFlexPin\n";
    } else {
      std::cout << "Error: non-colinear path in updateFlexPin\n";
    }
  }
  for (auto &mi: localConnComps) {
    if (!(mi == *(path.cbegin()))) {
      connComps.push_back(mi);
    }
  }

  return leaf;
}

void FlexGRWorker::routeNet_postAstarWritePath(grNet* net, vector<FlexMazeIdx> &points, grNode* leaf,
                                               map<FlexMazeIdx, grNode*> &mazeIdx2endPointNode) {
  bool enableOutput = false;
  
  if (points.empty()) {
    if (enableOutput) {
      cout << "Warning: path is empty in writeMazePath\n";
    }
  }

  if (enableOutput) {
    cout << "  start routeNet_postAstarWritePath\n";
  }
  
  auto &workerRegionQuery = getWorkerRegionQuery();
  grNode* child = nullptr;
  grNode* parent = nullptr;

  for (int i = 0; i < (int)points.size() - 1; i++) {
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
    if (enableOutput) {
      frPoint childLoc = child->getLoc();
      cout << "  child at (" << childLoc.x() << ", " << childLoc.y() << ") on layer " << child->getLayerNum() << "\n";
    }

    // need to check whether source node already exist
    // if not, need to create new node (and break existing pathSeg if needed)
    if (i == (int)points.size() - 2) {
      if (mazeIdx2endPointNode.find(points.back()) == mazeIdx2endPointNode.end()) {
        needGenParent = true;
      } else {
        needGenParent = false;
        parent = mazeIdx2endPointNode[points.back()];
      }
    }
    // if need to genParent, need to break pathSeg in case the source is not a pin (i.e., branch from pathSeg)
    if (needGenParent && i == (int)points.size() - 2) {
      frPoint srcLoc;
      gridGraph_.getPoint(points.back().x(), points.back().y(), srcLoc);
      frLayerNum srcLNum = (points.back().z() + 1) * 2;
      frBox srcBox(srcLoc, srcLoc);
      vector<grConnFig*> result;
      workerRegionQuery.query(srcBox, srcLNum, result);
      for (auto rptr: result) {
        if (rptr->typeId() == grcPathSeg) {
          auto cptr = static_cast<grPathSeg*>(rptr);
          if (cptr->hasGrNet() && cptr->getGrNet() == net) {
            parent = routeNet_postAstarWritePath_splitPathSeg(cptr->getGrChild(), cptr->getGrParent(), srcLoc);
            mazeIdx2endPointNode[points.back()] = parent;
            break;
          }
        }
      }
    }

    // create parent node if no existing done
    if (parent == nullptr) {
      frPoint parentLoc;
      frLayerNum parentLNum = (points[i+1].z() + 1) * 2;
      gridGraph_.getPoint(points[i+1].x(), points[i+1].y(), parentLoc);

      auto uParent = make_unique<grNode>();
      parent = uParent.get();
      parent->addToNet(net);
      parent->setLoc(parentLoc);
      parent->setLayerNum(parentLNum);
      parent->setType(frNodeTypeEnum::frcSteiner);

      net->addNode(uParent);
      mazeIdx2endPointNode[points[i+1]] = parent;
    }

    // child and parent are all set 
    if (startX != endX && startY == endY && startZ == endZ) {
      // horz pathSeg
      frPoint startLoc, endLoc;
      frLayerNum lNum = gridGraph_.getLayerNum(startZ);
      gridGraph_.getPoint(startX, startY, startLoc);
      gridGraph_.getPoint(endX, endY, endLoc);
      auto uPathSeg = make_unique<grPathSeg>();
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
      unique_ptr<grConnFig> uConnFig(std::move(uPathSeg));
      net->addRouteConnFig(uConnFig);

    } else if (startX == endX && startY != endY && startZ == endZ) {
      // vert pathSeg
      frPoint startLoc, endLoc;
      frLayerNum lNum = gridGraph_.getLayerNum(startZ);
      gridGraph_.getPoint(startX, startY, startLoc);
      gridGraph_.getPoint(endX, endY, endLoc);
      auto uPathSeg = make_unique<grPathSeg>();
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
      unique_ptr<grConnFig> uConnFig(std::move(uPathSeg));
      net->addRouteConnFig(uConnFig);

    } else if (startX == endX && startY == endY && startZ != endZ) {
      // via(s)
      bool isChildBP = (startZ == points[i].z());
      // currZ is the bottom enclosure layer
      if (isChildBP) {
        // currZ is child node idx and parent idx is currZ + 1
        for (auto currZ = startZ; currZ < endZ; currZ++) {
          frPoint loc;
          gridGraph_.getPoint(startX, startY, loc);
          grNode* tmpParent = nullptr;
          if (currZ == endZ - 1) {
            tmpParent = parent;
          } else {
            // create temporary parent node
            auto uNode = make_unique<grNode>();
            tmpParent = uNode.get();
            tmpParent->addToNet(net);
            tmpParent->setLoc(loc);
            tmpParent->setLayerNum(((currZ + 1) + 1) * 2);
            tmpParent->setType(frNodeTypeEnum::frcSteiner);

            net->addNode(uNode);

            // add to endPoint map
            mazeIdx2endPointNode[FlexMazeIdx(startX, startY, currZ + 1)] = tmpParent;
          }
          // create via
          auto uVia = make_unique<grVia>();
          auto via = uVia.get();
          via->setChild(child);
          via->setParent(tmpParent);
          via->addToNet(net);
          via->setOrigin(loc);
          via->setViaDef(getDesign()->getTech()->getLayer((currZ + 1) * 2 + 1)->getDefaultViaDef());

          // update connectivity
          child->setConnFig(via);
          child->setParent(tmpParent);
          tmpParent->addChild(child);

          // update rq
          workerRegionQuery.add(via);

          // update ownership
          unique_ptr<grConnFig> uConnFig(std::move(uVia));
          net->addRouteConnFig(uConnFig);

          child = tmpParent;
        }
      } else {
        // currZ is child idx and parent idx is currZ - 1
        for (auto currZ = endZ; currZ > startZ; currZ--) {
          frPoint loc;
          gridGraph_.getPoint(startX, startY, loc);
          grNode* tmpParent = nullptr;
          if (currZ == startZ + 1) {
            tmpParent = parent;
          } else {
            // create temporary parent node
            auto uNode = make_unique<grNode>();
            tmpParent = uNode.get();
            tmpParent->addToNet(net);
            tmpParent->setLoc(loc);
            tmpParent->setLayerNum(((currZ - 1) + 1) * 2);
            tmpParent->setType(frNodeTypeEnum::frcSteiner);

            net->addNode(uNode);

            // add to endPoint map
            mazeIdx2endPointNode[FlexMazeIdx(startX, startY, currZ - 1)] = tmpParent;
          }
          // create via
          auto uVia = make_unique<grVia>();
          auto via = uVia.get();
          via->setChild(child);
          via->setParent(tmpParent);
          via->addToNet(net);
          via->setOrigin(loc);
          via->setViaDef(getDesign()->getTech()->getLayer(currZ * 2 + 1)->getDefaultViaDef());

          // update connectivty
          child->setConnFig(via);
          child->setParent(tmpParent);
          tmpParent->addChild(child);

          // update rq
          workerRegionQuery.add(via);

          // update ownership
          unique_ptr<grConnFig> uConnFig(std::move(uVia));
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

  if (enableOutput) {
    cout << "  done routeNet_postAstarWritePath\n";
  }
}

grNode* FlexGRWorker::routeNet_postAstarWritePath_splitPathSeg(grNode* child, grNode* parent, const frPoint &breakPt) {
  auto &workerRegionQuery = getWorkerRegionQuery();

  auto pathSeg = static_cast<grPathSeg*>(child->getConnFig());
  // remove from rq first
  workerRegionQuery.remove(pathSeg);

  auto net = pathSeg->getGrNet();
  auto lNum = pathSeg->getLayerNum();
  frPoint bp, ep;
  pathSeg->getPoints(bp, ep);
  frPoint childLoc = child->getLoc();
  bool isChildBP = (childLoc == bp);

  auto uBreakNode = make_unique<grNode>();
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


  // shrink child - parent pathSeg to child - breakPt (modified from initBoundary_splitPathSeg_split)
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
  auto uPathSeg2 = make_unique<grPathSeg>();
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

  // update net ownershiop (no need to touch pathSeg1 because it is shrinked from original one)
  unique_ptr<grConnFig> uConnFig(std::move(uPathSeg2));
  net->addRouteConnFig(uConnFig);

  return breakNode;
}

void FlexGRWorker::routeNet_postRouteAddCong(grNet* net) {
  for (auto &uptr: net->getRouteConnFigs()) {
    if (uptr->typeId() == grcPathSeg) {
      auto cptr = static_cast<grPathSeg*>(uptr.get());
      modCong_pathSeg(cptr, /*isAdd*/true);
    }
  }
}

void FlexGRWorker::route_decayHistCost() {
  frMIdx xDim = 0;
  frMIdx yDim = 0;
  frMIdx zDim = 0;

  gridGraph_.getDim(xDim, yDim, zDim);

  vector<grConnFig*> result;

  for (int zIdx = 0; zIdx < zDim; zIdx++) {
    for (int xIdx = 0; xIdx <xDim; xIdx++) {
      for (int yIdx = 0; yIdx < yDim; yIdx++) {
        // decay history cost
        gridGraph_.decayHistoryCost(xIdx, yIdx, zIdx, 0.8);
      }
    }
  }
}
