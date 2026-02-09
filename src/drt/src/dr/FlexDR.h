// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "boost/polygon/polygon.hpp"
#include "boost/serialization/export.hpp"
#include "db/drObj/drAccessPattern.h"
#include "db/drObj/drFig.h"
#include "db/drObj/drMarker.h"
#include "db/drObj/drNet.h"
#include "db/infra/frBox.h"
#include "db/infra/frPoint.h"
#include "db/infra/frSegStyle.h"
#include "db/infra/frTime.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frInstTerm.h"
#include "db/obj/frShape.h"
#include "db/obj/frVia.h"
#include "db/tech/frConstraint.h"
#include "db/tech/frLayer.h"
#include "db/tech/frTechObject.h"
#include "db/tech/frViaDef.h"
#include "dr/AbstractDRGraphics.h"
#include "dr/FlexGridGraph.h"
#include "dr/FlexMazeTypes.h"
#include "dr/FlexWavefront.h"
#include "drt/TritonRoute.h"
#include "dst/JobMessage.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "frRegionQuery.h"
#include "gc/FlexGC.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"

using Rectangle = boost::polygon::rectangle_data<int>;
namespace dst {
class Distributed;
}
namespace odb {
class dbDatabase;
}
namespace utl {
class Logger;
}

namespace drt {

class frConstraint;
struct SearchRepairArgs;

struct FlexDRViaData
{
  // std::pair<layer1area, layer2area>
  std::vector<std::pair<frCoord, frCoord>> halfViaEncArea;

 private:
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & halfViaEncArea;
  }
  friend class boost::serialization::access;
};

class FlexDRFlow;
class FlexDR
{
 public:
  struct SearchRepairArgs
  {
    int size;
    int offset;
    int mazeEndIter;
    frUInt4 workerDRCCost;
    frUInt4 workerMarkerCost;
    frUInt4 workerFixedShapeCost;
    float workerMarkerDecay;
    RipUpMode ripupMode;
    bool followGuide;
    bool isEqualIgnoringSizeAndOffset(const SearchRepairArgs& other) const;
  };
  struct IterationProgress
  {
    int total_num_workers{0};
    int cnt_done_workers{0};
    int last_reported_perc{0};
    frTime time;
  };
  // constructors
  FlexDR(TritonRoute* router,
         frDesign* designIn,
         utl::Logger* loggerIn,
         odb::dbDatabase* dbIn,
         RouterConfiguration* router_cfg);
  ~FlexDR();
  // getters
  frTechObject* getTech() const { return design_->getTech(); }
  frDesign* getDesign() const { return design_; }
  frRegionQuery* getRegionQuery() const { return design_->getRegionQuery(); }
  // others
  void init();
  int main();
  void searchRepair(const SearchRepairArgs& args);
  void end(bool done = false);

  const FlexDRViaData* getViaData() const { return &via_data_; }
  void setDebug(std::unique_ptr<AbstractDRGraphics> dr_graphics);

  // For post-deserialization update
  void setLogger(utl::Logger* logger) { logger_ = logger; }
  void setDB(odb::dbDatabase* db) { db_ = db; }
  AbstractDRGraphics* getGraphics() { return graphics_.get(); }
  // distributed
  void setDistributed(dst::Distributed* dist,
                      const std::string& remote_ip,
                      uint16_t remote_port,
                      const std::string& dir)
  {
    dist_on_ = true;
    dist_ = dist;
    dist_ip_ = remote_ip;
    dist_port_ = remote_port;
    dist_dir_ = dir;
  }
  void sendWorkers(
      const std::vector<std::pair<int, FlexDRWorker*>>& remote_batch,
      std::vector<std::unique_ptr<FlexDRWorker>>& batch);

  void reportGuideCoverage();
  void incIter() { ++iter_; }
  // maxSpacing fix
  void fixMaxSpacing();

 private:
  std::unique_ptr<FlexDRFlow> flow_state_machine_;
  TritonRoute* router_;
  frDesign* design_;
  utl::Logger* logger_;
  odb::dbDatabase* db_;
  RouterConfiguration* router_cfg_;
  std::vector<std::vector<
      frOrderedIdMap<frNet*, std::set<std::pair<odb::Point, frLayerNum>>>>>
      gcell2BoundaryPin_;

  FlexDRViaData via_data_;
  std::vector<int> numViols_;
  std::unique_ptr<AbstractDRGraphics> graphics_{nullptr};
  std::string debugNetName_;
  int numWorkUnits_;

  // distributed
  dst::Distributed* dist_;
  bool dist_on_;
  std::string dist_ip_;
  uint16_t dist_port_;
  std::string dist_dir_;
  std::string router_cfg_path_;
  bool increaseClipsize_;
  float clipSizeInc_;
  int iter_;

  // others
  void initFromTA();
  void initGCell2BoundaryPin();
  void getBatchInfo(int& batchStepX, int& batchStepY);

  void init_halfViaEncArea();

  void removeGCell2BoundaryPin();
  frOrderedIdMap<frNet*, std::set<std::pair<odb::Point, frLayerNum>>>
  initDR_mergeBoundaryPin(int startX,
                          int startY,
                          int size,
                          const odb::Rect& routeBox) const;
  std::vector<frVia*> getLonelyVias(frLayer* layer, int max_spc, int cut_class);
  std::unique_ptr<FlexDRWorker> createWorker(int x_offset,
                                             int y_offset,
                                             const SearchRepairArgs& args,
                                             const odb::Rect& routeBox
                                             = odb::Rect());
  void reportIterationViolations() const;
  void endWorkersBatch(
      std::vector<std::unique_ptr<FlexDRWorker>>& workers_batch);
  void processWorkersBatch(
      std::vector<std::unique_ptr<FlexDRWorker>>& workers_batch,
      IterationProgress& iter_prog);

