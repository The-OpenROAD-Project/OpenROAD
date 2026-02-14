// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "boost/icl/interval_map.hpp"
#include "boost/icl/interval_set.hpp"
#include "db/grObj/grFig.h"
#include "db/grObj/grNet.h"
#include "db/grObj/grShape.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frNode.h"
#include "db/tech/frTechObject.h"
#include "dr/FlexMazeTypes.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "frRTree.h"
#include "frRegionQuery.h"
#include "global.h"
#include "gr/FlexGRCMap.h"
#include "gr/FlexGRGridGraph.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace stt {
class SteinerTreeBuilder;
}
namespace drt {

class FlexGR
{
 public:
  // constructors
  FlexGR(frDesign* designIn,
         utl::Logger* logger,
         stt::SteinerTreeBuilder* stt_builder,
         RouterConfiguration* router_cfg)
      : design_(designIn),
        logger_(logger),
        stt_builder_(stt_builder),
        router_cfg_(router_cfg)
  {
  }

  // getters
  frTechObject* getTech() const { return design_->getTech(); }
  frDesign* getDesign() const { return design_; }
  frRegionQuery* getRegionQuery() const { return design_->getRegionQuery(); }
  FlexGRCMap* getCMap(bool is2DCMap) const
  {
    if (is2DCMap) {
      return cmap2D_.get();
    }
    return cmap_.get();
  }

  // others
  void main(odb::dbDatabase* db = nullptr);

 private:
  odb::dbDatabase* db_{nullptr};
  frDesign* design_;
  std::unique_ptr<FlexGRCMap> cmap_;
  std::unique_ptr<FlexGRCMap> cmap2D_;
  utl::Logger* logger_;
  stt::SteinerTreeBuilder* stt_builder_;
  RouterConfiguration* router_cfg_;
  frOrderedIdMap<frNet*, std::map<std::pair<int, int>, std::vector<frNode*>>>
      net2GCellIdx2Nodes_;
  frOrderedIdMap<frNet*, std::vector<frNode*>> net2GCellNodes_;
  frOrderedIdMap<frNet*, std::vector<frNode*>> net2SteinerNodes_;
  frOrderedIdMap<frNet*, frOrderedIdMap<frNode*, std::vector<frNode*>>>
      net2GCellNode2RPinNodes_;
  std::vector<frCoord> trackPitches_;
  std::vector<frCoord> line2ViaPitches_;
  std::vector<frCoord> layerPitches_;
  std::vector<frCoord> zHeights_;

  // others
  void init();
  void initGCell();
  void initCMap();
  void initLayerPitch();

  void ra();

  void searchRepairMacro(int iter,
                         int size,
                         int mazeEndIter,
                         unsigned workerCongCost,
                         unsigned workerHistCost,
                         double congThresh,
                         bool is2DRouting,
                         RipUpMode mode);
  void searchRepair(int iter,
                    int size,
                    int offset,
                    int mazeEndIter,
                    unsigned workerCongCost,
                    unsigned workerHistCost,
                    double congThresh,
                    bool is2DRouting,
                    RipUpMode mode,
                    bool TEST);

  void end();

  void setCMap(std::unique_ptr<FlexGRCMap>& in) { cmap_ = std::move(in); }
  void setCMap2D(std::unique_ptr<FlexGRCMap>& in) { cmap2D_ = std::move(in); }

  // initGR
  void initGR();
  void initGR_genTopology();
  void initGR_genTopology_net(frNet* net);
  void initGR_updateCongestion();
  void initGR_updateCongestion_net(frNet* net);
  void initGR_updateCongestion2D_net(frNet* net);
  void initGR_patternRoute();
  void initGR_patternRoute_init(
      std::vector<std::pair<std::pair<frNode*, frNode*>, int>>& patternRoutes);
  void initGR_patternRoute_route(
      std::vector<std::pair<std::pair<frNode*, frNode*>, int>>& patternRoutes);
  bool initGR_patternRoute_route_iter(
      int iter,
      std::vector<std::pair<std::pair<frNode*, frNode*>, int>>& patternRoutes,
      int mode);
  void initGR_initObj();
  void initGR_initObj_net(frNet* net);

  // pattern route
  void patternRoute_LShape(frNode* child, frNode* parent);

  // layer assignment
  void layerAssign();
  void layerAssign_net(frNet* net);
  void layerAssign_node_compute(
      frNode* currNode,
      frNet* net,
      std::vector<std::vector<unsigned>>& bestLayerCosts,
      std::vector<std::vector<unsigned>>& bestLayerCombs);
  void layerAssign_node_commit(
      frNode* currNode,
      frNet* net,
      frLayerNum layerNum,
      std::vector<std::vector<unsigned>>& bestLayerCombs);

