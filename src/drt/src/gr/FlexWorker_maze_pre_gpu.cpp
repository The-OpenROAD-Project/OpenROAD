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

#include <vector>

#include "gr/FlexGR.h"

namespace drt {


void main_mt_prep(std::vector<grNet*>& rerouteNets, int iter) {
  // For incremental RRR (DRC mode), we need to update the history at the beginning of each iteration
  route_addHistCost_update();
  routePrep_update(rerouteNets, iter);
  for (auto net : rerouteNets) {
    net->setPreCost(calcPathCost(net)); // This is for early exit
  }
}


// Setup the history cost for the nets 
void FlexGRWorker::route_addHistCost_update()
{
  if (ripupMode_ == RipUpMode::DRC) {
    route_addHistCost();
  }
}


void FlexGRWorker::route_addHistCost()
{
  frMIdx xDim = 0;
  frMIdx yDim = 0;
  frMIdx zDim = 0;
  gridGraph_.getDim(xDim, yDim, zDim); 
  for (int zIdx = 0; zIdx < zDim; zIdx++) {
    for (int xIdx = 0; xIdx < xDim; xIdx++) {
      for (int yIdx = 0; yIdx < yDim; yIdx++) {
        const int rawDemandE = gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::E);
        const int rawDemandN = gridGraph_.getRawDemand(xIdx, yIdx, zIdx, frDirEnum::N);
        const int rawSupplyE = gridGraph_.getRawSupply(xIdx, yIdx, zIdx, frDirEnum::E) * congThresh_;
        const int rawSupplyN = gridGraph_.getRawSupply(xIdx, yIdx, zIdx, frDirEnum::N) * congThresh_;
        if (rawDemandE > rawSupplyE || rawDemandN > rawSupplyN) {
          // has congestion, need to add history cost
          int overflowH = std::max(0, rawDemandE - rawSupplyE);
          int overflowV = std::max(0, rawDemandN - rawSupplyN);
          gridGraph_.addHistoryCost(
              xIdx, yIdx, zIdx, (overflowH + overflowV) / 2 + 1);
        }
      }
    }
  }
}


void FlexGRWorker::routePrep_update(
  std::vector<grNet*> &rerouteNets, 
  int iter)
{
  if (iter == 0) {
    init_pinGCellIdxs();
  }  
   
  route_mazeIterInit();
  std::vector<grNet*> nets;
  route_getRerouteNets(nets);

  for (auto net : nets) {
    if (ripupMode_ == RipUpMode::DRC || 
        (ripupMode_ == RipUpMode::ALL && mazeNetHasCong(net))) {
      rerouteNets.push_back(net);  
    }
  }
}



float FlexGRWorker::calcPathCost(grNet* net)
{
  // Traverse all the metal segments in the net
  float pathCost = 0;
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
        // vert
        for (auto yIdx = bi.y(); yIdx < ei.y(); yIdx++) {
          pathCost += gridGraph_.getGridCost(bi.x(), yIdx, bi.z(), dir);
        }  
          
        if (bi.y() == ei.y()) {
          std::cout << "Single point pathseg" << std::endl;
        }
        
      } else {
        // horz
        for (auto xIdx = bi.x(); xIdx < ei.x(); xIdx++) {
          pathCost += gridGraph_.getGridCost(xIdx, bi.y(), bi.z(), dir);   
        }

        if (bi.x() == ei.x()) {
          std::cout << "Single point pathseg" << std::endl;
        }
      }
    }
  }

  return pathCost;  
}


void FlexGRWorker::init_pinGCellIdxs()
{
  for (auto &net : nets_) {
    net->clearPinGCellIdxs();
    for (auto pinGCellNode : net->getPinGCellNodes()) {
      auto loc = pinGCellNode->getLoc();
      auto lNum = pinGCellNode->getLayerNum();
      FlexMazeIdx mi;
      gridGraph_.getMazeIdx(loc, lNum, mi);
      net->addPinGCellIdx(mi);
    }

    // Check if the net is a feedthrough net
    const std::vector<FlexMazeIdx>& pinGCellIdxs = net->getPinGCellIdxs();
    if (pinGCellIdxs.size() == 2 && (pinGCellIdxs[0].x() == pinGCellIdxs[1].x() || pinGCellIdxs[0].y() == pinGCellIdxs[1].y())) {
      net->setFeedThrough(true);
    } else {
      net->setFeedThrough(false);
    }

    // Set the bounding box
    FlexMazeIdx ll(std::numeric_limits<frMIdx>::max(), std::numeric_limits<frMIdx>::max(), std::numeric_limits<frMIdx>::max());
    FlexMazeIdx ur(std::numeric_limits<frMIdx>::min(), std::numeric_limits<frMIdx>::min(), std::numeric_limits<frMIdx>::min());
    for (auto &idx : pinGCellIdxs) {
      ll.set(std::min(ll.x(), idx.x()), std::min(ll.y(), idx.y()), std::min(ll.z(), idx.z()));
      ur.set(std::max(ur.x(), idx.x()), std::max(ur.y(), idx.y()), std::max(ur.z(), idx.z()));
    }
   
   
    
    /*
    int xDim, yDim, zDim;
    gridGraph_.getDim(xDim, yDim, zDim); 
    int xDim_global, yDim_global, zDim_global;
    this->getCMap()->getDim(xDim_global, yDim_global, zDim_global);
    xDim = std::min(xDim, xDim_global);
    yDim = std::min(yDim, yDim_global);
    */
    //ur.set(std::min(xDim - 1, ur.x() + delta), std::min(yDim - 1, ur.y() + delta), ur.z());        
    //net->setRouteBBox(Rect(0, 0, xDim - 1, yDim - 1));
    auto LL = getRouteGCellIdxLL();
    auto UR = getRouteGCellIdxUR();

    int xDim = UR.x() - LL.x() + 1;
    int yDim = UR.y() - LL.y() + 1;
    
    if (1) {
      int deltaX = std::min(ur.x() - ll.x(), 10);
      int deltaY = std::min(ur.y() - ll.y(), 10);
      int delta = std::max(deltaX, deltaY);
      ll.set(std::max(0, ll.x() - delta), std::max(0, ll.y() - delta), ll.z());
      ur.set(std::min(xDim - 1, ur.x() + delta), std::min(yDim - 1, ur.y() + delta), ur.z());     
    }

    //net->setRouteBBox(Rect(0, 0, xDim - 1, yDim - 1));    
    //auto netRouteBBox = net->getRouteBBox();
    // std::cout << "[Test] xDim = " << xDim << ", yDim = " << yDim << std::endl;
    net->setRouteBBox(Rect(ll.x(), ll.y(), ur.x(), ur.y())); 
    // Set the routed wirelength as HPWL
    int hpwl = (ur.x() - ll.x()) + (ur.y() - ll.y());
    net->setHPWL(hpwl);
    net->setCPUFlag(true);
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
            Point gcellCenter;
            gridGraph_.getPoint(xIdx, yIdx, gcellCenter);
            Rect gcellCenterBox(gcellCenter, gcellCenter);
            result.clear();
            workerRegionQuery.query(gcellCenterBox, lNum, result);
            for (auto rptr : result) {
              if (rptr->typeId() == grcPathSeg) {
                auto cptr = static_cast<grPathSeg*>(rptr);
                if (cptr->hasGrNet()) {
                  cptr->getGrNet()->setRipup(true);
                } else {
                  std::cout << "Error: route_mazeIterInit hasGrNet() empty"
                            << std::endl;
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



}  // namespace drt
