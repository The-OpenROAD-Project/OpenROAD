// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "odb/util.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <numeric>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbCCSegSet.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace odb {

using std::string;
using std::vector;

static void buildRow(dbBlock* block,
                     const string& name,
                     dbSite* site,
                     int start_x,
                     int end_x,
                     int y,
                     dbOrientType& orient,
                     dbRowDir& direction,
                     int min_row_width)
{
  const int site_width = site->getWidth();
  const int new_row_num_sites = (end_x - start_x) / site_width;
  const int new_row_width = new_row_num_sites * site_width;

  if (new_row_num_sites > 0 && new_row_width >= min_row_width) {
    dbRow::create(block,
                  name.c_str(),
                  site,
                  start_x,
                  y,
                  orient,
                  direction,
                  new_row_num_sites,
                  site_width);
  }
}

static void cutRow(dbBlock* block,
                   dbRow* row,
                   vector<dbBox*>& row_blockages,
                   int min_row_width,
                   int halo_x,
                   int halo_y)
{
  string row_name = row->getName();
  Rect row_bb = row->getBBox();

  dbSite* row_site = row->getSite();
  const int site_width = row_site->getWidth();
  dbOrientType orient = row->getOrient();
  dbRowDir direction = row->getDirection();

  const int curr_min_row_width = min_row_width + 2 * site_width;

  vector<dbBox*> row_blockage_bboxs = row_blockages;
  vector<std::pair<int, int>> row_blockage_xs;
  row_blockage_xs.reserve(row_blockages.size());
  for (dbBox* row_blockage_bbox : row_blockages) {
    if (row_blockage_bbox->getOwnerType() == odb::dbBoxOwner::INST) {
      odb::dbInst* inst
          = static_cast<odb::dbInst*>(row_blockage_bbox->getBoxOwner());
      odb::dbBox* halo = inst->getHalo();
      if (halo != nullptr && !halo->isSoft()) {
        Rect halo_rect = inst->getTransformedHalo();
        row_blockage_xs.emplace_back(
            row_blockage_bbox->xMin() - halo_rect.xMin(),
            row_blockage_bbox->xMax() + halo_rect.xMax());
      } else {
        row_blockage_xs.emplace_back(row_blockage_bbox->xMin() - halo_x,
                                     row_blockage_bbox->xMax() + halo_x);
      }
    } else {
      row_blockage_xs.emplace_back(row_blockage_bbox->xMin() - halo_x,
                                   row_blockage_bbox->xMax() + halo_x);
    }
  }

  std::ranges::sort(row_blockage_xs);

  // Compute segment boundaries between blockages.
  std::vector<std::pair<int, int>> segments;
  int start_origin_x = row_bb.xMin();
  for (const auto& blockage : row_blockage_xs) {
    const int seg_end
        = makeSiteLoc(blockage.first, site_width, true, start_origin_x);
    segments.emplace_back(start_origin_x, seg_end);
    start_origin_x
        = makeSiteLoc(blockage.second, site_width, false, start_origin_x);
  }
  // Last segment: from after the last blockage to the row's right edge
  segments.emplace_back(start_origin_x, row_bb.xMax());

  // Fix steps between cut and uncut rows that are too small for endcap
  // corner cells.
  const int min_step_for_endcap = min_row_width / 2;
  if (min_step_for_endcap > 0) {
    /// @brief Returns true if a segment is wide enough to be created.
    auto isValid = [&](const std::pair<int, int>& seg) {
      const int width = (seg.second - seg.first) / site_width * site_width;
      return width >= curr_min_row_width;
    };

    // Left-side: push the first valid segment right if its left edge
    // creates a step too small for an endcap corner cell.
    auto seg = std::ranges::find_if(segments, isValid);
    if (seg != segments.end()) {
      const int left_step = seg->first - row_bb.xMin();
      if (left_step > 0 && left_step < min_step_for_endcap) {
        seg->first = makeSiteLoc(row_bb.xMin() + min_step_for_endcap,
                                 site_width,
                                 false,
                                 row_bb.xMin());
      }
    }

    // Right-side: pull the last valid segment left if its right edge
    // creates a step too small for an endcap corner cell.
    auto rseg = std::find_if(segments.rbegin(), segments.rend(), isValid);
    if (rseg != segments.rend()) {
      const int actual_end
          = rseg->first
            + (rseg->second - rseg->first) / site_width * site_width;
      const int right_step = row_bb.xMax() - actual_end;
      if (right_step > 0 && right_step < min_step_for_endcap) {
        rseg->second = makeSiteLoc(
            row_bb.xMax() - min_step_for_endcap, site_width, true, rseg->first);
      }
    }
  }

  // Create rows from the computed (and possibly adjusted) segments
  int row_sub_idx = 1;
  for (const auto& [seg_start, seg_end] : segments) {
    buildRow(block,
             row_name + "_" + std::to_string(row_sub_idx),
             row_site,
             seg_start,
             seg_end,
             row_bb.yMin(),
             orient,
             direction,
             curr_min_row_width);
    row_sub_idx++;
  }

  // Remove current row
  dbRow::destroy(row);
}

