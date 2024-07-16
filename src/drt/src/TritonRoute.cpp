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
#include "distributed/PinAccessJobDescription.h"
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

namespace sta {
// Tcl files encoded into strings.
extern const char* drt_tcl_inits[];
}  // namespace sta

extern "C" {
extern int Drt_Init(Tcl_Interp* interp);
}

namespace drt {

TritonRoute::TritonRoute()
    : debug_(std::make_unique<frDebugSettings>()),
      db_callback_(std::make_unique<DesignCallBack>(this)),
      gui_(gui::Gui::get())
{
}

TritonRoute::~TritonRoute() = default;

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

void TritonRoute::setDebugWriteNetTracks(bool on)
{
  debug_->writeNetTracks = on;
}

void TritonRoute::setDumpLastWorker(bool on)
{
  debug_->dumpLastWorker = on;
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

void TritonRoute::setDebugBox(int x1, int y1, int x2, int y2)
{
  debug_->box.init(x1, y1, x2, y2);
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

RipUpMode getMode(int ripupMode)
{
  switch (ripupMode) {
    case 0:
      return RipUpMode::DRC;
    case 1:
      return RipUpMode::ALL;
    case 2:
      return RipUpMode::NEARDRC;
    default:
      return RipUpMode::INCR;
  }
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
  if (graphics_) {
    graphics_->startIter(worker->getDRIter());
  }
  std::string result = worker->reloadedMain();
  return result;
}

void TritonRoute::debugSingleWorker(const std::string& dumpDir,
                                    const std::string& drcRpt)
{
  {
    io::Writer writer(this, logger_);
    writer.updateTrackAssignment(db_->getChip()->getBlock());
  }
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
  if (debug_->mazeEndIter != -1) {
    worker->setMazeEndIter(debug_->mazeEndIter);
  }
  if (debug_->markerCost != -1) {
    worker->setMarkerCost(debug_->markerCost);
  }
  if (debug_->drcCost != -1) {
    worker->setDrcCost(debug_->drcCost);
  }
  if (debug_->fixedShapeCost != -1) {
    worker->setFixedShapeCost(debug_->fixedShapeCost);
  }
  if (debug_->markerDecay != -1) {
    worker->setMarkerDecay(debug_->markerDecay);
  }
  if (debug_->ripupMode != -1) {
    worker->setRipupMode(getMode(debug_->ripupMode));
  }
  if (debug_->followGuide != -1) {
    worker->setFollowGuide((debug_->followGuide == 1));
  }
  worker->setSharedVolume(shared_volume_);
  worker->setDebugSettings(debug_.get());
  worker->setViaData(&viaData);
  if (graphics_) {
    graphics_->startIter(worker->getDRIter());
  }
  std::string result = worker->reloadedMain();
  bool updated = worker->end(design_.get());
  debugPrint(logger_,
             utl::DRT,
             "autotuner",
             1,
             "End number of markers {}. Updated={}",
             worker->getBestNumMarkers(),
             updated);
  if (updated && !drcRpt.empty()) {
    reportDRC(
        drcRpt, design_->getTopBlock()->getMarkers(), worker->getDrcBox());
  }
}

void TritonRoute::updateGlobals(const char* file_name)
{
  std::ifstream file(file_name);
  if (!file.good()) {
    return;
  }
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
  if (!db_->getChip()->getBlock()->getAccessPoints().empty()) {
    initGuide();
    prep();
    design_->getRegionQuery()->initDRObj();
  }
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
    for (const auto& update_batch : updates) {
      if (update_batch.size() <= j) {
        continue;
      }
      const auto& update = update_batch[j];
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
              if (update.getType() == drUpdate::REMOVE_FROM_NET) {
                net->removeShape(seg);
              }
              break;
            }
            case frcPatchWire: {
              auto pwire = static_cast<frPatchWire*>(pinfig);
              regionQuery->removeDRObj(pwire);
              if (update.getType() == drUpdate::REMOVE_FROM_NET) {
                net->removePatchWire(pwire);
              }
              break;
            }
            case frcVia: {
              auto via = static_cast<frVia*>(pinfig);
              regionQuery->removeDRObj(via);
              if (update.getType() == drUpdate::REMOVE_FROM_NET) {
                net->removeVia(via);
              }
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
              if (update.getType() == drUpdate::ADD_SHAPE) {
                regionQuery->addDRObj(sptr);
              }
              break;
            }
            case frcPatchWire: {
              auto net = update.getNet();
              frPatchWire pwire = update.getPatchWire();
              std::unique_ptr<frShape> uShape
                  = std::make_unique<frPatchWire>(pwire);
              auto sptr = uShape.get();
              net->addPatchWire(std::move(uShape));
              if (update.getType() == drUpdate::ADD_SHAPE) {
                regionQuery->addDRObj(sptr);
              }
              break;
            }
            case frcVia: {
              auto net = update.getNet();
              frVia via = update.getVia();
              auto uVia = std::make_unique<frVia>(via);
              auto sptr = uVia.get();
              net->addVia(std::move(uVia));
              if (update.getType() == drUpdate::ADD_SHAPE) {
                regionQuery->addDRObj(sptr);
              }
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
          std::vector<std::unique_ptr<frConnFig>> tmp;
          tmp.push_back(std::move(uSeg));
          auto idx = update.getIndexInOwner();
          if (idx < 0 || idx >= net->getGuides().size()) {
            logger_->error(DRT,
                           9199,
                           "Guide {} out of range {}",
                           idx,
                           net->getGuides().size());
          }
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
  dist->addCallBack(new RoutingCallBack(this, dist, logger));
  // Define swig TCL commands.
  Drt_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::drt_tcl_inits);
  FlexDRGraphics::init();
}

bool TritonRoute::initGuide()
{
  io::Parser parser(db_, getDesign(), logger_);
  bool guideOk = parser.readGuide();
  parser.postProcessGuide();
  parser.initRPin();
  return guideOk;
}
void TritonRoute::initDesign()
{
  io::Parser parser(db_, getDesign(), logger_);
  if (getDesign()->getTopBlock() != nullptr) {
    parser.updateDesign();
    return;
  }
  parser.readTechAndLibs(db_);
  processBTermsAboveTopLayer();
  parser.readDesign(db_);
  auto tech = getDesign()->getTech();

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
      GC_IGNORE_PDN_LAYER_NUM = layer->getLayerNum();
    } else {
      logger_->warn(
          utl::DRT, 617, "PDN layer {} not found.", REPAIR_PDN_LAYER_NAME);
    }
  }
  parser.postProcess();
  if (db_ != nullptr && db_->getChip() != nullptr
      && db_->getChip()->getBlock() != nullptr) {
    db_callback_->addOwner(db_->getChip()->getBlock());
  }
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
  FlexTA ta(getDesign(), logger_, distributed_);
  ta.setDebug(debug_.get(), db_);
  ta.main();
}

void TritonRoute::dr()
{
  num_drvs_ = -1;
  dr_ = std::make_unique<FlexDR>(this, getDesign(), logger_, db_);
  dr_->setDebug(debug_.get());
  if (distributed_) {
    dr_->setDistributed(dist_, dist_ip_, dist_port_, shared_volume_);
  }
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
                     getMode(ripupMode),
                     followGuide});
  num_drvs_ = design_->getTopBlock()->getNumMarkers();
}

