// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "ta/FlexTA.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "db/infra/frTime.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "frProfileTask.h"
#include "global.h"
#include "odb/dbTypes.h"
#include "omp.h"
#include "ta/AbstractTAGraphics.h"
#include "utl/Logger.h"
#include "utl/exception.h"

using odb::dbTechLayerDir;
using odb::dbTechLayerType;

namespace drt {

namespace {

constexpr int kMaxTaThreads = 8;

bool isBottomRoutingLayerHorizontal(frDesign* design)
{
  frLayerNum bottom_layer_num = design->getTech()->getBottomLayerNum();
  auto* bottom_layer = design->getTech()->getLayer(bottom_layer_num);
  if (bottom_layer->getType() != dbTechLayerType::ROUTING) {
    bottom_layer_num++;
    bottom_layer = design->getTech()->getLayer(bottom_layer_num);
  }
  return bottom_layer->getDir() == dbTechLayerDir::HORIZONTAL;
}

std::string getOrdinalSuffix(int iter)
{
  if (iter == 1 || (iter > 20 && iter % 10 == 1)) {
    return "st";
  }
  if (iter == 2 || (iter > 20 && iter % 10 == 2)) {
    return "nd";
  }
  if (iter == 3 || (iter > 20 && iter % 10 == 3)) {
    return "rd";
  }
  return "th";
}

}  // namespace

int FlexTAWorker::main_mt()
{
  ProfileTask profile("TA:main_mt");
  using std::chrono::high_resolution_clock;
  auto t0 = high_resolution_clock::now();
  if (router_cfg_->VERBOSE > 1) {
    std::stringstream ss;
    ss << std::endl
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
    ss << std::endl;
    std::cout << ss.str();
  }

  init();
  if (isInitTA()) {
    hardIroutesMode_ = true;
    sortIroutes();
    assign();
    hardIroutesMode_ = false;
  }
  sortIroutes();
  auto t1 = high_resolution_clock::now();
  assign();
  auto t2 = high_resolution_clock::now();
  // end();
  auto t3 = high_resolution_clock::now();

  using std::chrono::duration;
  using std::chrono::duration_cast;
  auto time_span0 = duration_cast<duration<double>>(t1 - t0);
  auto time_span1 = duration_cast<duration<double>>(t2 - t1);
  auto time_span2 = duration_cast<duration<double>>(t3 - t2);

  if (router_cfg_->VERBOSE > 1) {
    std::stringstream ss;
    ss << "time (INIT/ASSIGN/POST) " << time_span0.count() << " "
       << time_span1.count() << " " << time_span2.count() << " " << std::endl;
    std::cout << ss.str() << std::flush;
  }
  return 0;
}

FlexTA::FlexTA(frDesign* in,
               utl::Logger* logger,
               RouterConfiguration* router_cfg,
               bool save_updates)
    : tech_(in->getTech()),
      design_(in),
      logger_(logger),
      router_cfg_(router_cfg),
      save_updates_(save_updates)
{
}

FlexTA::~FlexTA() = default;

int FlexTA::initTA_helper(int iter,
                          int size,
                          int offset,
                          bool is_horizontal,
                          int& num_panels)
{
  const auto g_cell_patterns = getDesign()->getTopBlock()->getGCellPatterns();
  const auto& x_gp = g_cell_patterns.at(0);
  const auto& y_gp = g_cell_patterns.at(1);
  int num_assigned = 0;
  num_panels = 0;
  std::vector<std::vector<std::unique_ptr<FlexTAWorker>>> worker_batches;
  if (is_horizontal) {
    for (int i = offset; i < static_cast<int>(y_gp.getCount()); i += size) {
      auto worker_uptr = std::make_unique<FlexTAWorker>(
          getDesign(), logger_, router_cfg_, save_updates_);
      auto& worker = *worker_uptr;
      const odb::Rect begin_box
          = getDesign()->getTopBlock()->getGCellBox(odb::Point(0, i));
      const odb::Rect end_box
          = getDesign()->getTopBlock()->getGCellBox(odb::Point(
              static_cast<int>(x_gp.getCount()) - 1,
              std::min(i + size - 1, static_cast<int>(y_gp.getCount()) - 1)));
      const odb::Rect route_box(
          begin_box.xMin(), begin_box.yMin(), end_box.xMax(), end_box.yMax());
      odb::Rect ext_box;
      route_box.bloat(y_gp.getSpacing() / 2, ext_box);
      worker.setRouteBox(route_box);
      worker.setExtBox(ext_box);
      worker.setDir(dbTechLayerDir::HORIZONTAL);
      worker.setTAIter(iter);
      if (worker_batches.empty()
          || static_cast<int>(worker_batches.back().size())
                 >= router_cfg_->BATCHSIZETA) {
        worker_batches.emplace_back();
      }
      worker_batches.back().emplace_back(std::move(worker_uptr));
    }
  } else {
    for (int i = offset; i < static_cast<int>(x_gp.getCount()); i += size) {
      auto worker_uptr = std::make_unique<FlexTAWorker>(
          getDesign(), logger_, router_cfg_, save_updates_);
      auto& worker = *worker_uptr;
      const odb::Rect begin_box
          = getDesign()->getTopBlock()->getGCellBox(odb::Point(i, 0));
      const odb::Rect end_box
          = getDesign()->getTopBlock()->getGCellBox(odb::Point(
              std::min(i + size - 1, static_cast<int>(x_gp.getCount()) - 1),
              static_cast<int>(y_gp.getCount()) - 1));
      const odb::Rect route_box(
          begin_box.xMin(), begin_box.yMin(), end_box.xMax(), end_box.yMax());
      odb::Rect ext_box;
      route_box.bloat(x_gp.getSpacing() / 2, ext_box);
      worker.setRouteBox(route_box);
      worker.setExtBox(ext_box);
      worker.setDir(dbTechLayerDir::VERTICAL);
      worker.setTAIter(iter);
      if (worker_batches.empty()
          || static_cast<int>(worker_batches.back().size())
                 >= router_cfg_->BATCHSIZETA) {
        worker_batches.emplace_back();
      }
      worker_batches.back().emplace_back(std::move(worker_uptr));
    }
  }

  omp_set_num_threads(std::min(kMaxTaThreads, router_cfg_->MAX_THREADS));
  // Execute each worker batch in parallel and merge assignment counts.
  for (auto& worker_batch : worker_batches) {
    ProfileTask profile("TA:batch");
    utl::ThreadException exception;
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < static_cast<int>(worker_batch.size()); i++) {
      try {
        worker_batch[i]->main_mt();
#pragma omp critical
        {
          num_assigned += worker_batch[i]->getNumAssigned();
          num_panels++;
        }
      } catch (...) {
        exception.capture();
      }
    }
    exception.rethrow();
    for (auto& worker : worker_batch) {
      worker->end();
    }
    worker_batch.clear();
  }
  return num_assigned;
}

