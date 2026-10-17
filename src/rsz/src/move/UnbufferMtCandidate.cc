// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "UnbufferMtCandidate.hh"

#include <memory>
#include <vector>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"

namespace rsz {

UnbufferMtCandidate::UnbufferMtCandidate(Resizer& resizer, const Target& target)
    : MoveCandidate(resizer, target)
{
}

MoveImpact UnbufferMtCandidate::impact() const
{
  sta::Instance* buffer = target().inst(resizer_);
  MoveImpact impact = MoveCandidate::impact();
  impact.complete = true;
  impact.write_instances.push_back(buffer);
  impact.timing_effect_instances.push_back(buffer);
  sta::Network* network = resizer_.network();
  sta::LibertyCell* cell = network->libertyCell(buffer);
  sta::LibertyPort* input_port = nullptr;
  sta::LibertyPort* output_port = nullptr;
  if (cell != nullptr) {
    cell->bufferPorts(input_port, output_port);
  }

  for (sta::LibertyPort* port : {input_port, output_port}) {
    sta::Pin* pin = port != nullptr ? network->findPin(buffer, port) : nullptr;
    sta::Net* net = pin != nullptr ? network->net(pin) : nullptr;
    if (net == nullptr) {
      continue;
    }

    impact.write_nets.push_back(net);
    impact.timing_effect_nets.push_back(net);

    // The upstream driver and the loads change nets or load.  The load pins of
    // the output net move to the input net.
    std::unique_ptr<sta::NetConnectedPinIterator> pin_iter(
        network->connectedPinIterator(net));
    while (pin_iter->hasNext()) {
      const sta::Pin* net_pin = pin_iter->next();
      sta::Instance* inst = network->instance(net_pin);
      if (inst == nullptr || inst == buffer) {
        continue;
      }

      if (port == output_port) {
        impact.reconnect_pins.push_back(net_pin);
        impact.reconnect_instances.push_back(inst);
      }

      impact.timing_effect_instances.push_back(inst);
    }
  }

  return impact;
}

Estimate UnbufferMtCandidate::estimate()
{
  const float gain = target().unbuffer_gain.value();
  return {.legal = true,
          .score = gain,
          .raw_gain = gain,
          .reason = failReasonName(FailReason::kEstimateLegal)};
}

MoveResult UnbufferMtCandidate::apply()
{
  sta::Instance* buffer = target().inst(resizer_);
  const bool accepted = resizer_.removeBuffer(buffer);
  return {
      .accepted = accepted,
      .type = MoveType::kUnbuffer,
      .move_count = accepted ? 1 : 0,
      .touched_instances = accepted ? std::vector<sta::Instance*>{buffer}
                                    : std::vector<sta::Instance*>{},
  };
}

}  // namespace rsz
