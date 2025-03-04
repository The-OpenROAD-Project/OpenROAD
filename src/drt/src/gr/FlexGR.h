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

#pragma once

#include <boost/icl/interval_map.hpp>
#include <boost/icl/interval_set.hpp>
#include <memory>
#include <vector>

#include "FlexGRCMap.h"
#include "db/grObj/grNet.h"
#include "frDesign.h"
#include "frRTree.h"
#include "gr/FlexGRGridGraph.h"
namespace odb {
class dbDatabase;
class Rect;
}  // namespace odb
namespace stt {
class SteinerTreeBuilder;
}
namespace drt {
using odb::Rect;


struct Point2D_CUDA {
  int x;
  int y;

  Point2D_CUDA(int x, int y) : x(x), y(y) {}
};



struct Rect2D_CUDA {
  int xMin;
  int yMin;
  int xMax;
  int yMax;

  Rect2D_CUDA(int xMin, int yMin, int xMax, int yMax) : xMin(xMin), yMin(yMin), xMax(xMax), yMax(yMax) {}
};


enum Directions2D {
  DIR_NORTH    = 0,
  DIR_RIGHT = 1,
  DIR_SOUTH  = 2,
  DIR_LEFT  = 3,
  DIR_NONE  = 255
};


struct NodeData2D {
  // forward and backward propagation (heuristic and real cost) (32 bits each)
  uint32_t forward_h_cost; // heuristic cost
  uint32_t forward_g_cost; // real cost
  uint32_t backward_h_cost; // heuristic cost
  uint32_t backward_g_cost; // real cost
  uint32_t forward_h_cost_prev; 
  uint32_t forward_g_cost_prev;
  uint32_t backward_h_cost_prev;
  uint32_t backward_g_cost_prev;
  
  // Store the direction (for turning point cost and path reconstruction)
  uint8_t forward_direction;
  uint8_t backward_direction;
  uint8_t forward_direction_prev;
  uint8_t backward_direction_prev;
  int golden_parent_x;
  int golden_parent_y;

