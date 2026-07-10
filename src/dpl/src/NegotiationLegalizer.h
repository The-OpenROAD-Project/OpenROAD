// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2026, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
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
constexpr int kInfCost = std::numeric_limits<int>::max() / 2;
constexpr int kSiteSearchWindow = 20;  // search width, current row (sites)
constexpr int kRowSearchWindow = 5;    // search width, adjacent rows
constexpr double kDrcPenalty = 5.0;    // base DRC penalty (scaled per iter)
constexpr int kMaxIterNeg = 400;       // negotiation phase-1 limit
constexpr int kMaxIterNeg2 = 1000;     // negotiation phase-2 limit
constexpr int kIsolationPt = 1;        // isolation-point parameter I
constexpr double kMfDefault = 1.5;     // max-disp penalty multiplier
constexpr int kThDefault = 30;         // max-disp threshold (sites)
constexpr double kHfDefault = 1.0;     // history-cost increment factor
constexpr double kAlpha = 0.7;         // adaptive-pf α
constexpr double kBeta = 10.0;         // adaptive-pf β
constexpr double kGamma = 0.005;       // adaptive-pf γ
constexpr int kIth = 300;              // pf ramp-up threshold iteration

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
// NegCell – per-instance legalisation state
// ---------------------------------------------------------------------------
struct NegCell
{
  odb::dbInst* db_inst{nullptr};

  int init_x{0};     // position after global placement (sites)
  int init_y{0};     // position after global placement (rows)
  int x{0};          // current legalised position (sites)
  int y{0};          // current legalised position (rows)
  int width{0};      // footprint width  (sites)
  int height{0};     // footprint height (row units: 1–4)
  int pad_left{0};   // left padding (sites)
  int pad_right{0};  // right padding (sites)

  bool fixed{false};
  int fence_id{-1};   // -1 → default region
  bool legal{false};  // updated each negotiation iteration

  [[nodiscard]] int displacement() const
  {
    return std::abs(x - init_x) + std::abs(y - init_y);
  }
};

// ---------------------------------------------------------------------------
// AbacusCluster – transient state during the Abacus row sweep
// ---------------------------------------------------------------------------
struct AbacusCluster
{
  std::vector<int> cell_indices;  // ordered left-to-right within the row
  double optimal_x{0.0};          // solved optimal left-edge (fractional)
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
  // at the start of each call (cells_, grid_, fences_ are cleared).
  void legalize();

  // Pass positions back to the DPL original structure.
  void commitNegotiationPosToDpl();

  // Tuning knobs (all have paper-default values)
  void setRunAbacus(bool run) { run_abacus_ = run; }
  void setMf(double mf) { max_disp_multiplier_ = mf; }
  void setTh(int th) { max_disp_threshold_ = th; }
  void setMaxIterNeg(int n) { max_iter_neg_ = n; }
  void setSiteSearchWindow(int w) { site_search_window_ = w; }
  void setRowSearchWindow(int w) { row_search_window_ = w; }
  void setDrcPenalty(double p) { drc_penalty_ = p; }
  void setNumThreads(int n) { num_threads_ = n; }
  // When set, site_search_window_/row_search_window_ are used as hard
  // ranges: disables both window extensions.
  void setDisableWindowExtension(bool disable)
  {
    disable_window_extension_ = disable;
  }

  // Metrics (valid after legalize())
  [[nodiscard]] double avgDisplacement() const;
  [[nodiscard]] int maxDisplacement() const;
  [[nodiscard]] int numViolations() const;

 private:
  // Initialisation
  bool initFromDb();
  void buildGrid();
  void initFenceRegions();
  void commitNegotiationPosToOdb();  // Write current cell positions to ODB (for
                                     // GUI updates)
  void pushNegotiationPixels();
  void debugPause(const std::string& msg);