void FlexTA::initTA(int size)
{
  ProfileTask profile("TA:init");
  frTime t;

  if (router_cfg_->VERBOSE > 1) {
    std::cout << std::endl << "start initial track assignment ..." << std::endl;
  }

  const bool is_bottom_layer_h = isBottomRoutingLayerHorizontal(getDesign());

  // H first
  if (is_bottom_layer_h) {
    int num_panels_h;
    int num_assigned_h = initTA_helper(0, size, 0, true, num_panels_h);

    int num_panels_v;
    int num_assigned_v = initTA_helper(0, size, 0, false, num_panels_v);

    if (router_cfg_->VERBOSE > 0) {
      logger_->info(DRT,
                    183,
                    "Done with {} horizontal wires in {} frboxes and "
                    "{} vertical wires in {} frboxes.",
                    num_assigned_h,
                    num_panels_h,
                    num_assigned_v,
                    num_panels_v);
    }
    // V first
  } else {
    int num_panels_v;
    int num_assigned_v = initTA_helper(0, size, 0, false, num_panels_v);

    int num_panels_h;
    int num_assigned_h = initTA_helper(0, size, 0, true, num_panels_h);

    if (router_cfg_->VERBOSE > 0) {
      logger_->info(DRT,
                    184,
                    "Done with {} vertical wires in {} frboxes and "
                    "{} horizontal wires in {} frboxes.",
                    num_assigned_v,
                    num_panels_v,
                    num_assigned_h,
                    num_panels_h);
    }
  }
}

void FlexTA::searchRepair(int iter, int size, int offset)
{
  ProfileTask profile("TA:searchRepair");
  frTime t;

  if (router_cfg_->VERBOSE > 1) {
    std::cout << std::endl << "start " << iter;
    std::cout << getOrdinalSuffix(iter) << " optimization iteration ..."
              << std::endl;
  }
  const bool is_bottom_layer_h = isBottomRoutingLayerHorizontal(getDesign());

  // H first
  if (is_bottom_layer_h) {
    int num_panels_h;
    int num_assigned_h = initTA_helper(iter, size, offset, true, num_panels_h);

    int num_panels_v;
    int num_assigned_v = initTA_helper(iter, size, offset, false, num_panels_v);

    if (router_cfg_->VERBOSE > 0) {
      logger_->info(DRT,
                    268,
                    "Done with {} horizontal wires in {} frboxes and "
                    "{} vertical wires in {} frboxes.",
                    num_assigned_h,
                    num_panels_h,
                    num_assigned_v,
                    num_panels_v);
    }
    // V first
  } else {
    int num_panels_v;
    int num_assigned_v = initTA_helper(iter, size, offset, false, num_panels_v);

    int num_panels_h;
    int num_assigned_h = initTA_helper(iter, size, offset, true, num_panels_h);

    if (router_cfg_->VERBOSE > 0) {
      logger_->info(DRT,
                    186,
                    "Done with {} vertical wires in {} frboxes and "
                    "{} horizontal wires in {} frboxes.",
                    num_assigned_v,
                    num_panels_v,
                    num_assigned_h,
                    num_panels_h);
    }
  }
}

void FlexTA::setDebug(std::unique_ptr<AbstractTAGraphics> ta_graphics)
{
  graphics_ = std::move(ta_graphics);
}

int FlexTA::main()
{
  ProfileTask profile("TA:main");

  frTime t;
  if (router_cfg_->VERBOSE > 0) {
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

  if (router_cfg_->VERBOSE > 0) {
    logger_->info(DRT, 182, "Complete track assignment.");
  }
  if (router_cfg_->VERBOSE > 0) {
    t.print(logger_);
  }
  return 0;
}

}  // namespace drt
