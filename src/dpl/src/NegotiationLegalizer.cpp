// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2026, The OpenROAD Authors

#include "NegotiationLegalizer.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "PlacementDRC.h"
#include "dpl/Opendp.h"
#include "graphics/DplObserver.h"
#include "infrastructure/Grid.h"
#include "infrastructure/Objects.h"
#include "infrastructure/Padding.h"
#include "infrastructure/network.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"
#include "utl/timer.h"

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
                                           const Padding* padding,
                                           DplObserver* debug_observer,
                                           Network* network)
    : opendp_(opendp),
      db_(db),
      logger_(logger),
      padding_(padding),
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

  double init_from_db_s{0}, build_grid_s{0}, fence_regions_s{0}, abacus_s{0};
  double negotiation_s{0}, post_neg_sync_s{0}, metrics_s{0}, flush_s{0},
      orient_s{0};
  const utl::Timer total_timer;

  {
    utl::DebugScopedTimer t(init_from_db_s,
                            logger_,
                            utl::DPL,
                            "negotiation_runtime",
                            1,
                            "initFromDb: {}");
    if (!initFromDb()) {
      return;
    }
  }

  if (debug_observer_) {
    debug_observer_->startPlacement(db_->getChip()->getBlock());
    debugPause("Pause after initFromDb.");
  }

  {
    utl::DebugScopedTimer t(build_grid_s,
                            logger_,
                            utl::DPL,
                            "negotiation_runtime",
                            1,
                            "buildGrid: {}");
    buildGrid();
  }

  {
    utl::DebugScopedTimer t(fence_regions_s,
                            logger_,
                            utl::DPL,
                            "negotiation_runtime",
                            1,
                            "initFenceRegions: {}");
    initFenceRegions();
  }

  debugPrint(logger_,
             utl::DPL,
             "negotiation",
             1,
             "NegotiationLegalizer: {} cells, grid {}x{}.",
             cells_.size(),
             grid_w_,
             grid_h_);

  // --- Part 1: Abacus (handles the majority of cells cheaply) -------------
  std::vector<int> illegal;
  if (run_abacus_) {
    debugPrint(logger_,
               utl::DPL,
               "negotiation",
               1,
               "NegotiationLegalizer: running Abacus pass.");
    {
      utl::DebugScopedTimer t(abacus_s,
                              logger_,
                              utl::DPL,
                              "negotiation_runtime",
                              1,
                              "runAbacus: {}");
      illegal = runAbacus();
    }
    debugPrint(logger_,
               utl::DPL,
               "negotiation",
               1,
               "NegotiationLegalizer: Abacus done, {} cells still illegal.",
               illegal.size());
  } else {
    debugPrint(logger_,
               utl::DPL,
               "negotiation",
               1,
               "NegotiationLegalizer: skipping Abacus pass.");
    {
      utl::DebugScopedTimer t(abacus_s,
                              logger_,
                              utl::DPL,
                              "negotiation_runtime",
                              1,
                              "skip-Abacus addUsage: {}");
      // Populate usage from initial coordinates
      for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
        if (!cells_[i].fixed) {
          addUsage(i, 1);
        }
      }
    }
    {
      utl::DebugScopedTimer t(abacus_s,
                              logger_,
                              utl::DPL,
                              "negotiation_runtime",
                              1,
                              "skip-Abacus syncAllCellsToDplGrid: {}");
      // Sync all movable cells to the DPL Grid so PlacementDRC neighbour
      // lookups see the correct placement state.
      syncAllCellsToDplGrid();
    }
    {
      utl::DebugScopedTimer t(abacus_s,
                              logger_,
                              utl::DPL,
                              "negotiation_runtime",
                              1,
                              "skip-Abacus isCellLegal scan: {}");
      for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
        if (!cells_[i].fixed) {
          cells_[i].legal = isCellLegal(i);
          if (!cells_[i].legal) {
            illegal.push_back(i);
          }
        }
      }
    }
    debugPrint(logger_,
               utl::DPL,
               "negotiation",
               1,
               "{} illegal cells",
               illegal.size());
  }

  if (debug_observer_) {
    setDplPositions();
    // this flush may imply functional changes. It hides initial movements for
    // clean debugging negotiation phase.
    flushToDb();
    pushNegotiationPixels();
    logger_->report("Pause after Abacus pass.");
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
    utl::DebugScopedTimer t(negotiation_s,
                            logger_,
                            utl::DPL,
                            "negotiation_runtime",
                            1,
                            "runNegotiation: {}");
    runNegotiation(illegal);
  }

  // Re-sync the DPL pixel grid after negotiation.  During negotiation,
  // overlapping cells share a single pixel->cell slot; when one is ripped
  // up, the other's presence is lost.  A full re-sync ensures every cell
  // is correctly painted so subsequent DRC checks (numViolations, etc.)
  // see the true placement state.
  {
    utl::DebugScopedTimer t(post_neg_sync_s,
                            logger_,
                            utl::DPL,
                            "negotiation_runtime",
                            1,
                            "post-negotiation syncAllCellsToDplGrid: {}");
    syncAllCellsToDplGrid();
  }

  debugPause("Pause after negotiation pass");

  // --- Part 3: Post-optimisation ------------------------------------------
  debugPrint(logger_,
             utl::DPL,
             "negotiation",
             1,
             "NegotiationLegalizer: post-optimisation.");
  // greedyImprove(5);
  // cellSwap();
  // greedyImprove(1);

  double avgDisp{0};
  int maxDisp{0}, nViol{0};
  {
    utl::DebugScopedTimer t(metrics_s,
                            logger_,
                            utl::DPL,
                            "negotiation_runtime",
                            1,
                            "metrics (avgDisp/maxDisp/violations): {}");
    avgDisp = avgDisplacement();
    maxDisp = maxDisplacement();
    nViol = numViolations();
  }

  debugPrint(
      logger_,
      utl::DPL,
      "negotiation",
      1,
      "NegotiationLegalizer: done. AvgDisp={:.2f} MaxDisp={} Violations={}.",
      avgDisp,
      maxDisp,
      nViol);

  {
    utl::DebugScopedTimer t(
        flush_s, logger_, utl::DPL, "negotiation_runtime", 1, "flushToDb: {}");
    flushToDb();
  }

  {
    utl::DebugScopedTimer t(orient_s,
                            logger_,
                            utl::DPL,
                            "negotiation_runtime",
                            1,
                            "orientation update: {}");
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
  }

  const double total_s = total_timer.elapsed();
  auto pct
      = [total_s](double t) { return total_s > 0 ? 100.0 * t / total_s : 0.0; };
  auto to_ms = [](double s) { return s * 1e3; };
  debugPrint(logger_,
             utl::DPL,
             "negotiation_runtime",
             1,
             "legalize() total {:.1f}ms breakdown: "
             "initFromDb {:.1f}ms ({:.0f}%), "
             "buildGrid {:.1f}ms ({:.0f}%), "
             "initFenceRegions {:.1f}ms ({:.0f}%), "
             "abacus {:.1f}ms ({:.0f}%), "
             "negotiation {:.1f}ms ({:.0f}%), "
             "postNegSync {:.1f}ms ({:.0f}%), "
             "metrics {:.1f}ms ({:.0f}%), "
             "flushToDb {:.1f}ms ({:.0f}%), "
             "orientUpdate {:.1f}ms ({:.0f}%)",
             to_ms(total_s),
             to_ms(init_from_db_s),
             pct(init_from_db_s),
             to_ms(build_grid_s),
             pct(build_grid_s),
             to_ms(fence_regions_s),
             pct(fence_regions_s),
             to_ms(abacus_s),
             pct(abacus_s),
             to_ms(negotiation_s),
             pct(negotiation_s),
             to_ms(post_neg_sync_s),
             pct(post_neg_sync_s),
             to_ms(metrics_s),
             pct(metrics_s),
             to_ms(flush_s),
             pct(flush_s),
             to_ms(orient_s),
             pct(orient_s));
}

