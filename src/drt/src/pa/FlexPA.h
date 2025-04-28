// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <boost/polygon/polygon.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
// lowerArea, not align lower
using ViaRawPriorityTuple
    = std::tuple<bool, frCoord, frCoord, bool, frCoord, frCoord, bool>;

struct frInstLocationComp
{
  bool operator()(const frInst* lhs, const frInst* rhs) const
  {
    Point lp = lhs->getBoundaryBBox().ll(), rp = rhs->getBoundaryBBox().ll();
    if (lp.getY() != rp.getY()) {
      return lp.getY() < rp.getY();
    }
    return lp.getX() < rp.getX();
  }
};

using frInstLocationSet = std::set<frInst*, frInstLocationComp>;

class FlexPinAccessPattern;
class FlexDPNode;
class AbstractPAGraphics;

class FlexPA
{
 public:
  enum PatternType
  {
    Edge,
    Commit
  };

  FlexPA(frDesign* in,
         Logger* logger,
         dst::Distributed* dist,
         RouterConfiguration* router_cfg);
  ~FlexPA();

  void setDebug(std::unique_ptr<AbstractPAGraphics> pa_graphics);
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
  RouterConfiguration* router_cfg_;

  std::unique_ptr<AbstractPAGraphics> graphics_;
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
  std::unordered_map<frInst*,
                     std::vector<std::unique_ptr<FlexPinAccessPattern>>>
      unique_inst_patterns_;

  UniqueInsts unique_insts_;
  using UniqueMTerm = std::pair<const UniqueInsts::InstSet*, frMTerm*>;
  std::map<UniqueMTerm, bool> skip_unique_inst_term_;

  // helper structures
  std::vector<std::map<frCoord, frAccessPointEnum>> track_coords_;
  std::map<frLayerNum,
           std::map<int, std::map<ViaRawPriorityTuple, const frViaDef*>>>
      layer_num_to_via_defs_;
  frCollection<odb::dbInst*> target_insts_;
  frInstLocationSet insts_set_;

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
  ViaRawPriorityTuple getViaRawPriority(const frViaDef* via_def);
  bool isSkipInstTermLocal(frInstTerm* in);
  bool isSkipInstTerm(frInstTerm* in);
  bool isDistributed() const { return !remote_host_.empty(); }

  // init
  void init();
  void initTrackCoords();
  void initViaRawPriority();
  void initAllSkipInstTerm();
  void initSkipInstTerm(frInst* unique_inst);
  // prep
  void prep();

  bool isStdCell(frInst* unique_inst);
  bool isMacroCell(frInst* unique_inst);

  void deleteInst(frInst* inst);

  /**
   * @brief generates all access points of a single unique instance
   *
   * @param unique_inst the unique instance
   */
  void genInstAccessPoints(frInst* unique_inst);

  /**
   * @brief generates all access points of all unique instances
   */
  void genAllAccessPoints();

  /**
   * @brief fully generates a pin's access points
   *
   * @param pin the pin (frBPin)
   * @param inst_term terminal related to the pin
   *
   * @return the number of access points generated
   */
  template <typename T>
  int genPinAccess(T* pin, frInstTerm* inst_term = nullptr);

  /**
   * @brief determines if the current access points are enough to say PA is done
   * with this pin.
   *
   * for the access points to be considered enough there must exist a minimum of
   * aps:
   * 1. far enough from each other greater than the minimum specified in
   * router_cfg.
   * 2. far enough from the cell edge.
   *
   * @param aps the list of candidate access points
   * @param inst_term terminal related to the pin
   *
   * @returns True if the current aps are enough for the pin
   */
  bool EnoughAccessPoints(std::vector<std::unique_ptr<frAccessPoint>>& aps,
                          frInstTerm* inst_term);

  /**
   * @brief initializes the pin accesses of a given pin only considering a given
   * cost for both the lower and upper layer.
   *
   * @param aps access points of the pin
   * @param apset data of the access points (auxilary)
   * @param pin_shapes shapes of the pin
   * @param pin the pin
   * @param inst_term terminal
   * @param lower_type lower layer access type
   * @param upper_type upper layer access type
   *
   * @return if enough access points were found for the pin.
   */
  template <typename T>
  bool genPinAccessCostBounded(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      std::set<std::pair<Point, frLayerNum>>& apset,
      std::vector<gtl::polygon_90_set_data<frCoord>>& pin_shapes,
      T* pin,
      frInstTerm* inst_term,
      frAccessPointEnum lower_type,
      frAccessPointEnum upper_type);

