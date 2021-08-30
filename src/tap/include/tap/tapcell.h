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

#include "odb/db.h"

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

enum LocationType 
{
  AboveMacro,
  BelowMacro,
  None,
};

class Tapcell
{
 public:
  Tapcell();
  ~Tapcell();
  void init(odb::dbDatabase* db, utl::Logger* logger);
  void setTapPrefix(const std::string& tap_prefix);
  void setEndcapPrefix(const std::string& endcap_prefix);
  void clear();
  void run(odb::dbMaster* endcap_master,
           int halo_x,
           int halo_y,
           const std::string& cnrcap_nwin_master,
           const std::string& cnrcap_nwout_master,
           int add_boundary_cell,
           const std::string& tap_nwintie_master,
           const std::string& tap_nwin2_master,
           const std::string& tap_nwin3_master,
           const std::string& tap_nwouttie_master,
           const std::string& tap_nwout2_master,
           const std::string& tap_nwout3_master,
           const std::string& incnrcap_nwin_master,
           const std::string& incnrcap_nwout_master,
           const std::string& tapcell_master,
           int dist);
  void reset();
  int makeSiteLoc(int x, double site_x, bool at_left_from_macro, int offset);
  void buildRow(odb::dbBlock* block,
                const std::string& name,
                odb::dbSite* site,
                int start_x,
                int end_x,
                int y,
                odb::dbOrientType& orient,
                odb::dbRowDir& direction,
                int min_row_width);
  void cutRows(odb::dbMaster* endcap_master,
               const std::vector<odb::dbBox*>& blockages,
               int halo_x,
               int halo_y);
  bool overlaps(odb::dbBox* blockage,
              odb::dbRow* row,
              int halo_x,
              int halo_y);
  int removeCells(const std::string& prefix);
  std::vector<odb::dbBox*> findBlockages();
 
 private:
  struct FilledSites
  {
    int yMin;
    int xMin;
    int xMax;
  };
  //Cells placed at corners of macros & corners of core area
  struct CornercapMasters
  {
    std::string nwin_master;
    std::string nwout_master;
  };
  odb::dbDatabase* db_;
  utl::Logger* logger_;
  int phy_idx_;
  std::vector<FilledSites> filled_sites_;
  std::string tap_prefix_;
  std::string endcap_prefix_;
  void cutRow(odb::dbBlock* block,
              odb::dbRow* row,
              std::vector<odb::dbBox*>& row_blockages,
              int min_row_width,
              int halo_x,
              int halo_y);
  const std::pair<int, int> getMinMaxX(const std::vector<std::vector<odb::dbRow*>>& rows);
  odb::dbMaster* pickCornerMaster(LocationType top_bottom,
                                  odb::dbOrientType ori,
                                  odb::dbMaster* cnrcap_nwin_master,
                                  odb::dbMaster* cnrcap_nwout_master,
                                  odb::dbMaster* endcap_master);
  bool checkSymmetry(odb::dbMaster* master, odb::dbOrientType ori);
  LocationType getLocationType(const int x,
                const std::vector<odb::dbRow*>& rows_above,
                const std::vector<odb::dbRow*>& rows_below);
  void makeInstance(odb::dbBlock* block,
                 odb::dbMaster* master,
                 odb::dbOrientType orientation,
                 int x,
                 int y,
                 const std::string& prefix);
  bool isXInRow(const int x, const std::vector<odb::dbRow*>& subrow);
  int checkIfFilled(int x,
                     int width,
                     odb::dbOrientType& orient,
                     const std::vector<std::vector<int>>& row_insts);
  int insertAtTopBottom(const std::vector<std::vector<odb::dbRow*>>& rows,
                        const std::vector<std::string>& masters,
                        odb::dbMaster* endcap_master,
                        const std::string& prefix);
  void insertAtTopBottomHelper(odb::dbBlock* block,
                               int top_bottom,
                               bool is_macro,
                               odb::dbOrientType ori,
                               int x_start,
                               int x_end,
                               int lly,
                               odb::dbMaster* tap_nwintie_master,
                               odb::dbMaster* tap_nwin2_master,
                               odb::dbMaster* tap_nwin3_master,
                               odb::dbMaster* tap_nwouttie_master,
                               odb::dbMaster* tap_nwout2_master,
                               odb::dbMaster* tap_nwout3_master,
                               const std::string& prefix);
  int insertAroundMacros(const std::vector<std::vector<odb::dbRow*>>& rows,
                         const std::vector<std::string>& masters,
                         odb::dbMaster* corner_master,
                         const std::string& prefix);
  std::map<std::pair<int, int>, std::vector<int>> getMacroOutlines(
      const std::vector<std::vector<odb::dbRow*>>& rows);
  int insertEndcaps(const std::vector<std::vector<odb::dbRow*>>& rows,
                    odb::dbMaster* endcap_master,
                    const CornercapMasters& masters);
  std::vector<std::vector<odb::dbRow*>> organizeRows();
  int insertTapcells(const std::vector<std::vector<odb::dbRow*>>& rows,
                     const std::string& tapcell_master,
                     int dist);
};

}  // namespace tap