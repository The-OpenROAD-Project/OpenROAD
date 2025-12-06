// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "detailed_global.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include "boost/tokenizer.hpp"
#include "detailed_manager.h"
#include "dpl/Opendp.h"
#include "infrastructure/Objects.h"
#include "objective/detailed_hpwl.h"
#include "utl/Logger.h"

namespace dpl {

using utl::DPL;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedGlobalSwap::DetailedGlobalSwap(Architecture* arch, Network* network)
    : DetailedGenerator("global swap"),
      mgr_(nullptr),
      arch_(arch),
      network_(network),
      skipNetsLargerThanThis_(100),
      traversal_(0),
      attempts_(0),
      moves_(0),
      swaps_(0)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
DetailedGlobalSwap::DetailedGlobalSwap() : DetailedGlobalSwap(nullptr, nullptr)
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedGlobalSwap::run(DetailedMgr* mgrPtr, const std::string& command)
{
  // A temporary interface to allow for a string which we will decode to create
  // the arguments.
  boost::char_separator<char> separators(" \r\t\n;");
  boost::tokenizer<boost::char_separator<char>> tokens(command, separators);
  std::vector<std::string> args;
  for (const auto& token : tokens) {
    args.push_back(token);
  }
  run(mgrPtr, args);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedGlobalSwap::run(DetailedMgr* mgrPtr,
                             std::vector<std::string>& args)
{
  // Two-pass budget-constrained power-aware optimization using Journal-based state management

  mgr_ = mgrPtr;
  arch_ = mgr_->getArchitecture();
  network_ = mgr_->getNetwork();

  int passes = 1;
  double tol = 0.01;
  tradeoff_ = 0.2;  // Default: 20% exploration, 80% wirelength optimization
  
  for (size_t i = 1; i < args.size(); i++) {
    if (args[i] == "-p" && i + 1 < args.size()) {
      passes = std::atoi(args[++i].c_str());
    } else if (args[i] == "-t" && i + 1 < args.size()) {
      tol = std::atof(args[++i].c_str());
    } else if (args[i] == "-x" && i + 1 < args.size()) {
      tradeoff_ = std::atof(args[++i].c_str());
    }
  }
  passes = std::max(passes, 1);
  tol = std::max(tol, 0.01);
  tradeoff_ = std::max(0.0, std::min(1.0, tradeoff_));  // Clamp to [0.0, 1.0]

  uint64_t hpwl_x, hpwl_y;
  int64_t init_hpwl = Utility::hpwl(network_, hpwl_x, hpwl_y);
  if (init_hpwl == 0) {
    return;
  }

  // Store original displacement limits for restoration later
  int orig_disp_x, orig_disp_y;
  mgr_->getMaxDisplacement(orig_disp_x, orig_disp_y);
  
  // Get chip dimensions for unleashing the optimizer
  const int chip_width = arch_->getMaxX().v - arch_->getMinX().v;
  const int chip_height = arch_->getMaxY().v - arch_->getMinY().v;

  mgr_->getLogger()->info(DPL, 906, "Starting two-pass power-aware global swap optimization (tradeoff={:.1f})", tradeoff_);

  // PASS 1: HPWL Profiling Pass
  mgr_->getLogger()->info(DPL, 907, "Pass 1: HPWL profiling to determine budget");
  
  // Clear journal to ensure clean state tracking for profiling pass
  mgr_->getJournal().clear();
  
  is_profiling_pass_ = true;
  power_weight_ = 0.0;  // Pure HPWL optimization
  
  int64_t last_hpwl, curr_hpwl = init_hpwl;
  for (int p = 1; p <= passes; p++) {
    last_hpwl = curr_hpwl;
    globalSwap();
    curr_hpwl = Utility::hpwl(network_, hpwl_x, hpwl_y);
    
    mgr_->getLogger()->info(DPL, 316, "Profiling pass {:d}; hpwl is {:.6e}.", p, (double) curr_hpwl);
    
    if (last_hpwl == 0 || std::abs(curr_hpwl - last_hpwl) / (double) last_hpwl <= tol) {
      break;
    }
  }
  
  // Calculate budget: allow 10% degradation from optimal HPWL
  double optimal_hpwl = curr_hpwl;
  budget_hpwl_ = optimal_hpwl * 1.10;
  
  mgr_->getLogger()->info(DPL, 908, 
                         "Profiling complete. Optimal HPWL={:.2f}, Budget HPWL={:.2f} (+10%)", 
                         optimal_hpwl, budget_hpwl_);

  // Restore initial state using Journal's built-in undo mechanism
  mgr_->getLogger()->info(DPL, 917, "Undoing {} profiling moves to restore initial state", mgr_->getJournal().size());
  mgr_->getJournal().undo();
  mgr_->getJournal().clear();  // Clear journal for second pass
  
  // PASS 2: Iterative Budget-Constrained Power Optimization (4 iterations)
  mgr_->getLogger()->info(DPL, 909, "Pass 2: Iterative budget-constrained power optimization (4 stages)");
  is_profiling_pass_ = false;
  
  // Re-compute power density map to ensure it's synchronized with restored placement
  const float area_weight = 0.4f;
  const float pin_weight = 0.6f;
  mgr_->getGrid()->computePowerDensityMap(network_, area_weight, pin_weight);
  mgr_->getLogger()->info(DPL, 918, "Re-computed power density map after state restoration");
  
  // Calculate adaptive power weight once for all iterations
  power_weight_ = calculateAdaptivePowerWeight();
  
  // Define the iterative refinement schedule
  const std::vector<double> budget_multipliers = {1.50, 1.25, 1.10, 1.04};  // 50%, 25%, 10%, 4%
  const std::vector<std::string> stage_names = {"Exploratory", "Consolidation", "Fine-tuning", "Final Polish"};
  
  curr_hpwl = Utility::hpwl(network_, hpwl_x, hpwl_y);
  
  // Iterative refinement loop
  for (size_t iteration = 0; iteration < budget_multipliers.size(); iteration++) {
    // Update budget for this iteration
    budget_hpwl_ = optimal_hpwl * budget_multipliers[iteration];
    
    // Set dynamic displacement limits based on iteration stage
    if (iteration == 0) {
      // Iteration 1: Unleash the optimizer completely (chip-wide moves allowed)
      mgr_->setMaxDisplacement(chip_width, chip_height);
      mgr_->getLogger()->info(DPL, 921, "Unleashing optimizer: max displacement set to chip dimensions ({}, {})", 
                             chip_width, chip_height);
    } else if (iteration == 1) {
      // Iteration 2: Very loose but controlled (10x original)
      mgr_->setMaxDisplacement(orig_disp_x * 10, orig_disp_y * 10);
      mgr_->getLogger()->info(DPL, 922, "Loosened displacement: set to 10x original ({}, {})", 
                             orig_disp_x * 10, orig_disp_y * 10);
    } else {
      // Iterations 3 & 4: Restore original tight displacement for fine-tuning
      mgr_->setMaxDisplacement(orig_disp_x, orig_disp_y);
      mgr_->getLogger()->info(DPL, 923, "Restored tight displacement: set to original ({}, {})", 
                             orig_disp_x, orig_disp_y);
    }
    
    mgr_->getLogger()->info(DPL, 919, "Iteration {}: {} stage - Budget={:.2f} ({:.0f}% of optimal)", 
                           iteration + 1, stage_names[iteration], budget_hpwl_, 
                           (budget_multipliers[iteration] - 1.0) * 100.0);
    
    // Run optimization passes for this iteration
    for (int p = 1; p <= passes; p++) {
      last_hpwl = curr_hpwl;
      globalSwap();
      curr_hpwl = Utility::hpwl(network_, hpwl_x, hpwl_y);
      
      mgr_->getLogger()->info(DPL, 331, "Power optimization iteration {} pass {:d}; hpwl is {:.6e}.", 
                             iteration + 1, p, (double) curr_hpwl);
      
      if (last_hpwl == 0 || std::abs(curr_hpwl - last_hpwl) / (double) last_hpwl <= tol) {
        break;
      }
    }
    
    // Report iteration results
    double iteration_improvement = ((init_hpwl - curr_hpwl) / (double) init_hpwl) * 100.0;
    double budget_utilization = ((curr_hpwl - optimal_hpwl) / (budget_hpwl_ - optimal_hpwl)) * 100.0;
    mgr_->getLogger()->info(DPL, 920, "Iteration {} complete: HPWL={:.6e}, improvement={:.2f}%, budget utilization={:.1f}%", 
                           iteration + 1, (double) curr_hpwl, iteration_improvement, budget_utilization);
  }
  
  // Final reporting
  double final_improvement = (((init_hpwl - curr_hpwl) / (double) init_hpwl) * 100.);
  double budget_utilization = ((curr_hpwl - optimal_hpwl) / (budget_hpwl_ - optimal_hpwl)) * 100.0;
  
  mgr_->getLogger()->info(DPL, 910,
                          "Two-pass optimization complete: "
                          "final HPWL={:.6e}, improvement={:.2f}%, budget utilization={:.1f}%",
                          (double) curr_hpwl, final_improvement, budget_utilization);
  
  // Ensure original displacement limits are fully restored
  mgr_->setMaxDisplacement(orig_disp_x, orig_disp_y);
  mgr_->getLogger()->info(DPL, 924, "Final restoration: displacement limits restored to original ({}, {})", 
                         orig_disp_x, orig_disp_y);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void DetailedGlobalSwap::globalSwap()
{
  // Two-pass budget-constrained global swap: profiling pass or power optimization pass

  traversal_ = 0;
  edgeMask_.resize(network_->getNumEdges());
  std::fill(edgeMask_.begin(), edgeMask_.end(), 0);

  mgr_->resortSegments();

  // Get candidate cells.
  std::vector<Node*> candidates = mgr_->getSingleHeightCells();
  mgr_->shuffle(candidates);

  // Wirelength objective.
  DetailedHPWL hpwlObj(network_);
  hpwlObj.init(mgr_, nullptr);  // Ignore orientation.

  double currHpwl = hpwlObj.curr();
  const double initHpwl = currHpwl;
  
  // Determine budget constraint based on pass type
  double maxAllowedHpwl;
  if (is_profiling_pass_) {
    // In profiling pass: use generous budget for pure HPWL optimization
    maxAllowedHpwl = initHpwl * 2.0;  // Allow large changes during profiling
    mgr_->getLogger()->info(DPL, 914, 
                           "Profiling pass: initial HPWL={:.2f}, generous budget={:.2f}", 
                           initHpwl, maxAllowedHpwl);
  } else {
    // In power optimization pass: use strict budget from profiling
    maxAllowedHpwl = budget_hpwl_;
    mgr_->getLogger()->info(DPL, 915, 
                           "Power optimization pass: initial HPWL={:.2f}, budget={:.2f} (from profiling)", 
                           initHpwl, maxAllowedHpwl);
  }
  
  // Consider each candidate cell once.
  for (auto ndi : candidates) {
    // Hybrid move generation: Smart Swap logic
    bool move_generated = false;
    
    // Phase 1: Try wirelength-optimal move (unless we decide to override with exploration)
    if (mgr_->getRandom(1000) >= static_cast<int>(tradeoff_ * 1000)) {
      move_generated = generateWirelengthOptimalMove(ndi);
    }
    
    // Phase 2: If no move generated OR we decided to override, try random exploration move
    if (!move_generated) {
      move_generated = generateRandomMove(ndi);
    }
    
    if (!move_generated) {
      continue;  // No valid move found with either generator
    }

    // Calculate HPWL delta
    double hpwl_delta = hpwlObj.delta(mgr_->getJournal());
    double nextHpwl = currHpwl - hpwl_delta;  // Projected HPWL after this move

    // Calculate power density improvement (only relevant in second pass)
    double power_improvement = 0.0;
    if (!is_profiling_pass_) {  // Only calculate power improvement in second pass
      const auto& journal = mgr_->getJournal();
      if (!journal.empty()) {
        for (const auto& action_ptr : journal) {
          // Only handle MoveCellAction types
          if (action_ptr->typeId() != JournalActionTypeEnum::MOVE_CELL) {
            continue;
          }
          
          const MoveCellAction* move_action = static_cast<const MoveCellAction*>(action_ptr.get());
          Node* moved_cell = move_action->getNode();
          if (!moved_cell || moved_cell->getId() >= power_contribution_.size()) {
            continue;
          }
          
          // Get original and new grid coordinates
          const auto* grid = mgr_->getGrid();
          const GridX orig_grid_x = grid->gridX(move_action->getOrigLeft());
          const GridY orig_grid_y = grid->gridSnapDownY(move_action->getOrigBottom());
          const GridX new_grid_x = grid->gridX(move_action->getNewLeft());
          const GridY new_grid_y = grid->gridSnapDownY(move_action->getNewBottom());
          
          // Calculate pixel indices (row-major order)
          const int row_site_count = grid->getRowSiteCount().v;
          const int orig_pixel_idx = orig_grid_y.v * row_site_count + orig_grid_x.v;
          const int new_pixel_idx = new_grid_y.v * row_site_count + new_grid_x.v;
          
          // Get power densities at original and new locations
          const float orig_power_density = grid->getPowerDensity(orig_pixel_idx);
          const float new_power_density = grid->getPowerDensity(new_pixel_idx);
          
          // Get pre-calculated power contribution for this cell
          const double cell_power_contrib = power_contribution_[moved_cell->getId()];
          
          // Calculate power improvement: moving from high density to low density is good
          power_improvement += (orig_power_density - new_power_density) * cell_power_contrib;
        }
      }
    }
    
    // Hybrid acceptance criteria: budget constraint + combined objective
    if (nextHpwl > maxAllowedHpwl) {
      // Hard constraint violated: reject move regardless of other benefits
      mgr_->rejectMove();
      continue;
    }
    
    // Within budget: evaluate combined profit
    double combined_profit = hpwl_delta + power_weight_ * power_improvement;
    
    if (combined_profit > 0) {
      // Accept: move is profitable and within budget
      hpwlObj.accept();
      mgr_->acceptMove();
      currHpwl = nextHpwl;
      
      // Update power density map for accepted moves (only in power optimization pass)
      if (!is_profiling_pass_) {
        const auto& journal = mgr_->getJournal();
        if (!journal.empty()) {
          for (const auto& action_ptr : journal) {
            if (action_ptr->typeId() != JournalActionTypeEnum::MOVE_CELL) {
              continue;
            }
            
            const MoveCellAction* move_action = static_cast<const MoveCellAction*>(action_ptr.get());
            Node* moved_cell = move_action->getNode();
            if (!moved_cell) {
              continue;
            }
            
            // Remove cell from old location and add to new location
            mgr_->getGrid()->updatePowerDensity(moved_cell, move_action->getOrigLeft(), move_action->getOrigBottom(), false);
            mgr_->getGrid()->updatePowerDensity(moved_cell, move_action->getNewLeft(), move_action->getNewBottom(), true);
          }
        }
      }
    } else {
      mgr_->rejectMove();
    }
  }
  
  // Report final statistics
  const double finalDegradation = ((currHpwl - initHpwl) / initHpwl) * 100.0;
  const char* pass_name = is_profiling_pass_ ? "Profiling" : "Power optimization";
  mgr_->getLogger()->info(DPL, 916, 
                         "{} pass complete: final HPWL={:.2f}, change={:.1f}%", 
                         pass_name, currHpwl, finalDegradation);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedGlobalSwap::getRange(Node* nd, odb::Rect& nodeBbox)
{
  // Determines the median location for a node.

  Edge* ed;
  unsigned mid;

  Pin* pin;
  unsigned t = 0;

  DbuX xmin = arch_->getMinX();
  DbuX xmax = arch_->getMaxX();
  DbuY ymin = arch_->getMinY();
  DbuY ymax = arch_->getMaxY();

  xpts_.clear();
  ypts_.clear();
  for (int n = 0; n < nd->getNumPins(); n++) {
    pin = nd->getPins()[n];

    ed = pin->getEdge();

    nodeBbox.mergeInit();

    int numPins = ed->getNumPins();
    if (numPins <= 1) {
      continue;
    }
    if (numPins > skipNetsLargerThanThis_) {
      continue;
    }
    if (!calculateEdgeBB(ed, nd, nodeBbox)) {
      continue;
    }

    // We've computed an interval for the pin.  We need to alter it to work for
    // the cell center. Also, we need to avoid going off the edge of the chip.
    nodeBbox.set_xlo(std::min(
        std::max(xmin.v, nodeBbox.xMin() - pin->getOffsetX().v), xmax.v));
    nodeBbox.set_xhi(std::max(
        std::min(xmax.v, nodeBbox.xMax() - pin->getOffsetX().v), xmin.v));
    nodeBbox.set_ylo(std::min(
        std::max(ymin.v, nodeBbox.yMin() - pin->getOffsetY().v), ymax.v));
    nodeBbox.set_yhi(std::max(
        std::min(ymax.v, nodeBbox.yMax() - pin->getOffsetY().v), ymin.v));

    // Record the location and pin offset used to generate this point.

    xpts_.push_back(nodeBbox.xMin());
    xpts_.push_back(nodeBbox.xMax());

    ypts_.push_back(nodeBbox.yMin());
    ypts_.push_back(nodeBbox.yMax());

    ++t;
    ++t;
  }

  // If, for some weird reason, we didn't find anything connected, then
  // return false to indicate that there's nowhere to move the cell.
  if (t <= 1) {
    return false;
  }

  // Get the median values.
  mid = t >> 1;

  std::sort(xpts_.begin(), xpts_.end());
  std::sort(ypts_.begin(), ypts_.end());

  nodeBbox.set_xlo(xpts_[mid - 1]);
  nodeBbox.set_xhi(xpts_[mid]);

  nodeBbox.set_ylo(ypts_[mid - 1]);
  nodeBbox.set_yhi(ypts_[mid]);

  return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedGlobalSwap::calculateEdgeBB(Edge* ed, Node* nd, odb::Rect& bbox)
{
  // Computes the bounding box of an edge.  Node 'nd' is the node to SKIP.
  DbuX curX;
  DbuY curY;

  bbox.mergeInit();

  int count = 0;
  for (Pin* pin : ed->getPins()) {
    auto other = pin->getNode();
    if (other == nd) {
      continue;
    }
    curX = other->getCenterX() + pin->getOffsetX().v;
    curY = other->getCenterY() + pin->getOffsetY().v;

    bbox.set_xlo(std::min(curX.v, bbox.xMin()));
    bbox.set_xhi(std::max(curX.v, bbox.xMax()));
    bbox.set_ylo(std::min(curY.v, bbox.yMin()));
    bbox.set_yhi(std::max(curY.v, bbox.yMax()));

    ++count;
  }

  return (count == 0) ? false : true;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedGlobalSwap::generateWirelengthOptimalMove(Node* ndi)
{
  double yi = ndi->getBottom().v + 0.5 * ndi->getHeight().v;
  double xi = ndi->getLeft().v + 0.5 * ndi->getWidth().v;

  // Determine optimal region.
  odb::Rect bbox;
  if (!getRange(ndi, bbox)) {
    // Failed to find an optimal region.
    return false;
  }
  if (xi >= bbox.xMin() && xi <= bbox.xMax() && yi >= bbox.yMin()
      && yi <= bbox.yMax()) {
    // If cell inside box, do nothing.
    return false;
  }

  // Observe displacement limit.  I suppose there are options.
  // If we cannot move into the optimal region, we could try
  // to move closer to it.  Or, we could just reject if we cannot
  // get into the optimal region.
  int dispX, dispY;
  mgr_->getMaxDisplacement(dispX, dispY);
  odb::Rect lbox(ndi->getLeft().v - dispX,
                 ndi->getBottom().v - dispY,
                 ndi->getLeft().v + dispX,
                 ndi->getBottom().v + dispY);
  if (lbox.xMax() <= bbox.xMin()) {
    bbox.set_xlo(ndi->getLeft().v);
    bbox.set_xhi(lbox.xMax());
  } else if (lbox.xMin() >= bbox.xMax()) {
    bbox.set_xlo(lbox.xMin());
    bbox.set_xhi(ndi->getLeft().v);
  } else {
    bbox.set_xlo(std::max(bbox.xMin(), lbox.xMin()));
    bbox.set_xhi(std::min(bbox.xMax(), lbox.xMax()));
  }
  if (lbox.yMax() <= bbox.yMin()) {
    bbox.set_ylo(ndi->getBottom().v);
    bbox.set_yhi(lbox.yMax());
  } else if (lbox.yMin() >= bbox.yMax()) {
    bbox.set_ylo(lbox.yMin());
    bbox.set_yhi(ndi->getBottom().v);
  } else {
    bbox.set_ylo(std::max(bbox.yMin(), lbox.yMin()));
    bbox.set_yhi(std::min(bbox.yMax(), lbox.yMax()));
  }

  if (mgr_->getNumReverseCellToSegs(ndi->getId()) != 1) {
    return false;
  }
  int si = mgr_->getReverseCellToSegs(ndi->getId())[0]->getSegId();

  // Position target so center of cell at center of box.
  DbuX xj{(int) std::floor(0.5 * (bbox.xMin() + bbox.xMax())
                           - 0.5 * ndi->getWidth().v)};
  DbuY yj{(int) std::floor(0.5 * (bbox.yMin() + bbox.yMax())
                           - 0.5 * ndi->getHeight().v)};

  // Row and segment for the destination.
  int rj = arch_->find_closest_row(yj);
  yj = DbuY{arch_->getRow(rj)->getBottom()};  // Row alignment.
  int sj = -1;
  for (int s = 0; s < mgr_->getNumSegsInRow(rj); s++) {
    DetailedSeg* segPtr = mgr_->getSegsInRow(rj)[s];
    if (xj >= segPtr->getMinX() && xj <= segPtr->getMaxX()) {
      sj = segPtr->getSegId();
      break;
    }
  }
  if (sj == -1) {
    return false;
  }
  if (ndi->getGroupId() != mgr_->getSegment(sj)->getRegId()) {
    return false;
  }

  if (mgr_->tryMove(ndi, ndi->getLeft(), ndi->getBottom(), si, xj, yj, sj)) {
    ++moves_;
    return true;
  }
  if (mgr_->trySwap(ndi, ndi->getLeft(), ndi->getBottom(), si, xj, yj, sj)) {
    ++swaps_;
    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedGlobalSwap::generateRandomMove(Node* ndi)
{
  // Generate a random move within the current displacement constraints
  // This is for exploration and power optimization purposes
  
  if (mgr_->getNumReverseCellToSegs(ndi->getId()) != 1) {
    return false;
  }
  int si = mgr_->getReverseCellToSegs(ndi->getId())[0]->getSegId();

  // Get current displacement limits
  int dispX, dispY;
  mgr_->getMaxDisplacement(dispX, dispY);
  
  // Define the search area around the current cell position
  DbuX curr_x = ndi->getLeft();
  DbuY curr_y = ndi->getBottom();
  
  DbuX min_x = std::max(arch_->getMinX(), curr_x - dispX);
  DbuX max_x = std::min(arch_->getMaxX(), curr_x + dispX);
  DbuY min_y = std::max(arch_->getMinY(), curr_y - dispY);
  DbuY max_y = std::min(arch_->getMaxY(), curr_y + dispY);
  
  // Try up to 10 random locations within the displacement area
  const int max_attempts = 10;
  for (int attempt = 0; attempt < max_attempts; attempt++) {
    // Generate random coordinates within the allowed displacement area
    DbuX rand_x{min_x.v + mgr_->getRandom(max_x.v - min_x.v + 1)};
    DbuY rand_y{min_y.v + mgr_->getRandom(max_y.v - min_y.v + 1)};
    
    // Find the appropriate row and segment for this random location
    int rj = arch_->find_closest_row(rand_y);
    rand_y = DbuY{arch_->getRow(rj)->getBottom()};  // Row alignment
    
    int sj = -1;
    for (int s = 0; s < mgr_->getNumSegsInRow(rj); s++) {
      DetailedSeg* segPtr = mgr_->getSegsInRow(rj)[s];
      if (rand_x >= segPtr->getMinX() && rand_x <= segPtr->getMaxX()) {
        sj = segPtr->getSegId();
        break;
      }
    }
    
    if (sj == -1) {
      continue;  // Invalid segment, try another random location
    }
    
    if (ndi->getGroupId() != mgr_->getSegment(sj)->getRegId()) {
      continue;  // Wrong region, try another location
    }
    
    // Try to execute the move/swap to this random location
    if (mgr_->tryMove(ndi, curr_x, curr_y, si, rand_x, rand_y, sj)) {
      ++moves_;
      return true;
    }
    if (mgr_->trySwap(ndi, curr_x, curr_y, si, rand_x, rand_y, sj)) {
      ++swaps_;
      return true;
    }
  }
  
  return false;  // Could not find a valid random move after max_attempts
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedGlobalSwap::generate(Node* ndi)
{
  // Hybrid move generation: Smart Swap logic
  bool move_generated = false;
  
  // Phase 1: Try wirelength-optimal move (unless we decide to override with exploration)
  if (mgr_->getRandom(1000) >= static_cast<int>(tradeoff_ * 1000)) {
    move_generated = generateWirelengthOptimalMove(ndi);
  }
  
  // Phase 2: If no move generated OR we decided to override, try random exploration move
  if (!move_generated) {
    move_generated = generateRandomMove(ndi);
  }
  
  return move_generated;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedGlobalSwap::init(DetailedMgr* mgr)
{
  mgr_ = mgr;
  arch_ = mgr->getArchitecture();
  network_ = mgr->getNetwork();

  traversal_ = 0;
  edgeMask_.resize(network_->getNumEdges());
  std::fill(edgeMask_.begin(), edgeMask_.end(), 0);

  // Power-density-aware placement initialization
  const float area_weight = 0.4f;
  const float pin_weight = 0.6f;
  
  // Compute power density map
  mgr_->getGrid()->computePowerDensityMap(network_, area_weight, pin_weight);
  
  // Pre-calculate power contributions for all cells
  power_contribution_.resize(network_->getNumNodes());
  for (const auto& node_ptr : network_->getNodes()) {
    Node* node = node_ptr.get();
    if (node && node->getType() == Node::Type::CELL) {
      const double cell_area = static_cast<double>(node->getWidth().v * node->getHeight().v);
      const double num_pins = static_cast<double>(node->getNumPins());
      power_contribution_[node->getId()] = area_weight * cell_area + pin_weight * num_pins;
    }
  }
  
  // Calculate adaptive power weight by sampling typical HPWL deltas and power improvements
  power_weight_ = calculateAdaptivePowerWeight();
  
  mgr_->getLogger()->info(DPL, 901, "Initialized power-aware global swap with adaptive power_weight={:.3f}", power_weight_);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool DetailedGlobalSwap::generate(DetailedMgr* mgr,
                                  std::vector<Node*>& candidates)
{
  ++attempts_;

  mgr_ = mgr;
  arch_ = mgr->getArchitecture();
  network_ = mgr->getNetwork();

  Node* ndi = candidates[mgr_->getRandom(candidates.size())];

  return generate(ndi);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
double DetailedGlobalSwap::calculateAdaptivePowerWeight()
{
  const int num_samples = 150;  // Number of random swaps to sample
  const double user_knob = 35.0;  // Tuning parameter: how much to prioritize power over HPWL (increased for extremely aggressive trade-off) 
  
  // Get candidate cells for sampling
  std::vector<Node*> candidates = mgr_->getSingleHeightCells();
  if (candidates.size() < 2) {
    // Fallback to a reasonable default if insufficient cells
    return 1.0 * mgr_->getGrid()->getSiteWidth().v;
  }
  
  // Create temporary HPWL objective for sampling
  DetailedHPWL hpwlObj(network_);
  hpwlObj.init(mgr_, nullptr);
  
  double total_hpwl_delta = 0.0;
  double total_power_improvement = 0.0;
  int valid_samples = 0;
  
  // Sample random swaps to estimate typical deltas
  for (int i = 0; i < num_samples && i < candidates.size(); i++) {
    // Pick a random candidate cell
    Node* cell_a = candidates[mgr_->getRandom(candidates.size())];
    
    // Try to generate a move/swap for this cell
    if (!generate(cell_a)) {
      continue;  // Skip if no valid move found
    }
    
    // Calculate HPWL delta
    double hpwl_delta = hpwlObj.delta(mgr_->getJournal());
    
    // Calculate power improvement
    double power_improvement = 0.0;
    const auto& journal = mgr_->getJournal();
    if (!journal.empty()) {
      for (const auto& action_ptr : journal) {
        if (action_ptr->typeId() != JournalActionTypeEnum::MOVE_CELL) {
          continue;
        }
        
        const MoveCellAction* move_action = static_cast<const MoveCellAction*>(action_ptr.get());
        Node* moved_cell = move_action->getNode();
        if (!moved_cell || moved_cell->getId() >= power_contribution_.size()) {
          continue;
        }
        
        // Get grid coordinates
        const auto* grid = mgr_->getGrid();
        const GridX orig_grid_x = grid->gridX(move_action->getOrigLeft());
        const GridY orig_grid_y = grid->gridSnapDownY(move_action->getOrigBottom());
        const GridX new_grid_x = grid->gridX(move_action->getNewLeft());
        const GridY new_grid_y = grid->gridSnapDownY(move_action->getNewBottom());
        
        // Calculate pixel indices
        const int row_site_count = grid->getRowSiteCount().v;
        const int orig_pixel_idx = orig_grid_y.v * row_site_count + orig_grid_x.v;
        const int new_pixel_idx = new_grid_y.v * row_site_count + new_grid_x.v;
        
        // Get power densities
        const float orig_power_density = grid->getPowerDensity(orig_pixel_idx);
        const float new_power_density = grid->getPowerDensity(new_pixel_idx);
        
        // Get cell power contribution
        const double cell_power_contrib = power_contribution_[moved_cell->getId()];
        
        // Calculate power improvement
        power_improvement += (orig_power_density - new_power_density) * cell_power_contrib;
      }
    }
    
    // Accumulate magnitudes (we care about the scale, not the sign)
    total_hpwl_delta += std::abs(hpwl_delta);
    total_power_improvement += std::abs(power_improvement);
    valid_samples++;
    
    // Always reject the sample move to keep the design unchanged
    mgr_->rejectMove();
  }
  
  if (valid_samples == 0) {
    // Fallback if no valid samples
    mgr_->getLogger()->warn(DPL, 902, "No valid samples for adaptive power weight calculation, using fallback");
    return 1.0 * mgr_->getGrid()->getSiteWidth().v;
  }
  
  // Calculate averages
  double avg_hpwl_delta = total_hpwl_delta / valid_samples;
  double avg_power_improvement = total_power_improvement / valid_samples;
  
  // Calculate adaptive weight
  double adaptive_weight;
  if (avg_power_improvement > 0) {
    // Scale power improvement to be comparable to HPWL, then apply user knob
    adaptive_weight = (avg_hpwl_delta / avg_power_improvement) * user_knob;
  } else {
    // Fallback if power improvement is negligible
    adaptive_weight = 0.5 * mgr_->getGrid()->getSiteWidth().v;
  }
  
  mgr_->getLogger()->info(DPL, 903, 
                         "Adaptive power weight: avg_hpwl_delta={:.2f}, avg_power_improvement={:.6f}, "
                         "samples={}, weight={:.3f} (user_knob={:.1f})", 
                         avg_hpwl_delta, avg_power_improvement, valid_samples, adaptive_weight, user_knob);
  
  return adaptive_weight;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void DetailedGlobalSwap::stats()
{
  mgr_->getLogger()->info(
      DPL,
      334,
      "Generator {:s}, "
      "Cumulative attempts {:d}, swaps {:d}, moves {:5d} since last reset.",
      getName().c_str(),
      attempts_,
      swaps_,
      moves_);
}

}  // namespace dpl