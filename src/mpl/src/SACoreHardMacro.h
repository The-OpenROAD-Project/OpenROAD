// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

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

class SACoreHardMacro : public SimulatedAnnealingCore<HardMacro>
{
 public:
  SACoreHardMacro(PhysicalHierarchy* tree,
                  const odb::Rect& outline,
                  const std::vector<HardMacro>& macros,
                  const SACoreWeights& core_weights,
                  // probability of each action
                  float pos_swap_prob,
                  float neg_swap_prob,
                  float double_swap_prob,
                  float exchange_prob,
                  // Fast SA hyperparameter
                  float init_prob,
                  int max_num_step,
                  int num_perturb_per_step,
                  unsigned seed,
                  MplObserver* graphics,
                  utl::Logger* logger,
                  odb::dbBlock* block);

  void run() override;

  void setWeights(const SACoreWeights& weights);

  // Initialize the SA worker
  void initialize() override;
  void fillDeadSpace() override {}
  // print results
  void printResults() const;

 private:
  float calNormCost() const override;
  void calPenalty() override;

  void perturb() override;
  void saveState() override;
  void restoreState() override;
};

}  // namespace mpl
