// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "MplObserver.h"
#include "SACore.h"
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
                  const SACoreActionProbs& action_probs,
                  const SAHyperparams& hyperparams,
                  unsigned seed,
                  MplObserver* graphics,
                  utl::Logger* logger,
                  odb::dbBlock* block);

  void run() override;

  void setCoreWeights(const SACoreWeights& weights);

  void fillDeadSpace() override {}
};

}  // namespace mpl
