// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "gr/FlexGR.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <deque>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include "db/infra/frTime.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frGuide.h"
#include "db/obj/frInst.h"
#include "db/obj/frRPin.h"
#include "frBaseTypes.h"
#include "gr/FlexGRCMap.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "omp.h"
#include "utl/Logger.h"
#include "utl/exception.h"

using odb::dbTechLayerDir;
using utl::ThreadException;

namespace drt {

void FlexGR::main(odb::dbDatabase* db)
{
  db_ = db;
  init();
  // resource analysis
  ra();
  // cmap->print(true);

  FlexGRCMap baseCMap(cmap_.get(), router_cfg_);
  FlexGRCMap baseCMap2D(cmap2D_.get(), router_cfg_);

  // gen topology + pattern route for 2D connectivty
  initGR();
  // populate region query for 2D
  getRegionQuery()->initGRObj();

  getRegionQuery()->printGRObj();

  reportCong2D();

  searchRepairMacro(0,
                    10,
                    2,
                    1 * router_cfg_->CONGCOST,
                    0.5 * router_cfg_->HISTCOST,
                    1.0,
                    true,
                    /*mode*/ RipUpMode::ALL);
  // reportCong2D();
  searchRepairMacro(1,
                    30,
                    2,
                    1 * router_cfg_->CONGCOST,
                    1 * router_cfg_->HISTCOST,
                    0.9,
                    true,
                    /*mode*/ RipUpMode::ALL);
  // reportCong2D();
  searchRepairMacro(2,
                    50,
                    2,
                    1 * router_cfg_->CONGCOST,
                    1.5 * router_cfg_->HISTCOST,
                    0.9,
                    true,
                    RipUpMode::ALL);
  // reportCong2D();
  searchRepairMacro(3,
                    80,
                    2,
                    2 * router_cfg_->CONGCOST,
                    2 * router_cfg_->HISTCOST,
                    0.9,
                    true,
                    RipUpMode::ALL);
  // reportCong2D();

  //  reportCong2D();
  searchRepair(/*iter*/ 0,
               /*size*/ 200,
               /*offset*/ 0,
               /*mazeEndIter*/ 2,
               /*workerCongCost*/ 1 * router_cfg_->CONGCOST,
               /*workerHistCost*/ 0.5 * router_cfg_->HISTCOST,
               /*congThresh*/ 0.9,
               /*is2DRouting*/ true,
               /*mode*/ RipUpMode::ALL,
               /*TEST*/ false);
  // reportCong2D();
  searchRepair(/*iter*/ 1,
               /*size*/ 200,
               /*offset*/ -70,
               /*mazeEndIter*/ 2,
               /*workerCongCost*/ 1 * router_cfg_->CONGCOST,
               /*workerHistCost*/ 1 * router_cfg_->HISTCOST,
               /*congThresh*/ 0.9,
               /*is2DRouting*/ true,
               /*mode*/ RipUpMode::ALL,
               /*TEST*/ false);
  // reportCong2D();
  searchRepair(/*iter*/ 2,
               /*size*/ 200,
               /*offset*/ -150,
               /*mazeEndIter*/ 2,
               /*workerCongCost*/ 2 * router_cfg_->CONGCOST,
               /*workerHistCost*/ 2 * router_cfg_->HISTCOST,
               /*congThresh*/ 0.8,
               /*is2DRouting*/ true,
               /*mode*/ RipUpMode::ALL,
               /*TEST*/ false);
  // reportCong2D();

  reportCong2D();

  layerAssign();

  // populate region query for 3D
  getRegionQuery()->initGRObj();

  // reportCong3D();

  searchRepair(/*iter*/ 0,
               /*size*/ 10,
               /*offset*/ 0,
               /*mazeEndIter*/ 2,
               /*workerCongCost*/ 4 * router_cfg_->CONGCOST,
               /*workerHistCost*/ 0.25 * router_cfg_->HISTCOST,
               /*congThresh*/ 1.0,
               /*is2DRouting*/ false,
               RipUpMode::ALL,
               /*TEST*/ false);
  reportCong3D();
  if (db != nullptr) {
    updateDbCongestion(db, cmap_.get());
  }

  writeToGuide();

  updateDb();
}

void FlexGR::searchRepairMacro(int iter,
                               int size,
                               int mazeEndIter,
                               unsigned workerCongCost,
                               unsigned workerHistCost,
                               double congThresh,
                               bool is2DRouting,
                               RipUpMode mode)
{
  frTime t;

  if (router_cfg_->VERBOSE > 1) {
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
    std::cout << suffix << " optimization iteration for Macro..." << std::endl;
  }

  auto gCellPatterns = getDesign()->getTopBlock()->getGCellPatterns();
  auto& xgp = gCellPatterns.at(0);
  auto& ygp = gCellPatterns.at(1);

  std::vector<std::unique_ptr<FlexGRWorker>> uworkers;

  std::vector<frInst*> macros;

  for (auto& inst : getDesign()->getTopBlock()->getInsts()) {
    if (inst->getMaster()->getMasterType() == odb::dbMasterType::BLOCK) {
      odb::Rect macroBBox = inst->getBBox();
      odb::Point macroCenter((macroBBox.xMin() + macroBBox.xMax()) / 2,
                             (macroBBox.yMin() + macroBBox.yMax()) / 2);
      odb::Point macroCenterIdx
          = getDesign()->getTopBlock()->getGCellIdx(macroCenter);
      if (cmap2D_->hasBlock(
              macroCenterIdx.x(), macroCenterIdx.y(), 0, frDirEnum::E)
          && cmap2D_->hasBlock(
              macroCenterIdx.x(), macroCenterIdx.y(), 0, frDirEnum::N)) {
        macros.push_back(inst.get());
      }
    }
  }

  // create separate worker for each macro
  for (auto macro : macros) {
    auto worker = std::make_unique<FlexGRWorker>(this, router_cfg_);
    odb::Rect macroBBox = macro->getBBox();
    odb::Point macroLL(macroBBox.xMin(), macroBBox.yMin());
    odb::Point macroUR(macroBBox.xMax(), macroBBox.yMax());
    odb::Point gcellIdxLL = getDesign()->getTopBlock()->getGCellIdx(macroLL);
    odb::Point gcellIdxUR = getDesign()->getTopBlock()->getGCellIdx(macroUR);

    gcellIdxLL = {std::max((int) gcellIdxLL.x() - size, 0),
                  std::max((int) gcellIdxLL.y() - size, 0)};
    gcellIdxUR = {std::min((int) gcellIdxUR.x() + size, (int) xgp.getCount()),
                  std::min((int) gcellIdxUR.y() + size, (int) ygp.getCount())};

    odb::Rect routeBox1 = getDesign()->getTopBlock()->getGCellBox(gcellIdxLL);
    odb::Rect routeBox2 = getDesign()->getTopBlock()->getGCellBox(gcellIdxUR);
    odb::Rect extBox(
        routeBox1.xMin(), routeBox1.yMin(), routeBox2.xMax(), routeBox2.yMax());
    odb::Rect routeBox((routeBox1.xMin() + routeBox1.xMax()) / 2,
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

    uworkers.push_back(std::move(worker));
  }

  // omp_set_num_threads(1);
  // currently this is not mt-safe
  for (auto& worker : uworkers) {
    worker->initBoundary();
    worker->main_mt();
    worker->end();
  }
  uworkers.clear();
}

void FlexGR::searchRepair(int iter,
                          int size,
                          int offset,
                          int mazeEndIter,
                          unsigned workerCongCost,
                          unsigned workerHistCost,
                          double congThresh,
                          bool is2DRouting,
                          RipUpMode mode,
                          bool TEST)
{
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

  if (TEST) {
    std::cout << "search and repair test mode" << std::endl << std::flush;

    FlexGRWorker worker(this, router_cfg_);
    odb::Rect extBox(1847999, 440999, 1857000, 461999);
    odb::Rect routeBox(1849499, 442499, 1855499, 460499);
    odb::Point gcellIdxLL(616, 147);
    odb::Point gcellIdxUR(618, 153);

    worker.setRouteGCellIdxLL(gcellIdxLL);
    worker.setRouteGCellIdxUR(gcellIdxUR);
    worker.setExtBox(extBox);
    worker.setRouteBox(routeBox);
    worker.setMazeEndIter(0);
    worker.setGRIter(0);
    worker.setCongCost(workerCongCost);
    worker.setHistCost(workerHistCost);
    worker.setCongThresh(congThresh);
    worker.set2D(is2DRouting);
    worker.setRipupMode(mode);

    worker.initBoundary();
    worker.main_mt();
    worker.end();

  } else {
    std::vector<std::unique_ptr<FlexGRWorker>> uworkers;
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
        odb::Point gcellIdxLL = odb::Point(i, j);
        odb::Point gcellIdxUR
            = odb::Point(std::min((int) xgp.getCount() - 1, i + size - 1),
                         std::min((int) ygp.getCount(), j + size - 1));

        odb::Rect routeBox1
            = getDesign()->getTopBlock()->getGCellBox(gcellIdxLL);
        odb::Rect routeBox2
            = getDesign()->getTopBlock()->getGCellBox(gcellIdxUR);
        odb::Rect extBox(routeBox1.xMin(),
                         routeBox1.yMin(),
                         routeBox2.xMax(),
                         routeBox2.yMax());
        odb::Rect routeBox((routeBox1.xMin() + routeBox1.xMax()) / 2,
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
  }

  t.print(logger_);
  std::cout << std::endl << std::flush;
}

void FlexGR::reportCong2DGolden(FlexGRCMap* baseCMap2D)
{
  FlexGRCMap goldenCMap2D(baseCMap2D, router_cfg_);

  for (auto& net : design_->getTopBlock()->getNets()) {
    for (auto& uGRShape : net->getGRShapes()) {
      auto ps = static_cast<grPathSeg*>(uGRShape.get());
      auto [bp, ep] = ps->getPoints();

      odb::Point bpIdx = design_->getTopBlock()->getGCellIdx(bp);
      odb::Point epIdx = design_->getTopBlock()->getGCellIdx(ep);

      // update golden 2D congestion map
      unsigned zIdx = 0;
      if (bpIdx.y() == epIdx.y()) {
        for (int xIdx = bpIdx.x(); xIdx < epIdx.x(); xIdx++) {
          goldenCMap2D.addRawDemand(xIdx, bpIdx.y(), zIdx, frDirEnum::E);
          goldenCMap2D.addRawDemand(xIdx + 1, bpIdx.y(), zIdx, frDirEnum::E);
        }
      } else {
        for (int yIdx = bpIdx.y(); yIdx < epIdx.y(); yIdx++) {
          goldenCMap2D.addRawDemand(bpIdx.x(), yIdx, zIdx, frDirEnum::N);
          goldenCMap2D.addRawDemand(bpIdx.x(), yIdx + 1, zIdx, frDirEnum::N);
        }
      }
    }
  }

  std::cout << "start reporting golden 2D congestion...";
  reportCong2D(&goldenCMap2D);
}

void FlexGR::reportCong2D(FlexGRCMap* cmap2D)
{
  if (router_cfg_->VERBOSE > 0) {
    std::cout << std::endl << "start reporting 2D congestion ...\n\n";
  }

  auto& gCellPatterns = design_->getTopBlock()->getGCellPatterns();
  auto xgp = &(gCellPatterns.at(0));
  auto ygp = &(gCellPatterns.at(1));

  std::cout << "              #OverCon  %OverCon" << std::endl;
  std::cout << "Direction        GCell     GCell" << std::endl;
  std::cout << "--------------------------------" << std::endl;

  int numGCell = xgp->getCount() * ygp->getCount();
  int numOverConGCellH = 0;
  int numOverConGCellV = 0;
  double worstConH = 0.0;
  double worstConV = 0.0;
  std::vector<double> conH;
  std::vector<double> conV;

  for (unsigned xIdx = 0; xIdx < xgp->getCount(); xIdx++) {
    for (unsigned yIdx = 0; yIdx < ygp->getCount(); yIdx++) {
      // H
      // update num overCon GCell
      auto supplyH = cmap2D->getSupply(xIdx, yIdx, 0, frDirEnum::E);
      auto demandH = cmap2D->getDemand(xIdx, yIdx, 0, frDirEnum::E);
      if (demandH > supplyH
          && cmap2D->hasBlock(xIdx, yIdx, 0, frDirEnum::E) == false) {
        numOverConGCellH++;
      }
      // update worstCon
      if ((demandH * 100.0 / supplyH) > worstConH
          && cmap2D->hasBlock(xIdx, yIdx, 0, frDirEnum::E) == false) {
        worstConH = demandH * 100.0 / supplyH;
      }
      if (cmap2D->hasBlock(xIdx, yIdx, 0, frDirEnum::E) == false) {
        conH.push_back(demandH * 100.0 / supplyH);
      }
      // V
      // update num overCon GCell
      auto supplyV = cmap2D->getSupply(xIdx, yIdx, 0, frDirEnum::N);
      auto demandV = cmap2D->getDemand(xIdx, yIdx, 0, frDirEnum::N);
      if (demandV > supplyV
          && cmap2D->hasBlock(xIdx, yIdx, 0, frDirEnum::N) == false) {
        numOverConGCellV++;
      }
      // update worstCon
      if ((demandV * 100.0 / supplyV) > worstConV
          && cmap2D->hasBlock(xIdx, yIdx, 0, frDirEnum::N) == false) {
        worstConV = demandV * 100.0 / supplyV;
      }
      if (cmap2D->hasBlock(xIdx, yIdx, 0, frDirEnum::N) == false) {
        conV.push_back(demandV * 100.0 / supplyV);
      }
    }
  }

  sort(conH.begin(), conH.end());
  sort(conV.begin(), conV.end());

  std::cout << "    H        " << std::setw(8) << numOverConGCellH << "   "
            << std::setw(6) << std::fixed << numOverConGCellH * 100.0 / numGCell
            << "%\n";
  std::cout << "    V        " << std::setw(8) << numOverConGCellV << "   "
            << std::setw(6) << std::fixed << numOverConGCellV * 100.0 / numGCell
            << "%\n";
  std::cout << "worstConH: " << std::setw(6) << std::fixed << worstConH
            << "%\n";
  // std::cout << "  25-pencentile congestion H: " << conH[int(conH.size() *
  // 0.25)]
  // << "%\n"; std::cout << "  50-pencentile congestion H: " <<
  // conH[int(conH.size()
  // * 0.50)] << "%\n"; std::cout << "  75-pencentile congestion H: " <<
  // conH[int(conH.size() * 0.75)] << "%\n"; std::cout << "  90-pencentile
  // congestion H: " << conH[int(conH.size() * 0.90)] << "%\n"; std::cout << "
  // 95-pencentile congestion H: " << conH[int(conH.size() * 0.95)] << "%\n";
  std::cout << "worstConV: " << std::setw(6) << std::fixed << worstConV
            << "%\n";
  // std::cout << "  25-pencentile congestion V: " << conV[int(conV.size() *
  // 0.25)]
  // << "%\n"; std::cout << "  50-pencentile congestion V: " <<
  // conV[int(conV.size()
  // * 0.50)] << "%\n"; std::cout << "  75-pencentile congestion V: " <<
  // conV[int(conV.size() * 0.75)] << "%\n"; std::cout << "  90-pencentile
  // congestion V: " << conV[int(conV.size() * 0.90)] << "%\n"; std::cout << "
  // 95-pencentile congestion V: " << conV[int(conV.size() * 0.95)] << "%\n";

  std::cout << std::endl;
}

void FlexGR::reportCong2D()
{
  if (router_cfg_->VERBOSE > 0) {
    std::cout << std::endl << "start reporting 2D congestion ...\n\n";
  }

  auto& gCellPatterns = design_->getTopBlock()->getGCellPatterns();
  auto xgp = &(gCellPatterns.at(0));
  auto ygp = &(gCellPatterns.at(1));

  std::cout << "              #OverCon  %OverCon" << std::endl;
  std::cout << "Direction        GCell     GCell" << std::endl;
  std::cout << "--------------------------------" << std::endl;

  int numGCell = xgp->getCount() * ygp->getCount();
  int numOverConGCellH = 0;
  int numOverConGCellV = 0;
  double worstConH = 0.0;
  double worstConV = 0.0;
  std::vector<double> conH;
  std::vector<double> conV;

  for (unsigned xIdx = 0; xIdx < xgp->getCount(); xIdx++) {
    for (unsigned yIdx = 0; yIdx < ygp->getCount(); yIdx++) {
      // H
      // update num overCon GCell
      auto supplyH = cmap2D_->getSupply(xIdx, yIdx, 0, frDirEnum::E);
      auto demandH = cmap2D_->getDemand(xIdx, yIdx, 0, frDirEnum::E);
      if (demandH > supplyH
          && cmap2D_->hasBlock(xIdx, yIdx, 0, frDirEnum::E) == false) {
        numOverConGCellH++;
      }
      // update worstCon
      if ((demandH * 100.0 / supplyH) > worstConH
          && cmap2D_->hasBlock(xIdx, yIdx, 0, frDirEnum::E) == false) {
        worstConH = demandH * 100.0 / supplyH;
      }
      if (cmap2D_->hasBlock(xIdx, yIdx, 0, frDirEnum::E) == false) {
        conH.push_back(demandH * 100.0 / supplyH);
      }
      // V
      // update num overCon GCell
      auto supplyV = cmap2D_->getSupply(xIdx, yIdx, 0, frDirEnum::N);
      auto demandV = cmap2D_->getDemand(xIdx, yIdx, 0, frDirEnum::N);
      if (demandV > supplyV
          && cmap2D_->hasBlock(xIdx, yIdx, 0, frDirEnum::N) == false) {
        numOverConGCellV++;
      }
      // update worstCon
      if ((demandV * 100.0 / supplyV) > worstConV
          && cmap2D_->hasBlock(xIdx, yIdx, 0, frDirEnum::N) == false) {
        worstConV = demandV * 100.0 / supplyV;
      }
      if (cmap2D_->hasBlock(xIdx, yIdx, 0, frDirEnum::N) == false) {
        conV.push_back(demandV * 100.0 / supplyV);
      }
    }
  }

  sort(conH.begin(), conH.end());
  sort(conV.begin(), conV.end());

  std::cout << "    H        " << std::setw(8) << numOverConGCellH << "   "
            << std::setw(6) << std::fixed << numOverConGCellH * 100.0 / numGCell
            << "%\n";
  std::cout << "    V        " << std::setw(8) << numOverConGCellV << "   "
            << std::setw(6) << std::fixed << numOverConGCellV * 100.0 / numGCell
            << "%\n";
  std::cout << "worstConH: " << std::setw(6) << std::fixed << worstConH
            << "%\n";
  // std::cout << "  25-pencentile congestion H: " << conH[int(conH.size() *
  // 0.25)]
  // << "%\n"; std::cout << "  50-pencentile congestion H: " <<
  // conH[int(conH.size()
  // * 0.50)] << "%\n"; std::cout << "  75-pencentile congestion H: " <<
  // conH[int(conH.size() * 0.75)] << "%\n"; std::cout << "  90-pencentile
  // congestion H: " << conH[int(conH.size() * 0.90)] << "%\n"; std::cout << "
  // 95-pencentile congestion H: " << conH[int(conH.size() * 0.95)] << "%\n";
  std::cout << "worstConV: " << std::setw(6) << std::fixed << worstConV
            << "%\n";
  // std::cout << "  25-pencentile congestion V: " << conV[int(conV.size() *
  // 0.25)]
  // << "%\n"; std::cout << "  50-pencentile congestion V: " <<
  // conV[int(conV.size()
  // * 0.50)] << "%\n"; std::cout << "  75-pencentile congestion V: " <<
  // conV[int(conV.size() * 0.75)] << "%\n"; std::cout << "  90-pencentile
  // congestion V: " << conV[int(conV.size() * 0.90)] << "%\n"; std::cout << "
  // 95-pencentile congestion V: " << conV[int(conV.size() * 0.95)] << "%\n";

  std::cout << std::endl;
}

void FlexGR::reportCong3DGolden(FlexGRCMap* baseCMap)
{
  FlexGRCMap goldenCMap3D(baseCMap, router_cfg_);

  for (auto& net : design_->getTopBlock()->getNets()) {
    for (auto& uGRShape : net->getGRShapes()) {
      auto ps = static_cast<grPathSeg*>(uGRShape.get());
      auto [bp, ep] = ps->getPoints();
      frLayerNum lNum = ps->getLayerNum();

      odb::Point bpIdx = design_->getTopBlock()->getGCellIdx(bp);
      odb::Point epIdx = design_->getTopBlock()->getGCellIdx(ep);

      // update golden 3D congestion map
      unsigned zIdx = lNum / 2 - 1;
      if (bpIdx.y() == epIdx.y()) {
        for (int xIdx = bpIdx.x(); xIdx < epIdx.x(); xIdx++) {
          goldenCMap3D.addRawDemand(xIdx, bpIdx.y(), zIdx, frDirEnum::E);
          goldenCMap3D.addRawDemand(xIdx + 1, bpIdx.y(), zIdx, frDirEnum::E);
        }
      } else {
        for (int yIdx = bpIdx.y(); yIdx < epIdx.y(); yIdx++) {
          goldenCMap3D.addRawDemand(bpIdx.x(), yIdx, zIdx, frDirEnum::N);
          goldenCMap3D.addRawDemand(bpIdx.x(), yIdx + 1, zIdx, frDirEnum::N);
        }
      }
    }
  }

  std::cout << "start reporting golden 3D congestion...";
  reportCong3D(&goldenCMap3D);
}

void FlexGR::updateDbCongestion(odb::dbDatabase* db, FlexGRCMap* cmap)
{
  if (db->getChip() == nullptr || db->getChip()->getBlock() == nullptr
      || db->getTech() == nullptr) {
    logger_->error(utl::DRT, 201, "Must load design before global routing.");
  }
  auto block = db->getChip()->getBlock();
  auto tech = db->getTech();
  auto gcell = block->getGCellGrid();
  if (gcell == nullptr) {
    gcell = odb::dbGCellGrid::create(block);
  } else {
    logger_->warn(
        utl::DRT,
        203,
        "dbGcellGrid already exists in db. Clearing existing dbGCellGrid.");
    gcell->resetGrid();
  }
  auto& gCellPatterns = design_->getTopBlock()->getGCellPatterns();
  auto xgp = &(gCellPatterns.at(0));
  auto ygp = &(gCellPatterns.at(1));
  gcell->addGridPatternX(
      xgp->getStartCoord(), xgp->getCount(), xgp->getSpacing());
  gcell->addGridPatternY(
      ygp->getStartCoord(), ygp->getCount(), ygp->getSpacing());
  odb::Rect dieBox = design_->getTopBlock()->getDieBox();
  gcell->addGridPatternX(dieBox.xMax(), 1, 0);
  gcell->addGridPatternY(dieBox.yMax(), 1, 0);
  unsigned cmapLayerIdx = 0;
  for (auto& [layerNum, dir] : cmap->getZMap()) {
    std::string layerName(design_->getTech()->getLayer(layerNum)->getName());
    auto layer = tech->findLayer(layerName.c_str());
    if (layer == nullptr) {
      logger_->warn(utl::DRT,
                    202,
                    "Skipping layer {} not found in db for congestion map.",
                    layerName);
      cmapLayerIdx++;
      continue;
    }
    for (unsigned xIdx = 0; xIdx < xgp->getCount(); xIdx++) {
      for (unsigned yIdx = 0; yIdx < ygp->getCount(); yIdx++) {
        auto horizontal_capacity
            = cmap->getRawSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::E);
        auto horizontal_usage
            = cmap->getRawDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::E);
        auto vertical_capacity
            = cmap->getRawSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::N);
        auto vertical_usage
            = cmap->getRawDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::N);
        gcell->setCapacity(
            layer, xIdx, yIdx, horizontal_capacity + vertical_capacity);
        gcell->setUsage(layer, xIdx, yIdx, horizontal_usage + vertical_usage);
      }
    }
    cmapLayerIdx++;
  }
}