  // cost
  double getCongCost(unsigned supply, unsigned demand);

  // others
  void ripupRoute(frNode* child, frNode* parent);
  bool hasOverflow2D(frNode* child, frNode* parent);
  void reportCong2DGolden(FlexGRCMap* baseCMap2D);
  void reportCong2D(FlexGRCMap* cmap2D);
  void reportCong2D();
  void reportCong3DGolden(FlexGRCMap* baseCMap);
  void reportCong3D(FlexGRCMap* cmap);
  void reportCong3D();
  void updateDbCongestion(odb::dbDatabase* db, FlexGRCMap* cmap);

  // temp
  void initGR_patternRoute_layerAssignment();
  void initGR_patternRoute_layerAssignment_net(frNet* net);

  // search and repair

  // topology
  void genMSTTopology(std::vector<frNode*>& nodes);
  void genMSTTopology_PD(std::vector<frNode*>& nodes, double alpha = 0.3);
  int genMSTTopology_PD_minIdx(const std::vector<int>& keys,
                               const std::vector<bool>& isVisited);

  void genSTTopology_FLUTE(std::vector<frNode*>& nodes,
                           std::vector<frNode*>& steinerNodes);
  void genSTTopology_HVW(std::vector<frNode*>& nodes,
                         std::vector<frNode*>& steinerNodes);
  void genSTTopology_HVW_compute(frNode* currNode,
                                 std::vector<frNode*>& nodes,
                                 std::vector<unsigned>& overlapL,
                                 std::vector<unsigned>& overlapU,
                                 std::vector<unsigned>& bestCombL,
                                 std::vector<unsigned>& bestCombU);
  unsigned genSTTopology_HVW_levelOvlp(frNode* currNode,
                                       bool isCurrU,
                                       unsigned comb);
  void genSTTopology_HVW_levelOvlp_helper(frNode* parent,
                                          frNode* child,
                                          bool isCurrU,
                                          std::pair<frCoord, frCoord>& horzIntv,
                                          std::pair<frCoord, frCoord>& vertIntv,
                                          odb::Point& turnLoc);
  void genSTTopology_HVW_commit(frNode* currNode,
                                bool isCurrU,
                                std::vector<frNode*>& nodes,
                                std::vector<unsigned>& bestCombL,
                                std::vector<unsigned>& bestCombU,
                                std::vector<bool>& isU);

  void genSTTopology_build_tree(std::vector<frNode*>& pinNodes,
                                std::vector<bool>& isU,
                                std::vector<frNode*>& steinerNodes);
  void genSTTopology_build_tree_mergeSeg(
      std::vector<frNode*>& pinNodes,
      std::vector<bool>& isU,
      std::map<frCoord, boost::icl::interval_set<frCoord>>& horzIntvs,
      std::map<frCoord, boost::icl::interval_set<frCoord>>& vertIntvs);
  void genSTTopology_build_tree_splitSeg(
      std::vector<frNode*>& pinNodes,
      std::map<odb::Point, frNode*>& pinGCell2Nodes,
      std::map<frCoord, boost::icl::interval_set<frCoord>>& horzIntvs,
      std::map<frCoord, boost::icl::interval_set<frCoord>>& vertIntvs,
      std::map<odb::Point, frNode*>& steinerGCell2Nodes,
      std::vector<frNode*>& steinerNodes);
  // utility
  void writeToGuide();
  void updateDb();
  void getBatchInfo(int& batchStepX, int& batchStepY);
};

class FlexGRWorker;
class FlexGRWorkerRegionQuery
{
 public:
  FlexGRWorkerRegionQuery(FlexGRWorker* in) : grWorker_(in) {}
  frDesign* getDesign() const;
  FlexGRWorker* getGRWorker() const { return grWorker_; }
  void add(grConnFig* connFig);
  void add(grConnFig* connFig,
           std::vector<std::vector<rq_box_value_t<grConnFig*>>>& allShapes);
  void remove(grConnFig* connFig);
  void query(const odb::Rect& box,
             frLayerNum layerNum,
             std::vector<grConnFig*>& result) const;
  void query(const odb::Rect& box,
             frLayerNum layerNum,
             std::vector<rq_box_value_t<grConnFig*>>& result) const;
  void init(bool includeExt = false);
  void cleanup()
  {
    shapes_.clear();
    shapes_.shrink_to_fit();
  }
  void report()
  {
    for (int i = 0; i < (int) shapes_.size(); i++) {
      std::cout << " layerIdx = " << i << ", numObj = " << shapes_[i].size()
                << "\n";
    }
  }

