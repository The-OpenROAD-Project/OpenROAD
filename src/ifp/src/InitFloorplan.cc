// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "ifp/InitFloorplan.hh"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <ranges>
#include <set>
#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/util.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "sta/StringUtil.hh"
#include "upf/upf.h"
#include "utl/Logger.h"
#include "utl/validation.h"

namespace ifp {

using std::ceil;
using std::map;
using std::round;
using std::set;
using std::string;

using sta::StringVector;

using utl::IFP;
using utl::Logger;

using odb::dbBlock;
using odb::dbBox;
using odb::dbGroup;
using odb::dbGroupType;
using odb::dbInst;
using odb::dbLib;
using odb::dbMaster;
using odb::dbOrientType;
using odb::dbRegion;
using odb::dbRow;
using odb::dbRowDir;
using odb::dbSite;
using odb::dbTech;
using odb::dbTechLayerDir;
using odb::dbTechLayerType;
using odb::dbTrackGrid;
using odb::Rect;

using upf::eval_upf;

namespace {
void validateCoreSpacing(odb::dbBlock* block,
                         utl::Logger* logger,
                         int core_space_bottom,
                         int core_space_top,
                         int core_space_left,
                         int core_space_right)
{
  utl::Validator v(logger, IFP);
  v.check_non_negative(
      "core_space_bottom (um) ", block->dbuToMicrons(core_space_bottom), 32);
  v.check_non_negative(
      "core_space_top (um) ", block->dbuToMicrons(core_space_top), 33);
  v.check_non_negative(
      "core_space_left (um) ", block->dbuToMicrons(core_space_left), 34);
  v.check_non_negative(
      "core_space_right (um) ", block->dbuToMicrons(core_space_right), 35);
}
}  // namespace

InitFloorplan::InitFloorplan(dbBlock* block,
                             Logger* logger,
                             sta::dbNetwork* network)
    : block_(block), logger_(logger), network_(network)
{
}

void InitFloorplan::checkGap(const int gap)
{
  if (gap != std::numeric_limits<int32_t>::min() && gap <= 0) {
    logger_->error(IFP, 36, "Gap must be positive ({})", gap);
  }
}

void InitFloorplan::initFloorplan(
    double utilization,
    double aspect_ratio,
    int core_space_bottom,
    int core_space_top,
    int core_space_left,
    int core_space_right,
    odb::dbSite* base_site,
    const std::vector<odb::dbSite*>& additional_sites,
    RowParity row_parity,
    const std::set<odb::dbSite*>& flipped_sites,
    const int gap)
{
  checkGap(gap);

  makeDieUtilization(utilization,
                     aspect_ratio,
                     core_space_bottom,
                     core_space_top,
                     core_space_left,
                     core_space_right);
  makeRowsWithSpacing(core_space_bottom,
                      core_space_top,
                      core_space_left,
                      core_space_right,
                      base_site,
                      additional_sites,
                      row_parity,
                      flipped_sites,
                      gap);
}

// The base_site determines the single-height rows.  For hybrid rows it is
// a site containing a row pattern.
void InitFloorplan::initFloorplan(
    const odb::Rect& die,
    const odb::Rect& core,
    odb::dbSite* base_site,
    const std::vector<odb::dbSite*>& additional_sites,
    RowParity row_parity,
    const std::set<odb::dbSite*>& flipped_sites,
    const int gap)
{
  checkGap(gap);

  makeDie(die);
  makeRows(core, base_site, additional_sites, row_parity, flipped_sites, gap);
}

void InitFloorplan::makeDieUtilization(double utilization,
                                       double aspect_ratio,
                                       int core_space_bottom,
                                       int core_space_top,
                                       int core_space_left,
                                       int core_space_right)
{
  utl::Validator v(logger_, IFP);
  v.check_non_negative("utilization", utilization, 12);
  v.check_positive("aspect_ratio", aspect_ratio, 36);
  validateCoreSpacing(block_,
                      logger_,
                      core_space_bottom,
                      core_space_top,
                      core_space_left,
                      core_space_right);
  utilization /= 100;
  logger_->info(
      IFP,
      107,
      "Defining die area using utilization: {:.2f}\% and aspect ratio: {}.",
      utilization * 100,
      aspect_ratio);
  const double design_area = designArea();
  const double core_area = design_area / utilization;
  const int core_width = std::sqrt(core_area / aspect_ratio);
  const int core_height = round(core_width * aspect_ratio);

  const int core_lx = core_space_left;
  const int core_ly = core_space_bottom;
  const int core_ux = core_lx + core_width;
  const int core_uy = core_ly + core_height;
  const int die_lx = 0;
  const int die_ly = 0;
  const int die_ux = core_ux + core_space_right;
  const int die_uy = core_uy + core_space_top;

  makeDie({die_lx, die_ly, die_ux, die_uy});
}

void InitFloorplan::makeDie(const odb::Rect& die)
{
  debugPrint(logger_,
             IFP,
             "init",
             1,
             "Defining die area: ({}, {}) to ({}, {}) DBU",
             die.xMin(),
             die.yMin(),
             die.xMax(),
             die.yMax());

  Rect die_area(snapToMfgGrid(die.xMin()),
                snapToMfgGrid(die.yMin()),
                snapToMfgGrid(die.xMax()),
                snapToMfgGrid(die.yMax()));
  block_->setDieArea(die_area);

  resetTracks();
}

void InitFloorplan::makePolygonDie(const odb::Polygon& polygon)
{
  auto points = polygon.getPoints();
  logger_->info(IFP, 106, "Initializing floorplan in polygon mode.");

  if (points.empty()) {
    logger_->error(IFP, 987, "No polygon vertices provided.");
    return;
  }

  if (points.size() < 4) {
    logger_->error(IFP,
                   988,
                   "Polygon must have at least 4 vertices. Got {} vertices.",
                   points.size());
    return;
  }

  // Snap all coordinates to manufacturing grid
  std::vector<odb::Point> mfg_pts;
  mfg_pts.reserve(points.size());
  for (const auto& p : points) {
    mfg_pts.emplace_back(snapToMfgGrid(p.x()), snapToMfgGrid(p.y()));
  }

  // Set the die area using the polygon
  block_->setDieArea(polygon);

  resetTracks();
}

void InitFloorplan::makePolygonRows(
    const odb::Polygon& core_polygon,
    odb::dbSite* base_site,
    const std::vector<odb::dbSite*>& additional_sites,
    RowParity row_parity,
    const std::set<odb::dbSite*>& flipped_sites,
    const int gap)
{
  checkGap(gap);

  auto points = core_polygon.getPoints();

  if (points.empty()) {
    logger_->error(IFP, 991, "No core polygon vertices provided.");
    return;
  }

  if (points.size() < 4) {
    logger_->error(
        IFP,
        992,
        "Core polygon must have at least 4 vertices. Got {} vertices.",
        points.size());
    return;
  }

  // Snap all coordinates to manufacturing grid
  std::vector<odb::Point> mfg_pts;
  mfg_pts.reserve(points.size());
  for (const auto& p : points) {
    mfg_pts.emplace_back(snapToMfgGrid(p.x()), snapToMfgGrid(p.y()));
  }

  // Set up the sites as in the original makeRows function
  odb::Rect block_die_area = block_->getDieArea();
  if (block_die_area.area() == 0) {
    logger_->error(IFP, 1005, "Floorplan die area is 0. Cannot build rows.");
    return;
  }

  // Create a polygon from the core vertices and get bounding box
  odb::Polygon core_poly(mfg_pts);
  odb::Rect core_bbox = core_poly.getEnclosingRect();

  if (!block_die_area.contains(core_bbox)) {
    logger_->error(
        IFP, 1004, "Die area must contain the core polygon bounding box.");
    return;
  }

  checkInstanceDimensions(core_bbox);

  debugPrint(logger_,
             IFP,
             "init",
             1,
             "Defining core area: ({}, {}) to ({}, {}) DBU",
             core_bbox.xMin(),
             core_bbox.yMin(),
             core_bbox.xMax(),
             core_bbox.yMax());

  // Set up sites by name (same as original makeRows)
  SitesByName sites_by_name;
  sites_by_name[base_site->getName()] = base_site;
  if (base_site->hasRowPattern()) {
    for (const auto& [site, orient] : base_site->getRowPattern()) {
      sites_by_name[site->getName()] = site;
    }
  }
  for (auto site : additional_sites) {
    sites_by_name[site->getName()] = site;
  }
  addUsedSites(sites_by_name);

  // Remove all existing rows
  auto rows = block_->getRows();
  for (auto row_itr = rows.begin(); row_itr != rows.end();) {
    row_itr = dbRow::destroy(row_itr);
  }

  // Use the new scanline-based approach
  makePolygonRowsScanline(
      core_poly, base_site, sites_by_name, row_parity, flipped_sites, gap);

  logger_->info(IFP,
                997,
                "Completed polygon-aware row generation using {} vertices",
                points.size() - 1);
  reportAreas();
}

double InitFloorplan::designArea()
{
  double design_area = 0.0;
  for (dbInst* inst : block_->getInsts()) {
    dbMaster* master = inst->getMaster();
    const double area
        = master->getHeight() * static_cast<double>(master->getWidth());
    design_area += area;
  }
  return design_area;
}

void InitFloorplan::checkInstanceDimensions(const odb::Rect& core) const
{
  for (dbInst* inst : block_->getInsts()) {
    dbMaster* master = inst->getMaster();

    if (master->isPad() || master->isCover()) {
      continue;
    }

    bool fails = false;
    if (master->getSymmetryR90()) {
      fails
          = std::max(master->getWidth(), master->getHeight()) > core.maxDXDY();
    } else {
      fails = master->getWidth() > core.dx() || master->getHeight() > core.dy();
    }

    if (fails) {
      logger_->error(utl::IFP,
                     2,
                     "{} ({:.3f}um, {:.3f}um) does not fit in the core area: "
                     "({:.3f}um, {:.3f}um) - ({:.3f}um, {:.3f}um)",
                     inst->getName(),
                     block_->dbuToMicrons(master->getWidth()),
                     block_->dbuToMicrons(master->getHeight()),
                     block_->dbuToMicrons(core.xMin()),
                     block_->dbuToMicrons(core.yMin()),
                     block_->dbuToMicrons(core.xMax()),
                     block_->dbuToMicrons(core.yMax()));
    }
  }
}

static int divCeil(int dividend, int divisor)
{
  return ceil(static_cast<double>(dividend) / divisor);
}

void InitFloorplan::makeRowsWithSpacing(
    int core_space_bottom,
    int core_space_top,
    int core_space_left,
    int core_space_right,
    odb::dbSite* base_site,
    const std::vector<odb::dbSite*>& additional_sites,
    RowParity row_parity,
    const std::set<odb::dbSite*>& flipped_sites,
    const int gap)
{
  checkGap(gap);

  odb::Rect block_die_area = block_->getDieArea();
  if (block_die_area.area() == 0) {
    logger_->error(IFP, 64, "Floorplan die area is 0. Cannot build rows.");
  }

  validateCoreSpacing(block_,
                      logger_,
                      core_space_bottom,
                      core_space_top,
                      core_space_left,
                      core_space_right);

  int lower_left_x = block_die_area.ll().x();
  int lower_left_y = block_die_area.ll().y();
  int upper_right_x = block_die_area.ur().x();
  int upper_right_y = block_die_area.ur().y();

  int core_lx = lower_left_x + core_space_left;
  int core_ly = lower_left_y + core_space_bottom;
  int core_ux = upper_right_x - core_space_right;
  int core_uy = upper_right_y - core_space_top;

  makeRows({core_lx, core_ly, core_ux, core_uy},
           base_site,
           additional_sites,
           row_parity,
           flipped_sites,
           gap);
}

void InitFloorplan::makeRows(const odb::Rect& core,
                             odb::dbSite* base_site,
                             const std::vector<odb::dbSite*>& additional_sites,
                             RowParity row_parity,
                             const std::set<odb::dbSite*>& flipped_sites,
                             const int gap)
{
  checkGap(gap);

  debugPrint(logger_,
             IFP,
             "init",
             1,
             "Defining core area: ({}, {}) to ({}, {}) DBU",
             core.xMin(),
             core.yMin(),
             core.xMax(),
             core.yMax());

  odb::Rect block_die_area = block_->getDieArea();
  if (block_die_area.area() == 0) {
    logger_->error(IFP, 63, "Floorplan die area is 0. Cannot build rows.");
  }

  if (!block_die_area.contains(core)) {
    logger_->error(IFP, 55, "Die area must contain the core area.");
  }

  checkInstanceDimensions(core);

  // The same site can appear in more than one LEF file and therefore
  // in more than one dbLib.  We merge them by name to avoid duplicate
  // rows.
  SitesByName sites_by_name;
  sites_by_name[base_site->getName()] = base_site;
  if (base_site->hasRowPattern()) {
    for (const auto& [site, orient] : base_site->getRowPattern()) {
      sites_by_name[site->getName()] = site;
    }
  }
  for (auto site : additional_sites) {
    sites_by_name[site->getName()] = site;
  }
  addUsedSites(sites_by_name);

  // remove all rows
  auto rows = block_->getRows();
  for (auto row_itr = rows.begin(); row_itr != rows.end();) {
    row_itr = dbRow::destroy(row_itr);
  }

  if (core.xMin() >= 0 && core.yMin() >= 0) {
    eval_upf(network_, logger_, block_);

    const uint32_t site_dx = base_site->getWidth();
    const uint32_t site_dy = base_site->getHeight();
    // snap core lower left corner to multiple of site dx/dy.
    const int clx = divCeil(core.xMin(), site_dx) * site_dx;
    const int cly = divCeil(core.yMin(), site_dy) * site_dy;
    const int cux = core.xMax();
    const int cuy = core.yMax();

    const odb::Rect snapped_core(clx, cly, cux, cuy);

    if (clx != core.xMin() || cly != core.yMin()) {
      const double dbu = block_->getDbUnitsPerMicron();
      logger_->warn(IFP,
                    28,
                    "Core area lower left ({:.3f}, {:.3f}) snapped to "
                    "({:.3f}, {:.3f}).",
                    core.xMin() / dbu,
                    core.yMin() / dbu,
                    clx / dbu,
                    cly / dbu);
    }

    if (base_site->hasRowPattern()) {
      if (row_parity != RowParity::NONE) {
        logger_->error(
            IFP,
            51,
            "Constraining row parity is not supported for hybrid rows.");
      }
      makeHybridRows(base_site, sites_by_name, snapped_core);
    } else {
      makeUniformRows(
          base_site, sites_by_name, snapped_core, row_parity, flipped_sites);
    }

    updateVoltageDomain(clx, cly, cux, cuy, gap);
  }

  std::vector<dbBox*> blockage_bboxes;
  for (auto blockage : block_->getBlockages()) {
    blockage_bboxes.push_back(blockage->getBBox());
  }

  odb::cutRows(block_,
               /* min_row_width */ 0,
               blockage_bboxes,
               /* halo_x */ 0,
               /* halo_y */ 0,
               logger_);
  reportAreas();
}

// this function is used to create regions ( split overlapped rows and create
// new ones )
void InitFloorplan::updateVoltageDomain(const int core_lx,
                                        const int core_ly,
                                        const int core_ux,
                                        const int core_uy,
                                        const int gap)
{
  // checks if a group is defined as a voltage domain, if so it creates a region
  for (dbGroup* group : block_->getGroups()) {
    if (group->getType() == dbGroupType::VOLTAGE_DOMAIN
        || group->getType() == dbGroupType::POWER_DOMAIN) {
      dbRegion* domain_region = group->getRegion();
      int domain_x_min = std::numeric_limits<int>::max();
      int domain_y_min = std::numeric_limits<int>::max();
      int domain_x_max = std::numeric_limits<int>::min();
      int domain_y_max = std::numeric_limits<int>::min();
      for (auto boundary : domain_region->getBoundaries()) {
        domain_x_min = std::min(domain_x_min, boundary->xMin());
        domain_y_min = std::min(domain_y_min, boundary->yMin());
        domain_x_max = std::max(domain_x_max, boundary->xMax());
        domain_y_max = std::max(domain_y_max, boundary->yMax());
      }

      string domain_name = group->getName();

      std::vector<dbRow*> rows;
      for (dbRow* row : block_->getRows()) {
        if (row->getSite()->getClass() == odb::dbSiteClass::PAD) {
          continue;
        }
        rows.push_back(row);
      }

      int total_row_count = rows.size();

      // Search the minimum site width & height as base unit for snapping and
      // gap calculation
      int min_site_dx = std::numeric_limits<int>::max();
      int min_site_dy = std::numeric_limits<int>::max();
      std::vector<dbRow*>::iterator row_itr = rows.begin();
      for (int row_processed = 0; row_processed < total_row_count;
           row_processed++) {
        dbRow* row = *row_itr;
        auto site = row->getSite();
        int site_dy = site->getHeight();
        min_site_dy = std::min(site_dy, min_site_dy);
        int site_dx = site->getWidth();
        min_site_dx = std::min(site_dx, min_site_dx);
      }
      // Default space is 6 times the minimum site height
      const int power_domain_y_space
          = (gap == std::numeric_limits<int32_t>::min()) ? 6 * min_site_dy
                                                         : gap;

      row_itr = rows.begin();
      for (int row_processed = 0; row_processed < total_row_count;
           row_processed++) {
        dbRow* row = *row_itr;
        Rect row_bbox = row->getBBox();
        int row_y_min = row_bbox.yMin();
        int row_y_max = row_bbox.yMax();
        auto site = row->getSite();

        int site_dx = site->getWidth();

        // snap inward to site grid
        domain_x_min = odb::makeSiteLoc(domain_x_min, min_site_dx, false, 0);
        domain_x_max = odb::makeSiteLoc(domain_x_max, min_site_dx, true, 0);
        domain_y_min = odb::makeSiteLoc(domain_y_min, min_site_dy, false, 0);
        domain_y_max = odb::makeSiteLoc(domain_y_max, min_site_dy, true, 0);

        // check if the rows overlapped with the area of a defined voltage
        // domains + margin
        if (row_y_max + power_domain_y_space <= domain_y_min
            || row_y_min >= domain_y_max + power_domain_y_space) {
          row_itr++;
        } else {
          string row_name = row->getName();
          dbOrientType orient = row->getOrient();
          dbRow::destroy(row);
          row_itr++;

          // lcr stands for left core row
          int lcr_x_max = domain_x_min - power_domain_y_space;
          // in case there is at least one valid site width on the left, create
          // left core rows
          if (lcr_x_max > core_lx + site_dx) {
            string lcr_name = row_name + "_1";
            // warning message since tap cells might not be inserted
            if (lcr_x_max < core_lx + 10 * site_dx) {
              logger_->warn(IFP,
                            26,
                            "left core row: {} has less than 10 sites",
                            lcr_name);
            }
            int lcr_sites = (lcr_x_max - core_lx) / site_dx;
            dbRow::create(block_,
                          lcr_name.c_str(),
                          site,
                          core_lx,
                          row_y_min,
                          orient,
                          dbRowDir::HORIZONTAL,
                          lcr_sites,
                          site_dx);
          }

          // rcr stands for right core row
          int rcr_x_min = domain_x_max + power_domain_y_space;
          // snap to the site grid rightward
          rcr_x_min = odb::makeSiteLoc(rcr_x_min, site_dx, false, 0);

          // in case there is at least one valid site width on the right, create
          // right core rows
          if (rcr_x_min + site_dx < core_ux) {
            string rcr_name = row_name + "_2";
            if (rcr_x_min + 10 * site_dx > core_ux) {
              logger_->warn(IFP,
                            27,
                            "right core row: {} has less than 10 sites",
                            rcr_name);
            }
            int rcr_sites = (core_ux - rcr_x_min) / site_dx;
            dbRow::create(block_,
                          rcr_name.c_str(),
                          site,
                          rcr_x_min,
                          row_y_min,
                          orient,
                          dbRowDir::HORIZONTAL,
                          rcr_sites,
                          site_dx);
          }

          int domain_row_sites = (domain_x_max - domain_x_min) / site_dx;
          // create domain rows if current iterations are not in margin area
          if (row_y_min >= domain_y_min && row_y_max <= domain_y_max) {
            const string domain_row_name
                = fmt::format("{}_{}", row_name, domain_name);
            dbRow::create(block_,
                          domain_row_name.c_str(),
                          site,
                          domain_x_min,
                          row_y_min,
                          orient,
                          dbRowDir::HORIZONTAL,
                          domain_row_sites,
                          site_dx);
          }
        }
      }
    }
  }
}

// Add sites used by any instantiated cell in the block
void InitFloorplan::addUsedSites(
    std::map<std::string, odb::dbSite*>& sites_by_name) const
{
  for (dbInst* inst : block_->getInsts()) {
    dbMaster* master = inst->getMaster();

    if (master->isCoreAutoPlaceable() && !master->isBlock()) {
      auto site = master->getSite();
      if (site) {
        // Avoid adding a site with the same name (ie from two LEF files)
        if (sites_by_name.find(site->getName()) == sites_by_name.end()) {
          sites_by_name[site->getName()] = site;
        }
      } else {
        logger_->warn(IFP,
                      52,
                      "No site found for instance {} in block {}.",
                      inst->getName(),
                      block_->getName());
      }
    }
  }
}

// Create the rows for the core area
void InitFloorplan::makeUniformRows(odb::dbSite* base_site,
                                    const SitesByName& sites_by_name,
                                    const odb::Rect& core,
                                    RowParity row_parity,
                                    const std::set<odb::dbSite*>& flipped_sites)
{
  const int core_dx = core.dx();
  const int core_dy = core.dy();
  const uint32_t site_dx = base_site->getWidth();
  const int rows_x = core_dx / site_dx;

  auto make_rows = [&](dbSite* site) -> int {
    const uint32_t site_dy = site->getHeight();
    int rows_y = core_dy / site_dy;
    bool flip = flipped_sites.find(site) != flipped_sites.end();
    switch (row_parity) {
      case RowParity::NONE:
        break;
      case RowParity::EVEN:
        rows_y = (rows_y / 2) * 2;
        break;
      case RowParity::ODD:
        if (rows_y > 0) {
          rows_y = (rows_y % 2 == 0) ? rows_y - 1 : rows_y;
        } else {
          rows_y = 0;
        }
        break;
    }

    int y = core.yMin();
    for (int row = 0; row < rows_y; row++) {
      dbOrientType orient = ((row + flip) % 2 == 0) ? dbOrientType::R0   // N
                                                    : dbOrientType::MX;  // FS
      string row_name = fmt::format("ROW_{}", block_->getRows().size());
      dbRow::create(block_,
                    row_name.c_str(),
                    site,
                    core.xMin(),
                    y,
                    orient,
                    dbRowDir::HORIZONTAL,
                    rows_x,
                    site_dx);
      y += site_dy;
    }
    if (rows_y == 0) {
      logger_->warn(IFP, 61, "No rows created for site {}.", site->getName());
    } else {
      logger_->info(IFP,
                    1,
                    "Added {} rows of {} site {}.",
                    rows_y,
                    rows_x,
                    site->getName());
    }
    return rows_y;
  };

  int total_rows = 0;
  for (const auto& [name, site] : sites_by_name) {
    if (site->getHeight() % base_site->getHeight() != 0) {
      logger_->error(
          IFP,
          54,
          "Site {} height {}um is not a multiple of site {} height {}um.",
          site->getName(),
          block_->dbuToMicrons(site->getHeight()),
          base_site->getName(),
          block_->dbuToMicrons(base_site->getHeight()));
    }
    total_rows += make_rows(site);
  }
  block_->setCoreArea(block_->computeCoreArea());

  if (total_rows == 0) {
    logger_->error(IFP, 65, "No rows created in the core area.");
  }
}

int InitFloorplan::getOffset(dbSite* base_hybrid_site,
                             dbSite* site,
                             dbOrientType& orientation) const
{
  auto match_pattern
      = [&](const dbSite::RowPattern& pattern, int& offset) -> bool {
    const auto& base_pattern = base_hybrid_site->getRowPattern();

    // Find the common starting point of the patterns
    auto it = std::ranges::find(base_pattern, pattern[0]);
    if (it == base_pattern.end()) {
      return false;
    }
    // Ensure the full pattern matches from this starting point
    int pos = it - base_pattern.begin();
    for (const auto& site_orient : pattern) {
      if (site_orient != base_pattern[pos++ % base_pattern.size()]) {
        return false;
      }
    }
    // The offset is the sum of sites leading to the match start
    offset = 0;
    auto it2 = base_pattern.begin();
    while (it2 != it) {
      offset += it2->site->getHeight();
      ++it2;
    }
    return true;
  };

  const auto& search_pattern = site->getRowPattern();
  int offset;
  if (match_pattern(search_pattern, offset)) {
    orientation = dbOrientType::R0;
    return offset;
  }

  // We may have to flip the row (pattern) to match the parent
  dbSite::RowPattern flipped_search_pattern;
  for (auto [site, orientation] : std::ranges::reverse_view(search_pattern)) {
    dbSite::OrientedSite flipped{site, orientation.flipX()};
    flipped_search_pattern.emplace_back(flipped);
  }

  if (match_pattern(flipped_search_pattern, offset)) {
    orientation = dbOrientType::MX;
    return offset;
  }

  logger_->error(IFP,
                 48,
                 "Site {} is incompatible with site {}",
                 site->getName(),
                 base_hybrid_site->getName());
}

void InitFloorplan::makeHybridRows(dbSite* base_hybrid_site,
                                   const SitesByName& sites_by_name,
                                   const odb::Rect& core)
{
  auto row_pattern = base_hybrid_site->getRowPattern();
  auto first_site = row_pattern[0].site;
  const int site_width = first_site->getWidth();
  const int row_width = core.dx() / site_width;

  int row = 0;
  int y = core.yMin();

  while (true) {
    auto [site, orient] = row_pattern[row % row_pattern.size()];
    if (y + site->getHeight() > core.yMax()) {
      break;
    }
    const string row_name = fmt::format("ROW_{}", row);
    dbRow::create(block_,
                  row_name.c_str(),
                  site,
                  core.xMin(),
                  y,
                  orient,
                  dbRowDir::HORIZONTAL,
                  row_width,
                  site_width);
    y += site->getHeight();
    ++row;
  }
  logger_->info(IFP,
                49,
                "Added {} rows from site {} row pattern.",
                row,
                base_hybrid_site->getName());

  auto make_rows = [&](dbSite* site) {
    dbOrientType orient;
    int y = getOffset(base_hybrid_site, site, orient) + core.yMin();

    int row = 0;

    while (y + site->getHeight() <= core.yMax()) {
      const string row_name = fmt::format("ROW_{}", block_->getRows().size());
      dbRow::create(block_,
                    row_name.c_str(),
                    site,
                    core.xMin(),
                    y,
                    orient,
                    dbRowDir::HORIZONTAL,
                    row_width,
                    site_width);
      y += site->getHeight();
      ++row;
    }
    logger_->info(IFP, 50, "Added {} rows of site {}.", row, site->getName());
  };

  for (const auto& [name, site] : sites_by_name) {
    if (site->hasRowPattern()) {
      make_rows(site);
    }
  }
  block_->setCoreArea(block_->computeCoreArea());
}

dbSite* InitFloorplan::findSite(const char* site_name)
{
  for (dbLib* lib : block_->getDataBase()->getLibs()) {
    dbSite* site = lib->findSite(site_name);
    if (site) {
      return site;
    }
  }
  return nullptr;
}

////////////////////////////////////////////////////////////////

int InitFloorplan::snapToMfgGrid(int coord) const
{
  dbTech* tech = block_->getDataBase()->getTech();
  if (tech->hasManufacturingGrid()) {
    int grid = tech->getManufacturingGrid();
    return round(coord / (double) grid) * grid;
  }

  return coord;
}

void InitFloorplan::insertTiecells(odb::dbMTerm* tie_term,
                                   const std::string& prefix)
{
  utl::Validator v(logger_, IFP);
  v.check_non_null("tie_term", tie_term, 43);

  auto* master = tie_term->getMaster();

  auto* port = network_->dbToSta(tie_term);
  auto* lib_port = network_->libertyPort(port);
  if (!lib_port) {
    logger_->error(utl::IFP,
                   53,
                   "Liberty cell or port {}/{} not found.",
                   master->getName(),
                   tie_term->getName());
  }
  auto func_operation = lib_port->function()->op();
  const bool is_zero = func_operation == sta::FuncExpr::Op::zero;
  const bool is_one = func_operation == sta::FuncExpr::Op::one;

  odb::dbSigType look_for;
  if (is_zero) {
    look_for = odb::dbSigType::GROUND;
  } else if (is_one) {
    look_for = odb::dbSigType::POWER;
  } else {
    logger_->error(utl::IFP,
                   29,
                   "Unable to determine tiecell ({}) function.",
                   master->getName());
  }

  int count = 0;

  for (auto* net : block_->getNets()) {
    if (net->isSpecial()) {
      continue;
    }
    if (look_for != net->getSigType()) {
      continue;
    }

    const std::string inst_name = prefix + net->getName();

    auto* inst = odb::dbInst::create(block_, master, inst_name.c_str());
    auto* iterm = inst->findITerm(tie_term->getConstName());
    iterm->connect(net);
    net->setSigType(odb::dbSigType::SIGNAL);
    count++;
  }

  logger_->info(utl::IFP,
                30,
                "Inserted {} tiecells using {}/{}.",
                count,
                master->getName(),
                tie_term->getName());
}

void InitFloorplan::makeTracks()
{
  for (auto layer : block_->getDataBase()->getTech()->getLayers()) {
    if (layer->getType() == dbTechLayerType::ROUTING
        && layer->getRoutingLevel() != 0) {
      if (layer->getFirstLastPitch() > 0) {
        makeTracksNonUniform(layer,
                             layer->getOffsetX(),
                             layer->getPitchX(),
                             layer->getOffsetY(),
                             layer->getPitchY(),
                             layer->getFirstLastPitch());
      } else {
        const int x_pitch = layer->getPitchX();
        const int y_pitch = layer->getPitchY();
        if (x_pitch == 0 || y_pitch == 0) {
          logger_->warn(
              utl::IFP,
              56,
              "No pitch found layer {} so no tracks will be generated.",
              layer->getName());
          continue;
        }
        makeTracks(
            layer, layer->getOffsetX(), x_pitch, layer->getOffsetY(), y_pitch);
      }
    }
  }
}

void InitFloorplan::resetTracks() const
{
  for (auto layer : block_->getDataBase()->getTech()->getLayers()) {
    auto grid = block_->findTrackGrid(layer);
    if (grid) {
      odb::dbTrackGrid::destroy(grid);
    }
  }
}

void InitFloorplan::makeTracks(odb::dbTechLayer* layer,
                               int x_offset,
                               int x_pitch,
                               int y_offset,
                               int y_pitch)
{
  utl::Validator v(logger_, IFP);
  string layer_inform = "On layer " + layer->getName() + ": ";

  v.check_non_null("layer", layer, 38);
  v.check_non_negative((layer_inform + "x_offset (um)").c_str(),
                       block_->dbuToMicrons(x_offset),
                       39);
  v.check_positive((layer_inform + "x_pitch (um)").c_str(),
                   block_->dbuToMicrons(x_pitch),
                   40);
  v.check_non_negative((layer_inform + "y_offset (um)").c_str(),
                       block_->dbuToMicrons(y_offset),
                       41);
  v.check_positive((layer_inform + "y_pitch (um)").c_str(),
                   block_->dbuToMicrons(y_pitch),
                   42);

  Rect die_area = block_->getDieArea();

  if (x_offset == 0) {
    x_offset = x_pitch;
  }
  if (y_offset == 0) {
    y_offset = y_pitch;
  }

  if (x_offset > die_area.dx()) {
    logger_->warn(
        IFP,
        21,
        "Track pattern for {} will be skipped due to x_offset > die width.",
        layer->getName());
    return;
  }
  if (y_offset > die_area.dy()) {
    logger_->warn(
        IFP,
        22,
        "Track pattern for {} will be skipped due to y_offset > die height.",
        layer->getName());
    return;
  }

  auto grid = block_->findTrackGrid(layer);
  if (!grid) {
    grid = dbTrackGrid::create(block_, layer);
  }

  int layer_min_width = layer->getMinWidth();

  int x_track_count = (die_area.dx() - x_offset) / x_pitch + 1;
  int origin_x = die_area.xMin() + x_offset;
  // Check if the track origin is not usable during routing

  if (origin_x - layer_min_width / 2 < die_area.xMin()) {
    origin_x += x_pitch;
    x_track_count--;
  }

  // Check if the last track is not usable during routing
  int last_x = origin_x + (x_track_count - 1) * x_pitch;
  if (last_x + layer_min_width / 2 > die_area.xMax()) {
    x_track_count--;
  }

  grid->addGridPatternX(origin_x, x_track_count, x_pitch);

  int y_track_count = (die_area.dy() - y_offset) / y_pitch + 1;
  int origin_y = die_area.yMin() + y_offset;
  // Check if the track origin is not usable during routing
  if (origin_y - layer_min_width / 2 < die_area.yMin()) {
    origin_y += y_pitch;
    y_track_count--;
  }

  // Check if the last track is not usable during routing
  int last_y = origin_y + (y_track_count - 1) * y_pitch;
  if (last_y + layer_min_width / 2 > die_area.yMax()) {
    y_track_count--;
  }

  grid->addGridPatternY(origin_y, y_track_count, y_pitch);
}

void InitFloorplan::makeTracksNonUniform(odb::dbTechLayer* layer,
                                         int x_offset,
                                         int x_pitch,
                                         int y_offset,
                                         int y_pitch,
                                         int first_last_pitch)
{
  if (layer->getDirection() != dbTechLayerDir::HORIZONTAL) {
    logger_->error(IFP, 44, "Non horizontal layer uses property LEF58_PITCH.");
  }

  int cell_row_height = 0;
  auto rows = block_->getRows();
  for (auto row : rows) {
    if (row->getSite()->getClass() == odb::dbSiteClass::CORE) {
      cell_row_height = row->getSite()->getHeight();
      break;
    }
  }

  if (cell_row_height == 0) {
    logger_->error(
        IFP, 45, "No routing Row found in layer {}", layer->getName());
  }
  Rect die_area = block_->getDieArea();

  int y_track_count = (cell_row_height - 2 * first_last_pitch) / y_pitch + 1;
  int origin_y = die_area.yMin() + first_last_pitch;
  for (int i = 0; i < y_track_count; i++) {
    makeTracks(layer, x_offset, x_pitch, origin_y, cell_row_height);
    origin_y += y_pitch;
  }
  origin_y += first_last_pitch - y_pitch;
  makeTracks(layer, x_offset, x_pitch, origin_y, cell_row_height);
}

// Scanline-based polygon-aware row generation methods
void InitFloorplan::makePolygonRowsScanline(
    const odb::Polygon& core_polygon,
    odb::dbSite* base_site,
    const SitesByName& sites_by_name,
    RowParity row_parity,
    const std::set<odb::dbSite*>& flipped_sites,
    const int gap)
{
  // Get the bounding box for the polygon
  odb::Rect core_bbox = core_polygon.getEnclosingRect();

  if (base_site->hasRowPattern()) {
    logger_->error(
        IFP,
        1000,
        "Hybrid rows not yet supported with polygon-aware generation.");
  }

  if (core_bbox.xMin() >= 0 && core_bbox.yMin() >= 0) {
    eval_upf(network_, logger_, block_);

    const uint32_t site_dx = base_site->getWidth();
    const uint32_t site_dy = base_site->getHeight();

    // Snap core bounding box to site grid
    const int clx = divCeil(core_bbox.xMin(), site_dx) * site_dx;
    const int cly = divCeil(core_bbox.yMin(), site_dy) * site_dy;
    const int cux = core_bbox.xMax();
    const int cuy = core_bbox.yMax();

    if (clx != core_bbox.xMin() || cly != core_bbox.yMin()) {
      const double dbu = block_->getDbUnitsPerMicron();
      logger_->warn(
          IFP,
          1003,
          "Core polygon bounding box lower left ({:.3f}, {:.3f}) snapped to "
          "({:.3f}, {:.3f}).",
          core_bbox.xMin() / dbu,
          core_bbox.yMin() / dbu,
          clx / dbu,
          cly / dbu);
    }

    const odb::Rect snapped_bbox(clx, cly, cux, cuy);

    // For each site type, create polygon-aware rows
    for (const auto& [name, site] : sites_by_name) {
      if (site->getHeight() % base_site->getHeight() != 0) {
        logger_->error(
            IFP,
            1001,
            "Site {} height {}um is not a multiple of site {} height {}um.",
            site->getName(),
            block_->dbuToMicrons(site->getHeight()),
            base_site->getName(),
            block_->dbuToMicrons(base_site->getHeight()));
      }
      makeUniformRowsPolygon(
          site, core_polygon, snapped_bbox, row_parity, flipped_sites);
    }

    updateVoltageDomain(clx, cly, cux, cuy, gap);
  }

  // Handle blockages as usual
  std::vector<dbBox*> blockage_bboxes;
  for (auto blockage : block_->getBlockages()) {
    blockage_bboxes.push_back(blockage->getBBox());
  }

  odb::cutRows(block_,
               /* min_row_width */ 0,
               blockage_bboxes,
               /* halo_x */ 0,
               /* halo_y */ 0,
               logger_);
}

std::vector<odb::Rect> InitFloorplan::intersectRowWithPolygon(
    const odb::Rect& row,
    const odb::Polygon& polygon)
{
  std::vector<odb::Rect> result;
  auto polygon_points = polygon.getPoints();

  // Simple scanline intersection approach - more robust for inflection points
  const int row_y = row.yMin();
  const int row_height = row.dy();

  // Find all intersections of polygon edges with the row's horizontal strip
  std::vector<int> intersections;

  for (size_t i = 0; i < polygon_points.size(); ++i) {
    const odb::Point& p1 = polygon_points[i];
    const odb::Point& p2 = polygon_points[(i + 1) % polygon_points.size()];

    const int y1 = p1.y();
    const int y2 = p2.y();
    const int x1 = p1.x();
    const int x2 = p2.x();

    // Skip horizontal edges and ensure no division by zero
    if (y1 == y2) {
      continue;
    }

    // Check if edge crosses the scanline at row_y
    if ((y1 <= row_y && y2 > row_y) || (y1 > row_y && y2 <= row_y)) {
      // Calculate intersection point with proper precision handling
      const double x_intersect_d
          = x1 + static_cast<double>(x2 - x1) * (row_y - y1) / (y2 - y1);
      const int x_intersect = static_cast<int>(std::round(x_intersect_d));
      intersections.push_back(x_intersect);
    }
  }

  // Sort intersections by x-coordinate
  std::ranges::sort(intersections);

  // Create segments from pairs of intersections (polygon uses even-odd rule)
  for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
    const int x_start = intersections[i];
    const int x_end = intersections[i + 1];

    if (x_end > x_start) {
      // Clip to row bounds
      const int clipped_x1 = std::max(x_start, row.xMin());
      const int clipped_x2 = std::min(x_end, row.xMax());

      if (clipped_x2 > clipped_x1) {
        result.emplace_back(clipped_x1, row_y, clipped_x2, row_y + row_height);
      }
    }
  }