  void getViasFromMetalWidthMap(
      const Point& pt,
      frLayerNum layer_num,
      const gtl::polygon_90_set_data<frCoord>& polyset,
      std::vector<std::pair<int, const frViaDef*>>& via_defs);

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
   * @param lower_type lowest access type considered
   * @param upper_type highest access type considered
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
   * @param lower_type lowest access type considered
   * @param upper_type highest access type considered
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

  /**
   * @brief Generates all necessary access points from a rectangle shape (pin
   * fig)
   *
   * @param aps vector of access points that will be filled
   * @param apset set of access points data (auxilary)
   * @param layer_num layer in which the rectangle exists
   * @param allow_planar if planar access is allowed
   * @param allow_via if via access is allowed
   * @param lower_type lowest access type considered
   * @param upper_type highest access type considered
   * @param is_macro_cell_pin if the pin belongs to a macro
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

  void genViaEnclosedCoords(std::map<frCoord, frAccessPointEnum>& coords,
                            const gtl::rectangle_data<frCoord>& rect,
                            const frViaDef* via_def,
                            frLayerNum layer_num,
                            bool is_curr_layer_horz);

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

  /**
   * @brief Calls the other genAP functions according to the informed cost
   *
   * @param cost access point cost
   * @param coords access points cost map (will get at least one new entry)
   * @param track_coords coordinates of tracks on the layer
   * @param base_layer_num if two layers are being considered this is the lower,
   * if only one is being considered this is the layer
   * @param layer_num number of the current layer
   * @param rect rectangle representing pin shape
   * @param is_curr_layer_horz if the current layer is horizontal
   * @param offset TODO: not sure, something to do with macro cells
   */
  void genAPCosted(frAccessPointEnum cost,
                   std::map<frCoord, frAccessPointEnum>& coords,
                   const std::map<frCoord, frAccessPointEnum>& track_coords,
                   frLayerNum base_layer_num,
                   frLayerNum layer_num,
                   const gtl::rectangle_data<frCoord>& rect,
                   bool is_curr_layer_horz,
                   int offset = 0);

