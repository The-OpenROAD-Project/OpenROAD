// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "flow/import.h"

#include <cstdint>
#include <cstdlib>
#include <utility>

#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/PortDirection.hh"
#include "syn/ir/Bundle.h"
#include "syn/ir/Graph.h"
#include "syn/ir/Instance.h"
#include "utl/Logger.h"

namespace syn {

void importTargets(Graph& g, sta::Network* network, utl::Logger* logger)
{
  g.normalize();

  g.forEachInstance([&](const Instance* inst) {
    if (!inst->is<Other>()) {
      return;
    }
    auto* other = inst->as<Other>();
    sta::LibertyCell* cell = network->findLibertyCell(other->cellType());
    if (!cell) {
      return;
    }

    // Build expected input/output widths from liberty cell ports.
    uint32_t lib_input_width = 0;
    uint32_t lib_output_width = 0;
    sta::LibertyCellPortIterator port_iter(cell);
    while (port_iter.hasNext()) {
      sta::LibertyPort* port = port_iter.next();
      if (port->isPwrGnd()) {
        continue;
      }
      sta::PortDirection* dir = port->direction();
      if (dir->isInput()) {
        lib_input_width += port->size();
      } else if (dir->isOutput()) {
        lib_output_width += port->size();
      } else if (dir->isBidirect()) {
        lib_input_width += port->size();
        lib_output_width += port->size();
      }
    }

    // Compute Other's input/output widths.
    uint32_t other_input_width = 0;
    uint32_t other_output_width = other->outputWidth();
    for (auto& port : other->ports()) {
      if (port.direction == Other::Port::kInput) {
        other_input_width += port.width;
      } else if (port.direction == Other::Port::kInOut) {
        other_input_width += port.width;
      }
    }

    if (other_input_width != lib_input_width) {
      logger->error(utl::SYN,
                    10,
                    "importTargets: cell '{}' input width mismatch:"
                    " Other has {} bits, Liberty has {} bits. This is an "
                    "internal tool error.",
                    other->cellType(),
                    other_input_width,
                    lib_input_width);
    }

    if (other_output_width != lib_output_width) {
      logger->error(utl::SYN,
                    11,
                    "importTargets: cell '{}' output width mismatch:"
                    " Other has {} bits, Liberty has {} bits. This is an "
                    "internal tool error.",
                    other->cellType(),
                    other_output_width,
                    lib_output_width);
    }

    // Concatenate all input port values into a single Bundle.
    Bundle inputs;
    for (auto& port : other->ports()) {
      if (port.direction == Other::Port::kInput
          || port.direction == Other::Port::kInOut) {
        inputs = inputs.concat(port.value);
      }
    }

    // Create Target and replace outputs.
    BundleView old_output = g.output(other);
    Bundle new_output = g.add<Target>(cell, std::move(inputs));
    g.forceReplace(old_output, BundleView(new_output));
    g.removeInstance(
        const_cast<Instance*>(static_cast<const Instance*>(other)));
  });
}

}  // namespace syn
