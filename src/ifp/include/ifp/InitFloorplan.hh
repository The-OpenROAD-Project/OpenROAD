// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

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

enum class RowParity
{
  NONE,
  EVEN,
  ODD
};

class InitFloorplan
{
 public:
  InitFloorplan() = default;  // only for swig
  InitFloorplan(odb::dbBlock* block, Logger* logger, sta::dbNetwork* network);

  // utilization is in [0, 100]%
  // The base_site determines the single-height rows.  For hybrid rows it is
  // a site containing a row pattern.
  void initFloorplan(double utilization,
                     double aspect_ratio,
                     int core_space_bottom,
                     int core_space_top,
                     int core_space_left,
                     int core_space_right,
                     odb::dbSite* base_site,
                     const std::vector<odb::dbSite*>& additional_sites = {},
                     RowParity row_parity = RowParity::NONE,
                     const std::set<odb::dbSite*>& flipped_sites = {});

  // The base_site determines the single-height rows.  For hybrid rows it is
  // a site containing a row pattern.
  void initFloorplan(const odb::Rect& die,
                     const odb::Rect& core,
                     odb::dbSite* base_site,
                     const std::vector<odb::dbSite*>& additional_sites = {},
                     RowParity row_parity = RowParity::NONE,
                     const std::set<odb::dbSite*>& flipped_sites = {});

  void insertTiecells(odb::dbMTerm* tie_term,
                      const std::string& prefix = "TIEOFF_");

  // Rect in DBU to set the die area of the top level block.
  void makeDie(const odb::Rect& die);

  // utilization is in [0, 100]%. This routine will calculate the required
  // core area for all the standard cells and macros based on the desired
  // utilization. It will then add core space to each side, and will set
  // the die area of the top level block to that calculated value.
  void makeDieUtilization(double utilization,
                          double aspect_ratio,
                          int core_space_bottom,
                          int core_space_top,
                          int core_space_left,
                          int core_space_right);

  // The base_site determines the single-height rows.  For hybrid rows it is
  // a site containing a row pattern. core space is the padding on each side
  // to inset the rows.
  void makeRowsWithSpacing(int core_space_bottom,
                           int core_space_top,
                           int core_space_left,
                           int core_space_right,
                           odb::dbSite* base_site,
                           const std::vector<odb::dbSite*>& additional_sites
                           = {},
                           RowParity row_parity = RowParity::NONE,
                           const std::set<odb::dbSite*>& flipped_sites = {});

  // The base_site determines the single-height rows.  For hybrid rows it is
  // a site containing a row pattern.
  void makeRows(const odb::Rect& core,
                odb::dbSite* base_site,
                const std::vector<odb::dbSite*>& additional_sites = {},
                RowParity row_parity = RowParity::NONE,
                const std::set<odb::dbSite*>& flipped_sites = {});

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
  using SitesByName = std::map<std::string, odb::dbSite*>;

  double designArea();
  void checkInstanceDimensions(const odb::Rect& core) const;
  void makeRows(const odb::dbSite::RowPattern& pattern, const odb::Rect& core);
  void makeUniformRows(odb::dbSite* base_site,
                       const SitesByName& sites_by_name,
                       const odb::Rect& core,
                       RowParity row_parity,
                       const std::set<odb::dbSite*>& flipped_sites);
  void makeHybridRows(odb::dbSite* base_hybrid_site,
                      const SitesByName& sites_by_name,
                      const odb::Rect& core);
  int getOffset(odb::dbSite* base_hybrid_site,
                odb::dbSite* site,
                odb::dbOrientType& orientation) const;
  void makeTracks(const char* tracks_file, odb::Rect& die_area);
  void autoPlacePins(odb::dbTechLayer* pin_layer, odb::Rect& core);
  int snapToMfgGrid(int coord) const;
  void updateVoltageDomain(int core_lx, int core_ly, int core_ux, int core_uy);
  void addUsedSites(std::map<std::string, odb::dbSite*>& sites_by_name) const;

  odb::dbBlock* block_{nullptr};
  Logger* logger_{nullptr};
  sta::dbNetwork* network_{nullptr};

  // this is a set of sets of all constructed site ids.
  std::set<std::set<int>> constructed_patterns_;
  std::vector<std::vector<odb::dbSite*>> repeating_row_patterns_;
};

}  // namespace ifp