  /**
   * @brief Creates multiple access points from the coordinates
   *
   * @param aps Vector contaning the access points
   * @param apset Set containing access points data (auxilary)
   * @param rec Rect limiting where the point can be
   * @param layer_num access point layer
   * @param allow_planar if the access point allows planar access
   * @param allow_via if the access point allows via access
   * @param x_coords map of access point x coords
   * @param y_coords map of access point y coords
   * @param lower_type access cost of the lower layer
   * @param upper_type access cost of the upper layer
   */
  void createMultipleAccessPoints(
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
   * @brief Creates an access point object from x,y and layer num and adds it to
   * aps and apset. Also sets its initial accesses which will be filtered later
   *
   * @param aps Vector containing the access points
   * @param apset Set containing access points data (auxilary)
   * @param maxrect Rect limiting where the point can be
   * @param x access point x coord
   * @param y access point y coord
   * @param layer_num access point layer
   * @param allow_planar if the access point allows planar access
   * @param allow_via if the access point allows via access
   * @param lower_type lowest access cost considered
   * @param upper_type highest access cost considered
   */
  void createSingleAccessPoint(std::vector<std::unique_ptr<frAccessPoint>>& aps,
                               std::set<std::pair<Point, frLayerNum>>& apset,
                               const gtl::rectangle_data<frCoord>& maxrect,
                               frCoord x,
                               frCoord y,
                               frLayerNum layer_num,
                               bool allow_planar,
                               bool allow_via,
                               frAccessPointEnum lower_type,
                               frAccessPointEnum upper_type);

  /**
   * @brief Filters the accesses of all access points
   *
   * @details Receives every access point with their default
   * accesses to every direction. It will check if any access set as true is
   * valid, e.g. not cause DRVs. If it finds it to be invalid it will set that
   * access as false. If all accesses of an access point are found to be false
   * it will be deleted/disconsidered by the function that calls this.
   *
   * @param aps vector of access points of the pin
   * @param pin_shapes vector of pin shapes of the pin
   * @param pin the pin
   * @param inst_term terminal
   * @param is_std_cell_pin if the pin if from a standard cell
   */
  template <typename T>
  void filterMultipleAPAccesses(
      std::vector<std::unique_ptr<frAccessPoint>>& aps,
      const std::vector<gtl::polygon_90_set_data<frCoord>>& pin_shapes,
      T* pin,
      frInstTerm* inst_term,
      const bool& is_std_cell_pin);

  /**
   * @brief Filters the accesses of a single access point
   *
   * @param ap access point
   * @param polyset polys auxilary set (same information as polys)
   * @param polys a vector of pin shapes on all layers of the current pin
   * @param pin access pin
   * @param inst_term terminal
   * @param deep_search TODO: not sure
   */
  template <typename T>
  void filterSingleAPAccesses(
      frAccessPoint* ap,
      const gtl::polygon_90_set_data<frCoord>& polyset,
      const std::vector<gtl::polygon_90_data<frCoord>>& polys,
      T* pin,
      frInstTerm* inst_term,
      bool deep_search = false);

  /**
   * @brief Filters access in a given planar direction.
   *
   * @param ap access point
   * @param layer_polys vector of pin polygons on every layer
   * @param dir candidate dir to the access
   * @param pin access pin
   * @param inst_term terminal
   */
  template <typename T>
  void filterPlanarAccess(
      frAccessPoint* ap,
      const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys,
      frDirEnum dir,
      T* pin,
      frInstTerm* inst_term);

  /**
   * @brief Determines if an access on the given direction would cause a DRV.
   *
   * @param ap access point
   * @param pin access pin
   * @param ps virtual path segment that would need to exist for this access
   * @param point access point coordinates
   * @param layer access layer
   */
  template <typename T>
  bool isPlanarViolationFree(frAccessPoint* ap,
                             T* pin,
                             frPathSeg* ps,
                             frInstTerm* inst_term,
                             Point point,
                             frLayer* layer);

  /**
   * @brief Generates an end_point given an begin_point in the direction
   *
   * @param layer_polys Pin Polygons on the layer (used for a check)
   * TODO: maybe the check can be moves to isPointOusideShapes, but not sure
   * @param begin_point The begin reference point
   * @param layer_num layer where the point is being created
   * @param dir direction where the point will be created
   * @param is_block wether the begin_point is from a macro block
   *
   * @returns the generated end point
   */
  Point genEndPoint(
      const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys,
      const Point& begin_point,
      frLayerNum layer_num,
      frDirEnum dir,
      bool is_block);

  /**
   * @brief Checks if a point is outside the layer_polygons
   *
   * @return if the point is outside the pin shapes
   */
  bool isPointOutsideShapes(
      const Point& point,
      const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys);

  /**
   * @brief Filters access through via on the access point
   *
   * @details Besides checking if the via can even exist, this will also check
   * later if a planar access can be done on upper layer to reach the via.
   * Access through only 1 of the cardinal directions is enough.
   *
   * @param ap access point
   * @param layer_polys Pin Polygons on the layer (used for a check)
   * @param polyset polys auxilary set (same information as polys)
   * @param pin access pin
   * @param inst_term instance terminal
   * @param deep_search TODO: I understand one of its uses but not why "deep
   * search"
   */
  template <typename T>
  void filterViaAccess(
      frAccessPoint* ap,
      const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys,
      const gtl::polygon_90_set_data<frCoord>& polyset,
      T* pin,
      frInstTerm* inst_term,
      bool deep_search = false);

  /**
   * @brief Checks if a Via has at least one valid planar access
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
  bool checkViaPlanarAccess(
      frAccessPoint* ap,
      frVia* via,
      T* pin,
      frInstTerm* inst_term,
      const std::vector<gtl::polygon_90_data<frCoord>>& layer_polys);

  /**
   * @brief Checks if a the Via Access can be accessed from a given dir
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

  /**
   * @brief Determines if a Via would cause a DRV.
   */
  template <typename T>
  bool isViaViolationFree(frAccessPoint* ap,
                          frVia* via,
                          T* pin,
                          frPathSeg* ps,
                          frInstTerm* inst_term,
                          Point point);

  /**
   * @brief Serially updates some of general pin stats
   */
  template <typename T>
  void updatePinStats(
      const std::vector<std::unique_ptr<frAccessPoint>>& tmp_aps,
      T* pin,
      frInstTerm* inst_term);

  /**
   * @brief Adjusts the coordinates for all access points
   *
   * @details access points are created with their coordinates relative to the
   * chip. They have to have their coordinates altered to be relative to their
   * instances, including rotation.
   */
  void revertAccessPoints();

  void prepPattern();

  /**
   * @brief generates valid access patterns for the unique inst, considers both
   * x and y of prepPatternInstHelper.
   *
   * @param unique_inst unique inst
   */
  void prepPatternInst(frInst* unique_inst);

  /**
   * @brief generates valid access patterns for the unique inst
   *
   * @param unique_inst unique inst
   * @param use_x whether the x or y average coordinate of the access points of
   * a pin will be used for sorting it.
   *
   * @returns the number of access patterns found.
   */
  int prepPatternInstHelper(frInst* unique_inst, bool use_x);

  int genPatterns(frInst* unique_inst,
                  const std::vector<std::pair<frMPin*, frInstTerm*>>& pins);

  int genPatternsHelper(
      frInst* unique_inst,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      std::set<std::vector<int>>& inst_access_patterns,
      std::set<std::pair<int, int>>& used_access_points,
      std::set<std::pair<int, int>>& viol_access_points,
      int max_access_point_size);

  /**
   * @brief Initializes the nodes' data structures that will be used to solve
   * the DP problem
   */
  void genPatternsInit(
      std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      std::set<std::vector<int>>& inst_access_patterns,
      std::set<std::pair<int, int>>& used_access_points,
      std::set<std::pair<int, int>>& viol_access_points);

  /**
   * @brief Resets the nodes' data structures that will be used to solve the
   * DP problem
   */
  void genPatternsReset(
      std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins);

  /**
   * @brief Determines the value of all the paths of the DP problem
   */
  void genPatternsPerform(
      frInst* unique_inst,
      std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      std::vector<int>& vio_edges,
      const std::set<std::pair<int, int>>& used_access_points,
      const std::set<std::pair<int, int>>& viol_access_points,
      int max_access_point_size);

  /**
   * @brief Determines the edge cost between two DP nodes
   */
  int getEdgeCost(frInst* unique_inst,
                  FlexDPNode* prev_node,
                  FlexDPNode* curr_node,
                  const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
                  std::vector<int>& vio_edges,
                  const std::set<std::pair<int, int>>& used_access_points,
                  const std::set<std::pair<int, int>>& viol_access_points,
                  int max_access_point_size);

  /**
   * @brief Extracts the access patterns given the graph nodes composing the
   * access points relationship
   *
   * @param inst instance
   * @param nodes {pin,access_point} nodes of the access pattern graph
   * @param pins vector of pins of the unique instance
   * @param used_access_points a set of all used access points
   *
   * @returns a vector of ints representing the access pattern in the form:
   * access_pattern[pin_idx] = access_point_idx of the pin
   */
  std::vector<int> extractAccessPatternFromNodes(
      frInst* inst,
      const std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      std::set<std::pair<int, int>>& used_access_points);

  /**
   * @brief Commits to the best path (solution) on the DP graph
   */
  bool genPatternsCommit(
      frInst* unique_inst,
      const std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins,
      bool& is_valid,
      std::set<std::vector<int>>& inst_access_patterns,
      std::set<std::pair<int, int>>& used_access_points,
      std::set<std::pair<int, int>>& viol_access_points,
      int max_access_point_size);

  /**
   * @brief Auxilary function for debugging
   */
  void genPatternsPrintDebug(
      std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins);

  /**
   * @brief Auxilary function for debugging
   */
  void genPatternsPrint(
      std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
      const std::vector<std::pair<frMPin*, frInstTerm*>>& pins);

  /**
   * @brief Converts a nested indexing system of edges to a flat one
   */
  int getFlatEdgeIdx(int prev_idx_1,
                     int prev_idx_2,
                     int curr_idx_2,
                     int idx_2_dim);

  /**
   * @brief Checks for any DRVs.
   *
   * @returns false if any DRVs.
   */
  bool genPatternsGC(
      const std::set<frBlockObject*>& target_objs,
      const std::vector<std::pair<frConnFig*, frBlockObject*>>& objs,
      PatternType pattern_type,
      std::set<frBlockObject*>* owners = nullptr);

  /**
   * @brief populates the insts_set_ data structure
   */
  void buildInstsSet();

  /**
   * @brief organizes all the insts in a vector of clusters, each cluster being
   * a vector of insts a adjacent insts
   *
   * @returns the vector of vectors of insts
   */
  std::vector<std::vector<frInst*>> computeInstRows();

  /**
   * @brief Verifies if both instances are abuting
   *
   * @returns true if the instances abute
   */
  bool instancesAreAbuting(frInst* inst_1, frInst* inst_2) const;

  /**
   * @brief Find a cluster of instances that are touching the passed instance
   *
   * @returns a vector of the clusters of touching insts
   */
  std::vector<frInst*> getAdjacentInstancesCluster(frInst* inst) const;

  void prepPatternInstRows(std::vector<std::vector<frInst*>> inst_rows);

  /**
   * @brief determines the access patterns for all the instances on a row
   *
   * @details uses the DP graph approach described in TAO of PAO paper to
   * determine the access patters.
   *
   * @param insts instances on the row
   */
  void genInstRowPattern(std::vector<frInst*>& insts);

  /**
   * @brief initializes the nodes data strucutes that will be used to solve the
   * DP problem
   *
   * @param nodes the empy nodes data structure
   * @param insts instances on the row
   */
  void genInstRowPatternInit(
      std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
      const std::vector<frInst*>& insts);

  /**
   * @brief Determines the value of all the paths of the DP problem
   *
   * @param nodes the nodes data structure
   * @param insts instances on the row
   */
  void genInstRowPatternPerform(
      std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
      const std::vector<frInst*>& insts);

  /**
   * @brief Commits to the best path (solution) on the DP graph
   *
   * @param nodes the nodes data structure
   * @param insts instances on the row
   */
  void genInstRowPatternCommit(
      std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
      const std::vector<frInst*>& insts);

  /**
   * @brief Auxilary function for debugging
   *
   * @param nodes the nodes data structure
   * @param insts instances on the row
   */
  void genInstRowPatternPrint(
      std::vector<std::vector<std::unique_ptr<FlexDPNode>>>& nodes,
      const std::vector<frInst*>& insts);

  /**
   * @brief Determines the edge cost between two nodes
   */
  int getEdgeCost(FlexDPNode* prev_node,
                  FlexDPNode* curr_node,
                  const std::vector<frInst*>& insts);

  /**
   * @brief Auxilary to determine DRVs.
   *
   * @param inst unique instance
   * @param access_pattern access pattern
   * @param objs TODO: not sure why this is a parameter at all
   * @param vias TODO: ditto
   * @param isPrev whether this is the previous or current node
   */
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
  FlexDPNode* getPrevNode() const { return prev_node_; }
  std::pair<int, int> getIdx() const { return idx_; }
  bool isSource() const { return virtual_source_; }
  bool isSink() const { return virtual_sink_; }
  bool isVirtual() const { return (virtual_source_ || virtual_sink_); }

  // setters
  void setPathCost(int in) { pathCost_ = in; }
  void setNodeCost(int in) { nodeCost_ = in; }
  void setPrevNode(FlexDPNode* in) { prev_node_ = in; }
  void setIdx(std::pair<int, int> in) { idx_ = std::move(in); }
  void setAsSource() { virtual_source_ = true; }
  void setAsSink() { virtual_sink_ = true; }

  bool hasPrevNode() const { return prev_node_ != nullptr; }

 private:
  bool virtual_source_ = false;
  bool virtual_sink_ = false;
  int pathCost_ = std::numeric_limits<int>::max();
  int nodeCost_ = std::numeric_limits<int>::max();
  /*either {pin_idx, acc_point_idx} or {inst_idx, acc_pattern_idx} depending on
   * context*/
  std::pair<int, int> idx_ = {-1, -1};
  FlexDPNode* prev_node_ = nullptr;
};
}  // namespace drt