static bool overlaps(dbBox* blockage, dbRow* row, int halo_x, int halo_y)
{
  const Rect rowBB = row->getBBox();

  odb::dbBox* halo = nullptr;
  odb::Rect transformed_halo;
  if (blockage->getOwnerType() == odb::dbBoxOwner::INST) {
    halo = static_cast<odb::dbInst*>(blockage->getBoxOwner())->getHalo();
    transformed_halo = static_cast<odb::dbInst*>(blockage->getBoxOwner())
                           ->getTransformedHalo();
  }

  const bool use_inst_halo = halo != nullptr && !halo->isSoft();

  // Check if Y has overlap first since rows are long and skinny
  const int blockage_lly
      = blockage->yMin() - (use_inst_halo ? transformed_halo.yMin() : halo_y);
  const int blockage_ury
      = blockage->yMax() + (use_inst_halo ? transformed_halo.yMax() : halo_y);
  const int row_lly = rowBB.yMin();
  const int row_ury = rowBB.yMax();

  if (blockage_lly >= row_ury || row_lly >= blockage_ury) {
    return false;
  }

  const int blockage_llx
      = blockage->xMin() - (use_inst_halo ? transformed_halo.xMin() : halo_x);
  const int blockage_urx
      = blockage->xMax() + (use_inst_halo ? transformed_halo.xMax() : halo_x);
  const int row_llx = rowBB.xMin();
  const int row_urx = rowBB.xMax();

  return blockage_llx < row_urx && row_llx < blockage_urx;
}

int makeSiteLoc(int x, double site_width, bool at_left_from_macro, int offset)
{
  double site_x = (x - offset) / site_width;
  int site_x1 = at_left_from_macro ? floor(site_x) : ceil(site_x);
  return site_x1 * site_width + offset;
}

template <typename T>
bool hasOverflow(T a, T b)
{
  return (b > 0 && a > std::numeric_limits<T>::max() - b)
         || (b < 0 && a < std::numeric_limits<T>::lowest() - b);
}

void cutRows(dbBlock* block,
             const int min_row_width,
             const vector<dbBox*>& blockages,
             int halo_x,
             int halo_y,
             utl::Logger* logger)
{
  if (blockages.empty()) {
    return;
  }
  auto rows = block->getRows();
  const int initial_rows_count = rows.size();
  const std::int64_t initial_sites_count
      = std::accumulate(rows.begin(),
                        rows.end(),
                        (std::int64_t) 0,
                        [&](std::int64_t sum, dbRow* row) {
                          return sum + (std::int64_t) row->getSiteCount();
                        });

  std::map<dbRow*, int> placed_row_insts;
  for (dbInst* inst : block->getInsts()) {
    if (!inst->isFixed()) {
      continue;
    }
    if (inst->getMaster()->isCoreAutoPlaceable()
        && !inst->getMaster()->isBlock()) {
      const Rect inst_bbox = inst->getBBox()->getBox();
      for (dbRow* row : block->getRows()) {
        const Rect row_bbox = row->getBBox();
        if (row_bbox.contains(inst_bbox)) {
          placed_row_insts[row]++;
        }
      }
    }
  }

  // Gather rows needing to be cut up front
  for (dbRow* row : block->getRows()) {
    std::vector<dbBox*> row_blockages;
    for (dbBox* blockage : blockages) {
      if (overlaps(blockage, row, halo_x, halo_y)) {
        row_blockages.push_back(blockage);
      }
    }
    // Cut row around macros
    if (!row_blockages.empty()) {
      if (placed_row_insts.find(row) != placed_row_insts.end()) {
        logger->warn(utl::ODB,
                     386,
                     "{} contains {} placed instances and will not be cut.",
                     row->getName(),
                     placed_row_insts[row]);
      } else {
        cutRow(block, row, row_blockages, min_row_width, halo_x, halo_y);
      }
    }
  }

  const std::int64_t final_sites_count
      = std::accumulate(rows.begin(),
                        rows.end(),
                        (std::int64_t) 0,
                        [&](std::int64_t sum, dbRow* row) {
                          return sum + (std::int64_t) row->getSiteCount();
                        });

  logger->info(utl::ODB,
               303,
               "The initial {} rows ({} sites) were cut with {} shapes for a "
               "total of {} rows ({} sites).",
               initial_rows_count,
               initial_sites_count,
               blockages.size(),
               block->getRows().size(),
               final_sites_count);
}

