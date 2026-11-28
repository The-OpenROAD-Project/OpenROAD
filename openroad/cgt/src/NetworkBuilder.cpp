// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "NetworkBuilder.h"

#include <cassert>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/VerilogReader.hh"
#include "utl/Logger.h"

using utl::CGT;
using utl::UniquePtrWithDeleter;

namespace cgt {

InstanceBuilder& InstanceBuilder::connectPort(sta::LibertyPort* const port,
                                              sta::Net* const net)
{
  assert(port);
  auto pin = network_->findPin(inst_, port);
  const char* qualifier = "";
  if (port->isClock()) {
    qualifier = "clock ";
  } else if (port->direction() == sta::PortDirection::input()) {
    qualifier = "input";
  } else if (port->direction() == sta::PortDirection::output()) {
    qualifier = "output";
  }
  debugPrint(logger_,
             CGT,
             "clock_gating",
             3,
             "Connecting {} {} ({}/{}) to {}",
             qualifier,
             network_->name(pin),
             network_->name(inst_),
             port->name(),
             network_->name(net));
  network_->connectPin(pin, net);
  return *this;
}

// Records cell as gate when port's function is op applied directly to input
// ports: one for not_, two otherwise.
static void matchGate(sta::LibertyCell* const cell,
                      sta::LibertyPort* const port,
                      const sta::FuncExpr* const expr,
                      const sta::FuncExpr::Op op,
                      CellPorts& gate)
{
  if (expr->op() != op || expr->left()->op() != sta::FuncExpr::Op::port) {
    return;
  }
  if (op == sta::FuncExpr::Op::not_) {
    if (expr->right()) {
      return;
    }
    gate.inputs = {expr->left()->port(), nullptr};
  } else {
    if (expr->right()->op() != sta::FuncExpr::Op::port) {
      return;
    }
    gate.inputs = {expr->left()->port(), expr->right()->port()};
  }
  gate.cell = cell;
  gate.output = port;
}

void NetworkBuilder::init(utl::Logger* const logger,
                          sta::dbNetwork* const network)
{
  logger_ = logger;
  network_ = network;

  // Find Liberty cells
  sta::LibertyLibraryIterator* lib_iter = network_->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    sta::LibertyLibrary* lib = lib_iter->next();
    sta::LibertyCellIterator cell_iter(lib);
    while (cell_iter.hasNext()) {
      auto cell = cell_iter.next();
      if (cell->dontUse()) {
        continue;
      }
      if (!clkgate_.cell && cell->isClockGate()) {
        setClockGate(cell);
        continue;
      }
      sta::LibertyCellPortIterator port_iter(cell);
      while (port_iter.hasNext()) {
        auto port = port_iter.next();
        if (port->direction()->isAnyInput()) {
          assert(!port->function());
        } else if (auto expr = port->function()) {
          matchGate(cell, port, expr, sta::FuncExpr::Op::and_, and_);
          matchGate(cell, port, expr, sta::FuncExpr::Op::or_, or_);
          matchGate(cell, port, expr, sta::FuncExpr::Op::not_, not_);
        }
      }
    }
    if (and_.cell && not_.cell && or_.cell) {
      break;
    }
  }
  delete lib_iter;
  if (!and_.cell) {
    logger_->error(CGT, 6, "Cannot find AND stdcell");
  }
  if (!or_.cell) {
    logger_->error(CGT, 7, "Cannot find OR stdcell");
  }
  if (!not_.cell) {
    logger_->error(CGT, 8, "Cannot find NOT stdcell");
  }
  if (!clkgate_.cell) {
    logger_->error(CGT, 9, "Cannot find a compatible clock gate stdcell");
  }
}

void NetworkBuilder::setClockGate(sta::LibertyCell* const cell)
{
  if (!cell->isClockGateLatchPosedge()) {
    logger_->warn(CGT,
                  11,
                  "Skipping clock gate cell {}. Clock gates other than "
                  "with posedge latches are not supported",
                  cell->name());
    return;
  }
  clkgate_.cell = cell;
  sta::LibertyCellPortIterator port_iter(cell);
  while (port_iter.hasNext()) {
    auto port = port_iter.next();
    if (port->isClockGateEnable()) {
      clkgate_.inputs[0] = port;
    } else if (port->isClockGateClock()) {
      clkgate_.clk = port;
    } else if (port->isClockGateOut()) {
      clkgate_.output = port;
    }
  }
}

}  // namespace cgt
