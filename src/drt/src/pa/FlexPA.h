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
  ViaRawPriorityTuple getViaRawPriority(frViaDef* via_def);
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

  /**
   * @brief initializes all access points of all unique instances
   */
  void initAllAccessPoints();
  void getViasFromMetalWidthMap(
      const Point& pt,
      frLayerNum layer_num,
      const gtl::polygon_90_set_data<frCoord>& polyset,
      std::vector<std::pair<int, frViaDef*>>& via_defs);

  /**
   * @brief fully initializes a pin's access points
   *
   * @param pin the pin (frBPin)
   * @param inst_term terminal related to the pin
   *
   * @return the number of access points generated
   */
  template <typename T>
  int initPinAccess(T* pin, frInstTerm* inst_term = nullptr);

  /**
   * @brief Contructs a vector with all pin figures in each layer
   *
   * @param pin pin object which will have figures merged by layer
   * @param inst_term instance terminal from which to get xform
   * @param is_shrink if polygons will be shrunk
   *
   * @return A vector of pin shapes in each layer
   */
  template <typename T>
  std::vector<gtl::polygon_90_set_data<frCoord>>
  mergePinShapes(T* pin, frInstTerm* inst_term, bool is_shrink = false);
  // type 0 -- on-grid; 1 -- half-grid; 2 -- center; 3 -- via-enc-opt
  /**
   * @brief Generates all necessary access points from all pin_shapes (pin)
   *
   * @param aps vector of access points that will be filled
   * @param apset set of access points data (auxilary)
   * @param pin pin object
   * @param inst_term instance terminal, owner of the access points
   * @param pin_shapes vector of pin shapes in every layer
   * @param lower_type lowest access cost considered
   * @param upper_type highest access cost considered
   */
  template <typename T>
  void genAPsFromPinShapes(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      std::set<std::pair<Point, frLayerNum>>& apset,
      T* pin,
      frInstTerm* inst_term,
      const std::vector<gtl::polygon_90_set_data<frCoord>>& pin_shapes,
      frAccessPointEnum lower_type,
      frAccessPointEnum upper_type);

  /**
   * @brief Generates all necessary access points from all layer_shapes (pin)
   *
   * @param aps vector of access points that will be filled
   * @param apset set of access points data (auxilary)
   * @param inst_term instance terminal, owner of the access points
   * @param layer_shapes pin shapes on that layer
   * @param layer_num layer in which the shapes exists
   * @param allow_via if via access is allowed
   * @param lower_type lowest access cost considered
   * @param upper_type highest access cost considered
   */
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

  /**
   * @brief Generates all necessary access points from a rectangle shape (pin
   * fig)
   *
   * @param aps vector of access points that will be filled
   * @param apset set of access points data (auxilary)
   * @param layer_num layer in which the rectangle exists
   * @param allow_planar if planar access is allowed
   * @param allow_via if via access is allowed
   * @param lower_type lowest access cost considered
   * @param upper_type highest access cost considered
   * @param is_macro_cell_pin TODO: not sure
   */
  void genAPsFromRect(std::vector<std::unique_ptr<frAccessPoint>>& aps,
                      std::set<std::pair<Point, frLayerNum>>& apset,
                      const gtl::rectangle_data<frCoord>& rect,
                      frLayerNum layer_num,
                      bool allow_planar,
                      bool allow_via,
                      frAccessPointEnum lower_type,
                      frAccessPointEnum upper_type,
                      bool is_macro_cell_pin);

  /**
   * @brief Generates an OnGrid access point (on or half track)
   *
   * @param coords map from access points to their cost
   * @param track_coords all possible track coords with cost
   * @param low lower range of coordinates considered
   * @param high higher range of coordinates considered
   * @param use_nearby_grid if the associated cost should be NearbyGrid or the
   * track cost
   */
  void genAPOnTrack(std::map<frCoord, frAccessPointEnum>& coords,
                    const std::map<frCoord, frAccessPointEnum>& track_coords,
                    frCoord low,
                    frCoord high,
                    bool use_nearby_grid = false);

  /**
   * @brief If there are less than 3 OnGrid coords between low and high
   * will generate a Centered access point to compensate
   *
   * @param coords map from candidate access points to their cost
   * @param layer_num number of the layer
   * @param low lower range of coordinates considered
   * @param high higher range of coordinates considered
   */
  void genAPCentered(std::map<frCoord, frAccessPointEnum>& coords,
                     frLayerNum layer_num,
                     frCoord low,
                     frCoord high);

  /**
   * @brief Generates an Enclosed Boundary access point
   *
   * @param coords map from access points to their cost
   * @param rect pin rectangle to which via is bounded
   * @param layer_num number of the layer
   */
  void genAPEnclosedBoundary(std::map<frCoord, frAccessPointEnum>& coords,
                             const gtl::rectangle_data<frCoord>& rect,
                             frLayerNum layer_num,
                             bool is_curr_layer_horz);

  void gen_initializeAccessPoints(
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

  /**
   * @brief Created an access point from x,y and layer num and adds it to aps
   * and apset. Also sets its accesses
   *
   * @param aps Vector containing the access points
   * @param apset Set containing access points data (auxilary)
   * @param maxrect Rect limiting where the point can be
   * @param x access point x coord
   * @param y access point y coord
   * @param layer_num access point layer
   * @param allow_planar if the access point allows planar access
   * @param allow_via if the access point allows via access
   * @param low_cost lowest access cost considered
   * @param high_cost highest access cost considered
   */
  void gen_createAccessPoint(std::vector<std::unique_ptr<frAccessPoint>>& aps,
                             std::set<std::pair<Point, frLayerNum>>& apset,
                             const gtl::rectangle_data<frCoord>& maxrect,
                             frCoord x,
                             frCoord y,
                             frLayerNum layer_num,
                             bool allow_planar,
                             bool allow_via,
                             frAccessPointEnum low_cost,
                             frAccessPointEnum high_cost);

  /**
   * @brief Sets the allowed accesses of the access points of a given pin.
   *
   * @param aps vector of access points of the pin
   * @param pin_shapes vector of pin shapes of the pin
   * @param pin the pin
   * @param inst_term terminal
   * @param is_std_cell_pin if the pin if from a standard cell
   */
  template <typename T>
  void check_setAPsAccesses(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      const std::vector<gtl::polygon_90_set_data<frCoord>>& pin_shapes,
      T* pin,
      frInstTerm* inst_term,
      const bool& is_std_cell_pin);

  /**
   * @brief Adds accesses to the access point
   *
   * @param ap access point
   * @param polyset polys auxilary set (same information as polys)
   * @param polys a vector of pin shapes on all layers of the current pin
   * @param pin access pin
   * @param inst_term terminal
   * @param deep_search TODO: not sure
   */
  template <typename T>
  void check_addAccess(frAccessPoint* ap,
                       const gtl::polygon_90_set_data<frCoord>& polyset,
                       const std::vector<gtl::polygon_90_data<frCoord>>& polys,
                       T* pin,
                       frInstTerm* inst_term,
                       bool deep_search = false);

  /**
   * @brief Tries to add a planar access to in the direction.
   *
   * @param ap access point
   * @param layer_polys vector of pin polygons on every layer
   * @param dir candidate dir to the access
   * @param pin access pin
   * @param inst_term terminal
   */
  template <typename T>
  void check_addPlanarAccess(
      frAccessPoint* ap,
      const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys,
      frDirEnum dir,
      T* pin,
      frInstTerm* inst_term);

  /**
   * @brief Determines coordinates of an End Point given a Begin Point.
   *
   * @param end_point the End Point to be filled
   * @param layer_polys a vector with all the pin polygons
   * @param begin_point the Begin Point
   * @param layer_num the number of the layer where begin_point is
   * @param dir the direction the End Point is from the Begin Point
   * @param is_block if the instance is a macro block.
   *
   * @return if any polygon on the layer contains the End Point
   */
  bool check_endPointIsOutside(
      Point& end_point,
      const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys,
      const Point& begin_point,
      frLayerNum layer_num,
      frDirEnum dir,
      bool is_block);

  template <typename T>
  void check_addViaAccess(
      frAccessPoint* ap,
      const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys,
      const gtl::polygon_90_set_data<frCoord>& polyset,
      frDirEnum dir,
      T* pin,
      frInstTerm* inst_term,
      bool deep_search = false);

  /**
   * @brief Checks if a Via Access Point is legal
   *
   * @param ap Access Point
   * @param via Via checked
   * @param pin Pin checked
   * @param inst_term Instance Terminal
   * @param layer_polys The Pin polygons in the pertinent layer
   *
   * @return If the Via Access Point is legal
   */
  template <typename T>
  bool checkViaAccess(
      frAccessPoint* ap,
      frVia* via,
      T* pin,
      frInstTerm* inst_term,
      const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys);

  /**
   * @brief Checks if a the Via Access can be subsequently accesses from the
   * given dir
   *
   * @param ap Access Point
   * @param via Via checked
   * @param pin Pin checked
   * @param inst_term Instance Terminal
   * @param layer_polys The Pin polygons in the access point layer
   * @param dir The dir that the via will be accessed
   *
   * @return If an access from that direction causes no DRV
   */
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

  /**
   * @brief initializes the accesses of a given pin but only considered
   * acccesses costed bounded between lower and upper cost.
   *
   * @param aps access points of the pin
   * @param apset data of the access points (auxilary)
   * @param pin_shapes shapes of the pin
   * @param pin the pin
   * @param inst_term terminal
   * @param lower_type lower bound cost
   * @param upper_type upper bound cost
   *
   * @return if the initialization was sucessful
   */

  /**
   * @brief initializes the accesses of a given pin but only considered
   * acccesses costed bounded between lower and upper cost.
   *
   * @param aps access points of the pin
   * @param apset data of the access points (auxilary)
   * @param pin_shapes shapes of the pin
   * @param pin the pin
   * @param inst_term terminal
   * @param lower_type lower bound cost
   * @param upper_type upper bound cost
   *
   * @return if the initialization was sucessful
   */
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

  int genPatterns_helper(
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      std::set<std::vector<int>>& inst_access_patterns,
      std::set<std::pair<int, int>>& used_access_points,
      std::set<std::pair<int, int>>& viol_access_points,
      int curr_unique_inst_idx,
      int max_access_point_size);

  void genPatternsInit(std::vector<FlexDPNode>& nodes,
                       const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
                       std::set<std::vector<int>>& inst_access_patterns,
                       std::set<std::pair<int, int>>& used_access_points,
                       std::set<std::pair<int, int>>& viol_access_points,
                       int max_access_point_size);

  void genPatterns_reset(
      std::vector<FlexDPNode>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      int max_access_point_size);

  void genPatterns_perform(
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

  bool genPatterns_commit(
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

  void genPatterns_print(
      std::vector<FlexDPNode>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      int max_access_point_size);

  int getFlatIdx(int idx_1, int idx_2, int idx_2_dim);

  void getNestedIdx(int flat_idx, int& idx_1, int& idx_2, int idx_2_dim);

  int getFlatEdgeIdx(int prev_idx_1,
                     int prev_idx_2,
                     int curr_idx_2,
                     int idx_2_dim);

  bool genPatterns_gc(
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

  void genInstRowPattern_commit(std::vector<FlexDPNode>& nodes,
                                const std::vector<frInst*>& insts);

  void genInstRowPattern_print(std::vector<FlexDPNode>& nodes,
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
