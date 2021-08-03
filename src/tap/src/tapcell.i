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

%{

#include "tap/tapcell.h"
#include "ord/OpenRoad.hh"
#include "opendb/db.h"

namespace ord {

tap::Tapcell* getTapcell();
// {
//   return ord::OpenRoad::openRoad()->getTapcell();
// }

}

using ord::getTapcell;
using std::vector;
using std::set;
using std::string;

%}


%include "../../Exception.i"

%inline %{

namespace tap {

void run(odb::dbMaster* endcap_master, int halo_x, int halo_y, const char* cnrcap_nwin_master, const char* cnrcap_nwout_master, const char* endcap_prefix, int add_boundary_cell, const char* tap_nwintie_master, const char* tap_nwin2_master, const char* tap_nwin3_master, const char* tap_nwouttie_master, const char* tap_nwout2_master, const char* tap_nwout3_master, const char* incnrcap_nwin_master, const char* incnrcap_nwout_master, const char* tapcell_master, int dist, const char* tap_prefix)
{
  getTapcell()->run(endcap_master, halo_x, halo_y, cnrcap_nwin_master, cnrcap_nwout_master, endcap_prefix, add_boundary_cell, tap_nwintie_master, tap_nwin2_master, tap_nwin3_master, tap_nwouttie_master, tap_nwout2_master, tap_nwout3_master, incnrcap_nwin_master, incnrcap_nwout_master, tapcell_master, dist, tap_prefix);
}

void clear (const char* tap_prefix, const char* endcap_prefix ) {
  getTapcell()->clear(tap_prefix, endcap_prefix);
}

void reset()
{
  getTapcell()->reset();
}

void cut_rows(odb::dbMaster* endcap_master, std::vector<odb::dbInst*> blockages,int halo_x, int halo_y)
{
  getTapcell()->cutRows(endcap_master, blockages, halo_x, halo_y);
}

std::vector<std::vector<odb::dbRow*>> organize_rows()
{
  return getTapcell()->organizeRows();
}

int insert_endcaps(std::vector<std::vector<odb::dbRow*>> rows, odb::dbMaster* endcap_master, std::vector<std::string> cnrcap_masters, const char* prefix)
{
  return getTapcell()->insertEndcaps(rows, endcap_master, cnrcap_masters, prefix);
}

int insert_at_top_bottom(std::vector<std::vector<odb::dbRow*>> rows, std::vector<std::string> masters, odb::dbMaster* endcap_master, std::string prefix)
{
  return getTapcell()->insertAtTopBottom(rows, masters, endcap_master, prefix);
}

int insert_around_macros(std::vector<std::vector<odb::dbRow*>> rows, std::vector<std::string> masters, odb::dbMaster* corner_master, std::string prefix)
{
  return getTapcell()->insertAroundMacros(rows, masters, corner_master, prefix);
}

int remove_cells(const char* prefix)
{
  return getTapcell()->removeCells(prefix);
}

std::vector<odb::dbInst*> find_blockages()
{
  return getTapcell()->findBlockages();
}

int insert_tapcells(std::vector<vector<odb::dbRow*>> rows, std::string tapcell_master, int dist, std::string prefix)
{
  return getTapcell()->insertTapcells(rows, tapcell_master, dist, prefix);
}

bool overlaps(odb::dbInst* blockage, odb::dbRow* row, int& halo_x, int& halo_y) {
  return overlaps(blockage, row, halo_x, halo_y);
}

} // namespace

%}
