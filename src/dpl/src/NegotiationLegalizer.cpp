// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2026, The OpenROAD Authors

#include "NegotiationLegalizer.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <utility>
#include <vector>

#include "PlacementDRC.h"
#include "dpl/Opendp.h"
#include "graphics/DplObserver.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Objects.h"
#include "infrastructure/architecture.h"
#include "infrastructure/network.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "optimization/detailed_orient.h"
#include "utl/Logger.h"

namespace dpl {

// ===========================================================================
// FenceRegion
// ===========================================================================

bool FenceRegion::contains(int x, int y, int w, int h) const
{
  for (const auto& r : rects) {
    if (x >= r.xlo && y >= r.ylo && x + w <= r.xhi && y + h <= r.yhi) {
      return true;
    }
  }
  return false;
}

FenceRect FenceRegion::nearestRect(int cx, int cy) const
{
  assert(!rects.empty());
  const FenceRect* best = rects.data();
  int best_dist = std::numeric_limits<int>::max();
  for (const auto& r : rects) {
    const int d = odb::manhattanDistance(odb::Rect(r.xlo, r.ylo, r.xhi, r.yhi),
                                         odb::Point(cx, cy));
    if (d < best_dist) {
      best_dist = d;
      best = &r;
    }
  }
  return *best;
}

// ===========================================================================
// Constructor
// ===========================================================================

NegotiationLegalizer::NegotiationLegalizer(Opendp* opendp,
                                           odb::dbDatabase* db,
                                           utl::Logger* logger,
                                           DplObserver* debug_observer,
                                           Network* network)
    : opendp_(opendp),
      db_(db),
      logger_(logger),
      debug_observer_(debug_observer),
      network_(network)
{
}

// ===========================================================================
// legalize – top-level entry point
// ===========================================================================

void NegotiationLegalizer::legalize()
{
  debugPrint(logger_,
             utl::DPL,
             "negotiation",
             1,
             "NegotiationLegalizer: starting legalization.");

  logger_->info(utl::DPL,
                1103,
                "Negotiation base search window: +/-{} sites horizontally, "
                "+/-{} rows vertically ",
                site_search_window_,
                row_search_window_);
  logger_->report("\tAutomatic search window extension {}.",
                  disable_window_extension_ ? "disabled" : "enabled");
  if (!disable_window_extension_) {
    logger_->report(
        "\tSearch window extendable up to the max displacement cap of +/-{} "
        "sites, +/-{} rows near walls.",
        opendp_->max_displacement_x_,
        opendp_->max_displacement_y_);
  }

  logger_->info(
      utl::DPL, 1104, "NegotiationLegalizer DRC penalty: {}.", drc_penalty_);

  if (debug_observer_ && db_->getChip() && db_->getChip()->getBlock()) {
    debug_observer_->startPlacement(db_->getChip()->getBlock());
  }

  if (!initFromDb()) {
    return;
  }

  buildGrid();

  initFenceRegions();

  // Snap movable cells off invalid initial positions (no site, fixed
  // blockage, or outside their fence) and populate grid usage. Must run after
  // buildGrid()/initFenceRegions() so it can see fixed-cell blockages and
  // fence regions. Row legality (site type, orientation, power) is left to
  // negotiation.
  initialSnap();

  debugPause("Pause after initialization.");

  debugPrint(logger_,
             utl::DPL,
             "negotiation",
             1,
             "NegotiationLegalizer: {} cells, grid {}x{}.",
             cells_.size(),
             grid_w_,
             grid_h_);

  // --- Part 1: initial state (usage, grid sync, legality scan) ------------
  std::vector<int> illegal;
  debugPrint(logger_,
             utl::DPL,
             "negotiation",
             1,
             "NegotiationLegalizer: skipping Abacus pass.");
  // Grid usage was already populated by initialSnap().
  // Sync all movable cells to the DPL Grid so PlacementDRC neighbour
  // lookups see the correct placement state.
  syncAllCellsToDplGrid();
  for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
    if (!cells_[i].fixed) {
      cells_[i].legal = isCellLegal(i);
      if (!cells_[i].legal) {
        illegal.push_back(i);
      }
    }
  }
  debugPrint(
      logger_, utl::DPL, "negotiation", 1, "{} illegal cells", illegal.size());

  if (debug_observer_) {
    commitNegotiationPosToDpl();
    // this flush may imply functional changes. It hides initial movements for
    // clean debugging negotiation phase.
    logger_->report(
        "Committing post-init positions to odb; debug move line drawings will "
        "exclude gpl-to-init displacement.");
    commitNegotiationPosToOdb();
    pushNegotiationPixels();
    logger_->report("Pause after initialization.");
    debug_observer_->redrawAndPause();
  }

