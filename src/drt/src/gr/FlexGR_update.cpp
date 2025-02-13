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
  return !(r1.xMax() <= r2.xMin() || r1.xMin() >= r2.xMax() || r1.yMax() <= r2.yMin() || r1.yMin() >= r2.yMax());
}


// Batch Generation Related Functions
// Version 1:
// Greedy maximal independent set (MIS) based batch generation
void FlexGR::batchGenerationMIS(
  std::vector<std::vector<grNet*>> &rerouteNets,
  std::vector<std::vector<grNet*>> &batches,
  std::vector<int> &validBatchIds,
  int iter)
{      
  // Measure the runtime of std::sort.
  auto start_time = std::chrono::high_resolution_clock::now();

  for (auto& batch : batches) {
    batch.clear();
  }
  batches.clear();

  // Assign unique netId and workerId to each net
  int netId = 0;
  for (int workerId = 0; workerId < rerouteNets.size(); workerId++) {
    for (auto net : rerouteNets[workerId]) {
      net->setNetId(netId++);
      net->setWorkerId(workerId);
    }
  }

  if (netId == 0) {
    return;
  }

  // Implement the batch generation algorithm
  std::vector<int> batchStartPtr;
  for (int workerId = 0; workerId < rerouteNets.size(); workerId++) {
    // Perform bounding box relaxation later
    // Now just use the route bounding box
    // sort all the nets in the non-decreasing order of HPWL
    std::sort(rerouteNets[workerId].begin(), rerouteNets[workerId].end(), 
      [](const grNet* a, const grNet* b) {
        return a->getHPWL() > b->getHPWL();
      });

    // For the net in the same worker, determine the batch number
    for (auto net : rerouteNets[workerId]) {
      bool found = false;
      for (int batchId = 0; batchId < batches.size(); batchId++) {
        bool conflict = false;
        for (int net_idx = batchStartPtr[batchId]; net_idx < batches[batchId].size(); net_idx++) {
          if (overlap(net->getRouteBBox(), batches[batchId][net_idx]->getRouteBBox())) {
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
        batchStartPtr.push_back(0);
      }
    }

    // Update the batch start pointer
    for (int i = 0; i < batches.size(); i++) {
      batchStartPtr[i] = batches[i].size();
    } 
  }

  // Check the valid batch threshold
  validBatchIds.clear();
  int gpuNets = 0;
  int cpuNets = 0;
  for (int i = 0; i < batches.size(); i++) {
    if (batches[i].size() >= validBatchThreshold_) {
      validBatchIds.push_back(i);
      for (auto net : batches[i]) {
        net->setCPUFlag(false);
      }
      gpuNets += batches[i].size();
    } else {
      cpuNets += batches[i].size();
    }
  }
 
  // Report the batch information
  logger_->report("[INFO] Number of batches: " + std::to_string(batches.size()));
  logger_->report("[INFO] Batch information:");
  for (int i = 0; i < validBatchIds.size(); i++) {
    logger_->report("[INFO] Batch " + std::to_string(validBatchIds[i]) + ": " + std::to_string(batches[validBatchIds[i]].size()) + " nets");
  } 

  logger_->report("[INFO] Number of GPU nets: " + std::to_string(gpuNets) + " (" + std::to_string(gpuNets * 100.0 / (cpuNets + gpuNets)) + "%)");
  logger_->report("[INFO] Number of CPU nets: " + std::to_string(cpuNets) + " (" + std::to_string(cpuNets * 100.0 / (cpuNets + gpuNets)) + "%)");

  // Calculate elapsed time in milliseconds.
  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> elapsed = end_time - start_time;
  logger_->report("[INFO] Batch generation runtime: " + std::to_string(elapsed.count() * 1e-3) + " s");
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

  for (int iter = 0; iter < mazeEndIter; iter++) {
    std::vector<std::vector<grNet*>> rerouteNets(uworkers.size());
    
    // This can be done in parallel if necessary
    for (int j = 0; j < (int) uworkers.size(); j++) {  // NOLINT
      uworkers[j]->route_addHistCost_update();
      uworkers[j]->routePrep_update(rerouteNets[j], iter);
    }

    // Divide the nets into multiple batches and run the GPU-accelerated Maze Routing
    // Different nets has different relaxation strategies
    // The relaxation strategy is determined by the iteration number and the number of pins
    // For feedthrough nets, we also need to give them special treatment
    // Then determine the batch generation is the following manner:
    // Nets from different workers will naturally have no overlap
    // Nets from the same workers will be batched based on the relaxed bounding box


    std::vector<std::vector<grNet*>> batches;
    std::vector<int> validBatchIds;
    batchGenerationMIS(rerouteNets, batches, validBatchIds, iter);
        
    // Run the CPU-side maze routing
    // Parallel execution
    ThreadException exception;
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < (int) uworkers.size(); i++) {  // NOLINT
      try {
        uworkers[i]->route(rerouteNets[i]); 
      } catch (...) {
        exception.capture();
      }
    }  
    exception.rethrow();

    // Copy the cost map back to the cmap
    for (int i = 0; i < (int) uworkers.size(); i++) {
      uworkers[i]->initGridGraph_back2CMap();
    }
      
    auto& h_costMap = uworkers[0]->getCMap()->getBits();
    int xDim, yDim, zDim;
    uworkers[0]->getCMap()->getDim(xDim, yDim, zDim);
    logger_->report("[INFO] Routing iteration " + std::to_string(iter) + " start !");
    logger_->report("[INFO] xDim: " + std::to_string(xDim) + ", yDim: " + std::to_string(yDim) + ", zDim: " + std::to_string(zDim));

    if (is2DRouting == true) {
      std::cout << "This is for 2D routing" << std::endl;
      // Route the nets in the batches
      for (auto& batchId : validBatchIds) {
        auto& curBatch = batches[batchId];
        // This can be done in parallel if necessary
        for (auto net : curBatch) {  
          uworkers[net->getWorkerId()]->mazeNetInit(net);
          net->updateAbsGridCoords(uworkers[net->getWorkerId()]->getRouteGCellIdxLL());
        }
        
        // update the cmap based on the grid graph
        for (int i = 0; i < (int) uworkers.size(); i++) {
          uworkers[i]->initGridGraph_back2CMap();
        }
        
        h_costMap = uworkers[0]->getCMap()->getBits();
        GPUAccelerated2DMazeRoute(
          uworkers,
          curBatch, h_costMap, 
          xCoords, yCoords, router_cfg_, 
          congThresh, xDim, yDim);

        // Restore the net in the batch
        // This can be done in parallel if necessary
        for (auto net : curBatch) {
          uworkers[net->getWorkerId()]->restoreNet(net);
        }
      }  
    } else {
      std::cout << "This is for 3D routing" << std::endl;
      // Route the nets in the batches
      for (auto& batchId : validBatchIds) {
        for (auto net : batches[batchId]) {  
          uworkers[net->getWorkerId()]->mazeNetInit(net);
          uworkers[net->getWorkerId()]->routeNet(net);
        }
      }      
    }

    // Update the history cost
    // This can be done in parallel if necessary
    for (int j = 0; j < (int) uworkers.size(); j++) {  // NOLINT
      uworkers[j]->route_decayHistCost_update();
    }
  }

  for (auto& worker : uworkers) {
    worker->end();
  }
  uworkers.clear();
  
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