  void processWorkersBatchDistributed(
      std::vector<std::unique_ptr<FlexDRWorker>>& workers_batch,
      int& version,
      IterationProgress& iter_prog);
  odb::Rect getDRVBBox(const odb::Rect& drv_rect) const;
  void stubbornTilesFlow(const SearchRepairArgs& args,
                         IterationProgress& iter_prog);
  void guideTilesFlow(const SearchRepairArgs& args,
                      IterationProgress& iter_prog);
  void optimizationFlow(const SearchRepairArgs& args,
                        IterationProgress& iter_prog);
};

class FlexDRFlow
{
 public:
  enum class State
  {
    OPTIMIZATION,
    STUBBORN,
    GUIDES,
    SKIP
  };

  struct FlowContext
  {
    int num_violations{0};
    FlexDR::SearchRepairArgs args;
  };

  FlexDRFlow() = default;

  State determineNextFlow(const FlowContext& context);

  State getCurrentState() const;

  std::string getFlowName() const;

  void setLastIterationEffective(bool value);

  void setFixingMaxSpacing(bool value);

 private:
  bool isArgsChanged(const FlowContext& context) const;

  State current_state_{State::OPTIMIZATION};
  bool last_iteration_effective_{true};
  bool fixing_max_spacing_{false};
  FlexDR::SearchRepairArgs last_args_;

  static constexpr int STUBBORN_FLOW_VIOLATION_THRESHOLD = 100;
};

class FlexDRWorker;
class FlexDRWorkerRegionQuery
{
 public:
  FlexDRWorkerRegionQuery(FlexDRWorker* in);
  ~FlexDRWorkerRegionQuery();
  void add(drConnFig* connFig);
  void remove(drConnFig* connFig);
  void query(const odb::Rect& box,
             frLayerNum layerNum,
             std::vector<drConnFig*>& result) const;
  void query(const odb::Rect& box,
             frLayerNum layerNum,
             std::vector<rq_box_value_t<drConnFig*>>& result) const;
  void init();
  void cleanup();
  bool isEmpty() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

class FlexDRMinAreaVio
{
 public:
  // constructors
  FlexDRMinAreaVio() = default;
  FlexDRMinAreaVio(drNet* netIn,
                   FlexMazeIdx bpIn,
                   FlexMazeIdx epIn,
                   frCoord gapAreaIn)
      : net_(netIn), bp_(bpIn), ep_(epIn), gapArea_(gapAreaIn)
  {
  }
  // setters
  void setDRNet(drNet* netIn) { net_ = netIn; }
  void setPoints(FlexMazeIdx bpIn, FlexMazeIdx epIn)
  {
    bp_ = bpIn;
    ep_ = epIn;
  }
  void setGapArea(frCoord gapAreaIn) { gapArea_ = gapAreaIn; }

  // getters
  drNet* getNet() const { return net_; }
  void getPoints(FlexMazeIdx& bpIn, FlexMazeIdx& epIn) const
  {
    bpIn = bp_;
    epIn = ep_;
  }
  frCoord getGapArea() const { return gapArea_; }

 private:
  drNet* net_ = nullptr;
  FlexMazeIdx bp_, ep_;
  frCoord gapArea_ = 0;
};

class FlexGCWorker;
class FlexDRWorker
{
 public:
  // constructors
  FlexDRWorker(FlexDRViaData* via_data,
               frDesign* design,
               utl::Logger* logger,
               RouterConfiguration* router_cfg)
      : design_(design),
        logger_(logger),
        router_cfg_(router_cfg),
        via_data_(via_data),
        mazeEndIter_(1),
        ripupMode_(RipUpMode::ALL),
        workerDRCCost_(router_cfg->ROUTESHAPECOST),
        workerMarkerCost_(router_cfg->MARKERCOST),
        historyMarkers_(std::vector<std::set<FlexMazeIdx>>(3)),
        gridGraph_(design->getTech(), logger, this, router_cfg),
        rq_(this)
  {
  }
  // setters
  void setDebugSettings(frDebugSettings* settings)
  {
    debugSettings_ = settings;
  }
  void setRouteBox(const odb::Rect& boxIn) { routeBox_ = boxIn; }
  void setExtBox(const odb::Rect& boxIn) { extBox_ = boxIn; }
  void setDrcBox(const odb::Rect& boxIn) { drcBox_ = boxIn; }
  void setDRIter(int in) { drIter_ = in; }
  void setDRIter(
      int in,
      frOrderedIdMap<frNet*, std::set<std::pair<odb::Point, frLayerNum>>>& bp)
  {
    drIter_ = in;
    boundaryPin_ = std::move(bp);
  }
  bool isCongested() const { return isCongested_; }
  void setBoundaryPins(
      frOrderedIdMap<frNet*, std::set<std::pair<odb::Point, frLayerNum>>>& bp)
  {
    boundaryPin_ = std::move(bp);
  }
  void setMazeEndIter(int in) { mazeEndIter_ = in; }
  void setRipupMode(RipUpMode in) { ripupMode_ = in; }
  void setFollowGuide(bool in) { followGuide_ = in; }
  void setCost(frUInt4 drcCostIn,
               frUInt4 markerCostIn,
               frUInt4 workerFixedShapeCostIn,
               float workerMarkerDecayIn)
  {
    workerDRCCost_ = drcCostIn;
    workerMarkerCost_ = markerCostIn;
    workerFixedShapeCost_ = workerFixedShapeCostIn;
    workerMarkerDecay_ = workerMarkerDecayIn;
  }
  void setMarkerCost(frUInt4 markerCostIn) { workerMarkerCost_ = markerCostIn; }
  void setDrcCost(frUInt4 drcCostIn) { workerDRCCost_ = drcCostIn; }
  void setFixedShapeCost(frUInt4 fixedShapeCostIn)
  {
    workerFixedShapeCost_ = fixedShapeCostIn;
  }
  void setMarkerDecay(float markerDecayIn)
  {
    workerMarkerDecay_ = markerDecayIn;
  }
  void setMarkers(std::vector<frMarker>& in)
  {
    markers_.clear();
    for (auto& marker : in) {
      if (getDrcBox().intersects(marker.getBBox())) {
        markers_.push_back(marker);
      }
    }
  }
  void setMarkers(const std::vector<std::unique_ptr<frMarker>>& in)
  {
    markers_.clear();
    for (auto& uMarker : in) {
      auto& marker = *uMarker;
      if (getDrcBox().intersects(marker.getBBox())) {
        markers_.push_back(marker);
      }
    }
  }
  void setMarkers(std::vector<frMarker*>& in)
  {
    markers_.clear();
    for (auto& marker : in) {
      if (getDrcBox().intersects(marker->getBBox())) {
        markers_.push_back(*marker);
      }
    }
  }
  void setBestMarkers() { bestMarkers_ = markers_; }
  void clearMarkers() { markers_.clear(); }
  void setInitNumMarkers(int in) { initNumMarkers_ = in; }
  void setGCWorker(std::unique_ptr<FlexGCWorker> in)
  {
    gcWorker_ = std::move(in);
  }

