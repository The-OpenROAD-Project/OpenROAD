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

#include <boost/function_output_iterator.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <string>

#include "ant/AntennaChecker.hh"
#include "dpl/Opendp.h"
#include "grt/GRoute.h"
#include "opendb/db.h"
#include "opendb/dbBlockCallBackObj.h"
#include "opendb/dbShape.h"
#include "opendb/wOrder.h"
#include "sta/Liberty.hh"

// Forward declaration protects FastRoute code from any
// header file from the DB. FastRoute code keeps independent.
namespace odb {
class dbDatabase;
class dbChip;
class dbTech;
}  // namespace odb

namespace utl {
class Logger;
}  // namespace utl

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

namespace grt {

typedef std::map<odb::dbNet*, std::vector<ant::VINFO>> AntennaViolations;

class GlobalRouter;

class AntennaCbk : public odb::dbBlockCallBackObj
{
 public:
  AntennaCbk(GlobalRouter* grouter);
  virtual void inDbPostMoveInst(odb::dbInst*);

 private:
  GlobalRouter* grouter_;
};

class AntennaRepair
{
 public:
  AntennaRepair(GlobalRouter* grouter,
                ant::AntennaChecker* arc,
                dpl::Opendp* opendp,
                odb::dbDatabase* db,
                utl::Logger* logger);

  int checkAntennaViolations(NetRouteMap& routing,
                             int max_routing_layer,
                             odb::dbMTerm* diode_mterm);
  void repairAntennas(odb::dbMTerm* diode_mterm);
  void legalizePlacedCells();
  AntennaViolations getAntennaViolations() { return antenna_violations_; }
  void setAntennaViolations(AntennaViolations antenna_violations)
  {
    antenna_violations_ = antenna_violations;
  }
  int getDiodesCount() { return diode_insts_.size(); }
  void clearViolations() { antenna_violations_.clear(); }

 private:
  typedef int coord_type;
  typedef bg::cs::cartesian coord_sys_type;
  typedef bg::model::point<coord_type, 2, coord_sys_type> point;
  typedef bg::model::box<point> box;
  typedef std::pair<box, int> value;
  typedef bgi::rtree<value, bgi::quadratic<8, 4>> r_tree;

  void deleteFillerCells();
  void insertDiode(odb::dbNet* net,
                   odb::dbMTerm* diode_mterm,
                   odb::dbInst* sink_inst,
                   odb::dbITerm* sink_iterm,
                   std::string antenna_inst_name,
                   int site_width,
                   r_tree& fixed_insts);
  void getFixedInstances(r_tree& fixed_insts);
  void setInstsPlacementStatus(odb::dbPlacementStatus placement_status);
  odb::Rect getInstRect(odb::dbInst* inst, odb::dbITerm* iterm);

  GlobalRouter* grouter_;
  ant::AntennaChecker* arc_;
  dpl::Opendp* opendp_;
  odb::dbDatabase* db_;
  utl::Logger* logger_;
  odb::dbBlock* block_;
  std::vector<odb::dbInst*> diode_insts_;
  AntennaViolations antenna_violations_;
};

}  // namespace grt
