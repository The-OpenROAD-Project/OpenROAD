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
           const int halo_x,
           const int halo_y,
           const int row_min_width,
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
           const int dist,
           const bool disallow_one_site_gaps)
  {
    Options options;
    options.endcap_master = endcap_master;
    options.tapcell_master = tapcell_master;
    options.dist = dist;
    options.halo_x = halo_x;
    options.halo_y = halo_y;
    options.row_min_width = row_min_width;
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
    options.disallow_one_site_gaps = disallow_one_site_gaps;
    getTapcell()->run(options);
  }

  void cut_rows(odb::dbMaster* endcap_master, int halo_x, int halo_y, int row_min_width)
  {
    Options options;
    options.endcap_master = endcap_master;
    options.halo_x = halo_x;
    options.halo_y = halo_y;
    options.row_min_width = row_min_width;
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

  void place_endcaps(
    odb::dbMaster* left_top_corner,
    odb::dbMaster* right_top_corner,
    odb::dbMaster* left_bottom_corner,
    odb::dbMaster* right_bottom_corner,
    odb::dbMaster* left_top_edge,
    odb::dbMaster* right_top_edge,
    odb::dbMaster* left_bottom_edge,
    odb::dbMaster* right_bottom_edge,

    const std::vector<odb::dbMaster*>& top_edge,
    const std::vector<odb::dbMaster*>& bottom_edge,

    odb::dbMaster* left_edge,
    odb::dbMaster* right_edge,
    const char* prefix)
  {
    EndcapCellOptions options;

    options.left_top_corner = left_top_corner;
    options.right_top_corner = right_top_corner;
    options.left_bottom_corner = left_bottom_corner;
    options.right_bottom_corner = right_bottom_corner;
    options.left_top_edge = left_top_edge;
    options.right_top_edge = right_top_edge;
    options.left_bottom_edge = left_bottom_edge;
    options.right_bottom_edge = right_bottom_edge;

    options.top_edge = top_edge;
    options.bottom_edge = bottom_edge;

    options.left_edge = left_edge;
    options.right_edge = right_edge;
    options.prefix = prefix;

    getTapcell()->placeEndcaps(options);
  }

  void insert_tapcells(
    odb::dbMaster* master,
    int distance)
  {
    Options options;
    options.dist = distance;
    options.tapcell_master = master;
  
    getTapcell()->placeTapcells(options);
  }

  }  // namespace tap

%}
