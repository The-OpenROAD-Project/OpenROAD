// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "SACoreSoftMacro.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <set>
#include <utility>
#include <vector>

#include "MplObserver.h"
#include "SACore.h"
#include "boost/random/uniform_int_distribution.hpp"
#include "clusterEngine.h"
#include "mpl-util.h"
#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace mpl {
using utl::MPL;

//////////////////////////////////////////////////////////////////
// Class SACoreSoftMacro
// constructors
SACoreSoftMacro::SACoreSoftMacro(PhysicalHierarchy* tree,
                                 const odb::Rect& outline,
                                 const std::vector<SoftMacro>& macros,
                                 const SACoreWeights& core_weights,
                                 const SASoftWeights& soft_weights,
                                 // notch threshold
                                 float notch_h_threshold,
                                 float notch_v_threshold,
                                 // probability of each action
                                 const SACoreActionProbs& action_probs,
                                 const SASoftActionsProbs& soft_action_probs,
                                 const SAHyperparams& hyperparams,
                                 unsigned seed,
                                 MplObserver* graphics,
                                 utl::Logger* logger,
                                 odb::dbBlock* block)
    : SimulatedAnnealingCore<SoftMacro>(tree,
                                        outline,
                                        macros,
                                        core_weights,
                                        action_probs,
                                        hyperparams,
                                        seed,
                                        graphics,
                                        logger,
                                        block),
      root_(tree->root.get())
{
  penalties_.push_back({.name = "Boundary", .weight = soft_weights.boundary});
  penalties_.push_back(
      {.name = "Soft Blockage", .weight = soft_weights.soft_blockage});
  penalties_.push_back({.name = "Fixed Macros", .weight = 100.0f});
  penalties_.push_back({.name = "Notch", .weight = soft_weights.notch});

  // Resizes one cluster; only touches macros_ widths/heights.
  actions_.push_back({
      .probability = soft_action_probs.resize,
      .perturb = [this] { resizeOneCluster(); },
  });

  notch_h_th_ = notch_h_threshold;
  notch_v_th_ = notch_v_threshold;
  adjust_h_th_ = notch_h_th_;
  adjust_v_th_ = notch_v_th_;
}

void SACoreSoftMacro::findFixedMacros()
{
  for (const int macro_id : pos_seq_) {
    const SoftMacro& macro = macros_[macro_id];
    if (macro.isFixed()) {
      fixed_macros_.push_back(macro.getBBox());
    }
  }
}

void SACoreSoftMacro::run()
{
  if (graphics_) {
    graphics_->startSA(
        "soft", hyperparams_.max_num_step, hyperparams_.num_perturb_per_step);
  }

  fastSA();

  if (enhancements_on_) {
    attemptCentralization(calNormCost());
    if (centralization_was_reverted_) {
      attemptMacroClusterAlignment();
    }
  }

  if (graphics_) {
    graphics_->endSA(calNormCost());
  }
}

bool SACoreSoftMacro::isValid() const
{
  if (!fixed_macros_.empty() && penalties_[kFixedMacros].value > 0.0f) {
    return false;
  }

  return resultFitsInOutline();
}