void TritonRoute::endFR()
{
  if (SINGLE_STEP_DR) {
    dr_->end(/* done */ true);
  }
  dr_.reset();
  io::Writer writer(this, logger_);
  writer.updateDb(db_);
  if (debug_->writeNetTracks) {
    writer.updateTrackAssignment(db_->getChip()->getBlock());
  }

  num_drvs_ = design_->getTopBlock()->getNumMarkers();

  repairPDNVias();
}

void TritonRoute::repairPDNVias()
{
  if (REPAIR_PDN_LAYER_NAME.empty()) {
    return;
  }

  auto dbBlock = db_->getChip()->getBlock();
  auto pdnLayer = design_->getTech()->getLayer(REPAIR_PDN_LAYER_NAME);
  frLayerNum pdnLayerNum = pdnLayer->getLayerNum();
  frList<std::unique_ptr<frMarker>> markers;
  auto blockBox = design_->getTopBlock()->getBBox();
  REPAIR_PDN_LAYER_NUM = pdnLayerNum;
  GC_IGNORE_PDN_LAYER_NUM = -1;
  getDRCMarkers(markers, blockBox);
  markers.erase(std::remove_if(markers.begin(),
                               markers.end(),
                               [pdnLayerNum](const auto& marker) {
                                 if (marker->getLayerNum() != pdnLayerNum) {
                                   return true;
                                 }
                                 for (auto src : marker->getSrcs()) {
                                   if (src->typeId() == frcNet) {
                                     frNet* net = static_cast<frNet*>(src);
                                     if (net->getType().isSupply()) {
                                       return false;
                                     }
                                   }
                                 }
                                 return true;
                               }),
                markers.end());

  if (markers.empty()) {
    // nothing to do
    return;
  }

  std::vector<std::pair<odb::Rect, odb::dbId<odb::dbSBox>>> all_vias;
  std::vector<std::pair<odb::Rect, odb::dbId<odb::dbSBox>>> block_vias;
  for (auto* net : dbBlock->getNets()) {
    if (!net->getSigType().isSupply()) {
      continue;
    }
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
          if (layer != pdnLayer->getDbLayer()) {
            continue;
          }

          if (wire->getTechVia() != nullptr) {
            all_vias.emplace_back(via_box.getBox(), wire->getId());
          } else {
            block_vias.emplace_back(via_box.getBox(), wire->getId());
          }
        }
      }
    }
  }

  const RTree<odb::dbId<odb::dbSBox>> pdnBlockViaTree(block_vias);
  std::set<odb::dbId<odb::dbSBox>> removedBoxes;
  for (const auto& marker : markers) {
    odb::Rect queryBox;
    marker->getBBox().bloat(1, queryBox);
    std::vector<rq_box_value_t<odb::dbId<odb::dbSBox>>> results;
    pdnBlockViaTree.query(bgi::intersects(queryBox), back_inserter(results));
    for (auto& [rect, bid] : results) {
      if (removedBoxes.find(bid) == removedBoxes.end()) {
        removedBoxes.insert(bid);
        auto boxPtr = odb::dbSBox::getSBox(dbBlock, bid);

        const auto new_vias = boxPtr->smashVia();
        for (auto* new_via : new_vias) {
          std::vector<odb::dbShape> via_boxes;
          new_via->getViaBoxes(via_boxes);
          for (const auto& via_box : via_boxes) {
            auto* layer = via_box.getTechLayer();
            if (layer != pdnLayer->getDbLayer()) {
              continue;
            }
            all_vias.emplace_back(via_box.getBox(), new_via->getId());
          }
        }

        if (!new_vias.empty()) {
          odb::dbSBox::destroy(boxPtr);
        }
      }
    }
  }
  removedBoxes.clear();

  const RTree<odb::dbId<odb::dbSBox>> pdnTree(all_vias);
  for (const auto& marker : markers) {
    odb::Rect queryBox;
    marker->getBBox().bloat(1, queryBox);
    std::vector<rq_box_value_t<odb::dbId<odb::dbSBox>>> results;
    pdnTree.query(bgi::intersects(queryBox), back_inserter(results));
    for (auto& [rect, bid] : results) {
      if (removedBoxes.find(bid) == removedBoxes.end()) {
        removedBoxes.insert(bid);
        odb::dbSBox::destroy(odb::dbSBox::getSBox(dbBlock, bid));
      }
    }
  }
  logger_->report("Removed {} pdn vias on layer {}",
                  removedBoxes.size(),
                  pdnLayer->getName());
}

