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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FR_FLEXGR_H_
#define _FR_FLEXGR_H_

#include <memory>
#include "frDesign.h"
#include "FlexGRCMap.h"
#include "frRTree.h"
#include "db/grObj/grNet.h"
#include "gr/FlexGRGridGraph.h"
#include <boost/icl/interval_map.hpp>
#include <boost/icl/interval_set.hpp>
#include "flute.h"
namespace odb
{
  class dbDatabase;
}
namespace fr {

  class FlexGR {
  public:
    // constructors
    FlexGR(frDesign* designIn, Logger* logger)
      : design_(designIn),
        cmap_(nullptr),
        cmap2D_(nullptr),
        logger_(logger)
    {
    }

    // getters
    frTechObject* getTech() const {
      return design_->getTech();
    }
    frDesign* getDesign() const {
      return design_;
    }
    frRegionQuery* getRegionQuery() const {
      return design_->getRegionQuery();
    }
    FlexGRCMap* getCMap(bool is2DCMap) const {
      if (is2DCMap) {
        return cmap2D_.get();
      } else {
        return cmap_.get();
      }
    }

    // others
    void main(odb::dbDatabase* db = nullptr);

  protected:
    frDesign *design_;
    std::unique_ptr<FlexGRCMap> cmap_;
    std::unique_ptr<FlexGRCMap> cmap2D_;
    Logger*         logger_;
    std::map<frNet*, std::map<std::pair<int, int>, std::vector<frNode*> >, frBlockObjectComp> net2GCellIdx2Nodes_;
    std::map<frNet*, std::vector<frNode*>, frBlockObjectComp> net2GCellNodes_;
    std::map<frNet*, std::vector<frNode*>, frBlockObjectComp> net2SteinerNodes_;
    std::map<frNet*, std::map<frNode*, std::vector<frNode*>, frBlockObjectComp>, frBlockObjectComp> net2GCellNode2RPinNodes_;
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

    void searchRepairMacro(int iter, int size, int mazeEndIter, unsigned workerCongCost,
                           unsigned workerHistCost, double congThresh, bool is2DRouting,
                           int mode);
    void searchRepair(int iter, int size, int offset, int mazeEndIter, 
                      unsigned workerCongCost, unsigned workerHistCost,
                      double congThresh, bool is2DRouting, int mode, bool TEST);

    void end();

    void setCMap(std::unique_ptr<FlexGRCMap> &in) {
      cmap_ = std::move(in);
    }
    void setCMap2D(std::unique_ptr<FlexGRCMap> &in) {
      cmap2D_ = std::move(in);
    }

    // initGR
    void initGR();
    void initGR_genTopology();
    void initGR_genTopology_net(frNet *net);
    void initGR_updateCongestion();
    void initGR_updateCongestion_net(frNet *net);
    void initGR_updateCongestion2D_net(frNet *net);
    void initGR_patternRoute();
    void initGR_patternRoute_init(std::vector<std::pair<std::pair<frNode*, frNode*>, int> > &patternRoutes);
    void initGR_patternRoute_route(std::vector<std::pair<std::pair<frNode*, frNode*>, int> > &patternRoutes);
    bool initGR_patternRoute_route_iter(int iter, std::vector<std::pair<std::pair<frNode*, frNode*>, int> > &patternRoutes, int mode);
    void initGR_initObj();
    void initGR_initObj_net(frNet* net);

    // pattern route
    void patternRoute_LShape(frNode *child, frNode *parent);

    // layer assignment
    void layerAssign();
    void layerAssign_net(frNet *net);
    void layerAssign_node_compute(frNode *currNode, frNet *net, std::vector<std::vector<unsigned> > &bestLayerCosts,
                                         std::vector<std::vector<unsigned> > &bestLayers);
    void layerAssign_node_commit(frNode *currNode, frNet *net, frLayerNum layerNum,
                                        std::vector<std::vector<unsigned> > &bestLayers);
    

    // cost
    double getCongCost(unsigned supply, unsigned demand);

    // others
    void ripupRoute(frNode *child, frNode *parent);
    bool hasOverflow2D(frNode *child, frNode *parent);
    void reportCong2DGolden(FlexGRCMap *baseCMap2D);
    void reportCong2D(FlexGRCMap *cmap2D);
    void reportCong2D();
    void reportCong3DGolden(FlexGRCMap *baseCMap);
    void reportCong3D(FlexGRCMap *cmap);
    void reportCong3D();
    void updateDbCongestion(odb::dbDatabase* db, FlexGRCMap *cmap);

    // temp
    void initGR_patternRoute_layerAssignment();
    void initGR_patternRoute_layerAssignment_net(frNet* net);

