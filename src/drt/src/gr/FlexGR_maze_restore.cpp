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

#include <vector>
#include "gr/FlexGR.h"

namespace drt {

static const int VERBOSE = 0;

// To be updated
bool FlexGRWorker::restorePath(
  std::vector<FlexMazeIdx>& connComps,
  grNode* nextPinNode,
  std::vector<FlexMazeIdx>& path,
  FlexMazeIdx& ccMazeIdx1,
  FlexMazeIdx& ccMazeIdx2,
  const Point& centerPt)  // Actually, this is not used
{
  FlexMazeIdx mi;
  auto loc = nextPinNode->getLoc();
  auto lNum = nextPinNode->getLayerNum();
  gridGraph_.getMazeIdx(loc, lNum, mi);

  // check if the nextPinNode has been connected
  for (auto& idx : connComps) {
    if (gridGraph_.isDst(idx.x(), idx.y(), idx.z())) {
      path.emplace_back(idx.x(), idx.y(), idx.z());
      return true;
    }
  }

  // trace back from the nextPinNode mi  
  // Test flag  
  // To be updated later
  int maxLength = 10000; // This is should be related to HPWL
  int length = 0;
  // path.emplace_back(mi.x(), mi.y(), mi.z());
  // std::cout << "Dst:  x = " << mi.x() << " y = " << mi.y() << " z = " << mi.z() << " "
  //          << "isDst = " << gridGraph_.isDst(mi.x(), mi.y(), mi.z()) << std::endl;

  frDirEnum prevDir = frDirEnum::UNKNOWN;
  FlexMazeIdx parentMi = mi;
  frDirEnum currDir = frDirEnum::UNKNOWN;

  while (true) {
    currDir = gridGraph_.traceBackParent(mi, parentMi); 
    if (parentMi.x() == -1 || parentMi.y() == -1 || parentMi.z() == -1) {
      logger_->report("[ERROR] restorePath: traceBackParent failed");
      return false;
    }

    bool flag = gridGraph_.isSrc(mi.x(), mi.y(), mi.z()) || gridGraph_.isDst(mi.x(), mi.y(), mi.z()) 
      || (prevDir != frDirEnum::UNKNOWN && currDir != prevDir); 
    if (flag) {
      path.emplace_back(mi.x(), mi.y(), mi.z());
    }

    prevDir = currDir;
    mi = parentMi;

    // path.emplace_back(mi.x(), mi.y(), mi.z());      
    if (gridGraph_.isSrc(mi.x(), mi.y(), mi.z())) {
      if (path.empty() || !(path.back() == mi)) {
        path.emplace_back(mi.x(), mi.y(), mi.z());
      }
      
      // std::cout << "Exit at src x = " << mi.x() << " y = " << mi.y() << " z = " << mi.z() << std::endl;
      // check if the mi is the path
      // if (path.empty() || !(path.back() == mi)) {
      //  path.emplace_back(mi.x(), mi.y(), mi.z());
      // }
      // path.emplace_back(mi.x(), mi.y(), mi.z());
      break;
    }

    // Test flag
    length++;
    if (length > maxLength) {
      logger_->report("[ERROR] restorePath: maxLength reached");
      return false;
    }        
  }  

  for (auto& mi : path) {
    ccMazeIdx1.set(std::min(ccMazeIdx1.x(), mi.x()),
                   std::min(ccMazeIdx1.y(), mi.y()),
                   std::min(ccMazeIdx1.z(), mi.z()));
    ccMazeIdx2.set(std::max(ccMazeIdx2.x(), mi.x()),
                   std::max(ccMazeIdx2.y(), mi.y()),
                   std::max(ccMazeIdx2.z(), mi.z()));
  }

  return true;
}


bool FlexGRWorker::restoreNet(grNet* net)
{
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

  if (VERBOSE > 0) {
    std::cout << "Before restorNet" << std::endl;
    routeNet_printNet(net);
  }

  while (!unConnPinGCellNodes.empty()) {    
    path.clear();
    auto nextPinGCellNode = routeNet_getNextDst(
        ccMazeIdx1, ccMazeIdx2, mazeIdx2unConnPinGCellNode);
    bool restoreFlag = restorePath(connComps, nextPinGCellNode, path, 
        ccMazeIdx1, ccMazeIdx2, centerPt);
    
    if (VERBOSE > 0) {
      // for test
      std::cout << "restoreFlag = " << restoreFlag << std::endl;
      for (auto& vertex : path) {
        std::cout << "\t x = " << vertex.x() << " y = " << vertex.y() << " z = " << vertex.z() << std::endl;
      }
      std::cout << "pathEnd" << std::endl;    
    }
    
    if (restoreFlag == true) {
      auto leaf = routeNet_postAstarUpdate(
          path, connComps, unConnPinGCellNodes, mazeIdx2unConnPinGCellNode);
      // std::cout << "finish postAStarUpdate" << std::endl;
      routeNet_postAstarWritePath(net, path, leaf, mazeIdx2endPointNode);
    } else {
      logger_->report("Error: restorePath failed !!!");
      exit(1);
      if (gridGraph_.search(connComps,
                            nextPinGCellNode,
                            path,
                            ccMazeIdx1,
                            ccMazeIdx2,
                            centerPt)) {
        auto leaf = routeNet_postAstarUpdate(
          path, connComps, unConnPinGCellNodes, mazeIdx2unConnPinGCellNode);
        routeNet_postAstarWritePath(net, path, leaf, mazeIdx2endPointNode);
      }    
    }
    //routeNet_printNet(net);
  }

  //routeNet_checkNet(net);
  routeNet_postRouteAddCong(net);
  return true;
}


}