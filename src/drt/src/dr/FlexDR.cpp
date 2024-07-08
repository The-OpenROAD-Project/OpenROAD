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

#include "dr/FlexDR.h"

#include <dst/JobMessage.h>
#include <omp.h>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/io/ios_state.hpp>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>

#include "db/infra/frTime.h"
#include "distributed/RoutingJobDescription.h"
#include "distributed/frArchive.h"
#include "dr/FlexDR_conn.h"
#include "dr/FlexDR_graphics.h"
#include "dst/BalancerJobDescription.h"
#include "dst/Distributed.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
#include "io/io.h"
#include "ord/OpenRoad.hh"
#include "serialization.h"
#include "utl/exception.h"

BOOST_CLASS_EXPORT(drt::RoutingJobDescription)

namespace drt {

using utl::ThreadException;

enum class SerializationType
{
  READ,
  WRITE
};

void serializeWorker(FlexDRWorker* worker, std::string& workerStr)
{
  std::stringstream stream(std::ios_base::binary | std::ios_base::in
                           | std::ios_base::out);
  frOArchive ar(stream);
  registerTypes(ar);
  ar << *worker;
  workerStr = stream.str();
}

void deserializeWorker(FlexDRWorker* worker,
                       frDesign* design,
                       const std::string& workerStr)
{
  std::stringstream stream(
      workerStr,
      std::ios_base::binary | std::ios_base::in | std::ios_base::out);
  frIArchive ar(stream);
  ar.setDesign(design);
  registerTypes(ar);
  ar >> *worker;
}

void serializeViaData(const FlexDRViaData& viaData, std::string& serializedStr)
{
  std::stringstream stream(std::ios_base::binary | std::ios_base::in
                           | std::ios_base::out);
  frOArchive ar(stream);
  registerTypes(ar);
  ar << viaData;
  serializedStr = stream.str();
}

FlexDR::FlexDR(TritonRoute* router,
               frDesign* designIn,
               Logger* loggerIn,
               odb::dbDatabase* dbIn)
    : router_(router),
      design_(designIn),
      logger_(loggerIn),
      db_(dbIn),
      numWorkUnits_(0),
      dist_(nullptr),
      dist_on_(false),
      dist_port_(0),
      increaseClipsize_(false),
      clipSizeInc_(0),
      iter_(0)
{
}

FlexDR::~FlexDR() = default;

void FlexDR::setDebug(frDebugSettings* settings)
{
  bool on = settings->debugDR;
  graphics_
      = on && FlexDRGraphics::guiActive()
            ? std::make_unique<FlexDRGraphics>(settings, design_, db_, logger_)
            : nullptr;
}

std::string FlexDRWorker::reloadedMain()
{
  init(design_);
  debugPrint(logger_,
             utl::DRT,
             "autotuner",
             1,
             "Init number of markers {}",
             getInitNumMarkers());
  if (!skipRouting_) {
    route_queue();
  }
  setGCWorker(nullptr);
  cleanup();
  std::string workerStr;
  serializeWorker(this, workerStr);
  return workerStr;
}

void FlexDRWorker::writeUpdates(const std::string& file_name)
{
  std::vector<std::vector<drUpdate>> updates(1);
  for (const auto& net : getDesign()->getTopBlock()->getNets()) {
    for (const auto& guide : net->getGuides()) {
      for (const auto& connFig : guide->getRoutes()) {
        drUpdate update(drUpdate::ADD_GUIDE);
        frPathSeg* pathSeg = static_cast<frPathSeg*>(connFig.get());
        update.setPathSeg(*pathSeg);
        update.setIndexInOwner(guide->getIndexInOwner());
        update.setNet(net.get());
        updates.back().push_back(update);
      }
    }
    for (const auto& shape : net->getShapes()) {
      drUpdate update;
      auto pathSeg = static_cast<frPathSeg*>(shape.get());
      update.setPathSeg(*pathSeg);
      update.setNet(net.get());
      update.setUpdateType(drUpdate::ADD_SHAPE);
      updates.back().push_back(update);
    }
    for (const auto& shape : net->getVias()) {
      drUpdate update;
      auto via = static_cast<frVia*>(shape.get());
      update.setVia(*via);
      update.setNet(net.get());
      update.setUpdateType(drUpdate::ADD_SHAPE);
      updates.back().push_back(update);
    }
    for (const auto& shape : net->getPatchWires()) {
      drUpdate update;
      auto patch = static_cast<frPatchWire*>(shape.get());
      update.setPatchWire(*patch);
      update.setNet(net.get());
      update.setUpdateType(drUpdate::ADD_SHAPE);
      updates.back().push_back(update);
    }
  }
  for (const auto& marker : getDesign()->getTopBlock()->getMarkers()) {
    drUpdate update;
    update.setMarker(*(marker.get()));
    update.setUpdateType(drUpdate::ADD_SHAPE);
    updates.back().push_back(update);
  }
  std::ofstream file(file_name.c_str());
  frOArchive ar(file);
  registerTypes(ar);
  ar << updates;
  file.close();
}

int FlexDRWorker::main(frDesign* design)
{
  ProfileTask profile("DRW:main");
  using std::chrono::high_resolution_clock;
  high_resolution_clock::time_point t0 = high_resolution_clock::now();
  auto micronPerDBU = 1.0 / getTech()->getDBUPerUU();
  if (VERBOSE > 1) {
    logger_->report("start DR worker (BOX) ( {} {} ) ( {} {} )",
                    routeBox_.xMin() * micronPerDBU,
                    routeBox_.yMin() * micronPerDBU,
                    routeBox_.xMax() * micronPerDBU,
                    routeBox_.yMax() * micronPerDBU);
  }
  initMarkers(design);
  if (getDRIter() && getInitNumMarkers() == 0 && !needRecheck_) {
    skipRouting_ = true;
  }
  if (debugSettings_->debugDumpDR
      && (debugSettings_->box == Rect(-1, -1, -1, -1)
          || routeBox_.intersects(debugSettings_->box))
      && !skipRouting_
      && (debugSettings_->iter == getDRIter()
          || debugSettings_->dumpLastWorker)) {
    std::string workerPath = fmt::format("{}/workerx{}_y{}",
                                         debugSettings_->dumpDir,
                                         routeBox_.xMin(),
                                         routeBox_.yMin());
    if (debugSettings_->dumpLastWorker) {
      workerPath = fmt::format("{}/workerx{}_y{}",
                               debugSettings_->dumpDir,
                               debugSettings_->box.xMin(),
                               debugSettings_->box.yMin());
    }
    mkdir(workerPath.c_str(), 0777);

    writeUpdates(fmt::format("{}/updates.bin", workerPath));
    {
      std::string viaDataStr;
      serializeViaData(*via_data_, viaDataStr);
      std::ofstream viaDataFile(
          fmt::format("{}/viadata.bin", workerPath).c_str());
      viaDataFile << viaDataStr;
      viaDataFile.close();
    }
    {
      std::string workerStr;
      serializeWorker(this, workerStr);
      std::ofstream workerFile(
          fmt::format("{}/worker.bin", workerPath).c_str());
      workerFile << workerStr;
      workerFile.close();
    }
    {
      std::ofstream globalsFile(
          fmt::format("{}/worker_globals.bin", workerPath).c_str());
      frOArchive ar(globalsFile);
      registerTypes(ar);
      serializeGlobals(ar);
      globalsFile.close();
    }
  }
  if (!skipRouting_) {
    init(design);
  }
  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  if (!skipRouting_) {
    route_queue();
  }
  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  const int num_markers = getNumMarkers();
  cleanup();
  high_resolution_clock::time_point t3 = high_resolution_clock::now();

  using std::chrono::duration;
  using std::chrono::duration_cast;
  duration<double> time_span0 = duration_cast<duration<double>>(t1 - t0);
  duration<double> time_span1 = duration_cast<duration<double>>(t2 - t1);
  duration<double> time_span2 = duration_cast<duration<double>>(t3 - t2);

  if (VERBOSE > 1) {
    std::stringstream ss;
    ss << "time (INIT/ROUTE/POST) " << time_span0.count() << " "
       << time_span1.count() << " " << time_span2.count() << " " << std::endl;
    std::cout << ss.str() << std::flush;
  }

  debugPrint(logger_,
             DRT,
             "autotuner",
             1,
             "worker ({:.3f} {:.3f}) ({:.3f} {:.3f}) time {} prev_#DRVs {} "
             "curr_#DRVs {}",
             routeBox_.xMin() * micronPerDBU,
             routeBox_.yMin() * micronPerDBU,
             routeBox_.xMax() * micronPerDBU,
             routeBox_.yMax() * micronPerDBU,
             duration_cast<duration<double>>(t3 - t0).count(),
             getInitNumMarkers(),
             num_markers);

  return 0;
}

void FlexDRWorker::distributedMain(frDesign* design)
{
  ProfileTask profile("DR:main");
  if (VERBOSE > 1) {
    logger_->report("start DR worker (BOX) ( {} {} ) ( {} {} )",
                    routeBox_.xMin() * 1.0 / getTech()->getDBUPerUU(),
                    routeBox_.yMin() * 1.0 / getTech()->getDBUPerUU(),
                    routeBox_.xMax() * 1.0 / getTech()->getDBUPerUU(),
                    routeBox_.yMax() * 1.0 / getTech()->getDBUPerUU());
  }
  initMarkers(design);
  if (getDRIter() && getInitNumMarkers() == 0 && !needRecheck_) {
    skipRouting_ = true;
    return;
  }
}

void FlexDR::initFromTA()
{
  // initialize lists
  for (auto& net : getDesign()->getTopBlock()->getNets()) {
    for (auto& guide : net->getGuides()) {
      for (auto& connFig : guide->getRoutes()) {
        if (connFig->typeId() == frcPathSeg) {
          std::unique_ptr<frShape> ps = std::make_unique<frPathSeg>(
              *(static_cast<frPathSeg*>(connFig.get())));
          auto [bp, ep] = static_cast<frPathSeg*>(ps.get())->getPoints();

          // skip TA dummy segment
          if (Point::manhattanDistance(ep, bp) != 1) {
            net->addShape(std::move(ps));
          }
        } else {
          std::cout << "Error: initFromTA unsupported shape" << std::endl;
        }
      }
    }
  }
}

void FlexDR::initGCell2BoundaryPin()
{
  // initialize size
  auto topBlock = getDesign()->getTopBlock();
  auto gCellPatterns = topBlock->getGCellPatterns();
  auto& xgp = gCellPatterns.at(0);
  auto& ygp = gCellPatterns.at(1);
  auto tmpVec = std::vector<std::map<frNet*,
                                     std::set<std::pair<Point, frLayerNum>>,
                                     frBlockObjectComp>>((int) ygp.getCount());
  gcell2BoundaryPin_
      = std::vector<std::vector<std::map<frNet*,
                                         std::set<std::pair<Point, frLayerNum>>,
                                         frBlockObjectComp>>>(
          (int) xgp.getCount(), tmpVec);
  for (auto& net : topBlock->getNets()) {
    auto netPtr = net.get();
    for (auto& guide : net->getGuides()) {
      for (auto& connFig : guide->getRoutes()) {
        if (connFig->typeId() == frcPathSeg) {
          auto ps = static_cast<frPathSeg*>(connFig.get());
          auto [bp, ep] = ps->getPoints();
          frLayerNum layerNum = ps->getLayerNum();
          // skip TA dummy segment
          auto mdist = Point::manhattanDistance(ep, bp);
          if (mdist == 1 || mdist == 0) {
            continue;
          }
          Point idx1 = design_->getTopBlock()->getGCellIdx(bp);
          Point idx2 = design_->getTopBlock()->getGCellIdx(ep);

          // update gcell2BoundaryPin
          // horizontal
          if (bp.y() == ep.y()) {
            int x1 = idx1.x();
            int x2 = idx2.x();
            int y = idx1.y();
            for (auto x = x1; x <= x2; ++x) {
              Rect gcellBox = topBlock->getGCellBox(Point(x, y));
              frCoord leftBound = gcellBox.xMin();
              frCoord rightBound = gcellBox.xMax();
              const bool hasLeftBound = bp.x() < leftBound;
              const bool hasRightBound = ep.x() >= rightBound;
              if (hasLeftBound) {
                Point boundaryPt(leftBound, bp.y());
                gcell2BoundaryPin_[x][y][netPtr].emplace(boundaryPt, layerNum);
              }
              if (hasRightBound) {
                Point boundaryPt(rightBound, ep.y());
                gcell2BoundaryPin_[x][y][netPtr].emplace(boundaryPt, layerNum);
              }
            }
          } else if (bp.x() == ep.x()) {
            int x = idx1.x();
            int y1 = idx1.y();
            int y2 = idx2.y();
            for (auto y = y1; y <= y2; ++y) {
              Rect gcellBox = topBlock->getGCellBox(Point(x, y));
              frCoord bottomBound = gcellBox.yMin();
              frCoord topBound = gcellBox.yMax();
              const bool hasBottomBound = bp.y() < bottomBound;
              const bool hasTopBound = ep.y() >= topBound;
              if (hasBottomBound) {
                Point boundaryPt(bp.x(), bottomBound);
                gcell2BoundaryPin_[x][y][netPtr].emplace(boundaryPt, layerNum);
              }
              if (hasTopBound) {
                Point boundaryPt(ep.x(), topBound);
                gcell2BoundaryPin_[x][y][netPtr].emplace(boundaryPt, layerNum);
              }
            }
          } else {
            std::cout
                << "Error: non-orthogonal pathseg in initGCell2BoundryPin\n";
          }
        }
      }
    }
  }
}

void FlexDR::init_halfViaEncArea()
{
  auto bottomLayerNum = getTech()->getBottomLayerNum();
  auto topLayerNum = getTech()->getTopLayerNum();
  auto& halfViaEncArea = via_data_.halfViaEncArea;
  for (int i = bottomLayerNum; i <= topLayerNum; i++) {
    if (getTech()->getLayer(i)->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    if (i + 1 <= topLayerNum
        && getTech()->getLayer(i + 1)->getType() == dbTechLayerType::CUT) {
      auto viaDef = getTech()->getLayer(i + 1)->getDefaultViaDef();
      if (viaDef) {
        frVia via(viaDef);
        Rect layer1Box = via.getLayer1BBox();
        Rect layer2Box = via.getLayer2BBox();
        auto layer1HalfArea = layer1Box.area() / 2;
        auto layer2HalfArea = layer2Box.area() / 2;
        halfViaEncArea.emplace_back(layer1HalfArea, layer2HalfArea);
      } else {
        halfViaEncArea.emplace_back(0, 0);
      }
    } else {
      halfViaEncArea.emplace_back(0, 0);
    }
  }
}

void FlexDR::init()
{
  ProfileTask profile("DR:init");
  frTime t;
  if (VERBOSE > 0) {
    logger_->info(DRT, 187, "Start routing data preparation.");
  }
  initGCell2BoundaryPin();
  getRegionQuery()->initDRObj();  // first init in postProcess

  init_halfViaEncArea();

  if (VERBOSE > 0) {
    t.print(logger_);
  }

  iter_ = 0;

  if (VERBOSE > 0) {
    logger_->info(DRT, 194, "Start detail routing.");
  }
  for (const auto& net : getDesign()->getTopBlock()->getNets()) {
    if (net->hasInitialRouting()) {
      for (const auto& via : net->getVias()) {
        auto bottomBox = via->getLayer1BBox();
        auto topBox = via->getLayer2BBox();
        for (auto term : net->getInstTerms()) {
          if (term->getBBox(true).intersects(bottomBox)) {
            std::vector<frRect> shapes;
            term->getShapes(shapes, true);
            for (auto shape : shapes) {
              if (shape.getLayerNum() != via->getViaDef()->getLayer1Num()) {
                continue;
              }
              if (!shape.intersects(bottomBox)) {
                continue;
              }
              via->setBottomConnected(true);
              break;
            }
          }
          if (via->isBottomConnected()) {
            continue;
          }
          if (term->getBBox(true).intersects(topBox)) {
            std::vector<frRect> shapes;
            term->getShapes(shapes, true);
            for (auto shape : shapes) {
              if (shape.getLayerNum() != via->getViaDef()->getLayer2Num()) {
                continue;
              }
              if (!shape.intersects(topBox)) {
                continue;
              }
              via->setTopConnected(true);
              break;
            }
          }
        }
      }
    }
  }
}

void FlexDR::removeGCell2BoundaryPin()
{
  gcell2BoundaryPin_.clear();
  gcell2BoundaryPin_.shrink_to_fit();
}

std::map<frNet*, std::set<std::pair<Point, frLayerNum>>, frBlockObjectComp>
FlexDR::initDR_mergeBoundaryPin(int startX,
                                int startY,
                                int size,
                                const Rect& routeBox)
{
  std::map<frNet*, std::set<std::pair<Point, frLayerNum>>, frBlockObjectComp>
      bp;
  auto gCellPatterns = getDesign()->getTopBlock()->getGCellPatterns();
  auto& xgp = gCellPatterns.at(0);
  auto& ygp = gCellPatterns.at(1);
  for (int i = startX; i < (int) xgp.getCount() && i < startX + size; i++) {
    for (int j = startY; j < (int) ygp.getCount() && j < startY + size; j++) {
      auto& currBp = gcell2BoundaryPin_[i][j];
      for (auto& [net, s] : currBp) {
        for (auto& [pt, lNum] : s) {
          if (pt.x() == routeBox.xMin() || pt.x() == routeBox.xMax()
              || pt.y() == routeBox.yMin() || pt.y() == routeBox.yMax()) {
            bp[net].emplace(pt, lNum);
          }
        }
      }
    }
  }
  return bp;
}

void FlexDR::getBatchInfo(int& batchStepX, int& batchStepY)
{
  batchStepX = 2;
  batchStepY = 2;
}

void FlexDR::searchRepair(const SearchRepairArgs& args)
{
  const int iter = iter_++;
  const int size = args.size;
  const int offset = args.offset;
  const int mazeEndIter = args.mazeEndIter;
  const frUInt4 workerDRCCost = args.workerDRCCost;
  const frUInt4 workerMarkerCost = args.workerMarkerCost;
  const frUInt4 workerFixedShapeCost = args.workerFixedShapeCost;
  const float workerMarkerDecay = args.workerMarkerDecay;
  const RipUpMode ripupMode = args.ripupMode;
  const bool followGuide = args.followGuide;

  std::string profile_name("DR:searchRepair");
  profile_name += std::to_string(iter);
  ProfileTask profile(profile_name.c_str());
  if ((ripupMode == RipUpMode::DRC || ripupMode == RipUpMode::NEARDRC)
      && getDesign()->getTopBlock()->getMarkers().empty()) {
    return;
  }
  if (dist_on_) {
    if ((iter % 10 == 0 && iter != 60) || iter == 3 || iter == 15) {
      globals_path_ = fmt::format("{}globals.{}.ar", dist_dir_, iter);
      router_->writeGlobals(globals_path_);
    }
  }
  frTime t;
  if (VERBOSE > 0) {
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
    logger_->info(DRT, 195, "Start {}{} optimization iteration.", iter, suffix);
  }
  if (graphics_) {
    graphics_->startIter(iter);
  }
  auto gCellPatterns = getDesign()->getTopBlock()->getGCellPatterns();
  auto& xgp = gCellPatterns.at(0);
  auto& ygp = gCellPatterns.at(1);
  int cnt = 0;
  int tot = (((int) xgp.getCount() - 1 - offset) / size + 1)
            * (((int) ygp.getCount() - 1 - offset) / size + 1);
  int prev_perc = 0;
  bool isExceed = false;

  std::vector<std::unique_ptr<FlexDRWorker>> uworkers;
  int batchStepX, batchStepY;

  getBatchInfo(batchStepX, batchStepY);

  std::vector<std::vector<std::vector<std::unique_ptr<FlexDRWorker>>>> workers(
      batchStepX * batchStepY);

  int xIdx = 0, yIdx = 0;
  for (int i = offset; i < (int) xgp.getCount(); i += size) {
    for (int j = offset; j < (int) ygp.getCount(); j += size) {
      auto worker
          = std::make_unique<FlexDRWorker>(&via_data_, design_, logger_);
      Rect routeBox1 = getDesign()->getTopBlock()->getGCellBox(Point(i, j));
      const int max_i = std::min((int) xgp.getCount() - 1, i + size - 1);
      const int max_j = std::min((int) ygp.getCount(), j + size - 1);
      Rect routeBox2
          = getDesign()->getTopBlock()->getGCellBox(Point(max_i, max_j));
      Rect routeBox(routeBox1.xMin(),
                    routeBox1.yMin(),
                    routeBox2.xMax(),
                    routeBox2.yMax());
      Rect extBox;
      Rect drcBox;
      routeBox.bloat(MTSAFEDIST, extBox);
      routeBox.bloat(DRCSAFEDIST, drcBox);
      worker->setRouteBox(routeBox);
      worker->setExtBox(extBox);
      worker->setDrcBox(drcBox);
      worker->setGCellBox(Rect(i, j, max_i, max_j));
      worker->setMazeEndIter(mazeEndIter);
      worker->setDRIter(iter);
      worker->setDebugSettings(router_->getDebugSettings());
      if (dist_on_) {
        worker->setDistributed(dist_, dist_ip_, dist_port_, dist_dir_);
      }
      if (!iter) {
        // if (routeBox.xMin() == 441000 && routeBox.yMin() == 816100) {
        //   std::cout << "@@@ debug: " << i << " " << j << std::endl;
        // }
        // set boundary pin
        auto bp = initDR_mergeBoundaryPin(i, j, size, routeBox);
        worker->setDRIter(0, bp);
      }
      worker->setRipupMode(ripupMode);
      worker->setFollowGuide(followGuide);
      // TODO: only pass to relevant workers
      worker->setGraphics(graphics_.get());
      worker->setCost(workerDRCCost,
                      workerMarkerCost,
                      workerFixedShapeCost,
                      workerMarkerDecay);

      int batchIdx = (xIdx % batchStepX) * batchStepY + yIdx % batchStepY;
      if (workers[batchIdx].empty()
          || (!dist_on_
              && (int) workers[batchIdx].back().size() >= BATCHSIZE)) {
        workers[batchIdx].push_back(
            std::vector<std::unique_ptr<FlexDRWorker>>());
      }
      workers[batchIdx].back().push_back(std::move(worker));

      yIdx++;
    }
    yIdx = 0;
    xIdx++;
  }

  omp_set_num_threads(MAX_THREADS);
  int version = 0;
  increaseClipsize_ = false;
  numWorkUnits_ = 0;
  // parallel execution
  for (auto& workerBatch : workers) {
    ProfileTask profile("DR:checkerboard");
    for (auto& workersInBatch : workerBatch) {
      {
        const std::string batch_name = std::string("DR:batch<")
                                       + std::to_string(workersInBatch.size())
                                       + ">";
        ProfileTask profile(batch_name.c_str());
        if (dist_on_) {
          router_->dist_pool_.join();
          if (version++ == 0 && !design_->hasUpdates()) {
            std::string serializedViaData;
            serializeViaData(via_data_, serializedViaData);
            router_->sendGlobalsUpdates(globals_path_, serializedViaData);
          } else {
            router_->sendDesignUpdates(globals_path_);
          }
        }
        {
          ProfileTask task("DIST: PROCESS_BATCH");
          // multi thread
          ThreadException exception;
#pragma omp parallel for schedule(dynamic)
          for (int i = 0; i < (int) workersInBatch.size(); i++) {  // NOLINT
            try {
              if (dist_on_) {
                workersInBatch[i]->distributedMain(getDesign());
              } else {
                workersInBatch[i]->main(getDesign());
              }
#pragma omp critical
              {
                cnt++;
                if (VERBOSE > 0) {
                  if (cnt * 1.0 / tot >= prev_perc / 100.0 + 0.1
                      && prev_perc < 90) {
                    if (prev_perc == 0 && t.isExceed(0)) {
                      isExceed = true;
                    }
                    prev_perc += 10;
                    if (isExceed) {
                      logger_->report(
                          "    Completing {}% with {} violations.",
                          prev_perc,
                          getDesign()->getTopBlock()->getNumMarkers());
                      logger_->report("    {}.", t);
                    }
                  }
                }
              }
            } catch (...) {
              exception.capture();
            }
          }
          exception.rethrow();
          if (dist_on_) {
            int j = 0;
            std::vector<std::vector<std::pair<int, FlexDRWorker*>>>
                distWorkerBatches(router_->getCloudSize());
            for (int i = 0; i < workersInBatch.size(); i++) {
              auto worker = workersInBatch.at(i).get();
              if (!worker->isSkipRouting()) {
                distWorkerBatches[j].push_back({i, worker});
                j = (j + 1) % router_->getCloudSize();
              }
            }
            {
              ProfileTask task("DIST: SERIALIZE+SEND");
#pragma omp parallel for schedule(dynamic)
              for (int i = 0; i < distWorkerBatches.size(); i++)  // NOLINT
                sendWorkers(distWorkerBatches.at(i), workersInBatch);
            }
            logger_->report("    Received Batches:{}.", t);
            std::vector<std::pair<int, std::string>> workers;
            router_->getWorkerResults(workers);
            {
              ProfileTask task("DIST: DESERIALIZING_BATCH");
#pragma omp parallel for schedule(dynamic)
              for (int i = 0; i < workers.size(); i++) {  // NOLINT
                deserializeWorker(workersInBatch.at(workers.at(i).first).get(),
                                  design_,
                                  workers.at(i).second);
              }
            }
            logger_->report("    Deserialized Batches:{}.", t);
          }
        }
      }
      {
        ProfileTask profile("DR:end_batch");
        // single thread
        for (auto& worker : workersInBatch) {
          if (worker->end(getDesign())) {
            numWorkUnits_ += 1;
          }
          if (worker->isCongested()) {
            increaseClipsize_ = true;
          }
        }
        workersInBatch.clear();
      }
    }
  }

  if (!iter) {
    removeGCell2BoundaryPin();
  }
  if (VERBOSE > 0) {
    if (cnt * 1.0 / tot >= prev_perc / 100.0 + 0.1 && prev_perc >= 90) {
      if (prev_perc == 0 && t.isExceed(0)) {
        isExceed = true;
      }
      prev_perc += 10;
      if (isExceed) {
        logger_->report("    Completing {}% with {} violations.",
                        prev_perc,
                        getDesign()->getTopBlock()->getNumMarkers());
        logger_->report("    {}.", t);
      }
    }
  }
  FlexDRConnectivityChecker checker(
      router_, logger_, graphics_.get(), dist_on_);
  checker.check(iter);
  numViols_.push_back(getDesign()->getTopBlock()->getNumMarkers());
  debugPrint(logger_,
             utl::DRT,
             "workers",
             1,
             "Number of work units = {}.",
             numWorkUnits_);
  if (VERBOSE > 0) {
    logger_->info(DRT,
                  199,
                  "  Number of violations = {}.",
                  getDesign()->getTopBlock()->getNumMarkers());
    if (getDesign()->getTopBlock()->getNumMarkers() > 0) {
      // report violations
      std::map<std::string, std::map<frLayerNum, uint>> violations;
      std::set<frLayerNum> layers;
      const std::map<std::string, std::string> relabel
          = {{"Lef58SpacingEndOfLine", "EOL"},
             {"Lef58CutSpacingTable", "CutSpcTbl"},
             {"Lef58EolKeepOut", "eolKeepOut"}};
      for (const auto& marker : getDesign()->getTopBlock()->getMarkers()) {
        if (!marker->getConstraint()) {
          continue;
        }
        auto type = marker->getConstraint()->getViolName();
        if (relabel.find(type) != relabel.end()) {
          type = relabel.at(type);
        }
        violations[type][marker->getLayerNum()]++;
        layers.insert(marker->getLayerNum());
      }
      std::string line = fmt::format("{:<15}", "Viol/Layer");
      for (auto lNum : layers) {
        std::string lName = getTech()->getLayer(lNum)->getName();
        if (lName.size() >= 7) {
          lName = lName.substr(0, 2) + ".." + lName.substr(lName.size() - 2, 2);
        }
        line += fmt::format("{:>7}", lName);
      }
      logger_->report(line);
      for (auto& [type, typeViolations] : violations) {
        std::string typeName = type;
        if (typeName.size() >= 15) {
          typeName = typeName.substr(0, 12) + "..";
        }
        line = fmt::format("{:<15}", typeName);
        for (auto lNum : layers) {
          line += fmt::format("{:>7}", typeViolations[lNum]);
        }
        logger_->report(line);
      }
    }
    t.print(logger_);
    std::cout << std::flush;
  }
  end();
  if ((DRC_RPT_ITER_STEP && iter > 0 && iter % DRC_RPT_ITER_STEP.value() == 0)
      || logger_->debugCheck(DRT, "autotuner", 1)
      || logger_->debugCheck(DRT, "report", 1)) {
    router_->reportDRC(DRC_RPT_FILE + '-' + std::to_string(iter) + ".rpt",
                       design_->getTopBlock()->getMarkers());
  }
}

void FlexDR::end(bool done)
{
  if (done && DRC_RPT_FILE != std::string("")) {
    router_->reportDRC(DRC_RPT_FILE, design_->getTopBlock()->getMarkers());
  }
  if (done && VERBOSE > 0) {
    logger_->info(DRT, 198, "Complete detail routing.");
  }

  using ULL = uint64_t;
  const auto size = getTech()->getLayers().size();
  std::vector<ULL> wlen(size, 0);
  std::vector<ULL> sCut(size, 0);
  std::vector<ULL> mCut(size, 0);
  auto topBlock = getDesign()->getTopBlock();
  for (auto& net : topBlock->getNets()) {
    for (auto& shape : net->getShapes()) {
      if (shape->typeId() == frcPathSeg) {
        auto obj = static_cast<frPathSeg*>(shape.get());
        auto [bp, ep] = obj->getPoints();
        auto lNum = obj->getLayerNum();
        frCoord psLen = Point::manhattanDistance(ep, bp);
        wlen[lNum] += psLen;
      }
    }
    for (auto& via : net->getVias()) {
      auto lNum = via->getViaDef()->getCutLayerNum();
      if (via->getViaDef()->isMultiCut()) {
        ++mCut[lNum];
      } else {
        ++sCut[lNum];
      }
    }
  }

  const ULL totWlen = std::accumulate(wlen.begin(), wlen.end(), ULL(0));
  const ULL totSCut = std::accumulate(sCut.begin(), sCut.end(), ULL(0));
  const ULL totMCut = std::accumulate(mCut.begin(), mCut.end(), ULL(0));

  if (done) {
    logger_->metric("route__drc_errors", topBlock->getNumMarkers());
    logger_->metric("route__wirelength", totWlen / topBlock->getDBUPerUU());
    logger_->metric("route__vias", totSCut + totMCut);
    logger_->metric("route__vias__singlecut", totSCut);
    logger_->metric("route__vias__multicut", totMCut);
  } else {
    logger_->metric(fmt::format("route__drc_errors__iter:{}", iter_),
                    topBlock->getNumMarkers());
    logger_->metric(fmt::format("route__wirelength__iter:{}", iter_),
                    totWlen / topBlock->getDBUPerUU());
  }

  if (VERBOSE > 0) {
    logger_->report("Total wire length = {} um.",
                    totWlen / topBlock->getDBUPerUU());

    for (int i = getTech()->getBottomLayerNum();
         i <= getTech()->getTopLayerNum();
         i++) {
      if (getTech()->getLayer(i)->getType() == dbTechLayerType::ROUTING) {
        logger_->report("Total wire length on LAYER {} = {} um.",
                        getTech()->getLayer(i)->getName(),
                        wlen[i] / topBlock->getDBUPerUU());
      }
    }
    logger_->report("Total number of vias = {}.", totSCut + totMCut);
    if (totMCut > 0) {
      logger_->report("Total number of multi-cut vias = {} ({:5.1f}%).",
                      totMCut,
                      totMCut * 100.0 / (totSCut + totMCut));
      logger_->report("Total number of single-cut vias = {} ({:5.1f}%).",
                      totSCut,
                      totSCut * 100.0 / (totSCut + totMCut));
    }
    logger_->report("Up-via summary (total {}):", totSCut + totMCut);
    int nameLen = 0;
    for (int i = getTech()->getBottomLayerNum();
         i <= getTech()->getTopLayerNum();
         i++) {
      if (getTech()->getLayer(i)->getType() == dbTechLayerType::CUT) {
        nameLen = std::max(nameLen,
                           (int) getTech()->getLayer(i - 1)->getName().size());
      }
    }
    int maxL = 1 + nameLen + 4 + (int) std::to_string(totSCut).length();
    if (totMCut) {
      maxL += 9 + 4 + (int) std::to_string(totMCut).length() + 9 + 4
              + (int) std::to_string(totSCut + totMCut).length();
    }
    std::ostringstream msg;
    if (totMCut) {
      msg << " "
          << std::setw(nameLen + 4 + (int) std::to_string(totSCut).length() + 9)
          << "single-cut";
      msg << std::setw(4 + (int) std::to_string(totMCut).length() + 9)
          << "multi-cut"
          << std::setw(4 + (int) std::to_string(totSCut + totMCut).length())
          << "total";
    }
    msg << std::endl;
    for (int i = 0; i < maxL; i++) {
      msg << "-";
    }
    msg << std::endl;
    for (int i = getTech()->getBottomLayerNum();
         i <= getTech()->getTopLayerNum();
         i++) {
      if (getTech()->getLayer(i)->getType() == dbTechLayerType::CUT) {
        msg << " " << std::setw(nameLen)
            << getTech()->getLayer(i - 1)->getName() << "    "
            << std::setw((int) std::to_string(totSCut).length()) << sCut[i];
        if (totMCut) {
          msg << " (" << std::setw(5)
              << (double) ((sCut[i] + mCut[i])
                               ? sCut[i] * 100.0 / (sCut[i] + mCut[i])
                               : 0.0)
              << "%)";
          msg << "    " << std::setw((int) std::to_string(totMCut).length())
              << mCut[i] << " (" << std::setw(5)
              << (double) ((sCut[i] + mCut[i])
                               ? mCut[i] * 100.0 / (sCut[i] + mCut[i])
                               : 0.0)
              << "%)"
              << "    "
              << std::setw((int) std::to_string(totSCut + totMCut).length())
              << sCut[i] + mCut[i];
        }
        msg << std::endl;
      }
    }
    for (int i = 0; i < maxL; i++) {
      msg << "-";
    }
    msg << std::endl;
    msg << " " << std::setw(nameLen) << ""
        << "    " << std::setw((int) std::to_string(totSCut).length())
        << totSCut;
    if (totMCut) {
      msg << " (" << std::setw(5)
          << (double) ((totSCut + totMCut)
                           ? totSCut * 100.0 / (totSCut + totMCut)
                           : 0.0)
          << "%)";
      msg << "    " << std::setw((int) std::to_string(totMCut).length())
          << totMCut << " (" << std::setw(5)
          << (double) ((totSCut + totMCut)
                           ? totMCut * 100.0 / (totSCut + totMCut)
                           : 0.0)
          << "%)"
          << "    "
          << std::setw((int) std::to_string(totSCut + totMCut).length())
          << totSCut + totMCut;
    }
    msg << std::endl << std::endl;
    logger_->report("{}", msg.str());
  }
}

static std::vector<FlexDR::SearchRepairArgs> strategy()
{
  const frUInt4 shapeCost = ROUTESHAPECOST;

  // clang-format off
  return {
    {7,  0,  3,      shapeCost,               0,       shapeCost, 0.950, RipUpMode::ALL    ,  true}, //  0
    {7, -2,  3,      shapeCost,       shapeCost,       shapeCost, 0.950, RipUpMode::ALL    ,  true}, //  1
    {7, -5,  3,      shapeCost,       shapeCost,       shapeCost, 0.950, RipUpMode::ALL    ,  true}, //  2
    {7,  0,  8,      shapeCost,      MARKERCOST,   2 * shapeCost, 0.950, RipUpMode::DRC    , false}, //  3
    {7, -1,  8,      shapeCost,      MARKERCOST,   2 * shapeCost, 0.950, RipUpMode::DRC    , false}, //  4
    {7, -2,  8,      shapeCost,      MARKERCOST,   2 * shapeCost, 0.950, RipUpMode::DRC    , false}, //  5
    {7, -3,  8,      shapeCost,      MARKERCOST,   2 * shapeCost, 0.950, RipUpMode::DRC    , false}, //  6
    {7, -4,  8,      shapeCost,      MARKERCOST,   2 * shapeCost, 0.950, RipUpMode::DRC    , false}, //  7
    {7, -5,  8,      shapeCost,      MARKERCOST,   2 * shapeCost, 0.950, RipUpMode::DRC    , false}, //  8
    {7, -6,  8,      shapeCost,      MARKERCOST,   2 * shapeCost, 0.950, RipUpMode::DRC    , false}, //  9
    {7,  0,  8,  2 * shapeCost,      MARKERCOST,   3 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 10
    {7, -1,  8,  2 * shapeCost,      MARKERCOST,   3 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 11
    {7, -2,  8,  2 * shapeCost,      MARKERCOST,   3 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 12
    {7, -3,  8,  2 * shapeCost,      MARKERCOST,   3 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 13
    {7, -4,  8,  2 * shapeCost,      MARKERCOST,   3 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 14
    {7, -5,  8,  2 * shapeCost,      MARKERCOST,   4 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 15
    {7, -6,  8,  2 * shapeCost,      MARKERCOST,   4 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 16
    {7, -3,  8,      shapeCost,      MARKERCOST,   4 * shapeCost, 0.950, RipUpMode::ALL    , false}, // 17
    {7,  0,  8,  4 * shapeCost,      MARKERCOST,   4 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 18
    {7, -1,  8,  4 * shapeCost,      MARKERCOST,   4 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 19
    {7, -2,  8,  4 * shapeCost,      MARKERCOST,  10 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 20
    {7, -3,  8,  4 * shapeCost,      MARKERCOST,  10 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 21
    {7, -4,  8,  4 * shapeCost,      MARKERCOST,  10 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 22
    {7, -5,  8,      shapeCost,      MARKERCOST,  10 * shapeCost, 0.950, RipUpMode::NEARDRC, false}, // 23
    {7, -6,  8,  4 * shapeCost,      MARKERCOST,  10 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 24
    {5, -2,  8,      shapeCost,      MARKERCOST,  10 * shapeCost, 0.950, RipUpMode::ALL    , false}, // 25
    {7,  0,  8,  8 * shapeCost,  2 * MARKERCOST,  10 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 26
    {7, -1,  8,  8 * shapeCost,  2 * MARKERCOST,  10 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 27
    {7, -2,  8,  8 * shapeCost,  2 * MARKERCOST,  10 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 28
    {7, -3,  8,  8 * shapeCost,  2 * MARKERCOST,  10 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 29
    {7, -4,  8,      shapeCost,      MARKERCOST,  50 * shapeCost, 0.950, RipUpMode::NEARDRC, false}, // 30
    {7, -5,  8,  8 * shapeCost,  2 * MARKERCOST,  50 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 31
    {7, -6,  8,  8 * shapeCost,  2 * MARKERCOST,  50 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 32
    {3, -1,  8,      shapeCost,      MARKERCOST,  50 * shapeCost, 0.950, RipUpMode::ALL    , false}, // 33
    {7,  0,  8, 16 * shapeCost,  4 * MARKERCOST,  50 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 34
    {7, -1,  8, 16 * shapeCost,  4 * MARKERCOST,  50 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 35
    {7, -2,  8, 16 * shapeCost,  4 * MARKERCOST,  50 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 36
    {7, -3,  8,      shapeCost,      MARKERCOST,  50 * shapeCost, 0.950, RipUpMode::NEARDRC, false}, // 37
    {7, -4,  8, 16 * shapeCost,  4 * MARKERCOST,  50 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 38
    {7, -5,  8, 16 * shapeCost,  4 * MARKERCOST,  50 * shapeCost, 0.950, RipUpMode::DRC    , false}, // 39
    {7, -6,  8, 16 * shapeCost,  4 * MARKERCOST, 100 * shapeCost, 0.990, RipUpMode::DRC    , false}, // 40
    {3, -2,  8,      shapeCost,      MARKERCOST, 100 * shapeCost, 0.990, RipUpMode::ALL    , false}, // 41
    {7,  0, 16, 16 * shapeCost,  4 * MARKERCOST, 100 * shapeCost, 0.990, RipUpMode::DRC    , false}, // 42
    {7, -1, 16, 16 * shapeCost,  4 * MARKERCOST, 100 * shapeCost, 0.990, RipUpMode::DRC    , false}, // 43
    {7, -2, 16,      shapeCost,      MARKERCOST, 100 * shapeCost, 0.990, RipUpMode::NEARDRC, false}, // 44
    {7, -3, 16, 16 * shapeCost,  4 * MARKERCOST, 100 * shapeCost, 0.990, RipUpMode::DRC    , false}, // 45
    {7, -4, 16, 16 * shapeCost,  4 * MARKERCOST, 100 * shapeCost, 0.990, RipUpMode::DRC    , false}, // 46
    {7, -5, 16, 16 * shapeCost,  4 * MARKERCOST, 100 * shapeCost, 0.990, RipUpMode::DRC    , false}, // 47
    {7, -6, 16, 16 * shapeCost,  4 * MARKERCOST, 100 * shapeCost, 0.990, RipUpMode::DRC    , false}, // 48
    {3, -0,  8,      shapeCost,      MARKERCOST, 100 * shapeCost, 0.990, RipUpMode::ALL    , false}, // 49
    {7,  0, 32, 32 * shapeCost,  8 * MARKERCOST, 100 * shapeCost, 0.999, RipUpMode::DRC    , false}, // 50
    {7, -1, 32,      shapeCost,      MARKERCOST, 100 * shapeCost, 0.999, RipUpMode::NEARDRC, false}, // 51
    {7, -2, 32, 32 * shapeCost,  8 * MARKERCOST, 100 * shapeCost, 0.999, RipUpMode::DRC    , false}, // 52
    {7, -3, 32, 32 * shapeCost,  8 * MARKERCOST, 100 * shapeCost, 0.999, RipUpMode::DRC    , false}, // 53
    {7, -4, 32, 32 * shapeCost,  8 * MARKERCOST, 100 * shapeCost, 0.999, RipUpMode::DRC    , false}, // 54
    {7, -5, 32, 32 * shapeCost,  8 * MARKERCOST, 100 * shapeCost, 0.999, RipUpMode::DRC    , false}, // 55
    {7, -6, 32, 32 * shapeCost,  8 * MARKERCOST, 100 * shapeCost, 0.999, RipUpMode::DRC    , false}, // 56
    {3, -1,  8,      shapeCost,      MARKERCOST, 100 * shapeCost, 0.999, RipUpMode::ALL    , false}, // 57
    {7,  0, 64,      shapeCost,      MARKERCOST, 100 * shapeCost, 0.999, RipUpMode::NEARDRC, false}, // 58
    {7, -1, 64, 64 * shapeCost, 16 * MARKERCOST, 100 * shapeCost, 0.999, RipUpMode::DRC    , false}, // 59
    {7, -2, 64, 64 * shapeCost, 16 * MARKERCOST, 100 * shapeCost, 0.999, RipUpMode::DRC    , false}, // 60
    {7, -3, 64, 64 * shapeCost, 16 * MARKERCOST, 100 * shapeCost, 0.999, RipUpMode::DRC    , false}, // 61
    {7, -4, 64, 64 * shapeCost, 16 * MARKERCOST, 100 * shapeCost, 0.999, RipUpMode::DRC    , false}, // 62
    {7, -5, 64, 64 * shapeCost, 16 * MARKERCOST, 100 * shapeCost, 0.999, RipUpMode::DRC    , false}, // 63
    {7, -6, 64, 64 * shapeCost, 16 * MARKERCOST, 100 * shapeCost, 0.999, RipUpMode::DRC    , false}  // 64
  };
  // clang-format on
}

void addRectToPolySet(gtl::polygon_90_set_data<frCoord>& polySet, Rect rect)
{
  gtl::polygon_90_data<frCoord> poly;
  std::vector<gtl::point_data<frCoord>> points;
  for (const auto& point : rect.getPoints()) {
    points.emplace_back(point.x(), point.y());
  }
  poly.set(points.begin(), points.end());
  using boost::polygon::operators::operator+=;
  polySet += poly;
}

void FlexDR::reportGuideCoverage()
{
  using boost::polygon::operators::operator&;

  const auto numLayers = getTech()->getLayers().size();
  std::vector<uint64_t> totalAreaByLayerNum(numLayers, 0);
  std::vector<uint64_t> totalCoveredAreaByLayerNum(numLayers, 0);
  std::map<frNet*, std::vector<float>> netsCoverage;
  const auto& nets = getDesign()->getTopBlock()->getNets();
  omp_set_num_threads(MAX_THREADS);
#pragma omp parallel for schedule(dynamic)
  for (int i = 0; i < nets.size(); i++) {  // NOLINT
    const auto& net = nets.at(i);
    std::vector<gtl::polygon_90_set_data<frCoord>> routeSetByLayerNum(
        numLayers),
        guideSetByLayerNum(numLayers);
    for (const auto& shape : net->getShapes()) {
      odb::Rect rect = shape->getBBox();
      addRectToPolySet(routeSetByLayerNum[shape->getLayerNum()], rect);
    }
    for (const auto& pwire : net->getPatchWires()) {
      odb::Rect rect = pwire->getBBox();
      addRectToPolySet(routeSetByLayerNum[pwire->getLayerNum()], rect);
    }
    for (const auto& via : net->getVias()) {
      {
        odb::Rect rect = via->getLayer1BBox();
        addRectToPolySet(routeSetByLayerNum[via->getViaDef()->getLayer1Num()],
                         rect);
      }
      {
        odb::Rect rect = via->getLayer2BBox();
        addRectToPolySet(routeSetByLayerNum[via->getViaDef()->getLayer2Num()],
                         rect);
      }
    }

    for (const auto& shape : net->getOrigGuides()) {
      odb::Rect rect = shape.getBBox();
      addRectToPolySet(guideSetByLayerNum[shape.getLayerNum()], rect);
    }

    for (frLayerNum lNum = 0; lNum < numLayers; lNum++) {
      if (getTech()->getLayer(lNum)->getType() != dbTechLayerType::ROUTING
          || lNum > TOP_ROUTING_LAYER) {
        continue;
      }
      float coveredPercentage = -1.0;
      uint64_t routingArea = 0;
      uint64_t coveredArea = 0;
      if (!routeSetByLayerNum[lNum].empty()) {
        routingArea = gtl::area(routeSetByLayerNum[lNum]);
        coveredArea
            = gtl::area(routeSetByLayerNum[lNum] & guideSetByLayerNum[lNum]);
        if (routingArea == 0.0) {
          coveredPercentage = -1.0;
        } else {
          coveredPercentage = (coveredArea / (double) routingArea) * 100;
        }
      }

#pragma omp critical
      {
        netsCoverage[net.get()].push_back(coveredPercentage);
        totalAreaByLayerNum[lNum] += routingArea;
        totalCoveredAreaByLayerNum[lNum] += coveredArea;
      }
    }
  }

  std::ofstream file(GUIDE_REPORT_FILE);
  file << "Net,";
  for (const auto& layer : getTech()->getLayers()) {
    if (layer->getType() == dbTechLayerType::ROUTING
        && layer->getLayerNum() <= TOP_ROUTING_LAYER) {
      file << layer->getName() << ",";
    }
  }
  file << std::endl;
  for (const auto& [net, coverage] : netsCoverage) {
    file << net->getName() << ",";
    for (auto coveredPercentage : coverage) {
      if (coveredPercentage < 0.0) {
        file << "NA,";
      } else {
        file << fmt::format("{:.2f}%,", coveredPercentage);
      }
    }
    file << std::endl;
  }
  file << "Total,";
  uint64_t totalArea = 0;
  uint64_t totalCoveredArea = 0;
  for (const auto& layer : getTech()->getLayers()) {
    if (layer->getType() == dbTechLayerType::ROUTING
        && layer->getLayerNum() <= TOP_ROUTING_LAYER) {
      if (totalAreaByLayerNum[layer->getLayerNum()] == 0) {
        file << "NA,";
        continue;
      }
      totalArea += totalAreaByLayerNum[layer->getLayerNum()];
      totalCoveredArea += totalCoveredAreaByLayerNum[layer->getLayerNum()];
      auto coveredPercentage
          = (totalCoveredAreaByLayerNum[layer->getLayerNum()]
             / (double) totalAreaByLayerNum[layer->getLayerNum()])
            * 100;
      file << fmt::format("{:.2f}%,", coveredPercentage);
    }
  }
  if (totalArea == 0) {
    file << "NA";
  } else {
    auto totalCoveredPercentage = (totalCoveredArea / (double) totalArea) * 100;
    file << fmt::format("{:.2f}%,", totalCoveredPercentage);
  }
  file.close();
}

int FlexDR::main()
{
  ProfileTask profile("DR:main");
  init();
  frTime t;
  bool incremental = false;
  bool hasFixed = false;
  for (const auto& net : getDesign()->getTopBlock()->getNets()) {
    incremental |= net->hasInitialRouting();
    hasFixed |= net->isFixed();
    if (incremental && hasFixed) {
      break;
    }
  }
  for (auto& args : strategy()) {
    int clipSize = args.size;
    if (args.ripupMode != RipUpMode::ALL) {
      if (increaseClipsize_) {
        clipSizeInc_ += 2;
      } else {
        clipSizeInc_ = std::max((float) 0, clipSizeInc_ - 0.2f);
      }
      clipSize += std::min(MAX_CLIPSIZE_INCREASE, (int) round(clipSizeInc_));
    }
    args.size = clipSize;
    if (args.ripupMode == RipUpMode::ALL) {
      if (hasFixed || (incremental && iter_ <= 2)) {
        args.ripupMode = RipUpMode::INCR;
      }
    }
    searchRepair(args);
    if (getDesign()->getTopBlock()->getNumMarkers() == 0) {
      break;
    }
    if (iter_ > END_ITERATION) {
      break;
    }
    if (logger_->debugCheck(DRT, "snapshot", 1)) {
      io::Writer writer(router_, logger_);
      writer.updateDb(db_, false, true);
      // insert the stack of vias for bterms above max layer again.
      // all routing is deleted in updateDb, so it is necessary to insert the
      // stack again.
      router_->processBTermsAboveTopLayer(true);
      ord::OpenRoad::openRoad()->writeDb(
          fmt::format("drt_iter{}.odb", iter_ - 1).c_str());
    }
  }

  end(/* done */ true);
  if (!GUIDE_REPORT_FILE.empty()) {
    reportGuideCoverage();
  }
  if (VERBOSE > 0) {
    t.print(logger_);
    std::cout << std::endl;
  }
  return 0;
}

void FlexDR::sendWorkers(
    const std::vector<std::pair<int, FlexDRWorker*>>& remote_batch,
    std::vector<std::unique_ptr<FlexDRWorker>>& batch)
{
  if (remote_batch.empty()) {
    return;
  }
  std::vector<std::pair<int, std::string>> workers;
  {
    ProfileTask task("DIST: SERIALIZE_BATCH");
    for (auto& [idx, worker] : remote_batch) {
      std::string workerStr;
      serializeWorker(worker, workerStr);
      workers.emplace_back(idx, workerStr);
    }
  }
  std::string remote_ip = dist_ip_;
  uint16_t remote_port = dist_port_;
  if (router_->getCloudSize() > 1) {
    dst::JobMessage msg(dst::JobMessage::BALANCER),
        result(dst::JobMessage::NONE);
    bool ok = dist_->sendJob(msg, dist_ip_.c_str(), dist_port_, result);
    if (!ok) {
      logger_->error(utl::DRT, 7461, "Balancer failed");
    } else {
      dst::BalancerJobDescription* desc
          = static_cast<dst::BalancerJobDescription*>(
              result.getJobDescription());
      remote_ip = desc->getWorkerIP();
      remote_port = desc->getWorkerPort();
    }
  }
  {
    dst::JobMessage msg(dst::JobMessage::ROUTING),
        result(dst::JobMessage::NONE);
    std::unique_ptr<dst::JobDescription> desc
        = std::make_unique<RoutingJobDescription>();
    RoutingJobDescription* rjd
        = static_cast<RoutingJobDescription*>(desc.get());
    rjd->setWorkers(workers);
    rjd->setSharedDir(dist_dir_);
    rjd->setSendEvery(20);
    msg.setJobDescription(std::move(desc));
    ProfileTask task("DIST: SENDJOB");
    bool ok = dist_->sendJobMultiResult(
        msg, remote_ip.c_str(), remote_port, result);
    if (!ok) {
      logger_->error(utl::DRT, 500, "Sending worker {} failed");
    }
    for (const auto& one_desc : result.getAllJobDescriptions()) {
      RoutingJobDescription* result_desc
          = static_cast<RoutingJobDescription*>(one_desc.get());
      router_->addWorkerResults(result_desc->getWorkers());
    }
  }
}

template <class Archive>
void FlexDRWorker::serialize(Archive& ar, const unsigned int version)
{
  // // We always serialize before calling main on the work unit so various
  // // fields are empty and don't need to be serialized.  I skip these to
  // // save having to write lots of serializers that will never be called.
  // if (!apSVia_.empty() || !nets_.empty() || !owner2nets_.empty()
  //     || !rq_.isEmpty() || gcWorker_) {
  //   logger_->error(DRT, 999, "Can't serialize used worker");
  // }

  // The logger_, graphics_ and debugSettings_ are handled by the caller to
  // use the current ones.
  if (is_loading(ar)) {
    frDesign* design = ar.getDesign();
    design_ = design;
  }
  (ar) & routeBox_;
  (ar) & extBox_;
  (ar) & drcBox_;
  (ar) & gcellBox_;
  (ar) & drIter_;
  (ar) & mazeEndIter_;
  (ar) & followGuide_;
  (ar) & needRecheck_;
  (ar) & skipRouting_;
  (ar) & ripupMode_;
  (ar) & workerDRCCost_;
  (ar) & workerMarkerCost_;
  (ar) & workerFixedShapeCost_;
  (ar) & workerMarkerDecay_;
  (ar) & initNumMarkers_;
  (ar) & nets_;
  (ar) & markers_;
  (ar) & bestMarkers_;
  (ar) & isCongested_;
  if (is_loading(ar)) {
    gridGraph_.setTech(design_->getTech());
    gridGraph_.setWorker(this);
    // boundaryPin_
    int sz = 0;
    (ar) & sz;
    while (sz--) {
      frBlockObject* obj;
      serializeBlockObject(ar, obj);
      std::set<std::pair<Point, frLayerNum>> val;
      (ar) & val;
      boundaryPin_[(frNet*) obj] = val;
    }
    // owner2nets_
    for (auto& net : nets_) {
      owner2nets_[net->getFrNet()].push_back(net.get());
    }
    dist_on_ = true;
  } else {
    // boundaryPin_
    int sz = boundaryPin_.size();
    (ar) & sz;
    for (auto& [net, value] : boundaryPin_) {
      frBlockObject* obj = (frBlockObject*) net;
      serializeBlockObject(ar, obj);
      (ar) & value;
    }
  }
}

std::unique_ptr<FlexDRWorker> FlexDRWorker::load(const std::string& workerStr,
                                                 utl::Logger* logger,
                                                 frDesign* design,
                                                 FlexDRGraphics* graphics)
{
  auto worker = std::make_unique<FlexDRWorker>();
  deserializeWorker(worker.get(), design, workerStr);

  // We need to fix up the fields we want from the current run rather
  // than the stored ones.
  worker->setLogger(logger);
  worker->setGraphics(graphics);

  return worker;
}

// Explicit instantiations
template void FlexDRWorker::serialize<frIArchive>(
    frIArchive& ar,
    const unsigned int file_version);

template void FlexDRWorker::serialize<frOArchive>(
    frOArchive& ar,
    const unsigned int file_version);

}  // namespace drt
