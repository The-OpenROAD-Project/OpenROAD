// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "drt/TritonRoute.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

#include "AbstractGraphicsFactory.h"
#include "DesignCallBack.h"
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
#include "drt-global.h"
#include "drt/PinAccessService.h"
#include "dst/Distributed.h"
#include "dst/JobMessage.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "frProfileTask.h"
#include "frRTree.h"
#include "gc/FlexGC.h"
#include "io/GuideProcessor.h"
#include "io/io.h"
#include "odb/db.h"
#include "odb/dbId.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/dbWireCodec.h"
#include "omp.h"
#include "pa/AbstractPAGraphics.h"
#include "pa/FlexPA.h"
#include "rp/FlexRP.h"
#include "serialization.h"
#include "stt/SteinerTreeBuilder.h"
#include "ta/AbstractTAGraphics.h"
#include "ta/FlexTA.h"
#include "utl/Logger.h"
#include "utl/ScopedTemporaryFile.h"
#include "utl/ServiceRegistry.h"
#include "utl/timer.h"

using odb::dbTechLayerType;

namespace drt {
TritonRoute::TritonRoute(odb::dbDatabase* db,
                         utl::Logger* logger,
                         utl::ServiceRegistry* service_registry,
                         dst::Distributed* dist,
                         stt::SteinerTreeBuilder* stt_builder)
    : debug_(std::make_unique<frDebugSettings>()),
      db_callback_(std::make_unique<DesignCallBack>(this)),
      router_cfg_(std::make_unique<RouterConfiguration>())
{
  if (distributed_) {
    dist_pool_.emplace(1);
  }
  db_ = db;
  logger_ = logger;
  service_registry_ = service_registry;
  dist_ = dist;
  stt_builder_ = stt_builder;
  design_ = std::make_unique<frDesign>(logger_, router_cfg_.get());
  dist->addCallBack(new RoutingCallBack(this, dist, logger));
  service_registry_->provide<PinAccessService>(this);
}

TritonRoute::~TritonRoute()
{
  service_registry_->withdraw<PinAccessService>(this);
}

void TritonRoute::updateDirtyPinAccess()
{
  if (design_ == nullptr || design_->getTopBlock() == nullptr) {
    return;
  }
  updateDirtyPAData();
}

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
  utl::Timer timer;
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
    logger_->error(DRT, 626, "Guide loading failed.");
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
  logger_->info(DRT, 501, "Runtime: {:.2f}s", timer.elapsed());
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

void TritonRoute::setMaskAwareDrc(bool enable)
{
  router_cfg_->MASK_AWARE_DRC = enable;
}

void TritonRoute::setMaskDifferentSpacing(int spacing)
{
  router_cfg_->MASK_DIFFERENT_SPACING = spacing;
}

void TritonRoute::setMaskColorSolve(bool enable)
{
  router_cfg_->MASK_COLOR_SOLVE = enable;
}

void TritonRoute::setMaskNumColors(int num_colors)
{
  router_cfg_->MASK_NUM_COLORS = num_colors;
}

void TritonRoute::setMaskColorSolveCuts(bool enable)
{
  router_cfg_->MASK_COLOR_SOLVE_CUTS = enable;
}

namespace {

// A routed metal rectangle on a multi-mask layer, tagged with its mask
// color and owning net, used only by the mask-aware DRC audit.
struct MaskShape
{
  odb::Rect rect;
  int mask{0};
  odb::dbNet* net{nullptr};
};

// Order dbTechLayer pointers deterministically by layer number. odb
// deliberately deletes std::less for raw db pointers to avoid
// nondeterministic ordering, so containers keyed on layers need an
// explicit comparator.
struct TechLayerByNumber
{
  bool operator()(odb::dbTechLayer* a, odb::dbTechLayer* b) const
  {
    return a->getNumber() < b->getNumber();
  }
};

// Translate a via's cut-layer boxes to absolute coordinates at point (px, py).
// dbVia / dbTechVia store their boxes relative to the via origin, with the
// origin aligned to the via bbox min corner; getViaXY mirrors odb's own
// translation (placed rect = box rect shifted so the via bbox min lands at
// the path point). For wire vias the path point IS the via origin, so the
// placed cut rect is the box rect moved by (px - bbox.xMin, py - bbox.yMin).
template <typename ViaT>
void appendCutBoxes(
    ViaT* via,
    int px,
    int py,
    const std::set<odb::dbTechLayer*, TechLayerByNumber>& multi_mask_cut_layers,
    std::vector<std::pair<odb::Rect, odb::dbTechLayer*>>& out)
{
  if (via == nullptr) {
    return;
  }
  odb::dbBox* bbox = via->getBBox();
  if (bbox == nullptr) {
    return;
  }
  const int dx = px - bbox->xMin();
  const int dy = py - bbox->yMin();
  for (odb::dbBox* box : via->getBoxes()) {
    odb::dbTechLayer* blayer = box->getTechLayer();
    if (blayer == nullptr) {
      continue;
    }
    if (multi_mask_cut_layers.count(blayer) == 0) {
      continue;
    }
    odb::Rect r = box->getBox();
    r.moveDelta(dx, dy);
    out.emplace_back(r, blayer);
  }
}

// Decode a net's wire and append every routed metal path-segment rectangle
// on a multi-mask layer to per-layer buckets, tagged with its mask color.
// Mask color 0 means "uncolored" (no color assigned on the wire). When a
// multi-mask CUT layer appears in multi_mask_layers, via cut shapes on that
// layer are also collected, tagged with the via's cut mask color, so the
// mask DRC audit can check same-mask cut spacing.
void collectMaskShapes(
    odb::dbNet* net,
    const std::set<odb::dbTechLayer*, TechLayerByNumber>& multi_mask_layers,
    std::map<odb::dbTechLayer*, std::vector<MaskShape>, TechLayerByNumber>&
        shapes_by_layer)
{
  odb::dbWire* wire = net->getWire();
  if (wire == nullptr) {
    return;
  }
  odb::dbWireDecoder decoder;
  decoder.begin(wire);
  odb::dbWireDecoder::OpCode opcode = decoder.next();
  odb::dbTechLayer* layer = nullptr;
  bool has_prev = false;
  int prev_x = 0, prev_y = 0;
  int cur_color = 0;  // 0 == uncolored
  int cur_x = 0, cur_y = 0;
  bool has_pt = false;
  while (opcode != odb::dbWireDecoder::END_DECODE) {
    switch (opcode) {
      case odb::dbWireDecoder::PATH:
      case odb::dbWireDecoder::JUNCTION:
      case odb::dbWireDecoder::SHORT:
      case odb::dbWireDecoder::VWIRE:
        layer = decoder.getLayer();
        has_prev = false;
        cur_color = decoder.getColor().value_or(0);
        break;
      case odb::dbWireDecoder::POINT:
      case odb::dbWireDecoder::POINT_EXT: {
        int x, y, ext = 0;
        if (opcode == odb::dbWireDecoder::POINT) {
          decoder.getPoint(x, y);
        } else {
          decoder.getPoint(x, y, ext);
        }
        // refresh color in case it changed mid-path
        cur_color = decoder.getColor().value_or(0);
        if (has_prev && layer != nullptr
            && multi_mask_layers.count(layer) != 0) {
          const int half_w = static_cast<int>(layer->getWidth()) / 2;
          const int xl = std::min(prev_x, x) - half_w;
          const int yl = std::min(prev_y, y) - half_w;
          const int xh = std::max(prev_x, x) + half_w;
          const int yh = std::max(prev_y, y) + half_w;
          shapes_by_layer[layer].push_back(
              MaskShape{odb::Rect(xl, yl, xh, yh), cur_color, net});
        }
        prev_x = x;
        prev_y = y;
        has_prev = true;
        cur_x = x;
        cur_y = y;
        has_pt = true;
        break;
      }
      case odb::dbWireDecoder::VIA:
      case odb::dbWireDecoder::TECH_VIA: {
        // Vias change the active layer; the next PATH/POINT re-establishes
        // geometry. If a multi-mask CUT layer is being audited, also emit the
        // via's cut shapes tagged with its cut mask color.
        if (has_pt) {
          std::optional<odb::dbWireDecoder::ViaColor> vc
              = decoder.getViaColor();
          const int cut_col = vc ? static_cast<int>(vc->cut_color) : 0;
          std::vector<std::pair<odb::Rect, odb::dbTechLayer*>> cut_boxes;
          if (opcode == odb::dbWireDecoder::VIA) {
            appendCutBoxes(
                decoder.getVia(), cur_x, cur_y, multi_mask_layers, cut_boxes);
          } else {
            appendCutBoxes(decoder.getTechVia(),
                           cur_x,
                           cur_y,
                           multi_mask_layers,
                           cut_boxes);
          }
          for (const auto& [rect, clayer] : cut_boxes) {
            shapes_by_layer[clayer].push_back(MaskShape{rect, cut_col, net});
          }
        }
        has_prev = false;
        break;
      }
      default:
        break;
    }
    opcode = decoder.next();
  }
}

}  // namespace

int TritonRoute::checkMaskDRC(const char* filename,
                              int x1,
                              int y1,
                              int x2,
                              int y2)
{
  if (!router_cfg_->MASK_AWARE_DRC) {
    logger_->error(DRT,
                   630,
                   "check_mask_drc requires mask-aware checking to be "
                   "enabled (set_mask_aware_routing -enable).");
  }
  odb::dbChip* chip = db_->getChip();
  if (chip == nullptr || chip->getBlock() == nullptr) {
    logger_->error(DRT, 631, "No design loaded for check_mask_drc.");
  }
  odb::dbBlock* block = chip->getBlock();
  odb::dbTech* tech = db_->getTech();

  // Identify multi-mask routing layers.
  std::set<odb::dbTechLayer*, TechLayerByNumber> multi_mask_layers;
  for (odb::dbTechLayer* layer : tech->getLayers()) {
    if (layer->getType() == odb::dbTechLayerType::ROUTING
        && layer->getNumMasks() > 1) {
      multi_mask_layers.insert(layer);
    }
  }
  // Also audit multi-mask CUT layers (multi-patterning slice 6). Cut shapes
  // are gathered from via cuts by collectMaskShapes and checked against the
  // same same-mask / different-mask spacing logic. A single-patterned design
  // has no such layers so this is a no-op there.
  for (odb::dbTechLayer* layer : tech->getLayers()) {
    if (layer->getType() == odb::dbTechLayerType::CUT
        && layer->getNumMasks() > 1) {
      multi_mask_layers.insert(layer);
    }
  }
  if (multi_mask_layers.empty()) {
    logger_->warn(DRT,
                  632,
                  "check_mask_drc: no multi-mask (NUMMASKS>1) routing "
                  "layers found; nothing to check.");
  }

  // Collect routed shapes per multi-mask layer, tagged with mask color.
  std::map<odb::dbTechLayer*, std::vector<MaskShape>, TechLayerByNumber>
      shapes_by_layer;
  for (odb::dbNet* net : block->getNets()) {
    collectMaskShapes(net, multi_mask_layers, shapes_by_layer);
  }

  const odb::Rect query_box(x1, y1, x2, y2);
  const bool has_query_box = query_box.area() != 0;

  using value_t = std::pair<odb::Rect, int>;  // (rect, index)
  using rtree_t
      = boost::geometry::index::rtree<value_t,
                                      boost::geometry::index::quadratic<16>>;

  odb::dbMarkerCategory* category
      = odb::dbMarkerCategory::createOrReplace(block, "Mask DRC");
  category->setSource("DRT");

  int total_violations = 0;
  std::ofstream report;
  if (filename != nullptr && filename[0] != '\0') {
    report.open(filename);
  }

  for (odb::dbTechLayer* layer : multi_mask_layers) {
    auto it = shapes_by_layer.find(layer);
    if (it == shapes_by_layer.end()) {
      continue;
    }
    const std::vector<MaskShape>& shapes = it->second;
    // Same-mask required spacing = the layer minimum spacing. Different-mask
    // pairs use the relaxed different-mask spacing (MASK_DIFFERENT_SPACING,
    // default 0 = always legal down to a short).
    const int same_mask_spc = layer->getSpacing();
    if (same_mask_spc <= 0) {
      continue;
    }
    const int diff_mask_spc = router_cfg_->MASK_DIFFERENT_SPACING;
    // Neighbors must be queried out to the largest spacing that can produce a
    // violation, so the relaxed different-mask spacing widens the bloat when
    // it is larger than the same-mask spacing (it normally is not).
    const int query_spc = std::max(same_mask_spc, diff_mask_spc);
    // Build one R-tree over all shapes on this layer.
    rtree_t rtree;
    for (int i = 0; i < (int) shapes.size(); i++) {
      rtree.insert({shapes[i].rect, i});
    }
    for (int i = 0; i < (int) shapes.size(); i++) {
      const MaskShape& a = shapes[i];
      if (has_query_box && !query_box.intersects(a.rect)) {
        continue;
      }
      // Query a box bloated by the (max) required spacing.
      odb::Rect bloated = a.rect;
      bloated.set_xlo(bloated.xMin() - query_spc);
      bloated.set_ylo(bloated.yMin() - query_spc);
      bloated.set_xhi(bloated.xMax() + query_spc);
      bloated.set_yhi(bloated.yMax() + query_spc);
      std::vector<value_t> hits;
      rtree.query(boost::geometry::index::intersects(bloated),
                  std::back_inserter(hits));
      for (const value_t& hit : hits) {
        const int j = hit.second;
        if (j <= i) {
          continue;  // unordered pairs, avoid double counting and self
        }
        const MaskShape& b = shapes[j];
        // Touching/overlapping shapes of the same net are connected, skip.
        if (a.net == b.net && a.rect.overlaps(b.rect)) {
          continue;
        }
        // Both shapes must be colored to make a mask-based spacing decision.
        if (a.mask == 0 || b.mask == 0) {
          continue;
        }
        // Pick the required spacing by mask relationship: same color needs the
        // full same-mask spacing; different colors only need the relaxed
        // different-mask spacing (which may be 0 = always legal).
        const bool same_mask = (a.mask == b.mask);
        const int req_spc = same_mask ? same_mask_spc : diff_mask_spc;
        if (req_spc <= 0) {
          continue;  // no constraint (e.g. relaxed diff-mask spacing == 0)
        }
        // Compute the edge-to-edge gap. Negative/zero means overlap, which
        // is a short rather than a spacing violation; skip (handled by
        // ordinary DRC).
        const int dx = std::max(
            {a.rect.xMin() - b.rect.xMax(), b.rect.xMin() - a.rect.xMax(), 0});
        const int dy = std::max(
            {a.rect.yMin() - b.rect.yMax(), b.rect.yMin() - a.rect.yMax(), 0});
        if (dx == 0 && dy == 0) {
          continue;  // overlap / short
        }
        const int64_t gap2 = (int64_t) dx * dx + (int64_t) dy * dy;
        if (gap2 >= (int64_t) req_spc * req_spc) {
          continue;  // spacing satisfied
        }
        // Mask spacing violation (same-mask gap < same-mask spacing, or
        // different-mask gap < relaxed different-mask spacing).
        total_violations++;
        odb::Rect viol = a.rect;
        viol.merge(b.rect);
        if (report.is_open()) {
          report << "violation type: Mask Spacing\n";
          report << "\tsrcs: " << a.net->getName() << " " << b.net->getName()
                 << "\n";
          if (same_mask) {
            report << "\tmask: " << a.mask << "\n";
          } else {
            report << "\tmask: " << a.mask << " " << b.mask << "\n";
          }
          report << "\tbbox = (" << viol.xMin() << ", " << viol.yMin()
                 << ") - (" << viol.xMax() << ", " << viol.yMax()
                 << ") on Layer " << layer->getName() << "\n";
        }
        odb::dbMarkerCategory* layer_cat = odb::dbMarkerCategory::createOrGet(
            category, layer->getName().c_str());
        odb::dbMarker* marker = odb::dbMarker::create(layer_cat);
        if (marker != nullptr) {
          marker->setTechLayer(layer);
          marker->addShape(viol);
          marker->addSource(a.net);
          marker->addSource(b.net);
        }
      }
    }
  }
  if (report.is_open()) {
    report.close();
  }
  logger_->info(DRT,
                633,
                "check_mask_drc: found {} mask spacing violation(s) "
                "across {} multi-mask layer(s).",
                total_violations,
                multi_mask_layers.size());
  return total_violations;
}

namespace {

// One node of the mask conflict graph: a routed metal path-segment on a
// multi-mask layer. node_id is a global index; (net, seg_ordinal) identifies
// the segment within its wire's decode order so the solved color can be
// written back. solved_color is 0 until the solver assigns it (1..k).
struct ConflictNode
{
  odb::Rect rect;
  odb::dbNet* net{nullptr};
  odb::dbTechLayer* layer{nullptr};
  int seg_ordinal{0};   // index of this path-segment within net's wire decode
  int solved_color{0};  // 0 = unassigned/uncolorable
  bool is_cut{false};   // true for a CUT-layer (via) node; false for metal
  int via_ordinal{0};   // index of the via within net's wire decode (cuts)
};

// Collect conflict-graph nodes from a net's wire: every routed metal
// path-segment on a multi-mask layer, in decode order. seg_ordinal counts
// path-segments per net (each POINT after the first POINT of a PATH closes a
// segment). This ordinal scheme is replayed verbatim during write-back so the
// solved color lands on the right segment regardless of its existing color.
void collectConflictNodes(
    odb::dbNet* net,
    const std::set<odb::dbTechLayer*, TechLayerByNumber>& multi_mask_layers,
    std::vector<ConflictNode>& nodes)
{
  odb::dbWire* wire = net->getWire();
  if (wire == nullptr) {
    return;
  }
  odb::dbWireDecoder decoder;
  decoder.begin(wire);
  odb::dbWireDecoder::OpCode opcode = decoder.next();
  odb::dbTechLayer* layer = nullptr;
  bool has_prev = false;
  int prev_x = 0, prev_y = 0;
  int seg_ordinal = 0;  // counts every path-segment (colored or not)
  while (opcode != odb::dbWireDecoder::END_DECODE) {
    switch (opcode) {
      case odb::dbWireDecoder::PATH:
      case odb::dbWireDecoder::JUNCTION:
      case odb::dbWireDecoder::SHORT:
      case odb::dbWireDecoder::VWIRE:
        layer = decoder.getLayer();
        has_prev = false;
        break;
      case odb::dbWireDecoder::POINT:
      case odb::dbWireDecoder::POINT_EXT: {
        int x, y, ext = 0;
        if (opcode == odb::dbWireDecoder::POINT) {
          decoder.getPoint(x, y);
        } else {
          decoder.getPoint(x, y, ext);
        }
        if (has_prev && layer != nullptr) {
          const bool multi = multi_mask_layers.count(layer) != 0;
          if (multi) {
            const int half_w = static_cast<int>(layer->getWidth()) / 2;
            const int xl = std::min(prev_x, x) - half_w;
            const int yl = std::min(prev_y, y) - half_w;
            const int xh = std::max(prev_x, x) + half_w;
            const int yh = std::max(prev_y, y) + half_w;
            ConflictNode n;
            n.rect = odb::Rect(xl, yl, xh, yh);
            n.net = net;
            n.layer = layer;
            n.seg_ordinal = seg_ordinal;
            nodes.push_back(n);
          }
          // Every path-segment (multi-mask or not) advances the ordinal so
          // the write-back replay stays aligned with this decode order.
          seg_ordinal++;
        }
        prev_x = x;
        prev_y = y;
        has_prev = true;
        break;
      }
      case odb::dbWireDecoder::VIA:
      case odb::dbWireDecoder::TECH_VIA:
        has_prev = false;
        break;
      default:
        break;
    }
    opcode = decoder.next();
  }
}

// Collect conflict-graph nodes from a net's wire for CUT/VIA layers: every
// via cut shape on a multi-mask CUT layer, in decode order. via_ordinal counts
// every VIA/TECH_VIA opcode per net (regardless of layer) so the solved cut
// color can be replayed onto the right via during write-back. A via may drop
// several cut rects (a multi-cut via) onto the same cut layer; they all share
// the via's cut color, so they share via_ordinal and are colored together.
void collectCutConflictNodes(
    odb::dbNet* net,
    const std::set<odb::dbTechLayer*, TechLayerByNumber>& multi_mask_cut_layers,
    std::vector<ConflictNode>& nodes)
{
  odb::dbWire* wire = net->getWire();
  if (wire == nullptr) {
    return;
  }
  odb::dbWireDecoder decoder;
  decoder.begin(wire);
  odb::dbWireDecoder::OpCode opcode = decoder.next();
  bool has_pt = false;
  int cur_x = 0, cur_y = 0;
  int via_ordinal = 0;  // counts every via (colored or not)
  while (opcode != odb::dbWireDecoder::END_DECODE) {
    switch (opcode) {
      case odb::dbWireDecoder::PATH:
      case odb::dbWireDecoder::JUNCTION:
      case odb::dbWireDecoder::SHORT:
      case odb::dbWireDecoder::VWIRE:
        has_pt = false;
        break;
      case odb::dbWireDecoder::POINT:
      case odb::dbWireDecoder::POINT_EXT: {
        int x, y, ext = 0;
        if (opcode == odb::dbWireDecoder::POINT) {
          decoder.getPoint(x, y);
        } else {
          decoder.getPoint(x, y, ext);
        }
        cur_x = x;
        cur_y = y;
        has_pt = true;
        break;
      }
      case odb::dbWireDecoder::VIA:
      case odb::dbWireDecoder::TECH_VIA: {
        if (has_pt) {
          std::vector<std::pair<odb::Rect, odb::dbTechLayer*>> cut_boxes;
          if (opcode == odb::dbWireDecoder::VIA) {
            appendCutBoxes(decoder.getVia(),
                           cur_x,
                           cur_y,
                           multi_mask_cut_layers,
                           cut_boxes);
          } else {
            appendCutBoxes(decoder.getTechVia(),
                           cur_x,
                           cur_y,
                           multi_mask_cut_layers,
                           cut_boxes);
          }
          for (const auto& [rect, layer] : cut_boxes) {
            ConflictNode n;
            n.rect = rect;
            n.net = net;
            n.layer = layer;
            n.is_cut = true;
            n.via_ordinal = via_ordinal;
            nodes.push_back(n);
          }
        }
        // Every via advances the ordinal so the write-back replay stays
        // aligned with this decode order.
        via_ordinal++;
        // The via does not establish a new path point for the next segment.
        break;
      }
      default:
        break;
    }
    opcode = decoder.next();
  }
}
int64_t gapSquared(const odb::Rect& a, const odb::Rect& b)
{
  const int dx = std::max({a.xMin() - b.xMax(), b.xMin() - a.xMax(), 0});
  const int dy = std::max({a.yMin() - b.yMax(), b.yMin() - a.yMax(), 0});
  return (int64_t) dx * dx + (int64_t) dy * dy;
}

// Greedy + backtracking k-coloring over one connected component. nodes are
// global ConflictNode indices; adj is the global adjacency list. Returns true
// and fills color[] (values 1..k) on success; false if the component is not
// k-colorable. Bounded: orders vertices by descending degree (a good static
// heuristic) and backtracks. For k=2 this is exact (bipartite check); for
// k>=3 it is a complete search but only over one component, which for routed
// layouts is small.
bool colorComponent(const std::vector<int>& comp,
                    const std::vector<std::vector<int>>& adj,
                    int k,
                    std::vector<int>& color)
{
  // Order component vertices by descending degree for a tighter search.
  std::vector<int> order = comp;
  std::sort(order.begin(), order.end(), [&](int a, int b) {
    if (adj[a].size() != adj[b].size()) {
      return adj[a].size() > adj[b].size();
    }
    return a < b;  // deterministic tie-break
  });

  std::function<bool(size_t)> assign = [&](size_t idx) -> bool {
    if (idx == order.size()) {
      return true;
    }
    const int v = order[idx];
    for (int c = 1; c <= k; c++) {
      bool ok = true;
      for (int u : adj[v]) {
        if (color[u] == c) {
          ok = false;
          break;
        }
      }
      if (!ok) {
        continue;
      }
      color[v] = c;
      if (assign(idx + 1)) {
        return true;
      }
      color[v] = 0;  // backtrack
    }
    return false;
  };
  return assign(0);
}

// Re-encode net's wire faithfully, overriding the mask color of each
// multi-mask path-segment with seg_color[seg_ordinal] (1..k, or 0 to clear)
// and the CUT mask color of each via with via_color[via_ordinal] (1..k). The
// via's bottom/top mask colors are preserved from the original wire; only the
// cut color is overridden. Returns true if the wire was rewritten; false (and
// leaves the wire untouched) if it contains opcodes this faithful-copy path
// does not handle, so the solver never corrupts a wire it cannot reproduce
// exactly.
//
// Byte-identity guarantee: when via_color is empty (cut coloring off), the
// via opcodes are emitted exactly as before -- no setViaColor/clearViaColor
// call is made and any pre-existing via color is reproduced verbatim -- so
// the metal-only write-back path is unchanged.
//
// Handled opcodes: PATH/JUNCTION/SHORT/VWIRE (+ optional RULE), POINT,
// POINT_EXT, VIA, TECH_VIA, ITERM, BTERM. Unhandled (e.g. RECT patch wires)
// abort the rewrite for that net.
bool rewriteWireColors(
    odb::dbNet* net,
    odb::dbTech* db_tech,
    const std::set<odb::dbTechLayer*, TechLayerByNumber>& multi_mask_layers,
    const std::map<int, int>& seg_color,
    const std::map<int, int>& via_color)
{
  odb::dbWire* wire = net->getWire();
  if (wire == nullptr) {
    return true;  // nothing to do
  }

  // First pass: scan for unsupported opcodes. If found, refuse to rewrite.
  {
    odb::dbWireDecoder scan;
    scan.begin(wire);
    for (odb::dbWireDecoder::OpCode op = scan.next();
         op != odb::dbWireDecoder::END_DECODE;
         op = scan.next()) {
      switch (op) {
        case odb::dbWireDecoder::PATH:
        case odb::dbWireDecoder::JUNCTION:
        case odb::dbWireDecoder::SHORT:
        case odb::dbWireDecoder::VWIRE:
        case odb::dbWireDecoder::POINT:
        case odb::dbWireDecoder::POINT_EXT:
        case odb::dbWireDecoder::VIA:
        case odb::dbWireDecoder::TECH_VIA:
        case odb::dbWireDecoder::ITERM:
        case odb::dbWireDecoder::BTERM:
        case odb::dbWireDecoder::RULE:
          break;
        default:
          // RECT (patch wire) or anything else: do not attempt a rewrite.
          return false;
      }
    }
  }

  odb::dbWireDecoder decoder;
  decoder.begin(wire);
  odb::dbWireEncoder encoder;
  encoder.begin(wire);

  odb::dbTechLayer* layer = nullptr;
  int seg_ordinal = 0;
  int via_ordinal = 0;
  bool path_open = false;
  bool path_has_prev = false;  // a point has already been emitted on this path
  int pending_color = 0;       // color to apply to the next segment we emit
  // Tracks the via color currently programmed into the encoder so we only
  // emit setViaColor/clearViaColor when it actually changes (mirrors the
  // metal pending_color discipline and keeps emission minimal).
  bool via_color_pending = false;  // a non-default via color is programmed
  odb::dbWireDecoder::ViaColor pending_via_color{0, 0, 0};

  // Emit the via color statements needed so that the encoder's active via
  // color matches `want` (or default when has_want is false). Only emits when
  // the state changes, preserving byte-identity when nothing differs.
  auto syncViaColor = [&](bool has_want,
                          const odb::dbWireDecoder::ViaColor& want) {
    if (has_want) {
      if (!via_color_pending
          || pending_via_color.bottom_color != want.bottom_color
          || pending_via_color.cut_color != want.cut_color
          || pending_via_color.top_color != want.top_color) {
        encoder.setViaColor(want.bottom_color, want.cut_color, want.top_color);
        via_color_pending = true;
        pending_via_color = want;
      }
    } else {
      if (via_color_pending) {
        encoder.clearViaColor();
        via_color_pending = false;
      }
    }
  };

  for (odb::dbWireDecoder::OpCode op = decoder.next();
       op != odb::dbWireDecoder::END_DECODE;
       op = decoder.next()) {
    switch (op) {
      case odb::dbWireDecoder::PATH:
      case odb::dbWireDecoder::JUNCTION:
      case odb::dbWireDecoder::SHORT:
      case odb::dbWireDecoder::VWIRE: {
        layer = decoder.getLayer();
        const odb::dbWireType wtype = decoder.getWireType();
        if (op == odb::dbWireDecoder::PATH) {
          encoder.newPath(layer, wtype);
        } else if (op == odb::dbWireDecoder::JUNCTION) {
          encoder.newPath(decoder.getJunctionValue(), wtype);
        } else if (op == odb::dbWireDecoder::SHORT) {
          encoder.newPathShort(decoder.getJunctionValue(), layer, wtype);
        } else {  // VWIRE
          encoder.newPathVirtualWire(decoder.getJunctionValue(), layer, wtype);
        }
        path_open = true;
        path_has_prev = false;
        pending_color = 0;
        break;
      }
      case odb::dbWireDecoder::POINT:
      case odb::dbWireDecoder::POINT_EXT: {
        int x, y, ext = 0;
        const bool has_ext = (op == odb::dbWireDecoder::POINT_EXT);
        if (has_ext) {
          decoder.getPoint(x, y, ext);
        } else {
          decoder.getPoint(x, y);
        }
        const bool is_multi
            = layer != nullptr && multi_mask_layers.count(layer) != 0;
        // A point that is NOT the path's first point closes a segment (this
        // mirrors collectConflictNodes' has_prev gating exactly). Only such
        // segment-closing points carry a solved color and advance the ordinal.
        const bool closes_segment = path_open && path_has_prev;
        if (closes_segment && is_multi) {
          auto cit = seg_color.find(seg_ordinal);
          const int col = (cit != seg_color.end()) ? cit->second : 0;
          if (col != pending_color) {
            if (col != 0) {
              encoder.setColor(static_cast<uint8_t>(col));
            } else {
              encoder.clearColor();
            }
            pending_color = col;
          }
        }
        if (has_ext) {
          encoder.addPoint(x, y, ext);
        } else {
          encoder.addPoint(x, y);
        }
        if (closes_segment) {
          seg_ordinal++;
        }
        path_has_prev = true;
        break;
      }
      case odb::dbWireDecoder::VIA:
      case odb::dbWireDecoder::TECH_VIA: {
        // Determine the via color to emit. Start from whatever color the
        // original wire carried for this via (so bottom/top are preserved and,
        // critically, an untouched via is reproduced byte-for-byte). If the
        // solver assigned a cut color for this via, override the cut digit.
        std::optional<odb::dbWireDecoder::ViaColor> orig
            = decoder.getViaColor();
        auto vit = via_color.find(via_ordinal);
        if (vit != via_color.end()) {
          odb::dbWireDecoder::ViaColor want{0, 0, 0};
          if (orig) {
            want = *orig;
          }
          want.cut_color = static_cast<uint8_t>(vit->second);
          syncViaColor(true, want);
        } else if (orig) {
          syncViaColor(true, *orig);
        } else {
          syncViaColor(false, pending_via_color);
        }
        if (op == odb::dbWireDecoder::VIA) {
          encoder.addVia(decoder.getVia());
        } else {
          encoder.addTechVia(decoder.getTechVia());
        }
        via_ordinal++;
        path_has_prev = false;
        break;
      }
      case odb::dbWireDecoder::ITERM:
        encoder.addITerm(decoder.getITerm());
        break;
      case odb::dbWireDecoder::BTERM:
        encoder.addBTerm(decoder.getBTerm());
        break;
      case odb::dbWireDecoder::RULE:
        // Non-default rule: re-applied implicitly by re-opening paths with
        // the decoded wire type; explicit rule preservation is out of scope
        // and such wires were filtered by the scan pass when they carry a
        // rule opcode we cannot reproduce. (RULE alone is tolerated.)
        break;
      default:
        encoder.clear();
        return false;
    }
  }
  (void) db_tech;
  encoder.end();
  return true;
}

}  // namespace

int TritonRoute::solveMaskColoring(const char* filename,
                                   int x1,
                                   int y1,
                                   int x2,
                                   int y2)
{
  if (!router_cfg_->MASK_COLOR_SOLVE) {
    logger_->error(DRT,
                   636,
                   "solve_mask_coloring requires the mask-coloring solver to "
                   "be enabled (set_mask_aware_routing -solve_coloring).");
  }
  odb::dbChip* chip = db_->getChip();
  if (chip == nullptr || chip->getBlock() == nullptr) {
    logger_->error(DRT, 637, "No design loaded for solve_mask_coloring.");
  }
  odb::dbBlock* block = chip->getBlock();
  odb::dbTech* tech = db_->getTech();

  // Identify multi-mask routing layers.
  std::set<odb::dbTechLayer*, TechLayerByNumber> multi_mask_layers;
  for (odb::dbTechLayer* layer : tech->getLayers()) {
    if (layer->getType() == odb::dbTechLayerType::ROUTING
        && layer->getNumMasks() > 1) {
      multi_mask_layers.insert(layer);
    }
  }
  if (multi_mask_layers.empty() && !router_cfg_->MASK_COLOR_SOLVE_CUTS) {
    logger_->warn(DRT,
                  638,
                  "solve_mask_coloring: no multi-mask (NUMMASKS>1) routing "
                  "layers found; nothing to solve.");
    return 0;
  }

  const odb::Rect query_box(x1, y1, x2, y2);
  const bool has_query_box = query_box.area() != 0;

  std::ofstream report;
  if (filename != nullptr && filename[0] != '\0') {
    report.open(filename);
  }

  int total_uncolorable = 0;
  int total_colored = 0;

  // Accumulate solved colors across all layers, keyed by net then by the
  // net-global path-segment ordinal, so the write-back can rewrite each wire
  // exactly once (a net may carry colored segments on several multi-mask
  // layers). odb deletes std::less for raw db pointers, so use net id.
  struct NetById
  {
    bool operator()(odb::dbNet* a, odb::dbNet* b) const
    {
      return a->getId() < b->getId();
    }
  };
  std::map<odb::dbNet*, std::map<int, int>, NetById> net_seg_color;

  // Solve per layer (conflicts are intra-layer; vias/cuts out of scope).
  for (odb::dbTechLayer* layer : multi_mask_layers) {
    const int same_mask_spc = layer->getSpacing();
    if (same_mask_spc <= 0) {
      continue;
    }
    // k = requested colors, clamped to what the technology declares.
    const int layer_masks = static_cast<int>(layer->getNumMasks());
    int k = router_cfg_->MASK_NUM_COLORS;
    if (k < 2) {
      k = 2;
    }
    if (k > layer_masks) {
      k = layer_masks;
    }

    // Gather this layer's nodes (one per multi-mask path-segment).
    std::vector<ConflictNode> nodes;
    for (odb::dbNet* net : block->getNets()) {
      std::vector<ConflictNode> net_nodes;
      collectConflictNodes(net, multi_mask_layers, net_nodes);
      for (const ConflictNode& n : net_nodes) {
        if (n.layer != layer) {
          continue;
        }
        if (has_query_box && !query_box.intersects(n.rect)) {
          continue;
        }
        nodes.push_back(n);
      }
    }
    if (nodes.empty()) {
      continue;
    }

    // Build the conflict graph: edge between two nodes whose edge-to-edge gap
    // is below the same-mask spacing (they must therefore differ in mask).
    // Overlapping same-net segments are connected metal (no edge). Use an
    // R-tree for proximity.
    using value_t = std::pair<odb::Rect, int>;
    using rtree_t
        = boost::geometry::index::rtree<value_t,
                                        boost::geometry::index::quadratic<16>>;
    rtree_t rtree;
    for (int i = 0; i < (int) nodes.size(); i++) {
      rtree.insert({nodes[i].rect, i});
    }
    std::vector<std::vector<int>> adj(nodes.size());
    const int64_t spc2 = (int64_t) same_mask_spc * same_mask_spc;
    for (int i = 0; i < (int) nodes.size(); i++) {
      odb::Rect bloated = nodes[i].rect;
      bloated.set_xlo(bloated.xMin() - same_mask_spc);
      bloated.set_ylo(bloated.yMin() - same_mask_spc);
      bloated.set_xhi(bloated.xMax() + same_mask_spc);
      bloated.set_yhi(bloated.yMax() + same_mask_spc);
      std::vector<value_t> hits;
      rtree.query(boost::geometry::index::intersects(bloated),
                  std::back_inserter(hits));
      for (const value_t& hit : hits) {
        const int j = hit.second;
        if (j <= i) {
          continue;
        }
        // Connected same-net metal that overlaps is not a conflict.
        if (nodes[i].net == nodes[j].net
            && nodes[i].rect.overlaps(nodes[j].rect)) {
          continue;
        }
        const int64_t g2 = gapSquared(nodes[i].rect, nodes[j].rect);
        if (g2 == 0) {
          continue;  // overlap / short: ordinary DRC territory, not a mask edge
        }
        if (g2 < spc2) {
          adj[i].push_back(j);
          adj[j].push_back(i);
        }
      }
    }

    // Connected components via BFS; color each independently.
    std::vector<int> color(nodes.size(), 0);
    std::vector<char> visited(nodes.size(), 0);
    int layer_uncolorable = 0;
    for (int s = 0; s < (int) nodes.size(); s++) {
      if (visited[s]) {
        continue;
      }
      std::vector<int> comp;
      std::vector<int> stack = {s};
      visited[s] = 1;
      while (!stack.empty()) {
        const int v = stack.back();
        stack.pop_back();
        comp.push_back(v);
        for (int u : adj[v]) {
          if (!visited[u]) {
            visited[u] = 1;
            stack.push_back(u);
          }
        }
      }
      std::vector<int> comp_color(nodes.size(), 0);
      // Carry already-decided colors for vertices in this component.
      for (int v : comp) {
        comp_color[v] = color[v];
      }
      const bool ok = colorComponent(comp, adj, k, comp_color);
      if (ok) {
        for (int v : comp) {
          color[v] = comp_color[v];
        }
      } else {
        // Uncolorable component (e.g. an odd cycle when k=2): REPORT it and
        // leave its nodes uncolored rather than emitting an illegal coloring.
        layer_uncolorable++;
        total_uncolorable++;
        odb::Rect bbox = nodes[comp.front()].rect;
        for (int v : comp) {
          bbox.merge(nodes[v].rect);
        }
        if (report.is_open()) {
          report << "uncolorable conflict\n";
          report << "\tlayer: " << layer->getName() << "\n";
          report << "\tcolors: " << k << "\n";
          report << "\tnodes: " << comp.size() << "\n";
          report << "\tbbox = (" << bbox.xMin() << ", " << bbox.yMin()
                 << ") - (" << bbox.xMax() << ", " << bbox.yMax() << ")\n";
        }
        logger_->warn(DRT,
                      639,
                      "solve_mask_coloring: uncolorable {}-coloring conflict "
                      "on layer {} ({} shapes); not emitting an illegal "
                      "coloring for this component.",
                      k,
                      layer->getName(),
                      comp.size());
      }
    }

    // Accumulate this layer's solved colors. Only colored components
    // contribute; uncolorable components stay uncolored.
    for (int i = 0; i < (int) nodes.size(); i++) {
      if (color[i] != 0) {
        net_seg_color[nodes[i].net][nodes[i].seg_ordinal] = color[i];
        total_colored++;
      }
    }
  }

  // ---- CUT/VIA layers (multi-patterning slice 6) ----------------------------
  // Gated by MASK_COLOR_SOLVE_CUTS (default off). When off, no cut layer is
  // even identified, no cut node is collected, and net_via_color stays empty,
  // so the write-back emits zero via MASK tokens -- the metal-only solver
  // result is byte-identical. Cuts are colored by exactly the same
  // conflict-graph + k-coloring machinery as metal: nodes are via cut shapes,
  // an edge is a pair of cuts closer than the cut layer's same-mask spacing
  // (hence required on different masks), and uncolorable components are
  // REPORTED, never force-colored.
  std::map<odb::dbNet*, std::map<int, int>, NetById> net_via_color;
  std::set<odb::dbTechLayer*, TechLayerByNumber> multi_mask_cut_layers;
  if (router_cfg_->MASK_COLOR_SOLVE_CUTS) {
    for (odb::dbTechLayer* layer : tech->getLayers()) {
      if (layer->getType() == odb::dbTechLayerType::CUT
          && layer->getNumMasks() > 1) {
        multi_mask_cut_layers.insert(layer);
      }
    }
    for (odb::dbTechLayer* layer : multi_mask_cut_layers) {
      const int same_mask_spc = layer->getSpacing();
      if (same_mask_spc <= 0) {
        continue;
      }
      const int layer_masks = static_cast<int>(layer->getNumMasks());
      int k = router_cfg_->MASK_NUM_COLORS;
      if (k < 2) {
        k = 2;
      }
      if (k > layer_masks) {
        k = layer_masks;
      }

      // Gather this cut layer's nodes (one per via cut shape on the layer).
      std::vector<ConflictNode> nodes;
      for (odb::dbNet* net : block->getNets()) {
        std::vector<ConflictNode> net_nodes;
        collectCutConflictNodes(net, multi_mask_cut_layers, net_nodes);
        for (const ConflictNode& n : net_nodes) {
          if (n.layer != layer) {
            continue;
          }
          if (has_query_box && !query_box.intersects(n.rect)) {
            continue;
          }
          nodes.push_back(n);
        }
      }
      if (nodes.empty()) {
        continue;
      }

      // Build the conflict graph (same rule as metal: edge = cut-to-cut gap
      // below the same-mask cut spacing). Overlapping same-via cuts are not a
      // conflict (they belong to one via and share its color).
      using value_t = std::pair<odb::Rect, int>;
      using rtree_t = boost::geometry::index::
          rtree<value_t, boost::geometry::index::quadratic<16>>;
      rtree_t rtree;
      for (int i = 0; i < (int) nodes.size(); i++) {
        rtree.insert({nodes[i].rect, i});
      }
      std::vector<std::vector<int>> adj(nodes.size());
      const int64_t spc2 = (int64_t) same_mask_spc * same_mask_spc;
      for (int i = 0; i < (int) nodes.size(); i++) {
        odb::Rect bloated = nodes[i].rect;
        bloated.set_xlo(bloated.xMin() - same_mask_spc);
        bloated.set_ylo(bloated.yMin() - same_mask_spc);
        bloated.set_xhi(bloated.xMax() + same_mask_spc);
        bloated.set_yhi(bloated.yMax() + same_mask_spc);
        std::vector<value_t> hits;
        rtree.query(boost::geometry::index::intersects(bloated),
                    std::back_inserter(hits));
        for (const value_t& hit : hits) {
          const int j = hit.second;
          if (j <= i) {
            continue;
          }
          // Two cuts that belong to the same via on the same net share the
          // via's single cut color, so they must never be a conflict edge
          // (forcing them apart would be unsolvable and meaningless).
          if (nodes[i].net == nodes[j].net
              && nodes[i].via_ordinal == nodes[j].via_ordinal) {
            continue;
          }
          const int64_t g2 = gapSquared(nodes[i].rect, nodes[j].rect);
          if (g2 == 0) {
            continue;  // overlap/short: ordinary DRC territory, not a mask edge
          }
          if (g2 < spc2) {
            adj[i].push_back(j);
            adj[j].push_back(i);
          }
        }
      }

      // Connected components via DFS; color each independently.
      std::vector<int> color(nodes.size(), 0);
      std::vector<char> visited(nodes.size(), 0);
      for (int s = 0; s < (int) nodes.size(); s++) {
        if (visited[s]) {
          continue;
        }
        std::vector<int> comp;
        std::vector<int> stack = {s};
        visited[s] = 1;
        while (!stack.empty()) {
          const int v = stack.back();
          stack.pop_back();
          comp.push_back(v);
          for (int u : adj[v]) {
            if (!visited[u]) {
              visited[u] = 1;
              stack.push_back(u);
            }
          }
        }
        std::vector<int> comp_color(nodes.size(), 0);
        for (int v : comp) {
          comp_color[v] = color[v];
        }
        const bool ok = colorComponent(comp, adj, k, comp_color);
        if (ok) {
          for (int v : comp) {
            color[v] = comp_color[v];
          }
        } else {
          // Uncolorable cut component (e.g. an odd cycle of cuts when k=2):
          // REPORT it, leave uncolored. This is the honest DFM signal that the
          // cut layer cannot be legally decomposed at the requested colors.
          total_uncolorable++;
          odb::Rect bbox = nodes[comp.front()].rect;
          for (int v : comp) {
            bbox.merge(nodes[v].rect);
          }
          if (report.is_open()) {
            report << "uncolorable conflict\n";
            report << "\tlayer: " << layer->getName() << " (cut)\n";
            report << "\tcolors: " << k << "\n";
            report << "\tnodes: " << comp.size() << "\n";
            report << "\tbbox = (" << bbox.xMin() << ", " << bbox.yMin()
                   << ") - (" << bbox.xMax() << ", " << bbox.yMax() << ")\n";
          }
          logger_->warn(DRT,
                        643,
                        "solve_mask_coloring: uncolorable {}-coloring conflict "
                        "on cut layer {} ({} cut shapes); not emitting an "
                        "illegal coloring for this component.",
                        k,
                        layer->getName(),
                        comp.size());
        }
      }

      // Accumulate solved cut colors, keyed by via ordinal. Multiple cut
      // shapes of one via map to the same via_ordinal; they were colored
      // consistently (never edged against each other) so the assignment is
      // well-defined.
      for (int i = 0; i < (int) nodes.size(); i++) {
        if (color[i] != 0) {
          net_via_color[nodes[i].net][nodes[i].via_ordinal] = color[i];
          total_colored++;
        }
      }
    }
  }

  // Union of every net touched on metal or cut layers, so a single faithful
  // write-back per net applies both its segment and via colors at once.
  std::set<odb::dbNet*, NetById> dirty_nets;
  for (auto& [net, seg_color] : net_seg_color) {
    dirty_nets.insert(net);
  }
  for (auto& [net, via_color] : net_via_color) {
    dirty_nets.insert(net);
  }

  // Single faithful write-back per net, applying solved colors from every
  // layer at once (so a later layer's rewrite never clobbers an earlier
  // layer's colors).
  static const std::map<int, int> kEmpty;
  for (odb::dbNet* net : dirty_nets) {
    auto sit = net_seg_color.find(net);
    auto vit = net_via_color.find(net);
    const std::map<int, int>& seg_color
        = (sit != net_seg_color.end()) ? sit->second : kEmpty;
    const std::map<int, int>& via_color
        = (vit != net_via_color.end()) ? vit->second : kEmpty;
    const bool wrote
        = rewriteWireColors(net, tech, multi_mask_layers, seg_color, via_color);
    if (!wrote) {
      logger_->warn(DRT,
                    640,
                    "solve_mask_coloring: net {} has wire opcodes the "
                    "solver cannot rewrite faithfully; left uncolored.",
                    net->getName());
    }
  }

  if (report.is_open()) {
    report.close();
  }
  logger_->info(DRT,
                641,
                "solve_mask_coloring: colored {} shape(s); {} uncolorable "
                "conflict(s) across {} multi-mask layer(s).",
                total_colored,
                total_uncolorable,
                multi_mask_layers.size() + multi_mask_cut_layers.size());
  return total_uncolorable;
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
  router_cfg_->unidirectional_layer_names_.insert(layerName);
}

void TritonRoute::setParams(const ParamStruct& params)
{
  router_cfg_->OUT_MAZE_FILE = params.outputMazeFile;
  router_cfg_->DRC_RPT_FILE = params.outputDrcFile;
  router_cfg_->DRC_RPT_ITER_STEP = params.drcReportIterStep;
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

  std::vector<const frMarker*> sorted_markers;
  sorted_markers.reserve(std::distance(markers.begin(), markers.end()));
  for (const auto& marker : markers) {
    sorted_markers.push_back(marker.get());
  }
  auto marker_sort_key = [this](const frMarker* marker) {
    const odb::Rect bbox = marker->getBBox();
    auto tech = getDesign()->getTech();
    auto layer = tech->getLayer(marker->getLayerNum());
    auto con = marker->getConstraint();
    std::string viol_name = "unknown";
    if (con) {
      if (con->typeId() == frConstraintTypeEnum::frcShortConstraint
          && layer->getType() == dbTechLayerType::CUT) {
        viol_name = "Cut Short";
      } else {
        viol_name = con->getViolName();
      }
    }
    std::vector<std::string> sources;
    for (auto src : marker->getSrcs()) {
      if (!src) {
        continue;
      }
      switch (src->typeId()) {
        case frcNet:
          sources.push_back(static_cast<frNet*>(src)->getName());
          break;
        case frcInstTerm: {
          auto* inst_term = static_cast<frInstTerm*>(src);
          sources.push_back(inst_term->getInst()->getName() + "/"
                            + inst_term->getTerm()->getName());
          break;
        }
        case frcBTerm:
          sources.push_back(static_cast<frBTerm*>(src)->getName());
          break;
        default:
          sources.push_back(std::to_string(src->typeId()) + ":"
                            + std::to_string(src->getId()));
          break;
      }
    }
    std::ranges::sort(sources);
    return std::make_tuple(marker->getLayerNum(),
                           bbox.xMin(),
                           bbox.yMin(),
                           bbox.xMax(),
                           bbox.yMax(),
                           viol_name,
                           sources);
  };
  std::ranges::sort(sorted_markers,
                    [&](const frMarker* lhs, const frMarker* rhs) {
                      return marker_sort_key(lhs) < marker_sort_key(rhs);
                    });

  for (const frMarker* marker : sorted_markers) {
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
