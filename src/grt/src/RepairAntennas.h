// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ant/AntennaChecker.hh"
#include "boost/geometry/geometry.hpp"
#include "boost/geometry/index/rtree.hpp"
#include "boost/iterator/function_output_iterator.hpp"
#include "dpl/Opendp.h"
#include "grt/GRoute.h"
#include "grt/RoutePt.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/wOrder.h"

// Forward declaration protects FastRoute code from any
// header file from the DB. FastRoute code keeps independent.
namespace odb {
class dbDatabase;
class dbChip;
class dbTech;
class dbWireEncoder;
}  // namespace odb

namespace utl {
class Logger;
}  // namespace utl

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

namespace grt {

class GlobalRouter;
class Net;
class Pin;

using AntennaViolations = std::map<odb::dbNet*, std::vector<ant::Violation>>;

struct RoutePtPins
{
  std::vector<Pin*> pins;
  bool connected;
};

using RoutePtPinsMap = std::map<RoutePt, RoutePtPins>;

struct SegmentNode
{
  int node_id = -1;
  int seg_id;
  odb::Rect rect;
  std::vector<std::pair<odb::dbTechLayer*, int>> adjs;
  SegmentNode(int node_id_, int seg_id_, odb::Rect rect_)
  {
    node_id = node_id_;
    seg_id = seg_id_;
    rect = rect_;
  }
  SegmentNode() = default;
};

using LayerToSegmentNodeVector
    = std::unordered_map<odb::dbTechLayer*, std::vector<SegmentNode>>;
using SegmentNodeIds
    = std::unordered_map<odb::dbTechLayer*, std::unordered_set<int>>;
using SegmentToJumperPos = std::map<int, std::set<int>>;

enum class RoutingSource
{
  DetailedRouting,
  GlobalRouting,
  None
};

class RepairAntennas
{
 public:
  RepairAntennas(GlobalRouter* grouter,
                 ant::AntennaChecker* arc,
                 dpl::Opendp* opendp,
                 odb::dbDatabase* db,
                 utl::Logger* logger);

  bool checkAntennaViolations(NetRouteMap& routing,
                              const std::vector<odb::dbNet*>& nets_to_repair,
                              int max_routing_layer,
                              odb::dbMTerm* diode_mterm,
                              float ratio_margin,
                              int num_threads);
  void checkNetViolations(odb::dbNet* db_net,
                          odb::dbMTerm* diode_mterm,
                          float ratio_margin);
  void repairAntennas(odb::dbMTerm* diode_mterm);
  int illegalDiodePlacementCount() const
  {
    return illegal_diode_placement_count_;
  }
  void legalizePlacedCells();
  AntennaViolations getAntennaViolations() { return antenna_violations_; }
  int getDiodesCount() { return diode_insts_.size(); }
  void clearViolations() { antenna_violations_.clear(); }
  void destroyNetWires(const std::vector<odb::dbNet*>& nets_to_repair);
  odb::dbMTerm* findDiodeMTerm();
  double diffArea(odb::dbMTerm* mterm);
  bool hasNewViolations() { return has_new_violations_; }
  void jumperInsertion(NetRouteMap& routing,
                       const int& tile_size,
                       const int& max_routing_layer,
                       std::vector<odb::dbNet*>& modified_nets);

 private:
  using coord_type = int;
  using coord_sys_type = bg::cs::cartesian;
  using point = bg::model::point<coord_type, 2, coord_sys_type>;
  using box = bg::model::box<point>;
  using value = std::pair<box, int>;
  using r_tree = bgi::rtree<value, bgi::quadratic<8, 4>>;

