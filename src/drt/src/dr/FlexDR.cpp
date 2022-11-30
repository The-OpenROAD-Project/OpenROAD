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
#include <stdio.h>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/io/ios_state.hpp>
#include <chrono>
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

using namespace std;
using namespace fr;

using utl::ThreadException;

BOOST_CLASS_EXPORT(RoutingJobDescription)

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

void serializeViaData(FlexDRViaData viaData, std::string& serializedStr)
{
  std::stringstream stream(std::ios_base::binary | std::ios_base::in
                           | std::ios_base::out);
  frOArchive ar(stream);
  registerTypes(ar);
  ar << viaData;
  serializedStr = stream.str();
}

FlexDR::FlexDR(triton_route::TritonRoute* router,
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

FlexDR::~FlexDR()
{
}

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
  route_queue();
  setGCWorker(nullptr);
  cleanup();
  std::string workerStr;
  serializeWorker(this, workerStr);
  return workerStr;
}

void serializeUpdates(const std::vector<std::vector<drUpdate>>& updates,
                      const std::string& file_name)
{
  std::ofstream file(file_name.c_str());
  frOArchive ar(file);
  registerTypes(ar);
  ar << updates;
  file.close();
}

int FlexDRWorker::main(frDesign* design)
{
  ProfileTask profile("DRW:main");
  using namespace std::chrono;
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
      && routeBox_.intersects({debugSettings_->x, debugSettings_->y})
      && debugSettings_->iter == getDRIter()) {
    serializeUpdates(design_->getUpdates(),
                     fmt::format("{}/updates.bin", debugSettings_->dumpDir));
    design_->clearUpdates();
    std::string viaDataStr;
    serializeViaData(*via_data_, viaDataStr);
    ofstream viaDataFile(
        fmt::format("{}/viadata.bin", debugSettings_->dumpDir).c_str());
    viaDataFile << viaDataStr;
    std::string workerStr;
    serializeWorker(this, workerStr);
    ofstream workerFile(
        fmt::format("{}/worker.bin", debugSettings_->dumpDir).c_str());
    workerFile << workerStr;
    workerFile.close();
    std::ofstream file(
        fmt::format("{}/worker_globals.bin", debugSettings_->dumpDir).c_str());
    frOArchive ar(file);
    registerTypes(ar);
    serializeGlobals(ar);
    file.close();
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

  duration<double> time_span0 = duration_cast<duration<double>>(t1 - t0);
  duration<double> time_span1 = duration_cast<duration<double>>(t2 - t1);
  duration<double> time_span2 = duration_cast<duration<double>>(t3 - t2);

  if (VERBOSE > 1) {
    stringstream ss;
    ss << "time (INIT/ROUTE/POST) " << time_span0.count() << " "
       << time_span1.count() << " " << time_span2.count() << " " << endl;
    cout << ss.str() << flush;
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
          unique_ptr<frShape> ps = make_unique<frPathSeg>(
              *(static_cast<frPathSeg*>(connFig.get())));
          auto [bp, ep] = static_cast<frPathSeg*>(ps.get())->getPoints();
          if (ep.x() - bp.x() + ep.y() - bp.y() == 1) {
            ;  // skip TA dummy segment
          } else {
            net->addShape(std::move(ps));
          }
        } else {
          cout << "Error: initFromTA unsupported shape" << endl;
        }
      }
    }
  }
}

void FlexDR::initGCell2BoundaryPin()
{
  // initiailize size
  auto gCellPatterns = getDesign()->getTopBlock()->getGCellPatterns();
  auto& xgp = gCellPatterns.at(0);
  auto& ygp = gCellPatterns.at(1);
  auto tmpVec
      = vector<map<frNet*, set<pair<Point, frLayerNum>>, frBlockObjectComp>>(
          (int) ygp.getCount());
  gcell2BoundaryPin_ = vector<
      vector<map<frNet*, set<pair<Point, frLayerNum>>, frBlockObjectComp>>>(
      (int) xgp.getCount(), tmpVec);
  for (auto& net : getDesign()->getTopBlock()->getNets()) {
    auto netPtr = net.get();
    for (auto& guide : net->getGuides()) {
      for (auto& connFig : guide->getRoutes()) {
        if (connFig->typeId() == frcPathSeg) {
          auto ps = static_cast<frPathSeg*>(connFig.get());
          auto [bp, ep] = ps->getPoints();
          frLayerNum layerNum = ps->getLayerNum();
          // skip TA dummy segment
          if (ep.x() - bp.x() + ep.y() - bp.y() == 1
              || ep.x() - bp.x() + ep.y() - bp.y() == 0) {
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
              Rect gcellBox
                  = getDesign()->getTopBlock()->getGCellBox(Point(x, y));
              frCoord leftBound = gcellBox.xMin();
              frCoord rightBound = gcellBox.xMax();
              bool hasLeftBound = true;
              bool hasRightBound = true;
              if (bp.x() < leftBound) {
                hasLeftBound = true;
              } else {
                hasLeftBound = false;
              }
              if (ep.x() >= rightBound) {
                hasRightBound = true;
              } else {
                hasRightBound = false;
              }
              if (hasLeftBound) {
                Point boundaryPt(leftBound, bp.y());
                gcell2BoundaryPin_[x][y][netPtr].insert(
                    make_pair(boundaryPt, layerNum));
              }
              if (hasRightBound) {
                Point boundaryPt(rightBound, ep.y());
                gcell2BoundaryPin_[x][y][netPtr].insert(
                    make_pair(boundaryPt, layerNum));
              }
            }
          } else if (bp.x() == ep.x()) {
            int x = idx1.x();
            int y1 = idx1.y();
            int y2 = idx2.y();
            for (auto y = y1; y <= y2; ++y) {
              Rect gcellBox
                  = getDesign()->getTopBlock()->getGCellBox(Point(x, y));
              frCoord bottomBound = gcellBox.yMin();
              frCoord topBound = gcellBox.yMax();
              bool hasBottomBound = true;
              bool hasTopBound = true;
              if (bp.y() < bottomBound) {
                hasBottomBound = true;
              } else {
                hasBottomBound = false;
              }
              if (ep.y() >= topBound) {
                hasTopBound = true;
              } else {
                hasTopBound = false;
              }
              if (hasBottomBound) {
                Point boundaryPt(bp.x(), bottomBound);
                gcell2BoundaryPin_[x][y][netPtr].insert(
                    make_pair(boundaryPt, layerNum));
              }
              if (hasTopBound) {
                Point boundaryPt(ep.x(), topBound);
                gcell2BoundaryPin_[x][y][netPtr].insert(
                    make_pair(boundaryPt, layerNum));
              }
            }
          } else {
            cout << "Error: non-orthogonal pathseg in initGCell2BoundryPin\n";
          }
        }
      }
    }
  }
}

frCoord FlexDR::init_via2viaMinLen_minimumcut1(frLayerNum lNum,
                                               frViaDef* viaDef1,
                                               frViaDef* viaDef2)
{
  if (!(viaDef1 && viaDef2)) {
    return 0;
  }

  frCoord sol = 0;

  // check min len in lNum assuming pre dir routing
  bool isH
      = (getTech()->getLayer(lNum)->getDir() == dbTechLayerDir::HORIZONTAL);

  bool isVia1Above = false;
  frVia via1(viaDef1);
  Rect viaBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    viaBox1 = via1.getLayer1BBox();
    isVia1Above = true;
  } else {
    viaBox1 = via1.getLayer2BBox();
    isVia1Above = false;
  }
  Rect cutBox1 = via1.getCutBBox();
  int width1 = viaBox1.minDXDY();
  int length1 = viaBox1.maxDXDY();

  bool isVia2Above = false;
  frVia via2(viaDef2);
  Rect viaBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    viaBox2 = via2.getLayer1BBox();
    isVia2Above = true;
  } else {
    viaBox2 = via2.getLayer2BBox();
    isVia2Above = false;
  }
  Rect cutBox2 = via2.getCutBBox();
  int width2 = viaBox2.minDXDY();
  int length2 = viaBox2.maxDXDY();

  for (auto& con : getTech()->getLayer(lNum)->getMinimumcutConstraints()) {
    if ((!con->hasLength() || (con->hasLength() && length1 > con->getLength()))
        && width1 > con->getWidth()) {
      bool checkVia2 = false;
      if (!con->hasConnection()) {
        checkVia2 = true;
      } else {
        if (con->getConnection() == frMinimumcutConnectionEnum::FROMABOVE
            && isVia2Above) {
          checkVia2 = true;
        } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMBELOW
                   && !isVia2Above) {
          checkVia2 = true;
        }
      }
      if (!checkVia2) {
        continue;
      }
      if (isH) {
        sol = max(sol,
                  (con->hasLength() ? con->getDistance() : 0)
                      + max(cutBox2.xMax() - 0 + 0 - viaBox1.xMin(),
                            viaBox1.xMax() - 0 + 0 - cutBox2.xMin()));
      } else {
        sol = max(sol,
                  (con->hasLength() ? con->getDistance() : 0)
                      + max(cutBox2.yMax() - 0 + 0 - viaBox1.yMin(),
                            viaBox1.yMax() - 0 + 0 - cutBox2.yMin()));
      }
    }
    // check via1cut to via2metal
    if ((!con->hasLength() || (con->hasLength() && length2 > con->getLength()))
        && width2 > con->getWidth()) {
      bool checkVia1 = false;
      if (!con->hasConnection()) {
        checkVia1 = true;
      } else {
        if (con->getConnection() == frMinimumcutConnectionEnum::FROMABOVE
            && isVia1Above) {
          checkVia1 = true;
        } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMBELOW
                   && !isVia1Above) {
          checkVia1 = true;
        }
      }
      if (!checkVia1) {
        continue;
      }
      if (isH) {
        sol = max(sol,
                  (con->hasLength() ? con->getDistance() : 0)
                      + max(cutBox1.xMax() - 0 + 0 - viaBox2.xMin(),
                            viaBox2.xMax() - 0 + 0 - cutBox1.xMin()));
      } else {
        sol = max(sol,
                  (con->hasLength() ? con->getDistance() : 0)
                      + max(cutBox1.yMax() - 0 + 0 - viaBox2.yMin(),
                            viaBox2.yMax() - 0 + 0 - cutBox1.yMin()));
      }
    }
  }

  return sol;
}

