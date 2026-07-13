// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "flow/target_index.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "flow/combinational_mapper_npn.h"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "syn/synthesis.h"
#include "utl/Logger.h"

namespace syn::cm {

// Recursively converts a Liberty FuncExpr AST into a truth table (Truth6)
static Truth6 fexprEval(sta::FuncExpr* fexpr,
                        const std::vector<sta::LibertyPort*>& inputs)
{
  using Op = sta::FuncExpr::Op;
  switch (fexpr->op()) {
    case Op::port: {
      // Find the index of this port in inputs
      int in_idx;
      for (in_idx = 0; in_idx < inputs.size(); in_idx++) {
        if (inputs[in_idx] == fexpr->port()) {
          break;
        }
      }
      assert(in_idx != (int) inputs.size());
      // The function is 1 whenever this input = 1
      Truth6 ret = 0;
      for (int row = 0; row < (1 << (int) inputs.size()); row++) {
        if (row & 1 << in_idx) {
          ret |= (Truth6) 1 << row;
        }
      }
      return ret;
    }
    case Op::not_:
      return mask6(inputs.size()) & ~fexprEval(fexpr->left(), inputs);
    case Op::or_:
      return fexprEval(fexpr->left(), inputs)
             | fexprEval(fexpr->right(), inputs);
    case Op::and_:
      return fexprEval(fexpr->left(), inputs)
             & fexprEval(fexpr->right(), inputs);
    case Op::xor_:
      return fexprEval(fexpr->left(), inputs)
             ^ fexprEval(fexpr->right(), inputs);
    case Op::one:
      return mask6(inputs.size());
    case Op::zero:
      return 0;
  }
  assert(false);
  return 0;
}

void buildIndex(sta::Network* network,
                TargetIndex& index,
                utl::Logger* logger,
                const Synthesis& syn)
{
  std::unique_ptr<sta::LibertyLibraryIterator> lib_iter{
      network->libertyLibraryIterator()};
  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* lib = lib_iter->next();
    sta::LibertyCellIterator cell_iter(lib);
    while (cell_iter.hasNext()) {
      sta::LibertyCell* cell = cell_iter.next();
      if (cell->isSequential()) {
        debugPrint(logger,
                   utl::SYN,
                   "cm",
                   3,
                   "ignoring {} because of sequentials",
                   cell->name());
        continue;
      }

      // Collect the inputs and outputs
      std::vector<sta::LibertyPort*> inputs;
      std::vector<sta::LibertyPort*> outputs;

      sta::LibertyCellPortIterator port_iter(cell);
      bool skip = false;
      while (port_iter.hasNext()) {
        sta::LibertyPort* port = port_iter.next();
        if (port->isPwrGnd()) {
          continue;
        }
        if (port->direction()->isInput()) {
          inputs.push_back(port);
        } else if (port->direction()->isOutput()) {
          outputs.push_back(port);
        } else {
          skip = true;
          break;
        }
      }

      if (skip) {
        debugPrint(logger,
                   utl::SYN,
                   "cm",
                   3,
                   "ignoring {} because of unsupported port types",
                   cell->name());
        continue;
      }

      if (inputs.size() > kCutMaximum) {
        debugPrint(
            logger,
            utl::SYN,
            "cm",
            3,
            "ignoring {} because of too many inputs ({}, above limit {})",
            cell->name(),
            inputs.size(),
            kCutMaximum);
        continue;
      }

      if (logger->debugCheck(utl::SYN, "cm", 3)) {
        debugPrint(logger, utl::SYN, "cm", 3, "indexing {}", cell->name());
        debugPrint(logger, utl::SYN, "cm", 3, "  inputs:");
        for (auto input : inputs) {
          debugPrint(logger, utl::SYN, "cm", 3, "   - {}", input->name());
        }
        debugPrint(logger, utl::SYN, "cm", 3, "  outputs:");
        for (auto output : outputs) {
          if (output->function()) {
            debugPrint(logger,
                       utl::SYN,
                       "cm",
                       3,
                       "   - {} (function {:X})",
                       output->name(),
                       fexprEval(output->function(), inputs));
          } else {
            debugPrint(logger,
                       utl::SYN,
                       "cm",
                       3,
                       "   - {} (missing function)",
                       output->name());
          }
        }
      }

      // Handle tie cells (0 inputs, 2 outputs)
      if (inputs.empty()) {
        if (outputs.size() == 2 && outputs[0]->function()
            && outputs[1]->function()) {
          Truth6 f0 = fexprEval(outputs[0]->function(), inputs);
          Truth6 f1 = fexprEval(outputs[1]->function(), inputs);
          if ((f0 == 0 && f1 == 1) || (f0 == 1 && f1 == 0)) {
            if (!index.tie_low.first
                || cell->area() < index.tie_low.first->area()) {
              index.tie_low = {cell, (f0 == 0) ? 0 : 1};
            }
            if (!index.tie_high.first
                || cell->area() < index.tie_high.first->area()) {
              index.tie_high = {cell, (f0 == 1) ? 0 : 1};
            }
          }
        }

        if (outputs.size() == 1 && outputs[0]->function()) {
          Truth6 f0 = fexprEval(outputs[0]->function(), inputs);
          if (f0 == 0
              && (!index.tie_low.first
                  || cell->area() < index.tie_low.first->area())) {
            index.tie_low = {cell, 0};
          }
          if (f0 == 1
              && (!index.tie_high.first
                  || cell->area() < index.tie_high.first->area())) {
            index.tie_high = {cell, 0};
          }
        }
      }

      // Check dont use after detecting tie cells to work around nangate45
      // marking tie cells dont use inside .lib
      if (syn.dontUse(cell)) {
        debugPrint(logger,
                   utl::SYN,
                   "cm",
                   3,
                   "ignoring {} because of dont use",
                   cell->name());
        continue;
      }

      if (outputs.size() != 1 || !outputs[0]->function()) {
        continue;
      }

      Truth6 func = fexprEval(outputs[0]->function(), inputs);

      // Track inverter
      if (inputs.size() == 1 && func == 0b01) {
        if (!index.inverter || cell->area() < index.inverter->area()) {
          index.inverter = cell;
        }
      }

      // Register all NPN representatives
      npnSemiclassAllRepr(
          func, inputs.size(), [&](const Truth6 repr, const NPN& npn) {
            index.classes[{(int) inputs.size(), repr}].push_back(
                MapTarget{.cell = cell, .via = npn.inv()});
          });
    }
  }

  // Prune targets: keep smallest cell per cFingerprint
  for (auto& [key, targets] : index.classes) {
    std::ranges::sort(targets, [](const MapTarget& a, const MapTarget& b) {
      return std::make_pair(a.via.cFingerprint(), a.cell->area())
             < std::make_pair(b.via.cFingerprint(), b.cell->area());
    });
    auto unique_range = std::ranges::unique(
        targets, [](const MapTarget& a, const MapTarget& b) {
          return a.via.cFingerprint() == b.via.cFingerprint();
        });
    targets.erase(unique_range.begin(), targets.end());
  }
}

}  // namespace syn::cm