  void addJumperAndVias(GRoute& route,
                        const int& init_x,
                        const int& init_y,
                        const int& final_x,
                        const int& final_y,
                        const int& layer_level,
                        odb::dbNet* db_net);
  void addJumperToRoute(GRoute& route,
                        const int& seg_id,
                        const int& jumper_init_pos,
                        const int& jumper_final_pos,
                        const int& layer_level,
                        odb::dbNet* db_net);
  void findSegments(const GRoute& route,
                    odb::dbITerm* iterm,
                    const SegmentNodeIds& segment_ids,
                    LayerToSegmentNodeVector& segment_graph,
                    const int& num_nodes,
                    const int& violation_layer,
                    SegmentToJumperPos& segments_to_repair,
                    odb::dbNet* db_net);
  bool findPosToJumper(const GRoute& route,
                       LayerToSegmentNodeVector& segment_graph,
                       const SegmentNode& seg_node,
                       const odb::Point& parent_pos,
                       int& jumper_position,
                       odb::dbNet* db_net);
  int getJumperPosition(const int& init_pos,
                        const int& final_pos,
                        const int& target_pos);
  void getSegmentsConnectedToPin(const odb::dbITerm* iterm,
                                 LayerToSegmentNodeVector& segment_by_layer,
                                 SegmentNodeIds& seg_connected_to_pin);
  int buildSegmentGraph(const GRoute& route,
                        const int& max_layer,
                        LayerToSegmentNodeVector& graph);
  void setAdjacentSegments(LayerToSegmentNodeVector& segment_by_layer);
  int getSegmentsPerLayer(const GRoute& route,
                          const int& max_layer,
                          LayerToSegmentNodeVector& segment_by_layer);
  void addJumper(GRoute& route,
                 const int& segment_id,
                 const int& jumper_pos,
                 odb::dbNet* db_net);
  void findJumperCandidatePositions(const int& init_x,
                                    const int& init_y,
                                    const int& final_x,
                                    const int& final_y,
                                    const odb::Point& parent_pos,
                                    const bool& is_horizontal,
                                    std::vector<int>& candidate_positions);
  int getBestPosition(const std::vector<int>& candidate_positions,
                      const bool& is_horizontal,
                      const odb::Point& parent_pos);
  void getViolations(const std::vector<ant::Violation>& violations,
                     const int& max_routing_layer,
                     std::vector<int>& violation_id_to_repair,
                     int& max_layer_to_repair);
  int addJumperOnSegments(const SegmentToJumperPos& segments_to_repair,
                          GRoute& route,
                          odb::dbNet* db_net);
  void insertDiode(odb::dbNet* net,
                   odb::dbMTerm* diode_mterm,
                   odb::dbITerm* sink_iterm,
                   int site_width,
                   r_tree& fixed_insts,
                   odb::dbTechLayer* violation_layer);
  void getFixedInstances(r_tree& fixed_insts, int& fixed_inst_count);
  void getPlacementBlockages(r_tree& fixed_insts, int& fixed_inst_count);
  void setDiodesAndGatesPlacementStatus(
      odb::dbPlacementStatus placement_status);
  void setInstsPlacementStatus(std::vector<odb::dbInst*>& insts_to_restore);
  bool setDiodeLoc(odb::dbInst* diode_inst,
                   odb::dbITerm* gate,
                   int site_width,
                   bool place_vertically,
                   r_tree& fixed_insts);
  void getInstancePlacementData(odb::dbITerm* gate,
                                int& inst_loc_x,
                                int& inst_loc_y,
                                int& inst_width,
                                int& inst_height,
                                odb::dbOrientType& inst_orient);
  bool checkDiodeLoc(odb::dbInst* diode_inst,
                     int site_width,
                     r_tree& fixed_insts);
  void computeHorizontalOffset(int diode_width,
                               int inst_width,
                               int site_width,
                               int& left_offset,
                               int& right_offset,
                               bool& place_at_left,
                               int& offset);
  void computeVerticalOffset(int inst_height,
                             int& top_offset,
                             int& bottom_offset,
                             bool& place_at_top,
                             int& offset);
  odb::Rect getInstRect(odb::dbInst* inst, odb::dbITerm* iterm);
  bool diodeInRow(odb::Rect diode_rect);
  odb::dbOrientType getRowOrient(const odb::Point& point);
  RoutePtPinsMap findRoutePtPins(Net* net);
  bool pinOverlapsGSegment(const odb::Point& pin_position,
                           int pin_layer,
                           const std::vector<odb::Rect>& pin_boxes,
                           const GRoute& route);

  GlobalRouter* grouter_;
  ant::AntennaChecker* arc_;
  dpl::Opendp* opendp_;
  odb::dbDatabase* db_;
  utl::Logger* logger_;
  odb::dbBlock* block_;
  std::vector<odb::dbInst*> diode_insts_;
  AntennaViolations antenna_violations_;
  int unique_diode_index_;
  int illegal_diode_placement_count_;
  bool has_new_violations_;
  RoutingSource routing_source_;
  //////////////////////////////////////////
  int tile_size_;
  int jumper_size_;
  int smaller_seg_size_;
  //////////////////////////////////////////
};

}  // namespace grt