  // Flags (1 bit each, packed into a single 8-bit field)
  struct Flags {
    uint8_t src_flag : 1; // 1 if this node is the source
    uint8_t dst_flag : 1; // 1 if this node is the destination
    uint8_t forward_update_flag: 1; // 1 if the forward cost is updated
    uint8_t backward_update_flag: 1; // 1 if the backward cost is updated
    uint8_t forward_visited_flag: 1; // 1 if the forward node is visited
    uint8_t backward_visited_flag: 1; // 1 if the backward node is visited
    uint8_t forward_visited_flag_prev: 1; // 1 if the forward node is visited
    uint8_t backward_visited_flag_prev: 1; // 1 if the backward node is visited
  }  flags;
};






class FlexGR
{
 public:
  // constructors
  FlexGR(frDesign* designIn,
         Logger* logger,
         stt::SteinerTreeBuilder* stt_builder,
         RouterConfiguration* router_cfg)
      : db_(nullptr),
        design_(designIn),
        cmap_(nullptr),
        cmap2D_(nullptr),
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
  odb::dbDatabase* db_;
  frDesign* design_;
  std::unique_ptr<FlexGRCMap> cmap_;
  std::unique_ptr<FlexGRCMap> cmap2D_;
  Logger* logger_;
  stt::SteinerTreeBuilder* stt_builder_;
  RouterConfiguration* router_cfg_;
  std::map<frNet*,
           std::map<std::pair<int, int>, std::vector<frNode*>>,
           frBlockObjectComp>
      net2GCellIdx2Nodes_;
  std::map<frNet*, std::vector<frNode*>, frBlockObjectComp> net2GCellNodes_;
  std::map<frNet*, std::vector<frNode*>, frBlockObjectComp> net2SteinerNodes_;
  std::map<frNet*,
           std::map<frNode*, std::vector<frNode*>, frBlockObjectComp>,
           frBlockObjectComp>
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
                                          Point& turnLoc);
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
      std::map<Point, frNode*>& pinGCell2Nodes,
      std::map<frCoord, boost::icl::interval_set<frCoord>>& horzIntvs,
      std::map<frCoord, boost::icl::interval_set<frCoord>>& vertIntvs,
      std::map<Point, frNode*>& steinerGCell2Nodes,
      std::vector<frNode*>& steinerNodes);
  // utility
  void writeToGuide();
  void updateDb();
  void getBatchInfo(int& batchStepX, int& batchStepY);

  // For GPU-accelerated GGR-TR
  bool gpuFlag_ = true;
  void searchRepair_update(
    int iter,
    int size,
    int offset,
    int mazeEndIter,
    unsigned workerCongCost,
    unsigned workerHistCost,
    double congThresh,
    bool is2DRouting,
    RipUpMode mode);

  void batchGenerationMIS(
    std::vector<std::vector<grNet*>> &rerouteNets,
    std::vector<std::vector<grNet*>> &batches,
    std::vector<int>& validBatchIds,
    int iter,
    bool is2DRouting);

  float GPUAccelerated2DMazeRoute(
    std::vector<std::unique_ptr<FlexGRWorker>>& uworkers,
    std::vector<grNet*>& nets,
    std::vector<uint64_t>& h_costMap,
    std::vector<int>& xCoords,
    std::vector<int>& yCoords,
    RouterConfiguration* router_cfg,
    float congThresh,
    int xDim, int yDim);

  float GPUAccelerated2DMazeRoute_update(
    std::vector<std::unique_ptr<FlexGRWorker> >& uworkers,
    std::vector<std::vector<grNet*> >& netBatches,
    std::vector<int>& validBatches,
    std::vector<Point2D_CUDA>& h_parents,
    std::vector<uint64_t>& h_costMap,
    std::vector<int>& h_xCoords,
    std::vector<int>& h_yCoords,
    RouterConfiguration* router_cfg,
    float congThreshold,
    int xDim, int yDim);


  float GPUAccelerated2DMazeRoute_update_v2(
    std::vector<std::unique_ptr<FlexGRWorker> >& uworkers,
    std::vector<std::vector<grNet*> >& netBatches, 
    std::vector<int>& validBatches,
    std::vector<Point2D_CUDA>& h_parents,
    std::vector<uint64_t>& h_costMap,
    std::vector<int>& h_xCoords,
    std::vector<int>& h_yCoords,
    RouterConfiguration* router_cfg,
    float congThreshold,
    int xDim, int yDim);

  float GPUAccelerated2DMazeRoute_update_v3(
    std::vector<std::unique_ptr<FlexGRWorker> >& uworkers,
    std::vector<std::vector<grNet*> >& netBatches, 
    std::vector<int>& validBatches,
    std::vector<Point2D_CUDA>& h_parents,
    std::vector<uint64_t>& h_costMap,
    std::vector<int>& h_xCoords,
    std::vector<int>& h_yCoords,
    RouterConfiguration* router_cfg,
    float relaxThreshold,
    float congThreshold,
    int xDim, int yDim);

  float GPUAccelerated2DMazeRoute_update_v3_old(
      std::vector<std::unique_ptr<FlexGRWorker> >& uworkers,
      std::vector<std::vector<grNet*> >& netBatches, 
      std::vector<int>& validBatches,
      std::vector<Point2D_CUDA>& h_parents,
      std::vector<uint64_t>& h_costMap,
      std::vector<int>& h_xCoords,
      std::vector<int>& h_yCoords,
      RouterConfiguration* router_cfg,
      float relaxThreshold,
      float congThreshold,
      int xDim, int yDim);
  
  

  void batchGenerationMIS_update(
    std::vector<grNet*> &rerouteNets,
    std::vector<std::vector<grNet*>> &batches,
    std::vector<int> &validBatchIds,
    int iter,
    bool is2DRouting);      

  int validBatchThreshold_ = 100;
  //int maxChunkSize_ = 200;
  // MemPool Cluster Fail for maxChunkSize_ = 200;
  // So we set the maxChunkSize to 100
  int maxChunkSize_ = 150;
  std::vector<grNet*> nets2Ripup_;

  int* d_dX_ = nullptr;
  int* d_dY_ = nullptr;
  uint64_t* d_costMap_ = nullptr;
  int* d_xCoords_ = nullptr;
  int* d_yCoords_ = nullptr;
  NodeData2D* d_nodes_ = nullptr;
  Point2D_CUDA* d_parents_ = nullptr;
  int* d_pinIdxVec_ = nullptr;
  int* d_netPtr_ = nullptr;
  Rect2D_CUDA* d_netBBox_ = nullptr;
  int* d_netBatchIdx_ = nullptr;
  
  int h_costMap_size_ = 0;
  int h_xCoords_size_ = 0;
  int h_yCoords_size_ = 0;
  int h_nodes_size_ = 0;
  int h_parents_size_ = 0;
  int h_pinIdxVec_size_ = 0;
  int h_netPtr_size_ = 0;
  int h_netBBoxVec_size_ = 0;
  int h_netBatchIdxVec_size_ = 0;

  void allocateCUDAMem(
    std::vector<uint64_t>& h_costMap,
    std::vector<int>& h_xCoords,
    std::vector<int>& h_yCoords,
    std::vector<Point2D_CUDA>& h_parents,
    std::vector<int>& pinIdxVec,
    std::vector<int>& netPtr,
    std::vector<Rect2D_CUDA>& netBBoxVec,
    std::vector<int>& netBatchIdxVec,
    int numGrids,
    int maxChunkSize,
    int numNodes);

  void freeCUDAMem();


  







  
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
  void query(const Rect& box,
             frLayerNum layerNum,
             std::vector<grConnFig*>& result) const;
  void query(const Rect& box,
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
  FlexGRWorker(FlexGR* grIn, RouterConfiguration* router_cfg, Logger* logger)
      : design_(grIn->getDesign()),
        gr_(grIn),
        logger_(logger),
        gridGraph_(grIn->getDesign(), this, router_cfg),
        rq_(this)
  {
  }
  // setters
  void setRouteGCellIdxLL(const Point& in) { routeGCellIdxLL_ = in; }
  void setRouteGCellIdxUR(const Point& in) { routeGCellIdxUR_ = in; }
  void setExtBox(const Rect& in) { extBox_ = in; }
  void setRouteBox(const Rect& in) { routeBox_ = in; }
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
  const Point& getRouteGCellIdxLL() const { return routeGCellIdxLL_; }
  Point& getRouteGCellIdxLL() { return routeGCellIdxLL_; }
  const Point& getRouteGCellIdxUR() const { return routeGCellIdxUR_; }
  Point& getRouteGCellIdxUR() { return routeGCellIdxUR_; }
  void getExtBox(Rect& in) const { in = extBox_; }
  const Rect& getExtBox() const { return extBox_; }
  Rect& getExtBox() { return extBox_; }
  void getRouteBox(Rect& in) const { in = routeBox_; }
  const Rect& getRouteBox() const { return routeBox_; }
  Rect& getRouteBox() { return routeBox_; }
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

  float calcPathCost(grNet* net);

  // others
  void initBoundary();
  void main_mt();
  void end();
  void cleanup();

  // for GPU-accelerated Router
  int getWorkerId() const { return workerId_; }
  void setWorkerId(int id) { workerId_ = id; }

  // updated functions for GPU-accelerated Router
  void routePrep_update(std::vector<grNet*> &rerouteNets, int iter);

  // init
  void init();
  void route(std::vector<grNet*> &nets);
  void route_addHistCost_update();
  void route_decayHistCost_update();

  void initGridGraph_fromCMap();  // copy from the cmap
  void initGridGraph_back2CMap(); // copy back to the cmap

  bool routeNet(grNet* net);
  void mazeNetInit(grNet* net);

  bool restoreNet(grNet* net);
  FlexGRGridGraph& getGridGraph() { return gridGraph_; }

  void main_mt_prep(std::vector<grNet*>& rerouteNets, int iter) {
    route_addHistCost_update();
    routePrep_update(rerouteNets, iter);
    for (auto net : rerouteNets) {
      net->setPreCost(calcPathCost(net));
    }
  }

  void main_mt_init(std::vector<grNet*>& rerouteNets) {
    auto LLCorner = getRouteGCellIdxLL();
    for (auto net : rerouteNets) {
      //net->setCPUFlag(false);
      mazeNetInit(net);
      net->setCPUFlag(true);
      net->updateAbsGridCoords(LLCorner);
    }
  }

  void printSrc();

  void main_mt_restore(
    std::vector<grNet*>& rerouteNets, 
    std::vector<Point2D_CUDA>& h_parent,
    int xDim,
    int yDim) {
    for (auto& net : rerouteNets) {
      //net->setCPUFlag(true);
      if (net->getCPUFlag() == false) {
        // update the grid graph accroding to the parent
        int batchId = net->getBatchId();
        int nodeBase = batchId * xDim * yDim;
        auto& gridGraph = getGridGraph();
        auto workerLL = getRouteGCellIdxLL();
        int workerLX = workerLL.x();
        int workerLY = workerLL.y();
        auto& netRouteBox = net->getRouteAbsBBox();
        int LLX = netRouteBox.xMin();
        int LLY = netRouteBox.yMin();
        int URX = netRouteBox.xMax();
        int URY = netRouteBox.yMax();
        int xDimTemp = URX - LLX + 1;
        int numNodes = (URX - LLX + 1) * (URY - LLY + 1);
        for (int localIdx = 0; localIdx < numNodes; localIdx++) {
          int localX = localIdx % xDimTemp;
          int localY = localIdx / xDimTemp;
          int x = localX + LLX;
          int y = localY + LLY;      
          int idx = y * xDim + x + nodeBase;
          x -= workerLX;
          y -= workerLY;
          int parentX = h_parent[idx].x - workerLX;
          int parentY = h_parent[idx].y - workerLY;
          if (parentX < 0 || parentY < 0) {
            continue;
          }

          int absDist = abs(x - parentX) + abs(y - parentY);
          if (absDist > 2) {
            std::cout << "Error: absDist > 2" << std::endl;
            std::cout << "x = " << x << ", y = " << y << ", parentX = " << parentX << ", parentY = " << parentY << std::endl;
            exit(1);
          }

          gridGraph.setGoldenParent2D(x, y, parentX, parentY);      
        }    
        restoreNet(net);
      } else {
        routeNet(net);
      }
    }
  }


  void main_mt_restore_CPU_GPU(
    std::vector<grNet*>& rerouteNets, 
    std::vector<Point2D_CUDA>& h_parent,
    int xDim,
    int yDim) {
    for (auto& net : rerouteNets) {
      //net->setCPUFlag(true);
      if (net->getWorkerId() != getWorkerId()) {
        continue;
      }
      
      if (net->getCPUFlag() == false) {
        // update the grid graph accroding to the parent
        int batchId = net->getBatchId();
        int nodeBase = batchId * xDim * yDim;
        auto& gridGraph = getGridGraph();
        auto workerLL = getRouteGCellIdxLL();
        int workerLX = workerLL.x();
        int workerLY = workerLL.y();
        auto& netRouteBox = net->getRouteAbsBBox();
        int LLX = netRouteBox.xMin();
        int LLY = netRouteBox.yMin();
        int URX = netRouteBox.xMax();
        int URY = netRouteBox.yMax();
        int xDimTemp = URX - LLX + 1;
        int numNodes = (URX - LLX + 1) * (URY - LLY + 1);
        for (int localIdx = 0; localIdx < numNodes; localIdx++) {
          int localX = localIdx % xDimTemp;
          int localY = localIdx / xDimTemp;
          int x = localX + LLX;
          int y = localY + LLY;      
          int idx = y * xDim + x + nodeBase;
          x -= workerLX;
          y -= workerLY;
          if (idx >= h_parent.size()) {
            std::cout << "Error: idx >= h_parent.size()" << std::endl;
            std::cout << "idx = " << idx << ", h_parent.size() = " << h_parent.size() << std::endl;
            std::cout << "xDim = " << xDim << ", yDim = " << yDim << std::endl;
            std::cout << "batchId = " << batchId << ", nodeBase = " << nodeBase << std::endl;
            std::cout << "x = " << x << ", y = " << y << std::endl;
            std::cout << "numNodes = " << numNodes << std::endl;
            std::cout << "LLX = " << LLX << ", LLY = " << LLY << ", URX = " << URX << ", URY = " << URY << std::endl; 
            exit(1);
          }
          
          int parentX = h_parent[idx].x - workerLX;
          int parentY = h_parent[idx].y - workerLY;
          
          if (parentX < 0 || parentY < 0) {
            continue;
          }

          int absDist = abs(x - parentX) + abs(y - parentY);
          if (absDist > 2) {
            std::cout << "Error: absDist > 2" << std::endl;
            std::cout << "x = " << x << ", y = " << y << ", parentX = " << parentX << ", parentY = " << parentY << std::endl;
            exit(1);
          }

          gridGraph.setGoldenParent2D(x, y, parentX, parentY);      
        }    
        restoreNet(net);
      } else {
        routeNet(net);
      }
    }
  }


  void printPinNodePairs(grNet* net);

  void main_mt_restore_temp(
    std::vector<grNet*>& rerouteNets) {
    for (auto& net : rerouteNets) {
      net->setCPUFlag(true);
      if (net->getCPUFlag() == false) {
        restoreNet(net);
      } else {
        routeNet(net);
      }
    }
  }


  void main_mt_restore_CPU_only(
    std::vector<grNet*>& rerouteNets) {
    for (auto& net : rerouteNets) {
      if (net->getWorkerId() != getWorkerId()) {
        continue;
      }
      //net->setCPUFlag(true);
      routeNet(net);
    }
  }

  void batchGenerationRelax(
    std::vector<grNet*>& rerouteNets,
    std::vector<std::vector<grNet*>>& batches);

 private:
  frDesign* design_{nullptr};
  FlexGR* gr_{nullptr};
  Logger* logger_;
  Point routeGCellIdxLL_;
  Point routeGCellIdxUR_;
  Rect extBox_;
  Rect routeBox_;
  int grIter_{0};
  int mazeEndIter_{1};
  int workerCongCost_{0};
  int workerHistCost_{0};
  double congThresh_{1.0};
  bool is2DRouting_{false};
  RipUpMode ripupMode_{RipUpMode::DRC};
  int workerId_{0};

  // local storage
  std::vector<std::unique_ptr<grNet>> nets_;
  std::map<frNet*, std::vector<grNet*>, frBlockObjectComp> owner2nets_;

  FlexGRGridGraph gridGraph_;
  FlexGRWorkerRegionQuery rq_;

  // initBoundary
  void initBoundary_splitPathSeg(grPathSeg* pathSeg);
  void initBoundary_splitPathSeg_getBreakPts(const Point& bp,
                                             const Point& ep,
                                             Point& breakPt1,
                                             Point& breakPt2);
  frNode* initBoundary_splitPathSeg_split(frNode* child,
                                          frNode* parent,
                                          const Point& breakPt);

 
  void initNets();
  void initNets_roots(
      std::set<frNet*, frBlockObjectComp>& nets,
      std::map<frNet*, std::vector<frNode*>, frBlockObjectComp>& netRoots);
  void initNetObjs_roots_pathSeg(
      grPathSeg* pathSeg,
      std::set<frNet*, frBlockObjectComp>& nets,
      std::map<frNet*, std::vector<frNode*>, frBlockObjectComp>& netRoots);
  void initNetObjs_roots_via(
      grVia* via,
      std::set<frNet*, frBlockObjectComp>& nets,
      std::map<frNet*, std::vector<frNode*>, frBlockObjectComp>& netRoots);
  void initNets_searchRepair(
      std::set<frNet*, frBlockObjectComp>& nets,
      std::map<frNet*, std::vector<frNode*>, frBlockObjectComp>& netRoots);
  void initNet(frNet* net, const std::vector<frNode*>& netRoots);
  void initNet_initNodes(grNet* net, frNode* fRoot);
  void initNet_initRoot(grNet* net);
  void initNet_updateCMap(grNet* net, bool isAdd);
  void initNet_initPinGCellNodes(grNet* net);
  void initNet_initObjs(grNet* net);
  void initNet_addNet(std::unique_ptr<grNet>& in);
  void initNets_regionQuery();
  void initGridGraph();
 

  // debug
  void initNets_printNets();
  void initNets_printNet(grNet* net);
  void initNets_printFNets(
      std::map<frNet*, std::vector<frNode*>, frBlockObjectComp>& netRoots);
  void initNets_printFNet(frNode* root);

  // route
  void route();
  void route_addHistCost();
  void route_mazeIterInit();
  void route_getRerouteNets(std::vector<grNet*>& rerouteNets);
 
  bool mazeNetHasCong(grNet* net);
  void mazeNetInit_addHistCost(grNet* net);
  void mazeNetInit_decayHistCost(grNet* net);
  void mazeNetInit_removeNetObjs(grNet* net);
  void modCong_pathSeg(grPathSeg* pathSeg, bool isAdd);
  void mazeNetInit_removeNetNodes(grNet* net);
 
  void routeNet_prep(grNet* net,
                     std::set<grNode*, frBlockObjectComp>& unConnPinGCellNodes,
                     std::map<FlexMazeIdx, grNode*>& mazeIdx2unConnPinGCellNode,
                     std::map<FlexMazeIdx, grNode*>& mazeIdx2endPointNode);
  void routeNet_setSrc(
      grNet* net,
      std::set<grNode*, frBlockObjectComp>& unConnPinGCellNodes,
      std::map<FlexMazeIdx, grNode*>& mazeIdx2unConnPinGCellNode,
      std::vector<FlexMazeIdx>& connComps,
      FlexMazeIdx& ccMazeIdx1,
      FlexMazeIdx& ccMazeIdx2,
      Point& centerPt);
  grNode* routeNet_getNextDst(
      FlexMazeIdx& ccMazeIdx1,
      FlexMazeIdx& ccMazeIdx2,
      std::map<FlexMazeIdx, grNode*>& mazeIdx2unConnPinGCellNode);
  grNode* routeNet_postAstarUpdate(
      std::vector<FlexMazeIdx>& path,
      std::vector<FlexMazeIdx>& connComps,
      std::set<grNode*, frBlockObjectComp>& unConnPinGCellNodes,
      std::map<FlexMazeIdx, grNode*>& mazeIdx2unConnPinGCellNode);
  void routeNet_postAstarWritePath(
      grNet* net,
      std::vector<FlexMazeIdx>& points,
      grNode* leaf,
      std::map<FlexMazeIdx, grNode*>& mazeIdx2endPointNode);
  grNode* routeNet_postAstarWritePath_splitPathSeg(grNode* child,
                                                   grNode* parent,
                                                   const Point& breakPt);
  void routeNet_postRouteAddCong(grNet* net);
  void route_decayHistCost();

  // end
  void endGetModNets(std::set<frNet*, frBlockObjectComp>& modNets);
  void endRemoveNets(const std::set<frNet*, frBlockObjectComp>& modNets);
  void endRemoveNets_objs(const std::set<frNet*, frBlockObjectComp>& modNets);
  void endRemoveNets_pathSeg(grPathSeg* pathSeg);
  void endRemoveNets_via(grVia* via);
  void endRemoveNets_nodes(const std::set<frNet*, frBlockObjectComp>& modNets);
  void endRemoveNets_nodes_net(grNet* net, frNet* fnet);
  void endRemoveNets_node(frNode* node);
  void endAddNets(std::set<frNet*, frBlockObjectComp>& modNets);
  void endAddNets_stitchRouteBound(grNet* net);
  void endAddNets_stitchRouteBound_node(grNode* node);
  void endAddNets_addNet(grNet* net, frNet* fnet);
  void endStitchBoundary();
  void endStitchBoundary_net(grNet* net);
  void endWriteBackCMap();

  // other
  Point getBoundaryPinGCellNodeLoc(const Point& boundaryPinLoc);

  // debug
  void routeNet_printNet(grNet* net); 

  // For GPU-accelerated GGR-TR
  void init_pinGCellIdxs();
  bool restorePath(std::vector<FlexMazeIdx>& connComps,
    grNode* nextPinNode,
    std::vector<FlexMazeIdx>& path,
    FlexMazeIdx& ccMazeIdx1,
    FlexMazeIdx& ccMazeIdx2,
    const Point& centerPt);
  void routeNet_checkNet(grNet* net);
};


// GPU-accelerated maze routing









}  // namespace drt
