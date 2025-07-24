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


/*
void FlexGRWorker::batchGenerationRelax(
  std::vector<grNet*>& rerouteNets,
  std::vector<std::vector<grNet*>>& batches)
{
  batches.clear();
  batches.reserve(rerouteNets.size());
  
  // Use mask to track the occupied gcells for each batch to detect conflicts
  // batchMask[i] is a 2D vector with the same size as the gcell grid of size (xGrids_ x yGrids_)
  std::vector<std::vector<bool> > batchMask; 
  batchMask.reserve(rerouteNets.size());

  int xDim, yDim, zDim;
  gridGraph_.getDim(xDim, yDim, zDim);

  // logger_->report("[INFO][FlexGR] Number of effective nets for batch generation: {}\n", rerouteNets.size());
  // logger_->report("[INFO][FlexGR] Grid Graph size: {} x {}\n", xDim, yDim);

  // Define the lambda function to get the idx for each gcell
  // We use the row-major order to index the gcells
  auto getGCellIdx1D = [xDim](int x, int y) {
    return y * xDim + x;
  };


  std::sort(rerouteNets.begin(), rerouteNets.end(), 
    [](const grNet* a, const grNet* b) {
      return a->getHPWL() > b->getHPWL();
    });


  std::vector<NetStruct> netTrees;
  netTrees.reserve(rerouteNets.size());

  for (auto& net : rerouteNets) {
    NetStruct netTree;
    netTree.netId = static_cast<int>(netTrees.size());
    auto& points = netTree.points;
    auto& vSegments = netTree.vSegments;
    auto& hSegments = netTree.hSegments;

    grNode* root = nullptr;
    for (auto& [pinNode, gcellNode] : net->getPinGCellNodePairs()) {
      if (root == nullptr) {
        root = gcellNode;
        break;
      }
    }
  
    if (root == nullptr) {
      std::cout << "Error: root is nullptr\n";
    }
  
    std::queue<grNode*> nodeQ;
    //auto root = net->getRoot();
    nodeQ.push(root);

    while (!nodeQ.empty()) {
      auto node = nodeQ.front();
      nodeQ.pop();
      if (node->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }
      
      auto Loc = node->getLoc();
      auto lNum = node->getLayerNum();
      FlexMazeIdx mi;
      gridGraph_.getMazeIdx(Loc, lNum, mi);
      int nodeLocIdx = getGCellIdx1D(mi.x(), mi.y());
      points.push_back(nodeLocIdx);

      for (auto& child : node->getChildren()) {
        if (child->getType() != frNodeTypeEnum::frcSteiner) {
          continue;
        }
        
        nodeQ.push(child);
        auto childLoc = child->getLoc();
        auto childLNum = child->getLayerNum();
        FlexMazeIdx childMi;
        gridGraph_.getMazeIdx(childLoc, childLNum, childMi);
        int childLocIdx1D = getGCellIdx1D(childMi.x(), childMi.y());
        if (childMi.x() == mi.x()) {
          nodeLocIdx > childLocIdx1D 
            ? vSegments.push_back(std::make_pair(childLocIdx1D, nodeLocIdx)) 
            : vSegments.push_back(std::make_pair(nodeLocIdx, childLocIdx1D));
        } else if (childMi.y() == mi.y()) {
          nodeLocIdx > childLocIdx1D 
            ? hSegments.push_back(std::make_pair(childLocIdx1D, nodeLocIdx)) 
            : hSegments.push_back(std::make_pair(nodeLocIdx, childLocIdx1D));
        } else {
          logger_->error(DRT, 260, "current node and parent node are are not aligned collinearly\n");
        }
      }
    }

    netTrees.push_back(netTree);
  }

  // Define the lambda function to check if the net is in some batch
  // Here we use the representative point exhaustion, for non-exact overlap checking.
  // Only checks the two end points of a query segment
  // The checking may fail is the segment is too long 
  // and the two end points cover all the existing segments
  auto hasConflict = [&](std::vector<std::vector<bool> >::iterator maskIter, int netId) -> bool {
    for (auto& point : netTrees[netId].points) {
      if ((*maskIter)[point]) {
        return true;
      }
    }
    return false;
  };

  auto findBatch = [&](int netId) -> int {
    std::vector<std::vector<bool> >::iterator maskIter = batchMask.begin();
    while (maskIter != batchMask.end()) {
      if (!hasConflict(maskIter, netId)) {
        return std::distance(batchMask.begin(), maskIter);
      }
      maskIter++;
    }
    return -1; 
  };    
    
  auto maskExactRegion = [&](int netId, std::vector<bool>& mask) {    
    for (auto& vSeg : netTrees[netId].vSegments) {
      for (int id = vSeg.first; id <= vSeg.second; id += xDim) {
        mask[id] = true;
      }
    }

    for (auto& hSeg : netTrees[netId].hSegments) {
      for (int id = hSeg.first; id <= hSeg.second; id++) {
        mask[id] = true;
      }
    }
  };

  int numGrids = xDim * yDim;
  for (int netId = 0; netId < rerouteNets.size(); netId++) {
    int batchId = findBatch(netId);  
    // always create a new batch if no batch is found
    // for testing purpose
    // batchId = -1;
    if (batchId == -1 || batches[batchId].size() >= numGrids) {
      batchId = batches.size();
      batches.push_back(std::vector<grNet*>());
      batchMask.push_back(std::vector<bool>(static_cast<size_t>(numGrids), false));
    }  

    batches[batchId].push_back(rerouteNets[netId]);
    maskExactRegion(netId, batchMask[batchId]);      

    //if (netId % 100000 == 1) {
    //  std::cout << "Processed " << netId << " nets" << std::endl;
    //  std::cout << "Current batch size: " << batches.size() << std::endl;
    //}
  }

  // Two round of batch matching
  // logger_->report("[INFO][FlexGR] Number of batches: {}\n", batches.size());
  // print the basic statistics
  int sparseBatch = 0;
  for (size_t i = 0; i < batches.size(); i++) {
    if (batches[i].size() < 40) {
      sparseBatch++;
      continue;
    }
    
    //logger_->report("[INFO][FlexGR] Batch {} has {} nets", i, batches[i].size());
  }

  
  // for (auto& netTree : netTrees) {
  //  std::cout << "netTree.points.size() = " << netTree.points.size() << "  "
  //            << "netTree.vSegments.size() = " << netTree.vSegments.size() << "  "
  //            << "netTree.hSegments.size() = " << netTree.hSegments.size() << std::endl;
  //}

  // logger_->report("[INFO][FlexGR] Number of sparse batches (#nets < 40): {}", sparseBatch);
  // logger_->report("[INFO][FlexGR] Number of dense batches (#nets >= 40): {}", batches.size() - sparseBatch);
  // logger_->report("[INFO][FlexGR] Done batch generation...\n");

  //exit(1);
}
*/


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
    // std::cout << "Trace back: x = " << mi.x() << " y = " << mi.y() << " z = " << mi.z() << " "
    //           << "parentX = " << parentMi.x() << " parentY = " << parentMi.y() << " parentZ = " << parentMi.z() << " "
    //           << std::endl;
    
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
     
      if (VERBOSE > 0) {      
        std::cout << "Exit at src x = " << mi.x() << " y = " << mi.y() << " z = " << mi.z() << std::endl;
      }
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