  void setGraphics(AbstractDRGraphics* in)
  {
    graphics_ = in;
    gridGraph_.setGraphics(in);
  }
  void setViaData(FlexDRViaData* viaData) { via_data_ = viaData; }
  void setWorkerId(const int id) { worker_id_ = id; }
  // getters
  frTechObject* getTech() const { return design_->getTech(); }
  void getRouteBox(odb::Rect& boxIn) const { boxIn = routeBox_; }
  const odb::Rect& getRouteBox() const { return routeBox_; }
  odb::Rect& getRouteBox() { return routeBox_; }
  void getExtBox(odb::Rect& boxIn) const { boxIn = extBox_; }
  const odb::Rect& getExtBox() const { return extBox_; }
  odb::Rect& getExtBox() { return extBox_; }
  const odb::Rect& getDrcBox() const { return drcBox_; }
  odb::Rect& getDrcBox() { return drcBox_; }
  bool isInitDR() const { return (drIter_ == 0); }
  int getDRIter() const { return drIter_; }
  int getMazeEndIter() const { return mazeEndIter_; }
  bool isFollowGuide() const { return followGuide_; }
  RipUpMode getRipupMode() const { return ripupMode_; }
  const std::vector<std::unique_ptr<drNet>>& getNets() const { return nets_; }
  std::vector<std::unique_ptr<drNet>>& getNets() { return nets_; }
  const std::vector<drNet*>* getDRNets(frNet* net) const
  {
    if (net == nullptr || net->isSpecial()) {
      return nullptr;
    }
    auto it = owner2nets_.find(net);
    if (it != owner2nets_.end()) {
      return &(it->second);
    }
    return nullptr;
  }
  frDesign* getDesign() { return design_; }
  void setDesign(frDesign* design) { design_ = design; }
  const std::vector<frMarker>& getMarkers() const { return markers_; }
  std::vector<frMarker>& getMarkers() { return markers_; }
  const std::vector<frMarker>& getBestMarkers() const { return bestMarkers_; }
  std::vector<frMarker>& getBestMarkers() { return bestMarkers_; }
  const FlexDRWorkerRegionQuery& getWorkerRegionQuery() const { return rq_; }
  FlexDRWorkerRegionQuery& getWorkerRegionQuery() { return rq_; }
  int getInitNumMarkers() const { return initNumMarkers_; }
  int getNumMarkers() const { return markers_.size(); }
  int getBestNumMarkers() const { return bestMarkers_.size(); }
  FlexGCWorker* getGCWorker() { return gcWorker_.get(); }
  const FlexDRViaData* getViaData() const { return via_data_; }
  const FlexGridGraph& getGridGraph() const { return gridGraph_; }
  frUInt4 getWorkerMarkerCost() const { return workerMarkerCost_; }
  frUInt4 getWorkerDRCCost() const { return workerDRCCost_; }
  int getWorkerId() const { return worker_id_; }
  // others
  int main(frDesign* design);
  void distributedMain(frDesign* design);
  void writeUpdates(const std::string& file_name);
  void updateDesign(frDesign* design);
  std::string reloadedMain();
  bool end(frDesign* design);

  utl::Logger* getLogger() { return logger_; }
  void setLogger(utl::Logger* logger)
  {
    logger_ = logger;
    gridGraph_.setLogger(logger);
  }
  void setRouterCfg(RouterConfiguration* in) { router_cfg_ = in; }

  static std::unique_ptr<FlexDRWorker> load(const std::string& workerStr,
                                            FlexDRViaData* via_data,
                                            frDesign* design,
                                            utl::Logger* logger,
                                            RouterConfiguration* router_cfg);

  // distributed
  void setDistributed(dst::Distributed* dist,
                      const std::string& remote_ip,
                      uint16_t remote_port,
                      const std::string& dir)
  {
    dist_on_ = true;
    dist_ = dist;
    dist_ip_ = remote_ip;
    dist_port_ = remote_port;
    dist_dir_ = dir;
  }

  void setSharedVolume(const std::string& vol) { dist_dir_ = vol; }

  std::vector<Point3D> getSpecialAccessAPs() const { return specialAccessAPs_; }
  frCoord getHalfViaEncArea(frMIdx z, bool isLayer1, frNonDefaultRule* ndr);
  bool isSkipRouting() const { return skipRouting_; }