// Operations
// Align macro clusters to reduce notch
void SACoreSoftMacro::attemptMacroClusterAlignment()
{
  if (!isValid()) {
    return;
  }

  float pre_cost = calNormCost();
  // Cache current solution to allow reversal
  auto clusters_locations = getClustersLocations();

  // update threshold value
  adjust_h_th_ = notch_h_th_;
  adjust_v_th_ = notch_v_th_;
  for (auto& macro_id : pos_seq_) {
    if (macros_[macro_id].isMacroCluster()) {
      adjust_h_th_ = std::min(adjust_h_th_, macros_[macro_id].getWidth());
      adjust_v_th_ = std::min(adjust_v_th_, macros_[macro_id].getHeight());
    }
  }

  const int ratio = 10;
  adjust_h_th_ = std::min(adjust_h_th_, outline_.dy() / ratio);
  adjust_v_th_ = std::min(adjust_v_th_, outline_.dx() / ratio);

  // Align macro clusters to boundaries
  for (auto& macro_id : pos_seq_) {
    if (macros_[macro_id].isMacroCluster()) {
      const int lx = macros_[macro_id].getX();
      const int ly = macros_[macro_id].getY();
      const int ux = lx + macros_[macro_id].getWidth();
      const int uy = ly + macros_[macro_id].getHeight();
      // align to left / right boundaries
      if (lx < adjust_h_th_) {
        macros_[macro_id].setX(0);
      } else if (outline_.dx() - ux < adjust_h_th_) {
        macros_[macro_id].setX(outline_.dx() - macros_[macro_id].getWidth());
      }
      // align to top / bottom boundaries
      if (ly < adjust_v_th_) {
        macros_[macro_id].setY(0);
      } else if (outline_.dy() - uy < adjust_v_th_) {
        macros_[macro_id].setY(outline_.dy() - macros_[macro_id].getHeight());
      }
    }
  }

  calPenalty();

  // Revert macro alignemnt
  if (calNormCost() > pre_cost) {
    setClustersLocations(clusters_locations);

    if (graphics_) {
      graphics_->saStep(macros_);
    }

    calPenalty();
  }
}

void SACoreSoftMacro::resizeOneCluster()
{
  if (pos_seq_.empty()) {
    logger_->error(
        utl::MPL,
        51,
        "Position sequence array is empty, please report this internal error");
  }

  boost::random::uniform_int_distribution<> index_distribution(
      0, pos_seq_.size() - 1);
  const int idx = index_distribution(generator_);

  SoftMacro& src_macro = macros_[idx];
  if (src_macro.isMacroCluster()) {
    src_macro.resizeRandomly(distribution_, generator_);
    return;
  }

  const int lx = src_macro.getX();
  const int ly = src_macro.getY();
  const int ux = lx + src_macro.getWidth();
  const int uy = ly + src_macro.getHeight();
  // if the macro is outside of the outline, we randomly resize the macro
  if (ux >= outline_.dx() || uy >= outline_.dy()) {
    src_macro.resizeRandomly(distribution_, generator_);
    return;
  }

  if (distribution_(generator_) < 0.4) {
    src_macro.resizeRandomly(distribution_, generator_);
    return;
  }

  const float option = distribution_(generator_);
  if (option <= 0.25) {
    // Change the width of soft block to Rb = e.x2 - b.x1
    int e_x2 = outline_.dx();
    for (const int macro_id : pos_seq_) {
      SoftMacro& macro = macros_[macro_id];
      const int cur_x2 = macro.getX() + macro.getWidth();
      if (cur_x2 > ux && cur_x2 < e_x2) {
        e_x2 = cur_x2;
      }
    }
    src_macro.setWidth(e_x2 - lx);
  } else if (option <= 0.5) {
    int d_x2 = lx;
    for (const int macro_id : pos_seq_) {
      SoftMacro& macro = macros_[macro_id];
      const int cur_x2 = macro.getX() + macro.getWidth();
      if (cur_x2 < ux && cur_x2 > d_x2) {
        d_x2 = cur_x2;
      }
    }
    if (d_x2 > lx) {
      src_macro.setWidth(d_x2 - lx);
    }
  } else if (option <= 0.75) {
    // change the height of soft block to Tb = a.y2 - b.y1
    int a_y2 = outline_.dy();
    for (const int macro_id : pos_seq_) {
      SoftMacro& macro = macros_[macro_id];
      const int cur_y2 = macro.getY() + macro.getHeight();
      if (cur_y2 > uy && cur_y2 < a_y2) {
        a_y2 = cur_y2;
      }
    }
    src_macro.setHeight(a_y2 - ly);
  } else {
    // Change the height of soft block to Bb = c.y2 - b.y1
    int c_y2 = ly;
    for (const int macro_id : pos_seq_) {
      SoftMacro& macro = macros_[macro_id];
      const int cur_y2 = macro.getY() + macro.getHeight();
      if (cur_y2 < uy && cur_y2 > c_y2) {
        c_y2 = cur_y2;
      }
    }
    if (c_y2 > ly) {
      src_macro.setHeight(c_y2 - ly);
    }
  }
}

