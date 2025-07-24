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

// The file is designed for GPU-accelerated Layer Assignment

#include "FlexGR.h"
#include <omp.h>
#include <spdlog/common.h>
#include <sys/types.h>

#include <climits>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <mutex>
#include <queue>

#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include "db/infra/frTime.h"
#include "db/obj/frGuide.h"
#include "odb/db.h"
#include "utl/exception.h"
#include "gr/FlexGR_util.h"

namespace drt {

using utl::ThreadException;

void FlexGR::mazeRoute2D_gpu()
{
  logger_->report("--------------------------------------------------------------");
  logger_->report("[INFO] Start 2D Maze Routing ...");
  logger_->report("--------------------------------------------------------------");
  logger_->report("[INFO] 2D Maze Routing parameters: Clip Size = {}, Maze End Iter = {}",
                   clipSize2D_, mazeEndIter2D_);
            
  // Shift the boundary for each sBox
  searchRepair_gpu(0, // iter
                   clipSize2D_, // size
                   0, // offset
                   mazeEndIter2D_, // mazeEndIter
                   1 * router_cfg_->CONGCOST, // workerCongCost
                   0.5 * router_cfg_->HISTCOST, // workerHistCost
                   0.9, // congThresh
                   true, // is2DRouting
                   RipUpMode::ALL); // mode
}


void FlexGR::searchRepair_gpu(int iter,
  int size,
  int offset,
  int mazeEndIter,
  unsigned workerCongCost,
  unsigned workerHistCost,
  double congThresh,
  bool is2DRouting,
  RipUpMode mode)
{
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
  logger_->report("[INFO] Start {}{} optimization iteration ...", iter, suffix);    

  // Get the GCell information
  auto gCellPatterns = getDesign()->getTopBlock()->getGCellPatterns();
  auto& xgp = gCellPatterns.at(0);
  auto& ygp = gCellPatterns.at(1);
  const int gcellCountX = xgp.getCount();
  const int gcellCountY = ygp.getCount();
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


  // Step 1: create all the workers
  std::vector<std::unique_ptr<FlexGRWorker> > uworkers;
  // We do not use the original batch mode (divide the entire routing region into 
  // multiple batches based on batchStepX and batchStepY) in GPU-accelerated Maze Routing
  // We handle the entire routing region in a single batch
  for (int i = 0; i < gcellCountX; i += size) {
    for (int j = 0; j < gcellCountY; j += size) {
      auto worker = std::make_unique<FlexGRWorker>(this, router_cfg_, logger_);
      const Point gcellIdxLL = Point(i, j);
      const Point gcellIdxUR = Point(std::min(gcellCountX - 1, i + size - 1),
        std::min(gcellCountY - 1, j + size - 1));
      const Rect routeBox1 = getDesign()->getTopBlock()->getGCellBox(gcellIdxLL);
      const Rect routeBox2 = getDesign()->getTopBlock()->getGCellBox(gcellIdxUR);
      const Rect extBox(routeBox1.xMin(),
                        routeBox1.yMin(),
                        routeBox2.xMax(),
                        routeBox2.yMax());
      const Rect routeBox((routeBox1.xMin() + routeBox1.xMax()) / 2,
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
  omp_set_num_threads(numThreads);
  logger_->report("[INFO] Number of threads: " + std::to_string(numThreads));  

  // Step 2: Initialize all the workers
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
  
  // Step 3: GPU-accelerated maze routing
  // 3-level Batch Generation (it seems that atomic add is very slow, so we use 3-level batch generation)
  unsigned BLOCKCOST = router_cfg_->BLOCKCOST * 100;
  unsigned OVERFLOWCOST = workerCongCost;
  unsigned HISTCOST = workerHistCost;
  searchRepairMazeCore(uworkers, mazeEndIter,
    BLOCKCOST, OVERFLOWCOST, HISTCOST);

  // Step 4: Write back the local routing solutions (grNet) to the global routing (frNet) solution 
  // This part cannot be parallelized
  // There may be multiple grNets for a single frNet (avoiding race condition)
  for (auto& worker : uworkers) {
    worker->end();
  }
  uworkers.clear();

  logger_->report("[INFO] Finish {}{} optimization iteration ...", iter, suffix);    
}


void FlexGR::searchRepairMazeCore(
  std::vector<std::unique_ptr<FlexGRWorker> >& uworkers,
  int mazeEndIter,
  unsigned BLOCKCOST,
  unsigned OVERFLOWCOST,
  unsigned HISTCOST)
{
  for (int iter = 0; iter < mazeEndIter; iter++) {    
    logger_->report("[INFO] RRR iteration " + std::to_string(iter) + " start !"); 
    // Generate the batches in parallel
    // Get all the nets to be rerouted
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
    
    logger_->report("[INFO] RRR iteration " + std::to_string(iter) + " finish !"); 
  }
}























} // namespace drt