  // --- Part 2: Negotiation (main legalization) -------------------
  if (!illegal.empty()) {
    debugPrint(logger_,
               utl::DPL,
               "negotiation",
               1,
               "NegotiationLegalizer: negotiation pass on {} cells.",
               illegal.size());
    runNegotiation(illegal);
  }

  // Re-sync the DPL pixel grid after negotiation.  During negotiation,
  // overlapping cells share a single pixel->cell slot; when one is ripped
  // up, the other's presence is lost.  A full re-sync ensures every cell
  // is correctly painted so subsequent DRC checks (numViolations, etc.)
  // see the true placement state.
  syncAllCellsToDplGrid();

  debugPause("Pause after negotiation pass");

  const double avgDisp = avgDisplacement();
  const int maxDisp = maxDisplacement();
  const int nViol = numViolations();

  debugPrint(
      logger_,
      utl::DPL,
      "negotiation",
      1,
      "NegotiationLegalizer: done. AvgDisp={:.2f} MaxDisp={} Violations={}.",
      avgDisp,
      maxDisp,
      nViol);

  const Grid* dplGrid = opendp_->grid_.get();
  for (const auto& cell : cells_) {
    if (cell.fixed || cell.db_inst == nullptr) {
      continue;
    }
    // Set orientation from the row so cells are properly flipped.
    odb::dbSite* site = cell.db_inst->getMaster()->getSite();
    if (site != nullptr) {
      auto orient
          = dplGrid->getSiteOrientation(GridX{cell.x}, GridY{cell.y}, site);
      if (orient.has_value()) {
        cell.db_inst->setOrient(orient.value());
      }
    }
  }

  debugPause("Pause after legalization complete.");
}

// ===========================================================================
// commitNegotiationPosToOdb – write current cell positions to ODB so the GUI
// reflects them
// ===========================================================================

void NegotiationLegalizer::commitNegotiationPosToOdb()
{
  const Grid* dplGrid = opendp_->grid_.get();
  for (const auto& cell : cells_) {
    if (cell.fixed || cell.db_inst == nullptr) {
      continue;
    }
    const int db_x = die_xlo_ + cell.x * site_width_;
    const int db_y = die_ylo_ + dplGrid->gridYToDbu(GridY{cell.y}).v;
    cell.db_inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    cell.db_inst->setLocation(db_x, db_y);
  }
}

// ===========================================================================
// pushNegotiationPixels – send grid state to the debug observer for rendering
// ===========================================================================

void NegotiationLegalizer::pushNegotiationPixels()
{
  if (!debug_observer_) {
    return;
  }
  std::vector<NegotiationPixelState> pixels(static_cast<size_t>(grid_w_)
                                            * grid_h_);
  for (int gy = 0; gy < grid_h_; ++gy) {
    for (int gx = 0; gx < grid_w_; ++gx) {
      const Pixel& g = gridAt(gx, gy);
      const int idx = gy * grid_w_ + gx;
      if (g.capacity == 0) {
        pixels[idx] = (g.usage > 0) ? NegotiationPixelState::kBlocked
                                    : NegotiationPixelState::kNoRow;
      } else if (g.overuse() > 0) {
        pixels[idx] = NegotiationPixelState::kOveruse;
      } else if (g.usage > 0) {
        pixels[idx] = NegotiationPixelState::kOccupied;
      } else {
        pixels[idx] = NegotiationPixelState::kFree;
      }
    }
  }
  // Overlay cells that fail PlacementDRC checks.
  if (opendp_->drc_engine_ && network_) {
    for (const auto& cell : cells_) {
      if (cell.fixed) {
        continue;
      }
      Node* node = cell.node;
      if (node == nullptr) {
        continue;
      }
      if (!opendp_->drc_engine_->checkDRC(
              node, GridX{cell.x}, GridY{cell.y}, node->getOrient())) {
        for (int dy = 0; dy < cell.height; ++dy) {
          for (int gx = cell.x; gx < cell.x + cell.width; ++gx) {
            const int pidx = (cell.y + dy) * grid_w_ + gx;
            if (pidx >= 0 && pidx < static_cast<int>(pixels.size())) {
              pixels[pidx] = NegotiationPixelState::kDrcViolation;
            }
          }
        }
      }
    }
  }

  const Grid* dplGrid = opendp_->grid_.get();
  std::vector<int> row_y_dbu(grid_h_ + 1);
  for (int r = 0; r <= grid_h_; ++r) {
    row_y_dbu[r] = dplGrid->gridYToDbu(GridY{r}).v;
  }
  debug_observer_->setNegotiationPixels(
      pixels, grid_w_, grid_h_, die_xlo_, die_ylo_, site_width_, row_y_dbu);
}