void TritonRoute::reportConstraints()
{
  getDesign()->getTech()->printAllConstraints(logger_);
}

bool TritonRoute::writeGlobals(const std::string& name)
{
  std::ofstream file(name);
  if (!file.good()) {
    return false;
  }
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
    writeGlobals(globals_path);
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
    if (!ok) {
      logger_->error(DRT, 12304, "Updating design remotely failed");
    }
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
  if (!distributed_) {
    return;
  }
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
  if (!ok) {
    logger_->error(DRT, 9504, "Updating globals remotely failed");
  }
}

void TritonRoute::sendDesignUpdates(const std::string& globals_path)
{
  if (!distributed_) {
    return;
  }
  if (!design_->hasUpdates()) {
    return;
  }
  std::unique_ptr<ProfileTask> serializeTask;
  if (design_->getVersion() == 0) {
    serializeTask = std::make_unique<ProfileTask>("DIST: SERIALIZE_TA");
  } else {
    serializeTask = std::make_unique<ProfileTask>("DIST: SERIALIZE_UPDATES");
  }
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
  if (design_->getVersion() == 0) {
    task = std::make_unique<ProfileTask>("DIST: SENDING_TA");
  } else {
    task = std::make_unique<ProfileTask>("DIST: SENDING_UDPATES");
  }
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
  if (!ok) {
    logger_->error(DRT, 304, "Updating design remotely failed");
  }
  task->done();
  design_->clearUpdates();
  design_->incrementVersion();
}

