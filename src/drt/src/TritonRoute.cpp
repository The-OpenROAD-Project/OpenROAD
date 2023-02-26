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

#include "triton_route/TritonRoute.h"

#include <boost/asio/post.hpp>
#include <boost/bind/bind.hpp>
#include <fstream>
#include <iostream>

#include "DesignCallBack.h"
#include "db/tech/frTechObject.h"
#include "distributed/RoutingCallBack.h"
#include "distributed/drUpdate.h"
#include "distributed/frArchive.h"
#include "dr/FlexDR.h"
#include "dr/FlexDR_graphics.h"
#include "dst/Distributed.h"
#include "frDesign.h"
#include "frProfileTask.h"
#include "gc/FlexGC.h"
#include "global.h"
#include "gr/FlexGR.h"
#include "gui/gui.h"
#include "io/io.h"
#include "odb/dbShape.h"
#include "ord/OpenRoad.hh"
#include "pa/FlexPA.h"
#include "rp/FlexRP.h"
#include "serialization.h"
#include "sta/StaMain.hh"
#include "stt/SteinerTreeBuilder.h"
#include "ta/FlexTA.h"
using namespace std;
using namespace fr;
using namespace triton_route;

namespace sta {
// Tcl files encoded into strings.
extern const char* drt_tcl_inits[];
}  // namespace sta

extern "C" {
extern int Drt_Init(Tcl_Interp* interp);
}

TritonRoute::TritonRoute()
    : debug_(std::make_unique<frDebugSettings>()),
      db_callback_(std::make_unique<DesignCallBack>(this)),
      db_(nullptr),
      logger_(nullptr),
      stt_builder_(nullptr),
      num_drvs_(-1),
      gui_(gui::Gui::get()),
      dist_(nullptr),
      distributed_(false),
      dist_port_(0),
      results_sz_(0),
      cloud_sz_(0),
      dist_pool_(1)
{
}

TritonRoute::~TritonRoute()
{
}

void TritonRoute::setDebugDR(bool on)
{
  debug_->debugDR = on;
}

void TritonRoute::setDebugDumpDR(bool on, const std::string& dumpDir)
{
  debug_->debugDumpDR = on;
  debug_->dumpDir = dumpDir;
}

void TritonRoute::setDebugMaze(bool on)
{
  debug_->debugMaze = on;
}

void TritonRoute::setDebugPA(bool on)
{
  debug_->debugPA = on;
}

void TritonRoute::setDebugTA(bool on)
{
  debug_->debugTA = on;
}

void TritonRoute::setDistributed(bool on)
{
  distributed_ = on;
}

void TritonRoute::setWorkerIpPort(const char* ip, unsigned short port)
{
  dist_ip_ = ip;
  dist_port_ = port;
}

void TritonRoute::setSharedVolume(const std::string& vol)
{
  shared_volume_ = vol;
  if (!shared_volume_.empty() && shared_volume_.back() != '/') {
    shared_volume_ += '/';
  }
}

void TritonRoute::setDebugNetName(const char* name)
{
  debug_->netName = name;
}

void TritonRoute::setDebugPinName(const char* name)
{
  debug_->pinName = name;
}

void TritonRoute::setDebugWorker(int x, int y)
{
  debug_->x = x;
  debug_->y = y;
}

void TritonRoute::setDebugIter(int iter)
{
  debug_->iter = iter;
}

void TritonRoute::setDebugPaMarkers(bool on)
{
  debug_->paMarkers = on;
}

void TritonRoute::setDebugPaEdge(bool on)
{
  debug_->paEdge = on;
}

void TritonRoute::setDebugPaCommit(bool on)
{
  debug_->paCommit = on;
}

void TritonRoute::setDebugWorkerParams(int mazeEndIter,
                                       int drcCost,
                                       int markerCost,
                                       int fixedShapeCost,
                                       float markerDecay,
                                       int ripupMode,
                                       int followGuide)
{
  debug_->mazeEndIter = mazeEndIter;
  debug_->drcCost = drcCost;
  debug_->markerCost = markerCost;
  debug_->fixedShapeCost = fixedShapeCost;
  debug_->markerDecay = markerDecay;
  debug_->ripupMode = ripupMode;
  debug_->followGuide = followGuide;
}

int TritonRoute::getNumDRVs() const
{
  if (num_drvs_ < 0) {
    logger_->error(DRT, 2, "Detailed routing has not been run yet.");
  }
  return num_drvs_;
}

std::string TritonRoute::runDRWorker(const std::string& workerStr,
                                     FlexDRViaData* viaData)
{
  bool on = debug_->debugDR;
  std::unique_ptr<FlexDRGraphics> graphics_
      = on && FlexDRGraphics::guiActive() ? std::make_unique<FlexDRGraphics>(
            debug_.get(), design_.get(), db_, logger_)
                                          : nullptr;
  auto worker
      = FlexDRWorker::load(workerStr, logger_, design_.get(), graphics_.get());
  worker->setViaData(viaData);
  worker->setSharedVolume(shared_volume_);
  worker->setDebugSettings(debug_.get());
  if (graphics_)
    graphics_->startIter(worker->getDRIter());
  std::string result = worker->reloadedMain();
  return result;
}