  // Abacus pass
  [[nodiscard]] std::vector<int> runAbacus();
  void abacusRow(int rowIdx, std::vector<int>& cellsInRow);
  void collapseClusters(std::vector<AbacusCluster>& clusters, int rowIdx);
  void assignClusterPositions(const AbacusCluster& cluster, int rowIdx);
  [[nodiscard]] bool isCellLegal(int cell_idx) const;

  // Negotiation pass
  void runNegotiation(const std::vector<int>& illegalCells);
  int negotiationIter(std::vector<int>& activeCells,
                      int iter,
                      bool updateHistory,
                      bool print_row);
  void ripUp(int cell_idx);
  void place(int cell_idx, int x, int y);
  [[nodiscard]] std::pair<int, int> findBestLocation(int cell_idx,
                                                     int iter = 0) const;
  [[nodiscard]] double negotiationCost(int cell_idx, int x, int y) const;
  [[nodiscard]] double targetCost(int cell_idx, int x, int y) const;
  [[nodiscard]] double adaptivePf(int iter) const;
  void updateHistoryCosts(const std::vector<int>& activeCells);
  void updateDrcHistoryCosts(const std::vector<int>& activeCells);
  void sortByNegotiationOrder(std::vector<int>& indices) const;

  // Print a stuck-cell summary (overall counts + per-height breakdown).
  // No-op when both counts are zero.
  void printStuckSummary(
      const char* label,
      int no_cand_count,
      int same_pos_count,
      const std::unordered_map<int, int>& no_cand_by_height,
      const std::unordered_map<int, int>& same_pos_by_height) const;

  // Post-optimisation
  void greedyImprove(int passes);
  void cellSwap();
  void diamondRecovery(const std::vector<int>& activeCells);

  // Constraint helpers
  [[nodiscard]] bool isValidRow(int rowIdx,
                                const NegCell& cell,
                                int gridX) const;
  // Collect up to `count_per_side` rows on each side of `seed_y`
  // that can host the cell somewhere in [x_lo, x_hi]. A side
  // stops after `max_scan` steps or at an off-core wall; quota it could not
  // fill extends the other side.
  [[nodiscard]] std::vector<int> verticalWindowRows(const NegCell& cell,
                                                    int seed_y,
                                                    int x_lo,
                                                    int x_hi,
                                                    int count_per_side,
                                                    int max_scan) const;
  [[nodiscard]] bool respectsFence(int cell_idx, int x, int y) const;
  [[nodiscard]] bool inDie(int x, int y, int w, int h) const;
  [[nodiscard]] int effectiveSiteWindow(const NegCell& cell) const;
  [[nodiscard]] int effectiveRowCap(const NegCell& cell) const;

  // The rectangular candidate region findBestLocation scans around an anchor
  // point. The horizontal reach (dx_lo..dx_hi, inclusive offsets from
  // anchor_x) is already shifted away from any macro/off-core wall, and `rows`
  // is the set of valid rows to visit, vertically extended past an off-core
  // wall. Built once per anchor by buildSearchWindow().
  struct SearchWindow
  {
    int dx_lo{0};
    int dx_hi{0};
    std::vector<int> rows;
  };
  [[nodiscard]] SearchWindow buildSearchWindow(const NegCell& cell,
                                               int anchor_x,
                                               int anchor_y) const;

  // Asymmetric X search bounds around base_x on row target_y. When a macro or
  // the core boundary cuts one side of the symmetric [-site_window,
  // +site_window] window short, the lost reach is shifted to the opposite side
  // so the same number of candidate sites is still explored. Returns the
  // inclusive (dx_lo, dx_hi) offsets.
  [[nodiscard]] std::pair<int, int> horizontalWindowBounds(
      const NegCell& cell,
      int base_x,
      int target_y,
      int site_window) const;
  [[nodiscard]] std::pair<int, int> snapToLegal(int cell_idx,
                                                int x,
                                                int y) const;