int TritonRoute::main()
{
  if (DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
    USENONPREFTRACKS = false;
  }
  asio::thread_pool pa_pool(1);
  if (!distributed_) {
    pa_pool.join();
  }
  if (debug_->debugDumpDR) {
    std::string globals_path
        = fmt::format("{}/init_globals.bin", debug_->dumpDir);
    writeGlobals(globals_path);
  }
  MAX_THREADS = ord::OpenRoad::openRoad()->getThreadCount();
  if (distributed_) {
    if (DO_PA) {
      asio::post(pa_pool, [this]() {
        sendDesignDist();
        dst::JobMessage msg(dst::JobMessage::PIN_ACCESS,
                            dst::JobMessage::BROADCAST),
            result;
        auto uDesc = std::make_unique<PinAccessJobDescription>();
        uDesc->setType(PinAccessJobDescription::INIT_PA);
        msg.setJobDescription(std::move(uDesc));
        dist_->sendJob(msg, dist_ip_.c_str(), dist_port_, result);
      });
    } else {
      asio::post(dist_pool_, boost::bind(&TritonRoute::sendDesignDist, this));
    }
  }
  initDesign();
  if (DO_PA) {
    FlexPA pa(getDesign(), logger_, dist_);
    pa.setDistributed(dist_ip_, dist_port_, shared_volume_, cloud_sz_);
    pa.setDebug(debug_.get(), db_);
    pa_pool.join();
    pa.main();
    if (distributed_ || debug_->debugDR || debug_->debugDumpDR) {
      io::Writer writer(this, logger_);
      writer.updateDb(db_, true);
    }
    if (distributed_) {
      asio::post(dist_pool_, [this]() {
        dst::JobMessage msg(dst::JobMessage::GRDR_INIT,
                            dst::JobMessage::BROADCAST),
            result;
        dist_->sendJob(msg, dist_ip_.c_str(), dist_port_, result);
      });
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

void TritonRoute::pinAccess(const std::vector<odb::dbInst*>& target_insts)
{
  if (distributed_) {
    asio::post(dist_pool_, [this]() {
      sendDesignDist();
      dst::JobMessage msg(dst::JobMessage::PIN_ACCESS,
                          dst::JobMessage::BROADCAST),
          result;
      auto uDesc = std::make_unique<PinAccessJobDescription>();
      uDesc->setType(PinAccessJobDescription::INIT_PA);
      msg.setJobDescription(std::move(uDesc));
      dist_->sendJob(msg, dist_ip_.c_str(), dist_port_, result);
    });
  }
  clearDesign();
  MAX_THREADS = ord::OpenRoad::openRoad()->getThreadCount();
  ENABLE_VIA_GEN = true;
  initDesign();
  FlexPA pa(getDesign(), logger_, dist_);
  pa.setTargetInstances(target_insts);
  pa.setDebug(debug_.get(), db_);
  if (distributed_) {
    pa.setDistributed(dist_ip_, dist_port_, shared_volume_, cloud_sz_);
    dist_pool_.join();
  }
  pa.main();
  io::Writer writer(this, logger_);
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
      const int max_i = std::min((int) xgp.getCount() - 1, i + size - 1);
      const int max_j = std::min((int) ygp.getCount(), j + size - 1);
      Rect routeBox2 = design_->getTopBlock()->getGCellBox(Point(max_i, max_j));
      Rect routeBox(routeBox1.xMin(),
                    routeBox1.yMin(),
                    routeBox2.xMax(),
                    routeBox2.yMax());
      Rect extBox;
      Rect drcBox;
      routeBox.bloat(DRCSAFEDIST, drcBox);
      routeBox.bloat(MTSAFEDIST, extBox);
      if (!drcBox.intersects(requiredDrcBox)) {
        continue;
      }
      auto gcWorker
          = std::make_unique<FlexGCWorker>(design_->getTech(), logger_);
      gcWorker->setDrcBox(drcBox);
      gcWorker->setExtBox(extBox);
      if (workersBatches.back().size() >= BATCHSIZE) {
        workersBatches.emplace_back();
      }
      workersBatches.back().push_back(std::move(gcWorker));
    }
  }
  std::map<MarkerId, frMarker*> mapMarkers;
  omp_set_num_threads(MAX_THREADS);
  for (auto& workers : workersBatches) {
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < workers.size(); i++) {  // NOLINT
      workers[i]->init(design_.get());
      workers[i]->main();
    }
    for (const auto& worker : workers) {
      for (auto& marker : worker->getMarkers()) {
        Rect bbox = marker->getBBox();
        if (!bbox.intersects(requiredDrcBox)) {
          continue;
        }
        auto layerNum = marker->getLayerNum();
        auto con = marker->getConstraint();
        if (mapMarkers.find({bbox, layerNum, con, marker->getSrcs()})
            != mapMarkers.end()) {
          continue;
        }
        markers.push_back(std::make_unique<frMarker>(*marker));
        mapMarkers[{bbox, layerNum, con, marker->getSrcs()}]
            = markers.back().get();
      }
    }
    workers.clear();
  }
}

