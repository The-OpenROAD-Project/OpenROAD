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
      if (!clkgate_cell_ && cell->isClockGate()) {
        if (!cell->isClockGateLatchPosedge()) {
          logger_->warn(CGT,
                        11,
                        "Skipping clock gate cell {}. Clock gates other than "
                        "with posedge latches are not supported",
                        cell->name());
          continue;
        }
        clkgate_cell_ = cell;
        sta::LibertyCellPortIterator port_iter(cell);
        while (port_iter.hasNext()) {
          auto port = port_iter.next();
          if (port->isClockGateEnable()) {
            clkgate_enable_ = port;
          } else if (port->isClockGateClock()) {
            clkgate_clk_ = port;
          } else if (port->isClockGateOut()) {
            clkgate_output_ = port;
          }
        }
        continue;
      }
      sta::LibertyCellPortIterator port_iter(cell);
      while (port_iter.hasNext()) {
        auto port = port_iter.next();
        if (port->direction()->isAnyInput()) {
          assert(!port->function());
        } else if (auto expr = port->function()) {
          if (expr->op() == sta::FuncExpr::Op::and_) {
            if (expr->left()->op() == sta::FuncExpr::Op::port
                && expr->right()->op() == sta::FuncExpr::Op::port) {
              and_cell_ = cell;
              and_inputs_[0] = expr->left()->port();
              and_inputs_[1] = expr->right()->port();
              and_output_ = port;
            }
          }
          if (expr->op() == sta::FuncExpr::Op::or_) {
            if (expr->left()->op() == sta::FuncExpr::Op::port
                && expr->right()->op() == sta::FuncExpr::Op::port) {
              or_cell_ = cell;
              or_inputs_[0] = expr->left()->port();
              or_inputs_[1] = expr->right()->port();
              or_output_ = port;
            }
          }
          if (expr->op() == sta::FuncExpr::Op::not_) {
            if (expr->left()->op() == sta::FuncExpr::Op::port
                && !expr->right()) {
              not_cell_ = cell;
              not_input_ = expr->left()->port();
              not_output_ = port;
            }
          }
        }
      }
    }
    if (and_cell_ && not_cell_ && or_cell_) {
      break;
    }
  }
  delete lib_iter;
  if (!and_cell_) {
    logger_->error(CGT, 6, "Cannot find AND stdcell");
  }
  if (!or_cell_) {
    logger_->error(CGT, 7, "Cannot find OR stdcell");
  }
  if (!not_cell_) {
    logger_->error(CGT, 8, "Cannot find NOT stdcell");
  }
  if (!clkgate_cell_) {
    logger_->error(CGT, 9, "Cannot find a compatible clock gate stdcell");
  }
}

}  // namespace cgt
