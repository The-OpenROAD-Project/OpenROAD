// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include <algorithm>
#include <string>
#include <utility>

#include "dpl/Opendp.h"
#include "infrastructure/Coordinates.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Objects.h"
#include "infrastructure/network.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace dpl {

using std::to_string;

using utl::DPL;

using odb::dbMaster;
using odb::dbPlacementStatus;

using utl::format_as;  // NOLINT(misc-unused-using-decls)

static odb::dbTechLayer* getImplant(dbMaster* master)
{
  if (!master) {
    return nullptr;
  }

  for (auto obs : master->getObstructions()) {
    auto layer = obs->getTechLayer();
    if (layer->getType() == odb::dbTechLayerType::IMPLANT) {
      return layer;
    }
  }
  return nullptr;
}

Opendp::MasterByImplant Opendp::splitByImplant(
    const dbMasterSeq& filler_masters)
{
  MasterByImplant mapping;
  for (auto master : filler_masters) {
    mapping[getImplant(master)].emplace_back(master);
  }

  return mapping;
}

dbMasterSeq Opendp::filterFillerMasters(const dbMasterSeq& filler_masters) const
{
  // Remove fillers that cannot be used
  dbMasterSeq filtered_masters = filler_masters;

  if (logger_->debugCheck(DPL, "filler", 2)) {
    debugPrint(logger_,
               DPL,
               "filler",
               1,
               "Starting fillers: {}",
               filtered_masters.size()) for (auto* master : filtered_masters)
    {
      debugPrint(logger_, DPL, "filler", 2, "    {}", master->getName());
    }
  }

  // Remove fillers with PAD or BLOCK classes
  std::erase_if(filtered_masters, [](dbMaster* master) -> bool {
    return master->isPad() || master->isBlock();
  });

  if (logger_->debugCheck(DPL, "filler", 2)) {
    debugPrint(logger_,
               DPL,
               "filler",
               1,
               "Final filterered fillers: {}",
               filtered_masters.size()) for (auto* master : filtered_masters)
    {
      debugPrint(logger_, DPL, "filler", 2, "    {}", master->getName());
    }
  }

  return filtered_masters;
}

void Opendp::fillerPlacement(const dbMasterSeq& filler_masters,
                             const char* prefix,
                             bool verbose)
{
  if (network_->getNumCells() == 0) {
    importDb();
    adjustNodesOrient();
  }

  const auto filtered_masters = filterFillerMasters(filler_masters);

  auto filler_masters_by_implant = splitByImplant(filtered_masters);

  for (auto& [layer, masters] : filler_masters_by_implant) {
    std::ranges::sort(masters, [](dbMaster* master1, dbMaster* master2) {
      return master1->getWidth() > master2->getWidth();
    });
  }

  gap_fillers_.clear();
  filler_count_.clear();
  initGrid();
  setGridCells();

  for (GridY row{0}; row < grid_->getRowCount(); row++) {
    placeRowFillers(row, prefix, filler_masters_by_implant);
  }

  int filler_count = 0;
  int max_filler_master = 0;
  for (const auto& [master, count] : filler_count_) {
    filler_count += count;
    max_filler_master = std::max(max_filler_master, count);
  }
  logger_->info(DPL, 1, "Placed {} filler instances.", filler_count);

  if (verbose) {
    logger_->report("Filler usage:");
    int max_master_len = 0;
    for (const auto& [master, count] : filler_count_) {
      max_master_len = std::max(max_master_len,
                                static_cast<int>(master->getName().size()));
    }
    const int count_offset = fmt::format("{}", max_filler_master).size();
    for (const auto& [master, count] : filler_count_) {
      const int line_offset
          = count_offset + max_master_len - master->getName().size();
      logger_->report("  {}: {:>{}}", master->getName(), count, line_offset);
    }
  }
}

void Opendp::setGridCells()
{
  for (auto& cell : network_->getNodes()) {
    if (cell->getType() != Node::CELL) {
      continue;
    }
    grid_->visitCellPixels(*cell, false, [&](Pixel* pixel, bool padded) {
      setGridCell(*cell, pixel);
    });
  }
}

