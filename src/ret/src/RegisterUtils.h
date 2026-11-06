// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <utility>
#include <vector>

#include "odb/db.h"

namespace sta {
class dbSta;
}

namespace ret {

// Register classification shared by the graph builder, the search and the
// apply step, so the three can never disagree about which registers move.

// True for a register retiming must never move: scan cells, latches, cells
// without a liberty view or with more than one sequential element, and
// asynchronous set/reset registers whose controls are not tied inactive.
bool isDontTouchRegister(sta::dbSta* sta, odb::dbInst* inst);

// True when this input pin of a sequential cell is in the support of its
// next_state function (clock, async controls and scan pins are not).
bool isRegisterDataPin(sta::dbSta* sta, odb::dbITerm* pin);

// The register's data input pin, or nullptr.  For an enable register this is
// the pin the enable passes through, not the enable itself.
odb::dbITerm* registerDataPin(sta::dbSta* sta, odb::dbInst* inst);

// True for a register (not a latch) clocked on the falling edge.
bool isNegedgeRegister(sta::dbSta* sta, odb::dbInst* inst);

// The net on the register's clock pin, or nullptr.
odb::dbNet* registerClockNet(odb::dbInst* inst);

// Register classes: a retiming step may move several registers across one
// gate only when they all belong to one class.
//
//   plain   next_state is exactly one input pin; the class is the clock net
//           alone, whatever the cell (chains are rebuilt with the base cell).
//   enable  next_state holds the state while one or more enable pins are
//           inactive and passes one data pin otherwise; the class is the cell,
//           its clock net, the net on every enable pin and the output pin the
//           loads read.  Such a register moves as-is: the rebuilt register is
//           the same cell on the same enable nets.
//   inverting  no control pins, but one cycle later the loads see the
//           complement of the data pin: next_state is the inverse of one
//           input, or the loads read the inverted output pin.  The class is
//           the cell, its clock net and that output pin.  Such registers
//           move as-is only across a self-dual gate (see isSelfDualGate):
//           there, complementing every input complements the output, so the
//           inversion commutes with the gate.  Across any other gate they
//           are unmapped first.
//   other   anything else (synchronous set/reset, loads on two output pins,
//           extra pins): never class-preserved; unmapped to a plain register
//           when it moves.
enum class RegisterKind
{
  plain,
  enable,
  inverting,
  other
};

struct RegisterClass
{
  RegisterKind kind = RegisterKind::other;
  odb::dbMaster* master = nullptr;  // enable, inverting
  odb::dbMTerm* data = nullptr;     // data pin
  odb::dbMTerm* output = nullptr;   // output pin the loads read
  odb::dbNet* clock = nullptr;
  // Enable pins and the nets on them, ordered by pin name.
  std::vector<std::pair<odb::dbMTerm*, odb::dbNet*>> controls;
};

RegisterClass registerClass(sta::dbSta* sta, odb::dbInst* inst);

// True when registers of these classes may move together across a gate.
// Two plain registers always can; two enable registers when the cell, clock,
// enable nets and output pin agree; two inverting registers when the cell,
// clock and output pin agree (and the gate is self-dual); an `other`
// register never.
bool sameRegisterClass(const RegisterClass& a, const RegisterClass& b);

// True when complementing every input of the gate complements each of its
// outputs (buffer, inverter, majority, 3-input XOR, ...), read from the
// liberty functions.  Inverting registers can cross such a gate unchanged.
bool isSelfDualGate(sta::dbSta* sta, odb::dbInst* inst);

}  // namespace ret
