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

const sta::Sequential* singleSequential(const sta::LibertyCell* cell);
const sta::Sequential* singleSequential(sta::dbSta* sta, odb::dbInst* inst);

std::vector<sta::LibertyCell*> libertyCells(sta::dbSta* sta);
std::vector<const sta::LibertyPort*> libertyPorts(const sta::LibertyCell* cell);
sta::LibertyCell* libertyCell(sta::dbSta* sta, odb::dbMaster* master);
// resizer: nullptr ignores set_dont_use.
bool isUsableCell(const sta::LibertyCell* cell, rsz::Resizer* resizer);
const sta::LibertyPort* libertyPort(sta::dbSta* sta, odb::dbITerm* pin);

bool isNegedge(const sta::Sequential* seq);
bool hasAsyncControl(const sta::Sequential* seq);
// nullptr unless the cell is clocked on its one pin marked as a clock.
odb::dbMTerm* clockMTerm(const sta::Sequential* seq, odb::dbMaster* master);

// nullopt when the output does not read the state.
std::optional<bool> outputInverted(const sta::FuncExpr* func,
                                   const sta::Sequential* seq);
std::optional<bool> outputInverted(sta::dbSta* sta,
                                   odb::dbITerm* pin,
                                   const sta::Sequential* seq);
std::optional<bool> constantOutput(const sta::LibertyPort* port);

bool isSignalInput(odb::dbITerm* pin);
bool drivesNet(odb::dbITerm* pin);
bool isRead(odb::dbNet* net);
bool hasSecondDriver(odb::dbNet* net);

// dbNet::getFirstOutput of each net, asked once: odb walks the net's pins to
// answer it.  Valid while the netlist does not change.
class NetDrivers
{
 public:
  odb::dbITerm* driverOf(odb::dbNet* net);
  void clear() { drivers_.clear(); }

 private:
  std::unordered_map<odb::dbNet*, odb::dbITerm*> drivers_;
};

void movePin(odb::dbITerm* pin,
             odb::dbNet* net,
             odb::dbModNet* mod_net = nullptr);
void connectPin(
    odb::dbITerm* pin,
    odb::dbNet* net,
    const std::unordered_map<odb::dbNet*, odb::dbModNet*>& mod_nets);

std::vector<odb::dbInst*> inIdOrder(
    const std::unordered_set<odb::dbInst*>& cells);

bool hasOneSupplyNetPerKind(odb::dbInst* inst);

struct SupplyPin
{
  odb::dbMTerm* pin;
  odb::dbNet* net;
};
using SupplyPins = std::vector<SupplyPin>;
SupplyPins supplyPins(odb::dbInst* inst);
bool sameSupply(const SupplyPins& a, const SupplyPins& b);
bool hasSupplyPinsOf(odb::dbMaster* master, odb::dbMaster* other);

sta::Instance* parentInstance(sta::dbSta* sta, odb::dbInst* inst);
// The supply pins are connected like those of `like`.
odb::dbInst* makeCell(sta::dbSta* sta,
                      sta::Instance* parent,
                      const char* base_name,
                      odb::dbMaster* master,
                      odb::dbInst* like);
odb::dbNet* makeNet(sta::dbSta* sta,
                    sta::Instance* parent,
                    const char* base_name);

// Cells and nets a command replaced: disconnected at once and deleted when
// the command ends, so undoing a trial brings them back with their SDC
// constraints.
class Discards
{
 public:
  void add(odb::dbInst* cell);
  void add(odb::dbNet* net, odb::dbModNet* mod_net);
  void keep();
  void undo();
  int registerCount() const;
  void destroy(sta::dbSta* sta);
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

bool isGrouped(odb::dbInst* inst);
// A don't-touch net or one with a non-default rule.
bool drivesKeptNet(odb::dbITerm* pin);
// Don't-touch, fixed, grouped, in a scan chain, or on a net that is kept.
bool isHeld(odb::dbInst* inst);

bool hasPinConstraint(sta::dbSta* sta, odb::dbInst* inst);
bool hasCellConstraint(sta::dbSta* sta, odb::dbInst* inst);
bool hasDisabledEdge(sta::dbSta* sta, odb::dbInst* inst);
std::vector<odb::dbInst*> portDelayReferenceCells(sta::dbSta* sta);

bool hasScanPin(odb::dbMaster* master);
bool isScanCell(sta::dbSta* sta, odb::dbInst* inst);

// The master's signal pins are the cell's liberty pins.
bool hasLibertyPins(const sta::LibertyCell* cell, odb::dbMaster* master);
bool isReplaceable(const sta::LibertyCell* cell,
                   const sta::Sequential* seq,
                   odb::dbMaster* master);

bool hasLiveAsyncControl(sta::dbSta* sta,
                         odb::dbInst* inst,
                         const sta::Sequential* seq);

// Why a sequential cell is never rebuilt.
enum class UntouchedKind
{
  kNone,
  kOther,
  kLatch,
  kAsync,
  kScan
};
UntouchedKind untouchedKind(sta::dbSta* sta, odb::dbInst* inst);

bool isDontTouchRegister(sta::dbSta* sta, odb::dbInst* inst);

bool hasUnreadInput(const sta::LibertyCell* cell, const sta::Sequential* seq);

struct PinConstant
{
  const sta::LibertyPort* port;
  bool high;
};
using PinConstants = std::vector<PinConstant>;
// nullopt when no pin values hold every control inactive.
std::optional<PinConstants> inactiveAsyncControls(const sta::LibertyCell* cell,
                                                  const sta::Sequential* seq);

void discardRegister(sta::dbSta* sta, odb::dbInst* inst, Discards& discards);

bool isRegisterDataPin(sta::dbSta* sta, odb::dbITerm* pin);
odb::dbITerm* registerDataPin(sta::dbSta* sta, odb::dbInst* inst);
bool isNegedgeRegister(sta::dbSta* sta, odb::dbInst* inst);
odb::dbITerm* registerClockPin(sta::dbSta* sta, odb::dbInst* inst);
odb::dbNet* registerClockNet(sta::dbSta* sta, odb::dbInst* inst);

enum class RegisterKind
{
  kPlain,      // rebuilt as the base cell
  kEnable,     // moves as the same cell on the same enable nets
  kInverting,  // moves as the same cell, across self-dual gates only
  kOther       // unmapped to a plain register before it moves
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
  odb::dbMTerm* output = nullptr;  // the one the loads read
  odb::dbNet* clock = nullptr;
  SupplyPins supply;
  std::vector<EnablePin> enables;
  bool negedge = false;
  bool scan = false;
};

RegisterClass registerClass(sta::dbSta* sta,
                            odb::dbInst* inst,
                            rsz::Resizer* resizer = nullptr);
// The same clock net and supply nets.
bool sameDomain(const RegisterClass& a, const RegisterClass& b);
bool sameRegisterClass(const RegisterClass& a, const RegisterClass& b);
bool keepsClass(const RegisterClass& cls);

bool isCombinationalGate(sta::dbSta* sta, odb::dbInst* inst);

// Complementing every input complements every output.
bool isSelfDualGate(sta::dbSta* sta, odb::dbInst* inst);

}  // namespace ret
