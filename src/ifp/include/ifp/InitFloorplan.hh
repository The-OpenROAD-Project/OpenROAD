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

#pragma once

#include <set>
#include <string>
#include <vector>

#include "odb/db.h"

namespace utl {
class Logger;
}

namespace sta {
class dbNetwork;
class Report;
}  // namespace sta

namespace ifp {

using sta::dbNetwork;
using utl::Logger;

class InitFloorplan
{
 public:
  InitFloorplan(odb::dbBlock* block, Logger* logger, sta::dbNetwork* network);

  // utilization is in [0, 100]%
  void initFloorplan(double utilization,
                     double aspect_ratio,
                     int core_space_bottom,
                     int core_space_top,
                     int core_space_left,
                     int core_space_right,
                     const std::vector<odb::dbSite*>& extra_sites = {});

  void initFloorplan(const odb::Rect& die,
                     const odb::Rect& core,
                     const std::vector<odb::dbSite*>& extra_sites = {});

  void insertTiecells(odb::dbMTerm* tie_term,
                      const std::string& prefix = "TIEOFF_");

  void makeTracks();
  void makeTracks(odb::dbTechLayer* layer,
                  int x_offset,
                  int x_pitch,
                  int y_offset,
                  int y_pitch);

  void makeTracksNonUniform(odb::dbTechLayer* layer,
                            int x_offset,
                            int x_pitch,
                            int y_offset,
                            int y_pitch,
                            int first_last_pitch);

  odb::dbSite* findSite(const char* site_name);

 private:
  double designArea();
  int makeRows(odb::dbSite* site,
               int core_lx,
               int core_ly,
               int core_ux,
               int core_uy,
               int factor,
               int row_index);
  int getOffset(const std::vector<odb::dbSite*>& pattern) const;
  int makeHybridRows(odb::dbSite* parent_hybrid_site,
                     const odb::Point& core_ll,
                     const odb::Point& core_ur,
                     int row_index);
  void generateContiguousHybridRows(
      odb::dbSite* parent_hybrid_site,
      const odb::dbSite::RowPattern& row_pattern,
      std::vector<std::vector<odb::dbSite*>>& output_patterns_list);
  void makeTracks(const char* tracks_file, odb::Rect& die_area);
  void autoPlacePins(odb::dbTechLayer* pin_layer, odb::Rect& core);
  int snapToMfgGrid(int coord) const;
  void updateVoltageDomain(int core_lx, int core_ly, int core_ux, int core_uy);
  std::set<odb::dbSite*> getSites() const;

  odb::dbBlock* block_;
  Logger* logger_;
  sta::dbNetwork* network_;

  // this is a set of sets of all constructed site ids.
  std::set<std::set<int>> constructed_patterns_;
  std::vector<std::vector<odb::dbSite*>> repeating_row_patterns_;
};

}  // namespace ifp