  enum ModCostType
  {
    subRouteShape,
    addRouteShape,
    subFixedShape,
    addFixedShape,
    resetFixedShape,
    setFixedShape,
    resetBlocked,
    setBlocked
  };

 private:
  struct RouteQueueEntry
  {
    frBlockObject* block;
    int numReroute;
    bool doRoute;
    frBlockObject* checkingObj;
    RouteQueueEntry(frBlockObject* block_in,
                    int num_reroute_in,
                    bool do_route_in,
                    frBlockObject* checking_obj_in)
        : block(block_in),
          numReroute(num_reroute_in),
          doRoute(do_route_in),
          checkingObj(checking_obj_in)
    {
    }
  };
  frDesign* design_{nullptr};
  utl::Logger* logger_{nullptr};
  RouterConfiguration* router_cfg_{nullptr};
  AbstractDRGraphics* graphics_{nullptr};  // owned by FlexDR
  frDebugSettings* debugSettings_{nullptr};
  FlexDRViaData* via_data_{nullptr};
  odb::Rect routeBox_;
  odb::Rect extBox_;
  odb::Rect drcBox_;
  int drIter_{0};
  int mazeEndIter_{0};
  bool followGuide_{false};
  bool needRecheck_{false};
  bool skipRouting_{false};
  RipUpMode ripupMode_{RipUpMode::DRC};
  // drNetOrderingEnum netOrderingMode;
  frUInt4 workerDRCCost_{0};
  frUInt4 workerMarkerCost_{0};
  frUInt4 workerFixedShapeCost_{0};
  float workerMarkerDecay_{0};
  // used in init route as gr boundary pin
  frOrderedIdMap<frNet*, std::set<std::pair<odb::Point, frLayerNum>>>
      boundaryPin_;
  int pinCnt_{0};
  int initNumMarkers_{0};
  std::map<FlexMazeIdx, drAccessPattern*> apSVia_;
  std::set<FlexMazeIdx> planarHistoryMarkers_;
  std::set<FlexMazeIdx> viaHistoryMarkers_;
  std::vector<std::set<FlexMazeIdx>> historyMarkers_;

  // local storage
  std::vector<std::unique_ptr<drNet>> nets_;
  frOrderedIdMap<frNet*, std::vector<drNet*>> owner2nets_;
  FlexGridGraph gridGraph_;
  std::vector<frMarker> markers_;
  std::vector<frMarker> bestMarkers_;
  FlexDRWorkerRegionQuery rq_;
  std::vector<frNonDefaultRule*> ndrs_;

  // persistent gc worker
  std::unique_ptr<FlexGCWorker> gcWorker_;

  // on-the-fly access points that require adding access edges in the grid graph
  std::vector<Point3D> specialAccessAPs_;

  // distributed
  dst::Distributed* dist_{nullptr};
  std::string dist_ip_;
  uint16_t dist_port_{0};
  std::string dist_dir_;
  bool dist_on_{false};
  bool isCongested_{false};
  bool save_updates_{false};
  int worker_id_{0};

  // hellpers
  bool isRoutePatchWire(const frPatchWire* pwire) const;
  bool isRouteVia(const frVia* via) const;
  // init
  void init(const frDesign* design);
  void initNets(const frDesign* design);
  void initRipUpNetsFromMarkers();
  void initNetObjs(
      const frDesign* design,
      frOrderedIdSet<frNet*>& nets,
      frOrderedIdMap<frNet*, std::vector<std::unique_ptr<drConnFig>>>&
          netRouteObjs,
      frOrderedIdMap<frNet*, std::vector<std::unique_ptr<drConnFig>>>&
          netExtObjs,
      frOrderedIdMap<frNet*, std::vector<frRect>>& netOrigGuides,
      frOrderedIdMap<frNet*, std::vector<frRect>>& netGuides);
  void initNetObjs_pathSeg(
      frPathSeg* pathSeg,
      frOrderedIdSet<frNet*>& nets,
      frOrderedIdMap<frNet*, std::vector<std::unique_ptr<drConnFig>>>&
          netRouteObjs,
      frOrderedIdMap<frNet*, std::vector<std::unique_ptr<drConnFig>>>&
          netExtObjs);
  void initNetObjs_via(
      const frVia* via,
      frOrderedIdSet<frNet*>& nets,
      frOrderedIdMap<frNet*, std::vector<std::unique_ptr<drConnFig>>>&
          netRouteObjs,
      frOrderedIdMap<frNet*, std::vector<std::unique_ptr<drConnFig>>>&
          netExtObjs);
  void initNetObjs_patchWire(
      frPatchWire* pwire,
      frOrderedIdSet<frNet*>& nets,
      frOrderedIdMap<frNet*, std::vector<std::unique_ptr<drConnFig>>>&
          netRouteObjs,
      frOrderedIdMap<frNet*, std::vector<std::unique_ptr<drConnFig>>>&
          netExtObjs);
  void initNets_segmentTerms(const odb::Point& bp,
                             frLayerNum lNum,
                             const frNet* net,
                             frBlockObjectSet& terms);
  void initNets_initDR(
      const frDesign* design,
      frOrderedIdSet<frNet*>& nets,
      frOrderedIdMap<frNet*, std::vector<std::unique_ptr<drConnFig>>>&
          netRouteObjs,
      frOrderedIdMap<frNet*, std::vector<std::unique_ptr<drConnFig>>>&
          netExtObjs,
      frOrderedIdMap<frNet*, std::vector<frRect>>& netOrigGuides,
      frOrderedIdMap<frNet*, std::vector<frRect>>& netGuides);
  int initNets_initDR_helper_getObjComponent(
      drConnFig* obj,
      const std::vector<std::vector<int>>& connectedComponents,
      const std::vector<frRect>& netGuides);
  void initNets_initDR_helper(
      frNet* net,
      std::vector<std::unique_ptr<drConnFig>>& netRouteObjs,
      std::vector<std::unique_ptr<drConnFig>>& netExtObjs,
      const std::vector<frBlockObject*>& netTerms,
      const std::vector<frRect>& netOrigGuides,
      const std::vector<frRect>& netGuides);

