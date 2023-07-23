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

%{
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"
#include "tap/tapcell.h"

  namespace ord {

  tap::Tapcell* getTapcell();

  }

  using ord::getTapcell;
  using std::set;
  using std::string;
  using std::vector;

  static odb::dbMaster* findMaster(const char* name)
  {
    if (name[0] == '\0') {
      return nullptr;
    }
    auto db = ord::OpenRoad::openRoad()->getDb();
    auto master = db->findMaster(name);
    if (!master) {
      auto logger = ord::OpenRoad::openRoad()->getLogger();
      logger->error(utl::TAP, 35, "Master {} not found.", name);
    }
    return master;
  }
  
%}

%import <std_vector.i>
%import "dbtypes.i"

%include "../../Exception.i"

%inline %{
  namespace tap {

  void set_tap_prefix(const char* tap_prefix) {
    getTapcell()->setTapPrefix(tap_prefix);
  }

  void set_endcap_prefix(const char* endcap_prefix){
    getTapcell()->setEndcapPrefix(endcap_prefix);
  }

  void run(odb::dbMaster* endcap_master,
           int halo_x,
           int halo_y,
           const char* cnrcap_nwin_master,
           const char* cnrcap_nwout_master,
           const char* tap_nwintie_master,
           const char* tap_nwin2_master,
           const char* tap_nwin3_master,
           const char* tap_nwouttie_master,
           const char* tap_nwout2_master,
           const char* tap_nwout3_master,
           const char* incnrcap_nwin_master,
           const char* incnrcap_nwout_master,
           odb::dbMaster* tapcell_master,
           int dist)
  {
    Options options;
    options.endcap_master = endcap_master;
    options.tapcell_master = tapcell_master;
    options.dist = dist;
    options.halo_x = halo_x;
    options.halo_y = halo_y;
    options.cnrcap_nwin_master = findMaster(cnrcap_nwin_master);
    options.cnrcap_nwout_master = findMaster(cnrcap_nwout_master);
    options.tap_nwintie_master = findMaster(tap_nwintie_master);
    options.tap_nwin2_master = findMaster(tap_nwin2_master);
    options.tap_nwin3_master = findMaster(tap_nwin3_master);
    options.tap_nwouttie_master = findMaster(tap_nwouttie_master);
    options.tap_nwout2_master = findMaster(tap_nwout2_master);
    options.tap_nwout3_master = findMaster(tap_nwout3_master);
    options.incnrcap_nwin_master = findMaster(incnrcap_nwin_master);
    options.incnrcap_nwout_master = findMaster(incnrcap_nwout_master);
    getTapcell()->run(options);
  }

  void cut_rows(odb::dbMaster* endcap_master, int halo_x, int halo_y)
  {
    Options options;
    options.endcap_master = endcap_master;
    options.halo_x = halo_x;
    options.halo_y = halo_y;
    getTapcell()->cutRows(options);
  }

  void clear()
  {
    getTapcell()->clear();
  }

  void reset() { getTapcell()->reset(); }

  int remove_cells(const char* prefix)
  {
    return getTapcell()->removeCells(prefix);
  }

  void insert_boundary_cells(
    odb::dbMaster* outer_corner_top_left_r0,
    odb::dbMaster* outer_corner_top_left_mx,
    odb::dbMaster* outer_corner_top_right_r0,
    odb::dbMaster* outer_corner_top_right_mx,
    odb::dbMaster* outer_corner_bottom_left_r0,
    odb::dbMaster* outer_corner_bottom_left_mx,
    odb::dbMaster* outer_corner_bottom_right_r0,
    odb::dbMaster* outer_corner_bottom_right_mx,
    odb::dbMaster* inner_corner_top_left_r0,
    odb::dbMaster* inner_corner_top_left_mx,
    odb::dbMaster* inner_corner_top_right_r0,
    odb::dbMaster* inner_corner_top_right_mx,
    odb::dbMaster* inner_corner_bottom_left_r0,
    odb::dbMaster* inner_corner_bottom_left_mx,
    odb::dbMaster* inner_corner_bottom_right_r0,
    odb::dbMaster* inner_corner_bottom_right_mx,

    const std::vector<odb::dbMaster*>& top_r0,
    const std::vector<odb::dbMaster*>& top_mx,
    const std::vector<odb::dbMaster*>& bottom_r0,
    const std::vector<odb::dbMaster*>& bottom_mx,

    odb::dbMaster* left_r0,
    odb::dbMaster* left_mx,
    odb::dbMaster* right_r0,
    odb::dbMaster* right_mx,
    const char* prefix)
  {
    BoundaryCellOptions options;

    options.outer_corner_top_left_r0 = outer_corner_top_left_r0;
    options.outer_corner_top_left_mx = outer_corner_top_left_mx;
    options.outer_corner_top_right_r0 = outer_corner_top_right_r0;
    options.outer_corner_top_right_mx = outer_corner_top_right_mx;
    options.outer_corner_bottom_left_r0 = outer_corner_bottom_left_r0;
    options.outer_corner_bottom_left_mx = outer_corner_bottom_left_mx;
    options.outer_corner_bottom_right_r0 = outer_corner_bottom_right_r0;
    options.outer_corner_bottom_right_mx = outer_corner_bottom_right_mx;
    options.inner_corner_top_left_r0 = inner_corner_top_left_r0;
    options.inner_corner_top_left_mx = inner_corner_top_left_mx;
    options.inner_corner_top_right_r0 = inner_corner_top_right_r0;
    options.inner_corner_top_right_mx = inner_corner_top_right_mx;
    options.inner_corner_bottom_left_r0 = inner_corner_bottom_left_r0;
    options.inner_corner_bottom_left_mx = inner_corner_bottom_left_mx;
    options.inner_corner_bottom_right_r0 = inner_corner_bottom_right_r0;
    options.inner_corner_bottom_right_mx = inner_corner_bottom_right_mx;

    options.top_r0 = top_r0;
    options.top_mx = top_mx;
    options.bottom_r0 = bottom_r0;
    options.bottom_mx = bottom_mx;

    options.left_r0 = left_r0;
    options.left_mx = left_mx;
    options.right_r0 = right_r0;
    options.right_mx = right_mx;
    options.prefix = prefix;

    getTapcell()->insertBoundaryCells(options);
  }

  void insert_tapcells(
    odb::dbMaster* master,
    int distance)
  {
    Options options;
    options.dist = distance;
    options.tapcell_master = master;
  
    getTapcell()->insertTapcells(options);
  }

  }  // namespace tap

%}
