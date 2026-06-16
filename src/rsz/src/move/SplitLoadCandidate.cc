// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SplitLoadCandidate.hh"

#include <memory>
#include <utility>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "db_sta/dbSta.hh"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

SplitLoadCandidate::SplitLoadCandidate(Resizer& resizer,
                                       const Target& target,
                                       sta::Net* drvr_net,
                                       sta::LibertyCell* buffer_cell,
                                       const odb::Point& drvr_loc,
                                       std::unique_ptr<sta::PinSet> load_pins)
    : MoveCandidate(resizer, target),
      drvr_net_(drvr_net),
      buffer_cell_(buffer_cell),
      drvr_loc_(drvr_loc),
      load_pins_(std::move(load_pins))
{
}

SplitLoadCandidate::~SplitLoadCandidate() = default;

MoveResult SplitLoadCandidate::apply()
{
  return applySplitBuffer();
}

int SplitLoadCandidate::resizeInsertedBuffer(sta::Instance* buffer) const
{
  sta::LibertyPort *input, *output;
  buffer_cell_->bufferPorts(input, output);
  sta::Pin* buffer_out_pin = resizer_.network()->findPin(buffer, output);
  return resizer_.resizeToTargetSlew(buffer_out_pin);
}

void SplitLoadCandidate::invalidateAffectedParasitics(
    sta::Instance* buffer) const
{
  sta::LibertyPort *input, *output;
  buffer_cell_->bufferPorts(input, output);
  sta::Pin* buffer_out_pin = resizer_.network()->findPin(buffer, output);
  resizer_.invalidateParasitics(drvr_net_);
  resizer_.invalidateParasitics(resizer_.network()->net(buffer_out_pin));
}

MoveResult SplitLoadCandidate::applySplitBuffer()
{
  sta::Instance* buffer
      = resizer_.insertBufferBeforeLoads(drvr_net_,
                                         load_pins_.get(),
                                         buffer_cell_,
                                         &drvr_loc_,
                                         "split",
                                         nullptr,
                                         odb::dbNameUniquifyType::IF_NEEDED);
  if (buffer == nullptr) {
    debugPrint(resizer_.logger(),
               RSZ,
               "split_load_move",
               2,
               "REJECT SplitLoadMove {}: Couldn't insert buffer",
               resizer_.network()->pathName(target_.driver_pin));
    return rejectedMove();
  }

  debugPrint(resizer_.logger(),
             RSZ,
             "split_load_move",
             1,
             "ACCEPT SplitLoadMove {}: Inserted buffer {}",
             resizer_.network()->pathName(target_.driver_pin),
             resizer_.network()->pathName(buffer));

  static_cast<void>(resizeInsertedBuffer(buffer));
  invalidateAffectedParasitics(buffer);

  return {
      .accepted = true,
      .type = MoveType::kSplitLoad,
      .move_count = 1,
      .touched_instances = {buffer},
  };
}

}  // namespace rsz
