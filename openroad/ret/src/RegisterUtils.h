// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "odb/db.h"

namespace sta {
class dbSta;
class FuncExpr;
class LibertyCell;
class LibertyPort;
class Sequential;
}  // namespace sta

namespace ret {

// The cell's sequential when it has exactly one, else nullptr.
const sta::Sequential* singleSequential(const sta::LibertyCell* cell);
// nullptr also without a liberty cell.
const sta::Sequential* singleSequential(sta::dbSta* sta, odb::dbInst* inst);

// Every cell of the loaded liberty libraries, in library order.
std::vector<sta::LibertyCell*> libertyCells(sta::dbSta* sta);

bool isNegedge(const sta::Sequential* seq);
// Clocked by one pin or its inverse, not by an expression such as CK & EN.
bool hasSimpleClock(const sta::Sequential* seq);

// Whether the output whose function is `func` reads the state inverted;
// nullopt when it does not read the state.
std::optional<bool> outputInverted(const sta::FuncExpr* func,
                                   const sta::Sequential* seq);

void connectPin(odb::dbITerm* pin,
                odb::dbNet* net,
                const std::unordered_map<odb::dbNet*, odb::dbModNet*>& mod_nets);

// A cell with a scan pin in LEF or a liberty test_cell.
bool isScanCell(sta::dbSta* sta, odb::dbInst* inst);

bool isDontTouchRegister(sta::dbSta* sta, odb::dbInst* inst);

// An asynchronous set or reset that is not tied to its inactive value.
bool hasLiveAsyncControl(sta::dbSta* sta,
                         odb::dbInst* inst,
                         const sta::Sequential* seq);

// `constants`: (pin, tied high) holding every asynchronous control inactive.
// False without such a control or such constants, or when the cell has an
// input that is not a control, a data pin, or the clock.
using PinConstants = std::vector<std::pair<const sta::LibertyPort*, bool>>;
bool inactiveAsyncControls(const sta::LibertyCell* cell,
                           const sta::Sequential* seq,
                           PinConstants& constants);

// Deletes a register, its output nets with no load, and the tie cells that
// held only its asynchronous controls.  In a hierarchical netlist a tie cell
// in another module stays.
void deleteRegister(sta::dbSta* sta, odb::dbInst* inst);

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
//              An enable or inverting register whose cell has asynchronous
//              controls is one of these.
enum class RegisterKind
{
  kPlain,
  kEnable,
  kInverting,
  kOther
};

struct RegisterClass
{
  RegisterKind kind = RegisterKind::kOther;
  odb::dbMaster* master = nullptr;
  odb::dbMTerm* data = nullptr;
  odb::dbMTerm* output = nullptr;  // the output pin the loads read
  odb::dbNet* clock = nullptr;
  std::vector<std::pair<odb::dbMTerm*, odb::dbNet*>> enables;  // by pin name
  bool negedge = false;  // never moves
};

RegisterClass registerClass(sta::dbSta* sta, odb::dbInst* inst);
bool sameRegisterClass(const RegisterClass& a, const RegisterClass& b);
// An enable or inverting register keeps its own cell across a move.
bool keepsClass(const RegisterClass& cls);

// Complementing every input complements every output (buffer, inverter,
// majority, 3-input XOR, ...).
bool isSelfDualGate(sta::dbSta* sta, odb::dbInst* inst);

}  // namespace ret