  // DPL Grid synchronisation helpers – keep the Opendp pixel grid in sync
  // with NegotiationLegalizer cell positions so that PlacementDRC neighbour
  // lookups (edge spacing, padding, one-site gaps) see correct data.
  void syncCellToDplGrid(int cell_idx);
  void eraseCellFromDplGrid(int cell_idx);
  void syncAllCellsToDplGrid();

  // Pixel helpers – use the main DPL grid.
  Pixel& gridAt(int x, int y)
  {
    return opendp_->grid_->pixel(GridY{y}, GridX{x});
  }
  [[nodiscard]] const Pixel& gridAt(int x, int y) const
  {
    return opendp_->grid_->pixel(GridY{y}, GridX{x});
  }
  [[nodiscard]] bool gridExists(int x, int y) const
  {
    return x >= 0 && x < grid_w_ && y >= 0 && y < grid_h_;
  }
  void addUsage(int cell_idx, int delta);

  // Effective padded footprint helpers (inclusive of padding zones).
  [[nodiscard]] int effXBegin(const NegCell& cell) const
  {
    return std::max(0, cell.x - cell.pad_left);
  }
  [[nodiscard]] int effXEnd(const NegCell& cell) const
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
  int die_xlo_{0};
  int die_ylo_{0};
  int die_xhi_{0};
  int die_yhi_{0};
  int grid_w_{0};
  int grid_h_{0};

  std::vector<NegCell> cells_;
  std::vector<FenceRegion> fences_;
  std::vector<bool>
      row_has_sites_;  // true when at least one DB row exists at y

  // Reusable scratch set for updateHistoryCosts() pixel deduplication,
  // kept as a member so the per-iteration allocation is amortized.
  std::unordered_set<int> hist_seen_pixels_;

  double max_disp_multiplier_{kMfDefault};  // mf on the paper
  int max_disp_threshold_{kThDefault};      // th on the paper
  int max_iter_neg_{kMaxIterNeg};
  int site_search_window_{kSiteSearchWindow};
  int row_search_window_{kRowSearchWindow};
  int current_iter_{0};  // updated at the start of each negotiationIter call

  // Last-iteration stats, kept so runNegotiation can print the final row.
  int last_iter_{-1};
  int last_printed_iter_{-1};
  int last_violations_{0};
  int last_illegal_cells_{0};
  int last_illegal_sites_{0};

  // Cells that actually changed position during the current negotiation
  // iteration. Passed to the debug observer so cells from prior iterations
  // are rendered in grey while current-iteration movers keep directional
  // colors.
  std::unordered_set<odb::dbInst*> current_iter_movers_;
  double drc_penalty_{kDrcPenalty};
  int num_threads_{1};
  bool run_abacus_{false};
  bool disable_window_extension_{false};

  // Mutable profiling accumulators for findBestLocation breakdown (seconds).
  mutable double prof_init_search_s_{0};
  mutable double prof_curr_search_s_{0};
  mutable double prof_filter_s_{0};
  mutable double prof_neg_cost_s_{0};
  mutable double prof_drc_s_{0};
  mutable int prof_candidates_evaluated_{0};
  mutable int prof_candidates_filtered_{0};

  // Stuck-cell tallies for the current runNegotiation call. Reset at the
  // start of runNegotiation and printed at the end. The per-height maps are
  // keyed by cell.height (row units).
  mutable int stuck_no_candidate_count_{0};
  mutable int stuck_same_pos_count_{0};
  mutable std::unordered_map<int, int> stuck_no_candidate_by_height_;
  mutable std::unordered_map<int, int> stuck_same_pos_by_height_;

  // Per-iteration variants. Reset at the start of each negotiationIter and
  // printed at the end of that iteration.
  mutable int stuck_no_candidate_count_iter_{0};
  mutable int stuck_same_pos_count_iter_{0};
  mutable std::unordered_map<int, int> stuck_no_candidate_by_height_iter_;
  mutable std::unordered_map<int, int> stuck_same_pos_by_height_iter_;
};

}  // namespace dpl