void NegotiationLegalizer::debugPause(const std::string& msg)
{
  if (!debug_observer_) {
    return;
  }
  commitNegotiationPosToDpl();
  pushNegotiationPixels();
  logger_->report("{}", msg);
  debug_observer_->redrawAndPause();
  debug_observer_->clearAllDiamondSearches();
}

// ===========================================================================
// commitNegotiationPosToDpl – pass the positions to the DPL original structure
// (Node)
// ===========================================================================

void NegotiationLegalizer::commitNegotiationPosToDpl()
{
  if (!network_) {
    return;
  }
  for (const auto& cell : cells_) {
    if (cell.fixed || cell.db_inst == nullptr || cell.node == nullptr) {
      continue;
    }
    const int coreX = cell.x * site_width_;
    const int coreY = opendp_->grid_->gridYToDbu(GridY{cell.y}).v;
    cell.node->setLeft(DbuX(coreX));
    cell.node->setBottom(DbuY(coreY));
    cell.node->setPlaced(true);

    // Update orientation to match the row.
    odb::dbSite* site = cell.db_inst->getMaster()->getSite();
    if (site != nullptr) {
      auto orient = opendp_->grid_->getSiteOrientation(
          GridX{cell.x}, GridY{cell.y}, site);
      if (orient.has_value()) {
        cell.node->setOrient(orient.value());
      }
    }
  }
}

// ===========================================================================
// Initialisation
// ===========================================================================

bool NegotiationLegalizer::initFromDb()
{
  if (db_->getChip() == nullptr || !opendp_ || !opendp_->grid_) {
    return false;
  }
  auto* block = db_->getChip()->getBlock();
  if (block == nullptr) {
    return false;
  }

  const Grid* dpl_grid = opendp_->grid_.get();

  const odb::Rect core_area = dpl_grid->getCore();
  die_xlo_ = core_area.xMin();
  die_ylo_ = core_area.yMin();
  die_xhi_ = core_area.xMax();
  die_yhi_ = core_area.yMax();

  site_width_ = dpl_grid->getSiteWidth().v;
  assert(site_width_ > 0);

  // Grid dimensions from the DPL grid (accounts for actual DB rows).
  grid_w_ = dpl_grid->getRowSiteCount().v;
  grid_h_ = dpl_grid->getRowCount().v;

  // legalize() can run again on this same object, and grid_w_/grid_h_ may be
  // different next time.
  hist_seen_stamp_.assign(static_cast<size_t>(grid_w_) * grid_h_, 0);
  hist_gen_ = 0;

  // Cached, not derived per candidate: findBestLocation prices every position
  // in every active cell's window on every negotiation iteration.
  row_y_dbu_.resize(grid_h_ + 1);
  for (int gy = 0; gy <= grid_h_; ++gy) {
    row_y_dbu_[gy] = dpl_grid->gridYToDbu(GridY{gy}).v;
  }

  cells_.clear();
  cells_.reserve(block->getInsts().size());

  for (auto* db_inst : block->getInsts()) {
    const auto status = db_inst->getPlacementStatus();
    if (status == odb::dbPlacementStatus::NONE) {
      continue;
    }
    // Skip non-core-auto-placeable instances (pads, blocks, etc.) — these
    // are absent from the Opendp network so DRC/legality checks can't be
    // performed on them.  They are handled separately by setFixedGridCells().
    if (!db_inst->getMaster()->isCoreAutoPlaceable()) {
      continue;
    }

    NegCell cell;
    cell.db_inst = db_inst;
    cell.node = network_ ? network_->getNode(db_inst) : nullptr;
    cell.fixed = status.isFixed();

    int db_x = 0;
    int db_y = 0;
    db_inst->getLocation(db_x, db_y);
    // Snap to grid, findBestLocation() iterates over grid positions
    cell.init_x = dpl_grid->gridRoundX(DbuX{db_x - die_xlo_}).v;
    cell.init_y = dpl_grid->gridRoundY(DbuY{db_y - die_ylo_}).v;

    // Width/height must be computed before clamping so the upper bounds
    // account for the full cell footprint (x + width <= grid_w_, etc.).
    auto* master = db_inst->getMaster();
    cell.width = std::max(
        1,
        static_cast<int>(
            std::round(static_cast<double>(master->getWidth()) / site_width_)));
    cell.height = dpl_grid->gridHeight(master).v;

    // Clamp to valid grid range – gridRoundY can return grid_h_ when the
    // instance is near the top edge.  Use (grid_w_ - width) so the full
    // footprint stays within the grid (matches Opendp::legalPt behaviour).
    cell.init_x = std::max(0, std::min(cell.init_x, grid_w_ - cell.width));
    cell.init_y = std::max(0, std::min(cell.init_y, grid_h_ - cell.height));
    cell.x = cell.init_x;
    cell.y = cell.init_y;

    debugPrint(logger_,
               utl::DPL,
               "negotiation",
               2,
               "DEBUG cell init: {} height={}",
               db_inst->getName(),
               cell.height);

    cells_.push_back(cell);
  }

  std::map<int, int> neg_height_counts;
  for (const NegCell& c : cells_) {
    neg_height_counts[c.height]++;
  }
  logger_->info(
      utl::DPL,
      392,
      "Negotiation cell height distribution ({} unique row-count(s)):",
      neg_height_counts.size());
  for (const auto& [height, count] : neg_height_counts) {
    logger_->info(utl::DPL, 393, "  height {} row(s): {} cells", height, count);
  }

  return true;
}