  void initNets_searchRepair(
      const frDesign* design,
      const frOrderedIdSet<frNet*>& nets,
      frOrderedIdMap<frNet*, std::vector<std::unique_ptr<drConnFig>>>&
          netRouteObjs,
      frOrderedIdMap<frNet*, std::vector<std::unique_ptr<drConnFig>>>&
          netExtObjs,
      frOrderedIdMap<frNet*, std::vector<frRect>>& netOrigGuides);
  void initNets_searchRepair_pin2epMap(
      const frDesign* design,
      const frNet* net,
      const std::vector<std::unique_ptr<drConnFig>>& netRouteObjs,
      frOrderedIdMap<frBlockObject*,
                     std::set<std::pair<odb::Point, frLayerNum>>>& pin2epMap);

  void initNets_searchRepair_pin2epMap_helper(
      const frDesign* design,
      const frNet* net,
      const odb::Point& bp,
      frLayerNum lNum,
      frOrderedIdMap<frBlockObject*,
                     std::set<std::pair<odb::Point, frLayerNum>>>& pin2epMap);
  void initNets_searchRepair_nodeMap(
      const std::vector<std::unique_ptr<drConnFig>>& netRouteObjs,
      std::vector<frBlockObject*>& netPins,
      const frOrderedIdMap<frBlockObject*,
                           std::set<std::pair<odb::Point, frLayerNum>>>&
          pin2epMap,
      std::map<std::pair<odb::Point, frLayerNum>, std::set<int>>& nodeMap);

  void initNets_searchRepair_nodeMap_routeObjEnd(
      const std::vector<std::unique_ptr<drConnFig>>& netRouteObjs,
      std::map<std::pair<odb::Point, frLayerNum>, std::set<int>>& nodeMap);
  void initNets_searchRepair_nodeMap_routeObjSplit(
      const std::vector<std::unique_ptr<drConnFig>>& netRouteObjs,
      std::map<std::pair<odb::Point, frLayerNum>, std::set<int>>& nodeMap);
  void initNets_searchRepair_nodeMap_routeObjSplit_helper(
      const odb::Point& crossPt,
      frCoord trackCoord,
      frCoord splitCoord,
      frLayerNum lNum,
      std::vector<
          std::map<frCoord, std::map<frCoord, std::pair<frCoord, int>>>>&
          mergeHelper,
      std::map<std::pair<odb::Point, frLayerNum>, std::set<int>>& nodeMap);
  void initNets_searchRepair_nodeMap_pin(
      const std::vector<std::unique_ptr<drConnFig>>& netRouteObjs,
      std::vector<frBlockObject*>& netPins,
      const frOrderedIdMap<frBlockObject*,
                           std::set<std::pair<odb::Point, frLayerNum>>>&
          pin2epMap,
      std::map<std::pair<odb::Point, frLayerNum>, std::set<int>>& nodeMap);
  void initNets_searchRepair_connComp(
      frNet* net,
      std::map<std::pair<odb::Point, frLayerNum>, std::set<int>>& nodeMap,
      std::vector<int>& compIdx);

  void initNet(const frDesign* design,
               frNet* net,
               std::vector<std::unique_ptr<drConnFig>>& routeObjs,
               std::vector<std::unique_ptr<drConnFig>>& extObjs,
               const std::vector<frRect>& origGuides,
               const std::vector<frBlockObject*>& terms,
               std::vector<std::pair<odb::Point, frLayerNum>> bounds = {});
  void initNet_term(const frDesign* design,
                    drNet* dNet,
                    const std::vector<frBlockObject*>& terms);
  template <typename T>
  void initNet_term_helper(const frDesign* design,
                           T* trueTerm,
                           frBlockObject* term,
                           frInst* inst,
                           drNet* dNet,
                           const std::string& name,
                           const odb::dbTransform& shiftXform);
  bool isRestrictedRouting(frLayerNum lNum);
  void initNet_addNet(std::unique_ptr<drNet> in);
  void getTrackLocs(bool isHorzTracks,
                    frLayerNum currLayerNum,
                    frCoord low,
                    frCoord high,
                    std::set<frCoord>& trackLocs);
  bool findAPTracks(frLayerNum startLayerNum,
                    frLayerNum endLayerNum,
                    const Rectangle& pinRect,
                    std::set<frCoord>& xLocs,
                    std::set<frCoord>& yLocs);
  void initNet_boundary(drNet* net,
                        const std::vector<std::unique_ptr<drConnFig>>& extObjs,
                        std::vector<std::pair<odb::Point, frLayerNum>> bounds);
  void initNets_numPinsIn();
  void initNets_boundaryArea();

  void initGridGraph(const frDesign* design);
  void initTrackCoords(frLayerCoordTrackPatternMap& xMap,
                       frLayerCoordTrackPatternMap& yMap);
  void initTrackCoords_route(drNet* net,
                             frLayerCoordTrackPatternMap& xMap,
                             frLayerCoordTrackPatternMap& yMap);
  void initTrackCoords_pin(drNet* net,
                           frLayerCoordTrackPatternMap& xMap,
                           frLayerCoordTrackPatternMap& yMap);
  frLayerNum initTrackCoords_getNonPref(frLayerNum lNum);
  void initMazeIdx();
  void initMazeIdx_connFig(drConnFig* connFig);
  void initMazeIdx_ap(drAccessPattern* ap);
  void initMazeCost(const frDesign* design);
  void initMazeCost_connFig();
  void initMazeCost_planarTerm(const frDesign* design);
  void initMazeCost_pin(drNet* net, bool isAddPathCost);
  void initMazeCost_fixedObj(const frDesign* design);
  void initMazeCost_terms(const std::set<frBlockObject*>& objs,
                          bool isAddPathCost,
                          bool isSkipVia = false);
  void modBlockedEdgesForMacroPin(frInstTerm* instTerm,
                                  const odb::dbTransform& xForm,
                                  bool isAddCost);
  void initMazeCost_ap();  // disable maze edge
  void initMazeCost_marker_route_queue(const frMarker& marker);
  void initMazeCost_marker_route_queue_addHistoryCost(const frMarker& marker);