// fill the dead space by adjust the size of MixedCluster
void SACoreSoftMacro::fillDeadSpace()
{
  // if the floorplan is invalid, do nothing
  if (!isValid()) {
    return;
  }

  // adjust the location of MixedCluster
  // Step1 : Divide the entire floorplan into grids
  std::set<int> x_point;
  std::set<int> y_point;
  for (auto& macro_id : pos_seq_) {
    if (macros_[macro_id].getArea() == 0) {
      continue;
    }
    x_point.insert(macros_[macro_id].getX());
    x_point.insert(macros_[macro_id].getX() + macros_[macro_id].getWidth());
    y_point.insert(macros_[macro_id].getY());
    y_point.insert(macros_[macro_id].getY() + macros_[macro_id].getHeight());
  }
  x_point.insert(0);
  y_point.insert(0);
  x_point.insert(outline_.dx());
  y_point.insert(outline_.dy());
  // create grid
  std::vector<int> x_grid(x_point.begin(), x_point.end());
  std::vector<int> y_grid(y_point.begin(), y_point.end());
  // create grid in a row-based manner
  std::vector<std::vector<int>> grids;  // store the macro id
  const int num_x = x_grid.size() - 1;
  const int num_y = y_grid.size() - 1;
  for (int j = 0; j < num_y; j++) {
    std::vector<int> macro_ids(num_x, -1);
    grids.push_back(std::move(macro_ids));
  }

  for (int macro_id = 0; macro_id < pos_seq_.size(); macro_id++) {
    if (macros_[macro_id].getArea() == 0) {
      continue;
    }

    int x_start = getSegmentIndex(macros_[macro_id].getX(), x_grid);
    int x_end = getSegmentIndex(
        macros_[macro_id].getX() + macros_[macro_id].getWidth(), x_grid);
    int y_start = getSegmentIndex(macros_[macro_id].getY(), y_grid);
    int y_end = getSegmentIndex(
        macros_[macro_id].getY() + macros_[macro_id].getHeight(), y_grid);

    for (int j = y_start; j < y_end; j++) {
      for (int i = x_start; i < x_end; i++) {
        grids[j][i] = macro_id;
      }
    }
  }
  // propagate from the MixedCluster and then StdCellCluster
  for (int order = 0; order <= 1; order++) {
    for (int macro_id = 0; macro_id < pos_seq_.size(); macro_id++) {
      if (macros_[macro_id].getArea() == 0) {
        continue;
      }
      const bool forward_flag = (order == 0)
                                    ? macros_[macro_id].isMixedCluster()
                                    : macros_[macro_id].isStdCellCluster();
      if (!forward_flag) {
        continue;
      }

      int x_start = getSegmentIndex(macros_[macro_id].getX(), x_grid);
      int x_end = getSegmentIndex(
          macros_[macro_id].getX() + macros_[macro_id].getWidth(), x_grid);
      int y_start = getSegmentIndex(macros_[macro_id].getY(), y_grid);
      int y_end = getSegmentIndex(
          macros_[macro_id].getY() + macros_[macro_id].getHeight(), y_grid);

      int x_start_new = x_start;
      int x_end_new = x_end;
      int y_start_new = y_start;
      int y_end_new = y_end;
      // propagate left first
      for (int i = x_start - 1; i >= 0; i--) {
        bool flag = true;
        for (int j = y_start; j < y_end; j++) {
          if (grids[j][i] != -1) {
            flag = false;  // we cannot extend the current cluster
            break;
          }  // end if
        }  // end y
        if (!flag) {  // extension done
          break;
        }
        x_start_new--;  // extend left
        for (int j = y_start; j < y_end; j++) {
          grids[j][i] = macro_id;
        }
      }  // end left
      x_start = x_start_new;
      // propagate top second
      for (int j = y_end; j < num_y; j++) {
        bool flag = true;
        for (int i = x_start; i < x_end; i++) {
          if (grids[j][i] != -1) {
            flag = false;  // we cannot extend the current cluster
            break;
          }  // end if
        }  // end y
        if (!flag) {  // extension done
          break;
        }
        y_end_new++;  // extend top
        for (int i = x_start; i < x_end; i++) {
          grids[j][i] = macro_id;
        }
      }  // end top
      y_end = y_end_new;
      // propagate right third
      for (int i = x_end; i < num_x; i++) {
        bool flag = true;
        for (int j = y_start; j < y_end; j++) {
          if (grids[j][i] != -1) {
            flag = false;  // we cannot extend the current cluster
            break;
          }  // end if
        }  // end y
        if (!flag) {  // extension done
          break;
        }
        x_end_new++;  // extend right
        for (int j = y_start; j < y_end; j++) {
          grids[j][i] = macro_id;
        }
      }  // end right
      x_end = x_end_new;
      // propagate down second
      for (int j = y_start - 1; j >= 0; j--) {
        bool flag = true;
        for (int i = x_start; i < x_end; i++) {
          if (grids[j][i] != -1) {
            flag = false;  // we cannot extend the current cluster
            break;
          }  // end if
        }  // end y
        if (!flag) {  // extension done
          break;
        }
        y_start_new--;  // extend down
        for (int i = x_start; i < x_end; i++) {
          grids[j][i] = macro_id;
        }
      }  // end down
      y_start = y_start_new;
      // update the location of cluster
      macros_[macro_id].setLocationF(x_grid[x_start], y_grid[y_start]);
      macros_[macro_id].setShapeF(x_grid[x_end] - x_grid[x_start],
                                  y_grid[y_end] - y_grid[y_start]);
    }
  }
}

