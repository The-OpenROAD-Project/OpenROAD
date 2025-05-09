// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

%{

#include "db_sta/dbSta.hh"
#include "ifp/InitFloorplan.hh"
#include "ord/OpenRoad.hh"
#include "ord/Design.h"
#include "utl/Logger.h"
#include "odb/geom.h"

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

void make_die(ord::Design* design,
                    int die_lx,
                    int die_ly,
                    int die_ux,
                    int die_uy)
{
  design->getFloorplan().makeDie({die_lx, die_ly, die_ux, die_uy});
}

void make_die_util(ord::Design* design,
                  double util,
                  double aspect_ratio,
                  int core_space_bottom,
                  int core_space_top,
                  int core_space_left,
                  int core_space_right)
{
  design->getFloorplan().makeDieUtilization(util, 
                                            aspect_ratio, 
                                            core_space_bottom,
                                            core_space_top,
                                            core_space_left,
                                            core_space_right);
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
make_rows_with_spacing(ord::Design* design, 
          int spacing_lx,
          int spacing_ly,
          int spacing_ux,
          int spacing_uy,
          odb::dbSite* site,
          const std::vector<odb::dbSite*>& additional_sites,
          ifp::RowParity row_parity,
          const std::vector<odb::dbSite*>& flipped_sites)
{
  std::set<odb::dbSite*> flipped_sites_set(flipped_sites.begin(),
                                           flipped_sites.end());
  design->getFloorplan().makeRowsWithSpacing(spacing_lx, spacing_ly, 
                                             spacing_ux, spacing_uy,
                                             site,
                                             additional_sites,
                                             row_parity,
                                             flipped_sites_set);
}

void
make_rows(ord::Design* design, 
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
  design->getFloorplan().makeRows({core_lx, core_ly, core_ux, core_uy},
                                  site,
                                  additional_sites,
                                  row_parity,
                                  flipped_sites_set);
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