// ===========================================================================
// flushToDb – write current cell positions to ODB so the GUI reflects them
// ===========================================================================

void NegotiationLegalizer::flushToDb()
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
      Node* node = network_->getNode(cell.db_inst);
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
  setDplPositions();
  pushNegotiationPixels();
  logger_->report("{}", msg);
  debug_observer_->redrawAndPause();
}

// ===========================================================================
// setDplPositions – pass the positions to the DPL original structure (Node)
// ===========================================================================

void NegotiationLegalizer::setDplPositions()
{
  if (!network_) {
    return;
  }
  std::unordered_map<odb::dbInst*, Node*> inst_to_node;
  inst_to_node.reserve(network_->getNodes().size());
  for (const auto& node_ptr : network_->getNodes()) {
    if (node_ptr->getDbInst()) {
      inst_to_node[node_ptr->getDbInst()] = node_ptr.get();
    }
  }

  for (const auto& cell : cells_) {
    if (cell.fixed || cell.db_inst == nullptr) {
      continue;
    }
    auto it = inst_to_node.find(cell.db_inst);
    if (it != inst_to_node.end()) {
      const int coreX = cell.x * site_width_;
      const int coreY = opendp_->grid_->gridYToDbu(GridY{cell.y}).v;
      it->second->setLeft(DbuX(coreX));
      it->second->setBottom(DbuY(coreY));
      it->second->setPlaced(true);

      // Update orientation to match the row.
      odb::dbSite* site = cell.db_inst->getMaster()->getSite();
      if (site != nullptr) {
        auto orient = opendp_->grid_->getSiteOrientation(
            GridX{cell.x}, GridY{cell.y}, site);
        if (orient.has_value()) {
          it->second->setOrient(orient.value());
        }
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

  // Site width from the DPL grid; row height from the first DB row.
  site_width_ = dpl_grid->getSiteWidth().v;
  for (auto* row : block->getRows()) {
    row_height_ = row->getSite()->getHeight();
    break;
  }
  assert(site_width_ > 0 && row_height_ > 0);

  // Grid dimensions from the DPL grid (accounts for actual DB rows).
  grid_w_ = dpl_grid->getRowSiteCount().v;
  grid_h_ = dpl_grid->getRowCount().v;

  // Assign power-rail types using row-index parity (VSS at even rows).
  // Replace with explicit LEF pg_pin parsing for advanced PDKs.
  row_rail_.clear();
  row_rail_.resize(grid_h_);
  for (int r = 0; r < grid_h_; ++r) {
    row_rail_[r] = (r % 2 == 0) ? NLPowerRailType::kVss : NLPowerRailType::kVdd;
  }

  // Build NegCell records from all placed instances.
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
    cell.fixed = status.isFixed();

    int db_x = 0;
    int db_y = 0;
    db_inst->getLocation(db_x, db_y);
    // Snap to grid, findBestLocation() and snapToLegal() iterate over grid
    // positions
    cell.init_x = dpl_grid->gridX(DbuX{db_x - die_xlo_}).v;
    cell.init_y = dpl_grid->gridRoundY(DbuY{db_y - die_ylo_}).v;

    // Width/height must be computed before clamping so the upper bounds
    // account for the full cell footprint (x + width <= grid_w_, etc.).
    auto* master = db_inst->getMaster();
    cell.width = std::max(
        1,
        static_cast<int>(
            std::round(static_cast<double>(master->getWidth()) / site_width_)));
    cell.height = std::max(
        1,
        static_cast<int>(std::round(static_cast<double>(master->getHeight())
                                    / row_height_)));

    // Clamp to valid grid range – gridRoundY can return grid_h_ when the
    // instance is near the top edge.  Use (grid_w_ - width) so the full
    // footprint stays within the grid (matches Opendp::legalPt behaviour).
    cell.init_x = std::max(0, std::min(cell.init_x, grid_w_ - cell.width));
    cell.init_y = std::max(0, std::min(cell.init_y, grid_h_ - cell.height));
    cell.x = cell.init_x;
    cell.y = cell.init_y;

    // gridX() / gridRoundY() are purely arithmetic and don't check whether a
    // site actually exists at the computed position.  Instances near the chip
    // boundary or in sparse-row designs can land on invalid (is_valid=false)
    // pixels pixels or on pixels that don't support this cell's site type.  Fix
    // those with a diamond search from the initial position; we only check site
    // validity here, not blockages — the negotiation part handles those.
    if (!cell.fixed) {
      odb::dbSite* site = master->getSite();
      // Check that the full cell footprint (width x height) fits on valid
      // sites.
      auto isValidSite = [&](int gx, int gy) -> bool {
        if (gx < 0 || gx + cell.width > grid_w_ || gy < 0
            || gy + cell.height > grid_h_) {
          return false;
        }
        // Site type check at the anchor row is representative for all rows.
        if (site != nullptr
            && !dpl_grid->getSiteOrientation(GridX{gx}, GridY{gy}, site)
                    .has_value()) {
          return false;
        }
        for (int dy = 0; dy < cell.height; ++dy) {
          for (int dx = 0; dx < cell.width; ++dx) {
            if (!dpl_grid->pixel(GridY{gy + dy}, GridX{gx + dx}).is_valid) {
              return false;
            }
          }
        }
        return true;
      };

      // The snapping here is actually quite similar to the "hopeless" approach
      // in original DPL.
      //  they achieve the same objective, and the previous is more simple,
      //  consider replacing this.
      if (!isValidSite(cell.init_x, cell.init_y)) {
        debugPrint(logger_,
                   utl::DPL,
                   "negotiation",
                   1,
                   "Instance {} at ({}, {}) snaps to invalid site at "
                   "({}, {}). Searching for nearest valid site.",
                   cell.db_inst->getName(),
                   db_x,
                   db_y,
                   die_xlo_ + cell.init_x * site_width_,
                   die_ylo_ + cell.init_y * row_height_);

        // Priority queue keyed on physical Manhattan distance (DBU) so the
        // search expands in true physical proximity, not grid-unit proximity.
        // One step in X = site_width_ DBU; one step in Y = row_height_ DBU.
        using PQEntry = std::tuple<int, int, int>;  // physDist, gx, gy
        std::
            priority_queue<PQEntry, std::vector<PQEntry>, std::greater<PQEntry>>
                pq;
        std::unordered_set<int> visited;

        auto tryEnqueue = [&](int gx, int gy) {
          // Leave room for the full cell footprint before enqueueing.
          if (gx < 0 || gx + cell.width > grid_w_ || gy < 0
              || gy + cell.height > grid_h_) {
            return;
          }
          if (!visited.insert(gy * grid_w_ + gx).second) {
            return;
          }
          const int dist = std::abs(gx - cell.init_x) * site_width_
                           + std::abs(gy - cell.init_y) * row_height_;
          pq.emplace(dist, gx, gy);
        };

        tryEnqueue(cell.init_x, cell.init_y);
        constexpr std::array<std::pair<int, int>, 4> kNeighbors{
            {{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};
        while (!pq.empty()) {
          auto [dist, gx, gy] = pq.top();
          pq.pop();
          if (isValidSite(gx, gy)) {
            cell.init_x = gx;
            cell.init_y = gy;
            cell.x = cell.init_x;
            cell.y = cell.init_y;
            break;
          }
          for (auto [ox, oy] : kNeighbors) {
            tryEnqueue(gx + ox, gy + oy);
          }
        }
      }
    }

    cell.rail_type = inferRailType(cell.init_y);
    // If the instance is currently flipped relative to the row's standard
    // orientation, its internal rail design is opposite of the row's bottom
    // rail.
    auto siteOrient = dpl_grid->getSiteOrientation(
        GridX{cell.init_x}, GridY{cell.init_y}, master->getSite());
    if (siteOrient.has_value() && db_inst->getOrient() != siteOrient.value()) {
      cell.rail_type = (cell.rail_type == NLPowerRailType::kVss)
                           ? NLPowerRailType::kVdd
                           : NLPowerRailType::kVss;
    }

    cell.flippable
        = master->getSymmetryX();  // X-symmetry allows vertical flip (MX)
    if (cell.height % 2 == 1) {
      // For 1-row cells, we usually assume they are flippable in most PDKs.
      cell.flippable = true;
    }

    if (padding_ != nullptr) {
      cell.pad_left = padding_->padLeft(db_inst).v;
      cell.pad_right = padding_->padRight(db_inst).v;
    }

    cells_.push_back(cell);
  }

  return true;
}

NLPowerRailType NegotiationLegalizer::inferRailType(int rowIdx) const
{
  if (rowIdx >= 0 && rowIdx < static_cast<int>(row_rail_.size())) {
    return row_rail_[rowIdx];
  }
  return NLPowerRailType::kVss;
}

void NegotiationLegalizer::buildGrid()
{
  Grid* dplGrid = opendp_->grid_.get();

  // Reset all pixels to default negotiation state.
  for (int gy = 0; gy < grid_h_; ++gy) {
    for (int gx = 0; gx < grid_w_; ++gx) {
      Pixel& pixel = dplGrid->pixel(GridY{gy}, GridX{gx});
      pixel.capacity = pixel.is_valid ? 1 : 0;
      pixel.usage = 0;
      pixel.hist_cost = 1.0;
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

  // Mark blockages and record fixed-cell usage in one pass.
  // The padded range of fixed cells is also blocked so movable cells
  // cannot violate padding constraints relative to fixed instances.
  for (const NegCell& cell : cells_) {
    if (!cell.fixed) {
      continue;
    }
    const int xBegin = effXBegin(cell);
    const int xEnd = effXEnd(cell);
    for (int dy = 0; dy < cell.height; ++dy) {
      const int gy = cell.y + dy;
      for (int gx = xBegin; gx < xEnd; ++gx) {
        if (gridExists(gx, gy)) {
          gridAt(gx, gy).capacity = 0;
          // Physical footprint carries usage=1; padding slots do not.
          if (gx >= cell.x && gx < cell.x + cell.width) {
            gridAt(gx, gy).usage = 1;
          }
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

    for (auto* box : region->getBoundaries()) {
      FenceRect r;
      r.xlo = (box->xMin() - die_xlo_) / site_width_;
      r.ylo = (box->yMin() - die_ylo_) / row_height_;
      r.xhi = (box->xMax() - die_xlo_) / site_width_;
      r.yhi = (box->yMax() - die_ylo_) / row_height_;
      fr.rects.push_back(r);
    }

    if (!fr.rects.empty()) {
      fences_.push_back(std::move(fr));
    }
  }

  // Map each instance to its fence region (if any).
  for (auto& cell : cells_) {
    if (cell.db_inst == nullptr) {
      continue;
    }
    auto* region = cell.db_inst->getRegion();
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
// Grid helpers for Negotiation Legalizer
// ===========================================================================

void NegotiationLegalizer::addUsage(int cell_idx, int delta)
{
  const NegCell& cell = cells_[cell_idx];
  const int xBegin = effXBegin(cell);
  const int xEnd = effXEnd(cell);
  for (int dy = 0; dy < cell.height; ++dy) {
    const int gy = cell.y + dy;
    for (int gx = xBegin; gx < xEnd; ++gx) {
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
  Node* node = network_->getNode(neg_cell.db_inst);
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
  Node* node = network_->getNode(neg_cell.db_inst);
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
    Node* node = network_->getNode(neg_cell.db_inst);
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
    Node* node = network_->getNode(neg_cell.db_inst);
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
    Node* node = network_->getNode(cell.db_inst);
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
  // Every row the cell spans must have real sites.
  for (int dy = 0; dy < cell.height; ++dy) {
    if (!row_has_sites_[rowIdx + dy]) {
      return false;
    }
  }
  // Verify that the cell's site type is available on the target row.
  if (cell.db_inst != nullptr && opendp_ && opendp_->grid_) {
    odb::dbSite* site = cell.db_inst->getMaster()->getSite();
    if (site != nullptr
        && !opendp_->grid_->getSiteOrientation(
            GridX{gridX}, GridY{rowIdx}, site)) {
      return false;
    }
  }
  const NLPowerRailType rowBot = row_rail_[rowIdx];
  if (cell.height % 2 == 1) {
    // Odd-height: bottom rail must match, or cell can be vertically flipped.
    return cell.flippable || (rowBot == cell.rail_type);
  }
  // Even-height: bottom boundary must be the correct rail type, and the
  // cell may only move by an even number of rows.
  return rowBot == cell.rail_type;
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

std::pair<int, int> NegotiationLegalizer::snapToLegal(int cell_idx,
                                                      int x,
                                                      int y) const
{
  const NegCell& cell = cells_[cell_idx];
  int best_x = std::max(0, std::min(x, grid_w_ - cell.width));
  int best_y = y;
  double best_dist_sq = 1e18;

  for (int r = 0; r + cell.height <= grid_h_; ++r) {
    if (!isValidRow(r, cell, x)) {
      continue;
    }

    // Find nearest valid X region in this row.
    int local_best_x = -1;
    int local_best_dx = 1e9;

    for (int tx = 0; tx + cell.width <= grid_w_; ++tx) {
      bool ok = true;
      for (int dy = 0; dy < cell.height; ++dy) {
        for (int dx = 0; dx < cell.width; ++dx) {
          if (gridAt(tx + dx, r + dy).capacity == 0) {
            ok = false;
            break;
          }
        }
        if (!ok) {
          break;
        }
      }

      if (ok) {
        const int dx = std::abs(tx - x);
        if (dx < local_best_dx) {
          local_best_dx = dx;
          local_best_x = tx;
        }
      }
    }

    if (local_best_x != -1) {
      const double dy = static_cast<double>(r - y);
      const double dx = static_cast<double>(local_best_dx);
      const double distSq = dx * dx + dy * dy;
      if (distSq < best_dist_sq) {
        best_dist_sq = distSq;
        best_x = local_best_x;
        best_y = r;
      }
    }
  }

  return {best_x, best_y};
}

// ===========================================================================
// Abacus pass
// ===========================================================================

std::vector<int> NegotiationLegalizer::runAbacus()
{
  // Build sorted order: ascending y then x.
  std::vector<int> order;
  order.reserve(cells_.size());
  for (int i = 0; i < static_cast<int>(cells_.size()); ++i) {
    if (!cells_[i].fixed) {
      order.push_back(i);
    }
  }
  std::ranges::sort(order, [this](int a, int b) {
    if (cells_[a].y != cells_[b].y) {
      return cells_[a].y < cells_[b].y;
    }
    return cells_[a].x < cells_[b].x;
  });

  // Remove movable cell usage before replanting.
  for (int i : order) {
    addUsage(i, -1);
  }

  // Snap each cell to a legal row and group by that row.
  std::vector<std::vector<int>> byRow(grid_h_);
  for (int i : order) {
    auto [sx, sy] = snapToLegal(i, cells_[i].x, cells_[i].y);
    cells_[i].x = sx;
    cells_[i].y = sy;
    if (sy >= 0 && sy < grid_h_) {
      byRow[sy].push_back(i);
    }
  }

  // Run the Abacus sweep row by row.
  for (int r = 0; r < grid_h_; ++r) {
    if (byRow[r].empty()) {
      continue;
    }
    std::ranges::sort(
        byRow[r], [this](int a, int b) { return cells_[a].x < cells_[b].x; });
    abacusRow(r, byRow[r]);
  }

  // Restore movable cell usage after placement.
  for (int i : order) {
    addUsage(i, 1);
  }

  // Sync to DPL Grid before DRC checks.
  syncAllCellsToDplGrid();

  // Collect still-illegal cells.
  std::vector<int> illegal;
  for (int i : order) {
    cells_[i].legal = isCellLegal(i);
    if (!cells_[i].legal) {
      illegal.push_back(i);
    }
  }
  return illegal;
}

void NegotiationLegalizer::abacusRow(int rowIdx, std::vector<int>& cellsInRow)
{
  std::vector<AbacusCluster> clusters;

  for (int idx : cellsInRow) {
    const NegCell& cell = cells_[idx];

    // Skip cells that violate fence or row constraints – negotiation handles
    // them later.
    if (!respectsFence(idx, cell.x, rowIdx)
        || !isValidRow(rowIdx, cell, cell.x)) {
      continue;
    }

    AbacusCluster nc;
    nc.cell_indices.push_back(idx);
    const int eff_width = cell.width + cell.pad_left + cell.pad_right;
    // Target the padded-left position so that cell.init_x lands correctly.
    nc.optimal_x = static_cast<double>(cell.init_x - cell.pad_left);
    nc.total_weight = 1.0;
    nc.total_q = nc.optimal_x;
    nc.total_width = eff_width;

    // Clamp to die boundary (padded width).
    nc.optimal_x = std::max(
        0.0, std::min(nc.optimal_x, static_cast<double>(grid_w_ - eff_width)));
    clusters.push_back(std::move(nc));
    collapseClusters(clusters, rowIdx);
  }

  // Write solved positions back to cells_.
  for (const auto& cluster : clusters) {
    assignClusterPositions(cluster, rowIdx);
  }
}

void NegotiationLegalizer::collapseClusters(
    std::vector<AbacusCluster>& clusters,
    int /*rowIdx*/)
{
  while (clusters.size() >= 2) {
    AbacusCluster& last = clusters[clusters.size() - 1];
    AbacusCluster& prev = clusters[clusters.size() - 2];

    // Solve optimal position for the last cluster.
    last.optimal_x = last.total_q / last.total_weight;
    last.optimal_x
        = std::max(0.0,
                   std::min(last.optimal_x,
                            static_cast<double>(grid_w_ - last.total_width)));

    // If the last cluster overlaps the previous one, merge them.
    if (prev.optimal_x + prev.total_width > last.optimal_x) {
      prev.total_weight += last.total_weight;
      prev.total_q += last.total_q;
      prev.total_width += last.total_width;
      for (int idx : last.cell_indices) {
        prev.cell_indices.push_back(idx);
      }
      prev.optimal_x = prev.total_q / prev.total_weight;
      prev.optimal_x
          = std::max(0.0,
                     std::min(prev.optimal_x,
                              static_cast<double>(grid_w_ - prev.total_width)));
      clusters.pop_back();
    } else {
      break;
    }
  }

  // Re-solve the top cluster after any merge.
  if (!clusters.empty()) {
    AbacusCluster& top = clusters.back();
    top.optimal_x = top.total_q / top.total_weight;
    top.optimal_x
        = std::max(0.0,
                   std::min(top.optimal_x,
                            static_cast<double>(grid_w_ - top.total_width)));
  }
}

void NegotiationLegalizer::assignClusterPositions(const AbacusCluster& cluster,
                                                  int rowIdx)
{
  // cluster.optimal_x is the padded-left edge of the cluster.
  int paddedX = static_cast<int>(std::round(cluster.optimal_x));
  paddedX = std::max(0, std::min(paddedX, grid_w_ - cluster.total_width));

  for (int idx : cluster.cell_indices) {
    const int eff_width
        = cells_[idx].width + cells_[idx].pad_left + cells_[idx].pad_right;
    // Physical left edge = padded-left edge + pad_left of this cell.
    cells_[idx].x = paddedX + cells_[idx].pad_left;
    cells_[idx].y = rowIdx;
    paddedX += eff_width;
    if (debug_observer_ && cells_[idx].db_inst != nullptr) {
      debug_observer_->drawSelected(cells_[idx].db_inst, false);
      if (opendp_->iterative_debug_) {
        pushNegotiationPixels();
        debug_observer_->redrawAndPause();
      }
    }
  }
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
    Node* node = network_->getNode(cell.db_inst);
    if (node != nullptr
        && !opendp_->drc_engine_->checkDRC(
            node, GridX{cell.x}, GridY{cell.y}, node->getOrient())) {
      return false;
    }
  } else {
    logger_->report("DRC objects not available!");
    return false;
  }

  const int xBegin = effXBegin(cell);
  const int xEnd = effXEnd(cell);
  for (int dy = 0; dy < cell.height; ++dy) {
    for (int gx = xBegin; gx < xEnd; ++gx) {
      if (gridAt(gx, cell.y + dy).capacity == 0
          || gridAt(gx, cell.y + dy).overuse() > 0) {
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
      sum += cell.displacement();
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
      mx = std::max(mx, cell.displacement());
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

}  // namespace dpl