int SACoreSoftMacro::getSegmentIndex(const int segment,
                                     const std::vector<int>& coords)
{
  int index = std::distance(coords.begin(),
                            std::ranges::lower_bound(coords, segment));
  return index;
}

void SACoreSoftMacro::setSoftBlockages(const RectList& soft_blockages)
{
  soft_blockages_ = soft_blockages;
}

std::vector<odb::Point> SACoreSoftMacro::getClustersLocations() const
{
  std::vector<odb::Point> clusters_locations(pos_seq_.size());
  for (int id : pos_seq_) {
    clusters_locations[id] = {macros_[id].getX(), macros_[id].getY()};
  }

  return clusters_locations;
}

void SACoreSoftMacro::setClustersLocations(
    const std::vector<odb::Point>& clusters_locations)
{
  if (clusters_locations.size() != pos_seq_.size()) {
    logger_->error(MPL,
                   52,
                   "setClustersLocation called with a different numbers of "
                   "clusters of that in the sequence pair");
  }

  for (int& id : pos_seq_) {
    macros_[id].setX(clusters_locations[id].x());
    macros_[id].setY(clusters_locations[id].y());
  }
}

void SACoreSoftMacro::attemptCentralization(const float pre_cost)
{
  if (penalties_[kOutline].value > 0) {
    return;
  }

  // In order to revert the centralization, we cache the current location
  // of the clusters to avoid floating-point evilness when creating the
  // x,y grid to fill the dead space by expanding mixed clusters.
  auto clusters_locations = getClustersLocations();

  const odb::Point offset((outline_.dx() - width_) / 2,
                          (outline_.dy() - height_) / 2);
  moveFloorplan(offset);
  calPenalty();

  // revert centralization
  if (calNormCost() > pre_cost && !force_centralization_) {
    centralization_was_reverted_ = true;

    setClustersLocations(clusters_locations);

    if (graphics_) {
      graphics_->saStep(macros_);
    }

    calPenalty();
  }
}

void SACoreSoftMacro::moveFloorplan(const odb::Point& offset)
{
  for (auto& id : pos_seq_) {
    macros_[id].setX(macros_[id].getX() + offset.x());
    macros_[id].setY(macros_[id].getY() + offset.y());
  }

  if (graphics_) {
    graphics_->saStep(macros_);
  }
}

}  // namespace mpl