void TritonRoute::debugSingleWorker(const std::string& dumpDir,
                                    const std::string& drcRpt)
{
  bool on = debug_->debugDR;
  FlexDRViaData viaData;
  std::ifstream viaDataFile(fmt::format("{}/viadata.bin", dumpDir),
                            std::ios::binary);
  frIArchive ar(viaDataFile);
  ar >> viaData;

  std::unique_ptr<FlexDRGraphics> graphics_
      = on && FlexDRGraphics::guiActive() ? std::make_unique<FlexDRGraphics>(
            debug_.get(), design_.get(), db_, logger_)
                                          : nullptr;
  std::ifstream workerFile(fmt::format("{}/worker.bin", dumpDir),
                           std::ios::binary);
  std::string workerStr((std::istreambuf_iterator<char>(workerFile)),
                        std::istreambuf_iterator<char>());
  workerFile.close();
  auto worker
      = FlexDRWorker::load(workerStr, logger_, design_.get(), graphics_.get());
  if (debug_->mazeEndIter != -1)
    worker->setMazeEndIter(debug_->mazeEndIter);
  if (debug_->markerCost != -1)
    worker->setMarkerCost(debug_->markerCost);
  if (debug_->drcCost != -1)
    worker->setDrcCost(debug_->drcCost);
  if (debug_->fixedShapeCost != -1)
    worker->setFixedShapeCost(debug_->fixedShapeCost);
  if (debug_->markerDecay != -1)
    worker->setMarkerDecay(debug_->markerDecay);
  if (debug_->ripupMode != -1)
    worker->setRipupMode(debug_->ripupMode);
  if (debug_->followGuide != -1)
    worker->setFollowGuide((debug_->followGuide == 1));
  worker->setSharedVolume(shared_volume_);
  worker->setDebugSettings(debug_.get());
  worker->setViaData(&viaData);
  if (graphics_)
    graphics_->startIter(worker->getDRIter());
  std::string result = worker->reloadedMain();
  bool updated = worker->end(design_.get());
  debugPrint(logger_,
             utl::DRT,
             "autotuner",
             1,
             "End number of markers {}. Updated={}",
             worker->getBestNumMarkers(),
             updated);
  if (updated && !drcRpt.empty())
    reportDRC(
        drcRpt, design_->getTopBlock()->getMarkers(), worker->getDrcBox());
}

void TritonRoute::updateGlobals(const char* file_name)
{
  std::ifstream file(file_name);
  if (!file.good())
    return;
  frIArchive ar(file);
  registerTypes(ar);
  serializeGlobals(ar);
  file.close();
}

void TritonRoute::resetDb(const char* file_name)
{
  design_ = std::make_unique<frDesign>(logger_);
  ord::OpenRoad::openRoad()->readDb(file_name);
  initDesign();
  initGuide();
  prep();
  design_->getRegionQuery()->initDRObj();
}

void TritonRoute::clearDesign()
{
  design_ = std::make_unique<frDesign>(logger_);
}

static void deserializeUpdate(frDesign* design,
                              const std::string& updateStr,
                              std::vector<drUpdate>& updates)
{
  std::ifstream file(updateStr.c_str());
  frIArchive ar(file);
  ar.setDesign(design);
  registerTypes(ar);
  ar >> updates;
  file.close();
}

static void deserializeUpdates(frDesign* design,
                               const std::string& updateStr,
                               std::vector<std::vector<drUpdate>>& updates)
{
  std::ifstream file(updateStr.c_str());
  frIArchive ar(file);
  ar.setDesign(design);
  registerTypes(ar);
  ar >> updates;
  file.close();
}

void TritonRoute::updateDesign(const std::vector<std::string>& updatesStrs)
{
  omp_set_num_threads(ord::OpenRoad::openRoad()->getThreadCount());
  std::vector<std::vector<drUpdate>> updates(updatesStrs.size());
#pragma omp parallel for schedule(dynamic)
  for (int i = 0; i < updatesStrs.size(); i++) {
    deserializeUpdate(design_.get(), updatesStrs.at(i), updates[i]);
  }
  applyUpdates(updates);
}

void TritonRoute::updateDesign(const std::string& path)
{
  omp_set_num_threads(ord::OpenRoad::openRoad()->getThreadCount());
  std::vector<std::vector<drUpdate>> updates;
  deserializeUpdates(design_.get(), path, updates);
  applyUpdates(updates);
}

