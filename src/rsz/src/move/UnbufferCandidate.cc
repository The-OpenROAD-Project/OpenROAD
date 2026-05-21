// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "src/rsz/src/move/UnbufferCandidate.hh"

#include <vector>

#include "src/rsz/include/rsz/Resizer.hh"
#include "src/rsz/src/OptimizerTypes.hh"
#include "src/rsz/src/move/MoveCandidate.hh"
#include "src/sta/include/sta/Network.hh"

namespace rsz {

UnbufferCandidate::UnbufferCandidate(Resizer& resizer,
                                     const Target& target,
                                     sta::Instance* drvr)
    : MoveCandidate(resizer, target), drvr_(drvr)
{
}

MoveResult UnbufferCandidate::apply()
{
  const bool accepted = resizer_.removeBuffer(drvr_);
  return {
      .accepted = accepted,
      .type = MoveType::kUnbuffer,
      .move_count = accepted ? 1 : 0,
      .touched_instances = accepted ? std::vector<sta::Instance*>{drvr_}
                                    : std::vector<sta::Instance*>{},
  };
}

}  // namespace rsz