void FlexGR::reportCong3D(FlexGRCMap* cmap)
{
  if (router_cfg_->VERBOSE > 0) {
    std::cout << std::endl << "start reporting 3D congestion ...\n\n";
  }

  auto& gCellPatterns = design_->getTopBlock()->getGCellPatterns();
  auto xgp = &(gCellPatterns.at(0));
  auto ygp = &(gCellPatterns.at(1));

  int numGCell = xgp->getCount() * ygp->getCount();
  int numOverConGCell = 0;

  unsigned cmapLayerIdx = 0;
  for (auto& [layerNum, dir] : cmap->getZMap()) {
    std::vector<double> con;
    numOverConGCell = 0;

    auto layer = design_->getTech()->getLayer(layerNum);
    std::string layerName(layer->getName());
    std::cout << "---------- " << layerName << " ----------" << std::endl;
    // get congestion information
    for (unsigned xIdx = 0; xIdx < xgp->getCount(); xIdx++) {
      for (unsigned yIdx = 0; yIdx < ygp->getCount(); yIdx++) {
        if (layer->getDir() == dbTechLayerDir::HORIZONTAL) {
          auto supply
              = cmap->getRawSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::E);
          auto demand
              = cmap->getRawDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::E);
          if (demand > supply
              && cmap->hasBlock(xIdx, yIdx, cmapLayerIdx, frDirEnum::E)
                     == false) {
            numOverConGCell++;
          }
          if (cmap->hasBlock(xIdx, yIdx, cmapLayerIdx, frDirEnum::E) == false) {
            con.push_back(demand * 100.0 / supply);
          }
        } else {
          auto supply
              = cmap->getRawSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::N);
          auto demand
              = cmap->getRawDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::N);
          if (demand > supply
              && cmap->hasBlock(xIdx, yIdx, cmapLayerIdx, frDirEnum::N)
                     == false) {
            numOverConGCell++;
          }
          if (cmap->hasBlock(xIdx, yIdx, cmapLayerIdx, frDirEnum::N) == false) {
            con.push_back(demand * 100.0 / supply);
          }
        }
      }
    }

    sort(con.begin(), con.end());

    std::cout << "numOverConGCell: " << numOverConGCell
              << ", %OverConGCell: " << std::setw(6) << std::fixed
              << numOverConGCell * 100.0 / numGCell << "%\n";

    // std::cout << "  25-pencentile congestion: " << con[int(con.size() *
    // 0.25)] <<
    // "%\n"; std::cout << "  50-pencentile congestion: " << con[int(con.size()
    // * 0.50)] << "%\n"; std::cout << "  75-pencentile congestion: " <<
    // con[int(con.size() * 0.75)] << "%\n"; std::cout << "  90-pencentile
    // congestion: " << con[int(con.size() * 0.90)] << "%\n"; std::cout << "
    // 95-pencentile congestion: " << con[int(con.size() * 0.95)] << "%\n";

    cmapLayerIdx++;
  }

  std::cout << std::endl;
}