void TritonRoute::applyUpdates(
    const std::vector<std::vector<drUpdate>>& updates)
{
  auto topBlock = design_->getTopBlock();
  auto regionQuery = design_->getRegionQuery();
  const auto maxSz = updates[0].size();
  for (int j = 0; j < maxSz; j++) {
    for (int i = 0; i < updates.size(); i++) {
      if (updates[i].size() <= j)
        continue;
      const auto& update = updates[i][j];
      switch (update.getType()) {
        case drUpdate::REMOVE_FROM_BLOCK: {
          auto id = update.getIndexInOwner();
          auto marker = design_->getTopBlock()->getMarker(id);
          regionQuery->removeMarker(marker);
          topBlock->removeMarker(marker);
          break;
        }
        case drUpdate::REMOVE_FROM_NET:
        case drUpdate::REMOVE_FROM_RQ: {
          auto net = update.getNet();
          auto id = update.getIndexInOwner();
          auto pinfig = net->getPinFig(id);
          switch (pinfig->typeId()) {
            case frcPathSeg: {
              auto seg = static_cast<frPathSeg*>(pinfig);
              regionQuery->removeDRObj(seg);
              if (update.getType() == drUpdate::REMOVE_FROM_NET)
                net->removeShape(seg);
              break;
            }
            case frcPatchWire: {
              auto pwire = static_cast<frPatchWire*>(pinfig);
              regionQuery->removeDRObj(pwire);
              if (update.getType() == drUpdate::REMOVE_FROM_NET)
                net->removePatchWire(pwire);
              break;
            }
            case frcVia: {
              auto via = static_cast<frVia*>(pinfig);
              regionQuery->removeDRObj(via);
              if (update.getType() == drUpdate::REMOVE_FROM_NET)
                net->removeVia(via);
              break;
            }
            default:
              logger_->error(
                  DRT, 9999, "unknown update type {}", pinfig->typeId());
              break;
          }
          break;
        }
        case drUpdate::ADD_SHAPE:
        case drUpdate::ADD_SHAPE_NET_ONLY: {
          switch (update.getObjTypeId()) {
            case frcPathSeg: {
              auto net = update.getNet();
              frPathSeg seg = update.getPathSeg();
              std::unique_ptr<frShape> uShape
                  = std::make_unique<frPathSeg>(seg);
              auto sptr = uShape.get();
              net->addShape(std::move(uShape));
              if (update.getType() == drUpdate::ADD_SHAPE)
                regionQuery->addDRObj(sptr);
              break;
            }
            case frcPatchWire: {
              auto net = update.getNet();
              frPatchWire pwire = update.getPatchWire();
              std::unique_ptr<frShape> uShape
                  = std::make_unique<frPatchWire>(pwire);
              auto sptr = uShape.get();
              net->addPatchWire(std::move(uShape));
              if (update.getType() == drUpdate::ADD_SHAPE)
                regionQuery->addDRObj(sptr);
              break;
            }
            case frcVia: {
              auto net = update.getNet();
              frVia via = update.getVia();
              auto uVia = std::make_unique<frVia>(via);
              auto sptr = uVia.get();
              net->addVia(std::move(uVia));
              if (update.getType() == drUpdate::ADD_SHAPE)
                regionQuery->addDRObj(sptr);
              break;
            }
            default: {
              frMarker marker = update.getMarker();
              auto uMarker = std::make_unique<frMarker>(marker);
              auto sptr = uMarker.get();
              topBlock->addMarker(std::move(uMarker));
              regionQuery->addMarker(sptr);
              break;
            }
          }
          break;
        }
        case drUpdate::ADD_GUIDE: {
          frPathSeg seg = update.getPathSeg();
          std::unique_ptr<frPathSeg> uSeg = std::make_unique<frPathSeg>(seg);
          auto net = update.getNet();
          uSeg->addToNet(net);
          vector<unique_ptr<frConnFig>> tmp;
          tmp.push_back(std::move(uSeg));
          auto idx = update.getIndexInOwner();
          if (idx < 0 || idx >= net->getGuides().size())
            logger_->error(DRT,
                           9199,
                           "Guide {} out of range {}",
                           idx,
                           net->getGuides().size());
          const auto& guide = net->getGuides().at(idx);
          guide->setRoutes(tmp);
          break;
        }
        case drUpdate::UPDATE_SHAPE: {
          auto net = update.getNet();
          auto id = update.getIndexInOwner();
          auto pinfig = net->getPinFig(id);
          switch (pinfig->typeId()) {
            case frcPathSeg: {
              auto seg = static_cast<frPathSeg*>(pinfig);
              frPathSeg updatedSeg = update.getPathSeg();
              seg->setPoints(updatedSeg.getBeginPoint(),
                             updatedSeg.getEndPoint());
              frSegStyle style = updatedSeg.getStyle();
              seg->setStyle(style);
              regionQuery->addDRObj(seg);
              break;
            }
            default:
              break;
          }
        }
      }
    }
  }
}

void TritonRoute::init(Tcl_Interp* tcl_interp,
                       odb::dbDatabase* db,
                       Logger* logger,
                       dst::Distributed* dist,
                       stt::SteinerTreeBuilder* stt_builder)
{
  db_ = db;
  logger_ = logger;
  dist_ = dist;
  stt_builder_ = stt_builder;
  design_ = std::make_unique<frDesign>(logger_);
  dist->addCallBack(new fr::RoutingCallBack(this, dist, logger));
  // Define swig TCL commands.
  Drt_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::drt_tcl_inits);
  FlexDRGraphics::init();
}

