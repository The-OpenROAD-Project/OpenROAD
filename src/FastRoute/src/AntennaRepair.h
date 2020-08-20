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

#include "FastRoute.h"
#include "antennachecker/AntennaChecker.hh"
#include "opendb/db.h"
#include "opendb/dbShape.h"
#include "opendb/wOrder.h"
#include "opendp/Opendp.h"

// Forward declaration protects FastRoute code from any
// header file from the DB. FastRoute code keeps independent.
namespace odb {
class dbDatabase;
class dbChip;
class dbTech;
}  // namespace odb

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

namespace FastRoute {

class GlobalRouter;

class AntennaRepair
{
 public:
 	AntennaRepair(GlobalRouter *fr,
 								 antenna_checker::AntennaChecker* arc,
 							   opendp::Opendp* opendp, odb::dbDatabase* db);

	int checkAntennaViolations(const std::vector<FastRoute::NET>* routing,
                             int maxRoutingLayer);
  void fixAntennas(std::string antennaCellName, std::string antennaPinName);
  void legalizePlacedCells();
  std::map<odb::dbNet*, std::vector<VINFO>> getAntennaViolations()
                                            { return _antennaViolations; }
  void setAntennaViolations(std::map<odb::dbNet*, std::vector<VINFO>>
                            antennaViolations)
                           { _antennaViolations = antennaViolations; }

  private:
  typedef int coord_type;
  typedef bg::cs::cartesian coord_sys_type;
  typedef bg::model::point<coord_type, 2, coord_sys_type> point;
  typedef bg::model::box<point> box;
  typedef std::pair<box, int> value;
  typedef bgi::rtree<value, bgi::quadratic<8, 4>> r_tree;

  void insertDiode(odb::dbNet* net,
                   std::string antennaCellName,
                   std::string antennaPinName,
                   odb::dbInst* sinkInst,
                   odb::dbITerm* sinkITerm,
                   std::string antennaInstName,
                   int siteWidth,
                   r_tree& fixedInsts);
  void getFixedInstances(r_tree& fixedInsts);

  GlobalRouter *_fr;
  antenna_checker::AntennaChecker* _arc;
  opendp::Opendp* _opendp;
 	odb::dbDatabase* _db;
 	odb::dbBlock* _block;
  std::map<odb::dbNet*, std::vector<VINFO>>
    _antennaViolations;
};

}  // namespace FastRoute