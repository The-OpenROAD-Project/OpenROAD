// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "flow/import.h"

#include <cstdint>
#include <cstdlib>
#include <map>
#include <string_view>
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

    std::map<std::string_view, uint32_t> lib_input_offset;
    std::map<std::string_view, uint32_t> lib_output_offset;
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
        lib_input_offset[port->name()] = lib_input_width;
        lib_input_width += port->size();
      } else if (dir->isOutput()) {
        lib_output_offset[port->name()] = lib_output_width;
        lib_output_width += port->size();
      } else if (dir->isBidirect()) {
        lib_input_offset[port->name()] = lib_input_width;
        lib_output_offset[port->name()] = lib_output_width;
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
                    " Verilog instantiation has {} bits, Liberty has {} bits.",
                    other->cellType(),
                    other_input_width,
                    lib_input_width);
    }

    if (other_output_width != lib_output_width) {
      logger->error(utl::SYN,
                    11,
                    "importTargets: cell '{}' output width mismatch:"
                    " Verilog instantiation has {} bits, Liberty has {} bits.",
                    other->cellType(),
                    other_output_width,
                    lib_output_width);
    }

    // Concatenate all input port values into a single Bundle.
    Bundle inputs = Bundle::sentinel(lib_input_width);
    for (auto& port : other->ports()) {
      if (port.direction == Other::Port::kInput
          || port.direction == Other::Port::kInOut) {
        sta::LibertyPort* lib_port = cell->findLibertyPort(port.name);
        if (!lib_port || !lib_input_offset.contains(port.name)) {
          logger->error(
              utl::SYN,
              35,
              "importTargets: cell '{}' Liberty definition is missing "
              "port '{}' connected in Verilog instantiation.",
              other->cellType(),
              port.name);
        }

        if (((uint32_t) lib_port->size()) != port.width) {
          logger->error(
              utl::SYN,
              36,
              "importTargets: cell '{}' instantiated with mismatched width "
              "of port '{}': {} in Verilog vs {} in Liberty",
              other->cellType(),
              port.name,
              port.width,
              lib_port->size());
        }

        uint32_t offset = lib_input_offset.at(port.name);
        for (uint32_t i = 0; i < port.width; i++) {
          inputs.mutableNet(offset + i) = port.value[i];
        }
      }
    }

    // Create Target and replace outputs.
    BundleView old_output = g.output(other);
    Bundle target_output = g.add<Target>(cell, std::move(inputs));
    Bundle output_replacement = Bundle::sentinel(lib_output_width);

    uint32_t offset = 0;
    for (auto& port : other->ports()) {
      if (port.direction == Other::Port::kOutput
          || port.direction == Other::Port::kInOut) {
        sta::LibertyPort* lib_port = cell->findLibertyPort(port.name);
        if (!lib_port || !lib_output_offset.contains(port.name)) {
          logger->error(
              utl::SYN,
              37,
              "importTargets: cell '{}' Liberty definition is missing "
              "port '{}' connected in Verilog instantiation.",
              other->cellType(),
              port.name);
        }

        if (((uint32_t) lib_port->size()) != port.width) {
          logger->error(
              utl::SYN,
              38,
              "importTargets: cell '{}' instantiated with mismatched width "
              "of port '{}': {} in Verilog vs {} in Liberty",
              other->cellType(),
              port.name,
              port.width,
              lib_port->size());
        }

        uint32_t lib_offset = lib_output_offset.at(port.name);
        for (uint32_t i = 0; i < port.width; i++) {
          output_replacement.mutableNet(offset + i)
              = target_output[lib_offset + i];
        }
        offset += port.width;
      }
    }

    g.forceReplace(old_output, BundleView(output_replacement));
    g.removeInstance(
        const_cast<Instance*>(static_cast<const Instance*>(other)));
  });
}

}  // namespace syn