bool FlexDR::init_via2viaMinLen_minimumcut2(frLayerNum lNum,
                                            frViaDef* viaDef1,
                                            frViaDef* viaDef2)
{
  if (!(viaDef1 && viaDef2)) {
    return true;
  }
  // skip if same-layer via
  if (viaDef1 == viaDef2) {
    return true;
  }

  bool sol = true;

  bool isVia1Above = false;
  frVia via1(viaDef1);
  Rect viaBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    viaBox1 = via1.getLayer1BBox();
    isVia1Above = true;
  } else {
    viaBox1 = via1.getLayer2BBox();
    isVia1Above = false;
  }
  int width1 = viaBox1.minDXDY();

  bool isVia2Above = false;
  frVia via2(viaDef2);
  Rect viaBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    viaBox2 = via2.getLayer1BBox();
    isVia2Above = true;
  } else {
    viaBox2 = via2.getLayer2BBox();
    isVia2Above = false;
  }
  int width2 = viaBox2.minDXDY();

  for (auto& con : getTech()->getLayer(lNum)->getMinimumcutConstraints()) {
    if (con->hasLength()) {
      continue;
    }
    // check via2cut to via1metal
    if (width1 > con->getWidth()) {
      bool checkVia2 = false;
      if (!con->hasConnection()) {
        checkVia2 = true;
      } else {
        // has length rule
        if (con->getConnection() == frMinimumcutConnectionEnum::FROMABOVE
            && isVia2Above) {
          checkVia2 = true;
        } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMBELOW
                   && !isVia2Above) {
          checkVia2 = true;
        }
      }
      if (!checkVia2) {
        continue;
      }
      sol = false;
      break;
    }
    // check via1cut to via2metal
    if (width2 > con->getWidth()) {
      bool checkVia1 = false;
      if (!con->hasConnection()) {
        checkVia1 = true;
      } else {
        if (con->getConnection() == frMinimumcutConnectionEnum::FROMABOVE
            && isVia1Above) {
          checkVia1 = true;
        } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMBELOW
                   && !isVia1Above) {
          checkVia1 = true;
        }
      }
      if (!checkVia1) {
        continue;
      }
      sol = false;
      break;
    }
  }
  return sol;
}

frCoord FlexDR::init_via2viaMinLen_minSpc(frLayerNum lNum,
                                          frViaDef* viaDef1,
                                          frViaDef* viaDef2)
{
  if (!(viaDef1 && viaDef2)) {
    // cout <<"hehehehehe" <<endl;
    return 0;
  }

  frCoord sol = 0;

  // check min len in lNum assuming pre dir routing
  bool isH
      = (getTech()->getLayer(lNum)->getDir() == dbTechLayerDir::HORIZONTAL);
  frCoord defaultWidth = getTech()->getLayer(lNum)->getWidth();

  frVia via1(viaDef1);
  Rect viaBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    viaBox1 = via1.getLayer1BBox();
  } else {
    viaBox1 = via1.getLayer2BBox();
  }
  auto width1 = viaBox1.minDXDY();
  bool isVia1Fat = isH ? (viaBox1.yMax() - viaBox1.yMin() > defaultWidth)
                       : (viaBox1.xMax() - viaBox1.xMin() > defaultWidth);
  auto prl1 = isH ? (viaBox1.yMax() - viaBox1.yMin())
                  : (viaBox1.xMax() - viaBox1.xMin());

  frVia via2(viaDef2);
  Rect viaBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    viaBox2 = via2.getLayer1BBox();
  } else {
    viaBox2 = via2.getLayer2BBox();
  }
  auto width2 = viaBox2.minDXDY();
  bool isVia2Fat = isH ? (viaBox2.yMax() - viaBox2.yMin() > defaultWidth)
                       : (viaBox2.xMax() - viaBox2.xMin() > defaultWidth);
  auto prl2 = isH ? (viaBox2.yMax() - viaBox2.yMin())
                  : (viaBox2.xMax() - viaBox2.xMin());

  frCoord reqDist = 0;
  if (isVia1Fat && isVia2Fat) {
    reqDist = getTech()->getLayer(lNum)->getMinSpacingValue(
        width1, width2, min(prl1, prl2), false);
    if (isH) {
      reqDist += max((viaBox1.xMax() - 0), (0 - viaBox1.xMin()));
      reqDist += max((viaBox2.xMax() - 0), (0 - viaBox2.xMin()));
    } else {
      reqDist += max((viaBox1.yMax() - 0), (0 - viaBox1.yMin()));
      reqDist += max((viaBox2.yMax() - 0), (0 - viaBox2.yMin()));
    }
    sol = max(sol, reqDist);
  }

  // check min len in layer2 if two vias are in same layer
  if (viaDef1 != viaDef2) {
    return sol;
  }

  if (viaDef1->getLayer1Num() == lNum) {
    viaBox1 = via1.getLayer2BBox();
    lNum = lNum + 2;
  } else {
    viaBox1 = via1.getLayer1BBox();
    lNum = lNum - 2;
  }
  width1 = viaBox1.minDXDY();
  prl1 = isH ? (viaBox1.yMax() - viaBox1.yMin())
             : (viaBox1.xMax() - viaBox1.xMin());
  reqDist = getTech()->getLayer(lNum)->getMinSpacingValue(
      width1, width2, prl1, false);
  if (isH) {
    reqDist += (viaBox1.xMax() - 0) + (0 - viaBox1.xMin());
  } else {
    reqDist += (viaBox1.yMax() - 0) + (0 - viaBox1.yMin());
  }
  sol = max(sol, reqDist);

  return sol;
}

void FlexDR::init_via2viaMinLen()
{
  auto bottomLayerNum = getTech()->getBottomLayerNum();
  auto topLayerNum = getTech()->getTopLayerNum();
  auto& via2viaMinLen = via_data_.via2viaMinLen;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getTech()->getLayer(lNum)->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    vector<frCoord> via2viaMinLenTmp(4, 0);
    vector<bool> via2viaZeroLen(4, true);
    via2viaMinLen.push_back(make_pair(via2viaMinLenTmp, via2viaZeroLen));
  }
  // check prl
  int i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getTech()->getLayer(lNum)->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    frViaDef* downVia = nullptr;
    frViaDef* upVia = nullptr;
    if (getTech()->getBottomLayerNum() <= lNum - 1) {
      downVia = getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (getTech()->getTopLayerNum() >= lNum + 1) {
      upVia = getTech()->getLayer(lNum + 1)->getDefaultViaDef();
    }
    (via2viaMinLen[i].first)[0]
        = max((via2viaMinLen[i].first)[0],
              init_via2viaMinLen_minSpc(lNum, downVia, downVia));
    (via2viaMinLen[i].first)[1]
        = max((via2viaMinLen[i].first)[1],
              init_via2viaMinLen_minSpc(lNum, downVia, upVia));
    (via2viaMinLen[i].first)[2]
        = max((via2viaMinLen[i].first)[2],
              init_via2viaMinLen_minSpc(lNum, upVia, downVia));
    (via2viaMinLen[i].first)[3]
        = max((via2viaMinLen[i].first)[3],
              init_via2viaMinLen_minSpc(lNum, upVia, upVia));
    i++;
  }

  // check minimumcut
  i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getTech()->getLayer(lNum)->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    frViaDef* downVia = nullptr;
    frViaDef* upVia = nullptr;
    if (getTech()->getBottomLayerNum() <= lNum - 1) {
      downVia = getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (getTech()->getTopLayerNum() >= lNum + 1) {
      upVia = getTech()->getLayer(lNum + 1)->getDefaultViaDef();
    }
    vector<frCoord> via2viaMinLenTmp(4, 0);
    (via2viaMinLen[i].first)[0]
        = max((via2viaMinLen[i].first)[0],
              init_via2viaMinLen_minimumcut1(lNum, downVia, downVia));
    (via2viaMinLen[i].first)[1]
        = max((via2viaMinLen[i].first)[1],
              init_via2viaMinLen_minimumcut1(lNum, downVia, upVia));
    (via2viaMinLen[i].first)[2]
        = max((via2viaMinLen[i].first)[2],
              init_via2viaMinLen_minimumcut1(lNum, upVia, downVia));
    (via2viaMinLen[i].first)[3]
        = max((via2viaMinLen[i].first)[3],
              init_via2viaMinLen_minimumcut1(lNum, upVia, upVia));
    (via2viaMinLen[i].second)[0]
        = (via2viaMinLen[i].second)[0]
          && init_via2viaMinLen_minimumcut2(lNum, downVia, downVia);
    (via2viaMinLen[i].second)[1]
        = (via2viaMinLen[i].second)[1]
          && init_via2viaMinLen_minimumcut2(lNum, downVia, upVia);
    (via2viaMinLen[i].second)[2]
        = (via2viaMinLen[i].second)[2]
          && init_via2viaMinLen_minimumcut2(lNum, upVia, downVia);
    (via2viaMinLen[i].second)[3]
        = (via2viaMinLen[i].second)[3]
          && init_via2viaMinLen_minimumcut2(lNum, upVia, upVia);
    i++;
  }
}

