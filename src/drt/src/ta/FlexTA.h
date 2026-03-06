// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "db/obj/frBlockObject.h"
#include "db/obj/frVia.h"
#include "db/taObj/taFig.h"
#include "db/taObj/taPin.h"
#include "db/tech/frConstraint.h"
#include "db/tech/frTechObject.h"
#include "db/tech/frViaDef.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "frRegionQuery.h"
#include "global.h"
#include "utl/Logger.h"

namespace drt {
class FlexTAGraphics;
class AbstractTAGraphics;

class FlexTA
{
 public:
  // constructors
  FlexTA(frDesign* in,
         utl::Logger* logger,
         RouterConfiguration* router_cfg,
         bool save_updates_);
  ~FlexTA();
  // getters
  frTechObject* getTech() const { return tech_; }
  frDesign* getDesign() const { return design_; }
  // others
  int main();
  void setDebug(std::unique_ptr<AbstractTAGraphics> ta_graphics);

 private:
  frTechObject* tech_;
  frDesign* design_;
  utl::Logger* logger_;
  RouterConfiguration* router_cfg_;
  bool save_updates_;
  std::unique_ptr<AbstractTAGraphics> graphics_;
  // others
  void main_helper(frLayerNum lNum, int maxOffsetIter, int panelWidth);
  void initTA(int size);
  void searchRepair(int iter, int size, int offset);
  int initTA_helper(int iter,
                    int size,
                    int offset,
                    bool is_horizontal,
                    int& num_panels);
};

class FlexTAWorker;
class FlexTAWorkerRegionQuery
{
 public:
  FlexTAWorkerRegionQuery(FlexTAWorker* in);
  ~FlexTAWorkerRegionQuery();
  frDesign* getDesign() const;
  FlexTAWorker* getTAWorker() const;

  void add(taPinFig* fig);
  void remove(taPinFig* fig);
  void query(const odb::Rect& box,
             frLayerNum layer_num,
             frOrderedIdSet<taPin*>& result) const;

  void addCost(const odb::Rect& box,
               frLayerNum layer_num,
               frBlockObject* obj,
               frConstraint* con);
  void removeCost(const odb::Rect& box,
                  frLayerNum layer_num,
                  frBlockObject* obj,
                  frConstraint* con);
  void queryCost(
      const odb::Rect& box,
      frLayerNum layer_num,
      std::vector<rq_box_value_t<std::pair<frBlockObject*, frConstraint*>>>&
          result) const;
  void addViaCost(const odb::Rect& box,
                  frLayerNum layer_num,
                  frBlockObject* obj,
                  frConstraint* con);
  void removeViaCost(const odb::Rect& box,
                     frLayerNum layer_num,
                     frBlockObject* obj,
                     frConstraint* con);
  void queryViaCost(
      const odb::Rect& box,
      frLayerNum layer_num,
      std::vector<rq_box_value_t<std::pair<frBlockObject*, frConstraint*>>>&
          result) const;

  void init();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

class FlexTAWorker
{
 public:
  // constructors
  FlexTAWorker(frDesign* designIn,
               utl::Logger* logger,
               RouterConfiguration* router_cfg,
               bool save_updates)
      : design_(designIn),
        logger_(logger),
        router_cfg_(router_cfg),
        save_updates_(save_updates),
        dir_(odb::dbTechLayerDir::NONE),
        rq_(this)
  {
  }
  // setters
  void setRouteBox(const odb::Rect& boxIn) { routeBox_ = boxIn; }
  void setExtBox(const odb::Rect& boxIn) { extBox_ = boxIn; }
  void setDir(const odb::dbTechLayerDir& in) { dir_ = in; }
  void setTAIter(int in) { taIter_ = in; }
  void addIroute(std::unique_ptr<taPin> in, bool isExt = false)
  {
    in->setId(iroutes_.size() + extIroutes_.size());
    if (isExt) {
      extIroutes_.push_back(std::move(in));
    } else {
      iroutes_.push_back(std::move(in));
    }
  }
  void addToReassignIroutes(taPin* in) { reassignIroutes_.insert(in); }
  void removeFromReassignIroutes(taPin* in)
  {
    auto it = reassignIroutes_.find(in);
    if (it != reassignIroutes_.end()) {
      reassignIroutes_.erase(it);
    }
  }
  taPin* popFromReassignIroutes()
  {
    taPin* sol = nullptr;
    if (!reassignIroutes_.empty()) {
      sol = *reassignIroutes_.begin();
      reassignIroutes_.erase(reassignIroutes_.begin());
    }
    return sol;
  }

