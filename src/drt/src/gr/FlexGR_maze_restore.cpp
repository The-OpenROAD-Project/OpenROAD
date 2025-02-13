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

// To be updated
bool FlexGRWorker::restorePath(
  std::vector<FlexMazeIdx>& connComps,
  grNode* nextPinNode,
  std::vector<FlexMazeIdx>& path,
  FlexMazeIdx& ccMazeIdx1,
  FlexMazeIdx& ccMazeIdx2,
  const Point& centerPt) 
{
  
  
  
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

  while (!unConnPinGCellNodes.empty()) {
    path.clear();
    auto nextPinGCellNode = routeNet_getNextDst(
        ccMazeIdx1, ccMazeIdx2, mazeIdx2unConnPinGCellNode);
    if (restorePath(connComps, nextPinGCellNode, path, 
        ccMazeIdx1, ccMazeIdx2, centerPt)) {
      auto leaf = routeNet_postAstarUpdate(
          path, connComps, unConnPinGCellNodes, mazeIdx2unConnPinGCellNode);
      routeNet_postAstarWritePath(net, path, leaf, mazeIdx2endPointNode);
    } else {
      logger_->report("Error: restorePath failed !!!");
      return false;
    }
  }

  routeNet_postRouteAddCong(net);
  return true;
}


}