void FlexGR::reportCong3D()
{
  if (router_cfg_->VERBOSE > 0) {
    std::cout << std::endl << "start reporting 3D congestion ...\n\n";
  }

  auto& gCellPatterns = design_->getTopBlock()->getGCellPatterns();
  auto xgp = &(gCellPatterns.at(0));
  auto ygp = &(gCellPatterns.at(1));

  int numGCell = xgp->getCount() * ygp->getCount();
  int numOverConGCell = 0;

  unsigned cmapLayerIdx = 0;
  for (auto& [layerNum, dir] : cmap_->getZMap()) {
    std::vector<double> con;
    numOverConGCell = 0;

    auto layer = design_->getTech()->getLayer(layerNum);
    std::string layerName(layer->getName());
    std::cout << "---------- " << layerName << " ----------" << std::endl;
    // get congestion information
    for (unsigned xIdx = 0; xIdx < xgp->getCount(); xIdx++) {
      for (unsigned yIdx = 0; yIdx < ygp->getCount(); yIdx++) {
        if (layer->getDir() == dbTechLayerDir::HORIZONTAL) {
          auto supply
              = cmap_->getRawSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::E);
          auto demand
              = cmap_->getRawDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::E);
          if (demand > supply
              && cmap_->hasBlock(xIdx, yIdx, cmapLayerIdx, frDirEnum::E)
                     == false) {
            numOverConGCell++;
          }
          if (cmap_->hasBlock(xIdx, yIdx, cmapLayerIdx, frDirEnum::E)
              == false) {
            con.push_back(demand * 100.0 / supply);
          }
        } else {
          auto supply
              = cmap_->getRawSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::N);
          auto demand
              = cmap_->getRawDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::N);
          if (demand > supply
              && cmap_->hasBlock(xIdx, yIdx, cmapLayerIdx, frDirEnum::N)
                     == false) {
            numOverConGCell++;
          }
          if (cmap_->hasBlock(xIdx, yIdx, cmapLayerIdx, frDirEnum::N)
              == false) {
            con.push_back(demand * 100.0 / supply);
          }
        }
      }
    }

    sort(con.begin(), con.end());

    std::cout << "numOverConGCell: " << numOverConGCell
              << ", %OverConGCell: " << std::setw(6) << std::fixed
              << numOverConGCell * 100.0 / numGCell << "%\n";

    // std::cout << "  25-pencentile congestion: " << con[int(con.size() *
    // 0.25)] <<
    // "%\n"; std::cout << "  50-pencentile congestion: " << con[int(con.size()
    // * 0.50)] << "%\n"; std::cout << "  75-pencentile congestion: " <<
    // con[int(con.size() * 0.75)] << "%\n"; std::cout << "  90-pencentile
    // congestion: " << con[int(con.size() * 0.90)] << "%\n"; std::cout << "
    // 95-pencentile congestion: " << con[int(con.size() * 0.95)] << "%\n";

    cmapLayerIdx++;
  }

  std::cout << std::endl;
}

// resource analysis
void FlexGR::ra()
{
  frTime t;
  if (router_cfg_->VERBOSE > 0) {
    std::cout << std::endl << "start routing resource analysis ...\n\n";
  }

  auto& gCellPatterns = design_->getTopBlock()->getGCellPatterns();
  auto xgp = &(gCellPatterns.at(0));
  auto ygp = &(gCellPatterns.at(1));

  int totNumTrack = 0;
  int totNumBlockedTrack = 0;
  int totNumGCell = 0;
  int totNumBlockedGCell = 0;

  std::cout << "             Routing   #Avail     #Track     #Total     %Gcell"
            << std::endl;
  std::cout << "Layer      Direction    Track    Blocked      Gcell    Blocked"
            << std::endl;
  std::cout << "--------------------------------------------------------------"
            << std::endl;

  unsigned cmapLayerIdx = 0;
  for (auto& [layerNum, dir] : cmap_->getZMap()) {
    auto layer = design_->getTech()->getLayer(layerNum);
    std::string layerName(layer->getName());
    layerName.append(16 - layerName.size(), ' ');
    std::cout << layerName;

    int numTrack = 0;
    int numBlockedTrack = 0;
    int numGCell = xgp->getCount() * ygp->getCount();
    int numBlockedGCell = 0;
    if (dir == dbTechLayerDir::HORIZONTAL) {
      std::cout << "H      ";
      for (unsigned yIdx = 0; yIdx < ygp->getCount(); yIdx++) {
        numTrack += cmap_->getSupply(0, yIdx, cmapLayerIdx, frDirEnum::E);
        for (unsigned xIdx = 0; xIdx < xgp->getCount(); xIdx++) {
          auto supply
              = cmap_->getSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::E);
          auto demand
              = cmap_->getDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::E);
          if (demand >= supply) {
            // if (cmapLayerIdx == 0) {
            // std::cout << "blocked gcell: xIdx = " << xIdx << ", yIdx = " <<
            // yIdx
            // << ", supply = " << supply << ", demand = " << demand <<
            // std::endl;
            // }
            numBlockedGCell++;
          }
        }
      }
    } else if (dir == dbTechLayerDir::VERTICAL) {
      std::cout << "V      ";
      for (unsigned xIdx = 0; xIdx < xgp->getCount(); xIdx++) {
        numTrack += cmap_->getSupply(xIdx, 0, cmapLayerIdx, frDirEnum::N);
        for (unsigned yIdx = 0; yIdx < ygp->getCount(); yIdx++) {
          auto supply
              = cmap_->getSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::N);
          auto demand
              = cmap_->getDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::N);
          if (demand >= supply) {
            numBlockedGCell++;
          }
        }
      }
    } else {
      std::cout << "UNKNOWN";
    }

    std::cout << std::setw(6) << numTrack << "     ";

    std::cout << std::setw(6) << numBlockedTrack << "     ";

    std::cout << std::setw(6) << numGCell << "    ";

    std::cout << std::setw(6) << std::fixed << std::setprecision(2)
              << numBlockedGCell * 100.0 / numGCell << "%\n";

    // add to total
    totNumTrack += numTrack;
    totNumBlockedTrack += numBlockedTrack;
    totNumGCell += numGCell;
    totNumBlockedGCell += numBlockedGCell;

    cmapLayerIdx++;
  }

  std::cout
      << "--------------------------------------------------------------\n";
  std::cout << "Total                  ";
  std::cout << std::setw(6) << totNumTrack << "     ";
  std::cout << std::setw(5) << std::fixed << std::setprecision(2)
            << totNumBlockedTrack * 100.0 / totNumTrack << "%    ";
  std::cout << std::setw(7) << totNumGCell << "     ";
  std::cout << std::setw(5) << std::fixed << std::setprecision(2)
            << totNumBlockedGCell * 100.0 / totNumGCell << "%\n";

  if (router_cfg_->VERBOSE > 0) {
    std::cout << std::endl;
    t.print(logger_);
  }

  std::cout << std::endl << std::endl;
}

// information to be reported after each iteration
void FlexGR::end()
{
}

void FlexGR::initGR()
{
  // check rpin and node equivalence
  for (auto& net : design_->getTopBlock()->getNets()) {
    // std::cout << net->getName() << " " << net->getNodes().size() << " " <<
    // net->getRPins().size() << "\n";
    if (net->getNodes().size() != net->getRPins().size()) {
      std::cout << "Error: net " << net->getName()
                << " initial #node != #rpin\n";
    }
  }

  // generate topology
  initGR_genTopology();

  initGR_patternRoute();

  initGR_initObj();

  // cmap->print2D(true);
  // cmap2D->print2D(true);

  // cmap->print();

  // layerAssign();

  // cmap->print();
}

// update congestion for colinear route (between child and parent)
void FlexGR::initGR_updateCongestion()
{
  for (auto& net : design_->getTopBlock()->getNets()) {
    initGR_updateCongestion_net(net.get());
  }
}