  // getters
  frTechObject* getTech() const { return design_->getTech(); }
  frDesign* getDesign() const { return design_; }
  const odb::Rect& getRouteBox() const { return routeBox_; }
  const odb::Rect& getExtBox() const { return extBox_; }
  odb::dbTechLayerDir getDir() const { return dir_; }
  int getTAIter() const { return taIter_; }
  bool isInitTA() const { return (taIter_ == 0); }
  frRegionQuery* getRegionQuery() const { return design_->getRegionQuery(); }
  void getTrackIdx(frCoord loc1,
                   frCoord loc2,
                   frLayerNum layer_num,
                   int& idx1,
                   int& idx2) const
  {
    idx1 = int(std::lower_bound(trackLocs_[layer_num].begin(),
                                trackLocs_[layer_num].end(),
                                loc1)
               - trackLocs_[layer_num].begin());
    idx2 = int(std::upper_bound(trackLocs_[layer_num].begin(),
                                trackLocs_[layer_num].end(),
                                loc2)
               - trackLocs_[layer_num].begin())
           - 1;
  }
  const std::vector<frCoord>& getTrackLocs(frLayerNum in) const
  {
    return trackLocs_[in];
  }
  const FlexTAWorkerRegionQuery& getWorkerRegionQuery() const { return rq_; }
  FlexTAWorkerRegionQuery& getWorkerRegionQuery() { return rq_; }
  int getNumAssigned() const { return numAssigned_; }
  // others
  int main_mt();

 private:
  frDesign* design_;
  utl::Logger* logger_;
  RouterConfiguration* router_cfg_;
  bool save_updates_;
  odb::Rect routeBox_;
  odb::Rect extBox_;
  odb::dbTechLayerDir dir_;
  int taIter_{0};
  FlexTAWorkerRegionQuery rq_;

  std::vector<std::unique_ptr<taPin>> iroutes_;  // unsorted iroutes
  std::vector<std::unique_ptr<taPin>> extIroutes_;
  std::vector<std::vector<frCoord>> trackLocs_;
  std::set<taPin*, taPinComp>
      reassignIroutes_;  // iroutes to be assigned in sorted order
  int numAssigned_{0};
  int totCost_{0};
  int maxRetry_{1};
  bool hardIroutesMode_{false};