frCoord FlexDR::init_via2viaMinLenNew_minimumcut1(frLayerNum lNum,
                                                  frViaDef* viaDef1,
                                                  frViaDef* viaDef2,
                                                  bool isCurrDirY)
{
  if (!(viaDef1 && viaDef2)) {
    return 0;
  }

  frCoord sol = 0;

  // check min len in lNum assuming pre dir routing
  bool isCurrDirX = !isCurrDirY;

  bool isVia1Above = false;
  frVia via1(viaDef1);
  Rect viaBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    viaBox1 = via1.getLayer1BBox();
    isVia1Above = true;
  } else {
    viaBox1 = via1.getLayer2BBox();
    isVia1Above = false;
  }
  Rect cutBox1 = via1.getCutBBox();
  int width1 = viaBox1.minDXDY();
  int length1 = viaBox1.maxDXDY();

  bool isVia2Above = false;
  frVia via2(viaDef2);
  Rect viaBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    viaBox2 = via2.getLayer1BBox();
    isVia2Above = true;
  } else {
    viaBox2 = via2.getLayer2BBox();
    isVia2Above = false;
  }
  Rect cutBox2 = via2.getCutBBox();
  int width2 = viaBox2.minDXDY();
  int length2 = viaBox2.maxDXDY();

  for (auto& con : getTech()->getLayer(lNum)->getMinimumcutConstraints()) {
    // check via2cut to via1metal
    // no length OR metal1 shape satisfies --> check via2
    if ((!con->hasLength() || (con->hasLength() && length1 > con->getLength()))
        && width1 > con->getWidth()) {
      bool checkVia2 = false;
      if (!con->hasConnection()) {
        checkVia2 = true;
      } else {
        if (con->getConnection() == frMinimumcutConnectionEnum::FROMABOVE
            && isVia2Above) {
          checkVia2 = true;
        } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMBELOW
                   && !isVia2Above) {
          checkVia2 = true;
        }
      }
      if (!checkVia2) {
        continue;
      }
      if (isCurrDirX) {
        sol = max(sol,
                  (con->hasLength() ? con->getDistance() : 0)
                      + max(cutBox2.xMax() - 0 + 0 - viaBox1.xMin(),
                            viaBox1.xMax() - 0 + 0 - cutBox2.xMin()));
      } else {
        sol = max(sol,
                  (con->hasLength() ? con->getDistance() : 0)
                      + max(cutBox2.yMax() - 0 + 0 - viaBox1.yMin(),
                            viaBox1.yMax() - 0 + 0 - cutBox2.yMin()));
      }
    }
    // check via1cut to via2metal
    if ((!con->hasLength() || (con->hasLength() && length2 > con->getLength()))
        && width2 > con->getWidth()) {
      bool checkVia1 = false;
      if (!con->hasConnection()) {
        checkVia1 = true;
      } else {
        if (con->getConnection() == frMinimumcutConnectionEnum::FROMABOVE
            && isVia1Above) {
          checkVia1 = true;
        } else if (con->getConnection() == frMinimumcutConnectionEnum::FROMBELOW
                   && !isVia1Above) {
          checkVia1 = true;
        }
      }
      if (!checkVia1) {
        continue;
      }
      if (isCurrDirX) {
        sol = max(sol,
                  (con->hasLength() ? con->getDistance() : 0)
                      + max(cutBox1.xMax() - 0 + 0 - viaBox2.xMin(),
                            viaBox2.xMax() - 0 + 0 - cutBox1.xMin()));
      } else {
        sol = max(sol,
                  (con->hasLength() ? con->getDistance() : 0)
                      + max(cutBox1.yMax() - 0 + 0 - viaBox2.yMin(),
                            viaBox2.yMax() - 0 + 0 - cutBox1.yMin()));
      }
    }
  }

  return sol;
}

frCoord FlexDR::init_via2viaMinLenNew_minSpc(frLayerNum lNum,
                                             frViaDef* viaDef1,
                                             frViaDef* viaDef2,
                                             bool isCurrDirY)
{
  if (!(viaDef1 && viaDef2)) {
    return 0;
  }

  frCoord sol = 0;

  // check min len in lNum assuming pre dir routing
  bool isCurrDirX = !isCurrDirY;
  frCoord defaultWidth = getTech()->getLayer(lNum)->getWidth();

  frVia via1(viaDef1);
  Rect viaBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    viaBox1 = via1.getLayer1BBox();
  } else {
    viaBox1 = via1.getLayer2BBox();
  }
  auto width1 = viaBox1.minDXDY();
  bool isVia1Fat = isCurrDirX
                       ? (viaBox1.yMax() - viaBox1.yMin() > defaultWidth)
                       : (viaBox1.xMax() - viaBox1.xMin() > defaultWidth);
  auto prl1 = isCurrDirX ? (viaBox1.yMax() - viaBox1.yMin())
                         : (viaBox1.xMax() - viaBox1.xMin());

  frVia via2(viaDef2);
  Rect viaBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    viaBox2 = via2.getLayer1BBox();
  } else {
    viaBox2 = via2.getLayer2BBox();
  }
  auto width2 = viaBox2.minDXDY();
  bool isVia2Fat = isCurrDirX
                       ? (viaBox2.yMax() - viaBox2.yMin() > defaultWidth)
                       : (viaBox2.xMax() - viaBox2.xMin() > defaultWidth);
  auto prl2 = isCurrDirX ? (viaBox2.yMax() - viaBox2.yMin())
                         : (viaBox2.xMax() - viaBox2.xMin());

  frCoord reqDist = 0;
  if (isVia1Fat && isVia2Fat) {
    reqDist = getTech()->getLayer(lNum)->getMinSpacingValue(
        width1, width2, min(prl1, prl2), false);
    if (isCurrDirX) {
      reqDist += max((viaBox1.xMax() - 0), (0 - viaBox1.xMin()));
      reqDist += max((viaBox2.xMax() - 0), (0 - viaBox2.xMin()));
    } else {
      reqDist += max((viaBox1.yMax() - 0), (0 - viaBox1.yMin()));
      reqDist += max((viaBox2.yMax() - 0), (0 - viaBox2.yMin()));
    }
    sol = max(sol, reqDist);
  }

  // check min len in layer2 if two vias are in same layer
  if (viaDef1 != viaDef2) {
    return sol;
  }

  if (viaDef1->getLayer1Num() == lNum) {
    viaBox1 = via1.getLayer2BBox();
    lNum = lNum + 2;
  } else {
    viaBox1 = via1.getLayer1BBox();
    lNum = lNum - 2;
  }
  width1 = viaBox1.minDXDY();
  prl1 = isCurrDirX ? (viaBox1.yMax() - viaBox1.yMin())
                    : (viaBox1.xMax() - viaBox1.xMin());
  reqDist = getTech()->getLayer(lNum)->getMinSpacingValue(
      width1, width2, prl1, false);
  if (isCurrDirX) {
    reqDist += (viaBox1.xMax() - 0) + (0 - viaBox1.xMin());
  } else {
    reqDist += (viaBox1.yMax() - 0) + (0 - viaBox1.yMin());
  }
  sol = max(sol, reqDist);

  return sol;
}