void FlexGR::initGR_updateCongestion_net(frNet* net)
{
  for (auto& node : net->getNodes()) {
    if (node->getParent() == nullptr) {
      continue;
    }
    // only create shape and update congestion if haven't done before
    if (node->getConnFig()) {
      continue;
    }

    if (node->getType() != frNodeTypeEnum::frcSteiner
        || node->getParent()->getType() != frNodeTypeEnum::frcSteiner) {
      continue;
    }
    odb::Point loc = node->getLoc();
    odb::Point parentLoc = node->getParent()->getLoc();
    if (loc.x() != parentLoc.x() && loc.y() != parentLoc.y()) {
      continue;
    }

    // generate shape and update 2D congestion map
    odb::Point bp, ep;
    if (loc < parentLoc) {
      bp = loc;
      ep = parentLoc;
    } else {
      bp = parentLoc;
      ep = loc;
    }

    odb::Point bpIdx = design_->getTopBlock()->getGCellIdx(bp);
    odb::Point epIdx = design_->getTopBlock()->getGCellIdx(ep);

    // update 3D congestion map
    unsigned zIdx = 0;
    if (bpIdx.y() == epIdx.y()) {
      for (int xIdx = bpIdx.x(); xIdx < epIdx.x(); xIdx++) {
        cmap_->addRawDemand(xIdx, bpIdx.y(), zIdx, frDirEnum::E);
        cmap_->addRawDemand(xIdx + 1, bpIdx.y(), zIdx, frDirEnum::E);
      }
    } else {
      for (int yIdx = bpIdx.y(); yIdx < epIdx.y(); yIdx++) {
        cmap_->addRawDemand(bpIdx.x(), yIdx, zIdx, frDirEnum::N);
        cmap_->addRawDemand(bpIdx.x(), yIdx + 1, zIdx, frDirEnum::N);
      }
    }
  }
}

void FlexGR::initGR_updateCongestion2D_net(frNet* net)
{
  for (auto& node : net->getNodes()) {
    if (node->getParent() == nullptr) {
      continue;
    }

    if (node->getType() != frNodeTypeEnum::frcSteiner
        || node->getParent()->getType() != frNodeTypeEnum::frcSteiner) {
      continue;
    }
    odb::Point loc = node->getLoc();
    odb::Point parentLoc = node->getParent()->getLoc();
    if (loc.x() != parentLoc.x() && loc.y() != parentLoc.y()) {
      continue;
    }

    // generate shape and update 2D congestion map
    odb::Point bp, ep;
    if (loc < parentLoc) {
      bp = loc;
      ep = parentLoc;
    } else {
      bp = parentLoc;
      ep = loc;
    }

    odb::Point bpIdx = design_->getTopBlock()->getGCellIdx(bp);
    odb::Point epIdx = design_->getTopBlock()->getGCellIdx(ep);

    // update 2D congestion map
    unsigned zIdx = 0;
    if (bpIdx.y() == epIdx.y()) {
      for (int xIdx = bpIdx.x(); xIdx < epIdx.x(); xIdx++) {
        cmap2D_->addRawDemand(xIdx, bpIdx.y(), zIdx, frDirEnum::E);
        cmap2D_->addRawDemand(xIdx + 1, bpIdx.y(), zIdx, frDirEnum::E);
      }
    } else {
      for (int yIdx = bpIdx.y(); yIdx < epIdx.y(); yIdx++) {
        cmap2D_->addRawDemand(bpIdx.x(), yIdx, zIdx, frDirEnum::N);
        cmap2D_->addRawDemand(bpIdx.x(), yIdx + 1, zIdx, frDirEnum::N);
      }
    }
  }
}

// if topology is from Flute, there will be non-colinear route need to be
// pattern routed
void FlexGR::initGR_patternRoute()
{
  std::vector<std::pair<std::pair<frNode*, frNode*>, int>>
      patternRoutes;  // <childNode, parentNode>, ripup cnt
  // init
  initGR_patternRoute_init(patternRoutes);
  // route
  initGR_patternRoute_route(patternRoutes);
}

void FlexGR::initGR_patternRoute_init(
    std::vector<std::pair<std::pair<frNode*, frNode*>, int>>& patternRoutes)
{
  for (auto& net : design_->getTopBlock()->getNets()) {
    for (auto& node : net->getNodes()) {
      frNode* parentNode = node->getParent();
      if (parentNode == nullptr) {
        continue;
      }

      if (node->getType() != frNodeTypeEnum::frcSteiner
          || parentNode->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }

      odb::Point loc = node->getLoc();
      odb::Point parentLoc = parentNode->getLoc();
      if (loc.x() == parentLoc.x() || loc.y() == parentLoc.y()) {
        continue;
      }

      patternRoutes.emplace_back(std::make_pair(node.get(), parentNode), 0);
    }
  }
}

void FlexGR::initGR_patternRoute_route(
    std::vector<std::pair<std::pair<frNode*, frNode*>, int>>& patternRoutes)
{
  int maxIter = 2;
  for (int iter = 0; iter < maxIter; iter++) {
    initGR_patternRoute_route_iter(iter, patternRoutes, /*mode*/ 0);
  }
}

// mode 0 == L shape only
bool FlexGR::initGR_patternRoute_route_iter(
    int iter,
    std::vector<std::pair<std::pair<frNode*, frNode*>, int>>& patternRoutes,
    int mode)
{
  bool hasOverflow = false;
  for (auto& patternRoutePair : patternRoutes) {
    auto& patternRoute = patternRoutePair.first;
    auto startNode = patternRoute.first;
    auto endNode = patternRoute.second;
    auto net = startNode->getNet();
    auto& rerouteCnt = patternRoutePair.second;
    bool doRoute = false;
    // the route has not been routed yet
    if (rerouteCnt == 0) {
      doRoute = true;
    }
    // check overflow along the path
    if (startNode->getParent() != endNode) {
      auto currNode = startNode;
      while (currNode != endNode) {
        if (hasOverflow2D(currNode, currNode->getParent())) {
          doRoute = true;
          break;
        }
        currNode = currNode->getParent();
      }
    }
    if (doRoute) {
      // ripup pattern routed wire and update congestion map
      if (rerouteCnt > 0) {
        auto currNode = startNode;
        while (currNode != endNode) {
          ripupRoute(currNode, currNode->getParent());
          // remove from endNode if parent is endNode
          if (currNode->getParent() == endNode) {
            endNode->removeChild(currNode);
          }
          if (currNode != startNode && currNode != endNode) {
            net->removeNode(currNode);
          }
          currNode = currNode->getParent();
        }

        // restore connection from start node to end node
        startNode->setParent(endNode);
        endNode->addChild(startNode);
      }
      // find current best route based on mode and update congestion map
      switch (mode) {
        case 0:
          patternRoute_LShape(startNode, endNode);
          break;
        case 1:
          break;
        default:;
      }
      rerouteCnt++;
    }
  }
  return hasOverflow;
}

void FlexGR::patternRoute_LShape(frNode* child, frNode* parent)
{
  auto net = child->getNet();
  odb::Point childLoc = child->getLoc();
  odb::Point parentLoc = parent->getLoc();

  odb::Point childGCellIdx = design_->getTopBlock()->getGCellIdx(childLoc);
  odb::Point parentGCellIdx = design_->getTopBlock()->getGCellIdx(parentLoc);

  odb::Point cornerGCellIdx1(childGCellIdx.x(), parentGCellIdx.y());
  odb::Point cornerGCellIdx2(parentGCellIdx.x(), childGCellIdx.y());

  // calculate corner1 cost
  double corner1Cost = 0;
  for (int xIdx = std::min(cornerGCellIdx1.x(), parentGCellIdx.x());
       xIdx <= std::max(cornerGCellIdx1.x(), parentGCellIdx.x());
       xIdx++) {
    auto rawSupply
        = cmap2D_->getRawSupply(xIdx, cornerGCellIdx1.y(), 0, frDirEnum::E);
    auto rawDemand
        = cmap2D_->getRawDemand(xIdx, cornerGCellIdx1.y(), 0, frDirEnum::E);
    corner1Cost += getCongCost(rawSupply, rawDemand);
    if (cmap2D_->getRawDemand(xIdx, cornerGCellIdx1.y(), 0, frDirEnum::E)
        >= cmap2D_->getRawSupply(xIdx, cornerGCellIdx1.y(), 0, frDirEnum::E)) {
      corner1Cost += router_cfg_->BLOCKCOST;
    }
    if (cmap2D_->hasBlock(xIdx, cornerGCellIdx1.y(), 0, frDirEnum::E)) {
      corner1Cost += router_cfg_->BLOCKCOST * 100;
    }
  }
  for (int yIdx = std::min(cornerGCellIdx1.y(), childGCellIdx.y());
       yIdx <= std::max(cornerGCellIdx1.y(), childGCellIdx.y());
       yIdx++) {
    auto rawSupply
        = cmap2D_->getRawSupply(cornerGCellIdx1.x(), yIdx, 0, frDirEnum::N);
    auto rawDemand
        = cmap2D_->getRawDemand(cornerGCellIdx1.x(), yIdx, 0, frDirEnum::N);
    corner1Cost += getCongCost(rawSupply, rawDemand);
    if (cmap2D_->getRawDemand(cornerGCellIdx1.x(), yIdx, 0, frDirEnum::N)
        >= cmap2D_->getRawSupply(cornerGCellIdx1.x(), yIdx, 0, frDirEnum::N)) {
      corner1Cost += router_cfg_->BLOCKCOST;
    }
    if (cmap2D_->hasBlock(cornerGCellIdx1.x(), yIdx, 0, frDirEnum::N)) {
      corner1Cost += router_cfg_->BLOCKCOST * 100;
    }
  }

  // calculate corner2 cost
  double corner2Cost = 0;
  for (int xIdx = std::min(cornerGCellIdx2.x(), childGCellIdx.x());
       xIdx <= std::max(cornerGCellIdx2.x(), childGCellIdx.x());
       xIdx++) {
    auto rawSupply
        = cmap2D_->getRawSupply(xIdx, cornerGCellIdx2.y(), 0, frDirEnum::E);
    auto rawDemand
        = cmap2D_->getRawDemand(xIdx, cornerGCellIdx2.y(), 0, frDirEnum::E);
    corner2Cost += getCongCost(rawSupply, rawDemand);
    if (cmap2D_->getRawDemand(xIdx, cornerGCellIdx2.y(), 0, frDirEnum::E)
        >= cmap2D_->getRawSupply(xIdx, cornerGCellIdx2.y(), 0, frDirEnum::E)) {
      corner2Cost += router_cfg_->BLOCKCOST;
    }
    if (cmap2D_->hasBlock(xIdx, cornerGCellIdx2.y(), 0, frDirEnum::E)) {
      corner2Cost += router_cfg_->BLOCKCOST * 100;
    }
  }
  for (int yIdx = std::min(cornerGCellIdx2.y(), parentGCellIdx.y());
       yIdx <= std::max(cornerGCellIdx2.y(), parentGCellIdx.y());
       yIdx++) {
    auto rawSupply
        = cmap2D_->getRawSupply(cornerGCellIdx2.x(), yIdx, 0, frDirEnum::N);
    auto rawDemand
        = cmap2D_->getRawDemand(cornerGCellIdx2.x(), yIdx, 0, frDirEnum::N);
    corner2Cost += getCongCost(rawSupply, rawDemand);
    if (cmap2D_->getRawDemand(cornerGCellIdx2.x(), yIdx, 0, frDirEnum::N)
        >= cmap2D_->getRawSupply(cornerGCellIdx2.x(), yIdx, 0, frDirEnum::N)) {
      corner2Cost += router_cfg_->BLOCKCOST;
    }
    if (cmap2D_->hasBlock(cornerGCellIdx2.x(), yIdx, 0, frDirEnum::N)) {
      corner2Cost += router_cfg_->BLOCKCOST * 100;
    }
  }

  if (corner1Cost < corner2Cost) {
    // create corner1 node
    auto uNode = std::make_unique<frNode>();
    uNode->setType(frNodeTypeEnum::frcSteiner);
    odb::Point cornerLoc(childLoc.x(), parentLoc.y());
    uNode->setLoc(cornerLoc);
    uNode->setLayerNum(2);
    auto cornerNode = uNode.get();
    net->addNode(uNode);
    // maintain connectivity
    parent->removeChild(child);
    parent->addChild(cornerNode);
    cornerNode->setParent(parent);
    cornerNode->addChild(child);
    child->setParent(cornerNode);
    // update congestion
    for (int xIdx = std::min(cornerGCellIdx1.x(), parentGCellIdx.x());
         xIdx < std::max(cornerGCellIdx1.x(), parentGCellIdx.x());
         xIdx++) {
      cmap2D_->addRawDemand(xIdx, cornerGCellIdx1.y(), 0, frDirEnum::E);
      cmap2D_->addRawDemand(xIdx + 1, cornerGCellIdx1.y(), 0, frDirEnum::E);
    }
    for (int yIdx = std::min(cornerGCellIdx1.y(), childGCellIdx.y());
         yIdx < std::max(cornerGCellIdx1.y(), childGCellIdx.y());
         yIdx++) {
      cmap2D_->addRawDemand(cornerGCellIdx1.x(), yIdx, 0, frDirEnum::N);
      cmap2D_->addRawDemand(cornerGCellIdx1.x(), yIdx + 1, 0, frDirEnum::N);
    }
  } else {
    // create corner2 route
    auto uNode = std::make_unique<frNode>();
    uNode->setType(frNodeTypeEnum::frcSteiner);
    odb::Point cornerLoc(parentLoc.x(), childLoc.y());
    uNode->setLoc(cornerLoc);
    uNode->setLayerNum(2);
    auto cornerNode = uNode.get();
    net->addNode(uNode);
    // maintain connectivity
    parent->removeChild(child);
    parent->addChild(cornerNode);
    cornerNode->setParent(parent);
    cornerNode->addChild(child);
    child->setParent(cornerNode);
    // update congestion
    for (int xIdx = std::min(cornerGCellIdx2.x(), childGCellIdx.x());
         xIdx < std::max(cornerGCellIdx2.x(), childGCellIdx.x());
         xIdx++) {
      cmap2D_->addRawDemand(xIdx, cornerGCellIdx2.y(), 0, frDirEnum::E);
      cmap2D_->addRawDemand(xIdx + 1, cornerGCellIdx2.y(), 0, frDirEnum::E);
    }
    for (int yIdx = std::min(cornerGCellIdx2.y(), parentGCellIdx.y());
         yIdx < std::max(cornerGCellIdx2.y(), parentGCellIdx.y());
         yIdx++) {
      cmap2D_->addRawDemand(cornerGCellIdx2.x(), yIdx, 0, frDirEnum::N);
      cmap2D_->addRawDemand(cornerGCellIdx2.x(), yIdx + 1, 0, frDirEnum::N);
    }
  }
}

