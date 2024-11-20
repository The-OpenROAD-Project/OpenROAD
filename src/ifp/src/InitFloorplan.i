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
#include "ord/OpenRoad.hh"
#include "ord/Design.h"
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

static utl::Logger* getLogger() {
  return ord::OpenRoad::openRoad()->getLogger();
}
 
%}

////////////////////////////////////////////////////////////////
//
// C++ functions visible as TCL functions.
//
////////////////////////////////////////////////////////////////

%import <std_string.i>
%import <std_vector.i>
%import "dbtypes.i"
%include "../../Exception.i"
%include "../../Design.i"

%typemap(in) ifp::RowParity {
  char *str = Tcl_GetStringFromObj($input, 0);
  if (strcasecmp(str, "NONE") == 0) {
    $1 = ifp::RowParity::NONE;
  } else if (strcasecmp(str, "EVEN") == 0) {
    $1 = ifp::RowParity::EVEN;
  } else if (strcasecmp(str, "ODD") == 0) {
    $1 = ifp::RowParity::ODD;
  } else {
    $1 = ifp::RowParity::NONE;
  }
}

%inline %{

namespace ifp {

void
init_floorplan_core(ord::Design* design,
                    int die_lx,
                    int die_ly,
                    int die_ux,
                    int die_uy,
                    int core_lx,
                    int core_ly,
                    int core_ux,
                    int core_uy,
                    odb::dbSite* site,
                    const std::vector<odb::dbSite*>& additional_sites,
                    ifp::RowParity row_parity,
                    const std::vector<odb::dbSite*>& flipped_sites)
{
  std::set<odb::dbSite*> flipped_sites_set(flipped_sites.begin(),
                                           flipped_sites.end());
  design->getFloorplan().initFloorplan({die_lx, die_ly, die_ux, die_uy},
                                {core_lx, core_ly, core_ux, core_uy},
                                site, additional_sites, row_parity, flipped_sites_set); 
}

void
init_floorplan_util(ord::Design* design,
                    double util,
                    double aspect_ratio,
                    int core_space_bottom,
                    int core_space_top,
                    int core_space_left,
                    int core_space_right,
                    odb::dbSite* site,
                    const std::vector<odb::dbSite*>& additional_sites,
                    ifp::RowParity row_parity,
                    const std::vector<odb::dbSite*>& flipped_sites)
{
  std::set<odb::dbSite*> flipped_sites_set(flipped_sites.begin(),
                                           flipped_sites.end());
  design->getFloorplan().initFloorplan(util, aspect_ratio,
                                core_space_bottom, core_space_top,
                                core_space_left, core_space_right,
                                site, additional_sites, row_parity,
                                flipped_sites_set);
}

void
insert_tiecells_cmd(ord::Design* design,
                    odb::dbMTerm* tie_term, const char* prefix)
{
  design->getFloorplan().insertTiecells(tie_term, prefix);
}

void
make_layer_tracks(ord::Design* design)
{
  design->getFloorplan().makeTracks();
}

void
make_layer_tracks(ord::Design* design,
                  odb::dbTechLayer* layer,
                  int x_offset,
                  int x_pitch,
                  int y_offset,
                  int y_pitch)
{
  design->getFloorplan().makeTracks(layer, x_offset, x_pitch,
                                     y_offset, y_pitch);
}

odb::dbSite* find_site(ord::Design* design,
                       const char* site_name)
{
  auto site = design->getFloorplan().findSite(site_name);
  if (!site) {
    getLogger()->error(utl::IFP, 18, "Unable to find site: {}", site_name);
  }
  return site;
}

} // namespace

%} // inline
