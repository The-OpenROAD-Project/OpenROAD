// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <utility>
#include <vector>

#include "odb/db.h"

namespace sta {
class dbSta;
class LibertyCell;
class Sequential;
}  // namespace sta

namespace ret {

// Register classification shared by the graph, the search and the apply
// step.

// The cell's sequential when it has exactly one, else nullptr.
const sta::Sequential* singleSequential(const sta::LibertyCell* cell);

bool isDontTouchRegister(sta::dbSta* sta, odb::dbInst* inst);
bool isRegisterDataPin(sta::dbSta* sta, odb::dbITerm* pin);
odb::dbITerm* registerDataPin(sta::dbSta* sta, odb::dbInst* inst);
bool isNegedgeRegister(sta::dbSta* sta, odb::dbInst* inst);
odb::dbNet* registerClockNet(odb::dbInst* inst);

// Registers move together across a gate only when they are one class.
//   plain      next_state is one input pin; rebuilt as the base cell.
//   enable     holds the state while enable pins are inactive, else passes
//              one data pin; moves as the same cell on the same enable nets.
//   inverting  the loads see the complement of D one cycle later; moves as
//              the same cell across a self-dual gate only.
//   other      never keeps its class; unmapped to a plain register first.
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
  odb::dbMaster* master = nullptr;
  odb::dbMTerm* data = nullptr;
  odb::dbMTerm* output = nullptr;  // the output pin the loads read
  odb::dbNet* clock = nullptr;
  std::vector<std::pair<odb::dbMTerm*, odb::dbNet*>> enables;  // by pin name
};

RegisterClass registerClass(sta::dbSta* sta, odb::dbInst* inst);
bool sameRegisterClass(const RegisterClass& a, const RegisterClass& b);

// Complementing every input complements every output (buffer, inverter,
// majority, 3-input XOR, ...).
bool isSelfDualGate(sta::dbSta* sta, odb::dbInst* inst);

}  // namespace ret