frCoord FlexDR::init_via2viaMinLenNew_cutSpc(frLayerNum lNum,
                                             frViaDef* viaDef1,
                                             frViaDef* viaDef2,
                                             bool isCurrDirY)
{
  if (!(viaDef1 && viaDef2)) {
    return 0;
  }

  frCoord sol = 0;

  // check min len in lNum assuming pre dir routing
  frVia via1(viaDef1);
  Rect viaBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    viaBox1 = via1.getLayer1BBox();
  } else {
    viaBox1 = via1.getLayer2BBox();
  }
  Rect cutBox1 = via1.getCutBBox();

  frVia via2(viaDef2);
  Rect viaBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    viaBox2 = via2.getLayer1BBox();
  } else {
    viaBox2 = via2.getLayer2BBox();
  }
  Rect cutBox2 = via2.getCutBBox();
  auto boxLength = isCurrDirY ? (cutBox1.yMax() - cutBox1.yMin())
                              : (cutBox1.xMax() - cutBox1.xMin());
  // same layer (use samenet rule if exist, otherwise use diffnet rule)
  if (viaDef1->getCutLayerNum() == viaDef2->getCutLayerNum()) {
    auto layer = getTech()->getLayer(viaDef1->getCutLayerNum());
    auto samenetCons = layer->getCutSpacing(true);
    auto diffnetCons = layer->getCutSpacing(false);
    if (!samenetCons.empty()) {
      // check samenet spacing rule if exists
      for (auto con : samenetCons) {
        if (con == nullptr) {
          continue;
        }
        // filter rule, assuming default via will never trigger cutArea
        if (con->hasSecondLayer() || con->isAdjacentCuts()
            || con->isParallelOverlap() || con->isArea()
            || !con->hasSameNet()) {
          continue;
        }
        auto reqSpcVal = con->getCutSpacing();
        if (!con->hasCenterToCenter()) {
          reqSpcVal += boxLength;
        }
        sol = max(sol, reqSpcVal);
      }
    } else {
      // check diffnet spacing rule
      // filter rule, assuming default via will never trigger cutArea
      for (auto con : diffnetCons) {
        if (con == nullptr) {
          continue;
        }
        if (con->hasSecondLayer() || con->isAdjacentCuts()
            || con->isParallelOverlap() || con->isArea() || con->hasSameNet()) {
          continue;
        }
        auto reqSpcVal = con->getCutSpacing();
        if (!con->hasCenterToCenter()) {
          reqSpcVal += boxLength;
        }
        sol = max(sol, reqSpcVal);
      }
    }
    // TODO: diff layer
  } else {
    auto layerNum1 = viaDef1->getCutLayerNum();
    auto layerNum2 = viaDef2->getCutLayerNum();
    frCutSpacingConstraint* samenetCon = nullptr;
    if (getTech()->getLayer(layerNum1)->hasInterLayerCutSpacing(layerNum2,
                                                                true)) {
      samenetCon = getTech()->getLayer(layerNum1)->getInterLayerCutSpacing(
          layerNum2, true);
    }
    if (getTech()->getLayer(layerNum2)->hasInterLayerCutSpacing(layerNum1,
                                                                true)) {
      if (samenetCon) {
        cout << "Warning: duplicate diff layer samenet cut spacing, skipping "
                "cut spacing from "
             << layerNum2 << " to " << layerNum1 << endl;
      } else {
        samenetCon = getTech()->getLayer(layerNum2)->getInterLayerCutSpacing(
            layerNum1, true);
      }
    }
    if (samenetCon == nullptr) {
      if (getTech()->getLayer(layerNum1)->hasInterLayerCutSpacing(layerNum2,
                                                                  false)) {
        samenetCon = getTech()->getLayer(layerNum1)->getInterLayerCutSpacing(
            layerNum2, false);
      }
      if (getTech()->getLayer(layerNum2)->hasInterLayerCutSpacing(layerNum1,
                                                                  false)) {
        if (samenetCon) {
          cout << "Warning: duplicate diff layer diffnet cut spacing, skipping "
                  "cut spacing from "
               << layerNum2 << " to " << layerNum1 << endl;
        } else {
          samenetCon = getTech()->getLayer(layerNum2)->getInterLayerCutSpacing(
              layerNum1, false);
        }
      }
    }
    if (samenetCon) {
      // filter rule, assuming default via will never trigger cutArea
      auto reqSpcVal = samenetCon->getCutSpacing();
      if (reqSpcVal == 0) {
        ;
      } else {
        if (!samenetCon->hasCenterToCenter()) {
          reqSpcVal += boxLength;
        }
      }
      sol = max(sol, reqSpcVal);
    }
  }
  // LEF58_SPACINGTABLE
  if (viaDef2->getCutLayerNum() > viaDef1->getCutLayerNum()) {
    // swap via defs
    frViaDef* tempViaDef = viaDef2;
    viaDef2 = viaDef1;
    viaDef1 = tempViaDef;
    // swap boxes
    Rect tempCutBox(cutBox2);
    cutBox2 = cutBox1;
    cutBox1 = tempCutBox;
  }
  auto layer1 = getTech()->getLayer(viaDef1->getCutLayerNum());
  auto layer2 = getTech()->getLayer(viaDef2->getCutLayerNum());
  auto cutClassIdx1
      = layer1->getCutClassIdx(cutBox1.minDXDY(), cutBox1.maxDXDY());
  auto cutClassIdx2
      = layer2->getCutClassIdx(cutBox2.minDXDY(), cutBox2.maxDXDY());
  frString cutClass1, cutClass2;
  if (cutClassIdx1 != -1)
    cutClass1 = layer1->getCutClass(cutClassIdx1)->getName();
  if (cutClassIdx2 != -1)
    cutClass2 = layer2->getCutClass(cutClassIdx2)->getName();
  bool isSide1;
  bool isSide2;
  if (isCurrDirY) {
    isSide1
        = (cutBox1.xMax() - cutBox1.xMin()) > (cutBox1.yMax() - cutBox1.yMin());
    isSide2
        = (cutBox2.xMax() - cutBox2.xMin()) > (cutBox2.yMax() - cutBox2.yMin());
  } else {
    isSide1
        = (cutBox1.xMax() - cutBox1.xMin()) < (cutBox1.yMax() - cutBox1.yMin());
    isSide2
        = (cutBox2.xMax() - cutBox2.xMin()) < (cutBox2.yMax() - cutBox2.yMin());
  }
  if (layer1->getLayerNum() == layer2->getLayerNum()) {
    frLef58CutSpacingTableConstraint* lef58con = nullptr;
    if (layer1->hasLef58SameMetalCutSpcTblConstraint())
      lef58con = layer1->getLef58SameMetalCutSpcTblConstraint();
    else if (layer1->hasLef58SameNetCutSpcTblConstraint())
      lef58con = layer1->getLef58SameNetCutSpcTblConstraint();
    else if (layer1->hasLef58DiffNetCutSpcTblConstraint())
      lef58con = layer1->getLef58DiffNetCutSpcTblConstraint();
    if (lef58con != nullptr) {
      auto dbRule = lef58con->getODBRule();
      auto reqSpcVal
          = dbRule->getSpacing(cutClass1, isSide1, cutClass2, isSide2);
      if (!dbRule->isCenterToCenter(cutClass1, cutClass2)
          && !dbRule->isCenterAndEdge(cutClass1, cutClass2)) {
        reqSpcVal += boxLength;
      }
      sol = max(sol, reqSpcVal);
    }
  } else {
    frLef58CutSpacingTableConstraint* con = nullptr;
    if (layer1->hasLef58SameMetalInterCutSpcTblConstraint())
      con = layer1->getLef58SameMetalInterCutSpcTblConstraint();
    else if (layer1->hasLef58SameNetInterCutSpcTblConstraint())
      con = layer1->getLef58SameNetInterCutSpcTblConstraint();
    if (con != nullptr) {
      auto dbRule = con->getODBRule();
      auto reqSpcVal
          = dbRule->getSpacing(cutClass1, isSide1, cutClass2, isSide2);
      if (reqSpcVal != 0 && !dbRule->isCenterToCenter(cutClass1, cutClass2)
          && !dbRule->isCenterAndEdge(cutClass1, cutClass2)) {
        reqSpcVal += boxLength;
      }
      sol = max(sol, reqSpcVal);
    }
  }
  return sol;
}

