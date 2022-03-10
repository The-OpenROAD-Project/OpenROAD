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
#include "dr/FlexDR_conn.h"
#include "dr/FlexDR_graphics.h"
#include "dst/Distributed.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
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

static bool serialize_worker(SerializationType type,
                             FlexDRWorker* worker,
                             const std::string& name)
{
  if (type == SerializationType::READ) {
    std::ifstream file(name);
    if (!file.good())
      return false;
    InputArchive ar(file);
    register_types(ar);
    ar >> *worker;
    file.close();
  } else {
    std::ofstream file(name);
    if (!file.good())
      return false;
    OutputArchive ar(file);
    register_types(ar);
    ar << *worker;
    file.close();
  }
  return true;
}

static bool writeGlobals(const std::string& name)
{
  std::ofstream file(name);
  if (!file.good())
    return false;
  OutputArchive ar(file);
  register_types(ar);
  serialize_globals(ar);
  file.close();
  return true;
}

FlexDR::FlexDR(frDesign* designIn, Logger* loggerIn, odb::dbDatabase* dbIn)
    : design_(designIn), logger_(loggerIn), db_(dbIn), dist_on_(false)
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

// Used when reloaded a serialized worker.  init() will already have
// been run. We don't support end() in this case as it is intended for
// debugging route_queue().  The gc worker will have beeen reloaded
// from the serialization.
std::string FlexDRWorker::reloadedMain()
{
  route_queue();
  setGCWorker(nullptr);
  cleanup();
  std::string name = fmt::format("{}iter{}_x{}_y{}.worker.out",
                                 dist_dir_,
                                 getDRIter(),
                                 getGCellBox().xMin(),
                                 getGCellBox().yMin());
  serialize_worker(SerializationType::WRITE, this, name);
  return name;
  // reply with file path
}

int FlexDRWorker::main(frDesign* design)
{
  ProfileTask profile("DRW:main");
  using namespace std::chrono;
  high_resolution_clock::time_point t0 = high_resolution_clock::now();
  if (VERBOSE > 1) {
    logger_->report("start DR worker (BOX) ( {} {} ) ( {} {} )",
                    routeBox_.xMin() * 1.0 / getTech()->getDBUPerUU(),
                    routeBox_.yMin() * 1.0 / getTech()->getDBUPerUU(),
                    routeBox_.xMax() * 1.0 / getTech()->getDBUPerUU(),
                    routeBox_.yMax() * 1.0 / getTech()->getDBUPerUU());
  }

  init(design);
  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  if (!skipRouting_) {
    route_queue();
  }
  high_resolution_clock::time_point t2 = high_resolution_clock::now();
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

  return 0;
}
/**
 * Finds an equivalent object from the current design to obj. This is
 * because obj is a copy of the actual object that resides in design and
 * it is assumed that it has a different memory allocation.
 *
 * @param[in] obj pointer to the serialized object. Should be a copy of an
 * object in design.
 * @param[in] design pointer to frDesign.
 * @returns pointer to the equivalent frBlockObject in design. Returns nullptr
 * if no equivalent object was found or if the object is of an unexpected type.
 */
inline frBlockObject* getEquivalentObject(frBlockObject* obj, frDesign* design)
{
  switch (obj->typeId()) {
    case frcNet: {
      frNet* srNet = (static_cast<frNet*>(obj));
      auto net = design->getTopBlock()->findNet(srNet->getName());
      return net;
    }
    case frcInstTerm: {
      frInstTerm* srTerm = (static_cast<frInstTerm*>(obj));
      auto srInst = srTerm->getInst();
      auto inst = design->getTopBlock()->findInst(srInst->getName());
      if (inst == nullptr)
        return nullptr;
      for (auto& term : inst->getInstTerms())
        if (term->getId() == srTerm->getId())
          return term.get();
      return nullptr;
    }
    case frcBTerm: {
      auto bTerm = static_cast<frBTerm*>(obj);
      return design->getTopBlock()->getTerm(bTerm->getName());
    }
    case frcInstBlockage: {
      frInstBlockage* srBlkg = (static_cast<frInstBlockage*>(obj));
      auto srInst = srBlkg->getInst();
      auto inst = design->getTopBlock()->findInst(srInst->getName());
      if (inst == nullptr)
        return nullptr;
      for (auto& blkg : inst->getInstBlockages())
        if (blkg->getId() == srBlkg->getId())
          return blkg.get();
      return nullptr;
    }
    case frcBlockage: {
      frBlockage* srBlkg = (static_cast<frBlockage*>(obj));
      for (auto& blkg : design->getTopBlock()->getBlockages())
        if (blkg->getId() == srBlkg->getId())
          return blkg.get();
    }
    default:
      return nullptr;
  }
}
void FlexDRWorker::updateDesign(frDesign* design)
{
  design_ = design;
  for (auto& drNet_ : nets_) {
    auto frNet_ = design->getTopBlock()->findNet(drNet_->getFrNet()->getName());
    if (frNet_ != nullptr) {
      if (drNet_->getFrNet()->isModified())
        frNet_->setModified(true);
      drNet_->setFrNet(frNet_);
    } else {
      logger_->error(DRT,
                     501,
                     "No matching net in design for net {}",
                     drNet_->getFrNet()->getName());
    }
  }
  map<frNet*, set<pair<Point, frLayerNum>>, frBlockObjectComp> bp;
  for (auto [net, value] : boundaryPin_) {
    auto frNet_ = design->getTopBlock()->findNet(net->getName());
    if (frNet_ != nullptr) {
      bp[frNet_] = value;
    } else {
      logger_->error(
          DRT, 502, "No matching net in design for net {}", net->getName());
    }
  }
  setBoundaryPins(bp);
  std::vector<frMarker>& markers = getBestMarkers();
  for (auto& marker : markers) {
    std::set<frBlockObject*> newSrcs;
    for (auto src : marker.getSrcs()) {
      if (src != nullptr) {
        auto newSrc = getEquivalentObject(src, design);
        if (newSrc == nullptr)
          logger_->error(DRT, 503, "Fail in mapping serialized marker src");
        newSrcs.insert(newSrc);
      } else
        newSrcs.insert(nullptr);
    }
    marker.setSrcs(newSrcs);
    for (auto& [obj, value] : marker.getVictims()) {
      if (obj != nullptr) {
        auto newObj = getEquivalentObject(obj, design);
        if (newObj == nullptr)
          logger_->error(DRT, 504, "Fail in mapping serialized marker victim");
        obj = newObj;
      }
    }
    for (auto& [obj, value] : marker.getAggressors()) {
      if (obj != nullptr) {
        auto newObj = getEquivalentObject(obj, design);
        if (newObj == nullptr)
          logger_->error(
              DRT, 505, "Fail in mapping serialized marker aggressor");
        obj = newObj;
      }
    }
  }
}

