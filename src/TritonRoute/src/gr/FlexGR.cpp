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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include "FlexGR.h"
#include "db/obj/frGuide.h"
#include <fstream>
#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include <cmath>
#include "db/infra/frTime.h"
#include <omp.h>


using namespace std;
using namespace fr;

void FlexGR::main() {
  init();
  // resource analysis
  ra();
  // cmap->print(true);

  FlexGRCMap baseCMap(cmap_.get());
  FlexGRCMap baseCMap2D(cmap2D_.get());

  // gen topology + pattern route for 2D connectivty
  initGR(); 
  // populate region query for 2D
  getRegionQuery()->initGRObj(getTech()->getLayers().size());

  getRegionQuery()->printGRObj();
  
  reportCong2D();


  searchRepairMacro(0, 10, 2, 1 * CONGCOST, 0.5 * HISTCOST, 1.0, true, /*mode*/1);
  // reportCong2D();
  searchRepairMacro(1, 30, 2, 1 * CONGCOST, 1 * HISTCOST, 0.9, true, /*mode*/1);
  // reportCong2D();
  searchRepairMacro(2, 50, 2, 1 * CONGCOST, 1.5 * HISTCOST, 0.9, true, 1);
  // reportCong2D();
  searchRepairMacro(3, 80, 2, 2 * CONGCOST, 2 * HISTCOST, 0.9, true, 1);
  // reportCong2D();

  //  reportCong2D();
  searchRepair(/*iter*/0, /*size*/200, /*offset*/0, /*mazeEndIter*/2, /*workerCongCost*/1 * CONGCOST, /*workerHistCost*/0.5 * HISTCOST, /*congThresh*/0.9, /*is2DRouting*/true, /*mode*/1, /*TEST*/false);
  // reportCong2D();
  searchRepair(/*iter*/1, /*size*/200, /*offset*/-70, /*mazeEndIter*/2, /*workerCongCost*/1 * CONGCOST, /*workerHistCost*/1 * HISTCOST, /*congThresh*/0.9, /*is2DRouting*/true, /*mode*/1,  /*TEST*/false);
  // reportCong2D();
  searchRepair(/*iter*/2, /*size*/200, /*offset*/-150, /*mazeEndIter*/2, /*workerCongCost*/2 * CONGCOST, /*workerHistCost*/2 * HISTCOST, /*congThresh*/0.8, /*is2DRouting*/true, /*mode*/1, /*TEST*/false);
  // reportCong2D();
  
  reportCong2D();
  
  layerAssign();
  
  // populate region query for 3D
  getRegionQuery()->initGRObj(getTech()->getLayers().size());

  // reportCong3D();

  searchRepair(/*iter*/0, /*size*/10, /*offset*/0, /*mazeEndIter*/2, /*workerCongCost*/4 * CONGCOST, /*workerHistCost*/0.25 * HISTCOST, /*congThresh*/1.0, /*is2DRouting*/false, 1, /*TEST*/false);
  reportCong3D();

  writeToGuide();

  writeGuideFile();
}

void FlexGR::searchRepairMacro(int iter, int size, int mazeEndIter, unsigned workerCongCost, 
                               unsigned workerHistCost, double congThresh, bool is2DRouting, int mode) {
  frTime t;
  
  if (VERBOSE > 1) {
    cout <<endl <<"start " <<iter;
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
    cout <<suffix <<" optimization iteration for Macro..." <<endl;
  }

  frBox dieBox;
  getDesign()->getTopBlock()->getBoundaryBBox(dieBox);

  auto gCellPatterns = getDesign()->getTopBlock()->getGCellPatterns();
  auto &xgp = gCellPatterns.at(0);
  auto &ygp = gCellPatterns.at(1);

  vector<unique_ptr<FlexGRWorker> > uworkers;

  vector<frInst*> macros;

  for (auto &inst: getDesign()->getTopBlock()->getInsts()) {
    if (inst->getRefBlock()->getMacroClass() == MacroClassEnum::BLOCK) {
      frBox macroBBox;
      inst->getBBox(macroBBox);
      frPoint macroCenter((macroBBox.left() + macroBBox.right()) / 2, (macroBBox.bottom() + macroBBox.top()) / 2);
      frPoint macroCenterIdx;
      getDesign()->getTopBlock()->getGCellIdx(macroCenter, macroCenterIdx);
      if (cmap2D_->hasBlock(macroCenterIdx.x(), macroCenterIdx.y(), 0, frDirEnum::E) &&
          cmap2D_->hasBlock(macroCenterIdx.x(), macroCenterIdx.y(), 0, frDirEnum::N)) {
        macros.push_back(inst.get());
      }
    }
  }

  // create separate worker for each macro
  for (auto macro: macros) {
    auto worker = make_unique<FlexGRWorker>(this);
    frBox macroBBox;
    macro->getBBox(macroBBox);
    frPoint gcellIdxLL, gcellIdxUR;
    frPoint macroLL(macroBBox.left(), macroBBox.bottom());
    frPoint macroUR(macroBBox.right(), macroBBox.top());
    getDesign()->getTopBlock()->getGCellIdx(macroLL, gcellIdxLL);
    getDesign()->getTopBlock()->getGCellIdx(macroUR, gcellIdxUR);

    gcellIdxLL.set(max((int)gcellIdxLL.x() - size, 0), max((int)gcellIdxLL.y() - size, 0));
    gcellIdxUR.set(min((int)gcellIdxUR.x() + size, (int)xgp.getCount()), min((int)gcellIdxUR.y() + size, (int)ygp.getCount()));

    frBox routeBox1;
    getDesign()->getTopBlock()->getGCellBox(gcellIdxLL, routeBox1);
    frBox routeBox2;
    getDesign()->getTopBlock()->getGCellBox(gcellIdxUR, routeBox2);
    frBox extBox(routeBox1.left(), routeBox1.bottom(), routeBox2.right(), routeBox2.top());
    frBox routeBox((routeBox1.left() + routeBox1.right()) / 2,
                   (routeBox1.bottom() + routeBox1.top()) / 2,
                   (routeBox2.left() + routeBox2.right()) / 2,
                   (routeBox2.bottom() + routeBox2.top()) / 2);

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
  for (int i = 0; i < (int)uworkers.size(); i++) {
    uworkers[i]->initBoundary();
    uworkers[i]->main_mt();
    uworkers[i]->end();
  }
  uworkers.clear();
}

void FlexGR::searchRepair(int iter, int size, int offset, int mazeEndIter, 
                          unsigned workerCongCost, unsigned workerHistCost, 
                          double congThresh, bool is2DRouting, int mode, bool TEST) {
  frTime t;

  if (VERBOSE > 0) {
    cout <<endl <<"start " <<iter;
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
    cout <<suffix <<" optimization iteration ..." <<endl;
  }

  frBox dieBox;
  getDesign()->getTopBlock()->getBoundaryBBox(dieBox);

  auto gCellPatterns = getDesign()->getTopBlock()->getGCellPatterns();
  auto &xgp = gCellPatterns.at(0);
  auto &ygp = gCellPatterns.at(1);

  if (TEST) {
    cout <<"search and repair test mode" <<endl <<flush;

    FlexGRWorker worker(this);
    frBox extBox(1847999, 440999, 1857000, 461999);
    frBox routeBox(1849499, 442499, 1855499, 460499);
    frPoint gcellIdxLL(616, 147);
    frPoint gcellIdxUR(618, 153);

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
    vector<unique_ptr<FlexGRWorker> > uworkers;
    int batchStepX, batchStepY;

    getBatchInfo(batchStepX, batchStepY);

    vector<vector<vector<unique_ptr<FlexGRWorker> > > > workers(batchStepX * batchStepY);

    int xIdx = 0;
    int yIdx = 0;
    // sequential init
    for (int i = 0; i < (int)xgp.getCount(); i += size) {
      for (int j = 0; j < (int)ygp.getCount(); j += size) {
        auto worker = make_unique<FlexGRWorker>(this);
        frPoint gcellIdxLL = frPoint(i, j);
        frPoint gcellIdxUR = frPoint(min((int)xgp.getCount() - 1, i + size-1), min((int)ygp.getCount(), j + size-1));

        frBox routeBox1;
        getDesign()->getTopBlock()->getGCellBox(gcellIdxLL, routeBox1);
        frBox routeBox2;
        getDesign()->getTopBlock()->getGCellBox(gcellIdxUR, routeBox2);
        frBox extBox(routeBox1.left(), routeBox1.bottom(), routeBox2.right(), routeBox2.top());
        frBox routeBox((routeBox1.left() + routeBox1.right()) / 2,
                       (routeBox1.bottom() + routeBox1.top()) / 2,
                       (routeBox2.left() + routeBox2.right()) / 2,
                       (routeBox2.bottom() + routeBox2.top()) / 2);

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
        if (workers[batchIdx].empty() || (int)workers[batchIdx].back().size() >= BATCHSIZE) {
          workers[batchIdx].push_back(vector<unique_ptr<FlexGRWorker> >());
        }
        workers[batchIdx].back().push_back(std::move(worker));

        yIdx++;
      }
      yIdx = 0;
      xIdx++;
    }


    omp_set_num_threads(min(8, MAX_THREADS));
    // omp_set_num_threads(1);

    // parallel execution
    for (auto &workerBatch: workers) {
      for (auto &workersInBatch: workerBatch) {
        // single thread
        // split cross-worker boundary pathSeg
        for (int i = 0; i < (int)workersInBatch.size(); i++) {
          workersInBatch[i]->initBoundary();
        }
        // multi thread
        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < (int)workersInBatch.size(); i++) {
          workersInBatch[i]->main_mt();
        }
        // single thread
        for (int i = 0; i < (int)workersInBatch.size(); i++) {
          workersInBatch[i]->end();
        }
        workersInBatch.clear();
      }
    }
  }

  t.print(logger_);
  cout << endl << flush;

}

