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
#include "utl/Logger.h"

// Defined by OpenRoad.i
namespace ord {

OpenRoad *
getOpenRoad();

odb::dbDatabase *
getDb();

sta::dbSta *
getSta();
}

static ifp::InitFloorplan get_floorplan()
{
  auto app = ord::getOpenRoad();
  auto chip = app->getDb()->getChip();
  auto logger = app->getLogger();
  if (!chip || !chip->getBlock()) {
    logger->error(utl::IFP, 38, "No design is loaded.");
  }
  auto block = chip->getBlock();
  auto network = app->getDbNetwork();
  return ifp::InitFloorplan(block, logger, network);
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
init_floorplan_core(int die_lx,
		    int die_ly,
		    int die_ux,
		    int die_uy,
		    int core_lx,
		    int core_ly,
		    int core_ux,
		    int core_uy,
		    const char *site_name)
{
  get_floorplan().initFloorplan({die_lx, die_ly, die_ux, die_uy},
                                {core_lx, core_ly, core_ux, core_uy},
                                site_name);
}

void
init_floorplan_util(double util,
                    double aspect_ratio,
                    int core_space_bottom,
                     int core_space_top,
                    int core_space_left,
                    int core_space_right,
                    const char *site_name)
{
  get_floorplan().initFloorplan(util, aspect_ratio,
                                core_space_bottom, core_space_top,
                                core_space_left, core_space_right,
                                site_name);
}

void
insert_tiecells_cmd(odb::dbMTerm* tie_term, const char* prefix)
{
  get_floorplan().insertTiecells(tie_term, prefix);
}

void
make_layer_tracks()
{
  get_floorplan().makeTracks();
}

void
make_layer_tracks(odb::dbTechLayer* layer,
                  int x_offset,
                  int x_pitch,
                  int y_offset,
                  int y_pitch)
{
  get_floorplan().makeTracks(layer, x_offset, x_pitch, y_offset, y_pitch);
}

} // namespace

%} // inline