void FlexDR::init_via2viaMinLenNew()
{
  auto bottomLayerNum = getTech()->getBottomLayerNum();
  auto topLayerNum = getTech()->getTopLayerNum();
  auto& via2viaMinLenNew = via_data_.via2viaMinLenNew;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getTech()->getLayer(lNum)->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    vector<frCoord> via2viaMinLenTmp(8, 0);
    via2viaMinLenNew.push_back(via2viaMinLenTmp);
  }
  // check prl
  int i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getTech()->getLayer(lNum)->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    frViaDef* downVia = nullptr;
    frViaDef* upVia = nullptr;
    if (getTech()->getBottomLayerNum() <= lNum - 1) {
      downVia = getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (getTech()->getTopLayerNum() >= lNum + 1) {
      upVia = getTech()->getLayer(lNum + 1)->getDefaultViaDef();
    }
    via2viaMinLenNew[i][0]
        = max(via2viaMinLenNew[i][0],
              init_via2viaMinLenNew_minSpc(lNum, downVia, downVia, false));
    via2viaMinLenNew[i][1]
        = max(via2viaMinLenNew[i][1],
              init_via2viaMinLenNew_minSpc(lNum, downVia, downVia, true));
    via2viaMinLenNew[i][2]
        = max(via2viaMinLenNew[i][2],
              init_via2viaMinLenNew_minSpc(lNum, downVia, upVia, false));
    via2viaMinLenNew[i][3]
        = max(via2viaMinLenNew[i][3],
              init_via2viaMinLenNew_minSpc(lNum, downVia, upVia, true));
    via2viaMinLenNew[i][4]
        = max(via2viaMinLenNew[i][4],
              init_via2viaMinLenNew_minSpc(lNum, upVia, downVia, false));
    via2viaMinLenNew[i][5]
        = max(via2viaMinLenNew[i][5],
              init_via2viaMinLenNew_minSpc(lNum, upVia, downVia, true));
    via2viaMinLenNew[i][6]
        = max(via2viaMinLenNew[i][6],
              init_via2viaMinLenNew_minSpc(lNum, upVia, upVia, false));
    via2viaMinLenNew[i][7]
        = max(via2viaMinLenNew[i][7],
              init_via2viaMinLenNew_minSpc(lNum, upVia, upVia, true));
    i++;
  }

  // check minimumcut
  i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getTech()->getLayer(lNum)->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    frViaDef* downVia = nullptr;
    frViaDef* upVia = nullptr;
    if (getTech()->getBottomLayerNum() <= lNum - 1) {
      downVia = getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (getTech()->getTopLayerNum() >= lNum + 1) {
      upVia = getTech()->getLayer(lNum + 1)->getDefaultViaDef();
    }
    via2viaMinLenNew[i][0]
        = max(via2viaMinLenNew[i][0],
              init_via2viaMinLenNew_minimumcut1(lNum, downVia, downVia, false));
    via2viaMinLenNew[i][1]
        = max(via2viaMinLenNew[i][1],
              init_via2viaMinLenNew_minimumcut1(lNum, downVia, downVia, true));
    via2viaMinLenNew[i][2]
        = max(via2viaMinLenNew[i][2],
              init_via2viaMinLenNew_minimumcut1(lNum, downVia, upVia, false));
    via2viaMinLenNew[i][3]
        = max(via2viaMinLenNew[i][3],
              init_via2viaMinLenNew_minimumcut1(lNum, downVia, upVia, true));
    via2viaMinLenNew[i][4]
        = max(via2viaMinLenNew[i][4],
              init_via2viaMinLenNew_minimumcut1(lNum, upVia, downVia, false));
    via2viaMinLenNew[i][5]
        = max(via2viaMinLenNew[i][5],
              init_via2viaMinLenNew_minimumcut1(lNum, upVia, downVia, true));
    via2viaMinLenNew[i][6]
        = max(via2viaMinLenNew[i][6],
              init_via2viaMinLenNew_minimumcut1(lNum, upVia, upVia, false));
    via2viaMinLenNew[i][7]
        = max(via2viaMinLenNew[i][7],
              init_via2viaMinLenNew_minimumcut1(lNum, upVia, upVia, true));
    i++;
  }

  // check cut spacing
  i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getTech()->getLayer(lNum)->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    frViaDef* downVia = nullptr;
    frViaDef* upVia = nullptr;
    if (getTech()->getBottomLayerNum() <= lNum - 1) {
      downVia = getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (getTech()->getTopLayerNum() >= lNum + 1) {
      upVia = getTech()->getLayer(lNum + 1)->getDefaultViaDef();
    }
    via2viaMinLenNew[i][0]
        = max(via2viaMinLenNew[i][0],
              init_via2viaMinLenNew_cutSpc(lNum, downVia, downVia, false));
    via2viaMinLenNew[i][1]
        = max(via2viaMinLenNew[i][1],
              init_via2viaMinLenNew_cutSpc(lNum, downVia, downVia, true));
    via2viaMinLenNew[i][2]
        = max(via2viaMinLenNew[i][2],
              init_via2viaMinLenNew_cutSpc(lNum, downVia, upVia, false));
    via2viaMinLenNew[i][3]
        = max(via2viaMinLenNew[i][3],
              init_via2viaMinLenNew_cutSpc(lNum, downVia, upVia, true));
    via2viaMinLenNew[i][4]
        = max(via2viaMinLenNew[i][4],
              init_via2viaMinLenNew_cutSpc(lNum, upVia, downVia, false));
    via2viaMinLenNew[i][5]
        = max(via2viaMinLenNew[i][5],
              init_via2viaMinLenNew_cutSpc(lNum, upVia, downVia, true));
    via2viaMinLenNew[i][6]
        = max(via2viaMinLenNew[i][6],
              init_via2viaMinLenNew_cutSpc(lNum, upVia, upVia, false));
    via2viaMinLenNew[i][7]
        = max(via2viaMinLenNew[i][7],
              init_via2viaMinLenNew_cutSpc(lNum, upVia, upVia, true));
    i++;
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
      frVia via(viaDef);
      Rect layer1Box = via.getLayer1BBox();
      Rect layer2Box = via.getLayer2BBox();
      auto layer1HalfArea = layer1Box.minDXDY() * layer1Box.maxDXDY() / 2;
      auto layer2HalfArea = layer2Box.minDXDY() * layer2Box.maxDXDY() / 2;
      halfViaEncArea.push_back(make_pair(layer1HalfArea, layer2HalfArea));
    } else {
      halfViaEncArea.push_back(make_pair(0, 0));
    }
  }
}

frCoord FlexDR::init_via2turnMinLen_minSpc(frLayerNum lNum,
                                           frViaDef* viaDef,
                                           bool isCurrDirY)
{
  if (!viaDef) {
    return 0;
  }

  frCoord sol = 0;

  // check min len in lNum assuming pre dir routing
  bool isCurrDirX = !isCurrDirY;
  frCoord defaultWidth = getTech()->getLayer(lNum)->getWidth();

  frVia via1(viaDef);
  Rect viaBox1;
  if (viaDef->getLayer1Num() == lNum) {
    viaBox1 = via1.getLayer1BBox();
  } else {
    viaBox1 = via1.getLayer2BBox();
  }
  int width1 = viaBox1.minDXDY();
  bool isVia1Fat = isCurrDirX
                       ? (viaBox1.yMax() - viaBox1.yMin() > defaultWidth)
                       : (viaBox1.xMax() - viaBox1.xMin() > defaultWidth);
  auto prl1 = isCurrDirX ? (viaBox1.yMax() - viaBox1.yMin())
                         : (viaBox1.xMax() - viaBox1.xMin());

  frCoord reqDist = 0;
  if (isVia1Fat) {
    reqDist = getTech()->getLayer(lNum)->getMinSpacingValue(
        width1, defaultWidth, prl1, false);
    if (isCurrDirX) {
      reqDist += max((viaBox1.xMax() - 0), (0 - viaBox1.xMin()));
      reqDist += defaultWidth;
    } else {
      reqDist += max((viaBox1.yMax() - 0), (0 - viaBox1.yMin()));
      reqDist += defaultWidth;
    }
    sol = max(sol, reqDist);
  }

  return sol;
}

frCoord FlexDR::init_via2turnMinLen_minStp(frLayerNum lNum,
                                           frViaDef* viaDef,
                                           bool isCurrDirY)
{
  if (!viaDef) {
    return 0;
  }

  frCoord sol = 0;

  // check min len in lNum assuming pre dir routing
  bool isCurrDirX = !isCurrDirY;
  frCoord defaultWidth = getTech()->getLayer(lNum)->getWidth();

  frVia via1(viaDef);
  Rect viaBox1;
  if (viaDef->getLayer1Num() == lNum) {
    viaBox1 = via1.getLayer1BBox();
  } else {
    viaBox1 = via1.getLayer2BBox();
  }
  bool isVia1Fat = isCurrDirX
                       ? (viaBox1.yMax() - viaBox1.yMin() > defaultWidth)
                       : (viaBox1.xMax() - viaBox1.xMin() > defaultWidth);

  frCoord reqDist = 0;
  if (isVia1Fat) {
    auto con = getTech()->getLayer(lNum)->getMinStepConstraint();
    if (con
        && con->hasMaxEdges()) {  // currently only consider maxedge violation
      reqDist = con->getMinStepLength();
      if (isCurrDirX) {
        reqDist += max((viaBox1.xMax() - 0), (0 - viaBox1.xMin()));
        reqDist += defaultWidth;
      } else {
        reqDist += max((viaBox1.yMax() - 0), (0 - viaBox1.yMin()));
        reqDist += defaultWidth;
      }
      sol = max(sol, reqDist);
    }
  }
  return sol;
}