void TritonRoute::checkDRC(const char* filename, int x1, int y1, int x2, int y2)
{
  GC_IGNORE_PDN_LAYER_NUM = -1;
  REPAIR_PDN_LAYER_NUM = -1;
  initDesign();
  auto gcellGrid = db_->getChip()->getBlock()->getGCellGrid();
  if (gcellGrid != nullptr && gcellGrid->getNumGridPatternsX() == 1
      && gcellGrid->getNumGridPatternsY() == 1) {
    io::Parser parser(db_, getDesign(), logger_);
    parser.buildGCellPatterns(db_);
  } else if (!initGuide()) {
    logger_->error(DRT, 1, "GCELLGRID is undefined");
  }
  Rect requiredDrcBox(x1, y1, x2, y2);
  if (requiredDrcBox.area() == 0) {
    requiredDrcBox = design_->getTopBlock()->getBBox();
  }
  frList<std::unique_ptr<frMarker>> markers;
  getDRCMarkers(markers, requiredDrcBox);
  reportDRC(filename, markers, requiredDrcBox);
}

void TritonRoute::processBTermsAboveTopLayer(bool has_routing)
{
  odb::dbTech* tech = db_->getTech();
  odb::dbBlock* block = db_->getChip()->getBlock();

  odb::dbTechLayer* top_tech_layer
      = tech->findLayer(TOP_ROUTING_LAYER_NAME.c_str());
  if (top_tech_layer != nullptr) {
    int top_layer_idx = top_tech_layer->getRoutingLevel();
    for (auto bterm : block->getBTerms()) {
      if (bterm->getNet()->isSpecial()) {
        continue;
      }
      int bterm_bottom_layer_idx = std::numeric_limits<int>::max();
      for (auto bpin : bterm->getBPins()) {
        for (auto box : bpin->getBoxes()) {
          bterm_bottom_layer_idx = std::min(
              bterm_bottom_layer_idx, box->getTechLayer()->getRoutingLevel());
        }
      }

      if (bterm_bottom_layer_idx > top_layer_idx) {
        stackVias(bterm, top_layer_idx, bterm_bottom_layer_idx, has_routing);
      }
    }
  }
}

