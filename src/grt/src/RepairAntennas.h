/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/iterator/function_output_iterator.hpp>
#include <string>

#include "ant/AntennaChecker.hh"
#include "dpl/Opendp.h"
#include "grt/GRoute.h"
#include "grt/RoutePt.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/wOrder.h"
#include "sta/Liberty.hh"

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
  void setAntennaViolations(AntennaViolations antenna_violations)
  {
    antenna_violations_ = antenna_violations;
  }
  int getDiodesCount() { return diode_insts_.size(); }
  void clearViolations() { antenna_violations_.clear(); }
  void makeNetWires(NetRouteMap& routing,
                    const std::vector<odb::dbNet*>& nets_to_repair,
                    int max_routing_layer);
  void destroyNetWires(const std::vector<odb::dbNet*>& nets_to_repair);
  odb::dbMTerm* findDiodeMTerm();
  double diffArea(odb::dbMTerm* mterm);

 private:
  typedef int coord_type;
  typedef bg::cs::cartesian coord_sys_type;
  typedef bg::model::point<coord_type, 2, coord_sys_type> point;
  typedef bg::model::box<point> box;
  typedef std::pair<box, int> value;
  typedef bgi::rtree<value, bgi::quadratic<8, 4>> r_tree;

  void insertDiode(odb::dbNet* net,
                   odb::dbMTerm* diode_mterm,
                   odb::dbITerm* sink_iterm,
                   int site_width,
                   r_tree& fixed_insts,
                   odb::dbTechLayer* violation_layer);
  void getFixedInstances(r_tree& fixed_insts);
  void setInstsPlacementStatus(odb::dbPlacementStatus placement_status);
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
  odb::dbWire* makeNetWire(odb::dbNet* db_net,
                           GRoute& route,
                           std::map<int, odb::dbTechVia*>& default_vias);
  RoutePtPinsMap findRoutePtPins(Net* net);
  void addWireTerms(Net* net,
                    GRoute& route,
                    int grid_x,
                    int grid_y,
                    int layer,
                    odb::dbTechLayer* tech_layer,
                    RoutePtPinsMap& route_pt_pins,
                    odb::dbWireEncoder& wire_encoder,
                    std::map<int, odb::dbTechVia*>& default_vias,
                    bool connect_to_segment);
  void makeWire(odb::dbWireEncoder& wire_encoder,
                odb::dbTechLayer* layer,
                const odb::Point& start,
                const odb::Point& end);
  bool pinOverlapsGSegment(const odb::Point& pin_position,
                           const int pin_layer,
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
};

}  // namespace grt
