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

#include "FlexGR.h"

#include <omp.h>

#include <cmath>
#include <fstream>
#include <iostream>

#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include "db/infra/frTime.h"
#include "db/obj/frGuide.h"
#include "odb/db.h"
#include "utl/exception.h"

namespace drt {

using utl::ThreadException;


bool overlap(const Rect &r1, const Rect &r2)
{
  return !(r1.xMax() < r2.xMin() || r1.xMin() > r2.xMax() || r1.yMax() < r2.yMin() || r1.yMin() > r2.yMax());
}


// Jus for reference
/*
void FlexGR::layerAssign_batchGeneration_update(
  std::vector<std::tuple<frNet*, int, int> >& sortedNets, 
  std::vector<std::vector<int> >& batchNets)
{
  batchNets.clear();
  batchNets.reserve(sortedNets.size() * 100);
  // Use mask to track the occupied gcells for each batch to detect conflicts
  // batchMask[i] is a 2D vector with the same size as the gcell grid of size (xGrids_ x yGrids_)
  std::vector<std::vector<bool> > batchMask; 
  batchMask.reserve(sortedNets.size() * 100);

  auto gCellPatterns = getDesign()->getTopBlock()->getGCellPatterns();
  auto& xgp = gCellPatterns.at(0);
  auto& ygp = gCellPatterns.at(1);
  int gridXSize = xgp.getCount();
  int gridYSize = ygp.getCount();

  logger_->report("[INFO][FlexGR] Number of effective nets for batch generation: {}\n", sortedNets.size());
  logger_->report("[INFO][FlexGR] Grid size: {} x {}\n", gridXSize, gridYSize);

  // Define the lambda function to get the idx for each gcell
  // We use the row-major order to index the gcells
  auto getGCellIdx1D = [gridXSize](int x, int y) {
    return x * gridXSize + y;
  };


  std::vector<NetStruct> netTrees;
  netTrees.reserve(sortedNets.size());

  for (auto& netRatio : sortedNets) {
    NetStruct netTree;
    netTree.netId = static_cast<int>(netTrees.size());
    auto& points = netTree.points;
    auto& vSegments = netTree.vSegments;
    auto& hSegments = netTree.hSegments;

    std::queue<frNode*> queue;
    auto root = std::get<0>(netRatio)->getRootGCellNode();
    queue.push(root);

    while (!queue.empty()) {
      auto node = queue.front();
      queue.pop();
      if (node->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }

      Point locIdx = design_->getTopBlock()->getGCellIdx(node->getLoc());
      int nodeLocIdx = getGCellIdx1D(locIdx.x(), locIdx.y());
      points.push_back(nodeLocIdx);

      for (auto& child : node->getChildren()) {
        queue.push(child);
        Point childLocIdx = design_->getTopBlock()->getGCellIdx(child->getLoc());
        int childLocIdx1D = getGCellIdx1D(childLocIdx.x(), childLocIdx.y());
        if (childLocIdx.x() == locIdx.x()) {
          nodeLocIdx > childLocIdx1D 
            ? vSegments.push_back(std::make_pair(childLocIdx1D, nodeLocIdx)) 
            : vSegments.push_back(std::make_pair(nodeLocIdx, childLocIdx1D));
        } else if (childLocIdx.y() == locIdx.y()) {
          nodeLocIdx > childLocIdx1D 
            ? hSegments.push_back(std::make_pair(childLocIdx1D, nodeLocIdx)) 
            : hSegments.push_back(std::make_pair(nodeLocIdx, childLocIdx1D));
        } else {
          logger_->error(DRT, 264, "current node and parent node are are not aligned collinearly\n");
        } 
      }
    }

    netTrees.push_back(netTree);
  }


  int batchCntStart = 0;
  // Define the lambda function to check if the net is in some batch
  // Here we use the representative point exhaustion, for non-exact overlap checking.
  // Only checks the two end points of a query segment
  // The checking may fail is the segment is too long 
  // and the two end points cover all the existing segments
  auto findBatch = [&](int netId) -> int {
    std::vector<std::vector<bool> >::iterator maskIter = batchMask.begin();
    maskIter += batchCntStart;
    while (maskIter != batchMask.end()) {
      for (auto& point : netTrees[netId].points) {
        if ((*maskIter)[point]) {
          return std::distance(batchMask.begin(), maskIter);
        }
      }
      maskIter++;
    }
    return -1; 
  };    
    
  auto maskExactRegion = [&](int netId, std::vector<bool>& mask) {    
    for (auto& vSeg : netTrees[netId].vSegments) {
      for (int id = vSeg.first; id <= vSeg.second; id += gridXSize) {
        mask[id] = true;
      }
    }

    for (auto& hSeg : netTrees[netId].hSegments) {
      for (int id = hSeg.first; id <= hSeg.second; id++) {
        mask[id] = true;
      }
    }
  };

  for (int netId = 0; netId < sortedNets.size(); netId++) {
    int batchId = findBatch(netId);  
    if (batchId == -1 || batchNets[batchId].size() >= xDim_ * yDim_) {
      batchId = batchNets.size();
      batchNets.push_back(std::vector<int>());
      batchMask.push_back(std::vector<bool>(static_cast<size_t>(gridXSize * gridYSize), false));
    }  

    batchNets[batchId].push_back(netId);  
    maskExactRegion(netId, batchMask[batchId]);      

    if (netId % 100000 == 1) {
      std::cout << "Processed " << netId << " nets" << std::endl;
      std::cout << "Current batch size: " << batchNets.size() << std::endl;
    }
  }

  // Two round of batch matching
  logger_->report("[INFO][FlexGR] Number of batches: {}\n", batchNets.size());
  // print the basic statistics
  int sparseBatch = 0;
  for (size_t i = 0; i < batchNets.size(); i++) {
    if (batchNets[i].size() < 40) {
      sparseBatch++;
      continue;
    }
    
    logger_->report("[INFO][FlexGR] Batch {} has {} nets", i, batchNets[i].size());
  }

  logger_->report("[INFO][FlexGR] Number of sparse batches (#nets < 40): {}", sparseBatch);
  logger_->report("[INFO][FlexGR] Number of dense batches (#nets >= 40): {}", batchNets.size() - sparseBatch);
  logger_->report("[INFO][FlexGR] Done batch generation...\n");
}
*/



// Batch Generation Related Functions
// Version 1:
// Greedy maximal independent set (MIS) based batch generation
void FlexGR::batchGenerationMIS_update(
  std::vector<grNet*> &rerouteNets,
  std::vector<std::vector<grNet*>> &batches,
  std::vector<int> &validBatchIds,
  int iter,
  bool is2DRouting)
{      
  // Measure the runtime of std::sort.
  auto start_time = std::chrono::high_resolution_clock::now();

  for (auto& batch : batches) {
    batch.clear();
  }
  batches.clear();

  if (rerouteNets.empty()) {
    return;
  }

  // Implement the batch generation algorithm
  std::vector<int> batchStartPtr;
  // Sort all the nets in the non-increasing order of HPWL
  std::sort(rerouteNets.begin(), rerouteNets.end(), 
    [](const grNet* a, const grNet* b) {
      return a->getHPWL() > b->getHPWL();
    });  
  

  for (auto net : rerouteNets) {
    bool found = false;
    for (int batchId = 0; batchId < batches.size(); batchId++) {
      bool conflict = false;
      for (auto& tempNet : batches[batchId]) {
        if (tempNet->getWorkerId() != net->getWorkerId()) {
          continue;
        }
          
        if (overlap(net->getRouteBBox(), tempNet->getRouteBBox())) {
          conflict = true;
          break;
        }
      }
      
      if (!conflict) {
        batches[batchId].push_back(net);
        found = true;
        break;
      } 
    }
    
    if (!found) {
      batches.push_back({net});
    }
  }

  // Check the valid batch threshold
  validBatchIds.clear();
  std::vector<int> sortedBatchIds(batches.size());
  std::iota(sortedBatchIds.begin(), sortedBatchIds.end(), 0);
  std::sort(sortedBatchIds.begin(), sortedBatchIds.end(), 
    [&batches](const int a, const int b) {
      return batches[a].size() > batches[b].size();
    });
  
  int gpuNets = 0;
  for (int i = 0; i < sortedBatchIds.size(); i++) {
    int batchId = sortedBatchIds[i];
    if (validBatchIds.size() >= maxChunkSize_) {
      break;
    }
    
    validBatchIds.push_back(batchId);
    if (is2DRouting) {
      for (auto net : batches[batchId]) {
        net->setCPUFlag(false);
        net->setBatchId(i);
      }
      gpuNets += batches[batchId].size();
    }
  }

  //if (VERBOSE > 0) {
  if (1) {
    // Report the batch information
    logger_->report("[INFO] Number of batches: " + std::to_string(batches.size()));
    logger_->report("\t[INFO] Batch information:");
    for (int i = 0; i < validBatchIds.size(); i++) {
      logger_->report("\t[INFO] Valid Batch " + std::to_string(validBatchIds[i]) + ": " + std::to_string(batches[validBatchIds[i]].size()) + " nets");
    } 

    logger_->report("\t[INFO] Report all the batches:");
    for (int i = 0; i < batches.size(); i++) {
      logger_->report("\t[INFO] Batch " + std::to_string(i) + ": " + std::to_string(batches[i].size()) + " nets");
    }

    int totalNumNets = rerouteNets.size();
    int cpuNets = totalNumNets - gpuNets;
    logger_->report("\t[INFO] Number of GPU nets: " + std::to_string(gpuNets) + " (" + std::to_string(gpuNets * 100.0 / (cpuNets + gpuNets)) + "%)");
    logger_->report("\t[INFO] Number of CPU nets: " + std::to_string(cpuNets) + " (" + std::to_string(cpuNets * 100.0 / (cpuNets + gpuNets)) + "%)");

    // Calculate elapsed time in milliseconds.
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end_time - start_time;
    logger_->report("\t[INFO] Batch generation runtime: " + std::to_string(elapsed.count() * 1e-3) + " s");
  }
}


void FlexGR::searchRepair_update(int iter,
    int size,
    int offset,
    int mazeEndIter,
    unsigned workerCongCost,
    unsigned workerHistCost,
    double congThresh,
    bool is2DRouting,
    RipUpMode mode)
{
  logger_->report("[INFO] GPU-accelerated Maze Routing is enabled !"); 
  frTime t;
  if (router_cfg_->VERBOSE > 0) {
    std::cout << std::endl << "start " << iter;
    std::string suffix;
    if (iter == 1 || (iter > 20 && iter % 10 == 1)) {
      suffix = "st";
    } else if (iter == 2 || (iter > 20 && iter % 10 == 2)) {
      suffix = "nd";
    } else if (iter == 3 || (iter > 20 && iter % 10 == 3)) {
      suffix = "rd";
    } else {
      suffix = "th";
    }
    std::cout << suffix << " optimization iteration ..." << std::endl;
  }

  auto gCellPatterns = getDesign()->getTopBlock()->getGCellPatterns();
  auto& xgp = gCellPatterns.at(0);
  auto& ygp = gCellPatterns.at(1);

  std::vector<std::unique_ptr<FlexGRWorker>> uworkers;
  std::vector<int> xCoords;
  std::vector<int> yCoords;
  for (int xIdx = 0; xIdx < (int) xgp.getCount(); xIdx++) {
    Rect gcellBox = getDesign()->getTopBlock()->getGCellBox(Point(xIdx, 0));
    xCoords.push_back((gcellBox.xMin() + gcellBox.xMax()) / 2);
  }  
  
  for (int yIdx = 0; yIdx < (int)ygp.getCount(); yIdx++) {
    Rect gcellBox = getDesign()->getTopBlock()->getGCellBox(Point(0, yIdx));
    yCoords.push_back((gcellBox.yMin() + gcellBox.yMax()) / 2);
  }
    
  // We do not use the original batch mode (divide the entire routing region into 
  // multiple batches based on batchStepX and batchStepY) in GPU-accelerated Maze Routing
  // We handle the entire routing region in a single batch
  for (int i = 0; i < (int) xgp.getCount(); i += size) {
    for (int j = 0; j < (int) ygp.getCount(); j += size) {
      auto worker = std::make_unique<FlexGRWorker>(this, router_cfg_, logger_);
      Point gcellIdxLL = Point(i, j);
      // Fixed by Zhiang
      Point gcellIdxUR
          = Point(std::min((int) xgp.getCount() - 1, i + size - 1),
                  std::min((int) ygp.getCount() - 1, j + size - 1));

      Rect routeBox1 = getDesign()->getTopBlock()->getGCellBox(gcellIdxLL);
      Rect routeBox2 = getDesign()->getTopBlock()->getGCellBox(gcellIdxUR);
      Rect extBox(routeBox1.xMin(),
                  routeBox1.yMin(),
                  routeBox2.xMax(),
                  routeBox2.yMax());
      Rect routeBox((routeBox1.xMin() + routeBox1.xMax()) / 2,
                    (routeBox1.yMin() + routeBox1.yMax()) / 2,
                    (routeBox2.xMin() + routeBox2.xMax()) / 2,
                    (routeBox2.yMin() + routeBox2.yMax()) / 2);

      worker->setRouteGCellIdxLL(gcellIdxLL);
      worker->setRouteGCellIdxUR(gcellIdxUR);
      worker->setExtBox(extBox);
      worker->setRouteBox(routeBox);
      worker->setMazeEndIter(mazeEndIter);
      worker->setGRIter(iter);
      worker->setCongCost(workerCongCost);
      worker->setHistCost(workerHistCost);
      worker->setCongThresh(congThresh);
      worker->set2D(is2DRouting);
      worker->setRipupMode(mode);
      worker->setWorkerId(uworkers.size());
      uworkers.push_back(std::move(worker));
    }
  }

  logger_->report("[INFO] Number of workers: " + std::to_string(uworkers.size()));
  int numThreads = std::min(8, router_cfg_->MAX_THREADS);
  numThreads = std::min(numThreads, (int) uworkers.size());
  
  //int numThreads = 1;
  omp_set_num_threads(numThreads);
  logger_->report("[INFO] Number of threads: " + std::to_string(numThreads));  

  // Break cross-worker boundary pathSeg into multiple segments
  for (auto& worker : uworkers) {
    worker->initBoundary();
  }

  // Parallel execution
  ThreadException exception_init;
#pragma omp parallel for schedule(dynamic)
  for (int i = 0; i < (int) uworkers.size(); i++) {  // NOLINT
    try {
      uworkers[i]->init();
    } catch (...) {
      exception_init.capture();
    }
  }
  exception_init.rethrow();


  float routeInitTime = 0.0;
  float routeTime = 0.0;
  float restoreTime = 0.0;
  float syncTime = 0.0;
  // mazeEndIter = 10;  


  unsigned BLOCKCOST = router_cfg_->BLOCKCOST * 100;
  unsigned OVERFLOWCOST = workerCongCost;
  unsigned HISTCOST = workerHistCost;

  std::cout << "BLOCKCOST: " << BLOCKCOST << std::endl;
  std::cout << "OVERFLOWCOST: " << OVERFLOWCOST << std::endl;
  std::cout << "HISTCOST: " << HISTCOST << std::endl;


  for (int iter = 0; iter < mazeEndIter; iter++) {    
    int xDim, yDim, zDim;
    uworkers[0]->getCMap()->getDim(xDim, yDim, zDim);
    logger_->report("[INFO] Routing iteration " + std::to_string(iter) + " start !");
    logger_->report("[INFO] xDim: " + std::to_string(xDim) + ", yDim: " + std::to_string(yDim) + ", zDim: " + std::to_string(zDim));
    
    float GPUMazeRouteTimeTot = 0;
    float RestoreTimeTot = 0;

    // Generate the batches in parallel
    std::vector<std::vector<grNet*> > rerouteNets(uworkers.size());
    ThreadException exception1;
#pragma omp parallel for schedule(dynamic)
    for (int j = 0; j < (int) uworkers.size(); j++) {  // NOLINT
      try {
        uworkers[j]->main_mt_prep(rerouteNets[j], iter);
      } catch (...) {
        exception1.capture();
      }
    }
    exception1.rethrow();

    // Assign unique netId and workerId to each net
    int netId = 0;
    int totalNumNets = 0;
    for (int workerId = 0; workerId < rerouteNets.size(); workerId++) {
      for (auto net : rerouteNets[workerId]) {
        net->setNetId(netId++);
        net->setWorkerId(workerId);
      }
    }
    totalNumNets = netId;

    if (totalNumNets == 0) {
      logger_->report("[INFO] [Early Exit] Overflow free!");
      break;
    }

    std::cout << "Test a" << std::endl;


    // Generate the batch for each worker in parallel
    std::vector<std::vector<std::vector<grNet*>> > workerBatches(uworkers.size());
#pragma omp parallel for schedule(dynamic)
    for (int j = 0; j < (int) uworkers.size(); j++) {  // NOLINT
      uworkers[j]->batchGenerationRelax(rerouteNets[j], workerBatches[j]); // Use the lazy mode
    }

    
    std::cout << "Test b" << std::endl;

    // Create global level batches
    std::vector<std::vector<grNet*> > batches;
    int maxNumBatches = 0;
    for (int j = 0; j < (int) uworkers.size(); j++) {  // NOLINT
      maxNumBatches = std::max(maxNumBatches, (int) workerBatches[j].size());
    }

    batches.resize(maxNumBatches);


    std::cout << "[INFO] maxNumBatches = " << maxNumBatches << std::endl;

    // Option 1:  evenly distribute the nets to the batches

    std::vector<int> binCnt(maxNumBatches, 0);
    std::vector<bool> binVisited(maxNumBatches, false);
    int avgBinNet = (totalNumNets + maxNumBatches - 1) / maxNumBatches;
    std::cout << "avgBinNet: " << avgBinNet << std::endl;
    
    for (int workerId = 0; workerId < (int) uworkers.size(); workerId++) {
      std::fill(binVisited.begin(), binVisited.end(), false);
      // determine the batch id for each localBatch
      for (int localBatchId = workerBatches[workerId].size() - 1; localBatchId >= 0; localBatchId--) {
        int batchId = -1;
        auto& localBatch = workerBatches[workerId][localBatchId];
        for (int i = 0; i < maxNumBatches; i++) {
          if (binVisited[i]) {
            continue;
          }
          if (binCnt[i] + localBatch.size() <= avgBinNet) {
            batchId = i;
            break;
          }
        }

        if (batchId == -1) {
          int minBatchId = -1;
          int minBatchSize = totalNumNets;
          for (int i = 0; i < maxNumBatches; i++) {
            if (binVisited[i]) {
              continue;
            }
            if (binCnt[i] < minBatchSize) {
              minBatchSize = binCnt[i];
              minBatchId = i;
            }
          }

          batchId = minBatchId;
        }

        std::copy(localBatch.begin(), localBatch.end(), std::back_inserter(batches[batchId]));
        binCnt[batchId] += localBatch.size();
        binVisited[batchId] = true;
      }
    }
    

    // Option 2:  reverse the order of the batches
    /*
    for (int j = maxNumBatches - 1; j >= 0; j--) {
      auto& batch = batches[maxNumBatches - 1 - j];
      for (int k = 0; k < (int) uworkers.size(); k++) {  // NOLINT
        if (j < (int) workerBatches[k].size()) {
          std::copy(workerBatches[k][j].begin(), workerBatches[k][j].end(), std::back_inserter(batch));
        }
      }
    } */
    
    // print the batch information
    logger_->report("[INFO] (Relax) Number of batches: " + std::to_string(batches.size()));
    for (int i = 0; i < batches.size(); i++) {
      logger_->report("[INFO] Batch " + std::to_string(i) + ": " + std::to_string(batches[i].size()) + " nets");
    }
   
    // Copy the cost map back to the cmap
    // At this step, we ripup all the nets
    ThreadException exception2;
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < (int) uworkers.size(); i++) {
      try {
        uworkers[i]->main_mt_init(rerouteNets[i]);  
      } catch (...) {
        exception2.capture();
      }
    }
    exception2.rethrow();


    auto runCPURoute = [&]() {
      ThreadException exception3;
#pragma omp parallel for schedule(dynamic)
      for (int i = 0; i < (int) uworkers.size(); i++) {  // NOLINT
        try {
          uworkers[i]->main_mt_restore_temp(rerouteNets[i]); 
        } catch (...) {
          exception3.capture();
        }
      }  
      exception3.rethrow();
    };


    if (is2DRouting == true) {
      for (int batchIdx = 0; batchIdx < batches.size(); batchIdx++) {      
      //for (int batchIdx = batches.size() -  1;  batchIdx >= 0; batchIdx--) {
        if (batches[batchIdx].size() < validBatchThreshold_) {
          // Route all the nets on the CPU side
#pragma omp parallel for schedule(dynamic)      
          for (int i = 0; i < (int) uworkers.size(); i++) {  // NOLINT
            uworkers[i]->main_mt_restore_CPU_only(batches[batchIdx]);
          }
          continue;
        }

        // Copy the cost map back to the cmap
        for (int i = 0; i < (int) uworkers.size(); i++) {
          uworkers[i]->initGridGraph_back2CMap();
        }

        // std::cout << "Before GPU Routing" << std::endl;
        // reportCong2D();

        auto& h_costMap = uworkers[0]->getCMap()->getBits();



        // We need to use GPU-accelerated Maze Routing
        std::vector<std::vector<grNet*> > subBatches;
        std::vector<int> validBatchIds;
        batchGenerationMIS_update(batches[batchIdx], subBatches, validBatchIds, iter, is2DRouting);

        int numValidBatches = validBatchIds.size();
        int numGrids = xDim * yDim;
        if (VERBOSE > 0) {
          std::cout << "[INFO] Number of batches: " << numValidBatches << std::endl;
        }
          // std::vector<Point2D_CUDA> h_parents(numGrids * numValidBatches, Point2D_CUDA(-1, -1));
        std::vector<Point2D_CUDA> h_parents(numGrids * numValidBatches, Point2D_CUDA(-1, -1));
  
        // In the current strategy, we ripup all the nets at the beginning
        // multi thread
        // then we route all the nets in the batches
        // Then we restore all the nets at the end
        float relaxThreshold = 2.0;
        // Route the nets in the batches
        float GPURuntime = GPUAccelerated2DMazeRoute_update_v3(
          uworkers, subBatches, validBatchIds, h_parents, h_costMap, xCoords, yCoords, 
          relaxThreshold, congThresh, 
          BLOCKCOST, OVERFLOWCOST, HISTCOST, xDim, yDim);
        
        if (0) {
        //if (VERBOSE > 0) {
          logger_->report("[INFO] Batch GPU Maze Routing Runtime: " + std::to_string(GPURuntime) + " ms");
        }

        GPUMazeRouteTimeTot += GPURuntime;

        auto restoreRuntimeStart = std::chrono::high_resolution_clock::now();
        // For CPU-side net, we do routing          
#pragma omp parallel for schedule(dynamic)
        for (int j = 0; j < (int) uworkers.size(); j++) {  // NOLINT
          uworkers[j]->main_mt_restore_CPU_GPU(batches[batchIdx], h_parents, xDim, yDim);
        }

        auto restoreRuntimeEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> restoreRuntime = restoreRuntimeEnd - restoreRuntimeStart;
        if (VERBOSE > 0) {
          logger_->report("[INFO] Batch GPU Maze Restore Runtime: " + std::to_string(restoreRuntime.count()) + " ms");
        }

      
        // Copy the cost map back to the cmap
        for (int i = 0; i < (int) uworkers.size(); i++) {
          uworkers[i]->initGridGraph_back2CMap();
        }

        //std::cout << "After GPU Routing" << std::endl;
        //reportCong2D();

        RestoreTimeTot += restoreRuntime.count();
      }
    } else {
#pragma omp parallel for schedule(dynamic)
      for (int i = 0; i < (int) uworkers.size(); i++) {  // NOLINT
        uworkers[i]->main_mt_restore_temp(rerouteNets[i]); 
      }  
    }

    // Update the history cost
    // This can be done in parallel if necessary
    for (int j = 0; j < (int) uworkers.size(); j++) {  // NOLINT
      uworkers[j]->route_decayHistCost_update();
    }

        
    // Copy the cost map back to the cmap
    for (int i = 0; i < (int) uworkers.size(); i++) {
      uworkers[i]->initGridGraph_back2CMap();
    }

    //reportCong2D();

    std::cout << "Iteration GPU Maze Routing Time: " << GPUMazeRouteTimeTot << " ms" << std::endl;
    std::cout << "Iteration Restore Time: " << RestoreTimeTot << " ms" << std::endl;
  }