void NegotiationLegalizer::buildGrid()
{
  Grid* dplGrid = opendp_->grid_.get();

  // Reset all pixels to default negotiation state.  Also clear any cell
  // and padding pointers (including dummy_cell_ set by groupInitPixels2 for
  // region boundaries) — fixed cells and movable cells are re-painted later
  // by syncAllCellsToDplGrid() before any DRC check runs.
  for (int gy = 0; gy < grid_h_; ++gy) {
    for (int gx = 0; gx < grid_w_; ++gx) {
      Pixel& pixel = dplGrid->pixel(GridY{gy}, GridX{gx});
      pixel.capacity = pixel.is_valid ? 1 : 0;
      pixel.usage = 0;
      pixel.hist_cost = 1.0;
      pixel.cell = nullptr;
      pixel.padding_reserved_by = nullptr;
    }
  }

  // Derive per-row existence: a row "has sites" if at least one site
  // on that row has capacity > 0.
  row_has_sites_.assign(grid_h_, false);
  for (int gy = 0; gy < grid_h_; ++gy) {
    for (int gx = 0; gx < grid_w_; ++gx) {
      if (gridAt(gx, gy).capacity > 0) {
        row_has_sites_[gy] = true;
        break;
      }
    }
  }

  // Blockade each fixed cell and record its usage in one pass.  Footprint
  // only: padding is left to PlacementDRC, which knows both masters and so
  // can apply the class-pair rules that a plain capacity cannot express.
  for (const NegCell& cell : cells_) {
    if (!cell.fixed) {
      continue;
    }
    for (int dy = 0; dy < cell.height; ++dy) {
      const int gy = cell.y + dy;
      for (int gx = cell.x; gx < cell.x + cell.width; ++gx) {
        if (gridExists(gx, gy)) {
          Pixel& pixel = gridAt(gx, gy);
          pixel.capacity = 0;
          pixel.usage = 1;
        }
      }
    }
  }
}

void NegotiationLegalizer::initFenceRegions()
{
  fences_.clear();  // Guard against double-population if legalize() is re-run.

  auto* block = db_->getChip()->getBlock();

  for (auto* region : block->getRegions()) {
    FenceRegion fr;
    fr.id = region->getId();

    const Grid* dpl_grid = opendp_->grid_.get();
    for (auto* box : region->getBoundaries()) {
      FenceRect r;
      r.xlo = (box->xMin() - die_xlo_) / site_width_;
      r.ylo = dpl_grid->gridEndY(DbuY{box->yMin() - die_ylo_}).v;
      r.xhi = (box->xMax() - die_xlo_) / site_width_;
      r.yhi = dpl_grid->gridSnapDownY(DbuY{box->yMax() - die_ylo_}).v;
      fr.rects.push_back(r);
    }

    if (!fr.rects.empty()) {
      fences_.push_back(std::move(fr));
    }
  }

  // Map each instance to its fence region (if any).
  // Instances are assigned to regions via GROUPS in DEF, which sets
  // dbInst::group_ but NOT dbInst::region_.  We must go through the group.
  for (auto& cell : cells_) {
    if (cell.db_inst == nullptr) {
      continue;
    }
    // Try direct region first, then group-based region.
    odb::dbRegion* region = cell.db_inst->getRegion();
    if (region == nullptr) {
      auto* grp = cell.db_inst->getGroup();
      if (grp != nullptr) {
        region = grp->getRegion();
      }
    }
    if (region == nullptr) {
      continue;
    }
    const int rid = region->getId();
    for (int fi = 0; fi < static_cast<int>(fences_.size()); ++fi) {
      if (fences_[fi].id == rid) {
        cell.fence_id = fi;
        break;
      }
    }
  }
}

