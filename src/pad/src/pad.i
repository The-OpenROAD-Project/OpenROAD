// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

////////////////////////////////////////////////////////////////
//
// C++ functions visible as TCL functions.
//
////////////////////////////////////////////////////////////////

%{
#include "pad/ICeWall.h"
#include "utl/Logger.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace ord {
// Defined in OpenRoad.i
pad::ICeWall* getICeWall();
utl::Logger* getLogger();
} // namespace ord
%}

%import <std_vector.i>
%import "dbtypes.i"
%import "dbenums.i"
%include "exception.i"
%include "../../Exception.i"

%typemap(in) pad::PlacementStrategy {
  char *str = Tcl_GetStringFromObj($input, 0);
  if (strcasecmp(str, "bump_aligned") == 0) {
    $1 = pad::PlacementStrategy::BUMP_ALIGNED;
  } else if (strcasecmp(str, "uniform") == 0) {
    $1 = pad::PlacementStrategy::UNIFORM;
  } else if (strcasecmp(str, "linear") == 0) {
    $1 = pad::PlacementStrategy::LINEAR;
  } else if (strcasecmp(str, "placer") == 0) {
    $1 = pad::PlacementStrategy::PLACER;
  } else if (strcasecmp(str, "default") == 0) {
    $1 = pad::PlacementStrategy::DEFAULT;
  } else {
    $1 = pad::PlacementStrategy::DEFAULT;
  }
}

%inline %{

namespace pad {

void make_bump_array(odb::dbMaster* master, int x, int y, int rows, int columns, int xpitch, int ypitch, const char* prefix = "BUMP_")
{
  ord::getICeWall()->makeBumpArray(master, odb::Point(x, y), rows, columns, xpitch, ypitch, prefix);
}

void remove_bump_array(odb::dbMaster* master)
{
  ord::getICeWall()->removeBumpArray(master);
}

void remove_bump(odb::dbInst* inst)
{
  ord::getICeWall()->removeBump(inst);
}

void assign_net_to_bump(odb::dbInst* inst, odb::dbNet* net, odb::dbITerm* terminal, bool dont_route)
{
  ord::getICeWall()->assignBump(inst, net, terminal, dont_route);
}

void make_fake_site(const char* name, int width, int height)
{
  ord::getICeWall()->makeFakeSite(name, width, height);
}

void make_io_row(odb::dbSite* hsite, odb::dbSite* vsite, odb::dbSite* csite,
                 int west_offset, int north_offset, int east_offset, int south_offset,
                 odb::dbOrientType rotation_hor, odb::dbOrientType rotation_ver, odb::dbOrientType rotation_cor,
                 int ring_index)
{
  ord::getICeWall()->makeIORow(hsite, vsite, csite,
                               west_offset, north_offset, east_offset, south_offset,
                               rotation_hor, rotation_ver, rotation_cor,
                               ring_index);
}

void remove_io_rows()
{
  ord::getICeWall()->removeIORows();
}

void place_pad(odb::dbMaster* master, const char* name, odb::dbRow* row, int location, bool mirror)
{
  ord::getICeWall()->placePad(master, name, row, location, mirror);
}

void place_pads(const std::vector<odb::dbInst*>& insts, odb::dbRow* row, pad::PlacementStrategy mode)
{
  ord::getICeWall()->placePads(insts, row, mode);
}

void place_corner(odb::dbMaster* master, int ring_index)
{
  ord::getICeWall()->placeCorner(master, ring_index);
}

void place_filler(const std::vector<odb::dbMaster*>& masters, odb::dbRow* row, const std::vector<odb::dbMaster*>& overlapping_permitted)
{
  ord::getICeWall()->placeFiller(masters, row, overlapping_permitted);
}

void remove_filler(odb::dbRow* row)
{
  ord::getICeWall()->removeFiller(row);
}

void place_bondpads(odb::dbMaster* master, const std::vector<odb::dbInst*>& pads, odb::dbOrientType rotation, int x_offset, int y_offset, const char* prefix = "IO_BOND_")
{
  ord::getICeWall()->placeBondPads(master, pads, rotation, {x_offset, y_offset}, prefix);
}

void place_terminals(const std::vector<odb::dbITerm*>& iterms,
                     const bool allow_non_top_layer)
{
  ord::getICeWall()->placeTerminals(iterms, allow_non_top_layer);
}

void connect_by_abutment()
{
  ord::getICeWall()->connectByAbutment();
}

void route_rdl(odb::dbTechLayer* layer, 
               odb::dbTechVia* bump_via,
               odb::dbTechVia* pad_via,
               const std::vector<odb::dbNet*>& nets,
               int width = 0, int spacing = 0, bool allow45 = false,
               float penalty = 2.0,
               int max_iterations = 10)
{
  ord::getICeWall()->routeRDL(layer, bump_via, pad_via, nets, width, spacing, allow45, penalty, max_iterations);
}

void route_rdl_gui(bool enable)
{
  ord::getICeWall()->routeRDLDebugGUI(enable);
}

void route_rdl_debug_net(const char* name)
{
  ord::getICeWall()->routeRDLDebugNet(name);
}

void route_rdl_debug_pin(const char* name)
{
  ord::getICeWall()->routeRDLDebugPin(name);
}

odb::dbRow* get_row(const char* name)
{
  odb::dbRow* row = ord::getICeWall()->findRow(name);
  if (row == nullptr) {
    ord::getLogger()->error(utl::PAD, 106, "Unable to find row: {}", name);
  }
  return row;
}

}

%} // inline