bool TritonRoute::initGuide()
{
  if (DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB")
    USENONPREFTRACKS = false;
  io::Parser parser(db_, getDesign(), logger_);
  bool guideOk = parser.readGuide();
  parser.postProcessGuide();
  parser.initRPin();
  return guideOk;
}
void TritonRoute::initDesign()
{
  if (getDesign()->getTopBlock() != nullptr) {
    getDesign()->getTopBlock()->removeDeletedInsts();
    return;
  }
  io::Parser parser(db_, getDesign(), logger_);
  parser.readDb();
  auto tech = getDesign()->getTech();
  if (!BOTTOM_ROUTING_LAYER_NAME.empty()) {
    frLayer* layer = tech->getLayer(BOTTOM_ROUTING_LAYER_NAME);
    if (layer) {
      BOTTOM_ROUTING_LAYER = layer->getLayerNum();
    } else {
      logger_->warn(utl::DRT,
                    272,
                    "bottomRoutingLayer {} not found.",
                    BOTTOM_ROUTING_LAYER_NAME);
    }
  }

  if (!TOP_ROUTING_LAYER_NAME.empty()) {
    frLayer* layer = tech->getLayer(TOP_ROUTING_LAYER_NAME);
    if (layer) {
      TOP_ROUTING_LAYER = layer->getLayerNum();
    } else {
      logger_->warn(utl::DRT,
                    273,
                    "topRoutingLayer {} not found.",
                    TOP_ROUTING_LAYER_NAME);
    }
  }

  if (!VIAINPIN_BOTTOMLAYER_NAME.empty()) {
    frLayer* layer = tech->getLayer(VIAINPIN_BOTTOMLAYER_NAME);
    if (layer) {
      VIAINPIN_BOTTOMLAYERNUM = layer->getLayerNum();
    } else {
      logger_->warn(utl::DRT,
                    606,
                    "via in pin bottom layer {} not found.",
                    VIAINPIN_BOTTOMLAYER_NAME);
    }
  }

  if (!VIAINPIN_TOPLAYER_NAME.empty()) {
    frLayer* layer = tech->getLayer(VIAINPIN_TOPLAYER_NAME);
    if (layer) {
      VIAINPIN_TOPLAYERNUM = layer->getLayerNum();
    } else {
      logger_->warn(utl::DRT,
                    607,
                    "via in pin top layer {} not found.",
                    VIAINPIN_TOPLAYER_NAME);
    }
  }

  if (!REPAIR_PDN_LAYER_NAME.empty()) {
    frLayer* layer = tech->getLayer(REPAIR_PDN_LAYER_NAME);
    if (layer) {
      GC_IGNORE_PDN_LAYER = layer->getLayerNum();
    } else {
      logger_->warn(
          utl::DRT, 617, "PDN layer {} not found.", REPAIR_PDN_LAYER_NAME);
    }
  }
  parser.postProcess();
  if (db_ != nullptr && db_->getChip() != nullptr
      && db_->getChip()->getBlock() != nullptr)
    db_callback_->addOwner(db_->getChip()->getBlock());
}

void TritonRoute::prep()
{
  FlexRP rp(getDesign(), getDesign()->getTech(), logger_);
  rp.main();
}

void TritonRoute::gr()
{
  FlexGR gr(getDesign(), logger_, stt_builder_);
  gr.main(db_);
}

void TritonRoute::ta()
{
  FlexTA ta(getDesign(), logger_);
  ta.setDebug(debug_.get(), db_);
  ta.main();
}

void TritonRoute::dr()
{
  num_drvs_ = -1;
  dr_ = std::make_unique<FlexDR>(this, getDesign(), logger_, db_);
  dr_->setDebug(debug_.get());
  if (distributed_)
    dr_->setDistributed(dist_, dist_ip_, dist_port_, shared_volume_);
  if (SINGLE_STEP_DR) {
    dr_->init();
  } else {
    dr_->main();
  }
}

void TritonRoute::stepDR(int size,
                         int offset,
                         int mazeEndIter,
                         frUInt4 workerDRCCost,
                         frUInt4 workerMarkerCost,
                         frUInt4 workerFixedShapeCost,
                         float workerMarkerDecay,
                         int ripupMode,
                         bool followGuide)
{
  dr_->searchRepair({size,
                     offset,
                     mazeEndIter,
                     workerDRCCost,
                     workerMarkerCost,
                     workerFixedShapeCost,
                     workerMarkerDecay,
                     ripupMode,
                     followGuide});
  num_drvs_ = design_->getTopBlock()->getNumMarkers();
}

void TritonRoute::endFR()
{
  if (SINGLE_STEP_DR) {
    dr_->end(/* done */ true);
  }
  dr_.reset();
  io::Writer writer(getDesign(), logger_);
  writer.updateDb(db_);

  num_drvs_ = design_->getTopBlock()->getNumMarkers();
  if (!REPAIR_PDN_LAYER_NAME.empty()) {
    auto dbBlock = db_->getChip()->getBlock();
    auto pdnLayer = design_->getTech()->getLayer(REPAIR_PDN_LAYER_NAME);
    frLayerNum pdnLayerNum = pdnLayer->getLayerNum();
    frList<std::unique_ptr<frMarker>> markers;
    auto blockBox = design_->getTopBlock()->getBBox();
    GC_IGNORE_PDN_LAYER = -1;
    getDRCMarkers(markers, blockBox);
    std::vector<std::pair<odb::Rect, odb::dbId<odb::dbSBox>>> allWires;
    for (auto* net : dbBlock->getNets()) {
      if (!net->getSigType().isSupply())
        continue;
      for (auto* swire : net->getSWires()) {
        for (auto* wire : swire->getWires()) {
          if (!wire->isVia()) {
            continue;
          }
          //
          std::vector<odb::dbShape> via_boxes;
          wire->getViaBoxes(via_boxes);
          for (const auto& via_box : via_boxes) {
            auto* layer = via_box.getTechLayer();
            if (layer->getType() != odb::dbTechLayerType::CUT)
              continue;
            if (layer->getName() != pdnLayer->getName())
              continue;
            allWires.push_back({via_box.getBox(), wire->getId()});
          }
        }
      }
    }
    RTree<odb::dbId<odb::dbSBox>> pdnTree(allWires);
    std::set<odb::dbId<odb::dbSBox>> removedBoxes;
    for (const auto& marker : markers) {
      if (marker->getLayerNum() != pdnLayerNum)
        continue;
      bool supply = false;
      for (auto src : marker->getSrcs()) {
        if (src->typeId() == frcNet) {
          frNet* net = static_cast<frNet*>(src);
          if (net->getType().isSupply()) {
            supply = true;
            break;
          }
        }
      }
      if (!supply)
        continue;
      auto markerBox = marker->getBBox();
      odb::Rect queryBox;
      markerBox.bloat(1, queryBox);
      std::vector<rq_box_value_t<odb::dbId<odb::dbSBox>>> results;
      pdnTree.query(bgi::intersects(queryBox), back_inserter(results));
      for (auto& [rect, bid] : results) {
        if (removedBoxes.find(bid) == removedBoxes.end()) {
          removedBoxes.insert(bid);
          auto boxPtr = odb::dbSBox::getSBox(dbBlock, bid);
          odb::dbSBox::destroy(boxPtr);
        }
      }
    }
    logger_->report("Removed {} pdn vias on layer {}",
                    removedBoxes.size(),
                    pdnLayer->getName());
  }
}

void TritonRoute::reportConstraints()
{
  getDesign()->getTech()->printAllConstraints(logger_);
}

bool TritonRoute::writeGlobals(const std::string& name)
{
  std::ofstream file(name);
  if (!file.good())
    return false;
  frOArchive ar(file);
  registerTypes(ar);
  serializeGlobals(ar);
  file.close();
  return true;
}

void TritonRoute::sendDesignDist()
{
  if (distributed_) {
    std::string design_path = fmt::format("{}DESIGN.db", shared_volume_);
    std::string globals_path = fmt::format("{}DESIGN.globals", shared_volume_);
    ord::OpenRoad::openRoad()->writeDb(design_path.c_str());
    writeGlobals(globals_path.c_str());
    dst::JobMessage msg(dst::JobMessage::UPDATE_DESIGN,
                        dst::JobMessage::BROADCAST),
        result(dst::JobMessage::NONE);
    std::unique_ptr<dst::JobDescription> desc
        = std::make_unique<RoutingJobDescription>();
    RoutingJobDescription* rjd
        = static_cast<RoutingJobDescription*>(desc.get());
    rjd->setDesignPath(design_path);
    rjd->setSharedDir(shared_volume_);
    rjd->setGlobalsPath(globals_path);
    rjd->setDesignUpdate(false);
    msg.setJobDescription(std::move(desc));
    bool ok = dist_->sendJob(msg, dist_ip_.c_str(), dist_port_, result);
    if (!ok)
      logger_->error(DRT, 12304, "Updating design remotely failed");
  }
  design_->clearUpdates();
}
static void serializeUpdatesBatch(const std::vector<drUpdate>& batch,
                                  const std::string& file_name)
{
  std::ofstream file(file_name.c_str());
  frOArchive ar(file);
  registerTypes(ar);
  ar << batch;
  file.close();
}

void TritonRoute::sendGlobalsUpdates(const std::string& globals_path,
                                     const std::string& serializedViaData)
{
  if (!distributed_)
    return;
  ProfileTask task("DIST: SENDING GLOBALS");
  dst::JobMessage msg(dst::JobMessage::UPDATE_DESIGN,
                      dst::JobMessage::BROADCAST),
      result(dst::JobMessage::NONE);
  std::unique_ptr<dst::JobDescription> desc
      = std::make_unique<RoutingJobDescription>();
  RoutingJobDescription* rjd = static_cast<RoutingJobDescription*>(desc.get());
  rjd->setGlobalsPath(globals_path);
  rjd->setSharedDir(shared_volume_);
  rjd->setViaData(serializedViaData);
  msg.setJobDescription(std::move(desc));
  bool ok = dist_->sendJob(msg, dist_ip_.c_str(), dist_port_, result);
  if (!ok)
    logger_->error(DRT, 9504, "Updating globals remotely failed");
}

void TritonRoute::sendDesignUpdates(const std::string& globals_path)
{
  if (!distributed_)
    return;
  if (!design_->hasUpdates())
    return;
  std::unique_ptr<ProfileTask> serializeTask;
  if (design_->getVersion() == 0)
    serializeTask = std::make_unique<ProfileTask>("DIST: SERIALIZE_TA");
  else
    serializeTask = std::make_unique<ProfileTask>("DIST: SERIALIZE_UPDATES");
  const auto& designUpdates = design_->getUpdates();
  omp_set_num_threads(MAX_THREADS);
  std::vector<std::string> updates(designUpdates.size());
#pragma omp parallel for schedule(dynamic)
  for (int i = 0; i < designUpdates.size(); i++) {
    updates[i] = fmt::format("{}updates_{}.bin", shared_volume_, i);
    serializeUpdatesBatch(designUpdates.at(i), updates[i]);
  }
  serializeTask->done();
  std::unique_ptr<ProfileTask> task;
  if (design_->getVersion() == 0)
    task = std::make_unique<ProfileTask>("DIST: SENDING_TA");
  else
    task = std::make_unique<ProfileTask>("DIST: SENDING_UDPATES");
  dst::JobMessage msg(dst::JobMessage::UPDATE_DESIGN,
                      dst::JobMessage::BROADCAST),
      result(dst::JobMessage::NONE);
  std::unique_ptr<dst::JobDescription> desc
      = std::make_unique<RoutingJobDescription>();
  RoutingJobDescription* rjd = static_cast<RoutingJobDescription*>(desc.get());
  rjd->setUpdates(updates);
  rjd->setGlobalsPath(globals_path);
  rjd->setSharedDir(shared_volume_);
  rjd->setDesignUpdate(true);
  msg.setJobDescription(std::move(desc));
  bool ok = dist_->sendJob(msg, dist_ip_.c_str(), dist_port_, result);
  if (!ok)
    logger_->error(DRT, 304, "Updating design remotely failed");
  task->done();
  design_->clearUpdates();
  design_->incrementVersion();
}

int TritonRoute::main()
{
  if (debug_->debugDumpDR) {
    std::string globals_path
        = fmt::format("{}/init_globals.bin", debug_->dumpDir);
    writeGlobals(globals_path);
  }
  MAX_THREADS = ord::OpenRoad::openRoad()->getThreadCount();
  if (distributed_ && !DO_PA) {
    asio::post(dist_pool_, boost::bind(&TritonRoute::sendDesignDist, this));
  }
  initDesign();
  if (DO_PA) {
    FlexPA pa(getDesign(), logger_);
    pa.setDebug(debug_.get(), db_);
    pa.main();
    if (distributed_ || debug_->debugDR || debug_->debugDumpDR) {
      io::Writer writer(getDesign(), logger_);
      writer.updateDb(db_, true);
      if (distributed_)
        asio::post(dist_pool_, boost::bind(&TritonRoute::sendDesignDist, this));
    }
  }
  if (debug_->debugDumpDR) {
    ord::OpenRoad::openRoad()->writeDb(
        fmt::format("{}/design.odb", debug_->dumpDir).c_str());
  }
  if (!initGuide()) {
    gr();
    io::Parser parser(db_, getDesign(), logger_);
    ENABLE_VIA_GEN = true;
    parser.readGuide();
    parser.initDefaultVias();
    parser.postProcessGuide();
  }
  prep();
  ta();
  if (distributed_) {
    asio::post(dist_pool_,
               boost::bind(&TritonRoute::sendDesignUpdates, this, ""));
  }
  dr();
  if (!SINGLE_STEP_DR) {
    endFR();
  }
  return 0;
}

void TritonRoute::pinAccess(std::vector<odb::dbInst*> target_insts)
{
  clearDesign();
  MAX_THREADS = ord::OpenRoad::openRoad()->getThreadCount();
  ENABLE_VIA_GEN = true;
  initDesign();
  FlexPA pa(getDesign(), logger_);
  pa.setTargetInstances(target_insts);
  pa.setDebug(debug_.get(), db_);
  pa.main();
  io::Writer writer(getDesign(), logger_);
  writer.updateDb(db_, true);
}

void TritonRoute::getDRCMarkers(frList<std::unique_ptr<frMarker>>& markers,
                                const Rect& requiredDrcBox)
{
  MAX_THREADS = ord::OpenRoad::openRoad()->getThreadCount();
  std::vector<std::vector<std::unique_ptr<FlexGCWorker>>> workersBatches(1);
  auto size = 7;
  auto offset = 0;
  auto gCellPatterns = design_->getTopBlock()->getGCellPatterns();
  auto& xgp = gCellPatterns.at(0);
  auto& ygp = gCellPatterns.at(1);
  for (int i = offset; i < (int) xgp.getCount(); i += size) {
    for (int j = offset; j < (int) ygp.getCount(); j += size) {
      Rect routeBox1 = design_->getTopBlock()->getGCellBox(Point(i, j));
      const int max_i = min((int) xgp.getCount() - 1, i + size - 1);
      const int max_j = min((int) ygp.getCount(), j + size - 1);
      Rect routeBox2 = design_->getTopBlock()->getGCellBox(Point(max_i, max_j));
      Rect routeBox(routeBox1.xMin(),
                    routeBox1.yMin(),
                    routeBox2.xMax(),
                    routeBox2.yMax());
      Rect extBox;
      Rect drcBox;
      routeBox.bloat(DRCSAFEDIST, drcBox);
      routeBox.bloat(MTSAFEDIST, extBox);
      if (!drcBox.intersects(requiredDrcBox))
        continue;
      auto gcWorker
          = std::make_unique<FlexGCWorker>(design_->getTech(), logger_);
      gcWorker->setDrcBox(drcBox);
      gcWorker->setExtBox(extBox);
      if (workersBatches.back().size() >= BATCHSIZE)
        workersBatches.push_back(std::vector<std::unique_ptr<FlexGCWorker>>());
      workersBatches.back().push_back(std::move(gcWorker));
    }
  }
  std::map<MarkerId, frMarker*> mapMarkers;
  omp_set_num_threads(MAX_THREADS);
  for (auto& workers : workersBatches) {
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < workers.size(); i++) {
      workers[i]->init(design_.get());
      workers[i]->main();
    }
    for (const auto& worker : workers) {
      for (auto& marker : worker->getMarkers()) {
        Rect bbox = marker->getBBox();
        if (!bbox.intersects(requiredDrcBox))
          continue;
        auto layerNum = marker->getLayerNum();
        auto con = marker->getConstraint();
        std::vector<frBlockObject*> srcs(2, nullptr);
        int i = 0;
        for (auto& src : marker->getSrcs()) {
          srcs.at(i) = src;
          i++;
        }
        if (mapMarkers.find({bbox, layerNum, con, srcs[0], srcs[1]})
            != mapMarkers.end()) {
          continue;
        }
        if (mapMarkers.find({bbox, layerNum, con, srcs[1], srcs[0]})
            != mapMarkers.end()) {
          continue;
        }
        markers.push_back(std::make_unique<frMarker>(*marker));
        mapMarkers[{bbox, layerNum, con, srcs[0], srcs[1]}]
            = markers.back().get();
      }
    }
    workers.clear();
  }
}