  // void initMazeCost_via();
  void initMazeCost_via_helper(drNet* net, bool isAddPathCost);
  void initMazeCost_minCut_helper(drNet* net, bool isAddPathCost);
  void initMazeCost_guide_helper(drNet* net, bool isAdd);
  void initMazeCost_ap_helper(drNet* net, bool isAddPathCost);
  void initMazeCost_ap_planarGrid_helper(const FlexMazeIdx& mi,
                                         const frDirEnum& dir,
                                         frCoord bloatLen,
                                         bool isAddPathCost);
  void initMazeCost_boundary_helper(drNet* net, bool isAddPathCost);

  // DRC
  void initMarkers(const frDesign* design);

  // route_queue
  void route_queue();
  void route_queue_main(std::queue<RouteQueueEntry>& rerouteQueue);
  void addMinAreaPatches_poly(gcNet* drcNet, drNet* net);
  void cleanUnneededPatches_poly(gcNet* drcNet, drNet* net);
  void modEolCosts_poly(gcNet* net, ModCostType modType);
  void modEolCosts_poly(gcPin* shape, frLayer* layer, ModCostType modType);
  void modEolCost(frCoord low,
                  frCoord high,
                  frCoord line,
                  bool isVertical,
                  bool innerDirIsIncreasing,
                  frLayer* layer,
                  ModCostType modType);
  void route_queue_resetRipup();
  void route_queue_markerCostDecay();
  void route_queue_addMarkerCost(
      const std::vector<std::unique_ptr<frMarker>>& markers);
  void route_queue_addMarkerCost();
  void route_queue_init_queue(std::queue<RouteQueueEntry>& rerouteQueue);
  void route_queue_update_from_marker(
      frMarker* marker,
      std::set<frBlockObject*>& uniqueVictims,
      std::set<frBlockObject*>& uniqueAggressors,
      std::vector<RouteQueueEntry>& checks,
      std::vector<RouteQueueEntry>& routes,
      frBlockObject* checkingObj);
  void getRipUpNetsFromMarker(frMarker* marker,
                              std::set<drNet*>& nets,
                              frCoord bloatDist = 0);
  void route_queue_update_queue(const std::vector<RouteQueueEntry>& checks,
                                const std::vector<RouteQueueEntry>& routes,
                                std::queue<RouteQueueEntry>& rerouteQueue);
  void route_queue_update_queue(
      const std::vector<std::unique_ptr<frMarker>>& markers,
      std::queue<RouteQueueEntry>& rerouteQueue,
      frBlockObject* checkingObj = nullptr);
  bool canRipup(drNet* n);
  // route
  void addPathCost(drConnFig* connFig,
                   bool modEol = false,
                   bool modCutSpc = false);
  void subPathCost(drConnFig* connFig,
                   bool modEol = false,
                   bool modCutSpc = false);
  void modPathCost(drConnFig* connFig,
                   ModCostType type,
                   bool modEol = false,
                   bool modCutSpc = false);
  // minSpc
  void modMinSpacingCostPlanar(const odb::Rect& box,
                               frMIdx z,
                               ModCostType type,
                               bool isBlockage = false,
                               frNonDefaultRule* ndr = nullptr,
                               bool isMacroPin = false,
                               bool resetHorz = true,
                               bool resetVert = true);
  void modMinSpacingCostPlanarHelper(const odb::Rect& box,
                                     frMIdx z,
                                     ModCostType type,
                                     frCoord width,
                                     frCoord minSpacing,
                                     bool isBlockage,
                                     bool isMacroPin,
                                     bool resetHorz,
                                     bool resetVert,
                                     bool ndr);
  void modCornerToCornerSpacing(const odb::Rect& box,
                                frMIdx z,
                                ModCostType type);
  void modMinSpacingCostVia(const odb::Rect& box,
                            frMIdx z,
                            ModCostType type,
                            bool isUpperVia,
                            bool isCurrPs,
                            bool isBlockage = false,
                            frNonDefaultRule* ndr = nullptr);
  void modMinSpacingCostViaHelper(const odb::Rect& box,
                                  frMIdx z,
                                  ModCostType type,
                                  frCoord width,
                                  frCoord minSpacing,
                                  const frViaDef* viaDef,
                                  drEolSpacingConstraint drCon,
                                  bool isUpperVia,
                                  bool isCurrPs,
                                  bool isBlockage,
                                  bool ndr);

  void modCornerToCornerSpacing_helper(const odb::Rect& box,
                                       frMIdx z,
                                       ModCostType type);