// ===========================================================================
// initialSnap – relocate movable cells off invalid initial
// positions and record their usage on the grid (density-aware).
// ===========================================================================

void NegotiationLegalizer::initialSnap()
{
  const Grid* dpl_grid = opendp_->grid_.get();

  // Can cell `ci` geometrically occupy (px, py)? True when the body footprint
  // is in-die, on real sites (capacity 0 = no site or fixed blockage) and
  // inside the fence. Deliberately ignores row legality (site type,
  // orientation, power), movable overlap and padding — negotiation handles all
  // of those; the snap only keeps cells off fixed blockages and off-die.
  auto placeable = [&](int ci, int px, int py) -> bool {
    const NegCell& cell = cells_[ci];
    if (!inDie(px, py, cell.width, cell.height)) {
      return false;
    }
    for (int dy = 0; dy < cell.height; ++dy) {
      for (int gx = px; gx < px + cell.width; ++gx) {
        if (gridAt(gx, py + dy).capacity == 0) {
          return false;
        }
      }
    }
    return respectsFence(ci, px, py);
  };

  for (int idx = 0; idx < static_cast<int>(cells_.size()); ++idx) {
    NegCell& cell = cells_[idx];
    if (cell.fixed) {
      continue;
    }

    if (!placeable(idx, cell.init_x, cell.init_y)) {
      debugPrint(logger_,
                 utl::DPL,
                 "negotiation",
                 1,
                 "Instance '{}' has an invalid initial position at dbu "
                 "({}, {}). Will search for the nearest valid site.",
                 cell.db_inst->getName(),
                 die_xlo_ + cell.init_x * site_width_,
                 die_ylo_ + dpl_grid->gridYToDbu(GridY{cell.init_y}).v);

      const int init_y_dbu = dpl_grid->gridYToDbu(GridY{cell.init_y}).v;
      // Farthest an in-grid site can sit from the initial position; once the
      // budget reaches it the whole grid has been covered.
      const int max_budget
          = grid_w_ * site_width_
            + std::max(init_y_dbu - dpl_grid->gridYToDbu(GridY{0}).v,
                       dpl_grid->gridYToDbu(GridY{grid_h_ - cell.height}).v
                           - init_y_dbu);

      int best_x = cell.init_x;
      int best_y = cell.init_y;
      int best_dist = std::numeric_limits<int>::max();
      bool found = false;

      for (int budget = site_width_; !found;
           budget = std::min(budget * 2, max_budget)) {
        // Rows in rings of increasing distance from init_y. `reach` is the
        // current budget, tightened by the incumbent once a hit is seen.
        for (int r = 0;; ++r) {
          const int reach = std::min(budget, best_dist);
          const int ring_rows[2] = {cell.init_y - r, cell.init_y + r};
          bool ring_in_reach = false;
          for (int s = 0; s < (r == 0 ? 1 : 2); ++s) {
            const int py = ring_rows[s];
            if (py < 0 || py + cell.height > grid_h_) {
              continue;
            }
            const int vdist
                = std::abs(dpl_grid->gridYToDbu(GridY{py}).v - init_y_dbu);
            if (vdist >= reach) {
              continue;  // this row is beyond the current reach
            }
            ring_in_reach = true;
            // Nearest placeable column on this row within the reach budget.
            // dx == 0 first, then outward (left wins ties).
            const int max_dx = (reach - vdist) / site_width_;
            for (int dx = 0; dx <= max_dx; ++dx) {
              int hit_x = -1;
              if (cell.init_x - dx >= 0
                  && placeable(idx, cell.init_x - dx, py)) {
                hit_x = cell.init_x - dx;
              } else if (dx > 0 && cell.init_x + dx + cell.width <= grid_w_
                         && placeable(idx, cell.init_x + dx, py)) {
                hit_x = cell.init_x + dx;
              }
              if (hit_x >= 0) {
                const int total = vdist + dx * site_width_;
                if (total < best_dist) {
                  best_dist = total;
                  best_x = hit_x;
                  best_y = py;
                  found = true;
                }
                break;  // nearest column on this row
              }
            }
          }
          if (!ring_in_reach) {
            break;
          }
        }
        if (budget >= max_budget) {
          break;  // whole grid covered, nothing placeable
        }
      }

      if (found) {
        cell.init_x = best_x;
        cell.init_y = best_y;
        cell.x = cell.init_x;
        cell.y = cell.init_y;
      } else {
        debugPrint(
            logger_,
            utl::DPL,
            "negotiation",
            1,
            "No placeable site found for instance '{}' near its initial "
            "position dbu ({}, {}). Diamond search covered the grid with "
            "no match. Leaving instance at its initial position; "
            "negotiation will need to legalize it.",
            cell.db_inst->getName(),
            die_xlo_ + cell.init_x * site_width_,
            die_ylo_ + dpl_grid->gridYToDbu(GridY{cell.init_y}).v);
      }
    }
  }

  // Record grid usage for movable cells.
  // The snap search does not read usage, so this is done after all cells are
  // placed
  for (int idx = 0; idx < static_cast<int>(cells_.size()); ++idx) {
    if (!cells_[idx].fixed) {
      addUsage(idx, 1);
    }
  }
}

