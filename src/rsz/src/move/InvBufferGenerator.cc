// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "InvBufferGenerator.hh"

#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "InvBufferCandidate.hh"
#include "MoveCommitter.hh"
#include "OptimizerTypes.hh"
#include "db_sta/dbNetwork.hh"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/Path.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

InvBufferGenerator::InvBufferGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

std::vector<std::unique_ptr<MoveCandidate>> InvBufferGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;

  if (!target.canBePathDriver() || target.path_index < 2) {
    return candidates;
  }

  sta::Pin* drvr_pin = target.driver_pin;
  if (drvr_pin == nullptr && target.endpoint_path != nullptr) {
    drvr_pin = target.endpoint_path->pin(resizer_.staState());
  }
  if (drvr_pin == nullptr) {
    return candidates;
  }

  sta::dbNetwork* db_network = resizer_.dbNetwork();
  sta::Instance* drvr = db_network->instance(drvr_pin);
  sta::LibertyCell* drvr_cell
      = drvr != nullptr ? db_network->libertyCell(drvr) : nullptr;
  if (drvr == nullptr || drvr_cell == nullptr || !drvr_cell->isBuffer()) {
    return candidates;
  }

  // Reject if another pending/committed move on this buffer already claims it.
  // Same guard the unbuffer generator uses; note that the predicate returns
  // true when there is a blocking move.
  std::string reason;
  if (committer_.hasBlockingBufferRemovalMove(drvr, reason)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "inv_buffer_move",
               4,
               "Buffer {} is not inv-buffered because {}",
               db_network->name(drvr),
               reason);
    return candidates;
  }

  // Honor dont_touch / port-driven / multi-driver / SDC guards on the buffer
  if (!resizer_.canRemoveBuffer(drvr, /*honor_dont_touch_fixed=*/true)) {
    debugPrint(
        resizer_.logger(),
        RSZ,
        "inv_buffer_move",
        4,
        "Buffer {} is not inv-buffered because canRemoveBuffer rejected it",
        db_network->name(drvr));
    return candidates;
  }

  // Pick a replacement inverter cell with usable buffer ports, so apply()
  // cannot fail partway through the netlist mutation. Filter dont_use and
  // non-link cells.
  const float target_res = resizer_.cellDriveResistance(drvr_cell);
  const std::string& drvr_footprint = drvr_cell->footprint();
  sta::LibertyCell* best_inv = nullptr;
  float min_diff = std::numeric_limits<float>::max();

  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter(
      db_network->libertyLibraryIterator());
  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* lib = lib_iter->next();
    const sta::LibertyCellSeq* inverters = lib->inverters();
    if (inverters == nullptr) {
      continue;
    }
    for (sta::LibertyCell* cell : *inverters) {
      if (resizer_.dontUse(cell) || !resizer_.isLinkCell(cell)) {
        continue;
      }
      if (run_config_.match_cell_footprint && !drvr_footprint.empty()
          && !cell->footprint().empty()
          && drvr_footprint != cell->footprint()) {
        continue;
      }
      sta::LibertyPort* in_port = nullptr;
      sta::LibertyPort* out_port = nullptr;
      cell->bufferPorts(in_port, out_port);
      if (in_port == nullptr || out_port == nullptr) {
        continue;
      }
      const float diff
          = std::abs(resizer_.cellDriveResistance(cell) - target_res);
      if (diff < min_diff) {
        min_diff = diff;
        best_inv = cell;
      }
    }
  }

  if (best_inv == nullptr) {
    debugPrint(resizer_.logger(),
               RSZ,
               "inv_buffer_move",
               4,
               "Buffer {} has no usable inverter replacement",
               db_network->name(drvr));
    return candidates;
  }

  candidates.push_back(
      std::make_unique<InvBufferCandidate>(resizer_, target, drvr, best_inv));
  return candidates;
}

}  // namespace rsz
