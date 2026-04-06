///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, The OpenROAD Authors
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its contributors
//   may be used to endorse or promote products derived from this software
//   without specific prior written permission.
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
///////////////////////////////////////////////////////////////////////////////

// NegotiationLegalizer.h
//
// Negotiation-Based Legalizer for OpenROAD (dpl module).
//
// Pipeline:
//   1. Abacus pass   – fast, near-optimal for uncongested single/multi-row
//   2. Negotiation   – iterative rip-up/replace for remaining violations
//
// Supports mixed-cell-height (1x–4x), power-rail alignment, fence regions.
// Integration target: src/dpl/src/ inside the OpenROAD repository.

#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "infrastructure/Grid.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace dpl {

class DplObserver;
class Opendp;
class Padding;
class Node;
class Network;
class Group;
class Master;
class Edge;

// ---------------------------------------------------------------------------
// Constants  (defaults match the NBLG paper)
// ---------------------------------------------------------------------------
inline constexpr int kInfCost = std::numeric_limits<int>::max() / 2;
inline constexpr int kHorizWindow = 20;     // search width, current row (sites)
inline constexpr int kAdjWindow = 5;       // search width, adjacent rows
inline constexpr int kMaxIterNeg = 400;    // negotiation phase-1 limit
inline constexpr int kMaxIterNeg2 = 1000;  // negotiation phase-2 limit
inline constexpr int kIsolationPt = 1;     // isolation-point parameter I
inline constexpr double kMfDefault = 1.5;  // max-disp penalty multiplier
inline constexpr int kThDefault = 30;      // max-disp threshold (sites)
inline constexpr double kHfDefault = 1.0;  // history-cost increment factor
inline constexpr double kAlpha = 0.7;      // adaptive-pf α
inline constexpr double kBeta = 10.0;      // adaptive-pf β
inline constexpr double kGamma = 0.005;    // adaptive-pf γ
inline constexpr int kIth = 300;           // pf ramp-up threshold iteration

// ---------------------------------------------------------------------------
// NLPowerRailType
// ---------------------------------------------------------------------------
enum class NLPowerRailType
{
  kVss = 0,
  kVdd = 1
};

// ---------------------------------------------------------------------------
// FenceRect / FenceRegion
// ---------------------------------------------------------------------------
struct FenceRect
{
  int xlo{0};
  int ylo{0};
  int xhi{0};
  int yhi{0};
};

struct FenceRegion
{
  int id{-1};
  std::vector<FenceRect> rects;

  // True when footprint [x, x+w) × [y, y+h) lies inside at least one rect.
  [[nodiscard]] bool contains(int x, int y, int w, int h) const;

  // Sub-rectangle whose centre is nearest (cx, cy).
  [[nodiscard]] FenceRect nearestRect(int cx, int cy) const;
};

// ---------------------------------------------------------------------------
// HLCell – per-instance legalisation state
// ---------------------------------------------------------------------------
struct HLCell
{
  odb::dbInst* db_inst{nullptr};

  int init_x{0};   // position after global placement (sites)
  int init_y{0};   // position after global placement (rows)
  int x{0};       // current legalised position (sites)
  int y{0};       // current legalised position (rows)
  int width{0};    // footprint width  (sites)
  int height{0};   // footprint height (row units: 1–4)
  int pad_left{0};  // left padding (sites)
  int pad_right{0}; // right padding (sites)

  bool fixed{false};
  NLPowerRailType rail_type{NLPowerRailType::kVss};
  int fence_id{-1};       // -1 → default region
  bool flippable{true};  // odd-height cells may flip vertically
  bool legal{false};     // updated each negotiation iteration

  [[nodiscard]] int displacement() const
  {
    return std::abs(x - init_x) + std::abs(y - init_y);
  }
};

// Removed HLGrid struct - using dpl::Pixel instead.

// ---------------------------------------------------------------------------
// AbacusCluster – transient state during the Abacus row sweep
// ---------------------------------------------------------------------------
struct AbacusCluster
{
  std::vector<int> cell_indices;  // ordered left-to-right within the row
  double opt_x{0.0};              // solved optimal left-edge (fractional)
  double total_weight{0.0};
  double total_q{0.0};  // Σ w_i * x_i^0
  int total_width{0};   // Σ cell widths (sites)
};

// ---------------------------------------------------------------------------
// NegotiationLegalizer
// ---------------------------------------------------------------------------
class NegotiationLegalizer
{
 public:
  NegotiationLegalizer(Opendp* opendp,
                  odb::dbDatabase* db,
                  utl::Logger* logger,
                  const Padding* padding = nullptr,
                  DplObserver* debug_observer = nullptr,
                  Network* network = nullptr);
  ~NegotiationLegalizer() = default;

  NegotiationLegalizer(const NegotiationLegalizer&) = delete;
  NegotiationLegalizer& operator=(const NegotiationLegalizer&) = delete;
  NegotiationLegalizer(NegotiationLegalizer&&) = delete;
  NegotiationLegalizer& operator=(NegotiationLegalizer&&) = delete;

  // Main entry point – call instead of (or after) the existing opendp path.
  // May be called multiple times on the same object; internal state is reset
  // at the start of each call (cells_, grid_, fences_, row_rail_ are cleared).
  void legalize();

  // Pass positions back to the DPL original structure.
  void setDplPositions();