  std::cout << "Finish Routing Kernels" << std::endl;

  for (auto& worker : uworkers) {
    worker->end();
  }
  uworkers.clear();

  std::cout << "Finish ending" << std::endl;

  reportCong2D();

  /*
  for (int iter = 0; iter < mazeEndIter; iter++) {
    std::vector<uint64_t> h_costMap = uworkers[0]->getCMap()->getBits();
    std::vector<std::vector<grNet*> > rerouteNets(uworkers.size());
    ThreadException exception1;
#pragma omp parallel for schedule(dynamic)
    for (int j = 0; j < (int) uworkers.size(); j++) {  // NOLINT
      try {
        uworkers[j]->main_mt_prep(rerouteNets[j], iter);
      } catch (...) {
        exception1.capture();
      }
    }
    exception1.rethrow();

    // Copy the cost map back to the cmap
    ThreadException exception2;
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < (int) uworkers.size(); i++) {
      try {
        uworkers[i]->main_mt_init(rerouteNets[i]);  
      } catch (...) {
        exception2.capture();
      }
    }
    exception2.rethrow();
  
    if (is2DRouting == true) {     
      //auto& h_costMap = uworkers[0]->getCMap()->getBits();
      int xDim, yDim, zDim;
      uworkers[0]->getCMap()->getDim(xDim, yDim, zDim);
      logger_->report("[INFO] Routing iteration " + std::to_string(iter) + " start !");
      logger_->report("[INFO] xDim: " + std::to_string(xDim) + ", yDim: " + std::to_string(yDim) + ", zDim: " + std::to_string(zDim));
  
      // Divide the nets into multiple batches and run the GPU-accelerated Maze Routing
      // Different nets has different relaxation strategies
      // The relaxation strategy is determined by the iteration number and the number of pins
      // For feedthrough nets, we also need to give them special treatment
      // Then determine the batch generation is the following manner:
      // Nets from different workers will naturally have no overlap
      // Nets from the same workers will be batched based on the relaxed bounding box
      std::vector<std::vector<grNet*> > batches;
      std::vector<int> validBatchIds;
      batchGenerationMIS(rerouteNets, batches, validBatchIds, iter, is2DRouting);
     
      int numValidBatches = validBatchIds.size();
      int numGrids = xDim * yDim;
      std::cout << "[INFO] Number of batches: " << numValidBatches << std::endl;
      // std::vector<Point2D_CUDA> h_parents(numGrids * numValidBatches, Point2D_CUDA(-1, -1));
      std::vector<Point2D_CUDA> h_parents(numGrids * numValidBatches, Point2D_CUDA(-1, -1));

      // In the current strategy, we ripup all the nets at the beginning
      // multi thread
      // then we route all the nets in the batches
      // Then we restore all the nets at the end
    
      float relaxThreshold = 2.0;
      // Route the nets in the batches
      float GPURuntime = GPUAccelerated2DMazeRoute_update_v3(
        uworkers, batches, validBatchIds, h_parents, h_costMap, xCoords, yCoords, router_cfg_, 
        relaxThreshold, congThresh, xDim, yDim);
      logger_->report("[INFO] GPU Maze Routing Runtime: " + std::to_string(GPURuntime) + " ms");

      // For CPU-side net, we do routing
      // For GPU-side net, we do restore only
      // Parallel execution
      if (0) {
      ThreadException exception3;
#pragma omp parallel for schedule(dynamic)
      for (int i = 0; i < (int) uworkers.size(); i++) {  // NOLINT
        try {
          uworkers[i]->main_mt_restore(rerouteNets[i], h_parents, xDim, yDim); 
        } catch (...) {
          exception3.capture();
        }
      }  
      exception3.rethrow(); }
#pragma omp parallel for schedule(dynamic)
      for (int j = 0; j < (int) uworkers.size(); j++) {  // NOLINT
        uworkers[j]->main_mt_restore(rerouteNets[j], h_parents, xDim, yDim);
      }

    } else {
      ThreadException exception4;
#pragma omp parallel for schedule(dynamic)
      for (int i = 0; i < (int) uworkers.size(); i++) {  // NOLINT
        try {
          uworkers[i]->main_mt_restore_temp(rerouteNets[i]); 
        } catch (...) {
          exception4.capture();
        }
      }  
      exception4.rethrow();
    }
  
    */













