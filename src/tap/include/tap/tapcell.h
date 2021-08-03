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


#include <tcl.h>
#include "opendb/db.h"

namespace ord {
class OpenRoad;
}

namespace utl {
class Logger;
}

namespace odb {
class dbDatabase;
class dbTech;
class dbBlock;
}  // namespace odb

namespace tap {

class Tapcell
{
public:
  Tapcell();
  ~Tapcell();
  void init(odb::dbDatabase *db, utl::Logger* logger);
  //void run();
  void clear (const char* tap_prefix, const char* endcap_prefix);
  void run(odb::dbMaster* endcap_master, int& halo_x, int& halo_y, const char* cnrcap_nwin_master, const char* cnrcap_nwout_master, const char* endcap_prefix, int& add_boundary_cell, const char* tap_nwintie_master, const char* tap_nwin2_master, const char* tap_nwin3_master, const char* tap_nwouttie_master, const char* tap_nwout2_master, const char* tap_nwout3_master, const char* incnrcap_nwin_master, const char* incnrcap_nwout_master, const char* tapcell_master, int& dist, const char* tap_prefix);
  void reset();
  void cutRows(odb::dbMaster* endcap_master, std::vector<odb::dbInst*>
   blockages,int& halo_x, int& halo_y);
  int removeCells(const char* prefix);
  int insertEndcaps(std::vector<std::vector<odb::dbRow*>>& rows, odb::dbMaster* endcap_master, std::vector<std::string>& cnrcap_masters, const char* prefix);
  std::vector<std::vector<odb::dbRow*>> organizeRows();
  int insertTapcells(std::vector<std::vector<odb::dbRow*>>& rows, std::string tapcell_master, int& dist, std::string prefix);
  int insertAtTopBottom(std::vector<std::vector<odb::dbRow*>>& rows, std::vector<std::string> masters, odb::dbMaster* endcap_master, std::string prefix);
  int insertAroundMacros(std::vector<std::vector<odb::dbRow*>>& rows, std::vector<std::string>& masters, odb::dbMaster* corner_master, std::string
   prefix);
  bool overlaps(odb::dbInst* blockage, odb::dbRow* row, int& halo_x, int& halo_y);
  std::vector<odb::dbInst*> findBlockages();
private:
  odb::dbDatabase *db_;
  utl::Logger* logger_;
  int phy_idx;
  std::vector<std::vector<int>> filled_sites;

  void  cutRow(odb::dbBlock* block, odb::dbRow* row, std::map<std::string, std::vector<odb::dbBox*>>& row_blockages, int& min_row_width, int& halo_x, int& halo_y);
  int makeSiteLoc( int x, double site_x, int dirc, int& offset);
  void buildRow(odb::dbBlock* block, std::string name, odb::dbSite* site, int start_x, int end_x, int y, odb::dbOrientType& orient, odb::dbRowDir& direction, int& min_row_width);
  std::pair<int, int> getMinMaxX(std::vector<std::vector<odb::dbRow*>>& rows);
  odb::dbMaster* pickCornerMaster(int top_bottom, odb::dbOrientType ori, odb::dbMaster* cnrcap_nwin_master, odb::dbMaster* cnrcap_nwout_master, odb::dbMaster* endcap_master);
  bool checkSymmetry( odb::dbMaster* master, odb::dbOrientType ori);
  bool isXCorner(int& x, std::vector<odb::dbRow*>& rows_above, std::vector<odb::dbRow*>& rows_below);
  void buildCell( odb::dbBlock* block, odb::dbMaster* master, odb::dbOrientType orientation, int x, int y, std::string prefix);
  bool isXInRow(int& x, std::vector<odb::dbRow*>& subrow);
  bool checkIfFilled(int& x, int& width, odb::dbOrientType& orient, std::vector<std::vector<int>>& row_insts);
  void insertAtTopBottomHelper(odb::dbBlock* block, int top_bottom, bool is_macro, odb::dbOrientType ori, int& x_start, int& x_end, int& lly, odb::dbMaster* tap_nwintie_master, odb::dbMaster* tap_nwin2_master, odb::dbMaster* tap_nwin3_master, odb::dbMaster* tap_nwouttie_master, odb::dbMaster* tap_nwout2_master, odb::dbMaster* tap_nwout3_master, std::string prefix);
  std::map<std::pair<int, int>, std::vector<int>> getMacroOutlines(std::vector<std::vector<odb::dbRow*>>& rows);
};

}