  return result;
}

void InitFloorplan::makeUniformRowsPolygon(
    odb::dbSite* site,
    const odb::Polygon& core_polygon,
    const odb::Rect& core_bbox,
    RowParity row_parity,
    const std::set<odb::dbSite*>& flipped_sites)
{
  const uint32_t site_dx = site->getWidth();
  const uint32_t site_dy = site->getHeight();
  const int core_dy = core_bbox.dy();

  // Calculate number of rows
  int total_rows_y = core_dy / site_dy;
  bool flip = flipped_sites.find(site) != flipped_sites.end();

  // Apply row parity constraints
  switch (row_parity) {
    case RowParity::NONE:
      break;
    case RowParity::EVEN:
      total_rows_y = (total_rows_y / 2) * 2;
      break;
    case RowParity::ODD:
      if (total_rows_y > 0) {
        total_rows_y
            = (total_rows_y % 2 == 0) ? total_rows_y - 1 : total_rows_y;
      } else {
        total_rows_y = 0;
      }
      break;
  }

  int rows_created = 0;
  int y = core_bbox.yMin();

  // Create rows, clipping each one to the polygon
  for (int row_idx = 0; row_idx < total_rows_y; row_idx++) {
    // Create a row rectangle for this row
    odb::Rect row_rect(core_bbox.xMin(), y, core_bbox.xMax(), y + site_dy);

    // Intersect this row with the polygon
    std::vector<odb::Rect> row_segments
        = intersectRowWithPolygon(row_rect, core_polygon);

    // Create a row for each segment that's wide enough
    for (const auto& segment : row_segments) {
      int seg_width = segment.dx();
      int seg_sites = seg_width / site_dx;

      // Only create row if it has at least one site
      if (seg_sites > 0) {
        int seg_left = segment.xMin();

        // Snap to site grid
        int snapped_left = (seg_left / site_dx) * site_dx;
        if (snapped_left < seg_left) {
          snapped_left += site_dx;
          seg_sites = (seg_width - (snapped_left - seg_left)) / site_dx;
        }

        if (seg_sites > 0) {
          dbOrientType orient = ((row_idx + flip) % 2 == 0) ? dbOrientType::R0
                                                            : dbOrientType::MX;
          string row_name = "ROW_" + std::to_string(block_->getRows().size());

          dbRow::create(block_,
                        row_name.c_str(),
                        site,
                        snapped_left,
                        y,
                        orient,
                        dbRowDir::HORIZONTAL,
                        seg_sites,
                        site_dx);
          rows_created++;
        }
      }
    }

    y += site_dy;
  }

  block_->setCoreArea(block_->computeCoreArea());

  logger_->info(IFP,
                1002,
                "Added {} polygon-aware rows for site {}.",
                rows_created,
                site->getName());
}

