// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "drt/TritonRoute.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "AbstractGraphicsFactory.h"
#include "DesignCallBack.h"
#include "PACallBack.h"
#include "absl/synchronization/mutex.h"
#include "boost/asio/post.hpp"
#include "boost/bind/bind.hpp"
#include "boost/geometry/geometry.hpp"
#include "db/infra/frSegStyle.h"
#include "db/obj/frShape.h"
#include "db/obj/frVia.h"
#include "db/tech/frLayer.h"
#include "db/tech/frTechObject.h"
#include "db/tech/frViaDef.h"
#include "distributed/PinAccessJobDescription.h"
#include "distributed/RoutingCallBack.h"
#include "distributed/RoutingJobDescription.h"
#include "distributed/drUpdate.h"
#include "distributed/frArchive.h"
#include "dr/AbstractDRGraphics.h"
#include "dr/FlexDR.h"
#include "dst/Distributed.h"
#include "dst/JobMessage.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "frProfileTask.h"
#include "frRTree.h"
#include "gc/FlexGC.h"
#include "global.h"
#include "gr/FlexGR.h"
#include "io/GuideProcessor.h"
#include "io/io.h"
#include "odb/db.h"
#include "odb/dbId.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "pa/AbstractPAGraphics.h"
#include "pa/FlexPA.h"
#include "rp/FlexRP.h"
#include "serialization.h"
#include "stt/SteinerTreeBuilder.h"
#include "ta/AbstractTAGraphics.h"
#include "ta/FlexTA.h"
#include "utl/CallBackHandler.h"
#include "utl/Logger.h"
#include "utl/ScopedTemporaryFile.h"

using odb::dbTechLayerType;