void FlexDRWorker::distributedMain(frDesign* design, const char* globals_path)
{
  ProfileTask profile("DR:main");
  if (VERBOSE > 1) {
    logger_->report("start DR worker (BOX) ( {} {} ) ( {} {} )",
                    routeBox_.xMin() * 1.0 / getTech()->getDBUPerUU(),
                    routeBox_.yMin() * 1.0 / getTech()->getDBUPerUU(),
                    routeBox_.xMax() * 1.0 / getTech()->getDBUPerUU(),
                    routeBox_.yMax() * 1.0 / getTech()->getDBUPerUU());
  }

  init(design);
  if (skipRouting_)
    return;
  // Save the worker in its fully initialized state
  std::string name = fmt::format("{}iter{}_x{}_y{}.worker.in",
                                 dist_dir_,
                                 getDRIter(),
                                 getGCellBox().xMin(),
                                 getGCellBox().yMin());
  serialize_worker(SerializationType::WRITE, this, name);
  dst::JobMessage msg(dst::JobMessage::ROUTING), result(dst::JobMessage::NONE);
  std::unique_ptr<dst::JobDescription> desc
      = std::make_unique<RoutingJobDescription>(name, globals_path, dist_dir_);
  msg.setJobDescription(std::move(desc));
  bool ok = dist_->sendJob(msg, dist_ip_.c_str(), dist_port_, result);
  if (ok) {
    auto desc = static_cast<RoutingJobDescription*>(result.getJobDescription());
    ok = serialize_worker(SerializationType::READ, this, desc->getWorkerPath());
    if (!ok)
      logger_->error(DRT, 511, "Deserialization failed");
    std::remove(name.c_str());
    std::remove(desc->getWorkerPath().c_str());
    updateDesign(design);
  } else {
    logger_->error(utl::DRT, 500, "Sending worker {} failed", name);
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
          Point bp, ep;
          static_cast<frPathSeg*>(ps.get())->getPoints(bp, ep);
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
  Rect dieBox;
  getDesign()->getTopBlock()->getDieBox(dieBox);
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
          frLayerNum layerNum;
          Point bp, ep;
          ps->getPoints(bp, ep);
          layerNum = ps->getLayerNum();
          // skip TA dummy segment
          if (ep.x() - bp.x() + ep.y() - bp.y() == 1
              || ep.x() - bp.x() + ep.y() - bp.y() == 0) {
            continue;
          }
          Point idx1, idx2;
          getDesign()->getTopBlock()->getGCellIdx(bp, idx1);
          getDesign()->getTopBlock()->getGCellIdx(ep, idx2);
          // update gcell2BoundaryPin
          // horizontal
          if (bp.y() == ep.y()) {
            int x1 = idx1.x();
            int x2 = idx2.x();
            int y = idx1.y();
            for (auto x = x1; x <= x2; ++x) {
              Rect gcellBox;
              getDesign()->getTopBlock()->getGCellBox(Point(x, y), gcellBox);
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
              Rect gcellBox;
              getDesign()->getTopBlock()->getGCellBox(Point(x, y), gcellBox);
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
  Rect viaBox1, cutBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    via1.getLayer1BBox(viaBox1);
    isVia1Above = true;
  } else {
    via1.getLayer2BBox(viaBox1);
    isVia1Above = false;
  }
  via1.getCutBBox(cutBox1);
  int width1 = viaBox1.minDXDY();
  int length1 = viaBox1.maxDXDY();

  bool isVia2Above = false;
  frVia via2(viaDef2);
  Rect viaBox2, cutBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    via2.getLayer1BBox(viaBox2);
    isVia2Above = true;
  } else {
    via2.getLayer2BBox(viaBox2);
    isVia2Above = false;
  }
  via2.getCutBBox(cutBox2);
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
  Rect viaBox1, cutBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    via1.getLayer1BBox(viaBox1);
    isVia1Above = true;
  } else {
    via1.getLayer2BBox(viaBox1);
    isVia1Above = false;
  }
  via1.getCutBBox(cutBox1);
  int width1 = viaBox1.minDXDY();

  bool isVia2Above = false;
  frVia via2(viaDef2);
  Rect viaBox2, cutBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    via2.getLayer1BBox(viaBox2);
    isVia2Above = true;
  } else {
    via2.getLayer2BBox(viaBox2);
    isVia2Above = false;
  }
  via2.getCutBBox(cutBox2);
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
    via1.getLayer1BBox(viaBox1);
  } else {
    via1.getLayer2BBox(viaBox1);
  }
  auto width1 = viaBox1.minDXDY();
  bool isVia1Fat = isH ? (viaBox1.yMax() - viaBox1.yMin() > defaultWidth)
                       : (viaBox1.xMax() - viaBox1.xMin() > defaultWidth);
  auto prl1 = isH ? (viaBox1.yMax() - viaBox1.yMin())
                  : (viaBox1.xMax() - viaBox1.xMin());

  frVia via2(viaDef2);
  Rect viaBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    via2.getLayer1BBox(viaBox2);
  } else {
    via2.getLayer2BBox(viaBox2);
  }
  auto width2 = viaBox2.minDXDY();
  bool isVia2Fat = isH ? (viaBox2.yMax() - viaBox2.yMin() > defaultWidth)
                       : (viaBox2.xMax() - viaBox2.xMin() > defaultWidth);
  auto prl2 = isH ? (viaBox2.yMax() - viaBox2.yMin())
                  : (viaBox2.xMax() - viaBox2.xMin());

  frCoord reqDist = 0;
  if (isVia1Fat && isVia2Fat) {
    auto con = getTech()->getLayer(lNum)->getMinSpacing();
    if (con) {
      if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
        reqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
        reqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(
            max(width1, width2), min(prl1, prl2));
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
        reqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(
            width1, width2, min(prl1, prl2));
      }
    }
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
    via1.getLayer2BBox(viaBox1);
    lNum = lNum + 2;
  } else {
    via1.getLayer1BBox(viaBox1);
    lNum = lNum - 2;
  }
  width1 = viaBox1.minDXDY();
  prl1 = isH ? (viaBox1.yMax() - viaBox1.yMin())
             : (viaBox1.xMax() - viaBox1.xMin());
  reqDist = 0;
  auto con = getTech()->getLayer(lNum)->getMinSpacing();
  if (con) {
    if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
      reqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
    } else if (con->typeId()
               == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
      reqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(
          max(width1, width2), prl1);
    } else if (con->typeId()
               == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
      reqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(
          width1, width2, prl1);
    }
  }
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
  Rect viaBox1, cutBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    via1.getLayer1BBox(viaBox1);
    isVia1Above = true;
  } else {
    via1.getLayer2BBox(viaBox1);
    isVia1Above = false;
  }
  via1.getCutBBox(cutBox1);
  int width1 = viaBox1.minDXDY();
  int length1 = viaBox1.maxDXDY();

  bool isVia2Above = false;
  frVia via2(viaDef2);
  Rect viaBox2, cutBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    via2.getLayer1BBox(viaBox2);
    isVia2Above = true;
  } else {
    via2.getLayer2BBox(viaBox2);
    isVia2Above = false;
  }
  via2.getCutBBox(cutBox2);
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
    via1.getLayer1BBox(viaBox1);
  } else {
    via1.getLayer2BBox(viaBox1);
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
    via2.getLayer1BBox(viaBox2);
  } else {
    via2.getLayer2BBox(viaBox2);
  }
  auto width2 = viaBox2.minDXDY();
  bool isVia2Fat = isCurrDirX
                       ? (viaBox2.yMax() - viaBox2.yMin() > defaultWidth)
                       : (viaBox2.xMax() - viaBox2.xMin() > defaultWidth);
  auto prl2 = isCurrDirX ? (viaBox2.yMax() - viaBox2.yMin())
                         : (viaBox2.xMax() - viaBox2.xMin());

  frCoord reqDist = 0;
  if (isVia1Fat && isVia2Fat) {
    auto con = getTech()->getLayer(lNum)->getMinSpacing();
    if (con) {
      if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
        reqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
        reqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(
            max(width1, width2), min(prl1, prl2));
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
        reqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(
            width1, width2, min(prl1, prl2));
      }
    }
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
    via1.getLayer2BBox(viaBox1);
    lNum = lNum + 2;
  } else {
    via1.getLayer1BBox(viaBox1);
    lNum = lNum - 2;
  }
  width1 = viaBox1.minDXDY();
  prl1 = isCurrDirX ? (viaBox1.yMax() - viaBox1.yMin())
                    : (viaBox1.xMax() - viaBox1.xMin());
  reqDist = 0;
  auto con = getTech()->getLayer(lNum)->getMinSpacing();
  if (con) {
    if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
      reqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
    } else if (con->typeId()
               == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
      reqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(
          max(width1, width2), prl1);
    } else if (con->typeId()
               == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
      reqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(
          width1, width2, prl1);
    }
  }
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
  Rect viaBox1, cutBox1;
  if (viaDef1->getLayer1Num() == lNum) {
    via1.getLayer1BBox(viaBox1);
  } else {
    via1.getLayer2BBox(viaBox1);
  }
  via1.getCutBBox(cutBox1);

  frVia via2(viaDef2);
  Rect viaBox2, cutBox2;
  if (viaDef2->getLayer1Num() == lNum) {
    via2.getLayer1BBox(viaBox2);
  } else {
    via2.getLayer2BBox(viaBox2);
  }
  via2.getCutBBox(cutBox2);
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
      Rect layer1Box;
      Rect layer2Box;
      via.getLayer1BBox(layer1Box);
      via.getLayer2BBox(layer2Box);
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
    via1.getLayer1BBox(viaBox1);
  } else {
    via1.getLayer2BBox(viaBox1);
  }
  int width1 = viaBox1.minDXDY();
  bool isVia1Fat = isCurrDirX
                       ? (viaBox1.yMax() - viaBox1.yMin() > defaultWidth)
                       : (viaBox1.xMax() - viaBox1.xMin() > defaultWidth);
  auto prl1 = isCurrDirX ? (viaBox1.yMax() - viaBox1.yMin())
                         : (viaBox1.xMax() - viaBox1.xMin());

  frCoord reqDist = 0;
  if (isVia1Fat) {
    auto con = getTech()->getLayer(lNum)->getMinSpacing();
    if (con) {
      if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
        reqDist = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
        reqDist = static_cast<frSpacingTablePrlConstraint*>(con)->find(
            max(width1, defaultWidth), prl1);
      } else if (con->typeId()
                 == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
        reqDist = static_cast<frSpacingTableTwConstraint*>(con)->find(
            width1, defaultWidth, prl1);
      }
    }
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
    via1.getLayer1BBox(viaBox1);
  } else {
    via1.getLayer2BBox(viaBox1);
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

void FlexDR::searchRepair(int iter,
                          int size,
                          int offset,
                          int mazeEndIter,
                          frUInt4 workerDRCCost,
                          frUInt4 workerMarkerCost,
                          int ripupMode,
                          bool followGuide)
{
  std::string profile_name("DR:searchRepair");
  profile_name += std::to_string(iter);
  ProfileTask profile(profile_name.c_str());
  if (iter > END_ITERATION) {
    return;
  }
  if (ripupMode != 1 && getDesign()->getTopBlock()->getMarkers().size() == 0) {
    return;
  }
  if (iter < 3)
    FIXEDSHAPECOST = ROUTESHAPECOST;
  else if (iter < 10)
    FIXEDSHAPECOST = 2 * ROUTESHAPECOST;
  else if (iter < 15)
    FIXEDSHAPECOST = 3 * ROUTESHAPECOST;
  else if (iter < 20)
    FIXEDSHAPECOST = 4 * ROUTESHAPECOST;
  else if (iter < 30)
    FIXEDSHAPECOST = 10 * ROUTESHAPECOST;
  else if (iter < 40)
    FIXEDSHAPECOST = 50 * ROUTESHAPECOST;
  else
    FIXEDSHAPECOST = 100 * ROUTESHAPECOST;

  if (iter == 40)
    MARKERDECAY = 0.99;
  if (iter == 50)
    MARKERDECAY = 0.999;
  if (dist_on_) {
    if ((iter % 10 == 0 && iter != 60) || iter == 3 || iter == 15) {
      if (iter != 0)
        std::remove(globals_path_.c_str());
      globals_path_ = fmt::format("{}globals.{}.ar", dist_dir_, iter);
      writeGlobals(globals_path_);
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
  Rect dieBox;
  getDesign()->getTopBlock()->getDieBox(dieBox);
  auto gCellPatterns = getDesign()->getTopBlock()->getGCellPatterns();
  auto& xgp = gCellPatterns.at(0);
  auto& ygp = gCellPatterns.at(1);
  int clipSize = size;
  int cnt = 0;
  int tot = (((int) xgp.getCount() - 1 - offset) / clipSize + 1)
            * (((int) ygp.getCount() - 1 - offset) / clipSize + 1);
  int prev_perc = 0;
  bool isExceed = false;

  vector<unique_ptr<FlexDRWorker>> uworkers;
  int batchStepX, batchStepY;

  getBatchInfo(batchStepX, batchStepY);

  vector<vector<vector<unique_ptr<FlexDRWorker>>>> workers(batchStepX
                                                           * batchStepY);

  int xIdx = 0, yIdx = 0;
  for (int i = offset; i < (int) xgp.getCount(); i += clipSize) {
    for (int j = offset; j < (int) ygp.getCount(); j += clipSize) {
      auto worker = make_unique<FlexDRWorker>(&via_data_, design_, logger_);
      Rect routeBox1;
      getDesign()->getTopBlock()->getGCellBox(Point(i, j), routeBox1);
      Rect routeBox2;
      const int max_i = min((int) xgp.getCount() - 1, i + clipSize - 1);
      const int max_j = min((int) ygp.getCount(), j + clipSize - 1);
      getDesign()->getTopBlock()->getGCellBox(Point(max_i, max_j), routeBox2);
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
          || (int) workers[batchIdx].back().size() >= BATCHSIZE) {
        workers[batchIdx].push_back(vector<unique_ptr<FlexDRWorker>>());
      }
      workers[batchIdx].back().push_back(std::move(worker));

      yIdx++;
    }
    yIdx = 0;
    xIdx++;
  }

  omp_set_num_threads(MAX_THREADS);

  // parallel execution
  for (auto& workerBatch : workers) {
    ProfileTask profile("DR:checkerboard");
    for (auto& workersInBatch : workerBatch) {
      {
        const std::string batch_name = std::string("DR:batch<")
                                       + std::to_string(workersInBatch.size())
                                       + ">";
        ProfileTask profile(batch_name.c_str());
// multi thread
        ThreadException exception;
#pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < (int) workersInBatch.size(); i++) {
          try {
            if (dist_on_)
              workersInBatch[i]->distributedMain(getDesign(),
                                                 globals_path_.c_str());
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
                    logger_->report("    Completing {}% with {} violations.",
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
      }
      {
        ProfileTask profile("DR:end_batch");
        // single thread
        for (int i = 0; i < (int) workersInBatch.size(); i++) {
          workersInBatch[i]->end(getDesign());
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
  FlexDRConnectivityChecker checker(getDesign(), logger_, db_, graphics_.get());
  checker.check(iter);
  numViols_.push_back(getDesign()->getTopBlock()->getNumMarkers());
  if (VERBOSE > 0) {
    logger_->info(DRT,
                  199,
                  "  Number of violations = {}.",
                  getDesign()->getTopBlock()->getNumMarkers());
    t.print(logger_);
    cout << flush;
  }
  end();
  if (logger_->debugCheck(DRT, "drc", 1)) {
    reportDRC(DRC_RPT_FILE + '-' + std::to_string(iter) + ".rpt");
  }
}

void FlexDR::end(bool writeMetrics)
{
  using ULL = unsigned long long;
  vector<ULL> wlen(getTech()->getLayers().size(), 0);
  vector<ULL> sCut(getTech()->getLayers().size(), 0);
  vector<ULL> mCut(getTech()->getLayers().size(), 0);
  Point bp, ep;
  for (auto& net : getDesign()->getTopBlock()->getNets()) {
    for (auto& shape : net->getShapes()) {
      if (shape->typeId() == frcPathSeg) {
        auto obj = static_cast<frPathSeg*>(shape.get());
        obj->getPoints(bp, ep);
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

  if (writeMetrics) {
    logger_->metric("drt::wire length::total",
                    totWlen / getDesign()->getTopBlock()->getDBUPerUU());
    logger_->metric("drt::vias::total", totSCut + totMCut);
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

void FlexDR::reportDRC(const string& file_name)
{
  double dbu = getTech()->getDBUPerUU();

  if (file_name == string("")) {
    if (VERBOSE > 0) {
      logger_->warn(
          DRT,
          290,
          "Waring: no DRC report specified, skipped writing DRC report");
    }
    return;
  }

  ofstream drcRpt(file_name.c_str());
  if (drcRpt.is_open()) {
    for (auto& marker : getDesign()->getTopBlock()->getMarkers()) {
      auto con = marker->getConstraint();
      drcRpt << "  violation type: ";
      if (con) {
        switch (con->typeId()) {
          case frConstraintTypeEnum::frcShortConstraint: {
            if (getTech()->getLayer(marker->getLayerNum())->getType()
                == dbTechLayerType::ROUTING) {
              drcRpt << "Short";
            } else if (getTech()->getLayer(marker->getLayerNum())->getType()
                       == dbTechLayerType::CUT) {
              drcRpt << "CShort";
            }
            break;
          }
          case frConstraintTypeEnum::frcMinWidthConstraint:
            drcRpt << "MinWid";
            break;
          case frConstraintTypeEnum::frcSpacingConstraint:
            drcRpt << "MetSpc";
            break;
          case frConstraintTypeEnum::frcSpacingEndOfLineConstraint:
            drcRpt << "EOLSpc";
            break;
          case frConstraintTypeEnum::frcSpacingTablePrlConstraint:
            drcRpt << "MetSpc";
            break;
          case frConstraintTypeEnum::frcCutSpacingConstraint:
            drcRpt << "CutSpc";
            break;
          case frConstraintTypeEnum::frcMinStepConstraint:
            drcRpt << "MinStp";
            break;
          case frConstraintTypeEnum::frcNonSufficientMetalConstraint:
            drcRpt << "NSMet";
            break;
          case frConstraintTypeEnum::frcSpacingSamenetConstraint:
            drcRpt << "MetSpc";
            break;
          case frConstraintTypeEnum::frcOffGridConstraint:
            drcRpt << "OffGrid";
            break;
          case frConstraintTypeEnum::frcMinEnclosedAreaConstraint:
            drcRpt << "MinHole";
            break;
          case frConstraintTypeEnum::frcAreaConstraint:
            drcRpt << "MinArea";
            break;
          case frConstraintTypeEnum::frcLef58CornerSpacingConstraint:
            drcRpt << "CornerSpc";
            break;
          case frConstraintTypeEnum::frcLef58CutSpacingConstraint:
            drcRpt << "CutSpc";
            break;
          case frConstraintTypeEnum::frcLef58RectOnlyConstraint:
            drcRpt << "RectOnly";
            break;
          case frConstraintTypeEnum::frcLef58RightWayOnGridOnlyConstraint:
            drcRpt << "RightWayOnGridOnly";
            break;
          case frConstraintTypeEnum::frcLef58MinStepConstraint:
            drcRpt << "MinStp";
            break;
          case frConstraintTypeEnum::frcSpacingTableInfluenceConstraint:
            drcRpt << "MetSpcInf";
            break;
          case frConstraintTypeEnum::frcSpacingEndOfLineParallelEdgeConstraint:
            drcRpt << "SpacingEndOfLineParallelEdge";
            break;
          case frConstraintTypeEnum::frcSpacingTableConstraint:
            drcRpt << "SpacingTable";
            break;
          case frConstraintTypeEnum::frcSpacingTableTwConstraint:
            drcRpt << "SpacingTableTw";
            break;
          case frConstraintTypeEnum::frcLef58SpacingTableConstraint:
            drcRpt << "Lef58SpacingTable";
            break;
          case frConstraintTypeEnum::frcLef58CutSpacingTableConstraint:
            drcRpt << "Lef58CutSpacingTable";
            break;
          case frConstraintTypeEnum::frcLef58CutSpacingTablePrlConstraint:
            drcRpt << "Lef58CutSpacingTablePrl";
            break;
          case frConstraintTypeEnum::frcLef58CutSpacingTableLayerConstraint:
            drcRpt << "Lef58CutSpacingTableLayer";
            break;
          case frConstraintTypeEnum::frcLef58CutSpacingParallelWithinConstraint:
            drcRpt << "Lef58CutSpacingParallelWithin";
            break;
          case frConstraintTypeEnum::frcLef58CutSpacingAdjacentCutsConstraint:
            drcRpt << "Lef58CutSpacingAdjacentCuts";
            break;
          case frConstraintTypeEnum::frcLef58CutSpacingLayerConstraint:
            drcRpt << "Lef58CutSpacingLayer";
            break;
          case frConstraintTypeEnum::frcMinimumcutConstraint:
            drcRpt << "Minimumcut";
            break;
          case frConstraintTypeEnum::
              frcLef58CornerSpacingConcaveCornerConstraint:
            drcRpt << "Lef58CornerSpacingConcaveCorner";
            break;
          case frConstraintTypeEnum::
              frcLef58CornerSpacingConvexCornerConstraint:
            drcRpt << "Lef58CornerSpacingConvexCorner";
            break;
          case frConstraintTypeEnum::frcLef58CornerSpacingSpacingConstraint:
            drcRpt << "Lef58CornerSpacingSpacing";
            break;
          case frConstraintTypeEnum::frcLef58CornerSpacingSpacing1DConstraint:
            drcRpt << "Lef58CornerSpacingSpacing1D";
            break;
          case frConstraintTypeEnum::frcLef58CornerSpacingSpacing2DConstraint:
            drcRpt << "Lef58CornerSpacingSpacing2D";
            break;
          case frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint:
            drcRpt << "Lef58SpacingEndOfLine";
            break;
          case frConstraintTypeEnum::frcLef58SpacingEndOfLineWithinConstraint:
            drcRpt << "Lef58SpacingEndOfLineWithin";
            break;
          case frConstraintTypeEnum::
              frcLef58SpacingEndOfLineWithinEndToEndConstraint:
            drcRpt << "Lef58SpacingEndOfLineWithinEndToEnd";
            break;
          case frConstraintTypeEnum::
              frcLef58SpacingEndOfLineWithinEncloseCutConstraint:
            drcRpt << "Lef58SpacingEndOfLineWithinEncloseCut";
            break;
          case frConstraintTypeEnum::
              frcLef58SpacingEndOfLineWithinParallelEdgeConstraint:
            drcRpt << "Lef58SpacingEndOfLineWithinParallelEdge";
            break;
          case frConstraintTypeEnum::
              frcLef58SpacingEndOfLineWithinMaxMinLengthConstraint:
            drcRpt << "Lef58SpacingEndOfLineWithinMaxMinLength";
            break;
          case frConstraintTypeEnum::frcLef58CutClassConstraint:
            drcRpt << "Lef58CutClass";
            break;
          case frConstraintTypeEnum::frcRecheckConstraint:
            drcRpt << "Recheck";
            break;
          case frConstraintTypeEnum::frcLef58EolExtensionConstraint:
            drcRpt << "Lef58EolExtension";
            break;
          case frConstraintTypeEnum::frcLef58EolKeepOutConstraint:
            drcRpt << "Lef58EolKeepOut";
            break;
        }
      } else {
        drcRpt << "nullptr";
      }
      drcRpt << endl;
      // get source(s) of violation
      // format: type:name/identifier
      drcRpt << "    srcs: ";
      for (auto src : marker->getSrcs()) {
        if (src) {
          switch (src->typeId()) {
            case frcNet:
              drcRpt << "net:" << (static_cast<frNet*>(src))->getName() << " ";
              break;
            case frcInstTerm: {
              frInstTerm* instTerm = (static_cast<frInstTerm*>(src));
              drcRpt << "iterm:" << instTerm->getInst()->getName() << "/"
                     << instTerm->getTerm()->getName() << " ";
              break;
            }
            case frcBTerm: {
              frBTerm* bterm = (static_cast<frBTerm*>(src));
              drcRpt << "bterm:" << bterm->getName() << " ";
              break;
            }
            case frcInstBlockage: {
              frInstBlockage* instBlockage
                  = (static_cast<frInstBlockage*>(src));
              drcRpt << "inst:" << instBlockage->getInst()->getName() << " ";
              break;
            }
            case frcBlockage: {
              drcRpt << "obstruction: ";
              break;
            }
            default:
              logger_->error(DRT,
                             291,
                             "Unexpected source type in marker: {}",
                             src->typeId());
          }
        }
      }
      drcRpt << "\n";
      // get violation bbox
      Rect bbox;
      marker->getBBox(bbox);
      drcRpt << "    bbox = ( " << bbox.xMin() / dbu << ", "
             << bbox.yMin() / dbu << " ) - ( " << bbox.xMax() / dbu << ", "
             << bbox.yMax() / dbu << " ) on Layer ";
      if (getTech()->getLayer(marker->getLayerNum())->getType()
              == dbTechLayerType::CUT
          && marker->getLayerNum() - 1 >= getTech()->getBottomLayerNum()) {
        drcRpt << getTech()->getLayer(marker->getLayerNum() - 1)->getName()
               << "\n";
      } else {
        drcRpt << getTech()->getLayer(marker->getLayerNum())->getName() << "\n";
      }
    }
  } else {
    cout << "Error: Fail to open DRC report file\n";
  }
}

int FlexDR::main()
{
  ProfileTask profile("DR:main");
  init();
  frTime t;
  if (VERBOSE > 0) {
    logger_->info(DRT, 194, "Start detail routing.");
  }

  int iterNum = 0;
  searchRepair(
      iterNum++ /*  0 */, 7, 0, 3, ROUTESHAPECOST, 0 /*MAARKERCOST*/, 1, true);
  searchRepair(iterNum++ /*  1 */,
               7,
               -2,
               3,
               ROUTESHAPECOST,
               ROUTESHAPECOST /*MAARKERCOST*/,
               1,
               true);
  searchRepair(iterNum++ /*  2 */,
               7,
               -5,
               3,
               ROUTESHAPECOST,
               ROUTESHAPECOST /*MAARKERCOST*/,
               1,
               true);
  searchRepair(
      iterNum++ /*  3 */, 7, 0, 8, ROUTESHAPECOST, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /*  4 */, 7, -1, 8, ROUTESHAPECOST, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /*  5 */, 7, -2, 8, ROUTESHAPECOST, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /*  6 */, 7, -3, 8, ROUTESHAPECOST, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /*  7 */, 7, -4, 8, ROUTESHAPECOST, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /*  8 */, 7, -5, 8, ROUTESHAPECOST, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /*  9 */, 7, -6, 8, ROUTESHAPECOST, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /* 10 */, 7, 0, 8, ROUTESHAPECOST * 2, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /* 11 */, 7, -1, 8, ROUTESHAPECOST * 2, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /* 12 */, 7, -2, 8, ROUTESHAPECOST * 2, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /* 13 */, 7, -3, 8, ROUTESHAPECOST * 2, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /* 14 */, 7, -4, 8, ROUTESHAPECOST * 2, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /* 15 */, 7, -5, 8, ROUTESHAPECOST * 2, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /* 16 */, 7, -6, 8, ROUTESHAPECOST * 2, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /* 17 - ra'*/, 7, -3, 8, ROUTESHAPECOST, MARKERCOST, 1, false);
  searchRepair(
      iterNum++ /* 18 */, 7, 0, 8, ROUTESHAPECOST * 4, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /* 19 */, 7, -1, 8, ROUTESHAPECOST * 4, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /* 20 */, 7, -2, 8, ROUTESHAPECOST * 4, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /* 21 */, 7, -3, 8, ROUTESHAPECOST * 4, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /* 22 */, 7, -4, 8, ROUTESHAPECOST * 4, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /* 23 */, 7, -5, 8, ROUTESHAPECOST * 4, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /* 24 */, 7, -6, 8, ROUTESHAPECOST * 4, MARKERCOST, 0, false);
  searchRepair(
      iterNum++ /* 25 - ra'*/, 5, -2, 8, ROUTESHAPECOST, MARKERCOST, 1, false);
  searchRepair(iterNum++ /* 26 */,
               7,
               0,
               8,
               ROUTESHAPECOST * 8,
               MARKERCOST * 2,
               0,
               false);
  searchRepair(iterNum++ /* 27 */,
               7,
               -1,
               8,
               ROUTESHAPECOST * 8,
               MARKERCOST * 2,
               0,
               false);
  searchRepair(iterNum++ /* 28 */,
               7,
               -2,
               8,
               ROUTESHAPECOST * 8,
               MARKERCOST * 2,
               0,
               false);
  searchRepair(iterNum++ /* 29 */,
               7,
               -3,
               8,
               ROUTESHAPECOST * 8,
               MARKERCOST * 2,
               0,
               false);
  searchRepair(iterNum++ /* 30 */,
               7,
               -4,
               8,
               ROUTESHAPECOST * 8,
               MARKERCOST * 2,
               0,
               false);
  searchRepair(iterNum++ /* 31 */,
               7,
               -5,
               8,
               ROUTESHAPECOST * 8,
               MARKERCOST * 2,
               0,
               false);
  searchRepair(iterNum++ /* 32 */,
               7,
               -6,
               8,
               ROUTESHAPECOST * 8,
               MARKERCOST * 2,
               0,
               false);
  searchRepair(
      iterNum++ /* 33 - ra'*/, 3, -1, 8, ROUTESHAPECOST, MARKERCOST, 1, false);
  searchRepair(iterNum++ /* 34 */,
               7,
               0,
               8,
               ROUTESHAPECOST * 16,
               MARKERCOST * 4,
               0,
               false);
  searchRepair(iterNum++ /* 35 */,
               7,
               -1,
               8,
               ROUTESHAPECOST * 16,
               MARKERCOST * 4,
               0,
               false);
  searchRepair(iterNum++ /* 36 */,
               7,
               -2,
               8,
               ROUTESHAPECOST * 16,
               MARKERCOST * 4,
               0,
               false);
  searchRepair(iterNum++ /* 37 */,
               7,
               -3,
               8,
               ROUTESHAPECOST * 16,
               MARKERCOST * 4,
               0,
               false);
  searchRepair(iterNum++ /* 38 */,
               7,
               -4,
               8,
               ROUTESHAPECOST * 16,
               MARKERCOST * 4,
               0,
               false);
  searchRepair(iterNum++ /* 39 */,
               7,
               -5,
               8,
               ROUTESHAPECOST * 16,
               MARKERCOST * 4,
               0,
               false);
  searchRepair(iterNum++ /* 40 */,
               7,
               -6,
               8,
               ROUTESHAPECOST * 16,
               MARKERCOST * 4,
               0,
               false);
  searchRepair(
      iterNum++ /* 41 - ra'*/, 3, -2, 8, ROUTESHAPECOST, MARKERCOST, 1, false);
  searchRepair(iterNum++ /* 42 */,
               7,
               0,
               16,
               ROUTESHAPECOST * 16,
               MARKERCOST * 4,
               0,
               false);
  searchRepair(iterNum++ /* 43 */,
               7,
               -1,
               16,
               ROUTESHAPECOST * 16,
               MARKERCOST * 4,
               0,
               false);
  searchRepair(iterNum++ /* 44 */,
               7,
               -2,
               16,
               ROUTESHAPECOST * 16,
               MARKERCOST * 4,
               0,
               false);
  searchRepair(iterNum++ /* 45 */,
               7,
               -3,
               16,
               ROUTESHAPECOST * 16,
               MARKERCOST * 4,
               0,
               false);
  searchRepair(iterNum++ /* 46 */,
               7,
               -4,
               16,
               ROUTESHAPECOST * 16,
               MARKERCOST * 4,
               0,
               false);
  searchRepair(iterNum++ /* 47 */,
               7,
               -5,
               16,
               ROUTESHAPECOST * 16,
               MARKERCOST * 4,
               0,
               false);
  searchRepair(iterNum++ /* 48 */,
               7,
               -6,
               16,
               ROUTESHAPECOST * 16,
               MARKERCOST * 4,
               0,
               false);
  searchRepair(
      iterNum++ /* 49 - ra'*/, 3, -0, 8, ROUTESHAPECOST, MARKERCOST, 1, false);
  searchRepair(iterNum++ /* 50 */,
               7,
               0,
               32,
               ROUTESHAPECOST * 32,
               MARKERCOST * 8,
               0,
               false);
  searchRepair(iterNum++ /* 51 */,
               7,
               -1,
               32,
               ROUTESHAPECOST * 32,
               MARKERCOST * 8,
               0,
               false);
  searchRepair(iterNum++ /* 52 */,
               7,
               -2,
               32,
               ROUTESHAPECOST * 32,
               MARKERCOST * 8,
               0,
               false);
  searchRepair(iterNum++ /* 53 */,
               7,
               -3,
               32,
               ROUTESHAPECOST * 32,
               MARKERCOST * 8,
               0,
               false);
  searchRepair(iterNum++ /* 54 */,
               7,
               -4,
               32,
               ROUTESHAPECOST * 32,
               MARKERCOST * 8,
               0,
               false);
  searchRepair(iterNum++ /* 55 */,
               7,
               -5,
               32,
               ROUTESHAPECOST * 32,
               MARKERCOST * 8,
               0,
               false);
  searchRepair(iterNum++ /* 56 */,
               7,
               -6,
               32,
               ROUTESHAPECOST * 32,
               MARKERCOST * 8,
               0,
               false);
  searchRepair(
      iterNum++ /* 57 - ra'*/, 3, -1, 8, ROUTESHAPECOST, MARKERCOST, 1, false);
  searchRepair(iterNum++ /* 58 */,
               7,
               0,
               64,
               ROUTESHAPECOST * 64,
               MARKERCOST * 16,
               0,
               false);
  searchRepair(iterNum++ /* 59 */,
               7,
               -1,
               64,
               ROUTESHAPECOST * 64,
               MARKERCOST * 16,
               0,
               false);
  searchRepair(iterNum++ /* 60 */,
               7,
               -2,
               64,
               ROUTESHAPECOST * 64,
               MARKERCOST * 16,
               0,
               false);
  searchRepair(iterNum++ /* 61 */,
               7,
               -3,
               64,
               ROUTESHAPECOST * 64,
               MARKERCOST * 16,
               0,
               false);
  searchRepair(iterNum++ /* 56 */,
               7,
               -4,
               64,
               ROUTESHAPECOST * 64,
               MARKERCOST * 16,
               0,
               false);
  searchRepair(iterNum++ /* 62 */,
               7,
               -5,
               64,
               ROUTESHAPECOST * 64,
               MARKERCOST * 16,
               0,
               false);
  searchRepair(iterNum++ /* 63 */,
               7,
               -6,
               64,
               ROUTESHAPECOST * 64,
               MARKERCOST * 16,
               0,
               false);

  if (DRC_RPT_FILE != string("")) {
    reportDRC(DRC_RPT_FILE);
  }
  if (VERBOSE > 0) {
    logger_->info(DRT, 198, "Complete detail routing.");
    end(/* writeMetrics */ true);
  }
  if (VERBOSE > 0) {
    t.print(logger_);
    cout << endl;
  }
  return 0;
}

std::unique_ptr<FlexDRWorker> FlexDRWorker::load(const std::string& file_name,
                                                 utl::Logger* logger,
                                                 FlexDRGraphics* graphics)
{
  auto worker = std::make_unique<FlexDRWorker>();
  serialize_worker(SerializationType::READ, worker.get(), file_name);

  // We need to fix up the fields we want from the current run rather
  // than the stored ones.
  worker->setLogger(logger);
  worker->setGraphics(graphics);

  return worker;
}
