/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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

#include "ifp/InitFloorplan.hh"

#include <cmath>
#include <fstream>
#include <iostream>
#include <set>

#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/util.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "sta/StringUtil.hh"
#include "sta/Vector.hh"
#include "upf/upf.h"
#include "utl/Logger.h"
#include "utl/validation.h"

namespace ifp {

using std::abs;
using std::ceil;
using std::map;
using std::round;
using std::set;
using std::string;

using sta::dbNetwork;
using sta::stdstrPrint;
using sta::stringEqual;
using sta::StringVector;
using sta::Vector;

using utl::IFP;
using utl::Logger;

using odb::dbBlock;
using odb::dbBox;
using odb::dbBPin;
using odb::dbBTerm;
using odb::dbChip;
using odb::dbDatabase;
using odb::dbGroup;
using odb::dbGroupType;
using odb::dbInst;
using odb::dbLib;
using odb::dbMaster;
using odb::dbOrientType;
using odb::dbPlacementStatus;
using odb::dbRegion;
using odb::dbRow;
using odb::dbRowDir;
using odb::dbSet;
using odb::dbSite;
using odb::dbTech;
using odb::dbTechLayer;
using odb::dbTechLayerDir;
using odb::dbTechLayerType;
using odb::dbTrackGrid;
using odb::dbTransform;
using odb::Rect;

using upf::eval_upf;

InitFloorplan::InitFloorplan(dbBlock* block,
                             Logger* logger,
                             sta::dbNetwork* network)
    : block_(block), logger_(logger), network_(network)
{
}

void InitFloorplan::initFloorplan(double utilization,
                                  double aspect_ratio,
                                  int core_space_bottom,
                                  int core_space_top,
                                  int core_space_left,
                                  int core_space_right,
                                  const std::vector<odb::dbSite*>& extra_sites)
{
  utl::Validator v(logger_, IFP);
  v.check_percentage("utilization", utilization, 12);
  v.check_non_negative("core_space_bottom", core_space_bottom, 32);
  v.check_non_negative("core_space_top", core_space_top, 33);
  v.check_non_negative("core_space_left", core_space_left, 34);
  v.check_non_negative("core_space_right", core_space_right, 35);
  v.check_non_negative("aspect_ratio", aspect_ratio, 36);

  utilization /= 100;
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
  initFloorplan({die_lx, die_ly, die_ux, die_uy},
                {core_lx, core_ly, core_ux, core_uy},
                extra_sites);
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

static int divCeil(int dividend, int divisor)
{
  return ceil(static_cast<double>(dividend) / divisor);
}

void InitFloorplan::initFloorplan(const odb::Rect& die,
                                  const odb::Rect& core,
                                  const std::vector<odb::dbSite*>& extra_sites)
{
  Rect die_area(snapToMfgGrid(die.xMin()),
                snapToMfgGrid(die.yMin()),
                snapToMfgGrid(die.xMax()),
                snapToMfgGrid(die.yMax()));
  block_->setDieArea(die_area);
  std::set<odb::dbSite*> sites = getSites();
  sites.insert(extra_sites.begin(), extra_sites.end());

  // Handle duplicated sites
  std::map<std::string, odb::dbSite*> sites_by_name;
  for (auto* site : sites) {
    sites_by_name[site->getName()] = site;
  }

  if (sites.empty()) {
    logger_->error(IFP, 42, "No sites found.");
  }
  // get min site height using std::min_element
  const int min_site_height
      = (*std::min_element(sites.begin(),
                           sites.end(),
                           [](dbSite* a, dbSite* b) {
                             return a->getHeight() < b->getHeight();
                           }))
            ->getHeight();

  if (core.xMin() >= 0 && core.yMin() >= 0) {
    int row_index = 0;
    eval_upf(network_, logger_, block_);
    for (const auto& [site_name, site] : sites_by_name) {
      int x_height_site = site->getHeight() / min_site_height;
      auto rows = block_->getRows();
      for (dbSet<dbRow>::iterator row_itr = rows.begin();
           row_itr != rows.end();) {
        if (site != row_itr->getSite() || row_itr->getSite()->isHybrid()) {
          row_itr++;
        } else {
          row_itr = dbRow::destroy(row_itr);
        }
      }
      uint site_dx = site->getWidth();
      uint site_dy = site->getHeight();
      // core lower left corner to multiple of site dx/dy.
      int clx = divCeil(core.xMin(), site_dx) * site_dx;
      int cly = divCeil(core.yMin(), site_dy) * site_dy;
      if (clx != core.xMin() || cly != core.yMin()) {
        dbTech* tech = block_->getDataBase()->getTech();
        const double dbu = tech->getDbUnitsPerMicron();
        logger_->warn(IFP,
                      28,
                      "Core area lower left ({:.3f}, {:.3f}) snapped to "
                      "({:.3f}, {:.3f}).",
                      core.xMin() / dbu,
                      core.yMin() / dbu,
                      clx / dbu,
                      cly / dbu);
      }
      int cux = core.xMax();
      int cuy = core.yMax();
      int rows_placed = 0;
      if (!site->isHybrid()) {
        rows_placed
            += makeRows(site, clx, cly, cux, cuy, x_height_site, row_index);
        row_index += rows_placed;
      } else if (site->isHybridParent()) {
        rows_placed = makeHybridRows(
            site, odb::Point(clx, cly), odb::Point(cux, cuy), row_index);
        row_index += rows_placed;
      }

      updateVoltageDomain(clx, cly, cux, cuy);
    }
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
}

// this function is used to create regions ( split overlapped rows and create
// new ones )
void InitFloorplan::updateVoltageDomain(int core_lx,
                                        int core_ly,
                                        int core_ux,
                                        int core_uy)
{
  // The unit for power_domain_y_space is the site height. The real space is
  // power_domain_y_space * site_dy
  static constexpr int power_domain_y_space = 6;

  // checks if a group is defined as a voltage domain, if so it creates a region
  for (dbGroup* group : block_->getGroups()) {
    if (group->getType() == dbGroupType::VOLTAGE_DOMAIN
        || group->getType() == dbGroupType::POWER_DOMAIN) {
      dbRegion* domain_region = group->getRegion();
      int domain_xMin = std::numeric_limits<int>::max();
      int domain_yMin = std::numeric_limits<int>::max();
      int domain_xMax = std::numeric_limits<int>::min();
      int domain_yMax = std::numeric_limits<int>::min();
      for (auto boundary : domain_region->getBoundaries()) {
        domain_xMin = std::min(domain_xMin, boundary->xMin());
        domain_yMin = std::min(domain_yMin, boundary->yMin());
        domain_xMax = std::max(domain_xMax, boundary->xMax());
        domain_yMax = std::max(domain_yMax, boundary->yMax());
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

      std::vector<dbRow*>::iterator row_itr = rows.begin();
      for (int row_processed = 0; row_processed < total_row_count;
           row_processed++) {
        dbRow* row = *row_itr;
        Rect row_bbox = row->getBBox();
        int row_yMin = row_bbox.yMin();
        int row_yMax = row_bbox.yMax();
        auto site = row->getSite();

        int site_dy = site->getHeight();
        int site_dx = site->getWidth();

        // snap inward to site grid
        domain_xMin = odb::makeSiteLoc(domain_xMin, site_dx, false, 0);
        domain_xMax = odb::makeSiteLoc(domain_xMax, site_dx, true, 0);

        // check if the rows overlapped with the area of a defined voltage
        // domains + margin
        if (row_yMax + power_domain_y_space * site_dy <= domain_yMin
            || row_yMin >= domain_yMax + power_domain_y_space * site_dy) {
          row_itr++;
        } else {
          string row_name = row->getName();
          dbOrientType orient = row->getOrient();
          dbRow::destroy(row);
          row_itr++;

          // lcr stands for left core row
          int lcr_xMax = domain_xMin - power_domain_y_space * site_dy;
          // in case there is at least one valid site width on the left, create
          // left core rows
          if (lcr_xMax > core_lx + static_cast<int>(site_dx)) {
            string lcr_name = row_name + "_1";
            // warning message since tap cells might not be inserted
            if (lcr_xMax < core_lx + 10 * site_dx) {
              logger_->warn(IFP,
                            26,
                            "left core row: {} has less than 10 sites",
                            lcr_name);
            }
            int lcr_sites = (lcr_xMax - core_lx) / site_dx;
            dbRow::create(block_,
                          lcr_name.c_str(),
                          site,
                          core_lx,
                          row_yMin,
                          orient,
                          dbRowDir::HORIZONTAL,
                          lcr_sites,
                          site_dx);
          }

          // rcr stands for right core row
          // rcr_dx_site_number is the max number of site_dx that is less than
          // power_domain_y_space * site_dy. This helps align the rcr_xMin on
          // the multiple of site_dx.
          int rcr_dx_site_number = (power_domain_y_space * site_dy) / site_dx;
          int rcr_xMin = domain_xMax + rcr_dx_site_number * site_dx;
          // snap to the site grid rightward
          rcr_xMin = odb::makeSiteLoc(rcr_xMin, site_dx, false, 0);

          // in case there is at least one valid site width on the right, create
          // right core rows
          if (rcr_xMin + static_cast<int>(site_dx) < core_ux) {
            string rcr_name = row_name + "_2";
            if (rcr_xMin + 10 * site_dx > core_ux) {
              logger_->warn(IFP,
                            27,
                            "right core row: {} has less than 10 sites",
                            rcr_name);
            }
            int rcr_sites = (core_ux - rcr_xMin) / site_dx;
            dbRow::create(block_,
                          rcr_name.c_str(),
                          site,
                          rcr_xMin,
                          row_yMin,
                          orient,
                          dbRowDir::HORIZONTAL,
                          rcr_sites,
                          site_dx);
          }

          int domain_row_sites = (domain_xMax - domain_xMin) / site_dx;
          // create domain rows if current iterations are not in margin area
          if (row_yMin >= domain_yMin && row_yMax <= domain_yMax) {
            string domain_row_name = row_name + "_" + domain_name;
            dbRow::create(block_,
                          domain_row_name.c_str(),
                          site,
                          domain_xMin,
                          row_yMin,
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

std::set<dbSite*> InitFloorplan::getSites() const
{
  std::set<dbSite*> sites;

  // loop over all instantiated cells in the block
  for (dbInst* inst : block_->getInsts()) {
    dbMaster* master = inst->getMaster();

    auto site = master->getSite();
    // if site is null, and the core is auto placeable, then warn the user and
    // skip that cell
    if (!master->isCoreAutoPlaceable() || master->isBlock()) {
      continue;
    }
    if (site == nullptr) {
      if (master->isCoreAutoPlaceable()) {
        logger_->warn(IFP,
                      43,
                      "No site found for instance {} in block {}.",
                      inst->getName(),
                      block_->getName());
      }
      continue;
    }
    sites.insert(site);
  }
  if (sites.empty()) {
    return sites;
  }

  const int min_site_height
      = (*std::min_element(sites.begin(),
                           sites.end(),
                           [](dbSite* a, dbSite* b) {
                             return a->getHeight() < b->getHeight();
                           }))
            ->getHeight();

  for (auto site : sites) {
    // check if the site height is a multiple of the smallest site height
    if (site->getHeight() % min_site_height != 0) {
      if (site->isHybrid()) {
        logger_->warn(IFP,
                      46,
                      "Site {} is a hybrid row, but "
                      "its height {} is not a "
                      "multiple of the smallest site height {}.",
                      site->getName(),
                      site->getHeight(),
                      min_site_height);
        continue;
      }
      logger_->error(IFP,
                     40,
                     "Invalid height for site {} detected. The height value of "
                     "{} is not a multiple of the smallest site height {}.",
                     site->getName(),
                     site->getHeight(),
                     min_site_height);
    }
  }
  return sites;
}

// Create the rows for the core area and returns the number of rows it created
int InitFloorplan::makeRows(dbSite* site,
                            int core_lx,
                            int core_ly,
                            int core_ux,
                            int core_uy,
                            int factor,
                            int row_index)
{
  int core_dx = abs(core_ux - core_lx);
  int core_dy = abs(core_uy - core_ly);
  uint site_dx = site->getWidth();
  uint site_dy = site->getHeight();
  int rows_x = core_dx / site_dx;
  int rows_y = core_dy / site_dy;

  int y = core_ly;
  for (int row = 0; row < rows_y; row++) {
    dbOrientType orient = (factor % 2 == 0 or row % 2 == 0)
                              ? dbOrientType::R0   // N
                              : dbOrientType::MX;  // FS
    string row_name = stdstrPrint("ROW_%d", row_index + row);
    dbRow::create(block_,
                  row_name.c_str(),
                  site,
                  core_lx,
                  y,
                  orient,
                  dbRowDir::HORIZONTAL,
                  rows_x,
                  site_dx);
    y += site_dy;
  }
  logger_->info(IFP,
                1,
                "Added {} rows of {} site {} with height {}.",
                rows_y,
                rows_x,
                site->getName(),
                factor);  // using the factor instead of the cell height for
                          // reporting
  return rows_y;
}

int InitFloorplan::makeHybridRows(dbSite* parent_hybrid_site,
                                  const odb::Point& core_l,
                                  const odb::Point& core_u,
                                  int row_index)
{
  auto row_pattern = parent_hybrid_site->getRowPattern();
  auto hybrid_site = row_pattern[0].first;
  int site_width = hybrid_site->getWidth();
  for (const auto& [site, orient] : row_pattern) {
    debugPrint(logger_,
               utl::IFP,
               "hybrid",
               1,
               "Site {} with width {} and height {}",
               site->getName(),
               site->getWidth(),
               site->getHeight());
    if (site->getWidth() != site_width) {
      logger_->error(
          IFP,
          47,
          "The width of all hybrid sites "
          "should be the same. Site {}'s width is {} and site {}'s width is {}",
          hybrid_site->getName(),
          site_width,
          site->getName(),
          site->getWidth());
    }
  }

  int core_height = abs(core_u.y() - core_l.y());
  int core_width = abs(core_u.x() - core_l.x());
  int rows_x = core_width / site_width;
  int row = 0;
  std::vector<std::vector<dbSite*>> patterns_to_construct;
  generateContiguousHybridRows(
      parent_hybrid_site, row_pattern, patterns_to_construct);

  for (const auto& pattern : patterns_to_construct) {
    std::string pattern_sites = "[";
    if (pattern.empty()) {
      pattern_sites = "[]";
    }
    for (int i = 0; i < pattern.size(); i++) {
      pattern_sites += pattern[i]->getName() + ",]"[i == pattern.size() - 1];
    }
    debugPrint(logger_,
               utl::IFP,
               "hybrid",
               1,
               "Pattern {} with {} sites",
               pattern_sites,
               pattern.size());
    int y = core_l.y(), pattern_iterator = 0;

    while (y < core_height) {
      // TODO: should I get orient from hybrid_sites.second?
      dbOrientType orient = (pattern_iterator % 2 == 0)
                                ? dbOrientType::R0   // N
                                : dbOrientType::MX;  // FS
      string row_name = stdstrPrint("ROW_%d", row_index + row);
      dbSite* site_it = nullptr;
      if (pattern.empty())
        break;
      if (pattern.size() == 1) {
        site_it = parent_hybrid_site;
      } else {
        site_it = pattern[pattern_iterator % pattern.size()];
      }
      dbRow::create(block_,
                    (row_name).c_str(),
                    site_it,
                    core_l.x(),
                    y,
                    orient,
                    dbRowDir::HORIZONTAL,
                    rows_x,
                    site_width);
      y += site_it->getHeight();
      ++pattern_iterator;
      ++row;
    }
  }
  return row;
}

void InitFloorplan::generateContiguousHybridRows(
    dbSite* parent_hybrid_site,
    const std::vector<std::pair<dbSite*, dbOrientType>>& row_pattern,
    std::vector<std::vector<dbSite*>>& output_patterns_list)
{
  output_patterns_list.clear();
  output_patterns_list.resize(2);
  std::set<int>
      pattern_sites_ids;  // order doesn't matter because for HybridAG, the
                          // pattern is A then G, and for HybridGA it is G then
                          // A. yet both share the same grid.
  for (auto& [site, orient] : row_pattern) {
    pattern_sites_ids.insert(site->getId());
  }
  if (constructed_patterns.find(pattern_sites_ids)
      == constructed_patterns.end()) {  // it was not constructed before
    for (auto& [site, orient] : row_pattern) {
      output_patterns_list[0].push_back(site);
    }
    constructed_patterns.insert(pattern_sites_ids);
  }

  std::set<int> parent_site = {(int) parent_hybrid_site->getId()};
  if (constructed_patterns.find(parent_site)
      == constructed_patterns.end()) {  // it was not constructed before
    constructed_patterns.insert(parent_site);
    output_patterns_list[1].push_back(parent_hybrid_site);
  }
}

dbSite* InitFloorplan::findSite(const char* site_name)
{
  for (dbLib* lib : block_->getDataBase()->getLibs()) {
    dbSite* site = lib->findSite(site_name);
    if (site)
      return site;
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
                   39,
                   "Liberty cell or port {}/{} not found.",
                   master->getName(),
                   tie_term->getName());
  }
  auto func_operation = lib_port->function()->op();
  const bool is_zero = func_operation == sta::FuncExpr::op_zero;
  const bool is_one = func_operation == sta::FuncExpr::op_one;

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
        makeTracks(layer,
                   layer->getOffsetX(),
                   layer->getPitchX(),
                   layer->getOffsetY(),
                   layer->getPitchY());
      }
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
  v.check_non_null("layer", layer, 38);
  v.check_non_negative("x_offset", x_offset, 39);
  v.check_positive("x_pitch", x_pitch, 40);
  v.check_non_negative("y_offset", y_offset, 41);
  v.check_positive("y_pitch", y_pitch, 42);

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

  auto x_track_count = int((die_area.dx() - x_offset) / x_pitch) + 1;
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

  auto y_track_count = int((die_area.dy() - y_offset) / y_pitch) + 1;
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

  auto y_track_count
      = int((cell_row_height - 2 * first_last_pitch) / y_pitch) + 1;
  int origin_y = die_area.yMin() + first_last_pitch;
  for (int i = 0; i < y_track_count; i++) {
    makeTracks(layer, x_offset, x_pitch, origin_y, cell_row_height);
    origin_y += y_pitch;
  }
  origin_y += first_last_pitch - y_pitch;
  makeTracks(layer, x_offset, x_pitch, origin_y, cell_row_height);
}

}  // namespace ifp