void InitFloorplan::reportAreas()
{
  odb::Rect die = block_->getDieArea();
  odb::Rect core = block_->getCoreArea();
  logger_->info(IFP,
                100,
                "{:10} ( {:6.3f} {:6.3f} ) ( {:6.3f} {:6.3f} ) um",
                "Die BBox:",
                block_->dbuToMicrons(die.xMin()),
                block_->dbuToMicrons(die.yMin()),
                block_->dbuToMicrons(die.xMax()),
                block_->dbuToMicrons(die.yMax()));
  logger_->info(IFP,
                101,
                "{:10} ( {:6.3f} {:6.3f} ) ( {:6.3f} {:6.3f} ) um",
                "Core BBox:",
                block_->dbuToMicrons(core.xMin()),
                block_->dbuToMicrons(core.yMin()),
                block_->dbuToMicrons(core.xMax()),
                block_->dbuToMicrons(core.yMax()));
  logger_->info(IFP,
                102,
                "{:27} {:15.3f} um^2",
                "Core area:",
                block_->dbuAreaToMicrons(core.area()));
  int64_t design_area = static_cast<int64_t>(designArea());
  logger_->info(IFP,
                103,
                "{:27} {:15.3f} um^2",
                "Total instances area:",
                block_->dbuAreaToMicrons(design_area));
  double core_area_um = block_->dbuAreaToMicrons(core.area());
  if (core_area_um > 0) {
    logger_->info(IFP,
                  104,
                  "{:27} {:15.3f}",
                  "Effective utilization:",
                  block_->dbuAreaToMicrons(design_area) / core_area_um);
  }
  logger_->info(IFP,
                105,
                "{:27} {:15d}",
                "Number of instances:",
                block_->getInsts().size());
}

}  // namespace ifp
