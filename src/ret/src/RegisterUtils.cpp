// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "RegisterUtils.h"

#include <cstring>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/Sequential.hh"

namespace ret {

namespace {

const sta::Sequential* singleSequential(const sta::LibertyCell* cell)
{
  const sta::Sequential* seq = nullptr;
  int count = 0;
  for (const sta::Sequential& s : cell->sequentials()) {
    seq = &s;
    count++;
  }
  return count == 1 ? seq : nullptr;
}

// 0/1 when the net is a supply net or is driven by a tie cell, else -1.
int constNetValue(sta::dbSta* sta, odb::dbNet* net)
{
  if (net == nullptr) {
    return -1;
  }
  if (net->getSigType() == odb::dbSigType::POWER) {
    return 1;
  }
  if (net->getSigType() == odb::dbSigType::GROUND) {
    return 0;
  }
  sta::dbNetwork* network = sta->getDbNetwork();
  for (odb::dbITerm* iterm : net->getITerms()) {
    if (iterm->getIoType() != odb::dbIoType::OUTPUT) {
      continue;
    }
    sta::Pin* pin = network->dbToSta(iterm);
    sta::LibertyPort* port = pin ? network->libertyPort(pin) : nullptr;
    const sta::FuncExpr* func = port ? port->function() : nullptr;
    if (func == nullptr) {
      return -1;
    }
    if (func->op() == sta::FuncExpr::Op::one) {
      return 1;
    }
    if (func->op() == sta::FuncExpr::Op::zero) {
      return 0;
    }
    return -1;
  }
  return -1;
}

// Ternary evaluation of a control expression under the constants on the
// register's input nets: 0, 1, or -1 (unknown).
int evalConstExpr(sta::dbSta* sta, odb::dbInst* inst, const sta::FuncExpr* e)
{
  if (e == nullptr) {
    return -1;
  }
  switch (e->op()) {
    case sta::FuncExpr::Op::port: {
      for (odb::dbITerm* iterm : inst->getITerms()) {
        if (strcmp(iterm->getMTerm()->getConstName(), e->port()->name().c_str())
            == 0) {
          return constNetValue(sta, iterm->getNet());
        }
      }
      return -1;
    }
    case sta::FuncExpr::Op::not_: {
      const int v = evalConstExpr(sta, inst, e->left());
      return v < 0 ? -1 : 1 - v;
    }
    case sta::FuncExpr::Op::and_: {
      const int a = evalConstExpr(sta, inst, e->left());
      const int b = evalConstExpr(sta, inst, e->right());
      if (a == 0 || b == 0) {
        return 0;
      }
      return (a == 1 && b == 1) ? 1 : -1;
    }
    case sta::FuncExpr::Op::or_: {
      const int a = evalConstExpr(sta, inst, e->left());
      const int b = evalConstExpr(sta, inst, e->right());
      if (a == 1 || b == 1) {
        return 1;
      }
      return (a == 0 && b == 0) ? 0 : -1;
    }
    case sta::FuncExpr::Op::xor_: {
      const int a = evalConstExpr(sta, inst, e->left());
      const int b = evalConstExpr(sta, inst, e->right());
      return (a < 0 || b < 0) ? -1 : (a ^ b);
    }
    case sta::FuncExpr::Op::one:
      return 1;
    case sta::FuncExpr::Op::zero:
      return 0;
  }
  return -1;
}

void collectPorts(const sta::FuncExpr* e,
                  std::vector<const sta::LibertyPort*>& ports)
{
  if (e == nullptr) {
    return;
  }
  if (e->op() == sta::FuncExpr::Op::port) {
    ports.push_back(e->port());
    return;
  }
  collectPorts(e->left(), ports);
  collectPorts(e->right(), ports);
}

}  // namespace

bool isDontTouchRegister(sta::dbSta* sta, odb::dbInst* inst)
{
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getSigType() == odb::dbSigType::SCAN) {
      return true;
    }
  }
  sta::LibertyCell* cell = sta->getDbNetwork()->libertyCell(inst);
  if (cell == nullptr) {
    return true;
  }
  const sta::Sequential* seq = singleSequential(cell);
  if (seq == nullptr || !seq->isRegister()) {
    return true;
  }
  // Asynchronous controls tied inactive make the register plain in practice.
  for (const sta::FuncExpr* control : {seq->clear(), seq->preset()}) {
    if (control != nullptr && evalConstExpr(sta, inst, control) != 0) {
      return true;
    }
  }
  return false;
}

bool isRegisterDataPin(sta::dbSta* sta, odb::dbITerm* pin)
{
  if (pin->getSigType() == odb::dbSigType::CLOCK) {
    return false;
  }
  sta::LibertyCell* cell = sta->getDbNetwork()->libertyCell(pin->getInst());
  if (cell == nullptr) {
    return true;
  }
  const sta::Sequential* seq = singleSequential(cell);
  if (seq == nullptr) {
    return true;
  }
  std::vector<const sta::LibertyPort*> ports;
  collectPorts(seq->data(), ports);
  for (const sta::LibertyPort* port : ports) {
    if (strcmp(port->name().c_str(), pin->getMTerm()->getConstName()) == 0) {
      return true;
    }
  }
  return false;
}

odb::dbITerm* registerDataPin(sta::dbSta* sta, odb::dbInst* inst)
{
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getIoType() != odb::dbIoType::INPUT) {
      continue;
    }
    const odb::dbSigType sig = iterm->getSigType();
    if (sig == odb::dbSigType::CLOCK || sig == odb::dbSigType::POWER
        || sig == odb::dbSigType::GROUND) {
      continue;
    }
    if (isRegisterDataPin(sta, iterm)) {
      return iterm;
    }
  }
  return nullptr;
}

bool isNegedgeRegister(sta::dbSta* sta, odb::dbInst* inst)
{
  sta::LibertyCell* cell = sta->getDbNetwork()->libertyCell(inst);
  const sta::Sequential* seq = cell ? singleSequential(cell) : nullptr;
  if (seq == nullptr || !seq->isRegister()) {
    return false;  // latches are don't-touch, not negative-edge registers
  }
  const sta::FuncExpr* clock = seq->clock();
  return clock != nullptr && clock->op() == sta::FuncExpr::Op::not_;
}

odb::dbNet* registerClockNet(odb::dbInst* inst)
{
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getSigType() == odb::dbSigType::CLOCK) {
      return iterm->getNet();
    }
  }
  return nullptr;
}

}  // namespace ret