void FlexDR::init_via2turnMinLen()
{
  auto bottomLayerNum = getTech()->getBottomLayerNum();
  auto topLayerNum = getTech()->getTopLayerNum();
  auto& via2turnMinLen = via_data_.via2turnMinLen;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getTech()->getLayer(lNum)->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    vector<frCoord> via2turnMinLenTmp(4, 0);
    via2turnMinLen.push_back(via2turnMinLenTmp);
  }
  // check prl
  int i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getTech()->getLayer(lNum)->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    frViaDef* downVia = nullptr;
    frViaDef* upVia = nullptr;
    if (getTech()->getBottomLayerNum() <= lNum - 1) {
      downVia = getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (getTech()->getTopLayerNum() >= lNum + 1) {
      upVia = getTech()->getLayer(lNum + 1)->getDefaultViaDef();
    }
    via2turnMinLen[i][0] = max(
        via2turnMinLen[i][0], init_via2turnMinLen_minSpc(lNum, downVia, false));
    via2turnMinLen[i][1] = max(via2turnMinLen[i][1],
                               init_via2turnMinLen_minSpc(lNum, downVia, true));
    via2turnMinLen[i][2] = max(via2turnMinLen[i][2],
                               init_via2turnMinLen_minSpc(lNum, upVia, false));
    via2turnMinLen[i][3] = max(via2turnMinLen[i][3],
                               init_via2turnMinLen_minSpc(lNum, upVia, true));
    i++;
  }

  // check minstep
  i = 0;
  for (auto lNum = bottomLayerNum; lNum <= topLayerNum; lNum++) {
    if (getTech()->getLayer(lNum)->getType() != dbTechLayerType::ROUTING) {
      continue;
    }
    frViaDef* downVia = nullptr;
    frViaDef* upVia = nullptr;
    if (getTech()->getBottomLayerNum() <= lNum - 1) {
      downVia = getTech()->getLayer(lNum - 1)->getDefaultViaDef();
    }
    if (getTech()->getTopLayerNum() >= lNum + 1) {
      upVia = getTech()->getLayer(lNum + 1)->getDefaultViaDef();
    }
    vector<frCoord> via2turnMinLenTmp(4, 0);
    via2turnMinLen[i][0] = max(
        via2turnMinLen[i][0], init_via2turnMinLen_minStp(lNum, downVia, false));
    via2turnMinLen[i][1] = max(via2turnMinLen[i][1],
                               init_via2turnMinLen_minStp(lNum, downVia, true));
    via2turnMinLen[i][2] = max(via2turnMinLen[i][2],
                               init_via2turnMinLen_minStp(lNum, upVia, false));
    via2turnMinLen[i][3] = max(via2turnMinLen[i][3],
                               init_via2turnMinLen_minStp(lNum, upVia, true));
    i++;
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
  init_via2viaMinLen();
  init_via2viaMinLenNew();
  init_via2turnMinLen();

  if (VERBOSE > 0) {
    t.print(logger_);
  }

  iter_ = 0;

  if (VERBOSE > 0) {
    logger_->info(DRT, 194, "Start detail routing.");
  }
}

void FlexDR::removeGCell2BoundaryPin()
{
  gcell2BoundaryPin_.clear();
  gcell2BoundaryPin_.shrink_to_fit();
}