    // search and repair

    // topology
    void genMSTTopology(std::vector<frNode*> &nodes);
    void genMSTTopology_PD(std::vector<frNode*> &nodes, double alpha = 0.3);
    int genMSTTopology_PD_minIdx(const std::vector<int> &keys, const std::vector<bool> &isVisited);

    void genSTTopology_FLUTE(std::vector<frNode*> &nodes, std::vector<frNode*> &steinerNodes);
    void genSTTopology_HVW(std::vector<frNode*> &nodes, std::vector<frNode*> &steinerNodes);
    void genSTTopology_HVW_compute(frNode *currNode, std::vector<frNode*> &nodes, std::vector<unsigned> &overlapL, 
                                   std::vector<unsigned> &overlapU, std::vector<unsigned> &bestCombL, std::vector<unsigned> &bestCombU);
    unsigned genSTTopology_HVW_levelOvlp(frNode *currNode, bool isCurrU, unsigned comb);
    void genSTTopology_HVW_levelOvlp_helper(frNode *parent, frNode *child, bool isCurrU,
                                            std::pair<frCoord, frCoord> &horzIntv, std::pair<frCoord, frCoord> &vertIntv, frPoint &turnLoc);
    void genSTTopology_HVW_commit(frNode *currNode, bool isCurrU, std::vector<frNode*> &nodes,
                                  std::vector<unsigned> &bestCombL, std::vector<unsigned> &bestCombU, std::vector<bool> &isU);

    void genSTTopology_build_tree(std::vector<frNode*> &pinNodes, std::vector<bool> &isU, std::vector<frNode*> &steinerNodes);
    void genSTTopology_build_tree_mergeSeg(std::vector<frNode*> &pinNodes, std::vector<bool> &isU, 
                                           std::map<frCoord, boost::icl::interval_set<frCoord> > &horzIntvs,
                                           std::map<frCoord, boost::icl::interval_set<frCoord> > &vertIntvs);
    void genSTTopology_build_tree_splitSeg(std::vector<frNode*> &pinNodes,
                                           std::map<frPoint, frNode*> &pinGCell2Nodes,
                                           std::map<frCoord, boost::icl::interval_set<frCoord> > &horzIntvs,
                                           std::map<frCoord, boost::icl::interval_set<frCoord> > &vertIntvs,
                                           std::map<frPoint, frNode*> &steinerGCell2Nodes,
                                           std::vector<frNode*> &steinerNodes);
    // utility
    void writeToGuide();
    void writeGuideFile();
    void getBatchInfo(int &batchStepX, int &batchStepY);
  };

  class FlexGRWorker;
  class FlexGRWorkerRegionQuery {
  public:
    FlexGRWorkerRegionQuery(FlexGRWorker* in): grWorker_(in) {}
    frDesign* getDesign() const;
    FlexGRWorker* getGRWorker() const {
        return grWorker_;
    }
    void add(grConnFig* connFig);
    void add(grConnFig* connFig, std::vector<std::vector<rq_box_value_t<grConnFig*>>> &allShapes);
    void remove(grConnFig* connFig);
    void query(const frBox &box, frLayerNum layerNum, std::vector<grConnFig*> &result);
    void query(const frBox &box, frLayerNum layerNum, std::vector<rq_box_value_t<grConnFig*>> &result);
    void init(bool includeExt = false);
    void cleanup() {
      shapes_.clear();
      shapes_.shrink_to_fit();
    }
    void report() {
      for (int i = 0; i < (int)shapes_.size(); i++) {
        std::cout << " layerIdx = " << i << ", numObj = " << shapes_[i].size() << "\n";
      }
    }
    
  protected:
    FlexGRWorker* grWorker_;
    std::vector<bgi::rtree<rq_box_value_t<grConnFig*>, bgi::quadratic<16>>> shapes_; // only for routeConnFigs in gr worker
  };

  class FlexGRWorker {
  public:
    // constructors
    FlexGRWorker(FlexGR* grIn):
                 design_(grIn->getDesign()), gr_(grIn), routeGCellIdxLL_(), routeGCellIdxUR_(),
                 extBox_(), routeBox_(), grIter_(0), mazeEndIter_(1), workerCongCost_(0), workerHistCost_(0), 
                 congThresh_(1.0), is2DRouting_(false), ripupMode_(0),
                 nets_(), owner2nets_(), /*owner2extBoundPtNodes(), owner2routeBoundPtNodes(), owner2pinGCellNodes(),*/
                 gridGraph_(grIn->getDesign(), this), rq_(this) {}
    // setters
    void setRouteGCellIdxLL(const frPoint &in) {
      routeGCellIdxLL_ = in;
    }
    void setRouteGCellIdxUR(const frPoint &in) {
      routeGCellIdxUR_ = in;
    }
    void setExtBox(const frBox &in) {
      extBox_.set(in);
    }
    void setRouteBox(const frBox &in) {
      routeBox_.set(in);
    }
    void setGRIter(int in) {
      grIter_ = in;
    }
    void setMazeEndIter(int in) {
      mazeEndIter_ = in;
    }
    void setCongCost(int in) {
      workerCongCost_ = in;
    }
    void setHistCost(int in) {
      workerHistCost_ = in;
    }
    void setCongThresh(double in) {
      congThresh_ = in;
    }
    void set2D(bool in) {
      is2DRouting_ = in;
    }
    void setRipupMode(int in) {
      ripupMode_ = in;
    }

