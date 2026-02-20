// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace sta {
class dbNetwork;
class Report;
}  // namespace sta

namespace ifp {

enum class RowParity
{
  NONE,
  EVEN,
  ODD
};

class InitFloorplan
{
 public:
  void makePolygonDie(const odb::Polygon& polygon);

  InitFloorplan() = default;  // only for swig
  InitFloorplan(odb::dbBlock* block,
                utl::Logger* logger,
                sta::dbNetwork* network);

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
                     const std::set<odb::dbSite*>& flipped_sites = {},
                     int gap = std::numeric_limits<std::int32_t>::min());

  // The base_site determines the single-height rows.  For hybrid rows it is
  // a site containing a row pattern.
  void initFloorplan(const odb::Rect& die,
                     const odb::Rect& core,
                     odb::dbSite* base_site,
                     const std::vector<odb::dbSite*>& additional_sites = {},
                     RowParity row_parity = RowParity::NONE,
                     const std::set<odb::dbSite*>& flipped_sites = {},
                     int gap = std::numeric_limits<std::int32_t>::min());

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
                           const std::set<odb::dbSite*>& flipped_sites = {},
                           int gap = std::numeric_limits<std::int32_t>::min());

  // The base_site determines the single-height rows.  For hybrid rows it is
  // a site containing a row pattern.
  void makeRows(const odb::Rect& core,
                odb::dbSite* base_site,
                const std::vector<odb::dbSite*>& additional_sites = {},
                RowParity row_parity = RowParity::NONE,
                const std::set<odb::dbSite*>& flipped_sites = {},
                int gap = std::numeric_limits<std::int32_t>::min());

  // Create rows for a polygon core area using true polygon-aware generation
  void makePolygonRows(const odb::Polygon& core_polygon,
                       odb::dbSite* base_site,
                       const std::vector<odb::dbSite*>& additional_sites = {},
                       RowParity row_parity = RowParity::NONE,
                       const std::set<odb::dbSite*>& flipped_sites = {},
                       int gap = std::numeric_limits<std::int32_t>::min());

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
  void resetTracks() const;

  odb::dbSite* findSite(const char* site_name);

 private:
  using SitesByName = std::map<std::string, odb::dbSite*>;

  double designArea();
  void checkInstanceDimensions(const odb::Rect& core) const;
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
  void updateVoltageDomain(int core_lx,
                           int core_ly,
                           int core_ux,
                           int core_uy,
                           int gap);
  void addUsedSites(std::map<std::string, odb::dbSite*>& sites_by_name) const;
  void reportAreas();

  // Private methods for polygon-aware row generation using scanline
  // intersection
  void makePolygonRowsScanline(const odb::Polygon& core_polygon,
                               odb::dbSite* base_site,
                               const SitesByName& sites_by_name,
                               RowParity row_parity,
                               const std::set<odb::dbSite*>& flipped_sites,
                               int gap);

  std::vector<odb::Rect> intersectRowWithPolygon(const odb::Rect& row,
                                                 const odb::Polygon& polygon);

  void makeUniformRowsPolygon(odb::dbSite* site,
                              const odb::Polygon& core_polygon,
                              const odb::Rect& core_bbox,
                              RowParity row_parity,
                              const std::set<odb::dbSite*>& flipped_sites);

  odb::dbBlock* block_{nullptr};
  utl::Logger* logger_{nullptr};
  sta::dbNetwork* network_{nullptr};

  // this is a set of sets of all constructed site ids.
  std::set<std::set<int>> constructed_patterns_;
  std::vector<std::vector<odb::dbSite*>> repeating_row_patterns_;

  void checkGap(int gap);
};

}  // namespace ifp
