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

#include <boost/polygon/polygon.hpp>
#include <cstdint>

#include "FlexPA_unique.h"
#include "frDesign.h"
namespace gtl = boost::polygon;

namespace odb {
class dbDatabase;
}

namespace dst {
class Distributed;
}

namespace boost::serialization {
class access;
}

namespace drt {
// not default via, upperWidth, lowerWidth, not align upper, upperArea,
// lowerArea, not align lower, via name
using ViaRawPriorityTuple
    = std::tuple<bool, frCoord, frCoord, bool, frCoord, frCoord, bool>;

class FlexPinAccessPattern;
class FlexDPNode;
class FlexPAGraphics;

class FlexPA
{
 public:
  enum PatternType
  {
    Edge,
    Commit
  };

  FlexPA(frDesign* in, Logger* logger, dst::Distributed* dist);
  ~FlexPA();

  void setDebug(frDebugSettings* settings, odb::dbDatabase* db);
  void setTargetInstances(const frCollection<odb::dbInst*>& insts);
  void setDistributed(const std::string& rhost,
                      uint16_t rport,
                      const std::string& shared_vol,
                      int cloud_sz);

  int main();

 private:
  frDesign* design_;
  Logger* logger_;
  dst::Distributed* dist_;

  std::unique_ptr<FlexPAGraphics> graphics_;
  std::string debugPinName_;

  int std_cell_pin_gen_ap_cnt_ = 0;
  int std_cell_pin_valid_planar_ap_cnt_ = 0;
  int std_cell_pin_valid_via_ap_cnt_ = 0;
  int std_cell_pin_no_ap_cnt_ = 0;
  int inst_term_valid_via_ap_cnt_ = 0;
  int macro_cell_pin_gen_ap_cnt_ = 0;
  int macro_cell_pin_valid_planar_ap_cnt_ = 0;
  int macro_cell_pin_valid_via_ap_cnt_ = 0;
  int macro_cell_pin_no_ap_cnt_ = 0;
  std::vector<std::vector<std::unique_ptr<FlexPinAccessPattern>>>
      unique_inst_patterns_;

  UniqueInsts unique_insts_;
  using UniqueMTerm = std::pair<const UniqueInsts::InstSet*, frMTerm*>;
  std::map<UniqueMTerm, bool> skip_unique_inst_term_;

  // helper structures
  std::vector<std::map<frCoord, frAccessPointEnum>> track_coords_;
  std::map<frLayerNum, std::map<int, std::map<ViaRawPriorityTuple, frViaDef*>>>
      layer_num_to_via_defs_;
  frCollection<odb::dbInst*> target_insts_;

  std::string remote_host_;
  uint16_t remote_port_ = -1;
  std::string shared_vol_;
  int cloud_sz_ = -1;

  // helper functions
  frDesign* getDesign() const { return design_; }
  frTechObject* getTech() const { return design_->getTech(); }
  void setDesign(frDesign* in)
  {
    design_ = in;
    unique_insts_.setDesign(in);
  }
  void applyPatternsFile(const char* file_path);
  void getViaRawPriority(frViaDef* via_def, ViaRawPriorityTuple& priority);
  bool isSkipInstTermLocal(frInstTerm* in);
  bool isSkipInstTerm(frInstTerm* in);
  bool isDistributed() const { return !remote_host_.empty(); }