void TritonRoute::checkDRC(const char* filename, int x1, int y1, int x2, int y2)
{
  GC_IGNORE_PDN_LAYER = -1;
  initDesign();
  Rect requiredDrcBox(x1, y1, x2, y2);
  if (requiredDrcBox.area() == 0) {
    requiredDrcBox = design_->getTopBlock()->getBBox();
  }
  frList<std::unique_ptr<frMarker>> markers;
  getDRCMarkers(markers, requiredDrcBox);
  reportDRC(filename, markers, requiredDrcBox);
}

void TritonRoute::readParams(const string& fileName)
{
  logger_->warn(utl::DRT, 252, "params file is deprecated. Use tcl arguments.");

  ifstream fin(fileName.c_str());
  string line;
  if (fin.is_open()) {
    while (fin.good()) {
      getline(fin, line);
      if (line[0] != '#') {
        char delimiter = ':';
        int pos = line.find(delimiter);
        string field = line.substr(0, pos);
        string value = line.substr(pos + 1);
        stringstream ss(value);
        if (field == "lef") {
          logger_->warn(utl::DRT, 148, "Deprecated lef param in params file.");
        } else if (field == "def") {
          logger_->warn(utl::DRT, 227, "Deprecated def param in params file.");
        } else if (field == "guide") {
          logger_->warn(
              utl::DRT,
              309,
              "Deprecated guide param in params file. use read_guide instead.");
        } else if (field == "outputTA") {
          logger_->warn(
              utl::DRT, 266, "Deprecated outputTA param in params file.");
        } else if (field == "output") {
          logger_->warn(
              utl::DRT, 205, "Deprecated output param in params file.");
        } else if (field == "outputguide") {
          logger_->warn(utl::DRT,
                        310,
                        "Deprecated outputguide param in params file. use "
                        "write_guide instead.");
        } else if (field == "save_guide_updates") {
          SAVE_GUIDE_UPDATES = true;
        } else if (field == "outputMaze") {
          OUT_MAZE_FILE = value;
        } else if (field == "outputDRC") {
          DRC_RPT_FILE = value;
        } else if (field == "outputCMap") {
          CMAP_FILE = value;
        } else if (field == "threads") {
          logger_->warn(utl::DRT,
                        274,
                        "Deprecated threads param in params file."
                        " Use 'set_thread_count'.");
        } else if (field == "verbose")
          VERBOSE = atoi(value.c_str());
        else if (field == "dbProcessNode") {
          DBPROCESSNODE = value;
        } else if (field == "viaInPinBottomLayer") {
          VIAINPIN_BOTTOMLAYER_NAME = value;
        } else if (field == "viaInPinTopLayer") {
          VIAINPIN_TOPLAYER_NAME = value;
        } else if (field == "drouteEndIterNum") {
          END_ITERATION = atoi(value.c_str());
        } else if (field == "OR_SEED") {
          OR_SEED = atoi(value.c_str());
        } else if (field == "OR_K") {
          OR_K = atof(value.c_str());
        } else if (field == "bottomRoutingLayer") {
          BOTTOM_ROUTING_LAYER_NAME = value;
        } else if (field == "topRoutingLayer") {
          TOP_ROUTING_LAYER_NAME = value;
        } else if (field == "initRouteShapeCost") {
          ROUTESHAPECOST = atoi(value.c_str());
        } else if (field == "clean_patches")
          CLEAN_PATCHES = true;
      }
    }
    fin.close();
  }
}

