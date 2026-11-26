// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "SACoreHardMacro.h"

#include <cmath>
#include <vector>

#include "MplObserver.h"
#include "SACore.h"
#include "clusterEngine.h"
#include "mpl-util.h"
#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace mpl {
using utl::MPL;

//////////////////////////////////////////////////////////////////
// Class SACoreHardMacro
// constructors
SACoreHardMacro::SACoreHardMacro(PhysicalHierarchy* tree,
                                 const odb::Rect& outline,
                                 const std::vector<HardMacro>& macros,
                                 const SACoreWeights& core_weights,
                                 const SACoreActionProbs& action_probs,
                                 const SAHyperparams& hyperparams,
                                 unsigned seed,
                                 MplObserver* graphics,
                                 utl::Logger* logger,
                                 odb::dbBlock* block)
    : SimulatedAnnealingCore<HardMacro>(tree,
                                        outline,
                                        macros,
                                        core_weights,
                                        action_probs,
                                        hyperparams,
                                        seed,
                                        graphics,
                                        logger,
                                        block)
{
}

void SACoreHardMacro::run()
{
  if (graphics_) {
    graphics_->startSA(
        "hard", hyperparams_.max_num_step, hyperparams_.num_perturb_per_step);
  }

  fastSA();

  if (graphics_) {
    graphics_->endSA(calNormCost());
  }
}

void SACoreHardMacro::setCoreWeights(const SACoreWeights& weights)
{
  penalties_[kArea].weight = weights.area;
  penalties_[kOutline].weight = weights.outline;
  penalties_[kWirelength].weight = weights.wirelength;
  penalties_[kGuidance].weight = weights.guidance;
  penalties_[kFence].weight = weights.fence;
}

}  // namespace mpl
