// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2026, The OpenROAD Authors

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
constexpr int kInfCost = std::numeric_limits<int>::max() / 2;
constexpr int kHorizWindow = 20;    // search width, current row (sites)
constexpr int kAdjWindow = 5;       // search width, adjacent rows
constexpr int kMaxIterNeg = 400;    // negotiation phase-1 limit
constexpr int kMaxIterNeg2 = 1000;  // negotiation phase-2 limit
constexpr int kIsolationPt = 1;     // isolation-point parameter I
constexpr double kMfDefault = 1.5;  // max-disp penalty multiplier
constexpr int kThDefault = 30;      // max-disp threshold (sites)
constexpr double kHfDefault = 1.0;  // history-cost increment factor
constexpr double kAlpha = 0.7;      // adaptive-pf α
constexpr double kBeta = 10.0;      // adaptive-pf β
constexpr double kGamma = 0.005;    // adaptive-pf γ
constexpr int kIth = 300;           // pf ramp-up threshold iteration

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
  NLPowerRailType rail_type{NLPowerRailType::kVss};
  int fence_id{-1};      // -1 → default region
  bool flippable{true};  // odd-height cells may require fliping for moving
  bool legal{false};     // updated each negotiation iteration

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
  // at the start of each call (cells_, grid_, fences_, row_rail_ are cleared).
  void legalize();

  // Pass positions back to the DPL original structure.
  void setDplPositions();

  // Tuning knobs (all have paper-default values)
  void setRunAbacus(bool run) { run_abacus_ = run; }
  void setMf(double mf) { max_disp_multiplier_ = mf; }
  void setTh(int th) { max_disp_threshold_ = th; }
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
                      bool updateHistory);
  void ripUp(int cell_idx);
  void place(int cell_idx, int x, int y);
  [[nodiscard]] std::pair<int, int> findBestLocation(int cell_idx,
                                                     int iter = 0) const;
  [[nodiscard]] double negotiationCost(int cell_idx, int x, int y) const;
  [[nodiscard]] double targetCost(int cell_idx, int x, int y) const;
  [[nodiscard]] double adaptivePf(int iter) const;
  void updateHistoryCosts();
  void updateDrcHistoryCosts(const std::vector<int>& activeCells);
  void sortByNegotiationOrder(std::vector<int>& indices) const;

  // Post-optimisation
  void greedyImprove(int passes);
  void cellSwap();
  void diamondRecovery(const std::vector<int>& activeCells);

  // Constraint helpers
  [[nodiscard]] bool isValidRow(int rowIdx,
                                const NegCell& cell,
                                int gridX) const;
  [[nodiscard]] bool respectsFence(int cell_idx, int x, int y) const;
  [[nodiscard]] bool inDie(int x, int y, int w, int h) const;
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
  int row_height_{0};
  int die_xlo_{0};
  int die_ylo_{0};
  int die_xhi_{0};
  int die_yhi_{0};
  int grid_w_{0};
  int grid_h_{0};

  std::vector<NegCell> cells_;
  std::vector<FenceRegion> fences_;
  std::vector<NLPowerRailType> row_rail_;
  std::vector<bool>
      row_has_sites_;  // true when at least one DB row exists at y

  double max_disp_multiplier_{kMfDefault};  // mf on the paper
  int max_disp_threshold_{kThDefault};      // th on the paper
  int max_iter_neg_{kMaxIterNeg};
  int horiz_window_{kHorizWindow};
  int adj_window_{kAdjWindow};
  int num_threads_{1};
  bool run_abacus_{false};

  // Mutable profiling accumulators for findBestLocation breakdown (seconds).
  mutable double prof_init_search_s_{0};
  mutable double prof_curr_search_s_{0};
  mutable double prof_filter_s_{0};
  mutable double prof_neg_cost_s_{0};
  mutable double prof_drc_s_{0};
  mutable int prof_candidates_evaluated_{0};
  mutable int prof_candidates_filtered_{0};
};

}  // namespace dpl