  //// others
  void init();
  void initFixedObjs();
  frCoord initFixedObjs_calcBloatDist(frBlockObject* obj,
                                      frLayerNum layer_num,
                                      const odb::Rect& box);
  frCoord initFixedObjs_calcOBSBloatDistVia(const frViaDef* viaDef,
                                            frLayerNum layer_num,
                                            const odb::Rect& box,
                                            bool isOBS = true);
  void initFixedObjs_helper(const odb::Rect& box,
                            frCoord bloatDist,
                            frLayerNum layer_num,
                            frNet* net,
                            bool isViaCost = false);
  void initTracks();
  void initIroutes();
  void initIroute(frGuide* guide);
  void initIroute_helper(frGuide* guide,
                         frCoord& maxBegin,
                         frCoord& minEnd,
                         std::set<frCoord>& downViaCoordSet,
                         std::set<frCoord>& upViaCoordSet,
                         int& nextIrouteDir,
                         frCoord& pinCoord);
  void initIroute_helper_generic(frGuide* guide,
                                 frCoord& minBegin,
                                 frCoord& maxEnd,
                                 std::set<frCoord>& downViaCoordSet,
                                 std::set<frCoord>& upViaCoordSet,
                                 int& nextIrouteDir,
                                 frCoord& pinCoord);
  void initIroute_helper_generic_endpoint(frGuide* guide,
                                          const odb::Point& cp,
                                          bool is_begin,
                                          frLayerNum layer_num,
                                          bool is_horizontal,
                                          frCoord& min_begin,
                                          frCoord& max_end,
                                          bool& has_min_begin,
                                          bool& has_max_end,
                                          std::set<frCoord>& down_via_coord_set,
                                          std::set<frCoord>& up_via_coord_set,
                                          int& next_iroute_dir);
  void initIroute_helper_generic_helper(frGuide* guide, frCoord& pinCoord);
  bool initIroute_helper_pin(frGuide* guide,
                             frCoord& maxBegin,
                             frCoord& minEnd,
                             std::set<frCoord>& downViaCoordSet,
                             std::set<frCoord>& upViaCoordSet,
                             int& nextIrouteDir,
                             frCoord& pinCoord);
  bool initIroute_helper_pin_iterm(frInstTerm* iterm,
                                   frNet* net,
                                   frLayerNum layer_num,
                                   bool is_horizontal,
                                   bool has_down,
                                   bool has_up,
                                   frCoord& max_begin,
                                   frCoord& min_end,
                                   std::set<frCoord>& down_via_coord_set,
                                   std::set<frCoord>& up_via_coord_set,
                                   int& next_iroute_dir,
                                   frCoord& pin_coord);
  bool initIroute_helper_pin_bterm(frBTerm* bterm,
                                   frNet* net,
                                   frLayerNum layer_num,
                                   bool is_horizontal,
                                   bool has_down,
                                   bool has_up,
                                   frCoord& max_begin,
                                   frCoord& min_end,
                                   std::set<frCoord>& down_via_coord_set,
                                   std::set<frCoord>& up_via_coord_set,
                                   int& next_iroute_dir,
                                   frCoord& pin_coord);
  void initFixedObjs_processTerm(frBlockObject* obj,
                                 frLayerNum layer_num,
                                 const odb::Rect& bounds,
                                 odb::Rect& box,
                                 frCoord width);
  void initFixedObjs_processRouting(frBlockObject* obj,
                                    frLayerNum layer_num,
                                    const odb::Rect& bounds,
                                    odb::Rect& box,
                                    frCoord width);
  void initFixedObjs_processVia(frBlockObject* obj,
                                frLayerNum layer_num,
                                const odb::Rect& bounds,
                                odb::Rect& box,
                                frCoord width,
                                frNet* net_ptr);
  void initFixedObjs_applyBorderViaCosts(
      frLayerNum layer_num,
      frCoord width,
      bool upper,
      const frRegionQuery::Objects<frBlockObject>& result);
  void initCosts();
  void sortIroutes();
  bool outOfDieVia(frLayerNum layer_num,
                   const odb::Point& pt,
                   const odb::Rect& die_box) const;

  // quick drc
  frSquaredDistance box2boxDistSquare(const odb::Rect& box1,
                                      const odb::Rect& box2,
                                      frCoord& dx,
                                      frCoord& dy);
  void addCost(taPinFig* fig, frOrderedIdSet<taPin*>* pinS = nullptr);
  void subCost(taPinFig* fig, frOrderedIdSet<taPin*>* pinS = nullptr);
  void modCost(taPinFig* fig,
               bool isAddCost,
               frOrderedIdSet<taPin*>* pinS = nullptr);
  void modMinSpacingCostPlanar(const odb::Rect& box,
                               frLayerNum layer_num,
                               taPinFig* fig,
                               bool isAddCost,
                               frOrderedIdSet<taPin*>* pinS = nullptr);
  void modMinSpacingCostVia(const odb::Rect& box,
                            frLayerNum layer_num,
                            taPinFig* fig,
                            bool isAddCost,
                            bool isUpperVia,
                            bool isCurrPs,
                            frOrderedIdSet<taPin*>* pinS = nullptr);
  bool getFollowTrackLayerNum(frLayerNum cut_layer_num,
                              frLayerNum& follow_track_layer_num) const;
  void getViaTrackRange(const odb::Rect& box,
                        const odb::Rect& via_box,
                        frCoord bloat_dist,
                        bool is_horizontal,
                        frLayerNum follow_track_layer_num,
                        int& idx_1,
                        int& idx_2) const;
  frCoord getViaParallelRunLength(const odb::Rect& box,
                                  const odb::Rect& via_box,
                                  frCoord dx,
                                  frCoord dy,
                                  bool is_horizontal,
                                  bool is_curr_ps) const;
  bool buildViaBlockBox(const odb::Rect& box,
                        const odb::Rect& via_box,
                        frCoord dx,
                        frCoord dy,
                        frCoord req_dist,
                        bool is_horizontal,
                        bool is_center_to_center,
                        frCoord track_loc,
                        const odb::Point& box_center,
                        odb::Rect& block_box) const;
  void updateViaCost(const odb::Rect& block_box,
                     frLayerNum layer_num,
                     taPinFig* fig,
                     frConstraint* con,
                     bool is_add_cost,
                     frOrderedIdSet<taPin*>* pin_set,
                     bool is_cut_cost);
  bool adjustParallelOverlapBlockBox(const odb::Rect& box,
                                     const odb::Rect& via_box,
                                     frCoord dx,
                                     frCoord dy,
                                     bool is_horizontal,
                                     frCoord track_loc,
                                     odb::Rect& block_box) const;
  bool isCutSpacingConstraintViolated(const frCutSpacingConstraint* con,
                                      const odb::Rect& box,
                                      const odb::Rect& transformed_via_box,
                                      frCoord dx,
                                      frCoord dy,
                                      bool is_horizontal,
                                      frCoord track_loc,
                                      const odb::Point& box_center,
                                      const odb::Rect& via_box,
                                      odb::Rect& block_box) const;
  void modCutSpacingCost(const odb::Rect& box,
                         frLayerNum layer_num,
                         taPinFig* fig,
                         bool isAddCost,
                         frOrderedIdSet<taPin*>* pinS = nullptr);