  // init
  void init();
  void initTrackCoords();
  void initViaRawPriority();
  void initSkipInstTerm();
  // prep
  void prep();
  void initAllAccessPoints();
  void getViasFromMetalWidthMap(
      const Point& pt,
      frLayerNum layer_num,
      const gtl::polygon_90_set_data<frCoord>& polyset,
      std::vector<std::pair<int, frViaDef*>>& via_defs);
  template <typename T>
  int initPinAccess(T* pin, frInstTerm* inst_term = nullptr);
  template <typename T>
  void mergePinShapes(
      std::vector<gtl::polygon_90_set_data<frCoord>>& pin_shapes,
      T* pin,
      frInstTerm* inst_term,
      bool is_shrink = false);
  // type 0 -- on-grid; 1 -- half-grid; 2 -- center; 3 -- via-enc-opt
  template <typename T>
  void getAPsFromPinShapes(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      std::set<std::pair<Point, frLayerNum>>& apset,
      T* pin,
      frInstTerm* inst_term,
      const std::vector<gtl::polygon_90_set_data<frCoord>>& pin_shapes,
      frAccessPointEnum lower_type,
      frAccessPointEnum upper_type);
  void genAPsFromLayerShapes(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      std::set<std::pair<Point, frLayerNum>>& apset,
      frInstTerm* inst_term,
      const gtl::polygon_90_set_data<frCoord>& layer_shapes,
      frLayerNum layer_num,
      bool allow_via,
      frAccessPointEnum lower_type,
      frAccessPointEnum upper_type);
  bool enclosesOnTrackPlanarAccess(const gtl::rectangle_data<frCoord>& rect,
                                   frLayerNum layer_num);
  void genAPsFromRect(std::vector<std::unique_ptr<frAccessPoint>>& aps,
                      std::set<std::pair<Point, frLayerNum>>& apset,
                      const gtl::rectangle_data<frCoord>& rect,
                      frLayerNum layer_num,
                      bool allow_planar,
                      bool allow_via,
                      frAccessPointEnum lower_type,
                      frAccessPointEnum upper_type,
                      bool is_macro_cell_pin);
  void genAPOnTrack(std::map<frCoord, frAccessPointEnum>& coords,
                    const std::map<frCoord, frAccessPointEnum>& track_coords,
                    frCoord low,
                    frCoord high,
                    bool use_nearby_grid = false);
  void genAPCentered(std::map<frCoord, frAccessPointEnum>& coords,
                     frLayerNum layer_num,
                     frCoord low,
                     frCoord high);
  void genAPEnclosedBoundary(std::map<frCoord, frAccessPointEnum>& coords,
                             const gtl::rectangle_data<frCoord>& rect,
                             frLayerNum layer_num,
                             bool is_curr_layer_horz);
  void initializeAccessPoints(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      std::set<std::pair<Point, frLayerNum>>& apset,
      const gtl::rectangle_data<frCoord>& rect,
      frLayerNum layer_num,
      bool allow_planar,
      bool allow_via,
      bool is_layer1_horz,
      const std::map<frCoord, frAccessPointEnum>& x_coords,
      const std::map<frCoord, frAccessPointEnum>& y_coords,
      frAccessPointEnum lower_type,
      frAccessPointEnum upper_type);
  void createAccessPoint(std::vector<std::unique_ptr<frAccessPoint>>& aps,
                         std::set<std::pair<Point, frLayerNum>>& apset,
                         const gtl::rectangle_data<frCoord>& maxrect,
                         frCoord x,
                         frCoord y,
                         frLayerNum layer_num,
                         bool allow_planar,
                         bool allow_via,
                         frAccessPointEnum low_cost,
                         frAccessPointEnum high_cost);
  template <typename T>
  void setAPsAccesses(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      const std::vector<gtl::polygon_90_set_data<frCoord>>& pin_shapes,
      T* pin,
      frInstTerm* inst_term,
      const bool& is_std_cell_pin);
  template <typename T>
  void addAccess(frAccessPoint* ap,
                 const gtl::polygon_90_set_data<frCoord>& polyset,
                 const std::vector<gtl::polygon_90_data<frCoord>>& polys,
                 T* pin,
                 frInstTerm* inst_term,
                 bool deep_search = false);
  template <typename T>
  void addPlanarAccess(
      frAccessPoint* ap,
      const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys,
      frDirEnum dir,
      T* pin,
      frInstTerm* inst_term);
  bool endPointIsOutside(
      Point& end_point,
      const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys,
      const Point& begin_point,
      frLayerNum layer_num,
      frDirEnum dir,
      bool is_block);
  template <typename T>
  void addViaAccess(
      frAccessPoint* ap,
      const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys,
      const gtl::polygon_90_set_data<frCoord>& polyset,
      frDirEnum dir,
      T* pin,
      frInstTerm* inst_term,
      bool deep_search = false);
  template <typename T>
  bool checkViaAccess(
      frAccessPoint* ap,
      frVia* via,
      T* pin,
      frInstTerm* inst_term,
      const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys);
  template <typename T>
  bool checkDirectionalViaAccess(
      frAccessPoint* ap,
      frVia* via,
      T* pin,
      frInstTerm* inst_term,
      const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys,
      frDirEnum dir);
  template <typename T>
  void updatePinStats(
      const std::vector<std::unique_ptr<frAccessPoint>>& tmp_aps,
      T* pin,
      frInstTerm* inst_term);
  template <typename T>
  bool initPinAccessCostBounded(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      std::set<std::pair<Point, frLayerNum>>& apset,
      std::vector<gtl::polygon_90_set_data<frCoord>>& pin_shapes,
      T* pin,
      frInstTerm* inst_term,
      frAccessPointEnum lower_type,
      frAccessPointEnum upper_type);