// ===========================================================================
// Grid helpers for Negotiation Legalizer
// ===========================================================================

void NegotiationLegalizer::addUsage(int cell_idx, int delta)
{
  const NegCell& cell = cells_[cell_idx];
  for (int dy = 0; dy < cell.height; ++dy) {
    const int gy = cell.y + dy;
    for (int gx = cell.x; gx < cell.x + cell.width; ++gx) {
      if (gridExists(gx, gy)) {
        gridAt(gx, gy).usage += delta;
      }
    }
  }
}

// ===========================================================================
// DPL Grid synchronisation
// ===========================================================================

void NegotiationLegalizer::syncCellToDplGrid(int cell_idx)
{
  if (!opendp_ || !opendp_->grid_ || !network_) {
    return;
  }
  const NegCell& neg_cell = cells_[cell_idx];
  if (neg_cell.db_inst == nullptr) {
    return;
  }
  Node* node = neg_cell.node;
  if (node == nullptr) {
    return;
  }
  // Update the Node's position to match the NegCell so Grid operations
  // (which read Node left/bottom) see the current placement.
  node->setLeft(DbuX(neg_cell.x * site_width_));
  node->setBottom(DbuY(opendp_->grid_->gridYToDbu(GridY{neg_cell.y}).v));

  // Update orientation to match the row.
  odb::dbSite* site = neg_cell.db_inst->getMaster()->getSite();
  if (site != nullptr) {
    auto orient = opendp_->grid_->getSiteOrientation(
        GridX{neg_cell.x}, GridY{neg_cell.y}, site);
    if (orient.has_value()) {
      node->setOrient(orient.value());
    }
  }

  opendp_->grid_->paintPixel(node, GridX{neg_cell.x}, GridY{neg_cell.y});
}

void NegotiationLegalizer::eraseCellFromDplGrid(int cell_idx)
{
  if (!opendp_ || !opendp_->grid_ || !network_) {
    return;
  }
  const NegCell& neg_cell = cells_[cell_idx];
  if (neg_cell.db_inst == nullptr) {
    return;
  }
  Node* node = neg_cell.node;
  if (node == nullptr) {
    return;
  }
  // Ensure the Node's position matches the current NegCell position so
  // erasePixel clears the correct pixels (it reads gridCoveringPadded).
  node->setLeft(DbuX(neg_cell.x * site_width_));
  node->setBottom(DbuY(opendp_->grid_->gridYToDbu(GridY{neg_cell.y}).v));
  opendp_->grid_->erasePixel(node);
}

void NegotiationLegalizer::syncAllCellsToDplGrid()
{
  if (!opendp_ || !opendp_->grid_ || !network_) {
    return;
  }
  // Clear all movable cells from the DPL Grid first, then repaint at
  // their current positions.  Fixed cells were already painted during
  // Opendp::setFixedGridCells() and should not be touched.
  for (const NegCell& neg_cell : cells_) {
    if (neg_cell.fixed || neg_cell.db_inst == nullptr) {
      continue;
    }
    Node* node = neg_cell.node;
    if (node == nullptr) {
      continue;
    }
    // Update Node position then erase whatever was previously painted.
    node->setLeft(DbuX(neg_cell.x * site_width_));
    node->setBottom(DbuY(opendp_->grid_->gridYToDbu(GridY{neg_cell.y}).v));
    opendp_->grid_->erasePixel(node);
  }
  for (const NegCell& neg_cell : cells_) {
    if (neg_cell.fixed || neg_cell.db_inst == nullptr) {
      continue;
    }
    Node* node = neg_cell.node;
    if (node == nullptr) {
      continue;
    }
    // Set orientation to match the row, same as syncCellToDplGrid.
    odb::dbSite* site = neg_cell.db_inst->getMaster()->getSite();
    if (site != nullptr) {
      auto orient = opendp_->grid_->getSiteOrientation(
          GridX{neg_cell.x}, GridY{neg_cell.y}, site);
      if (orient.has_value()) {
        node->setOrient(orient.value());
      }
    }
    opendp_->grid_->paintPixel(node, GridX{neg_cell.x}, GridY{neg_cell.y});
  }
  // Re-paint fixed cells last so they always win over any movable cell that
  // may have been placed at an overlapping initial position.  Without this,
  // a movable cell painted on top of an endcap overwrites pixel->cell, and
  // the subsequent erasePixel call clears the endcap from the grid, making
  // checkOneSiteGap blind to it.
  for (const NegCell& cell : cells_) {
    if (!cell.fixed || cell.db_inst == nullptr) {
      continue;
    }
    Node* node = cell.node;
    if (node == nullptr) {
      continue;
    }
    opendp_->grid_->paintPixel(node);
  }
}