void TritonRoute::stackVias(odb::dbBTerm* bterm,
                            int top_layer_idx,
                            int bterm_bottom_layer_idx,
                            bool has_routing)
{
  odb::dbNet* net = bterm->getNet();
  if (netHasStackedVias(net)) {
    return;
  }

  odb::dbTech* tech = db_->getTech();
  auto fr_tech = getDesign()->getTech();
  std::map<int, odb::dbTechVia*> default_vias;

  for (auto layer : tech->getLayers()) {
    if (layer->getType() == odb::dbTechLayerType::CUT) {
      frLayer* fr_layer = fr_tech->getLayer(layer->getName());
      frViaDef* via_def = fr_layer->getDefaultViaDef();
      if (via_def == nullptr) {
        logger_->warn(utl::DRT,
                      204,
                      "Cut layer {} has no default via defined.",
                      layer->getName());
        continue;
      }
      odb::dbTechVia* tech_via = tech->findVia(via_def->getName().c_str());
      int via_bottom_layer_idx = tech_via->getBottomLayer()->getRoutingLevel();
      default_vias[via_bottom_layer_idx] = tech_via;
    }
  }

  // get bterm rect
  odb::Rect pin_rect;
  for (odb::dbBPin* bpin : bterm->getBPins()) {
    pin_rect = bpin->getBBox();
    break;
  }

  // set the via position as the first AP in the same layer of the bterm
  odb::Point via_position = odb::Point(pin_rect.xCenter(), pin_rect.yCenter());

  // insert the vias from the top routing layer to the bterm bottom layer
  odb::dbWire* wire = net->getWire();
  int bterms_above_max_layer = countNetBTermsAboveMaxLayer(net);

  odb::dbWireEncoder wire_encoder;
  if (wire == nullptr) {
    wire = odb::dbWire::create(net);
    wire_encoder.begin(wire);
  } else if (bterms_above_max_layer > 1 || has_routing) {
    // append wire when the net has other pins above the max routing layer
    wire_encoder.append(wire);
  } else {
    logger_->error(utl::DRT, 415, "Net {} already has routes.", net->getName());
  }

  odb::dbTechLayer* top_tech_layer = tech->findRoutingLayer(top_layer_idx);
  wire_encoder.newPath(top_tech_layer, odb::dbWireType::ROUTED);
  wire_encoder.addPoint(via_position.getX(), via_position.getY());
  for (int layer_idx = top_layer_idx; layer_idx < bterm_bottom_layer_idx;
       layer_idx++) {
    wire_encoder.addTechVia(default_vias[layer_idx]);
  }
  wire_encoder.end();
}