    // getters
    frTechObject* getTech() const {
      return design_->getTech();
    }
    frDesign* getDesign() const {
      return design_;
    }
    FlexGR* getGR() const {
      return gr_;
    }
    const frPoint& getRouteGCellIdxLL() const {
      return routeGCellIdxLL_;
    }
    frPoint& getRouteGCellIdxLL() {
      return routeGCellIdxLL_;
    }
    const frPoint& getRouteGCellIdxUR() const {
      return routeGCellIdxUR_;
    }
    frPoint& getRouteGCellIdxUR() {
      return routeGCellIdxUR_;
    }
    void getExtBox(frBox &in) const {
      in.set(extBox_);
    }
    const frBox& getExtBox() const {
      return extBox_;
    }
    frBox& getExtBox() {
      return extBox_;
    }
    void getRouteBox(frBox &in) const {
      in.set(routeBox_);
    }
    const frBox& getRouteBox() const {
      return routeBox_;
    }
    frBox& getRouteBox() {
      return routeBox_;
    }
    int getGRIter() const {
      return grIter_;
    }
    int getMazeEndIter() const {
      return mazeEndIter_;
    }
    double getCongThresh() const {
      return congThresh_;
    }
    bool is2D() const {
      return is2DRouting_;
    }
    frRegionQuery* getRegionQuery() const {
      return design_->getRegionQuery();
    }
    FlexGRCMap* getCMap() const {
      return gr_->getCMap(is2DRouting_);
    }
    const std::vector<std::unique_ptr<grNet> >& getNets() const {
      return nets_;
    }
    std::vector<std::unique_ptr<grNet> >& getNets() {
      return nets_;
    }
    const std::vector<grNet*>* getGRNets(frNet* net) const {
      auto it = owner2nets_.find(net);
      if (it != owner2nets_.end()) {
        return &(it->second);
      } else {
        return nullptr;
      }
    }
    const FlexGRWorkerRegionQuery& getWorkerRegionQuery() const {
      return rq_;
    }
    FlexGRWorkerRegionQuery& getWorkerRegionQuery() {
      return rq_;
    }

    // others
    void initBoundary();
    void main_mt();
    void end();
    void cleanup();


  protected:
    frDesign*  design_;
    FlexGR*    gr_;
    frPoint    routeGCellIdxLL_;
    frPoint    routeGCellIdxUR_;
    frBox      extBox_;
    frBox      routeBox_;
    int        grIter_;
    int        mazeEndIter_;
    int        workerCongCost_;
    int        workerHistCost_;
    double     congThresh_;
    bool       is2DRouting_;
    int        ripupMode_;

    // local storage
    std::vector<std::unique_ptr<grNet> >   nets_;
    std::map<frNet*, std::vector<grNet*>, frBlockObjectComp> owner2nets_;

    FlexGRGridGraph                        gridGraph_;
    FlexGRWorkerRegionQuery                rq_;

    // initBoundary
    void initBoundary_splitPathSeg(grPathSeg* pathSeg);
    void initBoundary_splitPathSeg_getBreakPts(const frPoint &bp, const frPoint &ep, frPoint &breakPt1, frPoint &breakPt2);
    frNode* initBoundary_splitPathSeg_split(frNode* child, frNode* parent, const frPoint &breakPt);