  void modMinSpacingCostVia_eol(const odb::Rect& box,
                                const odb::Rect& tmpBx,
                                ModCostType type,
                                const drEolSpacingConstraint& drCon,
                                frMIdx idx,
                                bool ndr = false);
  void modMinSpacingCostVia_eol_helper(const odb::Rect& box,
                                       const odb::Rect& testBox,
                                       ModCostType type,
                                       frMIdx idx,
                                       bool ndr = false);
  // eolSpc
  void modEolSpacingCost_helper(const odb::Rect& testbox,
                                frMIdx z,
                                ModCostType type,
                                int eolType,
                                bool resetHorz = true,
                                bool resetVert = true);
  void modEolSpacingRulesCost(const odb::Rect& box,
                              frMIdx z,
                              ModCostType type,
                              bool isSkipVia = false,
                              frNonDefaultRule* ndr = nullptr,
                              bool resetHorz = true,
                              bool resetVert = true);
  // cutSpc
  void modCutSpacingCost(const odb::Rect& box,
                         frMIdx z,
                         ModCostType type,
                         bool isBlockage = false,
                         int avoidI = -1,
                         int avoidJ = -1);
  void modInterLayerCutSpacingCost(const odb::Rect& box,
                                   frMIdx z,
                                   ModCostType type,
                                   bool isUpperVia,
                                   bool isBlockage = false);
  // adjCut
  void modAdjCutSpacingCost_fixedObj(const frDesign* design,
                                     const odb::Rect& box,
                                     frVia* origVia);
  void modMinimumcutCostVia(const odb::Rect& box,
                            frMIdx z,
                            ModCostType type,
                            bool isUpperVia);
  void modViaForbiddenThrough(const FlexMazeIdx& bi,
                              const FlexMazeIdx& ei,
                              ModCostType type);
  void modBlockedPlanar(const odb::Rect& box, frMIdx z, bool setBlock);
  void modBlockedVia(const odb::Rect& box, frMIdx z, bool setBlock);

  bool mazeIterInit_sortRerouteNets(int mazeIter,
                                    std::vector<drNet*>& rerouteNets);

  bool mazeIterInit_sortRerouteQueue(int mazeIter,
                                     std::vector<RouteQueueEntry>& rerouteNets);

  void mazeNetInit(drNet* net);
  void mazeNetEnd(drNet* net);
  bool routeNet(drNet* net, std::vector<FlexMazeIdx>& paths);
  void routeNet_prep(
      drNet* net,
      frOrderedIdSet<drPin*>& unConnPins,
      std::map<FlexMazeIdx, frOrderedIdSet<drPin*>>& mazeIdx2unConnPins,
      std::set<FlexMazeIdx>& apMazeIdx,
      std::set<FlexMazeIdx>& realPinAPMazeIdx,
      std::map<FlexMazeIdx, frBox3D*>& mazeIdx2TaperBox,
      std::list<std::pair<drPin*, frBox3D>>& pinTaperBoxes);
  void routeNet_prepAreaMap(drNet* net,
                            std::map<FlexMazeIdx, frCoord>& areaMap);
  void routeNet_setSrc(
      frOrderedIdSet<drPin*>& unConnPins,
      std::map<FlexMazeIdx, frOrderedIdSet<drPin*>>& mazeIdx2unConnPins,
      std::vector<FlexMazeIdx>& connComps,
      FlexMazeIdx& ccMazeIdx1,
      FlexMazeIdx& ccMazeIdx2,
      odb::Point& centerPt);
  void mazePinInit();
  drPin* routeNet_getNextDst(
      FlexMazeIdx& ccMazeIdx1,
      FlexMazeIdx& ccMazeIdx2,
      std::map<FlexMazeIdx, frOrderedIdSet<drPin*>>& mazeIdx2unConnPins,
      std::list<std::pair<drPin*, frBox3D>>& pinTaperBoxes);
  void routeNet_postAstarUpdate(
      std::vector<FlexMazeIdx>& path,
      std::vector<FlexMazeIdx>& connComps,
      frOrderedIdSet<drPin*>& unConnPins,
      std::map<FlexMazeIdx, frOrderedIdSet<drPin*>>& mazeIdx2unConnPins,
      bool isFirstConn);
  void routeNet_postAstarWritePath(
      drNet* net,
      std::vector<FlexMazeIdx>& points,
      const std::set<FlexMazeIdx>& realPinApMazeIdx,
      std::map<FlexMazeIdx, frBox3D*>& mazeIdx2Taperbox,
      const std::set<FlexMazeIdx>& apMazeIdx);
  bool addApPathSegs(const FlexMazeIdx& apIdx, drNet* net);
  /**
   * Updates external figures to connect to access-point if needed.
   *
   * While routing, there could be a case where we are routing a boundary pin to
   * an access point at the same location of the boundary pin. In this case, the
   * path would consist only of one point. The router may fail to addApPathSegs
   * if planar access is not allowed. In that case, we should update the
   * external object connected to the boundary pin to connect to the
   * access-point directly. For each net we keep a list of such updates under
   * drNet::ext_figs_updates_. This function modifies this list by going through
   * all external objects of the net and updating the one that begins or ends at
   * the current access-point/boundary-pin
   * @param net The current net being routed
   * @param ap_idx The graph idx of the access-point which is the same as the
   * boundary pin idx. This is the one point that constructs the current path.
   */
  void addApExtFigUpdate(drNet* net, const FlexMazeIdx& ap_idx) const;
  void setNDRStyle(drNet* net,
                   frSegStyle& currStyle,
                   frMIdx startX,
                   frMIdx endX,
                   frMIdx startY,
                   frMIdx endY,
                   frMIdx z,
                   FlexMazeIdx* prev,
                   FlexMazeIdx* next);
  void editStyleExt(frSegStyle& currStyle,
                    frMIdx startX,
                    frMIdx endX,
                    frMIdx z,
                    FlexMazeIdx* prev,
                    FlexMazeIdx* next);
  bool isInsideTaperBox(frMIdx x,
                        frMIdx y,
                        frMIdx startZ,
                        frMIdx endZ,
                        std::map<FlexMazeIdx, frBox3D*>& mazeIdx2TaperBox);
  bool splitPathSeg(frMIdx& midX,
                    frMIdx& midY,
                    bool& taperFirstPiece,
                    frMIdx startX,
                    frMIdx startY,
                    frMIdx endX,
                    frMIdx endY,
                    frMIdx z,
                    frBox3D* srcBox,
                    frBox3D* dstBox,
                    drNet* net);
  void processPathSeg(frMIdx startX,
                      frMIdx startY,
                      frMIdx endX,
                      frMIdx endY,
                      frMIdx z,
                      const std::set<FlexMazeIdx>& realApMazeIdx,
                      drNet* net,
                      bool vertical,
                      bool taper,
                      int i,
                      std::vector<FlexMazeIdx>& points,
                      const std::set<FlexMazeIdx>& apMazeIdx);
  bool isInWorkerBorder(frCoord x, frCoord y) const;
  void checkPathSegStyle(drPathSeg* ps,
                         bool isBegin,
                         frSegStyle& style,
                         const std::set<FlexMazeIdx>& apMazeIdx,
                         const FlexMazeIdx& idx);
  void checkViaConnectivityToAP(drVia* via,
                                bool isBottom,
                                frNet* net,
                                const std::set<FlexMazeIdx>& apMazeIdx,
                                const FlexMazeIdx& idx);
  bool hasAccessPoint(const odb::Point& pt, frLayerNum lNum, frNet* net);
  void routeNet_postAstarPatchMinAreaVio(
      drNet* net,
      const std::vector<FlexMazeIdx>& path,
      const std::map<FlexMazeIdx, frCoord>& areaMap);
  void routeNet_postAstarPatchMinAreaVio_helper(
      drNet* net,
      drt::frLayer* curr_layer,
      frArea reqArea,
      frArea currArea,
      frCoord startViaHalfEncArea,
      frCoord endViaHalfEncArea,
      std::vector<FlexMazeIdx>& points,
      int point_idx,
      int prev_point_idx);
  void routeNet_postAstarAddPatchMetal(drNet* net,
                                       const FlexMazeIdx& bpIdx,
                                       const FlexMazeIdx& epIdx,
                                       frCoord gapArea,
                                       frCoord patchWidth,
                                       bool bpPatchLeft = true,
                                       bool epPatchLeft = false);
  int routeNet_postAstarAddPathMetal_isClean(const FlexMazeIdx& bpIdx,
                                             bool isPatchHorz,
                                             bool isPatchLeft,
                                             frCoord patchLength);
  void routeNet_postAstarAddPatchMetal_addPWire(drNet* net,
                                                const FlexMazeIdx& bpIdx,
                                                bool isPatchHorz,
                                                bool isPatchLeft,
                                                frCoord patchLength,
                                                frCoord patchWidth);
  void routeNet_postRouteAddPathCost(drNet* net);
  void routeNet_AddCutSpcCost(std::vector<FlexMazeIdx>& path);
  void routeNet_postRouteAddPatchMetalCost(drNet* net);

