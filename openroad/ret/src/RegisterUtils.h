// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "odb/db.h"

namespace sta {
class dbSta;
class FuncExpr;
class Instance;
class LibertyCell;
class LibertyPort;
class Sequential;
}  // namespace sta
namespace rsz {
class Resizer;
}

namespace ret {

// The cell's sequential when it has exactly one, else nullptr.
const sta::Sequential* singleSequential(const sta::LibertyCell* cell);
const sta::Sequential* singleSequential(sta::dbSta* sta, odb::dbInst* inst);

std::vector<sta::LibertyCell*> libertyCells(sta::dbSta* sta);
sta::LibertyCell* libertyCell(sta::dbSta* sta, odb::dbMaster* master);
// A cell a new instance may be: not dont_use, and no isolation, level
// shifter, always-on, pad or macro cell.  resizer: its set_dont_use cells
// count; nullptr: only the liberty dont_use attribute does.
bool isUsableCell(const sta::LibertyCell* cell, rsz::Resizer* resizer);
const sta::LibertyPort* libertyPort(sta::dbSta* sta, odb::dbITerm* pin);

bool isNegedge(const sta::Sequential* seq);
// Clocked by one pin or its inverse, not by an expression such as CK & EN.
bool hasSimpleClock(const sta::Sequential* seq);

// Whether the output whose function is `func` reads the state inverted;
// nullopt when it does not read the state.
std::optional<bool> outputInverted(const sta::FuncExpr* func,
                                   const sta::Sequential* seq);

// An input pin on a net that is not a supply.
bool isSignalInput(odb::dbITerm* pin);
// The net has a pin besides its driver's, or a port.
bool isRead(odb::dbNet* net);
// More than one pin or input port drives the net (a tri-state bus).
bool hasSecondDriver(odb::dbNet* net);
// Takes the pin off its net and puts it on this one.
void movePin(odb::dbITerm* pin,
             odb::dbNet* net,
             odb::dbModNet* mod_net = nullptr);

void sortUnique(std::vector<int>& values);

// In id order, the order the block lists its cells in.
std::vector<odb::dbInst*> inIdOrder(
    const std::unordered_set<odb::dbInst*>& cells);

odb::dbInst* makeCell(sta::dbSta* sta,
                      sta::Instance* parent,
                      const char* base_name,
                      odb::dbMaster* master);
odb::dbNet* makeNet(sta::dbSta* sta,
                    sta::Instance* parent,
                    const char* base_name);

// The cells and nets a command replaced.  They are disconnected at once and
// deleted only when the command ends: STA drops the constraints that name a
// deleted cell or net, and undoing a trial, or the command after an error,
// would bring it back without them.
class Discards
{
 public:
  // Disconnects the cell's signal pins.
  void add(odb::dbInst* cell);
  // A net with nothing left on it; mod_net: its module net, if any.
  void add(odb::dbNet* net, odb::dbModNet* mod_net);
  // The trial that added the last ones is kept.
  void keep();
  // That trial is undone, which reconnected what it had not created: they
  // are forgotten.
  void undo();
  // The registers among the cells; they are still in the block.
  int registerCount() const;
  // Deletes everything, a module net once nothing is left on it.
  void destroy(sta::dbSta* sta);
  // Forgets everything: the command is undone.
  void clear();

