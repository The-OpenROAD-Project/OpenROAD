//////////////////////////////////////////////////////////////////////////////
// HybridLegalizer.h
//
// Hybrid Abacus + Negotiation-Based Legalizer for OpenROAD (opendp module)
//
// Pipeline:
//   1. Abacus pass  — fast, near-optimal for single/uncongested cells
//   2. Negotiation  — iterative rip-up/replace for remaining violations
//              (multirow cells, congested regions, fence boundaries)
//
// Supports:
//   - Mixed-cell-height  (1x / 2x / 3x / 4x row heights)
//   - Power-rail alignment (VDD/VSS)
//   - Fence region constraints
//
// Integration target: src/dpl/src/ inside OpenROAD
//
// Dependencies (all present in OpenROAD's opendp module):
//   odb::dbDatabase, odb::dbInst, odb::dbRow
//   utl::Logger
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "odb/db.h"
#include "utl/Logger.h"

namespace dpl {

using odb::dbDatabase;
using odb::dbInst;
using odb::dbRow;

//----------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------
constexpr int kInfCost       = std::numeric_limits<int>::max() / 2;
constexpr int kHorizWindow   = 9;   // sites, current row  (from NBLG paper)
constexpr int kAdjWindow     = 3;   // sites, adjacent rows
constexpr int kMaxIterNeg    = 600; // negotiation iterations (phase 1)
constexpr int kMaxIterNeg2   = 3000;// negotiation iterations (phase 2)
constexpr int kIsolationPt   = 1;   // I parameter from NBLG
constexpr double kMfDefault  = 1.5; // max-disp penalty multiplier
constexpr int kThDefault     = 30;  // max-disp threshold (sites)
constexpr double kHfDefault  = 1.0; // history cost increment factor
constexpr double kAlpha      = 0.7;
constexpr double kBeta       = 10.0;
constexpr double kGamma      = 0.005;
constexpr int kIth           = 300; // pf ramp-up threshold iteration

//----------------------------------------------------------------------------
// PowerRailType
//----------------------------------------------------------------------------
enum class PowerRailType { VSS = 0, VDD = 1 };

//----------------------------------------------------------------------------
// FenceRegion  — axis-aligned rectangle, cells must stay inside
//----------------------------------------------------------------------------
struct FenceRegion {
  int id;
  // sub-rectangles (a fence may be non-contiguous)
  struct Rect { int xlo, ylo, xhi, yhi; };
  std::vector<Rect> rects;

  bool contains(int x, int y, int w, int h) const;
  // Returns nearest sub-rect lower-left that fits cell (w,h)
  Rect nearestRect(int x, int y) const;
};

//----------------------------------------------------------------------------
// Cell  — internal representation
//----------------------------------------------------------------------------
struct Cell {
  dbInst*      inst       = nullptr;
  int          initX      = 0;    // initial position (sites)
  int          initY      = 0;    // initial position (rows)
  int          x          = 0;    // current legal position (sites)
  int          y          = 0;    // current legal position (rows)
  int          width      = 0;    // in sites
  int          height     = 0;    // in row units (1,2,3,4)
  bool         fixed      = false;
  PowerRailType railType  = PowerRailType::VSS;
  int          fenceId    = -1;   // -1 = default region
  bool         flippable  = true; // odd-height cells can flip

  // Abacus cluster linkage
  int          clusterHead = -1;
  double       clusterWeight = 0.0;
  double       clusterOptX   = 0.0;

  // Negotiation state
  bool         legal      = false;
  int          overuse    = 0;    // sum of overuse across occupied grids

  int displacement() const {
    return std::abs(x - initX) + std::abs(y - initY);
  }
};

//----------------------------------------------------------------------------
// Grid  — one placement site
//----------------------------------------------------------------------------
struct Grid {
  int  usage    = 0;
  int  capacity = 1;   // 0 = blockage
  double histCost = 0.0;

  int overuse() const { return std::max(usage - capacity, 0); }
};

//----------------------------------------------------------------------------
// AbacusCluster  — used during Abacus row sweep
//----------------------------------------------------------------------------
struct AbacusCluster {
  int    firstCellIdx = -1;
  int    lastCellIdx  = -1;
  double optX         = 0.0;   // optimal (possibly fractional) left edge
  double totalWeight  = 0.0;
  double totalQ       = 0.0;   // Σ w_i * x_i^0
  int    width        = 0;     // total cluster width (sites)
};

//----------------------------------------------------------------------------
// HybridLegalizer
//----------------------------------------------------------------------------
class HybridLegalizer {
 public:
  HybridLegalizer(dbDatabase* db, utl::Logger* logger);
  ~HybridLegalizer() = default;

  // Main entry point — called instead of (or after) existing opendp legalizer
  void legalize(float targetDensity = 1.0f);