void FlexGRWorker::printSrc()
{
  int xDim, yDim, zDim;
  gridGraph_.getDim(xDim, yDim, zDim);
  for (int z = 0; z < zDim; z++) {
    for (int y = 0; y < yDim; y++) {
      for (int x = 0; x < xDim; x++) {
        if (gridGraph_.isSrc(x, y, z)) {
          std::cout << "x = " << x << " y = " << y << " z = " << z << std::endl;
        }
      }
    }
  }
}


bool FlexGRWorker::restoreNet(grNet* net)
{
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

  if (VERBOSE > 0) {
                  
    printSrc();
    int llx = std::numeric_limits<int>::max();
    int lly = std::numeric_limits<int>::max();
    int urx = std::numeric_limits<int>::min();
    int ury = std::numeric_limits<int>::min();


    for (auto& idx : connComps) {
      std::cout << "connComps: x = " << idx.x() << " y = " << idx.y() << " z = " << idx.z() << std::endl;
      llx = std::min(llx, idx.x());
      lly = std::min(lly, idx.y());
      urx = std::max(urx, idx.x());
      ury = std::max(ury, idx.y());
    } 
    
    for (auto& [idx, node] : mazeIdx2unConnPinGCellNode) {
      std::cout << "unConnPinGCellNode: x = " << idx.x() << " y = " << idx.y() << " z = " << idx.z() << std::endl;
      llx = std::min(llx, idx.x());
      lly = std::min(lly, idx.y());
      urx = std::max(urx, idx.x());
      ury = std::max(ury, idx.y());
    }

    for (int x = llx; x <= urx; x++) {
      for (int y = lly; y <= ury; y++) {
        FlexMazeIdx idx(x, y, 0);
        // get the parent
        FlexMazeIdx parentIdx;
        gridGraph_.traceBackParent(idx, parentIdx);
        if (parentIdx.x() != -1 && parentIdx.y() != -1) {
          std::cout << "x = " << x << " y = " << y << " parentX = " << parentIdx.x() << " parentY = " << parentIdx.y() << std::endl;
        }
      }
    }
  }


  std::vector<FlexMazeIdx> path;  // astar must return with more than one idx

  //if (VERBOSE > 0) {
  //  std::cout << "Before restorNet" << std::endl;
  //  routeNet_printNet(net);
  //}

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
      auto bbox = net->getRouteBBox();
      std::cout << "netId = " << net->getNetId() << std::endl;
      std::cout << "BBox lx = " << bbox.xMin() << " " 
                << "ly = " << bbox.yMin() << " "
                << "ux = " << bbox.xMax() << " "
                << "uy = " << bbox.yMax() << std::endl;
      
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
    
   // printSrc();
    //routeNet_printNet(net);
  }

  // routeNet_checkNet(net);
  routeNet_postRouteAddCong(net);

  // net->setPostCost(calcPathCost(net));
  if (net->getPinGCellNodes().size() <= 0) {
    if (net->getPostCost() != net->getPreCost()) {
      std::cout << "Net " << net->getFrNet()->getName() << " ";
      if (net->getPostCost() < net->getPreCost()) {
        std::cout << " Improved: PostCost = " << net->getPostCost() << " < PreCost = " << net->getPreCost() << std::endl;
      } else {
        std::cout << " Deteriorated: PostCost = " << net->getPostCost() << " > PreCost = " << net->getPreCost() << std::endl;
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

  
  routeNet_checkNet(net);

  return true;
}


}