    // init
    void init();
    void initNets();
    void initNets_roots(std::set<frNet*, frBlockObjectComp> &nets,
                        std::map<frNet*, std::vector<frNode*>, frBlockObjectComp> &netRoots);
    void initNetObjs_roots_pathSeg(grPathSeg* pathSeg,
                                   std::set<frNet*, frBlockObjectComp> &nets,
                                   std::map<frNet*, std::vector<frNode*>, frBlockObjectComp> &netRoots);
    void initNetObjs_roots_via(grVia* via,
                               std::set<frNet*, frBlockObjectComp> &nets,
                               std::map<frNet*, std::vector<frNode*>, frBlockObjectComp> &netRoots);
    void initNets_searchRepair(std::set<frNet*, frBlockObjectComp> &nets,
                               std::map<frNet*, std::vector<frNode*>, frBlockObjectComp> &netRoots);
    void initNet(frNet* net, const std::vector<frNode*> &netRoots);
    void initNet_initNodes(grNet* net, frNode* fRoot);
    void initNet_initRoot(grNet* net);
    void initNet_updateCMap(grNet* net, bool isAdd);
    void initNet_initPinGCellNodes(grNet* net);
    void initNet_initObjs(grNet* net);
    void initNet_addNet(std::unique_ptr<grNet> &in);
    void initNets_regionQuery();
    void initGridGraph();
    void initGridGraph_fromCMap();


    // debug
    void initNets_printNets();
    void initNets_printNet(grNet* net);
    void initNets_printFNets(std::map<frNet*, std::vector<frNode*>, frBlockObjectComp> &netRoots);
    void initNets_printFNet(frNode* root);
   
    // route
    void route();
    void route_addHistCost();
    void route_mazeIterInit();
    void route_getRerouteNets(std::vector<grNet*> &rerouteNets);
    void mazeNetInit(grNet* net);
    bool mazeNetHasCong(grNet* net);
    void mazeNetInit_addHistCost(grNet* net);
    void mazeNetInit_decayHistCost(grNet* net);
    void mazeNetInit_removeNetObjs(grNet* net);
    void modCong_pathSeg(grPathSeg* pathSeg, bool isAdd);
    void mazeNetInit_removeNetNodes(grNet* net);
    bool routeNet(grNet* net);
    void routeNet_prep(grNet* net, std::set<grNode*, frBlockObjectComp> &unConnPinGCellNodes, 
                       std::map<FlexMazeIdx, grNode*> &mazeIdx2unConnPinGCellNode,
                       std::map<FlexMazeIdx, grNode*> &mazeIdx2endPointNode);
    void routeNet_setSrc(grNet* net, 
                         std::set<grNode*, frBlockObjectComp> &unConnPinGCellNodes, 
                         std::map<FlexMazeIdx, grNode*> &mazeIdx2unConnPinGCellNode,
                         std::vector<FlexMazeIdx> &connComps,
                         FlexMazeIdx &ccMazeIdx1, 
                         FlexMazeIdx &ccMazeIdx2, 
                         frPoint &centerPt);
    grNode* routeNet_getNextDst(FlexMazeIdx &ccMazeIdx1, FlexMazeIdx &ccMazeIdx2, 
                                std::map<FlexMazeIdx, grNode*> &mazeIdx2unConnPinGCellNode);
    grNode* routeNet_postAstarUpdate(std::vector<FlexMazeIdx> &path, std::vector<FlexMazeIdx> &connComps,
                                     std::set<grNode*, frBlockObjectComp> &unConnPinGCellNodes, 
                                     std::map<FlexMazeIdx, grNode*> &mazeIdx2unConnPinGCellNode);
    void routeNet_postAstarWritePath(grNet* net, std::vector<FlexMazeIdx> &points, grNode* leaf,
                                     std::map<FlexMazeIdx, grNode*> &mazeIdx2endPointNode);
    grNode* routeNet_postAstarWritePath_splitPathSeg(grNode* child, grNode* parent, const frPoint &breakPt);
    void routeNet_postRouteAddCong(grNet* net);
    void route_decayHistCost();

    // end
    void endGetModNets(std::set<frNet*, frBlockObjectComp> &modNets);
    void endRemoveNets(const std::set<frNet*, frBlockObjectComp> &modNets);
    void endRemoveNets_objs(const std::set<frNet*, frBlockObjectComp> &modNets);
    void endRemoveNets_pathSeg(grPathSeg* pathSeg);
    void endRemoveNets_via(grVia* via);
    void endRemoveNets_nodes(const std::set<frNet*, frBlockObjectComp> &modNets);
    void endRemoveNets_nodes_net(grNet* net, frNet* fnet);
    void endRemoveNets_node(frNode* node);
    void endAddNets(std::set<frNet*, frBlockObjectComp> &modNets);
    void endAddNets_stitchRouteBound(grNet* net);
    void endAddNets_stitchRouteBound_node(grNode* node);
    void endAddNets_addNet(grNet* net, frNet* fnet);
    void endStitchBoundary();
    void endStitchBoundary_net(grNet* net);
    void endWriteBackCMap();

    // other
    frPoint getBoundaryPinGCellNodeLoc(const frPoint &boundaryPinLoc);

    // debug
    void routeNet_printNet(grNet* net);

  };
}


#endif