 private:
  FlexGRWorker* grWorker_;
  // only for routeConnFigs in gr worker
  std::vector<RTree<grConnFig*>> shapes_;
};

class FlexGRWorker
{
 public:
  // constructors
  FlexGRWorker(FlexGR* grIn, RouterConfiguration* router_cfg)
      : design_(grIn->getDesign()),
        gr_(grIn),
        gridGraph_(grIn->getDesign(), this, router_cfg),
        rq_(this)
  {
  }
  // setters
  void setRouteGCellIdxLL(const odb::Point& in) { routeGCellIdxLL_ = in; }
  void setRouteGCellIdxUR(const odb::Point& in) { routeGCellIdxUR_ = in; }
  void setExtBox(const odb::Rect& in) { extBox_ = in; }
  void setRouteBox(const odb::Rect& in) { routeBox_ = in; }
  void setGRIter(int in) { grIter_ = in; }
  void setMazeEndIter(int in) { mazeEndIter_ = in; }
  void setCongCost(int in) { workerCongCost_ = in; }
  void setHistCost(int in) { workerHistCost_ = in; }
  void setCongThresh(double in) { congThresh_ = in; }
  void set2D(bool in) { is2DRouting_ = in; }
  void setRipupMode(RipUpMode in) { ripupMode_ = in; }

  // getters
  frTechObject* getTech() const { return design_->getTech(); }
  frDesign* getDesign() const { return design_; }
  FlexGR* getGR() const { return gr_; }
  const odb::Point& getRouteGCellIdxLL() const { return routeGCellIdxLL_; }
  odb::Point& getRouteGCellIdxLL() { return routeGCellIdxLL_; }
  const odb::Point& getRouteGCellIdxUR() const { return routeGCellIdxUR_; }
  odb::Point& getRouteGCellIdxUR() { return routeGCellIdxUR_; }
  void getExtBox(odb::Rect& in) const { in = extBox_; }
  const odb::Rect& getExtBox() const { return extBox_; }
  odb::Rect& getExtBox() { return extBox_; }
  void getRouteBox(odb::Rect& in) const { in = routeBox_; }
  const odb::Rect& getRouteBox() const { return routeBox_; }
  odb::Rect& getRouteBox() { return routeBox_; }
  int getGRIter() const { return grIter_; }
  int getMazeEndIter() const { return mazeEndIter_; }
  double getCongThresh() const { return congThresh_; }
  bool is2D() const { return is2DRouting_; }
  frRegionQuery* getRegionQuery() const { return design_->getRegionQuery(); }
  FlexGRCMap* getCMap() const { return gr_->getCMap(is2DRouting_); }
  const std::vector<std::unique_ptr<grNet>>& getNets() const { return nets_; }
  std::vector<std::unique_ptr<grNet>>& getNets() { return nets_; }
  const std::vector<grNet*>* getGRNets(frNet* net) const
  {
    auto it = owner2nets_.find(net);
    if (it != owner2nets_.end()) {
      return &(it->second);
    }
    return nullptr;
  }
  const FlexGRWorkerRegionQuery& getWorkerRegionQuery() const { return rq_; }
  FlexGRWorkerRegionQuery& getWorkerRegionQuery() { return rq_; }

  // others
  void initBoundary();
  void main_mt();
  void end();
  void cleanup();

 private:
  frDesign* design_{nullptr};
  FlexGR* gr_{nullptr};
  odb::Point routeGCellIdxLL_;
  odb::Point routeGCellIdxUR_;
  odb::Rect extBox_;
  odb::Rect routeBox_;
  int grIter_{0};
  int mazeEndIter_{1};
  int workerCongCost_{0};
  int workerHistCost_{0};
  double congThresh_{1.0};
  bool is2DRouting_{false};
  RipUpMode ripupMode_{RipUpMode::DRC};

  // local storage
  std::vector<std::unique_ptr<grNet>> nets_;
  frOrderedIdMap<frNet*, std::vector<grNet*>> owner2nets_;

  FlexGRGridGraph gridGraph_;
  FlexGRWorkerRegionQuery rq_;

  // initBoundary
  void initBoundary_splitPathSeg(grPathSeg* pathSeg);
  void initBoundary_splitPathSeg_getBreakPts(const odb::Point& bp,
                                             const odb::Point& ep,
                                             odb::Point& breakPt1,
                                             odb::Point& breakPt2);
  frNode* initBoundary_splitPathSeg_split(frNode* child,
                                          frNode* parent,
                                          const odb::Point& breakPt);