void FlexGR::reportCong2DGolden(FlexGRCMap *baseCMap2D) {
  FlexGRCMap goldenCMap2D(baseCMap2D);

  for (auto &net: design_->getTopBlock()->getNets()) {
    for (auto &uGRShape: net->getGRShapes()) {
      auto ps = static_cast<grPathSeg*>(uGRShape.get());
      frPoint bp, ep;
      ps->getPoints(bp, ep);

      frPoint bpIdx, epIdx;
      design_->getTopBlock()->getGCellIdx(bp, bpIdx);
      design_->getTopBlock()->getGCellIdx(ep, epIdx);

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

  cout << "start reporting golden 2D congestion...";
  reportCong2D(&goldenCMap2D);
}

void FlexGR::reportCong2D(FlexGRCMap *cmap2D) {
  if (VERBOSE > 0) {
    cout << endl << "start reporting 2D congestion ...\n\n";
  }

  auto &gCellPatterns = design_->getTopBlock()->getGCellPatterns();
  auto xgp = &(gCellPatterns.at(0));
  auto ygp = &(gCellPatterns.at(1));

  cout << "              #OverCon  %OverCon" << endl;
  cout << "Direction        GCell     GCell" << endl;
  cout << "--------------------------------" << endl;

  int numGCell = xgp->getCount() * ygp->getCount();
  int numOverConGCellH = 0;
  int numOverConGCellV = 0;
  double worstConH = 0.0;
  double worstConV = 0.0;
  vector<double> conH;
  vector<double> conV;

  for (unsigned xIdx = 0; xIdx < xgp->getCount(); xIdx++) {
    for (unsigned yIdx = 0; yIdx < ygp->getCount(); yIdx++) {
      // H
      // update num overCon GCell
      auto supplyH = cmap2D->getSupply(xIdx, yIdx, 0, frDirEnum::E);
      auto demandH = cmap2D->getDemand(xIdx, yIdx, 0, frDirEnum::E);
      if (demandH > supplyH && cmap2D->hasBlock(xIdx, yIdx, 0, frDirEnum::E) == false) {
        numOverConGCellH++;
      }
      // update worstCon
      if ((demandH * 100.0 / supplyH) > worstConH && cmap2D->hasBlock(xIdx, yIdx, 0, frDirEnum::E) == false) {
        worstConH = demandH * 100.0 / supplyH;
      }
      if (cmap2D->hasBlock(xIdx, yIdx, 0, frDirEnum::E) == false) {
        conH.push_back(demandH * 100.0 / supplyH);
      }
      // V
      // update num overCon GCell
      auto supplyV = cmap2D->getSupply(xIdx, yIdx, 0, frDirEnum::N);
      auto demandV = cmap2D->getDemand(xIdx, yIdx, 0, frDirEnum::N);
      if (demandV > supplyV && cmap2D->hasBlock(xIdx, yIdx, 0, frDirEnum::N) == false) {
        numOverConGCellV++;
      }
      // update worstCon
      if ((demandV * 100.0 / supplyV) > worstConV && cmap2D->hasBlock(xIdx, yIdx, 0, frDirEnum::N) == false) {
        worstConV = demandV * 100.0 / supplyV;
      }
      if (cmap2D->hasBlock(xIdx, yIdx, 0, frDirEnum::N) == false) {
        conV.push_back(demandV * 100.0 / supplyV);
      }
    }
  }

  sort(conH.begin(), conH.end());
  sort(conV.begin(), conV.end());

  cout << "    H        " << setw(8) << numOverConGCellH << "   " << setw(6) << fixed << numOverConGCellH * 100.0 / numGCell << "%\n";
  cout << "    V        " << setw(8) << numOverConGCellV << "   " << setw(6) << fixed << numOverConGCellV * 100.0 / numGCell << "%\n";
  cout << "worstConH: " << setw(6) << fixed << worstConH << "%\n";
  // cout << "  25-pencentile congestion H: " << conH[int(conH.size() * 0.25)] << "%\n";
  // cout << "  50-pencentile congestion H: " << conH[int(conH.size() * 0.50)] << "%\n";
  // cout << "  75-pencentile congestion H: " << conH[int(conH.size() * 0.75)] << "%\n";
  // cout << "  90-pencentile congestion H: " << conH[int(conH.size() * 0.90)] << "%\n";
  // cout << "  95-pencentile congestion H: " << conH[int(conH.size() * 0.95)] << "%\n";
  cout << "worstConV: " << setw(6) << fixed << worstConV << "%\n";
  // cout << "  25-pencentile congestion V: " << conV[int(conV.size() * 0.25)] << "%\n";
  // cout << "  50-pencentile congestion V: " << conV[int(conV.size() * 0.50)] << "%\n";
  // cout << "  75-pencentile congestion V: " << conV[int(conV.size() * 0.75)] << "%\n";
  // cout << "  90-pencentile congestion V: " << conV[int(conV.size() * 0.90)] << "%\n";
  // cout << "  95-pencentile congestion V: " << conV[int(conV.size() * 0.95)] << "%\n";

  cout << endl;
}

void FlexGR::reportCong2D() {
  if (VERBOSE > 0) {
    cout << endl << "start reporting 2D congestion ...\n\n";
  }

  auto &gCellPatterns = design_->getTopBlock()->getGCellPatterns();
  auto xgp = &(gCellPatterns.at(0));
  auto ygp = &(gCellPatterns.at(1));

  cout << "              #OverCon  %OverCon" << endl;
  cout << "Direction        GCell     GCell" << endl;
  cout << "--------------------------------" << endl;

  int numGCell = xgp->getCount() * ygp->getCount();
  int numOverConGCellH = 0;
  int numOverConGCellV = 0;
  double worstConH = 0.0;
  double worstConV = 0.0;
  vector<double> conH;
  vector<double> conV;

  for (unsigned xIdx = 0; xIdx < xgp->getCount(); xIdx++) {
    for (unsigned yIdx = 0; yIdx < ygp->getCount(); yIdx++) {
      // H
      // update num overCon GCell
      auto supplyH = cmap2D_->getSupply(xIdx, yIdx, 0, frDirEnum::E);
      auto demandH = cmap2D_->getDemand(xIdx, yIdx, 0, frDirEnum::E);
      if (demandH > supplyH && cmap2D_->hasBlock(xIdx, yIdx, 0, frDirEnum::E) == false) {
        numOverConGCellH++;
      }
      // update worstCon
      if ((demandH * 100.0 / supplyH) > worstConH && cmap2D_->hasBlock(xIdx, yIdx, 0, frDirEnum::E) == false) {
        worstConH = demandH * 100.0 / supplyH;
      }
      if (cmap2D_->hasBlock(xIdx, yIdx, 0, frDirEnum::E) == false) {
        conH.push_back(demandH * 100.0 / supplyH);
      }
      // V
      // update num overCon GCell
      auto supplyV = cmap2D_->getSupply(xIdx, yIdx, 0, frDirEnum::N);
      auto demandV = cmap2D_->getDemand(xIdx, yIdx, 0, frDirEnum::N);
      if (demandV > supplyV && cmap2D_->hasBlock(xIdx, yIdx, 0, frDirEnum::N) == false) {
        numOverConGCellV++;
      }
      // update worstCon
      if ((demandV * 100.0 / supplyV) > worstConV && cmap2D_->hasBlock(xIdx, yIdx, 0, frDirEnum::N) == false) {
        worstConV = demandV * 100.0 / supplyV;
      }
      if (cmap2D_->hasBlock(xIdx, yIdx, 0, frDirEnum::N) == false) {
        conV.push_back(demandV * 100.0 / supplyV);
      }
    }
  }

  sort(conH.begin(), conH.end());
  sort(conV.begin(), conV.end());

  cout << "    H        " << setw(8) << numOverConGCellH << "   " << setw(6) << fixed << numOverConGCellH * 100.0 / numGCell << "%\n";
  cout << "    V        " << setw(8) << numOverConGCellV << "   " << setw(6) << fixed << numOverConGCellV * 100.0 / numGCell << "%\n";
  cout << "worstConH: " << setw(6) << fixed << worstConH << "%\n";
  // cout << "  25-pencentile congestion H: " << conH[int(conH.size() * 0.25)] << "%\n";
  // cout << "  50-pencentile congestion H: " << conH[int(conH.size() * 0.50)] << "%\n";
  // cout << "  75-pencentile congestion H: " << conH[int(conH.size() * 0.75)] << "%\n";
  // cout << "  90-pencentile congestion H: " << conH[int(conH.size() * 0.90)] << "%\n";
  // cout << "  95-pencentile congestion H: " << conH[int(conH.size() * 0.95)] << "%\n";
  cout << "worstConV: " << setw(6) << fixed << worstConV << "%\n";
  // cout << "  25-pencentile congestion V: " << conV[int(conV.size() * 0.25)] << "%\n";
  // cout << "  50-pencentile congestion V: " << conV[int(conV.size() * 0.50)] << "%\n";
  // cout << "  75-pencentile congestion V: " << conV[int(conV.size() * 0.75)] << "%\n";
  // cout << "  90-pencentile congestion V: " << conV[int(conV.size() * 0.90)] << "%\n";
  // cout << "  95-pencentile congestion V: " << conV[int(conV.size() * 0.95)] << "%\n";

  cout << endl;
}

void FlexGR::reportCong3DGolden(FlexGRCMap *baseCMap) {
  FlexGRCMap goldenCMap3D(baseCMap);

  for (auto &net: design_->getTopBlock()->getNets()) {
    for (auto &uGRShape: net->getGRShapes()) {
      auto ps = static_cast<grPathSeg*>(uGRShape.get());
      frPoint bp, ep;
      ps->getPoints(bp, ep);
      frLayerNum lNum = ps->getLayerNum();

      frPoint bpIdx, epIdx;
      design_->getTopBlock()->getGCellIdx(bp, bpIdx);
      design_->getTopBlock()->getGCellIdx(ep, epIdx);

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

  cout << "start reporting golden 3D congestion...";
  reportCong3D(&goldenCMap3D);
  
}

void FlexGR::reportCong3D(FlexGRCMap *cmap) {
  bool enableOutput = false;

  if (VERBOSE > 0) {
    cout << endl << "start reporting 3D congestion ...\n\n";
  }

  auto &gCellPatterns = design_->getTopBlock()->getGCellPatterns();
  auto xgp = &(gCellPatterns.at(0));
  auto ygp = &(gCellPatterns.at(1));

  int numGCell = xgp->getCount() * ygp->getCount();
  int numOverConGCell = 0;

  unsigned cmapLayerIdx = 0;
  for (auto &[layerNum, dir]: cmap->getZMap()) {
    vector<double> con;
    numOverConGCell = 0;

    auto layer = design_->getTech()->getLayer(layerNum);
    string layerName(layer->getName());
    cout << "---------- " << layerName << " ----------" << endl;
    // get congestion information
    for (unsigned xIdx = 0; xIdx < xgp->getCount(); xIdx++) {
      for (unsigned yIdx = 0; yIdx < ygp->getCount(); yIdx++) {
        if (layer->getDir() == frPrefRoutingDirEnum::frcHorzPrefRoutingDir) {
          auto supply = cmap->getRawSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::E);
          auto demand = cmap->getRawDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::E);
          if (demand > supply && cmap->hasBlock(xIdx, yIdx, cmapLayerIdx, frDirEnum::E) == false) {
            numOverConGCell++;
            if (enableOutput) {
              frBox gcellBox;
              design_->getTopBlock()->getGCellBox(frPoint(xIdx, yIdx), gcellBox);
              double dbu = getDesign()->getTopBlock()->getDBUPerUU();
              cout << "  Warning: overCon GCell (" << gcellBox.left() / dbu << ", " << gcellBox.bottom() / dbu
                   << ") - (" << gcellBox.right() / dbu << ", " << gcellBox.top() / dbu 
                   << "), rawDemand / rawSupply = " << demand << "/" << supply << "\n";
            }
          }
          if (cmap->hasBlock(xIdx, yIdx, cmapLayerIdx, frDirEnum::E) == false) {
            con.push_back(demand * 100.0 / supply);
          }
        } else {
          auto supply = cmap->getRawSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::N);
          auto demand = cmap->getRawDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::N);
          if (demand > supply && cmap->hasBlock(xIdx, yIdx, cmapLayerIdx, frDirEnum::N) == false) {
            numOverConGCell++;
            if (enableOutput) {
              frBox gcellBox;
              design_->getTopBlock()->getGCellBox(frPoint(xIdx, yIdx), gcellBox);
              double dbu = getDesign()->getTopBlock()->getDBUPerUU();
              cout << "  Warning: overCon GCell (" << gcellBox.left() / dbu << ", " << gcellBox.bottom() / dbu
                   << ") - (" << gcellBox.right() / dbu << ", " << gcellBox.top() / dbu 
                   << "), rawDemand / rawSupply = " << demand << "/" << supply << "\n";
            }
          }
          if (cmap->hasBlock(xIdx, yIdx, cmapLayerIdx, frDirEnum::N) == false) {
            con.push_back(demand * 100.0 / supply);
          }
        }
      }
    }

    sort(con.begin(), con.end());

    cout << "numOverConGCell: " << numOverConGCell << ", %OverConGCell: " << setw(6) << fixed << numOverConGCell * 100.0 / numGCell << "%\n";
    
    // cout << "  25-pencentile congestion: " << con[int(con.size() * 0.25)] << "%\n";
    // cout << "  50-pencentile congestion: " << con[int(con.size() * 0.50)] << "%\n";
    // cout << "  75-pencentile congestion: " << con[int(con.size() * 0.75)] << "%\n";
    // cout << "  90-pencentile congestion: " << con[int(con.size() * 0.90)] << "%\n";
    // cout << "  95-pencentile congestion: " << con[int(con.size() * 0.95)] << "%\n";

    cmapLayerIdx++;
  }

  cout << endl;
}