  // initTA
  void assign();
  void assignIroute(taPin* iroute);
  void assignIroute_init(taPin* iroute, frOrderedIdSet<taPin*>* pinS);
  void assignIroute_availTracks(taPin* iroute,
                                frLayerNum& layer_num,
                                int& idx1,
                                int& idx2);
  int assignIroute_bestTrack(taPin* iroute,
                             frLayerNum layer_num,
                             int idx1,
                             int idx2);
  int assignIroute_clampStartTrackIdx(frLayerNum layer_num,
                                      frCoord pin_coord,
                                      int idx_1,
                                      int idx_2) const;
  bool assignIroute_scanAscending(taPin* iroute,
                                  frLayerNum layer_num,
                                  int start_idx,
                                  int end_idx,
                                  frUInt4& best_cost,
                                  frCoord& best_track_loc,
                                  int& best_track_idx,
                                  frUInt4& drc_cost);
  bool assignIroute_scanDescending(taPin* iroute,
                                   frLayerNum layer_num,
                                   int start_idx,
                                   int end_idx,
                                   frUInt4& best_cost,
                                   frCoord& best_track_loc,
                                   int& best_track_idx,
                                   frUInt4& drc_cost);
  bool assignIroute_scanAlternating(taPin* iroute,
                                    frLayerNum layer_num,
                                    int start_idx,
                                    int idx_1,
                                    int idx_2,
                                    frUInt4& best_cost,
                                    frCoord& best_track_loc,
                                    int& best_track_idx,
                                    frUInt4& drc_cost);
  void assignIroute_bestTrack_helper(taPin* iroute,
                                     frLayerNum layer_num,
                                     int trackIdx,
                                     frUInt4& bestCost,
                                     frCoord& bestTrackLoc,
                                     int& bestTrackIdx,
                                     frUInt4& drcCost);
  frUInt4 assignIroute_getCost(taPin* iroute,
                               frCoord trackLoc,
                               frUInt4& drcCost);
  frUInt4 assignIroute_getNextIrouteDirCost(taPin* iroute, frCoord trackLoc);
  frUInt4 assignIroute_getPinCost(taPin* iroute, frCoord trackLoc);
  frUInt4 assignIroute_getAlignCost(taPin* iroute, frCoord trackLoc);
  frUInt4 assignIroute_getDRCCost(taPin* iroute, frCoord trackLoc);
  frUInt4 assignIroute_getDRCCost_helper(taPin* iroute,
                                         odb::Rect& box,
                                         frLayerNum layer_num);
  void assignIroute_updateIroute(taPin* iroute,
                                 frCoord bestTrackLoc,
                                 frOrderedIdSet<taPin*>* pinS);
  void assignIroute_updateOthers(frOrderedIdSet<taPin*>& pinS);

  // end
  void end();
  void saveToGuides();

  friend class FlexTA;
};

}  // namespace drt