  void prepPattern();
  void prepPatternInstRows(std::vector<std::vector<frInst*>> inst_rows);
  int prepPatternInst(frInst* inst, int curr_unique_inst_idx, double x_weight);
  int genPatterns(const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
                  int curr_unique_inst_idx);
  void genPatternsInit(std::vector<FlexDPNode>& nodes,
                       const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
                       std::set<std::vector<int>>& inst_access_patterns,
                       std::set<std::pair<int, int>>& used_access_points,
                       std::set<std::pair<int, int>>& viol_access_points,
                       int max_access_point_size);
  void genPatternsReset(
      std::vector<FlexDPNode>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      int max_access_point_size);
  void genPatternsPerform(
      std::vector<FlexDPNode>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      std::vector<int>& vio_edges,
      const std::set<std::pair<int, int>>& used_access_points,
      const std::set<std::pair<int, int>>& viol_access_points,
      int curr_unique_inst_idx,
      int max_access_point_size);
  int getEdgeCost(int prev_node_idx,
                  int curr_node_idx,
                  const std::vector<FlexDPNode>& nodes,
                  const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
                  std::vector<int>& vio_edges,
                  const std::set<std::pair<int, int>>& used_access_points,
                  const std::set<std::pair<int, int>>& viol_access_points,
                  int curr_unique_inst_idx,
                  int max_access_point_size);
  bool genPatternsCommit(
      const std::vector<FlexDPNode>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      bool& is_valid,
      std::set<std::vector<int>>& inst_access_patterns,
      std::set<std::pair<int, int>>& used_access_points,
      std::set<std::pair<int, int>>& viol_access_points,
      int curr_unique_inst_idx,
      int max_access_point_size);
  void genPatternsPrintDebug(
      std::vector<FlexDPNode>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      int max_access_point_size);
  void genPatternsPrint(
      std::vector<FlexDPNode>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      int max_access_point_size);
  int getFlatIdx(int idx_1, int idx_2, int idx_2_dim);
  void getNestedIdx(int flat_idx, int& idx_1, int& idx_2, int idx_2_dim);
  int getFlatEdgeIdx(int prev_idx_1,
                     int prev_idx_2,
                     int curr_idx_2,
                     int idx_2_dim);

  bool genPatternsGC(
      const std::set<frBlockObject*>& target_objs,
      const std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
      PatternType pattern_type,
      std::set<frBlockObject*>* owners = nullptr);

  void getInsts(std::vector<frInst*>& insts);
  void genInstRowPattern(std::vector<frInst*>& insts);
  void genInstRowPatternInit(std::vector<FlexDPNode>& nodes,
                             const std::vector<frInst*>& insts);
  void genInstRowPatternPerform(std::vector<FlexDPNode>& nodes,
                                const std::vector<frInst*>& insts);
  void genInstRowPatternCommit(std::vector<FlexDPNode>& nodes,
                               const std::vector<frInst*>& insts);
  void genInstRowPatternPrint(std::vector<FlexDPNode>& nodes,
                              const std::vector<frInst*>& insts);
  int getEdgeCost(int prev_node_idx,
                  int curr_node_idx,
                  const std::vector<FlexDPNode>& nodes,
                  const std::vector<frInst*>& insts);
  void revertAccessPoints();
  void addAccessPatternObj(
      frInst* inst,
      FlexPinAccessPattern* access_pattern,
      std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
      std::vector<std::unique_ptr<frVia>>& vias,
      bool isPrev);

  friend class RoutingCallBack;
};

class FlexPinAccessPattern
{
 public:
  // getter
  const std::vector<frAccessPoint*>& getPattern() const { return pattern_; }
  frAccessPoint* getBoundaryAP(bool isLeft) const
  {
    return isLeft ? left_ : right_;
  }
  int getCost() const { return cost_; }
  // setter
  void addAccessPoint(frAccessPoint* in) { pattern_.push_back(in); }
  void setBoundaryAP(bool isLeft, frAccessPoint* in)
  {
    if (isLeft) {
      left_ = in;
    } else {
      right_ = in;
    }
  }
  void updateCost()
  {
    cost_ = 0;
    for (auto& ap : pattern_) {
      if (ap) {
        cost_ += ap->getCost();
      }
    }
  }

 private:
  std::vector<frAccessPoint*> pattern_;
  frAccessPoint* left_ = nullptr;
  frAccessPoint* right_ = nullptr;
  int cost_ = std::numeric_limits<int>::max();
  template <class Archive>
  void serialize(Archive& ar, unsigned int version);
  friend class boost::serialization::access;
};

// dynamic programming related
class FlexDPNode
{
 public:
  // getters
  int getPathCost() const { return pathCost_; }
  int getNodeCost() const { return nodeCost_; }
  int getPrevNodeIdx() const { return prev_node_idx_; }

  // setters
  void setPathCost(int in) { pathCost_ = in; }
  void setNodeCost(int in) { nodeCost_ = in; }
  void setPrevNodeIdx(int in) { prev_node_idx_ = in; }

 private:
  int pathCost_ = std::numeric_limits<int>::max();
  int nodeCost_ = std::numeric_limits<int>::max();
  int prev_node_idx_ = -1;
};
}  // namespace drt