int TritonRoute::countNetBTermsAboveMaxLayer(odb::dbNet* net)
{
  odb::dbTech* tech = db_->getTech();
  odb::dbTechLayer* top_tech_layer
      = tech->findLayer(TOP_ROUTING_LAYER_NAME.c_str());
  int bterm_count = 0;
  for (auto bterm : net->getBTerms()) {
    int bterm_bottom_layer_idx = std::numeric_limits<int>::max();
    for (auto bpin : bterm->getBPins()) {
      for (auto box : bpin->getBoxes()) {
        bterm_bottom_layer_idx = std::min(
            bterm_bottom_layer_idx, box->getTechLayer()->getRoutingLevel());
      }
    }
    if (bterm_bottom_layer_idx > top_tech_layer->getRoutingLevel()) {
      bterm_count++;
    }
  }

  return bterm_count;
}

bool TritonRoute::netHasStackedVias(odb::dbNet* net)
{
  int bterms_above_max_layer = countNetBTermsAboveMaxLayer(net);
  uint wire_cnt = 0, via_cnt = 0;
  net->getWireCount(wire_cnt, via_cnt);

  if (wire_cnt != 0 || via_cnt == 0) {
    return false;
  }

  odb::dbWirePath path;
  odb::dbWirePathShape pshape;
  odb::dbWire* wire = net->getWire();

  odb::dbWirePathItr pitr;
  std::set<odb::Point> via_points;
  for (pitr.begin(wire); pitr.getNextPath(path);) {
    while (pitr.getNextShape(pshape)) {
      via_points.insert(path.point);
    }
  }

  if (via_points.size() != bterms_above_max_layer) {
    return false;
  }

  return true;
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
  }
  if (dbLayer->getType() != dbTechLayerType::ROUTING) {
    logger_->error(utl::DRT,
                   618,
                   "Non-routing layer {} can't be set unidirectional",
                   layerName);
  }
  design_->getTech()->setUnidirectionalLayer(dbLayer);
}

void TritonRoute::setParams(const ParamStruct& params)
{
  OUT_MAZE_FILE = params.outputMazeFile;
  DRC_RPT_FILE = params.outputDrcFile;
  DRC_RPT_ITER_STEP = params.drcReportIterStep;
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
  if (workers_results_.empty()) {
    return false;
  }
  results = workers_results_;
  workers_results_.clear();
  results_sz_ = 0;
  return true;
}

int TritonRoute::getWorkerResultsSize()
{
  return results_sz_;
}

void TritonRoute::reportDRC(const std::string& file_name,
                            const frList<std::unique_ptr<frMarker>>& markers,
                            Rect drcBox)
{
  double dbu = getDesign()->getTech()->getDBUPerUU();

  if (file_name == std::string("")) {
    if (VERBOSE > 0) {
      logger_->warn(
          DRT,
          290,
          "Warning: no DRC report specified, skipped writing DRC report");
    }
    return;
  }
  std::ofstream drcRpt(file_name.c_str());
  if (drcRpt.is_open()) {
    for (const auto& marker : markers) {
      // get violation bbox
      Rect bbox = marker->getBBox();
      if (drcBox != Rect() && !drcBox.intersects(bbox)) {
        continue;
      }
      auto tech = getDesign()->getTech();
      auto layer = tech->getLayer(marker->getLayerNum());
      auto layerType = layer->getType();

      auto con = marker->getConstraint();
      drcRpt << "  violation type: ";
      if (con) {
        std::string violName;
        if (con->typeId() == frConstraintTypeEnum::frcShortConstraint
            && layerType == dbTechLayerType::CUT) {
          violName = "Cut Short";
        } else {
          violName = con->getViolName();
        }
        drcRpt << violName;
      } else {
        drcRpt << "nullptr";
      }
      drcRpt << std::endl;
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
              frInst* inst = (static_cast<frInstBlockage*>(src))->getInst();
              drcRpt << "inst:" << inst->getName() << " ";
              break;
            }
            case frcInst: {
              frInst* inst = (static_cast<frInst*>(src));
              drcRpt << "inst:" << inst->getName() << " ";
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
    std::cout << "Error: Fail to open DRC report file\n";
  }
}

}  // namespace drt