  // Tuning knobs (optional, defaults match paper)
  void setMf(double mf)           { mf_ = mf; }
  void setTh(int th)              { th_ = th; }
  void setMaxIterNeg(int n)       { maxIterNeg_ = n; }
  void setHorizWindow(int w)      { horizWindow_ = w; }
  void setAdjWindow(int w)        { adjWindow_ = w; }
  void setNumThreads(int n)       { numThreads_ = n; }

  // Metrics (available after legalize())
  double avgDisplacement()  const;
  int    maxDisplacement()  const;
  int    numViolations()    const;

 private:
  //------ Initialisation ----------------------------------------------------
  void initFromDB();
  void buildGrid();
  void initFenceRegions();
  PowerRailType inferRailType(dbInst* inst) const;
  int  siteWidth()  const { return siteWidth_; }
  int  rowHeight()  const { return rowHeight_; }

  //------ Abacus Pass --------------------------------------------------------
  // Returns set of cell indices that are still illegal after Abacus
  std::vector<int> runAbacus();

  // Per-row Abacus
  void abacusRow(int rowIdx, std::vector<int>& cellsInRow);

  // Abacus cluster operations
  void   addCellToCluster(AbacusCluster& c, int cellIdx);
  void   collapseClusters(std::vector<AbacusCluster>& clusters, int rowIdx);
  double clusterCost(const AbacusCluster& c, int cellIdx, int trialX) const;

  // Check if a cell is legal after Abacus placement
  bool isCellLegal(int cellIdx) const;

  //------ Negotiation Pass ---------------------------------------------------
  void runNegotiation(const std::vector<int>& illegalCells);

  // Single negotiation iteration over given cell set
  // Returns number of remaining overflows
  int  negotiationIter(std::vector<int>& activeCells, int iter,
                       bool updateHistory);

  // Rip up cell from grid
  void ripUp(int cellIdx);

  // Place cell at (x,y) and update grid usage
  void place(int cellIdx, int x, int y);

  // Find minimum-cost grid location for cell within search window
  // Returns {bestX, bestY}
  std::pair<int,int> findBestLocation(int cellIdx) const;

  // Negotiation cost for placing cellIdx at (x,y)
  double negotiationCost(int cellIdx, int x, int y) const;

  // Target (displacement) cost — Eq. 11 from NBLG
  double targetCost(int cellIdx, int x, int y) const;

  // Adaptive penalty function pf — Eq. 14 from NBLG
  double adaptivePf(int iter) const;

  // Update history costs on overused grids — Eq. 12 from NBLG
  void updateHistoryCosts();

  // Sort cells by negotiation order (overuse desc, height asc, width asc)
  void sortByNegotiationOrder(std::vector<int>& cellIndices) const;

  //------ Constraint Helpers ------------------------------------------------
  // True if (x,y) is a valid row start for cell with given height/rail
  bool isValidRow(int rowIdx, const Cell& cell) const;

  // True if cell fits inside its fence region at (x,y)
  bool respectsFence(int cellIdx, int x, int y) const;

  // True if (x,y)..(x+w-1, y+h-1) grids are within die
  bool inDie(int x, int y, int w, int h) const;

  // Snap x to nearest site, y to nearest valid row for cell
  std::pair<int,int> snapToLegal(int cellIdx, int x, int y) const;

  //------ Post-Optimisation -------------------------------------------------
  void greedyImprove(int passes = 5);
  void cellSwap();

  //------ Grid accessors ----------------------------------------------------
  Grid&       gridAt(int x, int y)       { return grid_[y * gridW_ + x]; }
  const Grid& gridAt(int x, int y) const { return grid_[y * gridW_ + x]; }

  bool gridExists(int x, int y) const {
    return x >= 0 && x < gridW_ && y >= 0 && y < gridH_;
  }

  // Increment/decrement usage for all grids under cell footprint
  void addUsage(int cellIdx, int delta);

  //------ Data --------------------------------------------------------------
  dbDatabase*   db_      = nullptr;
  utl::Logger*  logger_  = nullptr;

  // Layout parameters
  int siteWidth_  = 0;
  int rowHeight_  = 0;
  int dieXlo_     = 0;
  int dieYlo_     = 0;
  int dieXhi_     = 0;
  int dieYhi_     = 0;
  int gridW_      = 0;   // die width  in sites
  int gridH_      = 0;   // die height in rows

  // Cells and grid
  std::vector<Cell>        cells_;
  std::vector<Grid>        grid_;
  std::vector<FenceRegion> fences_;

  // Row → list of cell indices (populated during Abacus)
  std::vector<std::vector<int>> rowCells_;

  // Power rail of each row (indexed by row index)
  std::vector<PowerRailType> rowRail_;

  // Algorithm parameters
  double mf_          = kMfDefault;
  int    th_          = kThDefault;
  int    maxIterNeg_  = kMaxIterNeg;
  int    horizWindow_ = kHorizWindow;
  int    adjWindow_   = kAdjWindow;
  int    numThreads_  = 1;
};

}  // namespace dpl