map<frNet*, set<pair<Point, frLayerNum>>, frBlockObjectComp>
FlexDR::initDR_mergeBoundaryPin(int startX,
                                int startY,
                                int size,
                                const Rect& routeBox)
{
  map<frNet*, set<pair<Point, frLayerNum>>, frBlockObjectComp> bp;
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
            bp[net].insert(make_pair(pt, lNum));
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
  const int ripupMode = args.ripupMode;
  const bool followGuide = args.followGuide;

  std::string profile_name("DR:searchRepair");
  profile_name += std::to_string(iter);
  ProfileTask profile(profile_name.c_str());
  if (ripupMode != 1 && getDesign()->getTopBlock()->getMarkers().size() == 0) {
    return;
  }
  if (dist_on_) {
    if ((iter % 10 == 0 && iter != 60) || iter == 3 || iter == 15) {
      globals_path_ = fmt::format("{}globals.{}.ar", dist_dir_, iter);
      router_->writeGlobals(globals_path_.c_str());
    }
  }
  frTime t;
  if (VERBOSE > 0) {
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

  vector<unique_ptr<FlexDRWorker>> uworkers;
  int batchStepX, batchStepY;

  getBatchInfo(batchStepX, batchStepY);

  vector<vector<vector<unique_ptr<FlexDRWorker>>>> workers(batchStepX
                                                           * batchStepY);

  int xIdx = 0, yIdx = 0;
  for (int i = offset; i < (int) xgp.getCount(); i += size) {
    for (int j = offset; j < (int) ygp.getCount(); j += size) {
      auto worker = make_unique<FlexDRWorker>(&via_data_, design_, logger_);
      Rect routeBox1 = getDesign()->getTopBlock()->getGCellBox(Point(i, j));
      const int max_i = min((int) xgp.getCount() - 1, i + size - 1);
      const int max_j = min((int) ygp.getCount(), j + size - 1);
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
      if (dist_on_)
        worker->setDistributed(dist_, dist_ip_, dist_port_, dist_dir_);
      if (!iter) {
        // if (routeBox.xMin() == 441000 && routeBox.yMin() == 816100) {
        //   cout << "@@@ debug: " << i << " " << j << endl;
        // }
        // set boundary pin
        auto bp = initDR_mergeBoundaryPin(i, j, size, routeBox);
        worker->setDRIter(0, bp);
      }
      worker->setRipupMode(ripupMode);
      worker->setFollowGuide(followGuide);
      // TODO: only pass to relevant workers
      worker->setGraphics(graphics_.get());
      worker->setCost(workerDRCCost, workerMarkerCost);

      int batchIdx = (xIdx % batchStepX) * batchStepY + yIdx % batchStepY;
      if (workers[batchIdx].empty()
          || (!dist_on_
              && (int) workers[batchIdx].back().size() >= BATCHSIZE)) {
        workers[batchIdx].push_back(vector<unique_ptr<FlexDRWorker>>());
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
          } else
            router_->sendDesignUpdates(globals_path_);
        }
        {
          ProfileTask task("DIST: PROCESS_BATCH");
          // multi thread
          ThreadException exception;
#pragma omp parallel for schedule(dynamic)
          for (int i = 0; i < (int) workersInBatch.size(); i++) {
            try {
              if (dist_on_)
                workersInBatch[i]->distributedMain(getDesign());
              else
                workersInBatch[i]->main(getDesign());
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
              for (int i = 0; i < distWorkerBatches.size(); i++)
                sendWorkers(distWorkerBatches.at(i), workersInBatch);
            }
            logger_->report("    Received Batches:{}.", t);
            std::vector<std::pair<int, std::string>> workers;
            router_->getWorkerResults(workers);
            {
              ProfileTask task("DIST: DESERIALIZING_BATCH");
#pragma omp parallel for schedule(dynamic)
              for (int i = 0; i < workers.size(); i++) {
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
        for (int i = 0; i < (int) workersInBatch.size(); i++) {
          if (workersInBatch[i]->end(getDesign()))
            numWorkUnits_ += 1;
          if (workersInBatch[i]->isCongested())
            increaseClipsize_ = true;
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
      getDesign(),
      logger_,
      db_,
      graphics_.get(),
      dist_on_ || router_->getDebugSettings()->debugDumpDR);
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
        if (!marker->getConstraint())
          continue;
        auto type = marker->getConstraint()->getViolName();
        if (relabel.find(type) != relabel.end())
          type = relabel.at(type);
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
      for (auto [type, typeViolations] : violations) {
        std::string typeName = type;
        if (typeName.size() >= 15)
          typeName = typeName.substr(0, 12) + "..";
        line = fmt::format("{:<15}", typeName);
        for (auto lNum : layers) {
          line += fmt::format("{:>7}", typeViolations[lNum]);
        }
        logger_->report(line);
      }
    }
    t.print(logger_);
    cout << flush;
  }
  end();
  if (logger_->debugCheck(DRT, "autotuner", 1)
      || logger_->debugCheck(DRT, "report", 1)) {
    router_->reportDRC(DRC_RPT_FILE + '-' + std::to_string(iter) + ".rpt",
                       design_->getTopBlock()->getMarkers());
  }
}

void FlexDR::end(bool done)
{
  if (done && DRC_RPT_FILE != string("")) {
    router_->reportDRC(DRC_RPT_FILE, design_->getTopBlock()->getMarkers());
  }
  if (done && VERBOSE > 0) {
    logger_->info(DRT, 198, "Complete detail routing.");
  }

  using ULL = unsigned long long;
  vector<ULL> wlen(getTech()->getLayers().size(), 0);
  vector<ULL> sCut(getTech()->getLayers().size(), 0);
  vector<ULL> mCut(getTech()->getLayers().size(), 0);
  for (auto& net : getDesign()->getTopBlock()->getNets()) {
    for (auto& shape : net->getShapes()) {
      if (shape->typeId() == frcPathSeg) {
        auto obj = static_cast<frPathSeg*>(shape.get());
        auto [bp, ep] = obj->getPoints();
        auto lNum = obj->getLayerNum();
        frCoord psLen = ep.x() - bp.x() + ep.y() - bp.y();
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
    logger_->metric("route__drc_errors",
                    getDesign()->getTopBlock()->getNumMarkers());
    logger_->metric("route__wirelength",
                    totWlen / getDesign()->getTopBlock()->getDBUPerUU());
    logger_->metric("route__vias", totSCut + totMCut);
    logger_->metric("route__vias__singlecut", totSCut);
    logger_->metric("route__vias__multicut", totMCut);
  } else {
    logger_->metric(fmt::format("route__drc_errors__iter:{}", iter_),
                    getDesign()->getTopBlock()->getNumMarkers());
    logger_->metric(fmt::format("route__wirelength__iter:{}", iter_),
                    totWlen / getDesign()->getTopBlock()->getDBUPerUU());
  }

  if (VERBOSE > 0) {
    logger_->report("Total wire length = {} um.",
                    totWlen / getDesign()->getTopBlock()->getDBUPerUU());

    for (int i = getTech()->getBottomLayerNum();
         i <= getTech()->getTopLayerNum();
         i++) {
      if (getTech()->getLayer(i)->getType() == dbTechLayerType::ROUTING) {
        logger_->report("Total wire length on LAYER {} = {} um.",
                        getTech()->getLayer(i)->getName(),
                        wlen[i] / getDesign()->getTopBlock()->getDBUPerUU());
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
    logger_->report("Up-via summary (total {}):.", totSCut + totMCut);
    int nameLen = 0;
    for (int i = getTech()->getBottomLayerNum();
         i <= getTech()->getTopLayerNum();
         i++) {
      if (getTech()->getLayer(i)->getType() == dbTechLayerType::CUT) {
        nameLen
            = max(nameLen, (int) getTech()->getLayer(i - 1)->getName().size());
      }
    }
    int maxL = 1 + nameLen + 4 + (int) to_string(totSCut).length();
    if (totMCut) {
      maxL += 9 + 4 + (int) to_string(totMCut).length() + 9 + 4
              + (int) to_string(totSCut + totMCut).length();
    }
    std::ostringstream msg;
    if (totMCut) {
      msg << " " << setw(nameLen + 4 + (int) to_string(totSCut).length() + 9)
          << "single-cut";
      msg << setw(4 + (int) to_string(totMCut).length() + 9) << "multi-cut"
          << setw(4 + (int) to_string(totSCut + totMCut).length()) << "total";
    }
    msg << endl;
    for (int i = 0; i < maxL; i++) {
      msg << "-";
    }
    msg << endl;
    for (int i = getTech()->getBottomLayerNum();
         i <= getTech()->getTopLayerNum();
         i++) {
      if (getTech()->getLayer(i)->getType() == dbTechLayerType::CUT) {
        msg << " " << setw(nameLen) << getTech()->getLayer(i - 1)->getName()
            << "    " << setw((int) to_string(totSCut).length()) << sCut[i];
        if (totMCut) {
          msg << " (" << setw(5)
              << (double) ((sCut[i] + mCut[i])
                               ? sCut[i] * 100.0 / (sCut[i] + mCut[i])
                               : 0.0)
              << "%)";
          msg << "    " << setw((int) to_string(totMCut).length()) << mCut[i]
              << " (" << setw(5)
              << (double) ((sCut[i] + mCut[i])
                               ? mCut[i] * 100.0 / (sCut[i] + mCut[i])
                               : 0.0)
              << "%)"
              << "    " << setw((int) to_string(totSCut + totMCut).length())
              << sCut[i] + mCut[i];
        }
        msg << endl;
      }
    }
    for (int i = 0; i < maxL; i++) {
      msg << "-";
    }
    msg << endl;
    msg << " " << setw(nameLen) << ""
        << "    " << setw((int) to_string(totSCut).length()) << totSCut;
    if (totMCut) {
      msg << " (" << setw(5)
          << (double) ((totSCut + totMCut)
                           ? totSCut * 100.0 / (totSCut + totMCut)
                           : 0.0)
          << "%)";
      msg << "    " << setw((int) to_string(totMCut).length()) << totMCut
          << " (" << setw(5)
          << (double) ((totSCut + totMCut)
                           ? totMCut * 100.0 / (totSCut + totMCut)
                           : 0.0)
          << "%)"
          << "    " << setw((int) to_string(totSCut + totMCut).length())
          << totSCut + totMCut;
    }
    msg << endl << endl;
    logger_->report("{}", msg.str());
  }
}

static std::vector<FlexDR::SearchRepairArgs> strategy()
{
  const fr::frUInt4 shapeCost = ROUTESHAPECOST;

  return {/*  0 */ {7, 0, 3, shapeCost, 0 /*MARKERCOST*/, 1, true},
          /*  1 */ {7, -2, 3, shapeCost, shapeCost /*MARKERCOST*/, 1, true},
          /*  2 */ {7, -5, 3, shapeCost, shapeCost /*MAARKERCOST*/, 1, true},
          /*  3 */ {7, 0, 8, shapeCost, MARKERCOST, 0, false},
          /*  4 */ {7, -1, 8, shapeCost, MARKERCOST, 0, false},
          /*  5 */ {7, -2, 8, shapeCost, MARKERCOST, 0, false},
          /*  6 */ {7, -3, 8, shapeCost, MARKERCOST, 0, false},
          /*  7 */ {7, -4, 8, shapeCost, MARKERCOST, 0, false},
          /*  8 */ {7, -5, 8, shapeCost, MARKERCOST, 0, false},
          /*  9 */ {7, -6, 8, shapeCost, MARKERCOST, 0, false},
          /* 10 */ {7, 0, 8, shapeCost * 2, MARKERCOST, 0, false},
          /* 11 */ {7, -1, 8, shapeCost * 2, MARKERCOST, 0, false},
          /* 12 */ {7, -2, 8, shapeCost * 2, MARKERCOST, 0, false},
          /* 13 */ {7, -3, 8, shapeCost * 2, MARKERCOST, 0, false},
          /* 14 */ {7, -4, 8, shapeCost * 2, MARKERCOST, 0, false},
          /* 15 */ {7, -5, 8, shapeCost * 2, MARKERCOST, 0, false},
          /* 16 */ {7, -6, 8, shapeCost * 2, MARKERCOST, 0, false},
          /* 17 */ {7, -3, 8, shapeCost, MARKERCOST, 1, false},
          /* 18 */ {7, 0, 8, shapeCost * 4, MARKERCOST, 0, false},
          /* 19 */ {7, -1, 8, shapeCost * 4, MARKERCOST, 0, false},
          /* 20 */ {7, -2, 8, shapeCost * 4, MARKERCOST, 0, false},
          /* 21 */ {7, -3, 8, shapeCost * 4, MARKERCOST, 0, false},
          /* 22 */ {7, -4, 8, shapeCost * 4, MARKERCOST, 0, false},
          /* 23 */ {7, -5, 8, shapeCost * 4, MARKERCOST, 0, false},
          /* 24 */ {7, -6, 8, shapeCost * 4, MARKERCOST, 0, false},
          /* 25 */ {5, -2, 8, shapeCost, MARKERCOST, 1, false},
          /* 26 */ {7, 0, 8, shapeCost * 8, MARKERCOST * 2, 0, false},
          /* 27 */ {7, -1, 8, shapeCost * 8, MARKERCOST * 2, 0, false},
          /* 28 */ {7, -2, 8, shapeCost * 8, MARKERCOST * 2, 0, false},
          /* 29 */ {7, -3, 8, shapeCost * 8, MARKERCOST * 2, 0, false},
          /* 30 */ {7, -4, 8, shapeCost * 8, MARKERCOST * 2, 0, false},
          /* 31 */ {7, -5, 8, shapeCost * 8, MARKERCOST * 2, 0, false},
          /* 32 */ {7, -6, 8, shapeCost * 8, MARKERCOST * 2, 0, false},
          /* 33 */ {3, -1, 8, shapeCost, MARKERCOST, 1, false},
          /* 34 */ {7, 0, 8, shapeCost * 16, MARKERCOST * 4, 0, false},
          /* 35 */ {7, -1, 8, shapeCost * 16, MARKERCOST * 4, 0, false},
          /* 36 */ {7, -2, 8, shapeCost * 16, MARKERCOST * 4, 0, false},
          /* 37 */ {7, -3, 8, shapeCost * 16, MARKERCOST * 4, 0, false},
          /* 38 */ {7, -4, 8, shapeCost * 16, MARKERCOST * 4, 0, false},
          /* 39 */ {7, -5, 8, shapeCost * 16, MARKERCOST * 4, 0, false},
          /* 40 */ {7, -6, 8, shapeCost * 16, MARKERCOST * 4, 0, false},
          /* 41 */ {3, -2, 8, shapeCost, MARKERCOST, 1, false},
          /* 42 */ {7, 0, 16, shapeCost * 16, MARKERCOST * 4, 0, false},
          /* 43 */ {7, -1, 16, shapeCost * 16, MARKERCOST * 4, 0, false},
          /* 44 */ {7, -2, 16, shapeCost * 16, MARKERCOST * 4, 0, false},
          /* 45 */ {7, -3, 16, shapeCost * 16, MARKERCOST * 4, 0, false},
          /* 46 */ {7, -4, 16, shapeCost * 16, MARKERCOST * 4, 0, false},
          /* 47 */ {7, -5, 16, shapeCost * 16, MARKERCOST * 4, 0, false},
          /* 48 */ {7, -6, 16, shapeCost * 16, MARKERCOST * 4, 0, false},
          /* 49 */ {3, -0, 8, shapeCost, MARKERCOST, 1, false},
          /* 50 */ {7, 0, 32, shapeCost * 32, MARKERCOST * 8, 0, false},
          /* 51 */ {7, -1, 32, shapeCost * 32, MARKERCOST * 8, 0, false},
          /* 52 */ {7, -2, 32, shapeCost * 32, MARKERCOST * 8, 0, false},
          /* 53 */ {7, -3, 32, shapeCost * 32, MARKERCOST * 8, 0, false},
          /* 54 */ {7, -4, 32, shapeCost * 32, MARKERCOST * 8, 0, false},
          /* 55 */ {7, -5, 32, shapeCost * 32, MARKERCOST * 8, 0, false},
          /* 56 */ {7, -6, 32, shapeCost * 32, MARKERCOST * 8, 0, false},
          /* 57 */ {3, -1, 8, shapeCost, MARKERCOST, 1, false},
          /* 58 */ {7, 0, 64, shapeCost * 64, MARKERCOST * 16, 0, false},
          /* 59 */ {7, -1, 64, shapeCost * 64, MARKERCOST * 16, 0, false},
          /* 60 */ {7, -2, 64, shapeCost * 64, MARKERCOST * 16, 0, false},
          /* 61 */ {7, -3, 64, shapeCost * 64, MARKERCOST * 16, 0, false},
          /* 62 */ {7, -4, 64, shapeCost * 64, MARKERCOST * 16, 0, false},
          /* 63 */ {7, -5, 64, shapeCost * 64, MARKERCOST * 16, 0, false},
          /* 64 */ {7, -6, 64, shapeCost * 64, MARKERCOST * 16, 0, false}};
}

void addRectToPolySet(gtl::polygon_90_set_data<frCoord>& polySet, Rect rect)
{
  using namespace boost::polygon::operators;
  gtl::polygon_90_data<frCoord> poly;
  vector<gtl::point_data<frCoord>> points;
  for (const auto& point : rect.getPoints()) {
    points.push_back({point.x(), point.y()});
  }
  poly.set(points.begin(), points.end());
  polySet += poly;
}

void FlexDR::reportGuideCoverage()
{
  using namespace boost::polygon::operators;

  const auto numLayers = getTech()->getLayers().size();
  std::vector<unsigned long long> totalAreaByLayerNum(numLayers, 0);
  std::vector<unsigned long long> totalCoveredAreaByLayerNum(numLayers, 0);
  map<frNet*, std::vector<float>> netsCoverage;
  const auto& nets = getDesign()->getTopBlock()->getNets();
  omp_set_num_threads(MAX_THREADS);
#pragma omp parallel for schedule(dynamic)
  for (int i = 0; i < nets.size(); i++) {
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
          || lNum > TOP_ROUTING_LAYER)
        continue;
      float coveredPercentage = -1.0;
      unsigned long long routingArea = 0;
      unsigned long long coveredArea = 0;
      if (!routeSetByLayerNum[lNum].empty()) {
        routingArea = gtl::area(routeSetByLayerNum[lNum]);
        coveredArea
            = gtl::area(routeSetByLayerNum[lNum] & guideSetByLayerNum[lNum]);
        if (routingArea == 0.0)
          coveredPercentage = -1.0;
        else
          coveredPercentage = (coveredArea / (double) routingArea) * 100;
      }

#pragma omp critical
      {
        netsCoverage[net.get()].push_back(coveredPercentage);
        totalAreaByLayerNum[lNum] += routingArea;
        totalCoveredAreaByLayerNum[lNum] += coveredArea;
      }
    }
  }

  ofstream file(GUIDE_REPORT_FILE);
  file << "Net,";
  for (const auto& layer : getTech()->getLayers()) {
    if (layer->getType() == dbTechLayerType::ROUTING
        && layer->getLayerNum() <= TOP_ROUTING_LAYER) {
      file << layer->getName() << ",";
    }
  }
  file << std::endl;
  for (auto [net, coverage] : netsCoverage) {
    file << net->getName() << ",";
    for (auto coveredPercentage : coverage)
      if (coveredPercentage < 0.0)
        file << "NA,";
      else
        file << fmt::format("{:.2f}%,", coveredPercentage);
    file << std::endl;
  }
  file << "Total,";
  unsigned long long totalArea = 0;
  unsigned long long totalCoveredArea = 0;
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
  if (totalArea == 0)
    file << "NA";
  else {
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

  for (auto& args : strategy()) {
    if (iter_ < 3)
      FIXEDSHAPECOST = ROUTESHAPECOST;
    else if (iter_ < 10)
      FIXEDSHAPECOST = 2 * ROUTESHAPECOST;
    else if (iter_ < 15)
      FIXEDSHAPECOST = 3 * ROUTESHAPECOST;
    else if (iter_ < 20)
      FIXEDSHAPECOST = 4 * ROUTESHAPECOST;
    else if (iter_ < 30)
      FIXEDSHAPECOST = 10 * ROUTESHAPECOST;
    else if (iter_ < 40)
      FIXEDSHAPECOST = 50 * ROUTESHAPECOST;
    else
      FIXEDSHAPECOST = 100 * ROUTESHAPECOST;

    if (iter_ == 40)
      MARKERDECAY = 0.99;
    if (iter_ == 50)
      MARKERDECAY = 0.999;

    int clipSize = args.size;
    if (args.ripupMode != 1) {
      if (increaseClipsize_) {
        clipSizeInc_ += 2;
      } else
        clipSizeInc_ = max((float) 0, clipSizeInc_ - 0.2f);
      clipSize += min(MAX_CLIPSIZE_INCREASE, (int) round(clipSizeInc_));
    }
    args.size = clipSize;

    searchRepair(args);
    if (getDesign()->getTopBlock()->getNumMarkers() == 0) {
      break;
    }
    if (iter_ > END_ITERATION) {
      break;
    }
    if (logger_->debugCheck(DRT, "snapshot", 1)) {
      io::Writer writer(getDesign(), logger_);
      writer.updateDb(db_);
      ord::OpenRoad::openRoad()->writeDb(
          fmt::format("drt_iter{}.odb", iter_ - 1).c_str());
    }
  }

  end(/* done */ true);
  if (!GUIDE_REPORT_FILE.empty())
    reportGuideCoverage();
  if (VERBOSE > 0) {
    t.print(logger_);
    cout << endl;
  }
  return 0;
}

void FlexDR::sendWorkers(
    const std::vector<std::pair<int, FlexDRWorker*>>& remote_batch,
    std::vector<std::unique_ptr<FlexDRWorker>>& batch)
{
  if (remote_batch.empty())
    return;
  std::vector<std::pair<int, std::string>> workers;
  {
    ProfileTask task("DIST: SERIALIZE_BATCH");
    for (auto& [idx, worker] : remote_batch) {
      std::string workerStr;
      serializeWorker(worker, workerStr);
      workers.push_back({idx, workerStr});
    }
  }
  std::string remote_ip = dist_ip_;
  unsigned short remote_port = dist_port_;
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
  (ar) & pinCnt_;
  (ar) & initNumMarkers_;
  (ar) & apSVia_;
  (ar) & planarHistoryMarkers_;
  (ar) & viaHistoryMarkers_;
  (ar) & historyMarkers_;
  (ar) & nets_;
  (ar) & gridGraph_;
  (ar) & markers_;
  (ar) & bestMarkers_;
  (ar) & isCongested_;
  if (is_loading(ar)) {
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
                                                 fr::frDesign* design,
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