// ===========================================================================
// Constraint helpers
// ===========================================================================

bool NegotiationLegalizer::inDie(int x, int y, int w, int h) const
{
  return x >= 0 && y >= 0 && x + w <= grid_w_ && y + h <= grid_h_;
}

bool NegotiationLegalizer::isValidRow(int rowIdx,
                                      const NegCell& cell,
                                      int gridX) const
{
  if (rowIdx < 0 || rowIdx + cell.height > grid_h_) {
    return false;
  }
  for (int dy = 0; dy < cell.height; ++dy) {
    if (!row_has_sites_[rowIdx + dy]) {
      return false;
    }
  }
  // Mirror Opendp::canBePlaced: the row must offer the cell's site, cell master
  // must be able to take the orientation the row requires and a multi-row
  // cell's power stack must line up across the span.
  if (cell.db_inst != nullptr && opendp_ != nullptr && opendp_->grid_
      && network_ != nullptr) {
    auto* dbMaster = cell.db_inst->getMaster();
    odb::dbSite* site = dbMaster->getSite();
    if (site != nullptr) {
      const auto orient = opendp_->grid_->getSiteOrientation(
          GridX{gridX}, GridY{rowIdx}, site);
      if (!orient) {
        return false;
      }
      const unsigned masterSym = DetailedOrient::getMasterSymmetry(dbMaster);
      if (!opendp_->checkMasterSym(masterSym, orient.value())) {
        return false;
      }
    }
    Node* node = cell.node;
    if (node != nullptr && node->getMaster()->isMultiRow()
        && !opendp_->checkRowPowerCompatible(node, GridY{rowIdx})) {
      return false;
    }
  }
  return true;
}

std::vector<int> NegotiationLegalizer::verticalWindowRows(const NegCell& cell,
                                                          int seed_y,
                                                          int x_lo,
                                                          int x_hi,
                                                          int count_per_side,
                                                          int max_scan) const
{
  // A "wall" in the Y direction is off-core: the die edge, or a band of rows
  // with no placement sites at all (e.g. a full-width macro/blockage row).
  auto hardWall = [&](int r) {
    if (r < 0 || r + cell.height > grid_h_) {
      return true;
    }
    for (int dy = 0; dy < cell.height; ++dy) {
      if (!row_has_sites_[r + dy]) {
        return true;
      }
    }
    return false;
  };

  // A row counts toward the quota only when it can host the cell somewhere
  // inside the horizontal span: probing a single column would declare rows
  // beside a macro usable (or dead) based on one pixel.
  const int probe_lo = std::max(0, x_lo);
  const int probe_hi = std::min(grid_w_ - cell.width, x_hi);
  auto rowUsable = [&](int r) {
    if (r < 0 || r + cell.height > grid_h_) {
      return false;
    }
    for (int x = probe_lo; x <= probe_hi; ++x) {
      if (gridAt(x, r).capacity > 0 && isValidRow(r, cell, x)) {
        return true;
      }
    }
    return false;
  };

  // Each side walks outward collecting usable rows against its own quota
  // (`count_per_side`) and distance cap (`max_scan`). A side closes when it
  // hits a macro/off-core wall, exhausts its distance cap, or fills its
  // quota. Whatever quota a closing side could not fill — whether it was cut
  // short by a wall or ran out of steps over unusable rows (e.g. rows
  // fully covered by a macro inside the span, fences, power mismatch) — is
  // handed to the other side along with a longer distance cap
  // (`extended_cap`) to spend it in. A side that merely filled its quota is
  // reopened by a donation; a walled side cannot take one. If both sides
  // are walled short, the unspent quota is simply never used.
  const int extended_cap = std::min(2 * max_scan, opendp_->max_displacement_y_);
  struct Side
  {
    int dir;
    int step;
    int quota;
    int cap;
    bool closed;
    bool walled;  // closed at a wall or distance cap: cannot take donations
    std::vector<int> found;
  };
  Side below{+1, 0, count_per_side, max_scan, false, false, {}};
  Side above{-1, 0, count_per_side, max_scan, false, false, {}};

  auto close = [&](Side& self, Side& other) {
    self.closed = true;
    self.walled = true;
    if (!disable_window_extension_ && self.quota > 0 && !other.walled) {
      other.quota += self.quota;
      other.cap = extended_cap;
      other.closed = false;  // reopen if it had merely filled its quota
      self.quota = 0;
    }
  };
  auto stepSide = [&](Side& self, Side& other) {
    if (self.closed) {
      return;
    }
    if (self.quota == 0) {
      self.closed = true;  // quota filled; a later donation may reopen us
      return;
    }
    if (self.step >= self.cap
        || hardWall(seed_y + self.dir * (self.step + 1))) {
      close(self, other);
      return;
    }
    const int r = seed_y + self.dir * ++self.step;
    if (rowUsable(r)) {
      self.found.push_back(r);
      --self.quota;
    }
  };

  while (!below.closed || !above.closed) {
    stepSide(below, above);
    stepSide(above, below);
  }

  std::vector<int> rows;
  rows.reserve(below.found.size() + above.found.size() + 1);
  if (rowUsable(seed_y)) {
    rows.push_back(seed_y);
  }
  rows.insert(rows.end(), below.found.begin(), below.found.end());
  rows.insert(rows.end(), above.found.begin(), above.found.end());
  return rows;
}

