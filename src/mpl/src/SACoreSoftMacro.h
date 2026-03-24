// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <utility>
#include <vector>

#include "MplObserver.h"
#include "SimulatedAnnealingCore.h"
#include "clusterEngine.h"
#include "mpl-util.h"
#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace utl {
class Logger;
}

namespace mpl {
class Graphics;

class SACoreSoftMacro : public SimulatedAnnealingCore<SoftMacro>
{
 public:
  SACoreSoftMacro(PhysicalHierarchy* tree,
                  const odb::Rect& outline,
                  const std::vector<SoftMacro>& macros,
                  const SACoreWeights& core_weights,
                  const SASoftWeights& soft_weights,
                  // notch threshold
                  float notch_h_threshold,
                  float notch_v_threshold,
                  // action prob
                  float pos_swap_prob,
                  float neg_swap_prob,
                  float double_swap_prob,
                  float exchange_prob,
                  float resize_prob,
                  // Fast SA hyperparameter
                  float init_prob,
                  int max_num_step,
                  int num_perturb_per_step,
                  unsigned seed,
                  MplObserver* graphics,
                  utl::Logger* logger,
                  odb::dbBlock* block);

  void run() override;
  bool isValid() const override;

  // accessors
  float getBoundaryPenalty() const;
  float getNormBoundaryPenalty() const;
  float getNotchPenalty() const;
  float getNormNotchPenalty() const;
  float getMacroBlockagePenalty() const;
  float getNormMacroBlockagePenalty() const;

  void printResults() const;  // just for test

  // Initialize the SA worker
  void initialize() override;
  // adjust the size of MixedCluster to fill the empty space
  void fillDeadSpace() override;
  void attemptMacroClusterAlignment();
  void addBlockages(const std::vector<odb::Rect>& blockages);

  bool centralizationWasReverted() { return centralization_was_reverted_; }

  void enableEnhancements() { enhancements_on_ = true; };

  void forceCentralization() { force_centralization_ = true; }

 private:
  // Used to check the vicinity of candidate notches
  struct NotchVicinity
  {
    bool top = true;
    bool bottom = true;
    bool left = true;
    bool right = true;

    int total() { return top + bottom + left + right; }

    bool operator==(const NotchVicinity&) const = default;
  };

  float calNormCost() const override;
  void calPenalty() override;

  void perturb() override;
  void saveState() override;
  void restoreState() override;
  // actions used
  void resizeOneCluster();

  int getSegmentIndex(int segment, const std::vector<int>& coords);

  void calBoundaryPenalty();
  void fillCoordsLists(std::vector<int>& x_coords, std::vector<int>& y_coords);
  static NotchVicinity checkNotchVicinity(
      const std::vector<std::vector<bool>>& grid,
      int start_row,
      int start_col,
      int end_row,
      int end_col);
  static bool isRowEmpty(const std::vector<std::vector<bool>>& grid,
                         int row,
                         int start_col,
                         int end_col);
  static bool isColEmpty(const std::vector<std::vector<bool>>& grid,
                         int col,
                         int start_row,
                         int end_row);
  float calSingleNotchPenalty(int width, int height);
  void calNotchPenalty();
  void calMacroBlockagePenalty();
  void calFixedMacrosPenalty();

  std::vector<odb::Point> getClustersLocations() const;
  void setClustersLocations(const std::vector<odb::Point>& clusters_locations);
  // Only for Cluster Placement:
  void attemptCentralization(float pre_cost);
  void moveFloorplan(const odb::Point& offset);

  void findFixedMacros();

  std::vector<odb::Rect> blockages_;
  std::vector<odb::Rect> fixed_macros_;

  Cluster* root_;

  // notch threshold
  int notch_h_th_;
  int notch_v_th_;

  int adjust_h_th_;  // the threshold for adjust hard macro clusters
                     // horizontally
  int adjust_v_th_;  // the threshold for adjust hard macro clusters
                     // vertically

  // additional penalties
  float boundary_weight_ = 0.0;
  float macro_blockage_weight_ = 0.0;
  float notch_weight_ = 0.0;
  const float fixed_macros_weight_ = 100.0;

  float boundary_penalty_ = 0.0;
  float notch_penalty_ = 0.0;
  float macro_blockage_penalty_ = 0.0;
  float fixed_macros_penalty_ = 0.0;

  float pre_boundary_penalty_ = 0.0;
  float pre_notch_penalty_ = 0.0;
  float pre_macro_blockage_penalty_ = 0.0;

  float norm_boundary_penalty_ = 0.0;
  float norm_notch_penalty_ = 0.0;
  float norm_macro_blockage_penalty_ = 0.0;
  float norm_fixed_macros_penalty_ = 0.0;

  // action prob
  float resize_prob_ = 0.0;

  bool enhancements_on_ = false;
  bool centralization_was_reverted_ = false;
  bool force_centralization_ = false;
};

}  // namespace mpl