  // end
  void cleanup();
  void identifyCongestionLevel();
  void endGetModNets(frOrderedIdSet<frNet*>& modNets);
  void endRemoveNets(
      frDesign* design,
      frOrderedIdSet<frNet*>& modNets,
      frOrderedIdMap<frNet*, std::set<std::pair<odb::Point, frLayerNum>>>&
          boundPts);
  void endRemoveNets_pathSeg(
      frDesign* design,
      frPathSeg* pathSeg,
      std::set<std::pair<odb::Point, frLayerNum>>& boundPts);
  void endRemoveNets_via(frDesign* design, frVia* via);
  void endRemoveNets_patchWire(frDesign* design, frPatchWire* pwire);
  void endAddNets(
      frDesign* design,
      frOrderedIdMap<frNet*, std::set<std::pair<odb::Point, frLayerNum>>>&
          boundPts);
  void endAddNets_pathSeg(frDesign* design, drPathSeg* pathSeg);
  void endAddNets_via(frDesign* design, drVia* via);
  void endAddNets_patchWire(frDesign* design, drPatchWire* pwire);
  void endAddNets_merge(frDesign* design,
                        frNet* net,
                        std::set<std::pair<odb::Point, frLayerNum>>& boundPts);
  /**
   * Commits updates made by FlexDRWorker::addApExtFigUpdate to the design.
   *
   * This function goes through the ext_figs_updates_ of each net and applies
   * the required updates to the external objects.
   *
   * @param net The currently being modified drNet
   */
  void endAddNets_updateExtFigs(drNet* net);
  /**
   * Applies update to external pathsegs.
   *
   * This is a helper function for endAddNets_updateExtFigs that is responsible
   * for handling a pathseg update.
   *
   * @param update_pt The boundary point that should touch the external
   * path_seg.
   * @param path_seg The external pathseg being updated.
   * @returns True if the updates apply to the passed pathseg and False
   * otherwise.
   */
  bool endAddNets_updateExtFigs_pathSeg(drNet* net,
                                        const Point3D& update_pt,
                                        frPathSeg* path_seg);
  /**
   * Applies update to external via.
   *
   * This is a helper function for endAddNets_updateExtFigs that is responsible
   * for handling a via update.
   *
   * @param update_pt The boundary point that should touch the external
   * path_seg.
   * @param via The external via being updated.
   * @returns True if the updates apply to the passed via and False otherwise.
   */
  bool endAddNets_updateExtFigs_via(drNet* net,
                                    const Point3D& update_pt,
                                    frVia* via);
  void endRemoveMarkers(frDesign* design);
  void endAddMarkers(frDesign* design);

  // helper functions
  frCoord snapCoordToManufacturingGrid(frCoord coord, int lowerLeftCoord);
  void writeGCPatchesToDRWorker(drNet* target_net = nullptr,
                                const std::vector<FlexMazeIdx>& valid_indices
                                = {});

  template <class Archive>
  void serialize(Archive& ar, unsigned int version);
  friend class boost::serialization::access;
};
}  // namespace drt