void TritonRoute::addUserSelectedVia(const std::string& viaName)
{
  if (db_->getChip() == nullptr || db_->getChip()->getBlock() == nullptr
      || db_->getTech() == nullptr) {
    logger_->error(DRT, 610, "Load design before setting default vias");
  }
  auto block = db_->getChip()->getBlock();
  auto tech = db_->getTech();
  if (tech->findVia(viaName.c_str()) == nullptr
      && block->findVia(viaName.c_str()) == nullptr) {
    logger_->error(utl::DRT, 611, "Via {} not found", viaName);
  } else {
    design_->addUserSelectedVia(viaName);
  }
}

void TritonRoute::setUnidirectionalLayer(const std::string& layerName)
{
  if (db_->getTech() == nullptr) {
    logger_->error(DRT, 615, "Load tech before setting unidirectional layers");
  }
  auto tech = db_->getTech();
  auto dbLayer = tech->findLayer(layerName.c_str());
  if (dbLayer == nullptr) {
    logger_->error(utl::DRT, 616, "Layer {} not found", layerName);
  } else {
    design_->getTech()->setUnidirectionalLayer(dbLayer);
  }
}

void TritonRoute::setParams(const ParamStruct& params)
{
  OUT_MAZE_FILE = params.outputMazeFile;
  DRC_RPT_FILE = params.outputDrcFile;
  CMAP_FILE = params.outputCmapFile;
  GUIDE_REPORT_FILE = params.outputGuideCoverageFile;
  VERBOSE = params.verbose;
  ENABLE_VIA_GEN = params.enableViaGen;
  DBPROCESSNODE = params.dbProcessNode;
  CLEAN_PATCHES = params.cleanPatches;
  DO_PA = params.doPa;
  SINGLE_STEP_DR = params.singleStepDR;
  if (!params.viaInPinBottomLayer.empty()) {
    VIAINPIN_BOTTOMLAYER_NAME = params.viaInPinBottomLayer;
  }
  if (!params.viaInPinTopLayer.empty()) {
    VIAINPIN_TOPLAYER_NAME = params.viaInPinTopLayer;
  }
  if (params.drouteEndIter >= 0) {
    END_ITERATION = params.drouteEndIter;
  }
  OR_SEED = params.orSeed;
  OR_K = params.orK;
  if (!params.bottomRoutingLayer.empty()) {
    BOTTOM_ROUTING_LAYER_NAME = params.bottomRoutingLayer;
  }
  if (!params.topRoutingLayer.empty()) {
    TOP_ROUTING_LAYER_NAME = params.topRoutingLayer;
  }
  if (params.minAccessPoints > 0) {
    MINNUMACCESSPOINT_STDCELLPIN = params.minAccessPoints;
    MINNUMACCESSPOINT_MACROCELLPIN = params.minAccessPoints;
  }
  SAVE_GUIDE_UPDATES = params.saveGuideUpdates;
  REPAIR_PDN_LAYER_NAME = params.repairPDNLayerName;
}