void FlexGR::reportCong3D() {
  bool enableOutput = false;

  if (VERBOSE > 0) {
    cout << endl << "start reporting 3D congestion ...\n\n";
  }

  auto &gCellPatterns = design_->getTopBlock()->getGCellPatterns();
  auto xgp = &(gCellPatterns.at(0));
  auto ygp = &(gCellPatterns.at(1));

  int numGCell = xgp->getCount() * ygp->getCount();
  int numOverConGCell = 0;

  unsigned cmapLayerIdx = 0;
  for (auto &[layerNum, dir]: cmap_->getZMap()) {
    vector<double> con;
    numOverConGCell = 0;

    auto layer = design_->getTech()->getLayer(layerNum);
    string layerName(layer->getName());
    cout << "---------- " << layerName << " ----------" << endl;
    // get congestion information
    for (unsigned xIdx = 0; xIdx < xgp->getCount(); xIdx++) {
      for (unsigned yIdx = 0; yIdx < ygp->getCount(); yIdx++) {
        if (layer->getDir() == frPrefRoutingDirEnum::frcHorzPrefRoutingDir) {
          auto supply = cmap_->getRawSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::E);
          auto demand = cmap_->getRawDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::E);
          if (demand > supply && cmap_->hasBlock(xIdx, yIdx, cmapLayerIdx, frDirEnum::E) == false) {
            numOverConGCell++;
            if (enableOutput) {
              frBox gcellBox;
              design_->getTopBlock()->getGCellBox(frPoint(xIdx, yIdx), gcellBox);
              double dbu = getDesign()->getTopBlock()->getDBUPerUU();
              cout << "  Warning: overCon GCell (" << gcellBox.left() / dbu << ", " << gcellBox.bottom() / dbu
                   << ") - (" << gcellBox.right() / dbu << ", " << gcellBox.top() / dbu 
                   << "), rawDemand / rawSupply = " << demand << "/" << supply << "\n";
            }
          }
          if (cmap_->hasBlock(xIdx, yIdx, cmapLayerIdx, frDirEnum::E) == false) {
            con.push_back(demand * 100.0 / supply);
          }
        } else {
          auto supply = cmap_->getRawSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::N);
          auto demand = cmap_->getRawDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::N);
          if (demand > supply && cmap_->hasBlock(xIdx, yIdx, cmapLayerIdx, frDirEnum::N) == false) {
            numOverConGCell++;
            if (enableOutput) {
              frBox gcellBox;
              design_->getTopBlock()->getGCellBox(frPoint(xIdx, yIdx), gcellBox);
              double dbu = getDesign()->getTopBlock()->getDBUPerUU();
              cout << "  Warning: overCon GCell (" << gcellBox.left() / dbu << ", " << gcellBox.bottom() / dbu
                   << ") - (" << gcellBox.right() / dbu << ", " << gcellBox.top() / dbu 
                   << "), rawDemand / rawSupply = " << demand << "/" << supply << "\n";
            }
          }
          if (cmap_->hasBlock(xIdx, yIdx, cmapLayerIdx, frDirEnum::N) == false) {
            con.push_back(demand * 100.0 / supply);
          }
        }
      }
    }

    sort(con.begin(), con.end());

    cout << "numOverConGCell: " << numOverConGCell << ", %OverConGCell: " << setw(6) << fixed << numOverConGCell * 100.0 / numGCell << "%\n";
    
    // cout << "  25-pencentile congestion: " << con[int(con.size() * 0.25)] << "%\n";
    // cout << "  50-pencentile congestion: " << con[int(con.size() * 0.50)] << "%\n";
    // cout << "  75-pencentile congestion: " << con[int(con.size() * 0.75)] << "%\n";
    // cout << "  90-pencentile congestion: " << con[int(con.size() * 0.90)] << "%\n";
    // cout << "  95-pencentile congestion: " << con[int(con.size() * 0.95)] << "%\n";

    cmapLayerIdx++;
  }

  cout << endl;
}

// resource analysis
void FlexGR::ra() {
  frTime t;
  if (VERBOSE > 0) {
    cout << endl << "start routing resource analysis ...\n\n";
  }

  auto &gCellPatterns = design_->getTopBlock()->getGCellPatterns();
  auto xgp = &(gCellPatterns.at(0));
  auto ygp = &(gCellPatterns.at(1));

  int totNumTrack = 0;
  int totNumBlockedTrack = 0;
  int totNumGCell = 0;
  int totNumBlockedGCell = 0;

  cout << "             Routing   #Avail     #Track     #Total     %Gcell" << endl;
  cout << "Layer      Direction    Track    Blocked      Gcell    Blocked" << endl;
  cout << "--------------------------------------------------------------" << endl;
  
  unsigned cmapLayerIdx = 0;
  for (auto &[layerNum, dir]: cmap_->getZMap()) {
    auto layer = design_->getTech()->getLayer(layerNum);
    string layerName(layer->getName());
    layerName.append(16 - layerName.size(), ' ');
    cout << layerName;

    int numTrack = 0;
    int numBlockedTrack = 0;
    int numGCell = xgp->getCount() * ygp->getCount();
    int numBlockedGCell = 0;
    if (dir == frPrefRoutingDirEnum::frcHorzPrefRoutingDir) {
      cout << "H      ";
      for (unsigned yIdx = 0; yIdx < ygp->getCount(); yIdx++) {
        numTrack += cmap_->getSupply(0, yIdx, cmapLayerIdx, frDirEnum::E);
        for (unsigned xIdx = 0; xIdx < xgp->getCount(); xIdx++) {
          auto supply = cmap_->getSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::E);
          auto demand = cmap_->getDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::E);
          if (demand >= supply) {
            // if (cmapLayerIdx == 0) {
              // cout << "blocked gcell: xIdx = " << xIdx << ", yIdx = " << yIdx << ", supply = " << supply << ", demand = " << demand << endl;
            // }
            numBlockedGCell++;
          }
        }
      }
    } else if (dir == frPrefRoutingDirEnum::frcVertPrefRoutingDir) {
      cout << "V      ";
      for (unsigned xIdx = 0; xIdx < xgp->getCount(); xIdx++) {
        numTrack += cmap_->getSupply(xIdx, 0, cmapLayerIdx, frDirEnum::N);
        for (unsigned yIdx = 0; yIdx < ygp->getCount(); yIdx++) {
          auto supply = cmap_->getSupply(xIdx, yIdx, cmapLayerIdx, frDirEnum::N);
          auto demand = cmap_->getDemand(xIdx, yIdx, cmapLayerIdx, frDirEnum::N);
          if (demand >= supply) {
            numBlockedGCell++;
          }
        }
      }
    } else {
      cout << "UNKNOWN";
    }

    cout << setw(6) << numTrack << "     ";

    cout << setw(6) << numBlockedTrack << "     "; 

    cout << setw(6) << numGCell << "    ";

    cout << setw(6) << fixed << setprecision(2) << numBlockedGCell * 100.0 / numGCell << "%\n";

    // add to total
    totNumTrack += numTrack;
    totNumBlockedTrack += numBlockedTrack;
    totNumGCell += numGCell;
    totNumBlockedGCell += numBlockedGCell;

    cmapLayerIdx++;
  }
  
  cout << "--------------------------------------------------------------\n";
  cout << "Total                  ";
  cout << setw(6) << totNumTrack << "     ";
  cout << setw(5) << fixed << setprecision(2) << totNumBlockedTrack * 100.0 / totNumTrack << "%    ";
  cout << setw(7) << totNumGCell << "     ";
  cout << setw(5) << fixed << setprecision(2) << totNumBlockedGCell * 100.0 / totNumGCell << "%\n";


  if (VERBOSE > 0) {
    cout << endl;
    t.print(logger_);
  }

  cout << endl << endl;
}

