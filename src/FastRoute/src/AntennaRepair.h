/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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

#include "antennachecker/AntennaChecker.hh"
#include "fastroute/GRoute.h"
#include "opendb/db.h"
#include "opendb/dbBlockCallBackObj.h"
#include "opendb/dbShape.h"
#include "opendb/wOrder.h"
#include "dpl/Opendp.h"
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
} // namespace utl

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
  GlobalRouter* _grouter;
};

class AntennaRepair
{
 public:
  AntennaRepair(GlobalRouter* grouter,
                ant::AntennaChecker* arc,
                dpl::Opendp* opendp,
                odb::dbDatabase* db,
                utl::Logger *logger);

  int checkAntennaViolations(NetRouteMap& routing,
                             int maxRoutingLayer,
                             odb::dbMTerm* diodeMTerm);
  void fixAntennas(odb::dbMTerm* diodeMTerm);
  void legalizePlacedCells();
  AntennaViolations getAntennaViolations() { return _antennaViolations; }
  void setAntennaViolations(AntennaViolations antennaViolations)
  {
    _antennaViolations = antennaViolations;
  }
  int getDiodesCount() { return _diodeInsts.size(); }

 private:
  typedef int coord_type;
  typedef bg::cs::cartesian coord_sys_type;
  typedef bg::model::point<coord_type, 2, coord_sys_type> point;
  typedef bg::model::box<point> box;
  typedef std::pair<box, int> value;
  typedef bgi::rtree<value, bgi::quadratic<8, 4>> r_tree;

  void deleteFillerCells();
  void insertDiode(odb::dbNet* net,
                   odb::dbMTerm* diodeMTerm,
                   odb::dbInst* sinkInst,
                   odb::dbITerm* sinkITerm,
                   std::string antennaInstName,
                   int siteWidth,
                   r_tree& fixedInsts);
  void getFixedInstances(r_tree& fixedInsts);
  void setInstsPlacementStatus(odb::dbPlacementStatus placementStatus);

  GlobalRouter* _grouter;
  ant::AntennaChecker* _arc;
  dpl::Opendp* _opendp;
  odb::dbDatabase* _db;
  utl::Logger *_logger;
  odb::dbBlock* _block;
  std::vector<odb::dbInst*> _diodeInsts;
  AntennaViolations _antennaViolations;
};

}  // namespace grt
