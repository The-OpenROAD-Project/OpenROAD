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

#include "FlexTA.h"

#include <omp.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>

#include "FlexTA_graphics.h"
#include "db/infra/frTime.h"
#include "frProfileTask.h"
#include "global.h"
#include "utl/exception.h"

namespace fr {

int FlexTAWorker::main_mt()
{
  ProfileTask profile("TA:main_mt");
  using namespace std::chrono;
  auto t0 = high_resolution_clock::now();
  if (VERBOSE > 1) {
    stringstream ss;
    ss << endl
       << "start TA worker (BOX) ("
       << routeBox_.xMin() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU()
       << ", "
       << routeBox_.yMin() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU()
       << ") ("
       << routeBox_.xMax() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU()
       << ", "
       << routeBox_.yMax() * 1.0 / getDesign()->getTopBlock()->getDBUPerUU()
       << ") ";
    if (getDir() == dbTechLayerDir::HORIZONTAL) {
      ss << "H";
    } else {
      ss << "V";
    }
    ss << endl;
    cout << ss.str();
  }

  init();
  if (isInitTA()) {
    hardIroutesMode = true;
    sortIroutes();
    assign();
    hardIroutesMode = false;
  }
  sortIroutes();
  auto t1 = high_resolution_clock::now();
  assign();
  auto t2 = high_resolution_clock::now();
  // end();
  auto t3 = high_resolution_clock::now();

  auto time_span0 = duration_cast<duration<double>>(t1 - t0);
  auto time_span1 = duration_cast<duration<double>>(t2 - t1);
  auto time_span2 = duration_cast<duration<double>>(t3 - t2);

  if (VERBOSE > 1) {
    stringstream ss;
    ss << "time (INIT/ASSIGN/POST) " << time_span0.count() << " "
       << time_span1.count() << " " << time_span2.count() << " " << endl;
    cout << ss.str() << flush;
  }
  return 0;
}

FlexTA::FlexTA(frDesign* in, Logger* logger, bool save_updates)
    : tech_(in->getTech()),
      design_(in),
      logger_(logger),
      save_updates_(save_updates)
{
}

FlexTA::~FlexTA() = default;

int FlexTA::initTA_helper(int iter,
                          int size,
                          int offset,
                          bool isH,
                          int& numPanels)
{
  auto gCellPatterns = getDesign()->getTopBlock()->getGCellPatterns();
  auto& xgp = gCellPatterns.at(0);
  auto& ygp = gCellPatterns.at(1);
  int sol = 0;
  numPanels = 0;
  vector<vector<unique_ptr<FlexTAWorker>>> workers;
  if (isH) {
    for (int i = offset; i < (int) ygp.getCount(); i += size) {
      auto uworker
          = make_unique<FlexTAWorker>(getDesign(), logger_, save_updates_);
      auto& worker = *(uworker.get());
      Rect beginBox = getDesign()->getTopBlock()->getGCellBox(Point(0, i));
      Rect endBox = getDesign()->getTopBlock()->getGCellBox(
          Point((int) xgp.getCount() - 1,
                min(i + size - 1, (int) ygp.getCount() - 1)));
      Rect routeBox(
          beginBox.xMin(), beginBox.yMin(), endBox.xMax(), endBox.yMax());
      Rect extBox;
      routeBox.bloat(ygp.getSpacing() / 2, extBox);
      worker.setRouteBox(routeBox);
      worker.setExtBox(extBox);
      worker.setDir(dbTechLayerDir::HORIZONTAL);
      worker.setTAIter(iter);
      if (workers.empty() || (int) workers.back().size() >= BATCHSIZETA) {
        workers.emplace_back(vector<unique_ptr<FlexTAWorker>>());
      }
      workers.back().emplace_back(std::move(uworker));
    }
  } else {
    for (int i = offset; i < (int) xgp.getCount(); i += size) {
      auto uworker
          = make_unique<FlexTAWorker>(getDesign(), logger_, save_updates_);
      auto& worker = *(uworker.get());
      Rect beginBox = getDesign()->getTopBlock()->getGCellBox(Point(i, 0));
      Rect endBox = getDesign()->getTopBlock()->getGCellBox(
          Point(min(i + size - 1, (int) xgp.getCount() - 1),
                (int) ygp.getCount() - 1));
      Rect routeBox(
          beginBox.xMin(), beginBox.yMin(), endBox.xMax(), endBox.yMax());
      Rect extBox;
      routeBox.bloat(xgp.getSpacing() / 2, extBox);
      worker.setRouteBox(routeBox);
      worker.setExtBox(extBox);
      worker.setDir(dbTechLayerDir::VERTICAL);
      worker.setTAIter(iter);
      if (workers.empty() || (int) workers.back().size() >= BATCHSIZETA) {
        workers.emplace_back(vector<unique_ptr<FlexTAWorker>>());
      }
      workers.back().push_back(std::move(uworker));
    }
  }

  omp_set_num_threads(min(8, MAX_THREADS));
  // parallel execution
  // multi thread
  for (auto& workerBatch : workers) {
    ProfileTask profile("TA:batch");
    utl::ThreadException exception;
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < (int) workerBatch.size(); i++) {
      try {
        workerBatch[i]->main_mt();
#pragma omp critical
        {
          sol += workerBatch[i]->getNumAssigned();
          numPanels++;
        }
      } catch (...) {
        exception.capture();
      }
    }
    exception.rethrow();
    for (auto& worker : workerBatch) {
      worker->end();
    }
    workerBatch.clear();
  }
  return sol;
}