double FlexGR::getCongCost(unsigned supply, unsigned demand)
{
  return demand * (1.0 + 8.0 / (1.0 + exp(supply - demand))) / (supply + 1);
}

// child node and parent node must be colinear
void FlexGR::ripupRoute(frNode* child, frNode* parent)
{
  odb::Point childLoc = child->getLoc();
  odb::Point parentLoc = parent->getLoc();
  odb::Point bp, ep;
  if (childLoc < parentLoc) {
    bp = childLoc;
    ep = parentLoc;
  } else {
    bp = parentLoc;
    ep = childLoc;
  }

  odb::Point bpIdx = design_->getTopBlock()->getGCellIdx(bp);
  odb::Point epIdx = design_->getTopBlock()->getGCellIdx(ep);

  if (bpIdx.y() == epIdx.y()) {
    // horz
    int yIdx = bpIdx.y();
    for (int xIdx = bpIdx.x(); xIdx < epIdx.x(); xIdx++) {
      cmap2D_->subRawDemand(xIdx, yIdx, 0, frDirEnum::E);
      cmap2D_->subRawDemand(xIdx + 1, yIdx, 0, frDirEnum::E);
    }
  } else {
    // vert
    int xIdx = bpIdx.x();
    for (int yIdx = bpIdx.y(); yIdx < epIdx.y(); yIdx++) {
      cmap2D_->subRawDemand(xIdx, yIdx, 0, frDirEnum::N);
      cmap2D_->subRawDemand(xIdx, yIdx + 1, 0, frDirEnum::N);
    }
  }
}

// child node and parent node must be colinear
bool FlexGR::hasOverflow2D(frNode* child, frNode* parent)
{
  bool isOverflow = false;
  odb::Point childLoc = child->getLoc();
  odb::Point parentLoc = parent->getLoc();
  odb::Point bp, ep;
  if (childLoc < parentLoc) {
    bp = childLoc;
    ep = parentLoc;
  } else {
    bp = parentLoc;
    ep = childLoc;
  }

  odb::Point bpIdx = design_->getTopBlock()->getGCellIdx(bp);
  odb::Point epIdx = design_->getTopBlock()->getGCellIdx(ep);

  if (bpIdx.y() == epIdx.y()) {
    int yIdx = bpIdx.y();
    for (int xIdx = bpIdx.x(); xIdx <= epIdx.x(); xIdx++) {
      if (cmap2D_->getRawDemand(xIdx, yIdx, 0, frDirEnum::E)
          > cmap2D_->getRawSupply(xIdx, yIdx, 0, frDirEnum::E)) {
        isOverflow = true;
        break;
      }
    }
  } else {
    int xIdx = bpIdx.x();
    for (int yIdx = bpIdx.y(); yIdx <= epIdx.y(); yIdx++) {
      if (cmap2D_->getRawDemand(xIdx, yIdx, 0, frDirEnum::N)
          > cmap2D_->getRawSupply(xIdx, yIdx, 0, frDirEnum::N)) {
        isOverflow = true;
        break;
      }
    }
  }

  return isOverflow;
}

void FlexGR::initGR_initObj()
{
  for (auto& net : design_->getTopBlock()->getNets()) {
    initGR_initObj_net(net.get());

    int steinerNodeCnt = net->getNodes().size() - net->getRPins().size();
    if (steinerNodeCnt != 0
        && (int) net->getGRShapes().size() != (steinerNodeCnt - 1)) {
      std::cout << "Error: " << net->getName() << " has " << steinerNodeCnt
                << " steiner nodes, but " << net->getGRShapes().size()
                << " pathSegs\n";
    }
  }
}

void FlexGR::initGR_initObj_net(frNet* net)
{
  std::deque<frNode*> nodeQ;

  frNode* root = net->getRoot();
  if (root == nullptr) {
    return;  // dangling net with no connections
  }
  nodeQ.push_back(root);

  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop_front();
    // push children
    for (auto child : node->getChildren()) {
      nodeQ.push_back(child);
    }

    if (node->getParent() == nullptr) {
      continue;
    }
    if (node->getType() != frNodeTypeEnum::frcSteiner
        || node->getParent()->getType() != frNodeTypeEnum::frcSteiner) {
      continue;
    }

    if (node->getLayerNum() != 2) {
      std::cout << "Error: node not on layerNum == 2 (" << node->getLayerNum()
                << ") before layerAssignment\n";
    }
    if (node->getParent()->getLayerNum() != 2) {
      std::cout << "Error: node not on layerNum == 2 ("
                << node->getParent()->getLayerNum()
                << ") before layerAssignment\n";
    }

    auto parent = node->getParent();

    odb::Point nodeLoc = node->getLoc();
    odb::Point parentLoc = parent->getLoc();

    odb::Point bp, ep;
    if (nodeLoc < parentLoc) {
      bp = nodeLoc;
      ep = parentLoc;
    } else {
      bp = parentLoc;
      ep = nodeLoc;
    }

    if (nodeLoc.y() == parentLoc.y()) {
      // horz
      auto uPathSeg = std::make_unique<grPathSeg>();
      uPathSeg->setChild(node);
      node->setConnFig(uPathSeg.get());
      uPathSeg->setParent(parent);
      uPathSeg->addToNet(net);
      uPathSeg->setPoints(bp, ep);
      uPathSeg->setLayerNum(2);

      std::unique_ptr<grShape> uShape(std::move(uPathSeg));
      net->addGRShape(uShape);
    } else if (nodeLoc.x() == parentLoc.x()) {
      // vert
      auto uPathSeg = std::make_unique<grPathSeg>();
      uPathSeg->setChild(node);
      node->setConnFig(uPathSeg.get());
      uPathSeg->setParent(parent);
      uPathSeg->addToNet(net);
      uPathSeg->setPoints(bp, ep);
      uPathSeg->setLayerNum(2);

      std::unique_ptr<grShape> uShape(std::move(uPathSeg));
      net->addGRShape(uShape);
    } else {
      std::cout << "Error: non-colinear nodes in post patternRoute\n";
    }
  }
}

void FlexGR::initGR_genTopology()
{
  std::cout << "generating net topology...\n";
  // Flute::readLUT();
  for (auto& net : design_->getTopBlock()->getNets()) {
    // generate MST (currently using Prim-Dijkstra) and steiner tree (currently
    // using HVW)
    initGR_genTopology_net(net.get());
    initGR_updateCongestion2D_net(net.get());
  }
  std::cout << "done net topology...\n";
}