 private:
  struct Net
  {
    odb::dbNet* net;
    odb::dbModNet* mod_net;
  };
  std::vector<odb::dbInst*> cells_;
  std::vector<Net> nets_;
  size_t kept_cells_ = 0;
  size_t kept_nets_ = 0;
};

void connectPin(
    odb::dbITerm* pin,
    odb::dbNet* net,
    const std::unordered_map<odb::dbNet*, odb::dbModNet*>& mod_nets);

// In a group or a region (a power domain, a placement region): a new cell
// would be in neither.
bool isGrouped(odb::dbInst* inst);
// A cell that must stay as it is: don't-touch, fixed, grouped, in a scan
// chain, or with a pin on a don't-touch net, supply nets included (odb
// refuses to delete such a cell).
bool isHeld(odb::dbInst* inst);

// An SDC constraint on one of the cell's pins or on a net it drives (case
// analysis, disable_timing, a clock latency, a net derate, ...).
bool hasPinConstraint(sta::dbSta* sta, odb::dbInst* inst);
// STA's answer for the cell itself: a constraint on it (a derate,
// disable_timing, operating conditions, ...) or an exception that lists it.
bool hasCellConstraint(sta::dbSta* sta, odb::dbInst* inst);
// A timing edge into, out of or inside the cell that set_disable_timing
// disables, on the cell, a pin, the edge or the library cell.
bool hasDisabledEdge(sta::dbSta* sta, odb::dbInst* inst);
// The cells with the pin a port delay is measured from (-reference_pin).
std::vector<odb::dbInst*> portDelayReferenceCells(sta::dbSta* sta);

// A cell with a scan pin in LEF or a liberty test_cell.
bool isScanCell(sta::dbSta* sta, odb::dbInst* inst);

// A scan cell, a cell with an input left open, one that is not a register
// clocked by one pin, or one with a live asynchronous control.
bool isDontTouchRegister(sta::dbSta* sta, odb::dbInst* inst);

// An asynchronous set or reset that is not tied to its inactive value.
bool hasLiveAsyncControl(sta::dbSta* sta,
                         odb::dbInst* inst,
                         const sta::Sequential* seq);

// `constants`: (pin, tied high) holding every asynchronous control inactive.
// False without such a control or such constants, or when the cell has an
// input that is not a control, a data pin, or the clock.
struct PinConstant
{
  const sta::LibertyPort* port;
  bool high;
};
using PinConstants = std::vector<PinConstant>;
bool inactiveAsyncControls(const sta::LibertyCell* cell,
                           const sta::Sequential* seq,
                           PinConstants& constants);

// Discards a register, its output nets with no load, and the tie cells that
// held only its asynchronous controls.  A tie cell stays when it is held or
// has an SDC constraint, and in a hierarchical netlist when it is in another
// module.
void discardRegister(sta::dbSta* sta, odb::dbInst* inst, Discards& discards);

bool isRegisterDataPin(sta::dbSta* sta, odb::dbITerm* pin);
odb::dbITerm* registerDataPin(sta::dbSta* sta, odb::dbInst* inst);
bool isNegedgeRegister(sta::dbSta* sta, odb::dbInst* inst);
odb::dbITerm* registerClockPin(odb::dbInst* inst);
odb::dbNet* registerClockNet(odb::dbInst* inst);

// Registers move together across a gate only when they are one class.
//   plain      next_state is one input pin; rebuilt as the base cell.
//   enable     holds the state while enable pins are inactive, else passes
//              one data pin; moves as the same cell on the same enable nets.
//   inverting  the loads see the complement of D one cycle later; moves as
//              the same cell across a self-dual gate only.
//   other      never keeps its class; unmapped to a plain register first.
//              An enable or inverting register whose cell has asynchronous
//              controls or is not usable is one of these.
enum class RegisterKind
{
  kPlain,
  kEnable,
  kInverting,
  kOther
};

struct EnablePin
{
  odb::dbMTerm* pin;
  odb::dbNet* net;
  bool operator==(const EnablePin&) const = default;
};

struct RegisterClass
{
  RegisterKind kind = RegisterKind::kOther;
  odb::dbMaster* master = nullptr;
  odb::dbMTerm* data = nullptr;
  odb::dbMTerm* output = nullptr;  // the output pin the loads read
  odb::dbNet* clock = nullptr;
  std::vector<EnablePin> enables;  // by pin name
  bool negedge = false;            // never moves
  bool scan = false;  // a scan cell or in a scan chain: never moves
};

// resizer: as for isUsableCell.
RegisterClass registerClass(sta::dbSta* sta,
                            odb::dbInst* inst,
                            rsz::Resizer* resizer = nullptr);
bool sameRegisterClass(const RegisterClass& a, const RegisterClass& b);
// An enable or inverting register keeps its own cell across a move.
bool keepsClass(const RegisterClass& cls);

// Every output is a liberty function of the inputs alone: no state, no
// tri-state output, and not a macro, a pad, an isolation cell, a level
// shifter or a cell without a liberty model.  Registers only cross such a
// gate.
bool isCombinationalGate(sta::dbSta* sta, odb::dbInst* inst);

// Complementing every input complements every output (buffer, inverter,
// majority, 3-input XOR, ...).
bool isSelfDualGate(sta::dbSta* sta, odb::dbInst* inst);

}  // namespace ret
