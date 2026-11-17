// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "SwapPinsEquivalence.hh"

#include <algorithm>
#include <cstddef>
#include <unordered_map>
#include <utility>
#include <vector>

#include "OptimizerTypes.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"

namespace rsz {

namespace {

using PortStimulus
    = std::unordered_map<const sta::LibertyPort*, std::vector<bool>>;

// Inputs beyond this make the truth tables too large to simulate.
constexpr size_t kMaxSimulatedInputs = 16;

bool evaluate(utl::Logger* logger,
              const sta::FuncExpr* expr,
              const PortStimulus& stimulus,
              const size_t row)
{
  using Op = sta::FuncExpr::Op;
  switch (expr->op()) {
    case Op::not_:
      return !evaluate(logger, expr->left(), stimulus, row);
    case Op::and_:
      return evaluate(logger, expr->left(), stimulus, row)
             && evaluate(logger, expr->right(), stimulus, row);
    case Op::or_:
      return evaluate(logger, expr->left(), stimulus, row)
             || evaluate(logger, expr->right(), stimulus, row);
    case Op::xor_:
      return evaluate(logger, expr->left(), stimulus, row)
             ^ evaluate(logger, expr->right(), stimulus, row);
    case Op::one:
      return true;
    case Op::zero:
      return false;
    case Op::port:
      return stimulus.at(expr->port())[row];
  }

  logger->error(
      utl::RSZ, kMsgUnknownFuncExprOp, "unrecognized expr op from OpenSTA");
  return false;
}

std::vector<bool> truthTable(utl::Logger* logger,
                             const sta::FuncExpr* expr,
                             const PortStimulus& stimulus)
{
  const size_t rows = size_t{1} << stimulus.size();
  std::vector<bool> table(rows);
  for (size_t row = 0; row < rows; ++row) {
    table[row] = evaluate(logger, expr, stimulus, row);
  }
  return table;
}

}  // namespace

std::vector<sta::LibertyPort*> equivalentSwapInputs(
    utl::Logger* logger,
    const sta::LibertyCell* cell,
    const sta::LibertyPort* input_port)
{
  if (cell->isSequential() || cell->isIsolationCell()) {
    return {};
  }

  std::vector<const sta::LibertyPort*> inputs;
  std::vector<const sta::FuncExpr*> functions;
  bool has_input_in_function = false;
  sta::LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext()) {
    const sta::LibertyPort* port = port_iter.next();
    const sta::PortDirection* direction = port->direction();
    // A bus or bundle input is iterated as one port, not as its bits, and an
    // output without a function is opaque (a block's timing model): the
    // simulation below cannot tell whether the inputs are interchangeable.
    if (port->hasMembers()) {
      return {};
    }
    if (direction->isOutput()) {
      const sta::FuncExpr* function = port->function();
      if (function == nullptr) {
        return {};
      }
      has_input_in_function |= function->hasPort(input_port);
      functions.push_back(function);
    } else if (direction->isInput()) {
      inputs.push_back(port);
    } else if (!port->isPwrGnd()) {
      return {};
    }
  }

  // An input no output depends on, such as on a cell whose outputs are all
  // constant, is trivially interchangeable with every other input.
  if (functions.empty() || inputs.size() < 2
      || inputs.size() > kMaxSimulatedInputs || !has_input_in_function
      || std::ranges::find(inputs, input_port) == inputs.end()) {
    return {};
  }

  // Input i toggles every 2^i rows, so the rows cover every input vector.
  PortStimulus stimulus;
  const size_t rows = size_t{1} << inputs.size();
  for (size_t i = 0; i < inputs.size(); ++i) {
    std::vector<bool>& column = stimulus[inputs[i]];
    column.resize(rows);
    for (size_t row = 0; row < rows; ++row) {
      column[row] = ((row >> i) & 0x1) != 0;
    }
  }

  std::vector<std::vector<bool>> tables;
  tables.reserve(functions.size());
  for (const sta::FuncExpr* function : functions) {
    tables.push_back(truthTable(logger, function, stimulus));
  }

  std::vector<sta::LibertyPort*> equivalent;
  for (const sta::LibertyPort* candidate : inputs) {
    if (candidate == input_port) {
      continue;
    }
    std::swap(stimulus.at(input_port), stimulus.at(candidate));
    bool interchangeable = true;
    for (size_t i = 0; i < functions.size() && interchangeable; ++i) {
      interchangeable = truthTable(logger, functions[i], stimulus) == tables[i];
    }
    std::swap(stimulus.at(input_port), stimulus.at(candidate));
    if (interchangeable) {
      equivalent.push_back(const_cast<sta::LibertyPort*>(candidate));
    }
  }

  std::ranges::sort(
      equivalent, {}, [](const sta::LibertyPort* port) { return port->id(); });
  return equivalent;
}

}  // namespace rsz