// generate 2D topology, rpin node always connect to center of gcell
// to be followed by layer assignment
void FlexGR::initGR_genTopology_net(frNet* net)
{
  if (net->getNodes().empty()) {
    return;
  }

  if (net->getNodes().size() == 1) {
    net->setRoot(net->getNodes().front().get());
    return;
  }

  std::vector<frNode*> nodes(net->getNodes().size(), nullptr);  // 0 is source
  std::map<frBlockObject*, std::vector<frNode*>>
      pin2Nodes;  // vector order needs to align with map below
  std::map<frBlockObject*, std::vector<frRPin*>> pin2RPins;
  unsigned sinkIdx = 1;

  auto& netNodes = net->getNodes();
  // init nodes and populate pin2Nodes
  for (auto& node : netNodes) {
    if (node->getPin()) {
      if (node->getPin()->typeId() == frcInstTerm) {
        auto ioType = static_cast<frInstTerm*>(node->getPin())
                          ->getTerm()
                          ->getDirection();
        // for instTerm, direction OUTPUT is driver
        if (ioType == odb::dbIoType::OUTPUT && nodes[0] == nullptr) {
          nodes[0] = node.get();
        } else {
          if (sinkIdx >= nodes.size()) {
            sinkIdx %= nodes.size();
          }
          nodes[sinkIdx] = node.get();
          sinkIdx++;
        }
        pin2Nodes[node->getPin()].push_back(node.get());
      } else if (node->getPin()->typeId() == frcBTerm
                 || node->getPin()->typeId() == frcMTerm) {
        auto ioType = static_cast<frTerm*>(node->getPin())->getDirection();
        // for IO term, direction INPUT is driver
        if (ioType == odb::dbIoType::INPUT && nodes[0] == nullptr) {
          nodes[0] = node.get();
        } else {
          if (sinkIdx >= nodes.size()) {
            sinkIdx %= nodes.size();
          }
          nodes[sinkIdx] = node.get();
          sinkIdx++;
        }
        pin2Nodes[node->getPin()].push_back(node.get());
      } else {
        std::cout << "Error: unknown pin type in initGR_genTopology_net\n";
      }
    }
  }

  net->setRoot(nodes[0]);
  // populate pin2RPins
  for (auto& rpin : net->getRPins()) {
    if (rpin->getFrTerm()) {
      pin2RPins[rpin->getFrTerm()].push_back(rpin.get());
    }
  }
  // update nodes location based on rpin
  for (auto& [pin, nodes] : pin2Nodes) {
    if (pin2RPins.find(pin) == pin2RPins.end()) {
      std::cout << "Error: pin not found in pin2RPins\n";
      exit(1);
    }
    if (pin2RPins[pin].size() != nodes.size()) {
      std::cout << "Error: mismatch in nodes and ripins size\n";
      exit(1);
    }
    auto& rpins = pin2RPins[pin];
    for (int i = 0; i < (int) nodes.size(); i++) {
      auto rpin = rpins[i];
      auto node = nodes[i];
      odb::Point pt;
      if (rpin->getFrTerm()->typeId() == frcInstTerm) {
        auto inst = static_cast<frInstTerm*>(rpin->getFrTerm())->getInst();
        odb::dbTransform shiftXform = inst->getNoRotationTransform();
        pt = rpin->getAccessPoint()->getPoint();
        shiftXform.apply(pt);
      } else {
        pt = rpin->getAccessPoint()->getPoint();
      }
      node->setLoc(pt);
      node->setLayerNum(rpin->getAccessPoint()->getLayerNum());
    }
  }

  // std::map<std::pair<int, int>, std::vector<frNode*> > gcellIdx2Nodes;
  auto& gcellIdx2Nodes = net2GCellIdx2Nodes_[net];
  // std::map<frNode*, std::vector<frNode*> > gcellNode2RPinNodes;
  auto& gcellNode2RPinNodes = net2GCellNode2RPinNodes_[net];

  // prep for 2D topology generation in case two nodes are more than one rpin in
  // same gcell topology genration works on gcell (center-to-center) level
  for (auto node : nodes) {
    odb::Point apLoc = node->getLoc();
    odb::Point apGCellIdx = design_->getTopBlock()->getGCellIdx(apLoc);
    gcellIdx2Nodes[std::make_pair(apGCellIdx.x(), apGCellIdx.y())].push_back(
        node);
  }

  // generate gcell-level node
  // std::vector<frNode*> gcellNodes(gcellIdx2Nodes.size(), nullptr);
  auto& gcellNodes = net2GCellNodes_[net];
  gcellNodes.resize(gcellIdx2Nodes.size(), nullptr);

  std::vector<std::unique_ptr<frNode>> tmpGCellNodes;
  sinkIdx = 1;
  unsigned rootIdx = 0;
  unsigned rootIdxCnt = 0;
  for (auto& [gcellIdx, localNodes] : gcellIdx2Nodes) {
    bool hasRoot = false;
    for (auto localNode : localNodes) {
      if (localNode == nodes[0]) {
        hasRoot = true;
      }
    }

    auto gcellNode = std::make_unique<frNode>();
    gcellNode->setType(frNodeTypeEnum::frcSteiner);
    odb::Rect gcellBox = design_->getTopBlock()->getGCellBox(
        odb::Point(gcellIdx.first, gcellIdx.second));
    odb::Point loc((gcellBox.xMin() + gcellBox.xMax()) / 2,
                   (gcellBox.yMin() + gcellBox.yMax()) / 2);
    gcellNode->setLayerNum(2);
    gcellNode->setLoc(loc);
    if (!hasRoot) {
      gcellNode->setId(net->getNodes().back()->getId() + sinkIdx + 1);
      gcellNodes[sinkIdx] = gcellNode.get();
      sinkIdx++;
    } else {
      gcellNode->setId(net->getNodes().back()->getId() + 1);
      gcellNodes[0] = gcellNode.get();
      rootIdx = rootIdxCnt;
    }
    gcellNode2RPinNodes[gcellNode.get()] = localNodes;
    tmpGCellNodes.push_back(std::move(gcellNode));
    rootIdxCnt++;
  }
  net->setFirstNonRPinNode(gcellNodes[0]);

  net->addNode(tmpGCellNodes[rootIdx]);
  for (unsigned i = 0; i < tmpGCellNodes.size(); i++) {
    if (i != rootIdx) {
      net->addNode(tmpGCellNodes[i]);
    }
  }

  for (unsigned i = 0; i < gcellNodes.size(); i++) {
    auto node = gcellNodes[i];
    if (!node) {
      std::cout << "Error: gcell node " << i << " is 0x0\n";
    }
  }

  if (gcellNodes.size() <= 1) {
    return;
  }

  net->setRootGCellNode(gcellNodes[0]);

  auto& steinerNodes = net2SteinerNodes_[net];
  // if (gcellNodes.size() >= 150) {
  // TODO: remove connFig instantiation to match FLUTE behavior
  if (false) {
    // generate mst topology
    genMSTTopology(gcellNodes);

    // sanity check
    for (unsigned i = 1; i < gcellNodes.size(); i++) {
      if (gcellNodes[i]->getParent() == nullptr) {
        std::cout << "Error: non-root gcell node does not have parent\n";
      }
    }

    // generate steiner tree from MST
    genSTTopology_HVW(gcellNodes, steinerNodes);
    // generate shapes and update congestion map
    for (auto node : gcellNodes) {
      // add shape from child to parent
      if (node->getParent()) {
        auto parent = node->getParent();
        odb::Point childLoc = node->getLoc();
        odb::Point parentLoc = parent->getLoc();
        odb::Point bp, ep;
        if (childLoc < parentLoc) {
          bp = childLoc;
          ep = parentLoc;
        } else {
          bp = parentLoc;
          ep = childLoc;
        }

        auto uPathSeg = std::make_unique<grPathSeg>();
        uPathSeg->setChild(node);
        uPathSeg->setParent(parent);
        uPathSeg->addToNet(net);
        uPathSeg->setPoints(bp, ep);
        // 2D shapes are all on layerNum == 2
        // assuming (layerNum / - 1) == congestion map idx
        uPathSeg->setLayerNum(2);

        odb::Point bpIdx = design_->getTopBlock()->getGCellIdx(bp);
        odb::Point epIdx = design_->getTopBlock()->getGCellIdx(ep);

        // update congestion map
        // horizontal
        unsigned zIdx = 0;
        if (bpIdx.y() == epIdx.y()) {
          for (int xIdx = bpIdx.x(); xIdx < epIdx.x(); xIdx++) {
            cmap_->addDemand(xIdx, bpIdx.y(), zIdx, frDirEnum::E);
          }
        } else {
          for (int yIdx = bpIdx.y(); yIdx < epIdx.y(); yIdx++) {
            cmap_->addDemand(bpIdx.x(), yIdx, zIdx, frDirEnum::N);
          }
        }

        std::unique_ptr<grShape> uShape(std::move(uPathSeg));
        net->addGRShape(uShape);
      }
    }

    for (auto node : steinerNodes) {
      // add shape from child to parent
      if (node->getParent()) {
        auto parent = node->getParent();
        odb::Point childLoc = node->getLoc();
        odb::Point parentLoc = parent->getLoc();
        odb::Point bp, ep;
        if (childLoc < parentLoc) {
          bp = childLoc;
          ep = parentLoc;
        } else {
          bp = parentLoc;
          ep = childLoc;
        }

        auto uPathSeg = std::make_unique<grPathSeg>();
        uPathSeg->setChild(node);
        uPathSeg->setParent(parent);
        uPathSeg->addToNet(net);
        uPathSeg->setPoints(bp, ep);
        // 2D shapes are all on layerNum == 2
        // assuming (layerNum / - 1) == congestion map idx
        uPathSeg->setLayerNum(2);

        odb::Point bpIdx = design_->getTopBlock()->getGCellIdx(bp);
        odb::Point epIdx = design_->getTopBlock()->getGCellIdx(ep);

        // update congestion map
        // horizontal
        unsigned zIdx = 0;
        if (bpIdx.y() == epIdx.y()) {
          for (int xIdx = bpIdx.x(); xIdx < epIdx.x(); xIdx++) {
            cmap_->addDemand(xIdx, bpIdx.y(), zIdx, frDirEnum::E);
          }
        } else {
          for (int yIdx = bpIdx.y(); yIdx < epIdx.y(); yIdx++) {
            cmap_->addDemand(bpIdx.x(), yIdx, zIdx, frDirEnum::N);
          }
        }

        std::unique_ptr<grShape> uShape(std::move(uPathSeg));
        net->addGRShape(uShape);
      }
    }
  } else {
    genSTTopology_FLUTE(gcellNodes, steinerNodes);
  }

  // connect rpin node to gcell center node
  for (auto& [gcellNode, localNodes] : gcellNode2RPinNodes) {
    for (auto localNode : localNodes) {
      if (localNode == nodes[0]) {
        gcellNode->setParent(localNode);
        localNode->addChild(gcellNode);
      } else {
        gcellNode->addChild(localNode);
        localNode->setParent(gcellNode);
      }
    }
  }

  // sanity check
  for (size_t i = 1; i < nodes.size(); i++) {
    if (nodes[i]->getParent() == nullptr) {
      std::cout << "Error: non-root node does not have parent in "
                << net->getName() << '\n';
    }
  }
  if (nodes.size() > 1 && nodes[0]->getChildren().empty()) {
    std::cout << "Error: root does not have any children\n";
  }
}

void FlexGR::layerAssign()
{
  std::cout << "layer assignment...\n";
  std::vector<std::pair<int, frNet*>> sortedNets;
  for (auto& uNet : design_->getTopBlock()->getNets()) {
    auto net = uNet.get();
    if (net2GCellNodes_.find(net) == net2GCellNodes_.end()
        || net2GCellNodes_[net].size() <= 1) {
      continue;
    }
    frCoord llx = INT_MAX;
    frCoord lly = INT_MAX;
    frCoord urx = INT_MIN;
    frCoord ury = INT_MIN;
    for (auto& rpin : net->getRPins()) {
      odb::Rect bbox = rpin->getBBox();
      llx = std::min(bbox.xMin(), llx);
      lly = std::min(bbox.yMin(), lly);
      urx = std::max(bbox.xMax(), urx);
      ury = std::max(bbox.yMax(), ury);
    }
    int numRPins = net->getRPins().size();
    int ratio = ((urx - llx) + (ury - lly)) / (numRPins);
    sortedNets.emplace_back(ratio, net);
  }

  // sort
  struct sort_net
  {
    bool operator()(const std::pair<int, frNet*>& left,
                    const std::pair<int, frNet*>& right)
    {
      if (left.first == right.first) {
        return (left.second->getId() < right.second->getId());
      }
      return (left.first < right.first);
    }
  };
  sort(sortedNets.begin(), sortedNets.end(), sort_net());

  for (auto& [ratio, net] : sortedNets) {
    layerAssign_net(net);
  }

  std::cout << "done layer assignment...\n";
}