std::string generateMacroPlacementString(dbBlock* block)
{
  std::string macro_placement;

  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->isBlock()) {
      macro_placement += fmt::format(
          "place_macro -macro_name {{{}}} -location {{{} {}}} -orientation "
          "{}\n",
          inst->getName(),
          block->dbuToMicrons(inst->getLocation().x()),
          block->dbuToMicrons(inst->getLocation().y()),
          inst->getOrient().getString());
    }
  }

  return macro_placement;
}

void set_bterm_top_layer_grid(dbBlock* block,
                              dbTechLayer* layer,
                              int x_step,
                              int y_step,
                              Rect region,
                              int width,
                              int height,
                              int keepout)
{
  Polygon polygon_region(region);
  dbBlock::dbBTermTopLayerGrid top_layer_grid
      = {layer, x_step, y_step, polygon_region, width, height, keepout};
  block->setBTermTopLayerGrid(top_layer_grid);
}

bool dbHasCoreRows(dbDatabase* db)
{
  if (!db->getChip() || !db->getChip()->getBlock()) {
    return false;
  }

  for (odb::dbRow* row : db->getChip()->getBlock()->getRows()) {
    if (row->getSite()->getClass() != odb::dbSiteClass::PAD) {
      return true;
    }
  }

  return false;
}

bool hasOneSiteMaster(dbDatabase* db)
{
  for (dbLib* lib : db->getLibs()) {
    for (dbMaster* master : lib->getMasters()) {
      if (master->isBlock() || master->isPad() || master->isCover()) {
        continue;
      }

      // Ignore IO corner cells
      dbMasterType type = master->getType();
      if (type == dbMasterType::ENDCAP_TOPLEFT
          || type == dbMasterType::ENDCAP_TOPRIGHT
          || type == dbMasterType::ENDCAP_BOTTOMLEFT
          || type == dbMasterType::ENDCAP_BOTTOMRIGHT) {
        continue;
      }

      dbSite* site = master->getSite();
      if (site == nullptr) {
        continue;
      }

      if (site->getClass() == dbSiteClass::PAD) {
        continue;
      }

      if (site->getWidth() == master->getWidth()) {
        return true;
      }
    }
  }

  return false;
}

int64_t WireLengthEvaluator::hpwl() const
{
  int64_t hpwl_sum = 0;
  for (dbNet* net : block_->getNets()) {
    int64_t net_hpwl_x, net_hpwl_y;
    hpwl_sum += hpwl(net, net_hpwl_x, net_hpwl_y);
  }
  return hpwl_sum;
}

int64_t WireLengthEvaluator::hpwl(int64_t& hpwl_x, int64_t& hpwl_y) const
{
  int64_t hpwl_sum = 0;
  for (dbNet* net : block_->getNets()) {
    int64_t net_hpwl_x = 0;
    int64_t net_hpwl_y = 0;
    hpwl_sum += hpwl(net, net_hpwl_x, net_hpwl_y);
    hpwl_x += net_hpwl_x;
    hpwl_y += net_hpwl_y;
  }
  return hpwl_sum;
}

int64_t WireLengthEvaluator::hpwl(dbNet* net,
                                  int64_t& hpwl_x,
                                  int64_t& hpwl_y) const
{
  if (net->getSigType().isSupply() || net->isSpecial()) {
    return 0;
  }

  Rect bbox = net->getTermBBox();
  if (bbox.isInverted()) {
    hpwl_x = 0;
    hpwl_y = 0;
    return 0;
  }

  hpwl_x = bbox.dx();
  hpwl_y = bbox.dy();

  return hpwl_x + hpwl_y;
}

void WireLengthEvaluator::reportEachNetHpwl(utl::Logger* logger) const
{
  for (dbNet* net : block_->getNets()) {
    int64_t tmp;
    logger->report("{} {}",
                   net->getConstName(),
                   block_->dbuToMicrons(hpwl(net, tmp, tmp)));
  }
}

void WireLengthEvaluator::reportHpwl(utl::Logger* logger) const
{
  logger->report("{}", block_->dbuToMicrons(hpwl()));
}

}  // namespace odb