void TritonRoute::addWorkerResults(
    const std::vector<std::pair<int, std::string>>& results)
{
  std::unique_lock<std::mutex> lock(results_mutex_);
  workers_results_.insert(
      workers_results_.end(), results.begin(), results.end());
  results_sz_ = workers_results_.size();
}

bool TritonRoute::getWorkerResults(
    std::vector<std::pair<int, std::string>>& results)
{
  std::unique_lock<std::mutex> lock(results_mutex_);
  if (workers_results_.empty())
    return false;
  results = workers_results_;
  workers_results_.clear();
  results_sz_ = 0;
  return true;
}

int TritonRoute::getWorkerResultsSize()
{
  return results_sz_;
}

void TritonRoute::reportDRC(const string& file_name,
                            const frList<std::unique_ptr<frMarker>>& markers,
                            Rect drcBox)
{
  double dbu = getDesign()->getTech()->getDBUPerUU();

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
    for (const auto& marker : markers) {
      // get violation bbox
      Rect bbox = marker->getBBox();
      if (drcBox != Rect() && !drcBox.intersects(bbox))
        continue;
      auto tech = getDesign()->getTech();
      auto layer = tech->getLayer(marker->getLayerNum());
      auto layerType = layer->getType();

      auto con = marker->getConstraint();
      drcRpt << "  violation type: ";
      if (con) {
        std::string violName;
        if (con->typeId() == frConstraintTypeEnum::frcShortConstraint
            && layerType == dbTechLayerType::CUT)
          violName = "Cut Short";
        else
          violName = con->getViolName();
        drcRpt << violName;
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

      drcRpt << "    bbox = ( " << bbox.xMin() / dbu << ", "
             << bbox.yMin() / dbu << " ) - ( " << bbox.xMax() / dbu << ", "
             << bbox.yMax() / dbu << " ) on Layer ";
      drcRpt << layer->getName() << "\n";
    }
  } else {
    cout << "Error: Fail to open DRC report file\n";
  }
}