void FlexGR::layerAssign_net(frNet* net)
{
  net->clearGRShapes();

  if (net2GCellNodes_.find(net) == net2GCellNodes_.end()
      || net2GCellNodes_[net].size() <= 1) {
    return;
  }

  // update net2GCellNode2RPinNodes
  auto& gcellNode2RPinNodes = net2GCellNode2RPinNodes_[net];
  gcellNode2RPinNodes.clear();
  unsigned rpinNodeSize = net->getRPins().size();
  unsigned nodeCnt = 0;

  for (auto& node : net->getNodes()) {
    if (nodeCnt == rpinNodeSize) {
      net->setFirstNonRPinNode(node.get());
      break;
    }

    if (node.get() == net->getRoot()) {
      gcellNode2RPinNodes[node->getChildren().front()].push_back(node.get());
    } else {
      gcellNode2RPinNodes[node->getParent()].push_back(node.get());
    }
    nodeCnt++;
  }

  // break connections between rpin and gcell nodes
  auto& nodes = net->getNodes();
  unsigned rpinNodeCnt = 0;

  // std::cout << net->getName() << std::endl << std::flush;

  for (auto& node : nodes) {
    if (node.get() == net->getRoot()) {
      node->getChildren().front()->setParent(nullptr);
      net->setRootGCellNode(node->getChildren().front());
      node->clearChildren();
    } else {
      node->getParent()->removeChild(node.get());
      node->setParent(nullptr);
    }
    rpinNodeCnt++;
    if (rpinNodeCnt >= rpinNodeSize) {
      break;
    }
  }

  for (auto& node : net->getNodes()) {
    node->setConnFig(nullptr);
  }

  int numNodes = net->getNodes().size() - net->getRPins().size();
  std::vector<std::vector<unsigned>> bestLayerCosts(
      numNodes, std::vector<unsigned>(cmap_->getNumLayers(), UINT_MAX));
  std::vector<std::vector<unsigned>> bestLayerCombs(
      numNodes, std::vector<unsigned>(cmap_->getNumLayers(), 0));

  // recursively compute the best layer for each node from root (post-order
  // traversal)
  layerAssign_node_compute(
      net->getRootGCellNode(), net, bestLayerCosts, bestLayerCombs);

  // recursively update nodes to 3D
  frLayerNum minCostLayerNum = 0;
  unsigned minCost = UINT_MAX;
  int rootIdx = distance(net->getFirstNonRPinNode()->getIter(),
                         net->getRootGCellNode()->getIter());

  for (frLayerNum layerNum = 0; layerNum < cmap_->getNumLayers(); layerNum++) {
    if (bestLayerCosts[rootIdx][layerNum] < minCost) {
      minCostLayerNum = layerNum;
      minCost = bestLayerCosts[rootIdx][layerNum];
    }
  }
  layerAssign_node_commit(
      net->getRootGCellNode(), net, minCostLayerNum, bestLayerCombs);

  // create shapes and update congestion
  for (auto& uNode : net->getNodes()) {
    auto node = uNode.get();
    if (node->getParent() == nullptr) {
      continue;
    }
    if (node->getType() == frNodeTypeEnum::frcPin
        || node->getParent()->getType() == frNodeTypeEnum::frcPin) {
      continue;
    }
    // steiner-to-steiner
    auto parent = node->getParent();
    if (node->getLayerNum() == node->getParent()->getLayerNum()) {
      // pathSeg
      odb::Point currLoc = node->getLoc();
      odb::Point parentLoc = parent->getLoc();

      odb::Point bp, ep;
      if (currLoc < parentLoc) {
        bp = currLoc;
        ep = parentLoc;
      } else {
        bp = parentLoc;
        ep = currLoc;
      }

      auto uPathSeg = std::make_unique<grPathSeg>();
      uPathSeg->setChild(node);
      uPathSeg->setParent(parent);
      uPathSeg->addToNet(net);
      uPathSeg->setPoints(bp, ep);
      uPathSeg->setLayerNum(node->getLayerNum());

      odb::Point bpIdx = design_->getTopBlock()->getGCellIdx(bp);
      odb::Point epIdx = design_->getTopBlock()->getGCellIdx(ep);

      // update congestion map
      // horizontal
      unsigned zIdx = node->getLayerNum() / 2 - 1;
      if (bpIdx.y() == epIdx.y()) {
        for (int xIdx = bpIdx.x(); xIdx < epIdx.x(); xIdx++) {
          cmap_->addRawDemand(xIdx, bpIdx.y(), zIdx, frDirEnum::E);
          cmap_->addRawDemand(xIdx + 1, bpIdx.y(), zIdx, frDirEnum::E);
        }
      } else {
        for (int yIdx = bpIdx.y(); yIdx < epIdx.y(); yIdx++) {
          cmap_->addRawDemand(bpIdx.x(), yIdx, zIdx, frDirEnum::N);
          cmap_->addRawDemand(bpIdx.x(), yIdx + 1, zIdx, frDirEnum::N);
        }
      }

      // assign to child
      node->setConnFig(uPathSeg.get());

      std::unique_ptr<grShape> uShape(std::move(uPathSeg));
      net->addGRShape(uShape);
    } else {
      // via
      odb::Point loc = node->getLoc();
      frLayerNum beginLayerNum = node->getLayerNum();
      frLayerNum endLayerNum = parent->getLayerNum();

      auto uVia = std::make_unique<grVia>();
      uVia->setChild(node);
      uVia->setParent(parent);
      uVia->addToNet(net);
      uVia->setOrigin(loc);
      uVia->setViaDef(design_->getTech()
                          ->getLayer((beginLayerNum + endLayerNum) / 2)
                          ->getDefaultViaDef());

      // assign to child
      node->setConnFig(uVia.get());

      // TODO: update congestion map

      net->addGRVia(uVia);
    }
  }
}

// get the costs of having currNode to parent edge on all layers
void FlexGR::layerAssign_node_compute(
    frNode* currNode,
    frNet* net,
    std::vector<std::vector<unsigned>>& bestLayerCosts,
    std::vector<std::vector<unsigned>>& bestLayerCombs)
{
  if (currNode == nullptr) {
    return;
  }

  if (currNode->getChildren().empty()) {
    layerAssign_node_compute(nullptr, net, bestLayerCosts, bestLayerCombs);
  } else {
    for (auto child : currNode->getChildren()) {
      layerAssign_node_compute(child, net, bestLayerCosts, bestLayerCombs);
    }
  }

  unsigned numChild = currNode->getChildren().size();
  unsigned numComb = 1;
  unsigned currLayerCost = UINT_MAX;
  // since max degree is four (at most three children), so this should not
  // overflow
  for (unsigned i = 0; i < numChild; i++) {
    numComb *= cmap_->getNumLayers();
  }
  int currNodeIdx
      = distance(net->getFirstNonRPinNode()->getIter(), currNode->getIter());
  // iterate over all combinations and get the combination with lowest overall
  // cost
  for (int layerNum = 0; layerNum < cmap_->getNumLayers(); layerNum++) {
    unsigned currLayerBestCost = UINT_MAX;
    unsigned currLayerBestComb = 0;
    for (unsigned comb = 0; comb < numComb; comb++) {
      // get upstream via cost
      unsigned upstreamViaCost = 0;
      int minPinLayerNum = INT_MAX;
      int maxPinLayerNum = INT_MIN;
      if (net2GCellNode2RPinNodes_[net].find(currNode)
          != net2GCellNode2RPinNodes_[net].end()) {
        auto& rpinNodes = net2GCellNode2RPinNodes_[net][currNode];
        for (auto rpinNode : rpinNodes) {
          // convert to cmap layer
          auto pinLayerNum = rpinNode->getLayerNum() / 2 - 1;
          if (minPinLayerNum > pinLayerNum) {
            minPinLayerNum = pinLayerNum;
          }
          if (maxPinLayerNum < pinLayerNum) {
            maxPinLayerNum = pinLayerNum;
          }
        }
      }
      // get downstream via cost
      unsigned downstreamViaCost = 0;
      unsigned downstreamCost = 0;
      unsigned currComb = comb;
      int downstreamMinLayerNum = INT_MAX;
      int downstreamMaxLayerNum = INT_MIN;

      for (auto child : currNode->getChildren()) {
        int childNodeIdx
            = distance(net->getFirstNonRPinNode()->getIter(), child->getIter());
        int childLayerNum = currComb % cmap_->getNumLayers();
        downstreamMinLayerNum = std::min(downstreamMinLayerNum, childLayerNum);
        downstreamMaxLayerNum = std::max(downstreamMaxLayerNum, childLayerNum);
        currComb /= cmap_->getNumLayers();

        // add downstream cost
        downstreamCost += bestLayerCosts[childNodeIdx][childLayerNum];
      }

      // TODO: tune the via cost here
      downstreamViaCost
          = (std::max(layerNum, std::max(maxPinLayerNum, downstreamMaxLayerNum))
             - std::min(layerNum,
                        std::min(minPinLayerNum, downstreamMinLayerNum)))
            * router_cfg_->VIACOST;

      // get upstream edge congestion cost
      unsigned congestionCost = 0;
      // bool isLayerBlocked = layerNum <= (VIA_ACCESS_LAYERNUM / 2 - 1);
      bool isLayerBlocked = false;

      odb::Point currLoc = currNode->getLoc();
      odb::Point parentLoc;
      if (currNode->getParent()) {
        auto parent = currNode->getParent();
        parentLoc = parent->getLoc();
      } else {
        parentLoc = currLoc;
      }

      if (layerNum <= (router_cfg_->VIA_ACCESS_LAYERNUM / 2 - 1)) {
        congestionCost += router_cfg_->VIACOST * 8;
      }

      odb::Point beginIdx, endIdx;
      if (parentLoc.x() != currLoc.x() || parentLoc.y() != currLoc.y()) {
        if (parentLoc < currLoc) {
          beginIdx = design_->getTopBlock()->getGCellIdx(parentLoc);
          endIdx = design_->getTopBlock()->getGCellIdx(currLoc);
        } else {
          beginIdx = design_->getTopBlock()->getGCellIdx(currLoc);
          endIdx = design_->getTopBlock()->getGCellIdx(parentLoc);
        }
        // horz
        if (beginIdx.y() == endIdx.y()) {
          if (design_->getTech()->getLayer((layerNum + 1) * 2)->getDir()
              == dbTechLayerDir::VERTICAL) {
            isLayerBlocked = true;
          }
          int yIdx = beginIdx.y();
          for (int xIdx = beginIdx.x(); xIdx < endIdx.x(); xIdx++) {
            auto supply
                = cmap_->getRawSupply(xIdx, yIdx, layerNum, frDirEnum::E);
            auto demand
                = cmap_->getRawDemand(xIdx, yIdx, layerNum, frDirEnum::E);
            // block cost
            if (isLayerBlocked
                || cmap_->hasBlock(xIdx, yIdx, layerNum, frDirEnum::E)) {
              congestionCost += router_cfg_->BLOCKCOST * 100;
            }
            // congestion cost
            if (demand > supply / 4) {
              congestionCost += (demand * 10 / (supply + 1));
            }

            // overflow
            if (demand >= supply) {
              congestionCost += router_cfg_->MARKERCOST * 8;
            }
          }
        } else {
          if (design_->getTech()->getLayer((layerNum + 1) * 2)->getDir()
              == dbTechLayerDir::HORIZONTAL) {
            isLayerBlocked = true;
          }
          int xIdx = beginIdx.x();
          for (int yIdx = beginIdx.y(); yIdx < endIdx.y(); yIdx++) {
            auto supply
                = cmap_->getRawSupply(xIdx, yIdx, layerNum, frDirEnum::N);
            auto demand
                = cmap_->getRawDemand(xIdx, yIdx, layerNum, frDirEnum::N);
            if (isLayerBlocked
                || cmap_->hasBlock(xIdx, yIdx, layerNum, frDirEnum::N)) {
              congestionCost += router_cfg_->BLOCKCOST * 100;
            }
            // congestion cost
            if (demand > supply / 4) {
              congestionCost += (demand * 10 / (supply + 1));
            }
            // overflow
            if (demand >= supply) {
              congestionCost += router_cfg_->MARKERCOST * 8;
            }
          }
        }
      }

      currLayerCost = upstreamViaCost + downstreamCost + downstreamViaCost
                      + congestionCost;

      if (currLayerCost < currLayerBestCost) {
        currLayerBestCost = currLayerCost;
        currLayerBestComb = comb;
      }
    }
    bestLayerCosts[currNodeIdx][layerNum] = currLayerBestCost;
    bestLayerCombs[currNodeIdx][layerNum] = currLayerBestComb;
  }
}