void Opendp::placeRowFillers(GridY row,
                             const std::string& prefix,
                             const MasterByImplant& filler_masters_by_implant)
{
  // DbuY row_height;
  GridX j{0};

  const DbuX site_width = grid_->getSiteWidth();
  GridX row_site_count = grid_->getRowSiteCount();
  while (j < row_site_count) {
    Pixel* pixel = grid_->gridPixel(j, row);
    if (pixel->cell || !pixel->is_valid) {
      ++j;
      continue;
    }
    // Select the site and orientation to fill this row with.  Use the shortest
    // site.
    auto [site, orient] = grid_->getShortestSite(j, row);
    GridX k = j;
    while (k < row_site_count && grid_->gridPixel(k, row)->cell == nullptr
           && grid_->gridPixel(k, row)->is_valid) {
      k++;
    }

    odb::dbTechLayer* implant = nullptr;
    if (j > 0) {
      auto pixel = grid_->gridPixel(j - 1, row);
      if (pixel->cell && pixel->cell->getDbInst()) {
        implant = getImplant(pixel->cell->getDbInst()->getMaster());
      }
    } else if (k < row_site_count) {
      auto pixel = grid_->gridPixel(k, row);
      if (pixel->cell && pixel->cell->getDbInst()) {
        implant = getImplant(pixel->cell->getDbInst()->getMaster());
      }
    }

    if (!implant) {  // empty row or cell has no implant - use anything
      implant = filler_masters_by_implant.begin()->first;
    }

    GridX gap = k - j;
    const DbuY row_height{site->getHeight()};
    dbMasterSeq& fillers
        = gapFillers(implant, gap, row_height, filler_masters_by_implant);
    if (fillers.empty()) {
      DbuX x{core_.xMin() + gridToDbu(j, site_width)};
      DbuY y{core_.yMin() + grid_->gridYToDbu(row)};
      logger_->error(DPL,
                     2,
                     "Could not fill gap of {} sites ({:.2f}x{:.2f} um) "
                     "at ({:.2f}, {:.2f}) um between {} and {}",
                     gap,
                     block_->dbuToMicrons(gridToDbu(gap, site_width).v),
                     block_->dbuToMicrons(row_height.v),
                     block_->dbuToMicrons(x.v),
                     block_->dbuToMicrons(y.v),
                     gridInstName(row, j - 1),
                     gridInstName(row, k + 1));
    } else {
      k = j;
      debugPrint(
          logger_, DPL, "filler", 2, "fillers size is {}.", fillers.size());
      for (dbMaster* master : fillers) {
        std::string inst_name
            = prefix + to_string(row.v) + "_" + to_string(k.v);
        odb::dbInst* inst
            = odb::dbInst::makeUniqueDbInst(block_,
                                            master,
                                            inst_name.c_str(),
                                            /* physical_only */ true);
        DbuX x{core_.xMin() + gridToDbu(k, site_width)};
        DbuY y{core_.yMin() + grid_->gridYToDbu(row)};
        inst->setOrient(orient);
        inst->setLocation(x.v, y.v);
        inst->setPlacementStatus(dbPlacementStatus::PLACED);
        inst->setSourceType(odb::dbSourceType::DIST);
        filler_count_[master]++;
        k += master->getWidth() / site_width.v;
      }
      j += gap;
    }
  }
}

const char* Opendp::gridInstName(GridY row, GridX col)
{
  if (col < 0) {
    return "core_left";
  }
  if (col > grid_->getRowSiteCount()) {
    return "core_right";
  }

  const auto cell = grid_->gridPixel(col, row)->cell;
  if (cell) {
    return cell->getDbInst()->getConstName();
  }
  return "?";
}

// Return list of masters to fill gap (in site width units).
dbMasterSeq& Opendp::gapFillers(
    odb::dbTechLayer* implant,
    GridX gap,
    DbuY row_height,
    const MasterByImplant& filler_masters_by_implant)
{
  auto iter = filler_masters_by_implant.find(implant);
  if (iter == filler_masters_by_implant.end()) {
    logger_->error(DPL, 50, "No fillers found for {}.", implant->getName());
  }
  const dbMasterSeq& filler_masters = iter->second;

  GapFillers& gap_fillers = gap_fillers_[implant][row_height];
  if (gap_fillers.size() < gap + 1) {
    gap_fillers.resize(gap.v + 1);
  }
  dbMasterSeq& fillers = gap_fillers[gap.v];
  if (fillers.empty()) {
    int width = 0;
    dbMaster* smallest_filler = filler_masters[filler_masters.size() - 1];
    const DbuX site_width = grid_->getSiteWidth();
    bool have_filler1 = smallest_filler->getWidth() == site_width;
    for (dbMaster* filler_master : filler_masters) {
      if (DbuY{static_cast<int>(filler_master->getHeight())} != row_height) {
        continue;
      }
      int filler_width = filler_master->getWidth() / site_width.v;
      while ((width + filler_width) <= gap
             && (have_filler1 || (width + filler_width) != gap - 1)) {
        fillers.push_back(filler_master);
        width += filler_width;
        if (width == gap) {
          return fillers;
        }
      }
    }
    // Fail. Return empty fillers.
    fillers.clear();
  }
  return fillers;
}

void Opendp::removeFillers()
{
  block_ = db_->getChip()->getBlock();
  for (odb::dbInst* db_inst : block_->getInsts()) {
    if (isFiller(db_inst)) {
      odb::dbInst::destroy(db_inst);
    }
  }
}

/* static */
bool Opendp::isFiller(odb::dbInst* db_inst)
{
  dbMaster* db_master = db_inst->getMaster();
  return db_master->getType() == odb::dbMasterType::CORE_SPACER
         // Filter spacer cells used as tapcells.
         && db_inst->getPlacementStatus() != odb::dbPlacementStatus::LOCKED;
}

// Return true if cell is a single site Core Spacer.
bool Opendp::isOneSiteCell(odb::dbMaster* db_master) const
{
  return db_master->getType() == odb::dbMasterType::CORE_SPACER
         && db_master->getWidth() == grid_->getSiteWidth();
}

}  // namespace dpl