// information to be reported after each iteration
void FlexGR::end() {
}


void FlexGR::initGR() {
  // check rpin and node equivalence
  for (auto &net: design_->getTopBlock()->getNets()) {
    // cout << net->getName() << " " << net->getNodes().size() << " " << net->getRPins().size() << "\n";
    if (net->getNodes().size() != net->getRPins().size()) {
      cout << "Error: net " << net->getName() << " initial #node != #rpin\n";
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
void FlexGR::initGR_updateCongestion() {
  for (auto &net: design_->getTopBlock()->getNets()) {
    initGR_updateCongestion_net(net.get());
  }
}

void FlexGR::initGR_updateCongestion_net(frNet *net) {
  for (auto &node: net->getNodes()) {
    if (node->getParent() == nullptr) {
      continue;
    }
    // only create shape and update congestion if haven't done before
    if (node->getConnFig()) {
      continue;
    }

    if (node->getType() != frNodeTypeEnum::frcSteiner || node->getParent()->getType() != frNodeTypeEnum::frcSteiner) {
      continue;
    }
    frPoint loc, parentLoc;
    node->getLoc(loc);
    node->getParent()->getLoc(parentLoc);
    if (loc.x() != parentLoc.x() && loc.y() != parentLoc.y()) {
      continue;
    }

    // generate shape and update 2D congestion map
    frPoint bp, ep;
    if (loc < parentLoc) {
      bp = loc;
      ep = parentLoc;
    } else {
      bp = parentLoc;
      ep = loc;
    }

    frPoint bpIdx, epIdx;
    design_->getTopBlock()->getGCellIdx(bp, bpIdx);
    design_->getTopBlock()->getGCellIdx(ep, epIdx);

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

void FlexGR::initGR_updateCongestion2D_net(frNet *net) {
  for (auto &node: net->getNodes()) {
    if (node->getParent() == nullptr) {
      continue;
    }

    if (node->getType() != frNodeTypeEnum::frcSteiner || node->getParent()->getType() != frNodeTypeEnum::frcSteiner) {
      continue;
    }
    frPoint loc, parentLoc;
    node->getLoc(loc);
    node->getParent()->getLoc(parentLoc);
    if (loc.x() != parentLoc.x() && loc.y() != parentLoc.y()) {
      continue;
    }

    // generate shape and update 2D congestion map
    frPoint bp, ep;
    if (loc < parentLoc) {
      bp = loc;
      ep = parentLoc;
    } else {
      bp = parentLoc;
      ep = loc;
    }

    frPoint bpIdx, epIdx;
    design_->getTopBlock()->getGCellIdx(bp, bpIdx);
    design_->getTopBlock()->getGCellIdx(ep, epIdx);

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

// if topology is from Flute, there will be non-colinear route need to be pattern routed
void FlexGR::initGR_patternRoute() {
  vector<pair<pair<frNode*, frNode*>, int> > patternRoutes; // <childNode, parentNode>, ripup cnt
  // init
  initGR_patternRoute_init(patternRoutes);
  // route
  initGR_patternRoute_route(patternRoutes);
}

void FlexGR::initGR_patternRoute_init(vector<pair<pair<frNode*, frNode*>, int> > &patternRoutes) {
  for (auto &net: design_->getTopBlock()->getNets()) {
    for (auto &node: net->getNodes()) {
      frNode *parentNode = node->getParent();
      if (parentNode == nullptr) {
        continue;
      }

      if (node->getType() != frNodeTypeEnum::frcSteiner || parentNode->getType() != frNodeTypeEnum::frcSteiner) {
        continue;
      }

      frPoint loc, parentLoc;
      node->getLoc(loc);
      parentNode->getLoc(parentLoc);
      if (loc.x() == parentLoc.x() || loc.y() == parentLoc.y()) {
        continue;
      }

      patternRoutes.push_back(make_pair(make_pair(node.get(), parentNode), 0));
    }
  }
}

void FlexGR::initGR_patternRoute_route(vector<pair<pair<frNode*, frNode*>, int> > &patternRoutes) {
  int maxIter = 2;
  for (int iter = 0; iter < maxIter; iter++) {
    initGR_patternRoute_route_iter(iter, patternRoutes, /*mode*/0);
  }
}

// mode 0 == L shape only
bool FlexGR::initGR_patternRoute_route_iter(int iter, vector<pair<pair<frNode*, frNode*>, int> > &patternRoutes, int mode) {
  bool hasOverflow = false;
  for (auto &patternRoutePair: patternRoutes) {
    auto &patternRoute = patternRoutePair.first;
    auto startNode = patternRoute.first;
    auto endNode = patternRoute.second;
    auto net = startNode->getNet();
    auto &rerouteCnt = patternRoutePair.second;
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
        default:
          ;
      }
      rerouteCnt++;
    }
  }
  return hasOverflow;
}

void FlexGR::patternRoute_LShape(frNode *child, frNode *parent) {
  auto net = child->getNet();
  frPoint childLoc, parentLoc;
  frPoint childGCellIdx, parentGCellIdx;
  child->getLoc(childLoc);
  parent->getLoc(parentLoc);

  design_->getTopBlock()->getGCellIdx(childLoc, childGCellIdx);
  design_->getTopBlock()->getGCellIdx(parentLoc, parentGCellIdx);

  frPoint cornerGCellIdx1(childGCellIdx.x(), parentGCellIdx.y());
  frPoint cornerGCellIdx2(parentGCellIdx.x(), childGCellIdx.y());

  // calculate corner1 cost
  double corner1Cost = 0;
  for (int xIdx = min(cornerGCellIdx1.x(), parentGCellIdx.x()); xIdx <= max(cornerGCellIdx1.x(), parentGCellIdx.x()); xIdx++) {
    auto rawSupply = cmap2D_->getRawSupply(xIdx, cornerGCellIdx1.y(), 0, frDirEnum::E);
    auto rawDemand = cmap2D_->getRawDemand(xIdx, cornerGCellIdx1.y(), 0, frDirEnum::E);
    corner1Cost += getCongCost(rawSupply, rawDemand);
    if (cmap2D_->getRawDemand(xIdx, cornerGCellIdx1.y(), 0, frDirEnum::E) >= cmap2D_->getRawSupply(xIdx, cornerGCellIdx1.y(), 0, frDirEnum::E)) {
      corner1Cost += BLOCKCOST;
    }
    if (cmap2D_->hasBlock(xIdx, cornerGCellIdx1.y(), 0, frDirEnum::E)) {
      corner1Cost += BLOCKCOST * 100;
    }
  }
  for (int yIdx = min(cornerGCellIdx1.y(), childGCellIdx.y()); yIdx <= max(cornerGCellIdx1.y(), childGCellIdx.y()); yIdx++) {
    auto rawSupply = cmap2D_->getRawSupply(cornerGCellIdx1.x(), yIdx, 0, frDirEnum::N);
    auto rawDemand = cmap2D_->getRawDemand(cornerGCellIdx1.x(), yIdx, 0, frDirEnum::N);
    corner1Cost += getCongCost(rawSupply, rawDemand);
    if (cmap2D_->getRawDemand(cornerGCellIdx1.x(), yIdx, 0, frDirEnum::N) >= cmap2D_->getRawSupply(cornerGCellIdx1.x(), yIdx, 0, frDirEnum::N)) {
      corner1Cost += BLOCKCOST;
    }
    if (cmap2D_->hasBlock(cornerGCellIdx1.x(), yIdx, 0, frDirEnum::N)) {
      corner1Cost += BLOCKCOST * 100;
    }
  }

  // calculate corner2 cost
  double corner2Cost = 0;
  for (int xIdx = min(cornerGCellIdx2.x(), childGCellIdx.x()); xIdx <= max(cornerGCellIdx2.x(), childGCellIdx.x()); xIdx++) {
    auto rawSupply = cmap2D_->getRawSupply(xIdx, cornerGCellIdx2.y(), 0, frDirEnum::E);
    auto rawDemand = cmap2D_->getRawDemand(xIdx, cornerGCellIdx2.y(), 0, frDirEnum::E);
    corner2Cost += getCongCost(rawSupply, rawDemand);
    if (cmap2D_->getRawDemand(xIdx, cornerGCellIdx2.y(), 0, frDirEnum::E) >= cmap2D_->getRawSupply(xIdx, cornerGCellIdx2.y(), 0, frDirEnum::E)) {
      corner2Cost += BLOCKCOST;
    }
    if (cmap2D_->hasBlock(xIdx, cornerGCellIdx2.y(), 0, frDirEnum::E)) {
      corner2Cost += BLOCKCOST * 100;
    }
  }
  for (int yIdx = min(cornerGCellIdx2.y(), parentGCellIdx.y()); yIdx <= max(cornerGCellIdx2.y(), parentGCellIdx.y()); yIdx++) {
    auto rawSupply = cmap2D_->getRawSupply(cornerGCellIdx2.x(), yIdx, 0, frDirEnum::N);
    auto rawDemand = cmap2D_->getRawDemand(cornerGCellIdx2.x(), yIdx, 0, frDirEnum::N);
    corner2Cost += getCongCost(rawSupply, rawDemand);
    if (cmap2D_->getRawDemand(cornerGCellIdx2.x(), yIdx, 0, frDirEnum::N) >= cmap2D_->getRawSupply(cornerGCellIdx2.x(), yIdx, 0, frDirEnum::N)) {
      corner2Cost += BLOCKCOST;
    }
    if (cmap2D_->hasBlock(cornerGCellIdx2.x(), yIdx, 0, frDirEnum::N)) {
      corner2Cost += BLOCKCOST * 100;
    }
  }

  if (corner1Cost < corner2Cost) {
    // create corner1 node
    auto uNode = make_unique<frNode>();
    uNode->setType(frNodeTypeEnum::frcSteiner);
    frPoint cornerLoc(childLoc.x(), parentLoc.y());
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
    for (int xIdx = min(cornerGCellIdx1.x(), parentGCellIdx.x()); xIdx < max(cornerGCellIdx1.x(), parentGCellIdx.x()); xIdx++) {
      cmap2D_->addRawDemand(xIdx, cornerGCellIdx1.y(), 0, frDirEnum::E);
      cmap2D_->addRawDemand(xIdx + 1, cornerGCellIdx1.y(), 0, frDirEnum::E);
    }
    for (int yIdx = min(cornerGCellIdx1.y(), childGCellIdx.y()); yIdx < max(cornerGCellIdx1.y(), childGCellIdx.y()); yIdx++) {
      cmap2D_->addRawDemand(cornerGCellIdx1.x(), yIdx, 0, frDirEnum::N);
      cmap2D_->addRawDemand(cornerGCellIdx1.x(), yIdx + 1, 0, frDirEnum::N);
    }
  } else {
    // create corner2 route
    auto uNode = make_unique<frNode>();
    uNode->setType(frNodeTypeEnum::frcSteiner);
    frPoint cornerLoc(parentLoc.x(), childLoc.y());
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
    for (int xIdx = min(cornerGCellIdx2.x(), childGCellIdx.x()); xIdx < max(cornerGCellIdx2.x(), childGCellIdx.x()); xIdx++) {
      cmap2D_->addRawDemand(xIdx, cornerGCellIdx2.y(), 0, frDirEnum::E);
      cmap2D_->addRawDemand(xIdx + 1, cornerGCellIdx2.y(), 0, frDirEnum::E);
    }
    for (int yIdx = min(cornerGCellIdx2.y(), parentGCellIdx.y()); yIdx < max(cornerGCellIdx2.y(), parentGCellIdx.y()); yIdx++) {
      cmap2D_->addRawDemand(cornerGCellIdx2.x(), yIdx, 0, frDirEnum::N);
      cmap2D_->addRawDemand(cornerGCellIdx2.x(), yIdx + 1, 0, frDirEnum::N);
    }
  }
}

double FlexGR::getCongCost(unsigned supply, unsigned demand) {
  return demand * (1.0 + 8.0 / (1.0 + exp(supply - demand))) / (supply + 1);
}

// child node and parent node must be colinear
void FlexGR::ripupRoute(frNode *child, frNode *parent) {
  frPoint childLoc, parentLoc, bp, ep;
  child->getLoc(childLoc);
  parent->getLoc(parentLoc);
  if (childLoc < parentLoc) {
    bp = childLoc;
    ep = parentLoc;
  } else {
    bp = parentLoc;
    ep = childLoc;
  }

  frPoint bpIdx, epIdx;
  design_->getTopBlock()->getGCellIdx(bp, bpIdx);
  design_->getTopBlock()->getGCellIdx(ep, epIdx);

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
bool FlexGR::hasOverflow2D(frNode *child, frNode *parent) {
  bool isOverflow = false;
  frPoint childLoc, parentLoc, bp, ep;
  child->getLoc(childLoc);
  parent->getLoc(parentLoc);
  if (childLoc < parentLoc) {
    bp = childLoc;
    ep = parentLoc;
  } else {
    bp = parentLoc;
    ep = childLoc;
  }

  frPoint bpIdx, epIdx;
  design_->getTopBlock()->getGCellIdx(bp, bpIdx);
  design_->getTopBlock()->getGCellIdx(ep, epIdx);

  if (bpIdx.y() == epIdx.y()) {
    int yIdx = bpIdx.y();
    for (int xIdx = bpIdx.x(); xIdx <= epIdx.x(); xIdx++) {
      if (cmap2D_->getRawDemand(xIdx, yIdx, 0, frDirEnum::E) > cmap2D_->getRawSupply(xIdx, yIdx, 0, frDirEnum::E)) {
        isOverflow = true;
        break;
      }
    }
  } else {
    int xIdx = bpIdx.x();
    for (int yIdx = bpIdx.y(); yIdx <= epIdx.y(); yIdx++) {
      if (cmap2D_->getRawDemand(xIdx, yIdx, 0, frDirEnum::N) > cmap2D_->getRawSupply(xIdx, yIdx, 0, frDirEnum::N)) {
        isOverflow = true;
        break;
      }
    }
  }

  return isOverflow;
}

void FlexGR::initGR_initObj() {
  for (auto &net: design_->getTopBlock()->getNets()) {
    initGR_initObj_net(net.get());

    int steinerNodeCnt = net->getNodes().size() - net->getRPins().size();
    if (steinerNodeCnt != 0 && (int) net->getGRShapes().size() != (steinerNodeCnt - 1)) {
      cout << "Error: " << net->getName() << " has " << steinerNodeCnt << " steiner nodes, but " << net->getGRShapes().size() << " pathSegs\n";
    }
  }
}

void FlexGR::initGR_initObj_net(frNet* net) {
  deque<frNode*> nodeQ;

  nodeQ.push_back(net->getRoot());

  while (!nodeQ.empty()) {
    auto node = nodeQ.front();
    nodeQ.pop_front();
    // push children
    for (auto child: node->getChildren()) {
      nodeQ.push_back(child);
    }

    if (node->getParent() == nullptr) {
      continue;
    }
    if (node->getType() != frNodeTypeEnum::frcSteiner || node->getParent()->getType() != frNodeTypeEnum::frcSteiner) {
      continue;
    }

    if (node->getLayerNum() != 2) {
      cout << "Error: node not on layerNum == 2 (" << node->getLayerNum() << ") before layerAssignment\n";
    }
    if (node->getParent()->getLayerNum() != 2) {
      cout << "Error: node not on layerNum == 2 (" << node->getParent()->getLayerNum() << ") before layerAssignment\n";
    }

    auto parent = node->getParent();

    frPoint nodeLoc = node->getLoc();
    frPoint parentLoc = parent->getLoc();

    frPoint bp, ep;
    if (nodeLoc < parentLoc) {
      bp = nodeLoc;
      ep = parentLoc;
    } else {
      bp = parentLoc;
      ep = nodeLoc;
    }

    if (nodeLoc.y() == parentLoc.y()) {
      // horz
      auto uPathSeg = make_unique<grPathSeg>();
      uPathSeg->setChild(node);
      node->setConnFig(uPathSeg.get());
      uPathSeg->setParent(parent);
      uPathSeg->addToNet(net);
      uPathSeg->setPoints(bp, ep);
      uPathSeg->setLayerNum(2);

      unique_ptr<grShape> uShape(std::move(uPathSeg));
      net->addGRShape(uShape);
    } else if (nodeLoc.x() == parentLoc.x()) {
      // vert
      auto uPathSeg = make_unique<grPathSeg>();
      uPathSeg->setChild(node);
      node->setConnFig(uPathSeg.get());
      uPathSeg->setParent(parent);
      uPathSeg->addToNet(net);
      uPathSeg->setPoints(bp, ep);
      uPathSeg->setLayerNum(2);

      unique_ptr<grShape> uShape(std::move(uPathSeg));
      net->addGRShape(uShape);
    } else {
      cout << "Error: non-colinear nodes in post patternRoute\n";
    }

  }
}

void FlexGR::initGR_genTopology() {
  cout << "generating net topology...\n";
  // Flute::readLUT();
  for (auto &net: design_->getTopBlock()->getNets()) {
    // generate MST (currently using Prim-Dijkstra) and steiner tree (currently using HVW)
    initGR_genTopology_net(net.get());
    initGR_updateCongestion2D_net(net.get());
  }
  cout << "done net topology...\n";
}


// generate 2D topology, rpin node always connect to center of gcell
// to be followed by layer assignment
void FlexGR::initGR_genTopology_net(frNet *net) {
  bool enableOutput = false;

  if (net->getNodes().size() == 0) {
    return;
  }

  if (net->getNodes().size() == 1) {
    net->setRoot(net->getNodes().front().get());
    return;
  }
  
  vector<frNode*> nodes(net->getNodes().size(), nullptr); // 0 is source
  map<frBlockObject*, std::vector<frNode*> > pin2Nodes; // vector order needs to align with map below
  map<frBlockObject*, std::vector<frRPin*> > pin2RPins;
  unsigned sinkIdx = 1;

  auto &netNodes = net->getNodes();
  // init nodes and populate pin2Nodes
  for (auto &node: netNodes) {
    if (node->getPin()) {
      if (node->getPin()->typeId() == frcInstTerm) {
        auto term = static_cast<frInstTerm*>(node->getPin())->getTerm();
        // for instTerm, direction OUTPUT is driver
        if (term->getDirection() == frTermDirectionEnum::OUTPUT && nodes[0] == nullptr) {
          nodes[0] = node.get();
        } else {
          if (term->getDirection() == frTermDirectionEnum::OUTPUT) {
            if (enableOutput) {
              cout << "Warning: " << net->getName() << " has more than one driver pin\n";
            }
          }
          if (sinkIdx >= nodes.size()) {
            if (enableOutput) {
              cout << "Warning: " << net->getName() << " does not have driver pin\n";
            }
            sinkIdx %= nodes.size();
          }
          nodes[sinkIdx] = node.get();
          sinkIdx++;
        }
        pin2Nodes[node->getPin()].push_back(node.get());
      } else if (node->getPin()->typeId() == frcTerm) {
        auto term = static_cast<frTerm*>(node->getPin());
        // for IO term, direction INPUT is driver
        if (term->getDirection() == frTermDirectionEnum::INPUT && nodes[0] == nullptr) {
          nodes[0] = node.get();
        } else {
          if (term->getDirection() == frTermDirectionEnum::INPUT) {
            if (enableOutput) {
              cout << "Warning: " << net->getName() << " has more than one driver pin\n";
            }
          }
          if (sinkIdx >= nodes.size()) {
            if (enableOutput) {
              cout << "Warning: " << net->getName() << " does not have driver pin\n";
            }
            sinkIdx %= nodes.size();
          }
          nodes[sinkIdx] = node.get();
          sinkIdx++;
        }
        pin2Nodes[node->getPin()].push_back(node.get());
      } else {
        cout << "Error: unknown pin type in initGR_genTopology_net\n";
      }
    }
  }

  net->setRoot(nodes[0]);
  // populate pin2RPins
  for (auto &rpin: net->getRPins()) {
    if (rpin->getFrTerm()) {
      pin2RPins[rpin->getFrTerm()].push_back(rpin.get());
    }
  }
  // update nodes location based on rpin
  for (auto &[pin, nodes]: pin2Nodes) {
    if (pin2RPins.find(pin) == pin2RPins.end()) {
      cout << "Error: pin not found in pin2RPins\n";
      exit(1);
    }
    if (pin2RPins[pin].size() != nodes.size()) {
      cout << "Error: mismatch in nodes and ripins size\n";
      exit(1);
    }
    auto &rpins = pin2RPins[pin];
    for (int i = 0; i < (int)nodes.size(); i++) {
      auto rpin = rpins[i];
      auto node = nodes[i];
      frPoint pt;
      if (rpin->getFrTerm()->typeId() == frcInstTerm) {
        auto inst = static_cast<frInstTerm*>(rpin->getFrTerm())->getInst();
        frTransform shiftXform;
        inst->getTransform(shiftXform);
        shiftXform.set(frOrient(frcR0));
        rpin->getAccessPoint()->getPoint(pt);
        pt.transform(shiftXform);
      } else {
        rpin->getAccessPoint()->getPoint(pt);
      }
      node->setLoc(pt);
      node->setLayerNum(rpin->getAccessPoint()->getLayerNum());
    }
  }

  // map<pair<int, int>, vector<frNode*> > gcellIdx2Nodes;
  auto &gcellIdx2Nodes = net2GCellIdx2Nodes_[net];
  // map<frNode*, vector<frNode*> > gcellNode2RPinNodes;
  auto &gcellNode2RPinNodes = net2GCellNode2RPinNodes_[net];

  // prep for 2D topology generation in case two nodes are more than one rpin in same gcell
  // topology genration works on gcell (center-to-center) level
  frPoint apLoc, apGCellIdx;
  for (auto node: nodes) {
    node->getLoc(apLoc);
    design_->getTopBlock()->getGCellIdx(apLoc, apGCellIdx);
    gcellIdx2Nodes[make_pair(apGCellIdx.x(), apGCellIdx.y())].push_back(node);
  }

  // generate gcell-level node 
  // vector<frNode*> gcellNodes(gcellIdx2Nodes.size(), nullptr);
  auto &gcellNodes = net2GCellNodes_[net];
  gcellNodes.resize(gcellIdx2Nodes.size(), nullptr);

  vector<unique_ptr<frNode> > tmpGCellNodes;
  sinkIdx = 1;
  unsigned rootIdx = 0;
  unsigned rootIdxCnt = 0;
  for (auto &[gcellIdx, localNodes]: gcellIdx2Nodes) {
    bool hasRoot = false;
    for (auto localNode: localNodes) {
      if (localNode == nodes[0]) {
        hasRoot = true;
      }
    }

    frBox gcellBox;
    auto gcellNode = make_unique<frNode>();
    gcellNode->setType(frNodeTypeEnum::frcSteiner);
    design_->getTopBlock()->getGCellBox(frPoint(gcellIdx.first, gcellIdx.second), gcellBox);
    frPoint loc((gcellBox.left() + gcellBox.right()) / 2, (gcellBox.bottom() + gcellBox.top()) / 2);
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
    tmpGCellNodes.push_back(move(gcellNode));
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
      cout << "Error: gcell node " << i << " is 0x0\n";
    }
  }

  if (gcellNodes.size() <= 1) {
    return;
  }

  net->setRootGCellNode(gcellNodes[0]);

  auto &steinerNodes = net2SteinerNodes_[net];
  // if (gcellNodes.size() >= 150) {
  // TODO: remove connFig instantiation to match FLUTE behavior
  if (false) {
    // generate mst topology
    genMSTTopology(gcellNodes);

    // sanity check
    for (unsigned i = 1; i < gcellNodes.size(); i++) {
      if (gcellNodes[i]->getParent() == nullptr) {
        cout << "Error: non-root gcell node does not have parent\n";
      }
    }

    // generate steiner tree from MST
    genSTTopology_HVW(gcellNodes, steinerNodes);
    // generate shapes and update congestion map
    for (auto node: gcellNodes) {
      // add shape from child to parent
      if (node->getParent()) {
        auto parent = node->getParent();
        frPoint childLoc, parentLoc;
        frPoint bp, ep;
        node->getLoc(childLoc);
        parent->getLoc(parentLoc);
        if (childLoc < parentLoc) {
          bp = childLoc;
          ep = parentLoc;
        } else {
          bp = parentLoc;
          ep = childLoc;
        }

        auto uPathSeg = make_unique<grPathSeg>();
        uPathSeg->setChild(node);
        uPathSeg->setParent(parent);
        uPathSeg->addToNet(net);
        uPathSeg->setPoints(bp, ep);
        // 2D shapes are all on layerNum == 2
        // assuming (layerNum / - 1) == congestion map idx
        uPathSeg->setLayerNum(2);

        frPoint bpIdx, epIdx;
        design_->getTopBlock()->getGCellIdx(bp, bpIdx);
        design_->getTopBlock()->getGCellIdx(ep, epIdx);

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

        unique_ptr<grShape> uShape(std::move(uPathSeg));
        net->addGRShape(uShape);
      }
    }

    for (auto node: steinerNodes) {
      // add shape from child to parent
      if (node->getParent()) {
        auto parent = node->getParent();
        frPoint childLoc, parentLoc;
        frPoint bp, ep;
        node->getLoc(childLoc);
        parent->getLoc(parentLoc);
        if (childLoc < parentLoc) {
          bp = childLoc;
          ep = parentLoc;
        } else {
          bp = parentLoc;
          ep = childLoc;
        }

        auto uPathSeg = make_unique<grPathSeg>();
        uPathSeg->setChild(node);
        uPathSeg->setParent(parent);
        uPathSeg->addToNet(net);
        uPathSeg->setPoints(bp, ep);
        // 2D shapes are all on layerNum == 2
        // assuming (layerNum / - 1) == congestion map idx
        uPathSeg->setLayerNum(2);

        frPoint bpIdx, epIdx;
        design_->getTopBlock()->getGCellIdx(bp, bpIdx);
        design_->getTopBlock()->getGCellIdx(ep, epIdx);

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

        unique_ptr<grShape> uShape(std::move(uPathSeg));
        net->addGRShape(uShape);
      }
    }
  } else {
    genSTTopology_FLUTE(gcellNodes, steinerNodes);
  }

  // connect rpin node to gcell center node
  for (auto &[gcellNode, localNodes]: gcellNode2RPinNodes) {
    for (auto localNode: localNodes) {
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
  for (frNode* node : nodes) {
    if (node->getParent() == nullptr) {
      cout << "Error: non-root node does not have parent\n";
    }
  }
  if (nodes.size() > 1 && nodes[0]->getChildren().size() == 0) {
    cout << "Error: root does not have any children\n";
  }
}

void FlexGR::layerAssign() {
  cout << "layer assignment...\n";
  vector<pair<int, frNet*> > sortedNets;
  for (auto &uNet: design_->getTopBlock()->getNets()) {
    auto net = uNet.get();
    if (net2GCellNodes_.find(net) == net2GCellNodes_.end() || net2GCellNodes_[net].size() <= 1) {
      continue;
    }
    frCoord llx = INT_MAX;
    frCoord lly = INT_MAX;
    frCoord urx = INT_MIN;
    frCoord ury = INT_MIN;
    for (auto &rpin: net->getRPins()) {
      frBox bbox;
      rpin->getBBox(bbox);
      llx = min(bbox.left(), llx);
      lly = min(bbox.bottom(), lly);
      urx = max(bbox.right(), urx);
      ury = max(bbox.top(), ury);
    }
    int numRPins = net->getRPins().size();
    int ratio = ((urx - llx) + (ury - lly)) / (numRPins);
    sortedNets.push_back(make_pair(ratio, net));
  }

  // sort
  struct sort_net {
    bool operator()(const std::pair<int, frNet*> &left, const std::pair<int, frNet*> &right) {
      if (left.first == right.first) {
        return (left.second->getId() < right.second->getId());
      } else {
        return (left.first < right.first);
      }
    }
  };
  sort(sortedNets.begin(), sortedNets.end(), sort_net());

  for (auto &[ratio, net]: sortedNets) {
    layerAssign_net(net);
  }

  cout << "done layer assignment...\n";
}

void FlexGR::layerAssign_net(frNet *net) {
  net->clearGRShapes();

  if (net2GCellNodes_.find(net) == net2GCellNodes_.end() || net2GCellNodes_[net].size() <= 1) {
    return;
  }

  // update net2GCellNode2RPinNodes
  auto &gcellNode2RPinNodes = net2GCellNode2RPinNodes_[net];
  gcellNode2RPinNodes.clear();
  unsigned rpinNodeSize = net->getRPins().size();
  unsigned nodeCnt = 0;

  for (auto &node: net->getNodes()) {
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
  auto &nodes = net->getNodes();
  unsigned rpinNodeCnt = 0;

  // cout << net->getName() << endl << flush;

  for (auto &node: nodes) {
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

  for (auto &node: net->getNodes()) {
    node->setConnFig(nullptr);
  }

  int numNodes = net->getNodes().size() - net->getRPins().size();
  vector<vector<unsigned> > bestLayerCosts(numNodes, vector<unsigned>(cmap_->getNumLayers(), UINT_MAX));
  vector<vector<unsigned> > bestLayerCombs(numNodes, vector<unsigned>(cmap_->getNumLayers(), 0));


  // recursively compute the best layer for each node from root (post-order traversal)
  layerAssign_node_compute(net->getRootGCellNode(), net, bestLayerCosts, bestLayerCombs);

  // recursively update nodes to 3D
  frLayerNum minCostLayerNum = 0;
  unsigned minCost = UINT_MAX;
  int rootIdx = distance(net->getFirstNonRPinNode()->getIter(), net->getRootGCellNode()->getIter());

  for (frLayerNum layerNum = 0; layerNum < cmap_->getNumLayers(); layerNum++) {
    if (bestLayerCosts[rootIdx][layerNum] < minCost) {
      minCostLayerNum = layerNum;
      minCost = bestLayerCosts[rootIdx][layerNum];
    }
  }
  layerAssign_node_commit(net->getRootGCellNode(), net, minCostLayerNum, bestLayerCombs);

  // create shapes and update congestion
  for (auto &uNode: net->getNodes()) {
    auto node = uNode.get();
    if (node->getParent() == nullptr) {
      continue;
    }
    if (node->getType() == frNodeTypeEnum::frcPin ||
        node->getParent()->getType() == frNodeTypeEnum::frcPin) {
      continue;
    }
    // steiner-to-steiner
    auto parent = node->getParent();
    if (node->getLayerNum() == node->getParent()->getLayerNum()) {
      // pathSeg
      frPoint currLoc, parentLoc, bp, ep;
      node->getLoc(currLoc);
      parent->getLoc(parentLoc);

      if (currLoc < parentLoc) {
        bp = currLoc;
        ep = parentLoc;
      } else {
        bp = parentLoc;
        ep = currLoc;
      }

      auto uPathSeg = make_unique<grPathSeg>();
      uPathSeg->setChild(node);
      uPathSeg->setParent(parent);
      uPathSeg->addToNet(net);
      uPathSeg->setPoints(bp, ep);
      uPathSeg->setLayerNum(node->getLayerNum());

      frPoint bpIdx, epIdx;
      design_->getTopBlock()->getGCellIdx(bp, bpIdx);
      design_->getTopBlock()->getGCellIdx(ep, epIdx);

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

      unique_ptr<grShape> uShape(std::move(uPathSeg));
      net->addGRShape(uShape);
    } else {
      // via
      frPoint loc;
      frLayerNum beginLayerNum, endLayerNum;
      node->getLoc(loc);
      beginLayerNum = node->getLayerNum();
      endLayerNum = parent->getLayerNum();

      auto uVia = make_unique<grVia>();
      uVia->setChild(node);
      uVia->setParent(parent);
      uVia->addToNet(net);
      uVia->setOrigin(loc);
      uVia->setViaDef(design_->getTech()->getLayer((beginLayerNum + endLayerNum) / 2)->getDefaultViaDef());

      // assign to child
      node->setConnFig(uVia.get());

      // TODO: update congestion map

      net->addGRVia(uVia);
    }

  }
}

// get the costs of having currNode to parent edge on all layers
void FlexGR::layerAssign_node_compute(frNode *currNode,
                                      frNet *net,
                                      vector<vector<unsigned> > &bestLayerCosts,
                                      vector<vector<unsigned> > &bestLayerCombs) {
  if (currNode == nullptr) {
    return;
  }

  if (currNode->getChildren().empty()) {
    layerAssign_node_compute(nullptr, net, bestLayerCosts, bestLayerCombs);
  } else {
    for (auto child: currNode->getChildren()) {
      layerAssign_node_compute(child, net, bestLayerCosts, bestLayerCombs);
    }
  }

  unsigned numChild = currNode->getChildren().size();
  unsigned numComb = 1;
  unsigned currLayerCost = UINT_MAX;
  // since max degree is four (at most three children), so this should not overflow
  for (unsigned i = 0; i < numChild; i++) {
    numComb *= cmap_->getNumLayers();
  }
  int currNodeIdx = distance(net->getFirstNonRPinNode()->getIter(), currNode->getIter());
  // iterate over all combinations and get the combination with lowest overall cost
  for (int layerNum = 0; layerNum < cmap_->getNumLayers(); layerNum++) {
    unsigned currLayerBestCost = UINT_MAX;
    unsigned currLayerBestComb = 0;
    for (unsigned comb = 0; comb < numComb; comb++) {
      // get upstream via cost
      unsigned upstreamViaCost = 0;
      int minPinLayerNum = INT_MAX;
      int maxPinLayerNum = INT_MIN;
      if (net2GCellNode2RPinNodes_[net].find(currNode) != net2GCellNode2RPinNodes_[net].end()) {
        auto &rpinNodes = net2GCellNode2RPinNodes_[net][currNode];
        for (auto rpinNode: rpinNodes) {
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

      for (auto child: currNode->getChildren()) {
        int childNodeIdx = distance(net->getFirstNonRPinNode()->getIter(), child->getIter());
        int childLayerNum = currComb % cmap_->getNumLayers();
        if (downstreamMinLayerNum > childLayerNum) {
          downstreamMinLayerNum = childLayerNum;
        }
        if (downstreamMaxLayerNum < childLayerNum) {
          downstreamMaxLayerNum = childLayerNum;
        }
        currComb /= cmap_->getNumLayers();

        // add downstream cost
        downstreamCost += bestLayerCosts[childNodeIdx][childLayerNum];
      }

      // TODO: tune the via cost here
      downstreamViaCost = (max(layerNum, max(maxPinLayerNum, downstreamMaxLayerNum)) - 
                           min(layerNum, min(minPinLayerNum, downstreamMinLayerNum))) * VIACOST;

      // get upstream edge congestion cost
      unsigned congestionCost = 0;
      // bool isLayerBlocked = layerNum <= (VIA_ACCESS_LAYERNUM / 2 - 1);
      bool isLayerBlocked = false;
      
      frPoint currLoc, parentLoc;
      frPoint beginIdx, endIdx;
      currNode->getLoc(currLoc);
      if (currNode->getParent()) {
        auto parent = currNode->getParent();
        parent->getLoc(parentLoc);
      } else {
        parentLoc = currLoc;
      }

      if (layerNum <= (VIA_ACCESS_LAYERNUM / 2 - 1)) {
        congestionCost += VIACOST * 8;
      }

      if (parentLoc.x() != currLoc.x() || parentLoc.y() != currLoc.y()) {
        if (parentLoc < currLoc) {
          design_->getTopBlock()->getGCellIdx(parentLoc, beginIdx);
          design_->getTopBlock()->getGCellIdx(currLoc, endIdx);
        } else {
          design_->getTopBlock()->getGCellIdx(currLoc, beginIdx);
          design_->getTopBlock()->getGCellIdx(parentLoc, endIdx);
        }
        // horz
        if (beginIdx.y() == endIdx.y()) {
          if (design_->getTech()->getLayer((layerNum + 1) * 2)->getDir() == frcVertPrefRoutingDir) {
            isLayerBlocked = true;
          }
          int yIdx = beginIdx.y();
          for (int xIdx = beginIdx.x(); xIdx < endIdx.x(); xIdx++) {
            auto supply = cmap_->getRawSupply(xIdx, yIdx, layerNum, frDirEnum::E);
            auto demand = cmap_->getRawDemand(xIdx, yIdx, layerNum, frDirEnum::E);
            // block cost
            if (isLayerBlocked || cmap_->hasBlock(xIdx, yIdx, layerNum, frDirEnum::E)) {
              congestionCost += BLOCKCOST * 100;
            }
            // congestion cost            
            if (demand > supply / 4) {
              congestionCost += (demand * 10 / (supply + 1));
            }

            // overflow
            if (demand >= supply) {
              congestionCost += MARKERCOST * 8;
            }
          }
        } else {
          if (design_->getTech()->getLayer((layerNum + 1) * 2)->getDir() == frcHorzPrefRoutingDir) {
            isLayerBlocked = true;
          }
          int xIdx = beginIdx.x();
          for (int yIdx = beginIdx.y(); yIdx < endIdx.y(); yIdx++) {
            auto supply = cmap_->getRawSupply(xIdx, yIdx, layerNum, frDirEnum::N);
            auto demand = cmap_->getRawDemand(xIdx, yIdx, layerNum, frDirEnum::N);
            if (isLayerBlocked || cmap_->hasBlock(xIdx, yIdx, layerNum, frDirEnum::N)) {
              congestionCost += BLOCKCOST * 100;
            } 
            // congestion cost 
            if (demand > supply / 4) {
              congestionCost += (demand * 10 / (supply + 1));
            }
            // overflow
            if (demand >= supply) {
              congestionCost += MARKERCOST * 8;
            }
          }
        }
      }

      currLayerCost = upstreamViaCost + downstreamCost + downstreamViaCost + congestionCost;

      if (currLayerCost < currLayerBestCost) {
        currLayerBestCost = currLayerCost;
        currLayerBestComb = comb;
      }
    }
    bestLayerCosts[currNodeIdx][layerNum] = currLayerBestCost;
    bestLayerCombs[currNodeIdx][layerNum] = currLayerBestComb;
  }
}

void FlexGR::layerAssign_node_commit(frNode *currNode,
                                     frNet *net,
                                     frLayerNum layerNum, // which layer the connection from currNode to parentNode should be on
                                     vector<vector<unsigned> > &bestLayerCombs) {
  if (currNode == nullptr) {
    return;
  }
  int currNodeIdx = distance(net->getFirstNonRPinNode()->getIter(), currNode->getIter());
  vector<frNode*> children(currNode->getChildren().size(), nullptr);
  unsigned childIdx = 0;
  for (auto child: currNode->getChildren()) {
    int childNodeIdx = distance(net->getFirstNonRPinNode()->getIter(), child->getIter());
    if (childNodeIdx >= (int)bestLayerCombs.size()) {
      cout << net->getName() << endl;
      cout << "Error: non-pin gcell or non-steiner child node, childNodeIdx = " << childNodeIdx << ", currNodeIdx = " << currNodeIdx << ", bestLayerCombs.size() = " << bestLayerCombs.size() << "\n";
      frPoint loc1, loc2;
      currNode->getLoc(loc1);
      child->getLoc(loc2);
      cout << "currNodeLoc = (" << loc1.x() / 2000.0 << ", " << loc1.y() / 2000.0 << "), childLoc = (" << loc2.x() / 2000.0 << ", " << loc2.y() / 2000.0 << ")\n";
      cout << "currNodeType = " << (int)(currNode->getType()) << ", childNodeType = " << (int)(child->getType()) << endl;
      exit(1);
    }


    if (currNodeIdx >= (int)bestLayerCombs.size()) {
      cout << "Error: non-pin gcell or non-steiner node, currNodeIdx = " << currNodeIdx << ", parentNodeIdx = " << distance(net->getFirstNonRPinNode()->getIter(), currNode->getParent()->getIter()) << "\n";
      frPoint loc1, loc2;
      currNode->getLoc(loc1);
      currNode->getParent()->getLoc(loc2);
      cout << "currNodeLoc = (" << loc1.x() / 2000.0 << ", " << loc1.y() / 2000.0 << "), parentLoc = (" << loc2.x() / 2000.0 << ", " << loc2.y() / 2000.0 << ")\n";
      cout << "currNodeType = " << (int)(currNode->getType()) << endl;
      exit(1);
    }
    if (child->getType() == frNodeTypeEnum::frcPin) {
      frPoint loc;
      child->getLoc(loc);
      cout << "Error1: currNodeIdx = " << currNodeIdx << ", should not commit pin node, loc(" << loc.x() / 2000.0 << ", " << loc.y() / 2000.0 << ")\n";
      exit(1);
    }
    children[childIdx] = child;
    childIdx++;
  }

  auto comb = bestLayerCombs[currNodeIdx][layerNum];

  if (children.empty()) {
    layerAssign_node_commit(nullptr, net, 0, bestLayerCombs);
  } else {
    for (auto &child: children) {
      if (child->getType() == frNodeTypeEnum::frcPin) {
        frPoint loc;
        child->getLoc(loc);
        cout << "Error2: should not commit pin node, loc(" << loc.x() / 2000.0 << ", " << loc.y() / 2000.0 << ")\n";
        exit(1);
      }
      layerAssign_node_commit(child, net, comb % cmap_->getNumLayers(), bestLayerCombs);
      comb /= cmap_->getNumLayers();
    }
  }

  // move currNode to its best layer (tech layerNum)
  currNode->setLayerNum((layerNum + 1) * 2);

  // tech layer num, not grid layer num
  set<frLayerNum> nodeLayerNums;
  map<frLayerNum, vector<frNode*> > layerNum2Children;
  // sub nodes are created at same loc as currNode but differnt layerNum
  // since we move from 2d to 3d
  map<frLayerNum, frNode*> layerNum2SubNode;
  
  map<frLayerNum, vector<frNode*> > layerNum2RPinNodes;

  nodeLayerNums.insert(currNode->getLayerNum());
  for (auto &child: children) {
    nodeLayerNums.insert(child->getLayerNum());
    layerNum2Children[child->getLayerNum()].push_back(child);
  }

  bool hasRootNode = false;
  // insert rpin layerNum if exists
  if (net2GCellNode2RPinNodes_[net].find(currNode) != net2GCellNode2RPinNodes_[net].end()) {
    auto &rpinNodes = net2GCellNode2RPinNodes_[net][currNode];
    for (auto &rpinNode: rpinNodes) {
      if (rpinNode->getType() != frNodeTypeEnum::frcPin) {
        cout << "Error: rpinNode is not rpin" << endl;
        exit(1);
      }
      nodeLayerNums.insert(rpinNode->getLayerNum());
      layerNum2RPinNodes[rpinNode->getLayerNum()].push_back(rpinNode);
      if (rpinNode == net->getRoot()) {
        hasRootNode = true;
      }
    }
  }

  frPoint currNodeLoc;
  currNode->getLoc(currNodeLoc);

  for (auto layerNum = *(nodeLayerNums.begin()); layerNum <= *(nodeLayerNums.rbegin()); layerNum += 2) {
    // create node if the layer number is not equal to currNode layerNum
    if (layerNum != currNode->getLayerNum()) {
      auto uNode = make_unique<frNode>();
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

  frLayerNum parentLayer = hasRootNode ? net->getRoot()->getLayerNum() : currNode->getLayerNum();

  for (auto layerNum = *(nodeLayerNums.begin()); layerNum <= *(nodeLayerNums.rbegin()); layerNum += 2) {
    // connect children nodes and sub node (including currNode) (i.e., planar)
    if (layerNum2Children.find(layerNum) != layerNum2Children.end()) {
      for (auto child: layerNum2Children[layerNum]) {
        child->setParent(layerNum2SubNode[layerNum]);
        layerNum2SubNode[layerNum]->addChild(child);
      }
    }
    // connect vertical
    if (layerNum < parentLayer) {
      if (layerNum + 2 > *(nodeLayerNums.rbegin())) {
        cout << "Error: layerNum out of upper bound\n";
        exit(1);
      }
      layerNum2SubNode[layerNum]->setParent(layerNum2SubNode[layerNum + 2]);
      layerNum2SubNode[layerNum + 2]->addChild(layerNum2SubNode[layerNum]);
    } else if (layerNum > parentLayer) {
      if (layerNum - 2 < *(nodeLayerNums.begin())) {
        cout << "Error: layerNum out of lower bound\n";
        exit(1);
      }
      layerNum2SubNode[layerNum]->setParent(layerNum2SubNode[layerNum - 2]);
      layerNum2SubNode[layerNum - 2]->addChild(layerNum2SubNode[layerNum]);
    }
  }

  // update connectivity if there is local rpin node
  for (auto &[layerNum, rpinNodes]: layerNum2RPinNodes) {
    for (auto rpinNode: rpinNodes) {
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

void FlexGR::writeToGuide() {
  for (auto &uNet: design_->getTopBlock()->getNets()) {
    auto net = uNet.get();
    bool hasGRShape = false;
    // pathSeg guide
    for (auto &uShape: net->getGRShapes()) {
      hasGRShape = true;
      if (uShape->typeId() == grcPathSeg) {
        auto pathSeg = static_cast<grPathSeg*>(uShape.get());
        frPoint bp, ep;
        pathSeg->getPoints(bp, ep);
        frLayerNum layerNum;
        layerNum = pathSeg->getLayerNum();
        auto routeGuide = make_unique<frGuide>();
        routeGuide->setPoints(bp, ep);
        routeGuide->setBeginLayerNum(layerNum);
        routeGuide->setEndLayerNum(layerNum);
        routeGuide->addToNet(net);
        net->addGuide(std::move(routeGuide));
      } else {
        cout << "Error: unsupported gr type\n";
      }
    }

    // via guide
    for (auto &uVia: net->getGRVias()) {
      hasGRShape = true;
      auto via = uVia.get();
      frPoint loc;
      via->getOrigin(loc);
      frLayerNum beginLayerNum, endLayerNum;
      beginLayerNum = via->getViaDef()->getLayer1Num();
      endLayerNum = via->getViaDef()->getLayer2Num();

      auto viaGuide = make_unique<frGuide>();
      viaGuide->setPoints(loc, loc);
      viaGuide->setBeginLayerNum(beginLayerNum);
      viaGuide->setEndLayerNum(endLayerNum);
      viaGuide->addToNet(net);
      net->addGuide(std::move(viaGuide));
    }

    // pure local net
    if (!hasGRShape) {
      if (net2GCellNodes_.find(net) == net2GCellNodes_.end() || net2GCellNodes_[net].empty()) {
        continue;
      }

      if (net2GCellNodes_[net].size() > 1) {
        cout << "Error: net " << net->getName() << " spans more than one gcell but not globally routed\n";
        exit(1);
      }

      auto gcellNode = net->getFirstNonRPinNode();
      frPoint loc;
      gcellNode->getLoc(loc);
      frLayerNum minPinLayerNum = INT_MAX;
      frLayerNum maxPinLayerNum = INT_MIN;

      auto &rpinNodes = net2GCellNode2RPinNodes_[net][gcellNode];
      for (auto rpinNode: rpinNodes) {
        frLayerNum layerNum = rpinNode->getLayerNum();
        if (layerNum < minPinLayerNum) {
          minPinLayerNum = layerNum;
        }
        if (layerNum > maxPinLayerNum) {
          maxPinLayerNum = layerNum;
        }
      }

      for (auto layerNum = minPinLayerNum; (layerNum + 2) <= max(minPinLayerNum + 4, maxPinLayerNum) && (layerNum + 2) <= design_->getTech()->getTopLayerNum(); layerNum += 2) {
        auto viaGuide = make_unique<frGuide>();
        viaGuide->setPoints(loc, loc);
        viaGuide->setBeginLayerNum(layerNum);
        viaGuide->setEndLayerNum(layerNum + 2);
        viaGuide->addToNet(net);
        net->addGuide(std::move(viaGuide));
      }
    }
  }
}

void FlexGR::writeGuideFile() {
  if (OUTGUIDE_FILE == string("")) {
    OUTGUIDE_FILE = string("./route.guide");
  }
  ofstream outputGuide(OUTGUIDE_FILE.c_str());
  if (outputGuide.is_open()) {
    for (auto &net: design_->getTopBlock()->getNets()) {
      auto netName = net->getName();
      outputGuide << netName << endl;
      outputGuide << "(\n"; 
      for (auto &guide: net->getGuides()) {
        frPoint bp, ep;
        guide->getPoints(bp, ep);
        frPoint bpIdx, epIdx;
        design_->getTopBlock()->getGCellIdx(bp, bpIdx);
        design_->getTopBlock()->getGCellIdx(ep, epIdx);
        frBox bbox, ebox;
        design_->getTopBlock()->getGCellBox(bpIdx, bbox);
        design_->getTopBlock()->getGCellBox(epIdx, ebox);
        frLayerNum bNum = guide->getBeginLayerNum();
        frLayerNum eNum = guide->getEndLayerNum();
        // append unit guide in case of stacked via
        if (bNum != eNum) {
          for (auto lNum = min(bNum, eNum); lNum <= max(bNum, eNum); lNum += 2) {
            auto layerName = design_->getTech()->getLayer(lNum)->getName();
            outputGuide << bbox.left()  << " " << bbox.bottom() << " "
                        << bbox.right() << " " << bbox.top()    << " "
                        << layerName << endl;
          }
        } else {
          auto layerName = design_->getTech()->getLayer(bNum)->getName();
          outputGuide << bbox.left()  << " " << bbox.bottom() << " "
                      << ebox.right() << " " << ebox.top()    << " "
                      << layerName << endl;
        }
      }
      outputGuide << ")\n";
    }
  }
}

void FlexGR::getBatchInfo(int &batchStepX, int &batchStepY) {
  batchStepX = 2;
  batchStepY = 2;
}


// GRWorker related
void FlexGRWorker::main_mt() {
  bool enableOutput = false;

  if (enableOutput) {
    stringstream ss;
    ss <<endl <<"start GR worker (BOX) "
                <<"( " <<extBox_.left()   * 1.0 / getTech()->getDBUPerUU() <<" "
                <<extBox_.bottom() * 1.0 / getTech()->getDBUPerUU() <<" ) ( "
                <<extBox_.right()  * 1.0 / getTech()->getDBUPerUU() <<" "
                <<extBox_.top()    * 1.0 / getTech()->getDBUPerUU() <<" )" <<endl;
    cout <<ss.str() <<flush;
  }

  init();
  route();
}