void FlexTA::initTA(int size)
{
  ProfileTask profile("TA:init");
  frTime t;

  if (VERBOSE > 1) {
    cout << endl << "start initial track assignment ..." << endl;
  }

  auto bottomLNum = getDesign()->getTech()->getBottomLayerNum();
  auto bottomLayer = getDesign()->getTech()->getLayer(bottomLNum);
  if (bottomLayer->getType() != dbTechLayerType::ROUTING) {
    bottomLNum++;
    bottomLayer = getDesign()->getTech()->getLayer(bottomLNum);
  }
  bool isBottomLayerH = (bottomLayer->getDir() == dbTechLayerDir::HORIZONTAL);

  // H first
  if (isBottomLayerH) {
    int numPanelsH;
    int numAssignedH = initTA_helper(0, size, 0, true, numPanelsH);

    int numPanelsV;
    int numAssignedV = initTA_helper(0, size, 0, false, numPanelsV);

    if (VERBOSE > 0) {
      logger_->info(DRT,
                    183,
                    "Done with {} horizontal wires in {} frboxes and "
                    "{} vertical wires in {} frboxes.",
                    numAssignedH,
                    numPanelsH,
                    numAssignedV,
                    numPanelsV);
    }
    // V first
  } else {
    int numPanelsV;
    int numAssignedV = initTA_helper(0, size, 0, false, numPanelsV);

    int numPanelsH;
    int numAssignedH = initTA_helper(0, size, 0, true, numPanelsH);

    if (VERBOSE > 0) {
      logger_->info(DRT,
                    184,
                    "Done with {} vertical wires in {} frboxes and "
                    "{} horizontal wires in {} frboxes.",
                    numAssignedV,
                    numPanelsV,
                    numAssignedH,
                    numPanelsH);
    }
  }
}

void FlexTA::searchRepair(int iter, int size, int offset)
{
  ProfileTask profile("TA:searchRepair");
  frTime t;

  if (VERBOSE > 1) {
    cout << endl << "start " << iter;
    string suffix;
    if (iter == 1 || (iter > 20 && iter % 10 == 1)) {
      suffix = "st";
    } else if (iter == 2 || (iter > 20 && iter % 10 == 2)) {
      suffix = "nd";
    } else if (iter == 3 || (iter > 20 && iter % 10 == 3)) {
      suffix = "rd";
    } else {
      suffix = "th";
    }
    cout << suffix << " optimization iteration ..." << endl;
  }
  auto bottomLNum = getDesign()->getTech()->getBottomLayerNum();
  auto bottomLayer = getDesign()->getTech()->getLayer(bottomLNum);
  if (bottomLayer->getType() != dbTechLayerType::ROUTING) {
    bottomLNum++;
    bottomLayer = getDesign()->getTech()->getLayer(bottomLNum);
  }
  bool isBottomLayerH = (bottomLayer->getDir() == dbTechLayerDir::HORIZONTAL);

  // H first
  if (isBottomLayerH) {
    int numPanelsH;
    int numAssignedH = initTA_helper(iter, size, offset, true, numPanelsH);

    int numPanelsV;
    int numAssignedV = initTA_helper(iter, size, offset, false, numPanelsV);

    if (VERBOSE > 0) {
      logger_->info(DRT,
                    268,
                    "Done with {} horizontal wires in {} frboxes and "
                    "{} vertical wires in {} frboxes.",
                    numAssignedH,
                    numPanelsH,
                    numAssignedV,
                    numPanelsV);
    }
    // V first
  } else {
    int numPanelsV;
    int numAssignedV = initTA_helper(iter, size, offset, false, numPanelsV);

    int numPanelsH;
    int numAssignedH = initTA_helper(iter, size, offset, true, numPanelsH);

    if (VERBOSE > 0) {
      logger_->info(DRT,
                    186,
                    "Done with {} vertical wires in {} frboxes and "
                    "{} horizontal wires in {} frboxes.",
                    numAssignedV,
                    numPanelsV,
                    numAssignedH,
                    numPanelsH);
    }
  }
}

void FlexTA::setDebug(frDebugSettings* settings, odb::dbDatabase* db)
{
  bool on = settings->debugTA;
  graphics_ = on && FlexTAGraphics::guiActive()
                  ? std::make_unique<FlexTAGraphics>(settings, design_, db)
                  : nullptr;
}

int FlexTA::main()
{
  ProfileTask profile("TA:main");

  frTime t;
  if (VERBOSE > 0) {
    logger_->info(DRT, 181, "Start track assignment.");
  }
  initTA(50);
  if (graphics_) {
    graphics_->endIter(0);
  }

  searchRepair(1, 50, 0);
  if (graphics_) {
    graphics_->endIter(1);
  }

  if (VERBOSE > 0) {
    logger_->info(DRT, 182, "Complete track assignment.");
  }
  if (VERBOSE > 0) {
    t.print(logger_);
  }
  return 0;
}

}  // namespace fr