  // Tuning knobs (all have paper-default values)
  void setRunAbacus(bool run) { run_abacus_ = run; }
  void setMf(double mf) { mf_ = mf; }
  void setTh(int th) { th_ = th; }
  void setMaxIterNeg(int n) { max_iter_neg_ = n; }
  void setHorizWindow(int w) { horiz_window_ = w; }
  void setAdjWindow(int w) { adj_window_ = w; }
  void setNumThreads(int n) { num_threads_ = n; }

  // Metrics (valid after legalize())
  [[nodiscard]] double avgDisplacement() const;
  [[nodiscard]] int maxDisplacement() const;
  [[nodiscard]] int numViolations() const;

 private:
  // Initialisation
  bool initFromDb();
  void buildGrid();
  void initFenceRegions();
  [[nodiscard]] NLPowerRailType inferRailType(int rowIdx) const;
  void flushToDb();  // Write current cell positions to ODB (for GUI updates)
  void pushNegotiationPixels();  // Send grid state to debug observer for rendering
  void debugPause(const std::string& msg);  // setDplPositions + pushNegotiationPixels + redrawAndPause

  // Abacus pass
  [[nodiscard]] std::vector<int> runAbacus();
  void abacusRow(int rowIdx, std::vector<int>& cellsInRow);
  void collapseClusters(std::vector<AbacusCluster>& clusters, int rowIdx);
  void assignClusterPositions(const AbacusCluster& cluster, int rowIdx);
  [[nodiscard]] bool isCellLegal(int cellIdx) const;

  // Negotiation pass
  void runNegotiation(const std::vector<int>& illegalCells);
  int negotiationIter(std::vector<int>& activeCells,
                      int iter,
                      bool updateHistory);
  void ripUp(int cellIdx);
  void place(int cellIdx, int x, int y);
  [[nodiscard]] std::pair<int, int> findBestLocation(int cellIdx,
                                                      int iter = 0) const;
  [[nodiscard]] double negotiationCost(int cellIdx, int x, int y) const;
  [[nodiscard]] double targetCost(int cellIdx, int x, int y) const;
  [[nodiscard]] double adaptivePf(int iter) const;
  void updateHistoryCosts();
  void updateDrcHistoryCosts(const std::vector<int>& activeCells);
  void sortByNegotiationOrder(std::vector<int>& indices) const;

  // Post-optimisation
  void greedyImprove(int passes);
  void cellSwap();
  void diamondRecovery(const std::vector<int>& activeCells);

  // Constraint helpers
  [[nodiscard]] bool isValidRow(int rowIdx, const HLCell& cell, int gridX) const;
  [[nodiscard]] bool respectsFence(int cellIdx, int x, int y) const;
  [[nodiscard]] bool inDie(int x, int y, int w, int h) const;
  [[nodiscard]] std::pair<int, int> snapToLegal(int cellIdx,
                                                int x,
                                                int y) const;

  // DPL Grid synchronisation helpers – keep the Opendp pixel grid in sync
  // with NegotiationLegalizer cell positions so that PlacementDRC neighbour
  // lookups (edge spacing, padding, one-site gaps) see correct data.
  void syncCellToDplGrid(int cellIdx);
  void eraseCellFromDplGrid(int cellIdx);
  void syncAllCellsToDplGrid();

  // Pixel helpers – use the main DPL grid.
  Pixel& gridAt(int x, int y) { return opendp_->grid_->pixel(GridY{y}, GridX{x}); }
  [[nodiscard]] const Pixel& gridAt(int x, int y) const
  {
    return opendp_->grid_->pixel(GridY{y}, GridX{x});
  }
  [[nodiscard]] bool gridExists(int x, int y) const
  {
    return x >= 0 && x < grid_w_ && y >= 0 && y < grid_h_;
  }
  void addUsage(int cellIdx, int delta);

  // Effective padded footprint helpers (inclusive of padding zones).
  [[nodiscard]] int effXBegin(const HLCell& cell) const
  {
    return std::max(0, cell.x - cell.pad_left);
  }
  [[nodiscard]] int effXEnd(const HLCell& cell) const
  {
    return std::min(grid_w_, cell.x + cell.width + cell.pad_right);
  }

  // Data
  Opendp* opendp_{nullptr};
  odb::dbDatabase* db_{nullptr};
  utl::Logger* logger_{nullptr};
  const Padding* padding_{nullptr};
  DplObserver* debug_observer_{nullptr};
  Network* network_{nullptr};

  int site_width_{0};
  int row_height_{0};
  int die_xlo_{0};
  int die_ylo_{0};
  int die_xhi_{0};
  int die_yhi_{0};
  int grid_w_{0};
  int grid_h_{0};

  std::vector<HLCell> cells_;
  std::vector<FenceRegion> fences_;
  std::vector<NLPowerRailType> row_rail_;
  std::vector<bool> row_has_sites_;  // true when at least one DB row exists at y

  double mf_{kMfDefault};
  int th_{kThDefault};
  int max_iter_neg_{kMaxIterNeg};
  int horiz_window_{kHorizWindow};
  int adj_window_{kAdjWindow};
  int num_threads_{1};
  bool run_abacus_{false};

  // Mutable profiling accumulators for findBestLocation breakdown.
  mutable double prof_init_search_ns_{0};
  mutable double prof_curr_search_ns_{0};
  mutable double prof_snap_ns_{0};
  mutable double prof_filter_ns_{0};
  mutable double prof_neg_cost_ns_{0};
  mutable double prof_drc_ns_{0};
  mutable int prof_candidates_evaluated_{0};
  mutable int prof_candidates_filtered_{0};
};

}  // namespace dpl