namespace drt {
TritonRoute::TritonRoute(odb::dbDatabase* db,
                         utl::Logger* logger,
                         utl::CallBackHandler* callback_handler,
                         dst::Distributed* dist,
                         stt::SteinerTreeBuilder* stt_builder)
    : debug_(std::make_unique<frDebugSettings>()),
      db_callback_(std::make_unique<DesignCallBack>(this)),
      pa_callback_(std::make_unique<PACallBack>(this)),
      router_cfg_(std::make_unique<RouterConfiguration>())
{
  if (distributed_) {
    dist_pool_.emplace(1);
  }
  db_ = db;
  logger_ = logger;
  dist_ = dist;
  stt_builder_ = stt_builder;
  design_ = std::make_unique<frDesign>(logger_, router_cfg_.get());
  dist->addCallBack(new RoutingCallBack(this, dist, logger));
  pa_callback_->setOwner(callback_handler);
}

TritonRoute::~TritonRoute() = default;

void TritonRoute::initGraphics(
    std::unique_ptr<AbstractGraphicsFactory> graphics_factory)
{
  graphics_factory_ = std::move(graphics_factory);
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

void TritonRoute::setDebugSnapshotDir(const std::string& snapshotDir)
{
  debug_->snapshotDir = snapshotDir;
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
  if (distributed_ && !dist_pool_.has_value()) {
    dist_pool_.emplace(1);
  }
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
  auto worker = FlexDRWorker::load(
      workerStr, viaData, design_.get(), logger_, router_cfg_.get());
  worker->setSharedVolume(shared_volume_);
  worker->setDebugSettings(debug_.get());
  if (graphics_factory_->guiActive() && debug_->debugDR) {
    std::unique_ptr<AbstractDRGraphics> dr_graphics
        = graphics_factory_->makeUniqueDRGraphics();
    worker->setGraphics(dr_graphics.get());
    dr_graphics->startIter(worker->getDRIter(), router_cfg_.get());
  }
  std::string result = worker->reloadedMain();
  return result;
}

void TritonRoute::debugSingleWorker(const std::string& dumpDir,
                                    const std::string& drcRpt)
{
  {
    io::Writer writer(getDesign(), logger_);
    writer.updateTrackAssignment(db_->getChip()->getBlock());
  }
  FlexDRViaData viaData;
  std::ifstream viaDataFile(fmt::format("{}/viadata.bin", dumpDir),
                            std::ios::binary);
  frIArchive ar(viaDataFile);
  ar >> viaData;

  std::ifstream workerFile(fmt::format("{}/worker.bin", dumpDir),
                           std::ios::binary);
  std::string workerStr((std::istreambuf_iterator<char>(workerFile)),
                        std::istreambuf_iterator<char>());
  workerFile.close();
  auto worker = FlexDRWorker::load(
      workerStr, &viaData, design_.get(), logger_, router_cfg_.get());
  std::unique_ptr<AbstractDRGraphics> graphics
      = debug_->debugDR ? graphics_factory_->makeUniqueDRGraphics() : nullptr;
  worker->setGraphics(graphics.get());
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
  if (graphics) {
    graphics->startIter(worker->getDRIter(), router_cfg_.get());
  }
  worker->reloadedMain();
  bool updated = worker->end(design_.get());
  debugPrint(logger_,
             utl::DRT,
             "autotuner",
             1,
             "End number of markers {}. Updated={}",
             worker->getBestNumMarkers(),
             updated);
  if (updated) {
    reportDRC(drcRpt,
              design_->getTopBlock()->getMarkers(),
              "DRC - debug single worker",
              worker->getDrcBox());
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
  serializeGlobals(ar, router_cfg_.get());
  file.close();
}

void TritonRoute::resetDb(const char* file_name)
{
  std::ifstream stream;
  stream.open(file_name, std::ios::binary);
  try {
    if (db_->getChip() && db_->getChip()->getBlock()) {
      logger_->error(
          DRT,
          9947,
          "You can't load a new db file as the db is already populated");
    }

    stream.exceptions(std::ifstream::failbit | std::ifstream::badbit
                      | std::ios::eofbit);

    db_->read(stream);
  } catch (const std::ios_base::failure& f) {
    logger_->error(
        DRT, 9954, "odb file {} is invalid: {}", file_name, f.what());
  }
  design_ = std::make_unique<frDesign>(logger_, router_cfg_.get());
  initDesign();
  if (!db_->getChip()->getBlock()->getAccessPoints().empty()) {
    initGuide();
    prep();
    design_->getRegionQuery()->initDRObj();
  }
}

void TritonRoute::clearDesign()
{
  design_ = std::make_unique<frDesign>(logger_, router_cfg_.get());
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

void TritonRoute::updateDesign(const std::vector<std::string>& updatesStrs,
                               int num_threads)
{
  omp_set_num_threads(num_threads);
  std::vector<std::vector<drUpdate>> updates(updatesStrs.size());
#pragma omp parallel for schedule(dynamic)
  for (int i = 0; i < updatesStrs.size(); i++) {
    deserializeUpdate(design_.get(), updatesStrs.at(i), updates[i]);
  }
  applyUpdates(updates);
}

void TritonRoute::updateDesign(const std::string& path, int num_threads)
{
  omp_set_num_threads(num_threads);
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
            case frcVia: {
              auto via = static_cast<frVia*>(pinfig);
              frVia updatedVia = update.getVia();
              via->setBottomConnected(updatedVia.isBottomConnected());
              via->setTopConnected(updatedVia.isTopConnected());
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

bool TritonRoute::initGuide()
{
  io::GuideProcessor guide_processor(
      getDesign(), db_, logger_, router_cfg_.get());
  bool guideOk = guide_processor.readGuides();
  guide_processor.processGuides();
  io::Parser parser(db_, getDesign(), logger_, router_cfg_.get());
  parser.initRPin();
  return guideOk;
}
void TritonRoute::initDesign()
{
  if (db_ == nullptr || db_->getChip() == nullptr
      || db_->getChip()->getBlock() == nullptr) {
    logger_->error(utl::DRT, 151, "Database, chip or block not initialized.");
  }
  const bool design_exists = getDesign()->getTopBlock() != nullptr;
  io::Parser parser(db_, getDesign(), logger_, router_cfg_.get());
  if (design_exists) {
    parser.updateDesign();
  } else {
    parser.readTechAndLibs(db_);
    parser.readDesign(db_);
  }
  auto tech = getDesign()->getTech();

  if (!router_cfg_->VIAINPIN_BOTTOMLAYER_NAME.empty()) {
    frLayer* layer = tech->getLayer(router_cfg_->VIAINPIN_BOTTOMLAYER_NAME);
    if (layer) {
      router_cfg_->VIAINPIN_BOTTOMLAYERNUM = layer->getLayerNum();
    } else {
      logger_->warn(utl::DRT,
                    606,
                    "via in pin bottom layer {} not found.",
                    router_cfg_->VIAINPIN_BOTTOMLAYER_NAME);
    }
  }

  if (!router_cfg_->VIAINPIN_TOPLAYER_NAME.empty()) {
    frLayer* layer = tech->getLayer(router_cfg_->VIAINPIN_TOPLAYER_NAME);
    if (layer) {
      router_cfg_->VIAINPIN_TOPLAYERNUM = layer->getLayerNum();
    } else {
      logger_->warn(utl::DRT,
                    607,
                    "via in pin top layer {} not found.",
                    router_cfg_->VIAINPIN_TOPLAYER_NAME);
    }
  }

  if (!router_cfg_->VIA_ACCESS_LAYER_NAME.empty()) {
    frLayer* layer = tech->getLayer(router_cfg_->VIA_ACCESS_LAYER_NAME);
    if (layer) {
      router_cfg_->VIA_ACCESS_LAYERNUM = layer->getLayerNum();
    } else {
      logger_->warn(utl::DRT,
                    609,
                    "via access layer {} not found.",
                    router_cfg_->VIA_ACCESS_LAYER_NAME);
    }
  }

  if (!router_cfg_->REPAIR_PDN_LAYER_NAME.empty()) {
    frLayer* layer = tech->getLayer(router_cfg_->REPAIR_PDN_LAYER_NAME);
    if (layer) {
      router_cfg_->GC_IGNORE_PDN_LAYER_NUM = layer->getLayerNum();
    } else {
      logger_->warn(utl::DRT,
                    617,
                    "PDN layer {} not found.",
                    router_cfg_->REPAIR_PDN_LAYER_NAME);
    }
  }
  if (!design_exists) {
    parser.postProcess();
    db_callback_->addOwner(db_->getChip()->getBlock());
    initGraphics();
  }
}

void TritonRoute::initGraphics()
{
  graphics_factory_->reset(
      debug_.get(), design_.get(), db_, logger_, router_cfg_.get());
}

void TritonRoute::prep()
{
  FlexRP rp(getDesign(), logger_, router_cfg_.get());
  rp.main();
}

void TritonRoute::gr()
{
  FlexGR gr(getDesign(), logger_, stt_builder_, router_cfg_.get());
  gr.main(db_);
}

void TritonRoute::ta()
{
  std::unique_ptr<FlexTA> ta = std::make_unique<FlexTA>(
      getDesign(), logger_, router_cfg_.get(), distributed_);
  if (debug_->debugTA) {
    ta->setDebug(graphics_factory_->makeUniqueTAGraphics());
  }
  ta->main();
  if (debug_->writeNetTracks) {
    io::Writer writer(getDesign(), logger_);
    writer.updateTrackAssignment(db_->getChip()->getBlock());
  }
}

void TritonRoute::dr()
{
  num_drvs_ = -1;
  dr_ = std::make_unique<FlexDR>(
      this, getDesign(), logger_, db_, router_cfg_.get());
  if (debug_->debugDR) {
    dr_->setDebug(graphics_factory_->makeUniqueDRGraphics());
  }
  if (distributed_) {
    dr_->setDistributed(dist_, dist_ip_, dist_port_, shared_volume_);
  }
  if (router_cfg_->SINGLE_STEP_DR) {
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
  FlexDR::SearchRepairArgs args = {.size = size,
                                   .offset = offset,
                                   .mazeEndIter = mazeEndIter,
                                   .workerDRCCost = workerDRCCost,
                                   .workerMarkerCost = workerMarkerCost,
                                   .workerFixedShapeCost = workerFixedShapeCost,
                                   .workerMarkerDecay = workerMarkerDecay,
                                   .ripupMode = getMode(ripupMode),
                                   .followGuide = followGuide};
  dr_->searchRepair(args);
  dr_->incIter();
  num_drvs_ = design_->getTopBlock()->getNumMarkers();
}
void TritonRoute::endFR()
{
  if (router_cfg_->SINGLE_STEP_DR) {
    dr_->end(/* done */ true);
  }
  dr_.reset();
  io::Writer writer(getDesign(), logger_);
  writer.updateDb(db_, router_cfg_.get());

  num_drvs_ = design_->getTopBlock()->getNumMarkers();

  repairPDNVias();
}

void TritonRoute::repairPDNVias()
{
  if (router_cfg_->REPAIR_PDN_LAYER_NAME.empty()) {
    return;
  }

  auto dbBlock = db_->getChip()->getBlock();
  auto pdnLayer
      = design_->getTech()->getLayer(router_cfg_->REPAIR_PDN_LAYER_NAME);
  frLayerNum pdnLayerNum = pdnLayer->getLayerNum();
  frList<std::unique_ptr<frMarker>> markers;
  auto blockBox = design_->getTopBlock()->getBBox();
  router_cfg_->REPAIR_PDN_LAYER_NUM = pdnLayerNum;
  router_cfg_->GC_IGNORE_PDN_LAYER_NUM = -1;
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
  serializeGlobals(ar, router_cfg_.get());
  file.close();
  return true;
}

void TritonRoute::sendDesignDist()
{
  if (distributed_) {
    std::string design_path = fmt::format("{}DESIGN.db", shared_volume_);
    std::string router_cfg_path
        = fmt::format("{}DESIGN.router_cfg", shared_volume_);

    db_->write(utl::OutStreamHandler(design_path.c_str(), true).getStream());
    writeGlobals(router_cfg_path);
    dst::JobMessage msg(dst::JobMessage::kUpdateDesign,
                        dst::JobMessage::kBroadcast),
        result(dst::JobMessage::kNone);
    std::unique_ptr<dst::JobDescription> desc
        = std::make_unique<RoutingJobDescription>();
    RoutingJobDescription* rjd
        = static_cast<RoutingJobDescription*>(desc.get());
    rjd->setDesignPath(design_path);
    rjd->setSharedDir(shared_volume_);
    rjd->setGlobalsPath(router_cfg_path);
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

void TritonRoute::sendGlobalsUpdates(const std::string& router_cfg_path,
                                     const std::string& serializedViaData)
{
  if (!distributed_) {
    return;
  }
  ProfileTask task("DIST: SENDING GLOBALS");
  dst::JobMessage msg(dst::JobMessage::kUpdateDesign,
                      dst::JobMessage::kBroadcast),
      result(dst::JobMessage::kNone);
  std::unique_ptr<dst::JobDescription> desc
      = std::make_unique<RoutingJobDescription>();
  RoutingJobDescription* rjd = static_cast<RoutingJobDescription*>(desc.get());
  rjd->setGlobalsPath(router_cfg_path);
  rjd->setSharedDir(shared_volume_);
  rjd->setViaData(serializedViaData);
  msg.setJobDescription(std::move(desc));
  bool ok = dist_->sendJob(msg, dist_ip_.c_str(), dist_port_, result);
  if (!ok) {
    logger_->error(DRT, 9504, "Updating router_cfg remotely failed");
  }
}

void TritonRoute::sendDesignUpdates(const std::string& router_cfg_path,
                                    int num_threads)
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
  omp_set_num_threads(num_threads);
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
  dst::JobMessage msg(dst::JobMessage::kUpdateDesign,
                      dst::JobMessage::kBroadcast),
      result(dst::JobMessage::kNone);
  std::unique_ptr<dst::JobDescription> desc
      = std::make_unique<RoutingJobDescription>();
  RoutingJobDescription* rjd = static_cast<RoutingJobDescription*>(desc.get());
  rjd->setUpdates(updates);
  rjd->setGlobalsPath(router_cfg_path);
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
  // Just to verify that OMP support is compiled in correctly.
  omp_set_num_threads(2);
#pragma omp parallel
  {
    if (omp_get_num_threads() != 2) {
      logger_->error(DRT, 623, "OMP threading is not working.");
    }
  }

  if (router_cfg_->DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
    router_cfg_->USENONPREFTRACKS = false;
  }
  std::unique_ptr<std::thread> pa_thread;

  if (debug_->debugDumpDR) {
    std::string router_cfg_path
        = fmt::format("{}/init_router_cfg.bin", debug_->dumpDir);
    writeGlobals(router_cfg_path);
  }
  if (distributed_) {
    if (router_cfg_->DO_PA) {
      pa_thread = std::make_unique<std::thread>([this]() {
        sendDesignDist();
        dst::JobMessage msg(dst::JobMessage::kPinAccess,
                            dst::JobMessage::kBroadcast),
            result;
        auto uDesc = std::make_unique<PinAccessJobDescription>();
        uDesc->setType(PinAccessJobDescription::INIT_PA);
        msg.setJobDescription(std::move(uDesc));
        dist_->sendJob(msg, dist_ip_.c_str(), dist_port_, result);
      });
    } else {
      asio::post(*dist_pool_, boost::bind(&TritonRoute::sendDesignDist, this));
    }
  }
  initDesign();
  bool has_routable_nets = false;
  for (auto net : db_->getChip()->getBlock()->getNets()) {
    if (net->getITerms().size() + net->getBTerms().size() > 1) {
      has_routable_nets = true;
      break;
    }
  }
  if (!has_routable_nets) {
    logger_->warn(DRT,
                  40,
                  "Design does not have any routable net "
                  "(with at least 2 terms)");
    return 0;
  }
  if (router_cfg_->DO_PA) {
    pa_ = std::make_unique<FlexPA>(
        getDesign(), logger_, dist_, router_cfg_.get());
    pa_->setDistributed(dist_ip_, dist_port_, shared_volume_, cloud_sz_);
    if (debug_->debugPA) {
      pa_->setDebug(graphics_factory_->makeUniquePAGraphics());
    }
    if (pa_thread) {
      pa_thread->join();
    }
    pa_->main();
    /// bookmark
    if (distributed_ || debug_->debugDR || debug_->debugDumpDR) {
      io::Writer writer(getDesign(), logger_);
      writer.updateDb(db_, router_cfg_.get(), true);
    }
    if (distributed_) {
      asio::post(*dist_pool_, [this]() {
        dst::JobMessage msg(dst::JobMessage::kGrdrInit,
                            dst::JobMessage::kBroadcast),
            result;
        dist_->sendJob(msg, dist_ip_.c_str(), dist_port_, result);
      });
    }
  }
  if (debug_->debugDumpDR) {
    db_->write(utl::OutStreamHandler(
                   fmt::format("{}/design.odb", debug_->dumpDir).c_str(), true)
                   .getStream());
  }
  if (!initGuide()) {
    gr();
    router_cfg_->ENABLE_VIA_GEN = true;
    io::GuideProcessor guide_processor(
        getDesign(), db_, logger_, router_cfg_.get());
    guide_processor.readGuides();
    guide_processor.processGuides();
  }
  prep();
  ta();
  if (distributed_) {
    asio::post(*dist_pool_,
               [this] { sendDesignUpdates("", router_cfg_->MAX_THREADS); });
  }
  dr();
  if (!router_cfg_->SINGLE_STEP_DR) {
    endFR();
  }
  return 0;
}

void TritonRoute::pinAccess(const std::vector<odb::dbInst*>& target_insts)
{
  if (router_cfg_->DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
    router_cfg_->USENONPREFTRACKS = false;
  }
  if (distributed_) {
    asio::post(*dist_pool_, [this]() {
      sendDesignDist();
      dst::JobMessage msg(dst::JobMessage::kPinAccess,
                          dst::JobMessage::kBroadcast),
          result;
      auto uDesc = std::make_unique<PinAccessJobDescription>();
      uDesc->setType(PinAccessJobDescription::INIT_PA);
      msg.setJobDescription(std::move(uDesc));
      dist_->sendJob(msg, dist_ip_.c_str(), dist_port_, result);
    });
  }
  clearDesign();
  router_cfg_->ENABLE_VIA_GEN = true;
  initDesign();
  pa_ = std::make_unique<FlexPA>(
      getDesign(), logger_, dist_, router_cfg_.get());
  pa_->setTargetInstances(target_insts);
  if (debug_->debugPA) {
    pa_->setDebug(graphics_factory_->makeUniquePAGraphics());
  }
  if (distributed_) {
    pa_->setDistributed(dist_ip_, dist_port_, shared_volume_, cloud_sz_);
    dist_pool_->join();
  }
  pa_->main();
  io::Writer writer(getDesign(), logger_);
  writer.updateDb(db_, router_cfg_.get(), true);
}

void TritonRoute::deleteInstancePAData(frInst* inst, bool delete_inst)
{
  if (pa_) {
    pa_->removeFromInstsSet(inst);
    if (delete_inst) {
      pa_->deleteInst(inst);
    }
  }
}

void TritonRoute::addInstancePAData(frInst* inst)
{
  if (pa_) {
    pa_->addDirtyInst(inst);
  }
}

void TritonRoute::addAvoidViaDefPA(const frViaDef* via_def)
{
  if (pa_) {
    pa_->addAvoidViaDef(via_def);
  }
}
void TritonRoute::updateDirtyPAData()
{
  if (pa_) {
    design_->getTopBlock()->removeDeletedObjects();
    pa_->updateDirtyInsts();
    io::Writer writer(getDesign(), logger_);
    writer.updateDb(getDb(), getRouterConfiguration(), true);
  }
}

void TritonRoute::fixMaxSpacing(int num_threads)
{
  initDesign();
  initGuide();
  prep();
  router_cfg_->MAX_THREADS = num_threads;
  dr_ = std::make_unique<FlexDR>(
      this, getDesign(), logger_, db_, router_cfg_.get());
  dr_->init();
  dr_->fixMaxSpacing();
  io::Writer writer(getDesign(), logger_);
  writer.updateDb(db_, router_cfg_.get());
}

void TritonRoute::getDRCMarkers(frList<std::unique_ptr<frMarker>>& markers,
                                const odb::Rect& requiredDrcBox)
{
  std::vector<std::vector<std::unique_ptr<FlexGCWorker>>> workersBatches(1);
  auto size = 7;
  auto offset = 0;
  auto gCellPatterns = design_->getTopBlock()->getGCellPatterns();
  auto& xgp = gCellPatterns.at(0);
  auto& ygp = gCellPatterns.at(1);
  for (int i = offset; i < (int) xgp.getCount(); i += size) {
    for (int j = offset; j < (int) ygp.getCount(); j += size) {
      odb::Rect routeBox1
          = design_->getTopBlock()->getGCellBox(odb::Point(i, j));
      const int max_i = std::min((int) xgp.getCount() - 1, i + size - 1);
      const int max_j = std::min((int) ygp.getCount(), j + size - 1);
      odb::Rect routeBox2
          = design_->getTopBlock()->getGCellBox(odb::Point(max_i, max_j));
      odb::Rect routeBox(routeBox1.xMin(),
                         routeBox1.yMin(),
                         routeBox2.xMax(),
                         routeBox2.yMax());
      odb::Rect extBox;
      odb::Rect drcBox;
      routeBox.bloat(router_cfg_->DRCSAFEDIST, drcBox);
      routeBox.bloat(router_cfg_->MTSAFEDIST, extBox);
      if (!drcBox.intersects(requiredDrcBox)) {
        continue;
      }
      auto gcWorker = std::make_unique<FlexGCWorker>(
          design_->getTech(), logger_, router_cfg_.get());
      gcWorker->setDrcBox(drcBox);
      gcWorker->setExtBox(extBox);
      if (workersBatches.back().size() >= router_cfg_->BATCHSIZE) {
        workersBatches.emplace_back();
      }
      workersBatches.back().push_back(std::move(gcWorker));
    }
  }
  std::map<MarkerId, frMarker*> mapMarkers;
  omp_set_num_threads(router_cfg_->MAX_THREADS);
  for (auto& workers : workersBatches) {
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < workers.size(); i++) {  // NOLINT
      workers[i]->init(design_.get());
      workers[i]->main();
    }
    for (const auto& worker : workers) {
      for (auto& marker : worker->getMarkers()) {
        odb::Rect bbox = marker->getBBox();
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

void TritonRoute::checkDRC(const char* filename,
                           int x1,
                           int y1,
                           int x2,
                           int y2,
                           const std::string& marker_name,
                           int num_threads)
{
  router_cfg_->GC_IGNORE_PDN_LAYER_NUM = -1;
  router_cfg_->REPAIR_PDN_LAYER_NUM = -1;
  router_cfg_->MAX_THREADS = num_threads;
  initDesign();
  auto gcellGrid = db_->getChip()->getBlock()->getGCellGrid();
  if (gcellGrid != nullptr && gcellGrid->getNumGridPatternsX() == 1
      && gcellGrid->getNumGridPatternsY() == 1) {
    io::GuideProcessor guide_processor(
        getDesign(), db_, logger_, router_cfg_.get());
    guide_processor.readGuides();
    guide_processor.buildGCellPatterns();
  } else if (!initGuide()) {
    logger_->error(DRT, 1, "GCELLGRID is undefined");
  }
  odb::Rect requiredDrcBox(x1, y1, x2, y2);
  if (requiredDrcBox.area() == 0) {
    requiredDrcBox = design_->getTopBlock()->getBBox();
  }
  frList<std::unique_ptr<frMarker>> markers;
  getDRCMarkers(markers, requiredDrcBox);
  reportDRC(filename, markers, marker_name, requiredDrcBox);
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
  router_cfg_->unidirectional_layers_.insert(dbLayer);
}

void TritonRoute::setParams(const ParamStruct& params)
{
  router_cfg_->OUT_MAZE_FILE = params.outputMazeFile;
  router_cfg_->DRC_RPT_FILE = params.outputDrcFile;
  router_cfg_->DRC_RPT_ITER_STEP = params.drcReportIterStep;
  router_cfg_->CMAP_FILE = params.outputCmapFile;
  router_cfg_->GUIDE_REPORT_FILE = params.outputGuideCoverageFile;
  router_cfg_->VERBOSE = params.verbose;
  router_cfg_->ENABLE_VIA_GEN = params.enableViaGen;
  router_cfg_->DBPROCESSNODE = params.dbProcessNode;
  router_cfg_->CLEAN_PATCHES = params.cleanPatches;
  router_cfg_->DO_PA = params.doPa;
  router_cfg_->SINGLE_STEP_DR = params.singleStepDR;
  if (!params.viaInPinBottomLayer.empty()) {
    router_cfg_->VIAINPIN_BOTTOMLAYER_NAME = params.viaInPinBottomLayer;
  }
  if (!params.viaInPinTopLayer.empty()) {
    router_cfg_->VIAINPIN_TOPLAYER_NAME = params.viaInPinTopLayer;
  }
  if (!params.viaAccessLayer.empty()) {
    router_cfg_->VIA_ACCESS_LAYER_NAME = params.viaAccessLayer;
  }
  if (params.drouteEndIter >= 0) {
    router_cfg_->END_ITERATION = params.drouteEndIter;
  }
  router_cfg_->OR_SEED = params.orSeed;
  router_cfg_->OR_K = params.orK;
  if (params.minAccessPoints > 0) {
    router_cfg_->MINNUMACCESSPOINT_STDCELLPIN = params.minAccessPoints;
    router_cfg_->MINNUMACCESSPOINT_MACROCELLPIN = params.minAccessPoints;
  }
  router_cfg_->SAVE_GUIDE_UPDATES = params.saveGuideUpdates;
  router_cfg_->REPAIR_PDN_LAYER_NAME = params.repairPDNLayerName;
  router_cfg_->MAX_THREADS = params.num_threads;
}

void TritonRoute::addWorkerResults(
    const std::vector<std::pair<int, std::string>>& results)
{
  absl::MutexLock lock(&results_mutex_);
  workers_results_.insert(
      workers_results_.end(), results.begin(), results.end());
  results_sz_ = workers_results_.size();
}

bool TritonRoute::getWorkerResults(
    std::vector<std::pair<int, std::string>>& results)
{
  absl::MutexLock lock(&results_mutex_);
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
                            const std::string& marker_name,
                            odb::Rect drcBox) const
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  odb::dbMarkerCategory* tool_category
      = odb::dbMarkerCategory::createOrReplace(block, marker_name.c_str());
  tool_category->setSource("DRT");

  // Obstructions Rtree
  std::vector<std::pair<odb::Rect, odb::dbObstruction*>> obstructions;
  for (odb::dbObstruction* obs : block->getObstructions()) {
    obstructions.emplace_back(obs->getBBox()->getBox(), obs);
  }
  const boost::geometry::index::rtree<std::pair<odb::Rect, odb::dbObstruction*>,
                                      boost::geometry::index::quadratic<16UL>>
      obs_rtree(obstructions.begin(), obstructions.end());

  for (const auto& marker : markers) {
    // get violation bbox
    odb::Rect bbox = marker->getBBox();
    if (drcBox != odb::Rect() && !drcBox.intersects(bbox)) {
      continue;
    }
    auto tech = getDesign()->getTech();
    auto layer = tech->getLayer(marker->getLayerNum());
    auto layerType = layer->getType();

    auto con = marker->getConstraint();
    std::string violName;
    if (con) {
      if (con->typeId() == frConstraintTypeEnum::frcShortConstraint
          && layerType == dbTechLayerType::CUT) {
        violName = "Cut Short";
      } else {
        violName = con->getViolName();
      }
    } else {
      violName = "unknown";
    }

    odb::dbMarkerCategory* category
        = odb::dbMarkerCategory::createOrGet(tool_category, violName.c_str());

    odb::dbMarker* db_marker = odb::dbMarker::create(category);
    if (db_marker == nullptr) {
      continue;
    }

    // get source(s) of violation
    for (auto src : marker->getSrcs()) {
      if (src) {
        switch (src->typeId()) {
          case frcNet:
            db_marker->addSource(
                block->findNet(static_cast<frNet*>(src)->getName().c_str()));
            break;
          case frcInstTerm: {
            frInstTerm* instTerm = (static_cast<frInstTerm*>(src));
            std::string iterm_name;
            iterm_name += instTerm->getInst()->getName();
            iterm_name += "/";
            iterm_name += instTerm->getTerm()->getName();
            db_marker->addSource(block->findITerm(iterm_name.c_str()));
            break;
          }
          case frcBTerm: {
            frBTerm* bterm = static_cast<frBTerm*>(src);
            db_marker->addSource(block->findBTerm(bterm->getName().c_str()));
            break;
          }
          case frcInstBlockage: {
            frInst* inst = (static_cast<frInstBlockage*>(src))->getInst();
            db_marker->addSource(block->findInst(inst->getName().c_str()));
            break;
          }
          case frcInst: {
            frInst* inst = (static_cast<frInst*>(src));
            db_marker->addSource(block->findInst(inst->getName().c_str()));
            break;
          }
          case frcBlockage: {
            for (auto itr
                 = obs_rtree.qbegin(boost::geometry::index::intersects(bbox));
                 itr != obs_rtree.qend();
                 itr++) {
              db_marker->addSource(itr->second);
            }
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
    db_marker->addShape(bbox);
    db_marker->setTechLayer(
        block->getTech()->findLayer(layer->getName().c_str()));
  }

  if (file_name.empty()) {
    if (router_cfg_->VERBOSE > 0) {
      logger_->warn(
          DRT,
          290,
          "Warning: no DRC report specified, skipped writing DRC report");
    }
    return;
  }

  tool_category->writeTR(file_name);
}

std::vector<int> TritonRoute::routeLayerLengths(odb::dbWire* wire) const
{
  std::vector<int> lengths;
  lengths.resize(db_->getTech()->getLayerCount());
  odb::dbWireShapeItr shapes;
  odb::dbShape s;

  for (shapes.begin(wire); shapes.next(s);) {
    if (!s.isVia()) {
      lengths[s.getTechLayer()->getNumber()] += s.getLength();
    } else {
      if (s.getTechVia()) {
        lengths[s.getTechVia()->getBottomLayer()->getNumber() + 1] += 1;
      }
    }
  }

  return lengths;
}

}  // namespace drt