void FlexGR::layerAssign_node_commit(
    frNode* currNode,
    frNet* net,
    frLayerNum layerNum,  // which layer the connection from currNode to
                          // parentNode should be on
    std::vector<std::vector<unsigned>>& bestLayerCombs)
{
  if (currNode == nullptr) {
    return;
  }
  int currNodeIdx
      = distance(net->getFirstNonRPinNode()->getIter(), currNode->getIter());
  std::vector<frNode*> children(currNode->getChildren().size(), nullptr);
  unsigned childIdx = 0;
  for (auto child : currNode->getChildren()) {
    int childNodeIdx
        = distance(net->getFirstNonRPinNode()->getIter(), child->getIter());
    if (childNodeIdx >= (int) bestLayerCombs.size()) {
      std::cout << net->getName() << std::endl;
      std::cout
          << "Error: non-pin gcell or non-steiner child node, childNodeIdx = "
          << childNodeIdx << ", currNodeIdx = " << currNodeIdx
          << ", bestLayerCombs.size() = " << bestLayerCombs.size() << "\n";
      odb::Point loc1 = currNode->getLoc();
      odb::Point loc2 = child->getLoc();
      std::cout << "currNodeLoc = (" << loc1.x() / 2000.0 << ", "
                << loc1.y() / 2000.0 << "), childLoc = (" << loc2.x() / 2000.0
                << ", " << loc2.y() / 2000.0 << ")\n";
      std::cout << "currNodeType = " << (int) (currNode->getType())
                << ", childNodeType = " << (int) (child->getType())
                << std::endl;
      exit(1);
    }

    if (currNodeIdx >= (int) bestLayerCombs.size()) {
      std::cout << "Error: non-pin gcell or non-steiner node, currNodeIdx = "
                << currNodeIdx << ", parentNodeIdx = "
                << distance(net->getFirstNonRPinNode()->getIter(),
                            currNode->getParent()->getIter())
                << "\n";
      odb::Point loc1 = currNode->getLoc();
      odb::Point loc2 = currNode->getParent()->getLoc();
      std::cout << "currNodeLoc = (" << loc1.x() / 2000.0 << ", "
                << loc1.y() / 2000.0 << "), parentLoc = (" << loc2.x() / 2000.0
                << ", " << loc2.y() / 2000.0 << ")\n";
      std::cout << "currNodeType = " << (int) (currNode->getType())
                << std::endl;
      exit(1);
    }
    if (child->getType() == frNodeTypeEnum::frcPin) {
      odb::Point loc = child->getLoc();
      std::cout << "Error1: currNodeIdx = " << currNodeIdx
                << ", should not commit pin node, loc(" << loc.x() / 2000.0
                << ", " << loc.y() / 2000.0 << ")\n";
      exit(1);
    }
    children[childIdx] = child;
    childIdx++;
  }

  auto comb = bestLayerCombs[currNodeIdx][layerNum];

  if (children.empty()) {
    layerAssign_node_commit(nullptr, net, 0, bestLayerCombs);
  } else {
    for (auto& child : children) {
      if (child->getType() == frNodeTypeEnum::frcPin) {
        odb::Point loc = child->getLoc();
        std::cout << "Error2: should not commit pin node, loc("
                  << loc.x() / 2000.0 << ", " << loc.y() / 2000.0 << ")\n";
        exit(1);
      }
      layerAssign_node_commit(
          child, net, comb % cmap_->getNumLayers(), bestLayerCombs);
      comb /= cmap_->getNumLayers();
    }
  }

  // move currNode to its best layer (tech layerNum)
  currNode->setLayerNum((layerNum + 1) * 2);

  // tech layer num, not grid layer num
  std::set<frLayerNum> nodeLayerNums;
  std::map<frLayerNum, std::vector<frNode*>> layerNum2Children;
  // sub nodes are created at same loc as currNode but differnt layerNum
  // since we move from 2d to 3d
  std::map<frLayerNum, frNode*> layerNum2SubNode;

  std::map<frLayerNum, std::vector<frNode*>> layerNum2RPinNodes;

  nodeLayerNums.insert(currNode->getLayerNum());
  for (auto& child : children) {
    nodeLayerNums.insert(child->getLayerNum());
    layerNum2Children[child->getLayerNum()].push_back(child);
  }

  bool hasRootNode = false;
  // insert rpin layerNum if exists
  if (net2GCellNode2RPinNodes_[net].find(currNode)
      != net2GCellNode2RPinNodes_[net].end()) {
    auto& rpinNodes = net2GCellNode2RPinNodes_[net][currNode];
    for (auto& rpinNode : rpinNodes) {
      if (rpinNode->getType() != frNodeTypeEnum::frcPin) {
        std::cout << "Error: rpinNode is not rpin" << std::endl;
        exit(1);
      }
      nodeLayerNums.insert(rpinNode->getLayerNum());
      layerNum2RPinNodes[rpinNode->getLayerNum()].push_back(rpinNode);
      if (rpinNode == net->getRoot()) {
        hasRootNode = true;
      }
    }
  }

  odb::Point currNodeLoc = currNode->getLoc();

  for (auto layerNum = *(nodeLayerNums.begin());
       layerNum <= *(nodeLayerNums.rbegin());
       layerNum += 2) {
    // create node if the layer number is not equal to currNode layerNum
    if (layerNum != currNode->getLayerNum()) {
      auto uNode = std::make_unique<frNode>();
      uNode->setType(frNodeTypeEnum::frcSteiner);
      uNode->setLoc(currNodeLoc);
      uNode->setLayerNum(layerNum);
      layerNum2SubNode[layerNum] = uNode.get();
      net->addNode(uNode);
    } else {
      layerNum2SubNode[layerNum] = currNode;
    }
  }

  // update connectivity between children and current sub nodes
  currNode->clearChildren();

  frLayerNum parentLayer
      = hasRootNode ? net->getRoot()->getLayerNum() : currNode->getLayerNum();

  for (auto layerNum = *(nodeLayerNums.begin());
       layerNum <= *(nodeLayerNums.rbegin());
       layerNum += 2) {
    // connect children nodes and sub node (including currNode) (i.e., planar)
    if (layerNum2Children.find(layerNum) != layerNum2Children.end()) {
      for (auto child : layerNum2Children[layerNum]) {
        child->setParent(layerNum2SubNode[layerNum]);
        layerNum2SubNode[layerNum]->addChild(child);
      }
    }
    // connect vertical
    if (layerNum < parentLayer) {
      if (layerNum + 2 > *(nodeLayerNums.rbegin())) {
        std::cout << "Error: layerNum out of upper bound\n";
        exit(1);
      }
      layerNum2SubNode[layerNum]->setParent(layerNum2SubNode[layerNum + 2]);
      layerNum2SubNode[layerNum + 2]->addChild(layerNum2SubNode[layerNum]);
    } else if (layerNum > parentLayer) {
      if (layerNum - 2 < *(nodeLayerNums.begin())) {
        std::cout << "Error: layerNum out of lower bound\n";
        exit(1);
      }
      layerNum2SubNode[layerNum]->setParent(layerNum2SubNode[layerNum - 2]);
      layerNum2SubNode[layerNum - 2]->addChild(layerNum2SubNode[layerNum]);
    }
  }

  // update connectivity if there is local rpin node
  for (auto& [layerNum, rpinNodes] : layerNum2RPinNodes) {
    for (auto rpinNode : rpinNodes) {
      // rpinNode->reset();
      if (rpinNode == net->getRoot()) {
        layerNum2SubNode[layerNum]->setParent(rpinNode);
        rpinNode->addChild(layerNum2SubNode[layerNum]);
      } else {
        rpinNode->setParent(layerNum2SubNode[layerNum]);
        layerNum2SubNode[layerNum]->addChild(rpinNode);
      }
    }
  }
}

void FlexGR::writeToGuide()
{
  for (auto& uNet : design_->getTopBlock()->getNets()) {
    auto net = uNet.get();
    bool hasGRShape = false;
    // pathSeg guide
    for (auto& uShape : net->getGRShapes()) {
      hasGRShape = true;
      if (uShape->typeId() == grcPathSeg) {
        auto pathSeg = static_cast<grPathSeg*>(uShape.get());
        auto [bp, ep] = pathSeg->getPoints();
        frLayerNum layerNum;
        layerNum = pathSeg->getLayerNum();
        auto routeGuide = std::make_unique<frGuide>();
        routeGuide->setPoints(bp, ep);
        routeGuide->setBeginLayerNum(layerNum);
        routeGuide->setEndLayerNum(layerNum);
        routeGuide->addToNet(net);
        net->addGuide(std::move(routeGuide));
      } else {
        std::cout << "Error: unsupported gr type\n";
      }
    }

    // via guide
    for (auto& uVia : net->getGRVias()) {
      hasGRShape = true;
      auto via = uVia.get();
      odb::Point loc = via->getOrigin();
      frLayerNum beginLayerNum, endLayerNum;
      beginLayerNum = via->getViaDef()->getLayer1Num();
      endLayerNum = via->getViaDef()->getLayer2Num();

      auto viaGuide = std::make_unique<frGuide>();
      viaGuide->setPoints(loc, loc);
      viaGuide->setBeginLayerNum(beginLayerNum);
      viaGuide->setEndLayerNum(endLayerNum);
      viaGuide->addToNet(net);
      net->addGuide(std::move(viaGuide));
    }

    // pure local net
    if (!hasGRShape) {
      if (net2GCellNodes_.find(net) == net2GCellNodes_.end()
          || net2GCellNodes_[net].empty()) {
        continue;
      }

      if (net2GCellNodes_[net].size() > 1) {
        std::cout << "Error: net " << net->getName()
                  << " spans more than one gcell but not globally routed\n";
        exit(1);
      }

      auto gcellNode = net->getFirstNonRPinNode();
      odb::Point loc = gcellNode->getLoc();
      frLayerNum minPinLayerNum = INT_MAX;
      frLayerNum maxPinLayerNum = INT_MIN;

      auto& rpinNodes = net2GCellNode2RPinNodes_[net][gcellNode];
      for (auto rpinNode : rpinNodes) {
        frLayerNum layerNum = rpinNode->getLayerNum();
        if (layerNum < minPinLayerNum) {
          minPinLayerNum = layerNum;
        }
        if (layerNum > maxPinLayerNum) {
          maxPinLayerNum = layerNum;
        }
      }

      for (auto layerNum = minPinLayerNum;
           (layerNum + 2) <= std::max(minPinLayerNum + 4, maxPinLayerNum)
           && (layerNum + 2) <= design_->getTech()->getTopLayerNum();
           layerNum += 2) {
        auto viaGuide = std::make_unique<frGuide>();
        viaGuide->setPoints(loc, loc);
        viaGuide->setBeginLayerNum(layerNum);
        viaGuide->setEndLayerNum(layerNum + 2);
        viaGuide->addToNet(net);
        net->addGuide(std::move(viaGuide));
      }
    }
  }
}

void FlexGR::updateDb()
{
  auto block = db_->getChip()->getBlock();
  auto dbTech = db_->getTech();
  for (auto& net : design_->getTopBlock()->getNets()) {
    auto dbNet = block->findNet(net->getName().c_str());
    dbNet->clearGuides();
    for (auto& guide : net->getGuides()) {
      auto [bp, ep] = guide->getPoints();
      odb::Point bpIdx = design_->getTopBlock()->getGCellIdx(bp);
      odb::Point epIdx = design_->getTopBlock()->getGCellIdx(ep);

      odb::Rect bbox = design_->getTopBlock()->getGCellBox(bpIdx);
      odb::Rect ebox = design_->getTopBlock()->getGCellBox(epIdx);
      frLayerNum bNum = guide->getBeginLayerNum();
      frLayerNum eNum = guide->getEndLayerNum();
      // append unit guide in case of stacked via
      if (bNum != eNum) {
        for (auto lNum = std::min(bNum, eNum); lNum <= std::max(bNum, eNum);
             lNum += 2) {
          auto layer = design_->getTech()->getLayer(lNum);
          auto dbLayer = dbTech->findLayer(layer->getName().c_str());
          odb::dbGuide::create(dbNet, dbLayer, dbLayer, bbox, false);
        }
      } else {
        auto layer = design_->getTech()->getLayer(bNum);
        auto dbLayer = dbTech->findLayer(layer->getName().c_str());
        odb::dbGuide::create(
            dbNet,
            dbLayer,
            dbLayer,
            {bbox.xMin(), bbox.yMin(), ebox.xMax(), ebox.yMax()},
            false);
      }
    }
    auto dbGuides = dbNet->getGuides();
    if (dbGuides.orderReversed() && dbGuides.reversible()) {
      dbGuides.reverse();
    }
  }
}

void FlexGR::getBatchInfo(int& batchStepX, int& batchStepY)
{
  batchStepX = 2;
  batchStepY = 2;
}

// GRWorker related
void FlexGRWorker::main_mt()
{
  init();
  route();
}

}  // namespace drt
