///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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

#include "odb/util.h"

#include <map>
#include <numeric>
#include <string>

#include "odb/db.h"
#include "odb/dbShape.h"
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
    row_blockage_xs.emplace_back(row_blockage_bbox->xMin(),
                                 row_blockage_bbox->xMax());
  }

  std::sort(row_blockage_xs.begin(), row_blockage_xs.end());

  int start_origin_x = row_bb.xMin();
  int start_origin_y = row_bb.yMin();
  int row_sub_idx = 1;
  for (std::pair<int, int> blockage : row_blockage_xs) {
    const int blockage_x0 = blockage.first;
    const int new_row_end_x
        = makeSiteLoc(blockage_x0 - halo_x, site_width, true, start_origin_x);
    buildRow(block,
             row_name + "_" + std::to_string(row_sub_idx),
             row_site,
             start_origin_x,
             new_row_end_x,
             start_origin_y,
             orient,
             direction,
             curr_min_row_width);
    row_sub_idx++;
    const int blockage_x1 = blockage.second;
    start_origin_x
        = makeSiteLoc(blockage_x1 + halo_x, site_width, false, start_origin_x);
  }
  // Make last row
  buildRow(block,
           row_name + "_" + std::to_string(row_sub_idx),
           row_site,
           start_origin_x,
           row_bb.xMax(),
           start_origin_y,
           orient,
           direction,
           curr_min_row_width);
  // Remove current row
  dbRow::destroy(row);
}

static bool overlaps(dbBox* blockage, dbRow* row, int halo_x, int halo_y)
{
  Rect rowBB = row->getBBox();

  // Check if Y has overlap first since rows are long and skinny
  const int blockage_lly = blockage->yMin() - halo_y;
  const int blockage_ury = blockage->yMax() + halo_y;
  const int row_lly = rowBB.yMin();
  const int row_ury = rowBB.yMax();

  if (blockage_lly >= row_ury || row_lly >= blockage_ury) {
    return false;
  }

  const int blockage_llx = blockage->xMin() - halo_x;
  const int blockage_urx = blockage->xMax() + halo_x;
  const int row_llx = rowBB.xMin();
  const int row_urx = rowBB.xMax();

  if (blockage_llx >= row_urx || row_llx >= blockage_urx) {
    return false;
  }

  return true;
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
  if ((b > 0 && a > std::numeric_limits<T>::max() - b)
      || (b < 0 && a < std::numeric_limits<T>::lowest() - b)) {
    return true;
  }
  return false;
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
          "place_macro -macro_name {} -location {{{} {}}} -orientation {}\n",
          inst->getName(),
          block->dbuToMicrons(inst->getLocation().x()),
          block->dbuToMicrons(inst->getLocation().y()),
          inst->getOrient().getString());
    }
  }

  return macro_placement;
}

int64_t WireLengthEvaluator::hpwl() const
{
  int64_t hpwl_sum = 0;
  for (dbNet* net : block_->getNets()) {
    hpwl_sum += hpwl(net);
  }
  return hpwl_sum;
}

int64_t WireLengthEvaluator::hpwl(dbNet* net) const
{
  if (net->getSigType().isSupply()) {
    return 0;
  }

  Rect bbox = net->getTermBBox();
  return bbox.dx() + bbox.dy();
}

}  // namespace odb
