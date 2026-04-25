// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "BufferCandidate.hh"

#include <vector>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "Rebuffer.hh"
#include "rsz/Resizer.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

BufferCandidate::BufferCandidate(Resizer& resizer,
                                 const Target& target,
                                 sta::Pin* driver_pin)
    : MoveCandidate(resizer, target), driver_pin_(driver_pin)
{
}

MoveResult BufferCandidate::apply()
{
  // Let Rebuffer choose and materialize the buffered tree at apply time.
  const int rebuffer_count = resizer_.rebuffer().rebufferPin(driver_pin_);
  if (rebuffer_count <= 0) {
    debugPrint(resizer_.logger(),
               RSZ,
               "buffer_move",
               2,
               "REJECT BufferMove {}: Couldn't insert any buffers",
               resizer_.network()->pathName(driver_pin_));
    return rejectedMove();
  }

  sta::Instance* drvr_inst = resizer_.network()->instance(driver_pin_);
  debugPrint(resizer_.logger(),
             RSZ,
             "buffer_move",
             1,
             "ACCEPT BufferMove {}: Inserted {} buffers",
             resizer_.network()->pathName(driver_pin_),
             rebuffer_count);
  return {
      .accepted = true,
      .type = MoveType::kBuffer,
      .move_count = rebuffer_count,
      .touched_instances = {drvr_inst},
  };
}

}  // namespace rsz