  /* -------------------------------------------------------------------------------------------
  // Original code snippet
  // We do not use the original batch mode
  int batchStepX, batchStepY;
  getBatchInfo(batchStepX, batchStepY);
  
  std::vector<std::vector<std::vector<std::unique_ptr<FlexGRWorker>>>>
    workers(batchStepX * batchStepY);

  int xIdx = 0;
  int yIdx = 0;
  // sequential init
  for (int i = 0; i < (int) xgp.getCount(); i += size) {
    for (int j = 0; j < (int) ygp.getCount(); j += size) {
      auto worker = std::make_unique<FlexGRWorker>(this, router_cfg_);
      Point gcellIdxLL = Point(i, j);
      Point gcellIdxUR
          = Point(std::min((int) xgp.getCount() - 1, i + size - 1),
                  std::min((int) ygp.getCount(), j + size - 1));

      Rect routeBox1 = getDesign()->getTopBlock()->getGCellBox(gcellIdxLL);
      Rect routeBox2 = getDesign()->getTopBlock()->getGCellBox(gcellIdxUR);
      Rect extBox(routeBox1.xMin(),
                  routeBox1.yMin(),
                  routeBox2.xMax(),
                  routeBox2.yMax());
      Rect routeBox((routeBox1.xMin() + routeBox1.xMax()) / 2,
                    (routeBox1.yMin() + routeBox1.yMax()) / 2,
                    (routeBox2.xMin() + routeBox2.xMax()) / 2,
                    (routeBox2.yMin() + routeBox2.yMax()) / 2);

      // worker->setGCellIdx(gcellIdxLL, gcellIdxUR);
      worker->setRouteGCellIdxLL(gcellIdxLL);
      worker->setRouteGCellIdxUR(gcellIdxUR);
      worker->setExtBox(extBox);
      worker->setRouteBox(routeBox);
      worker->setMazeEndIter(mazeEndIter);
      worker->setGRIter(iter);
      worker->setCongCost(workerCongCost);
      worker->setHistCost(workerHistCost);
      worker->setCongThresh(congThresh);
      worker->set2D(is2DRouting);
      worker->setRipupMode(mode);

      int batchIdx = (xIdx % batchStepX) * batchStepY + yIdx % batchStepY;
      if (workers[batchIdx].empty()
          || (int) workers[batchIdx].back().size()
                  >= router_cfg_->BATCHSIZE) {
        workers[batchIdx].push_back(
            std::vector<std::unique_ptr<FlexGRWorker>>());
      }
      workers[batchIdx].back().push_back(std::move(worker));

      yIdx++;
    }
    yIdx = 0;
    xIdx++;
  }

  omp_set_num_threads(std::min(8, router_cfg_->MAX_THREADS));
  // omp_set_num_threads(1);

  
  // parallel execution
  for (auto& workerBatch : workers) {
    for (auto& workersInBatch : workerBatch) {
      // single thread
      // split cross-worker boundary pathSeg
      for (auto& worker : workersInBatch) {
        worker->initBoundary();
      }
      
      // Get all the nets for batch generation      
      // multi thread
      ThreadException exception;
#pragma omp parallel for schedule(dynamic)
      for (int i = 0; i < (int) workersInBatch.size(); i++) {  // NOLINT
        try {
          workersInBatch[i]->main_mt();
        } catch (...) {
          exception.capture();
        }
      }
      exception.rethrow();
      // single thread
      for (auto& worker : workersInBatch) {
        worker->end();
      }
      workersInBatch.clear();
    }
  }
  */

  t.print(logger_);
  std::cout << std::endl << std::flush;
}

} // namespace drt