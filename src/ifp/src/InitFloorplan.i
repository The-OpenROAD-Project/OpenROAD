%module ifp

/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, James Cherry, Parallax Software, Inc.
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

%{

#include "db_sta/dbSta.hh"
#include "ifp/InitFloorplan.hh"

// Defined by OpenRoad.i
namespace ord {

OpenRoad *
getOpenRoad();

odb::dbDatabase *
getDb();

sta::dbSta *
getSta();
}

%}

////////////////////////////////////////////////////////////////
//
// C++ functions visible as TCL functions.
//
////////////////////////////////////////////////////////////////

%include "../../Exception.i"

%inline %{

namespace ifp {

void
init_floorplan_core(double die_lx,
		    double die_ly,
		    double die_ux,
		    double die_uy,
		    double core_lx,
		    double core_ly,
		    double core_ux,
		    double core_uy,
		    const char *site_name,
		    const char *tracks_file)
{
  odb::dbDatabase *db = ord::getDb();
  sta::dbSta *sta = ord::getSta();
  utl::Logger *logger = ord::getOpenRoad()->getLogger();
  ifp::initFloorplan(die_lx, die_ly, die_ux, die_uy,
		     core_lx, core_ly, core_ux, core_uy,
		     site_name, tracks_file,
		     db, logger);
}

void
init_floorplan_util(double util,
                    double aspect_ratio,
                    double core_space_bottom,
                    double core_space_top,
                    double core_space_left,
                    double core_space_right,
                    const char *site_name,
                    const char *tracks_file)
{
  odb::dbDatabase *db = ord::getDb();
  sta::dbSta *sta = ord::getSta();
  utl::Logger *logger = ord::getOpenRoad()->getLogger();
  ifp::initFloorplan(util, aspect_ratio,
                     core_space_bottom, core_space_top,
                     core_space_left, core_space_right,
                     site_name, tracks_file,
                     db, logger);
}

void
auto_place_pins_cmd(const char *pin_layer)
{
  odb::dbDatabase *db = ord::getDb();
  sta::dbSta *sta = ord::getSta();
  utl::Logger *logger = ord::getOpenRoad()->getLogger();
  ifp::autoPlacePins(pin_layer, db, logger);
}

} // namespace

%} // inline