bool NegotiationLegalizer::respectsFence(int cell_idx, int x, int y) const
{
  const NegCell& cell = cells_[cell_idx];
  if (cell.fence_id < 0) {
    // Default region: must not overlap any named fence.
    for (const auto& fence : fences_) {
      if (fence.contains(x, y, cell.width, cell.height)) {
        return false;
      }
    }
    return true;
  }
  return fences_[cell.fence_id].contains(x, y, cell.width, cell.height);
}

bool NegotiationLegalizer::isCellLegal(int cell_idx) const
{
  const NegCell& cell = cells_[cell_idx];
  if (!inDie(cell.x, cell.y, cell.width, cell.height)) {
    return false;
  }
  if (!isValidRow(cell.y, cell, cell.x)) {
    return false;
  }
  if (!respectsFence(cell_idx, cell.x, cell.y)) {
    return false;
  }

  // Check placement DRCs (edge spacing, blocked layers, padding,
  // one-site gaps) against neighbours on the DPL Grid.
  if (opendp_ && opendp_->drc_engine_ && network_) {
    Node* node = cell.node;
    if (node != nullptr
        && !opendp_->drc_engine_->checkDRC(
            node, GridX{cell.x}, GridY{cell.y}, node->getOrient())) {
      return false;
    }
  } else {
    logger_->report("DRC objects not available!");
    return false;
  }

  for (int dy = 0; dy < cell.height; ++dy) {
    for (int gx = cell.x; gx < cell.x + cell.width; ++gx) {
      const Pixel& g = gridAt(gx, cell.y + dy);
      if (g.capacity == 0 || g.overuse() > 0) {
        return false;
      }
    }
  }
  return true;
}

// ===========================================================================
// Metrics
// ===========================================================================

double NegotiationLegalizer::avgDisplacement() const
{
  double sum = 0.0;
  int count = 0;
  for (const auto& cell : cells_) {
    if (!cell.fixed) {
      sum += displacementInSites(cell, cell.x, cell.y);
      ++count;
    }
  }
  return count > 0 ? sum / count : 0.0;
}

int NegotiationLegalizer::maxDisplacement() const
{
  int mx = 0;
  for (const auto& cell : cells_) {
    if (!cell.fixed) {
      mx = std::max(mx, displacementInSites(cell, cell.x, cell.y));
    }
  }
  return mx;
}

int NegotiationLegalizer::numViolations() const
{
  int count = 0;
  for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
    if (!cells_[i].fixed && !isCellLegal(i)) {
      ++count;
    }
  }
  return count;
}

std::vector<Node*> NegotiationLegalizer::getIllegalNodes() const
{
  std::vector<Node*> illegal;
  if (!network_) {
    return illegal;
  }
  for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
    if (!cells_[i].fixed && !isCellLegal(i)) {
      if (Node* node = cells_[i].node) {
        illegal.push_back(node);
      }
    }
  }
  return illegal;
}

}  // namespace dpl