  // init
  void init();
  void initNets();
  void initNets_roots(frOrderedIdSet<frNet*>& nets,
                      frOrderedIdMap<frNet*, std::vector<frNode*>>& netRoots);
  void initNetObjs_roots_pathSeg(
      grPathSeg* pathSeg,
      frOrderedIdSet<frNet*>& nets,
      frOrderedIdMap<frNet*, std::vector<frNode*>>& netRoots);
  void initNetObjs_roots_via(
      grVia* via,
      frOrderedIdSet<frNet*>& nets,
      frOrderedIdMap<frNet*, std::vector<frNode*>>& netRoots);
  void initNets_searchRepair(
      frOrderedIdSet<frNet*>& nets,
      frOrderedIdMap<frNet*, std::vector<frNode*>>& netRoots);
  void initNet(frNet* net, const std::vector<frNode*>& netRoots);
  void initNet_initNodes(grNet* net, frNode* fRoot);
  void initNet_initRoot(grNet* net);
  void initNet_updateCMap(grNet* net, bool isAdd);
  void initNet_initPinGCellNodes(grNet* net);
  void initNet_initObjs(grNet* net);
  void initNet_addNet(std::unique_ptr<grNet>& in);
  void initNets_regionQuery();
  void initGridGraph();
  void initGridGraph_fromCMap();

  // debug
  void initNets_printNets();
  void initNets_printNet(grNet* net);
  void initNets_printFNets(
      frOrderedIdMap<frNet*, std::vector<frNode*>>& netRoots);
  void initNets_printFNet(frNode* root);

  // route
  void route();
  void route_addHistCost();
  void route_mazeIterInit();
  void route_getRerouteNets(std::vector<grNet*>& rerouteNets);
  void mazeNetInit(grNet* net);
  bool mazeNetHasCong(grNet* net);
  void mazeNetInit_addHistCost(grNet* net);
  void mazeNetInit_decayHistCost(grNet* net);
  void mazeNetInit_removeNetObjs(grNet* net);
  void modCong_pathSeg(grPathSeg* pathSeg, bool isAdd);
  void mazeNetInit_removeNetNodes(grNet* net);
  bool routeNet(grNet* net);
  void routeNet_prep(grNet* net,
                     frOrderedIdSet<grNode*>& unConnPinGCellNodes,
                     std::map<FlexMazeIdx, grNode*>& mazeIdx2unConnPinGCellNode,
                     std::map<FlexMazeIdx, grNode*>& mazeIdx2endPointNode);
  void routeNet_setSrc(
      grNet* net,
      frOrderedIdSet<grNode*>& unConnPinGCellNodes,
      std::map<FlexMazeIdx, grNode*>& mazeIdx2unConnPinGCellNode,
      std::vector<FlexMazeIdx>& connComps,
      FlexMazeIdx& ccMazeIdx1,
      FlexMazeIdx& ccMazeIdx2,
      odb::Point& centerPt);
  grNode* routeNet_getNextDst(
      FlexMazeIdx& ccMazeIdx1,
      FlexMazeIdx& ccMazeIdx2,
      std::map<FlexMazeIdx, grNode*>& mazeIdx2unConnPinGCellNode);
  grNode* routeNet_postAstarUpdate(
      std::vector<FlexMazeIdx>& path,
      std::vector<FlexMazeIdx>& connComps,
      frOrderedIdSet<grNode*>& unConnPinGCellNodes,
      std::map<FlexMazeIdx, grNode*>& mazeIdx2unConnPinGCellNode);
  void routeNet_postAstarWritePath(
      grNet* net,
      std::vector<FlexMazeIdx>& points,
      grNode* leaf,
      std::map<FlexMazeIdx, grNode*>& mazeIdx2endPointNode);
  grNode* routeNet_postAstarWritePath_splitPathSeg(grNode* child,
                                                   grNode* parent,
                                                   const odb::Point& breakPt);
  void routeNet_postRouteAddCong(grNet* net);
  void route_decayHistCost();

  // end
  void endGetModNets(frOrderedIdSet<frNet*>& modNets);
  void endRemoveNets(const frOrderedIdSet<frNet*>& modNets);
  void endRemoveNets_objs(const frOrderedIdSet<frNet*>& modNets);
  void endRemoveNets_pathSeg(grPathSeg* pathSeg);
  void endRemoveNets_via(grVia* via);
  void endRemoveNets_nodes(const frOrderedIdSet<frNet*>& modNets);
  void endRemoveNets_nodes_net(grNet* net, frNet* fnet);
  void endRemoveNets_node(frNode* node);
  void endAddNets(frOrderedIdSet<frNet*>& modNets);
  void endAddNets_stitchRouteBound(grNet* net);
  void endAddNets_stitchRouteBound_node(grNode* node);
  void endAddNets_addNet(grNet* net, frNet* fnet);
  void endStitchBoundary();
  void endStitchBoundary_net(grNet* net);
  void endWriteBackCMap();

  // other
  odb::Point getBoundaryPinGCellNodeLoc(const odb::Point& boundaryPinLoc);

  // debug
  void routeNet_printNet(grNet* net);
};
}  // namespace drt
