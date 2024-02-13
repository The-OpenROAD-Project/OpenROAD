///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Precision Innovations Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "mbff.h"

#include <lemon/list_graph.h>
#include <lemon/network_simplex.h>
#include <omp.h>
#include <ortools/linear_solver/linear_solver.h>
#include <ortools/sat/cp_model.h>

#include <algorithm>
#include <random>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "graphics.h"
#include "odb/dbTransform.h"
#include "sta/ArcDelayCalc.hh"
#include "sta/ClkNetwork.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/ExceptionPath.hh"
#include "sta/FuncExpr.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/InputDrive.hh"
#include "sta/Liberty.hh"
#include "sta/Parasitics.hh"
#include "sta/PathAnalysisPt.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathRef.hh"
#include "sta/PathVertex.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/Sequential.hh"
#include "sta/TimingArc.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

namespace gpl {

struct Point
{
  float x;
  float y;
};

struct Tray
{
  Point pt;
  std::vector<Point> slots;
  std::vector<int> cand;
};

struct Flop
{
  Point pt;
  int idx;
  float prob;

  bool operator<(const Flop& a) const
  {
    return std::tie(prob, idx) < std::tie(a.prob, a.idx);
  }
};

// Get the function for a port.  If the port has no function then check
// the parent bus/bundle, if any.  This covers:
//    bundle (QN) {
//      members (QN0, QN1, QN2, QN3);
//      function : "IQN";
static sta::FuncExpr* getFunction(sta::LibertyPort* port)
{
  sta::FuncExpr* function = port->function();
  if (function) {
    return function;
  }

  // There is no way to go from a bit to the containing bus/bundle
  // so we have to walk all the ports of the cell.
  sta::LibertyCellPortIterator port_iter(port->libertyCell());
  while (port_iter.hasNext()) {
    sta::LibertyPort* next_port = port_iter.next();
    function = next_port->function();
    if (!function) {
      continue;
    }
    if (next_port->hasMembers()) {
      std::unique_ptr<sta::ConcretePortMemberIterator> mem_iter(
          next_port->memberIterator());
      while (mem_iter->hasNext()) {
        sta::ConcretePort* mem_port = mem_iter->next();
        if (mem_port == port) {
          return function;
        }
      }
    }
  }
  return nullptr;
}

float MBFF::GetDist(const Point& a, const Point& b)
{
  return (abs(a.x - b.x) + abs(a.y - b.y));
}

int MBFF::GetRows(int slot_cnt, std::vector<int> array_mask)
{
  const int idx = GetBitIdx(slot_cnt);

  std::set<float> vals;
  for (const auto& val : slot_to_tray_y_[array_mask][idx]) {
    vals.insert(val);
  }

  return static_cast<int>(vals.size());
}

int MBFF::GetBitCnt(int bit_idx)
{
  return (1 << bit_idx);
}

int MBFF::GetBitIdx(int bit_cnt)
{
  for (int i = 0; i < num_sizes_; i++) {
    if ((1 << i) == bit_cnt) {
      return i;
    }
  }
  log_->error(utl::GPL, 122, "{} is not in 2^[0,{}]", bit_cnt, num_sizes_);
}

bool MBFF::IsClockPin(odb::dbITerm* iterm)
{
  const bool yes = (iterm->getSigType() == odb::dbSigType::CLOCK);
  const sta::Pin* pin = network_->dbToSta(iterm);
  return yes || sta_->isClock(pin);
}

bool MBFF::ClockOn(odb::dbInst* inst)
{
  sta::Cell* cell = network_->dbToSta(inst->getMaster());
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  for (auto seq : lib_cell->sequentials()) {
    sta::FuncExpr* left = seq->clock()->left();
    sta::FuncExpr* right = seq->clock()->right();
    // !CLK
    if (left && !right) {
      return false;
    }
  }
  return true;
}

bool MBFF::IsDPin(odb::dbITerm* iterm)
{
  odb::dbInst* inst = iterm->getInst();
  sta::Cell* cell = network_->dbToSta(inst->getMaster());
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);

  // check that the iterm isn't a (re)set pin
  auto pin = network_->dbToSta(iterm);
  if (pin == nullptr) {
    return false;
  }
  auto lib_port = network_->libertyPort(pin);
  if (lib_port == nullptr) {
    return false;
  }

  for (auto seq : lib_cell->sequentials()) {
    if (seq->clear() && seq->clear()->hasPort(lib_port)) {
      return false;
    }
    if (seq->preset() && seq->preset()->hasPort(lib_port)) {
      return false;
    }
  }

  const bool exclude = (IsClockPin(iterm) || IsSupplyPin(iterm)
                        || IsScanIn(iterm) || IsScanEnable(iterm));
  const bool yes = (iterm->getIoType() == odb::dbIoType::INPUT);
  return (yes & !exclude);
}

int MBFF::GetNumD(odb::dbInst* inst)
{
  int cnt_d = 0;
  sta::Cell* cell = network_->dbToSta(inst->getMaster());
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);

  for (auto seq : lib_cell->sequentials()) {
    auto data = seq->data();
    sta::FuncExprPortIterator port_itr(data);
    while (port_itr.hasNext()) {
      sta::LibertyPort* port = port_itr.next();
      if (port != nullptr) {
        odb::dbMTerm* mterm = network_->staToDb(port);
        if (mterm == nullptr) {
          continue;
        }
        odb::dbITerm* iterm = inst->getITerm(mterm);
        if (iterm == nullptr) {
          continue;
        }
        cnt_d += !(IsScanIn(iterm) || IsScanEnable(iterm));
      } else {
        break;
      }
    }
  }
  return cnt_d;
}

bool MBFF::IsQPin(odb::dbITerm* iterm)
{
  const bool exclude = (IsClockPin(iterm) || IsSupplyPin(iterm));
  const bool yes = (iterm->getIoType() == odb::dbIoType::OUTPUT);
  return (yes & !exclude);
}

bool MBFF::IsInvertingQPin(odb::dbITerm* iterm)
{
  odb::dbInst* inst = iterm->getInst();
  sta::Cell* cell = network_->dbToSta(inst->getMaster());
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);

  std::set<std::string> invert;
  for (auto seq : lib_cell->sequentials()) {
    sta::LibertyPort* output_inv = seq->outputInv();
    invert.insert(output_inv->name());
  }

  sta::Pin* pin = network_->dbToSta(iterm);
  sta::LibertyPort* lib_port = network_->libertyPort(pin);
  sta::LibertyPort* func_port = getFunction(lib_port)->port();
  std::string func_name = func_port->name();
  if (func_port->hasMembers()) {
    sta::LibertyPortMemberIterator port_iter(func_port);
    while (port_iter.hasNext()) {
      sta::LibertyPort* mem_port = port_iter.next();
      func_name = mem_port->name();
      break;
    }
  }
  return (invert.count(func_name));
}

int MBFF::GetNumQ(odb::dbInst* inst)
{
  int num_q = 0;
  for (auto iterm : inst->getITerms()) {
    num_q += IsQPin(iterm);
  }
  return num_q;
}

bool MBFF::HasClear(odb::dbInst* inst)
{
  sta::Cell* cell = network_->dbToSta(inst->getMaster());
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  for (auto seq : lib_cell->sequentials()) {
    if (seq->clear()) {
      return true;
    }
  }
  return false;
}

bool MBFF::IsClearPin(odb::dbITerm* iterm)
{
  odb::dbInst* inst = iterm->getInst();
  sta::Cell* cell = network_->dbToSta(inst->getMaster());
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  sta::Pin* pin = network_->dbToSta(iterm);
  if (pin == nullptr) {
    return false;
  }
  sta::LibertyPort* lib_port = network_->libertyPort(pin);
  if (lib_port == nullptr) {
    return false;
  }
  for (auto seq : lib_cell->sequentials()) {
    if (seq->clear() && seq->clear()->hasPort(lib_port)) {
      return true;
    }
  }
  return false;
}

bool MBFF::HasPreset(odb::dbInst* inst)
{
  sta::Cell* cell = network_->dbToSta(inst->getMaster());
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  for (auto seq : lib_cell->sequentials()) {
    if (seq->preset()) {
      return true;
    }
  }
  return false;
}

bool MBFF::IsPresetPin(odb::dbITerm* iterm)
{
  odb::dbInst* inst = iterm->getInst();
  sta::Cell* cell = network_->dbToSta(inst->getMaster());
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  sta::Pin* pin = network_->dbToSta(iterm);
  if (pin == nullptr) {
    return false;
  }
  sta::LibertyPort* lib_port = network_->libertyPort(pin);
  if (lib_port == nullptr) {
    return false;
  }
  for (auto seq : lib_cell->sequentials()) {
    if (seq->preset() && seq->preset()->hasPort(lib_port)) {
      return true;
    }
  }
  return false;
}

bool MBFF::IsScanCell(odb::dbInst* inst)
{
  sta::Cell* cell = network_->dbToSta(inst->getMaster());
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  sta::TestCell* test_cell = lib_cell->testCell();
  if (test_cell != nullptr && test_cell->scanIn() != nullptr
      && test_cell->scanEnable() != nullptr) {
    return true;
  }
  return false;
}

bool MBFF::IsScanIn(odb::dbITerm* iterm)
{
  sta::Cell* cell = network_->dbToSta(iterm->getInst()->getMaster());
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  sta::TestCell* test_cell = lib_cell->testCell();
  if (test_cell != nullptr && test_cell->scanIn() != nullptr) {
    odb::dbMTerm* mterm = network_->staToDb(test_cell->scanIn());
    return (iterm->getInst()->getITerm(mterm) == iterm);
  }
  return false;
}

odb::dbITerm* MBFF::GetScanIn(odb::dbInst* inst)
{
  sta::Cell* cell = network_->dbToSta(inst->getMaster());
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  sta::TestCell* test_cell = lib_cell->testCell();
  if (test_cell != nullptr && test_cell->scanIn() != nullptr) {
    odb::dbMTerm* mterm = network_->staToDb(test_cell->scanIn());
    return inst->getITerm(mterm);
  }
  return nullptr;
}

bool MBFF::IsScanEnable(odb::dbITerm* iterm)
{
  sta::Cell* cell = network_->dbToSta(iterm->getInst()->getMaster());
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  sta::TestCell* test_cell = lib_cell->testCell();
  if (test_cell != nullptr && test_cell->scanEnable() != nullptr) {
    odb::dbMTerm* mterm = network_->staToDb(test_cell->scanEnable());
    return (iterm->getInst()->getITerm(mterm) == iterm);
  }
  return false;
}

odb::dbITerm* MBFF::GetScanEnable(odb::dbInst* inst)
{
  sta::Cell* cell = network_->dbToSta(inst->getMaster());
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  sta::TestCell* test_cell = lib_cell->testCell();
  if (test_cell != nullptr && test_cell->scanEnable() != nullptr) {
    odb::dbMTerm* mterm = network_->staToDb(test_cell->scanEnable());
    return inst->getITerm(mterm);
  }
  return nullptr;
}

bool MBFF::IsSupplyPin(odb::dbITerm* iterm)
{
  return iterm->getSigType().isSupply();
}

bool MBFF::IsValidFlop(odb::dbInst* FF)
{
  sta::Cell* cell = network_->dbToSta(FF->getMaster());
  if (cell == nullptr) {
    return false;
  }
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  if (lib_cell == nullptr) {
    return false;
  }
  if (!lib_cell->hasSequentials()) {
    return false;
  }

  int q = 0;
  int qn = 0;
  int scan = 0;
  int supply = 0;
  int preset = 0;
  int clear = 0;
  int clock = 0;

  for (auto iterm : FF->getITerms()) {
    q += (IsQPin(iterm) && !IsInvertingQPin(iterm));
    qn += (IsQPin(iterm) && IsInvertingQPin(iterm));
    scan += (IsScanIn(iterm) || IsScanEnable(iterm));
    supply += (IsSupplyPin(iterm));
    preset += (IsPresetPin(iterm));
    clear += (IsClearPin(iterm));
    clock += (IsClockPin(iterm));
  }
  // #D = 1
  return (GetNumD(FF) == 1
          && clock + q + qn + scan + supply + preset + clear + 1
                 == static_cast<int>(FF->getITerms().size()));
}

bool MBFF::IsValidTray(odb::dbInst* tray)
{
  sta::Cell* cell = network_->dbToSta(tray->getMaster());
  if (cell == nullptr) {
    return false;
  }
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  if (lib_cell == nullptr) {
    return false;
  }
  if (!lib_cell->hasSequentials()) {
    return false;
  }

  int q = 0;
  int qn = 0;
  int scan = 0;
  int supply = 0;
  int preset = 0;
  int clear = 0;
  int clock = 0;

  for (auto iterm : tray->getITerms()) {
    q += (IsQPin(iterm) && !IsInvertingQPin(iterm));
    qn += (IsQPin(iterm) && IsInvertingQPin(iterm));
    scan += (IsScanIn(iterm) || IsScanEnable(iterm));
    supply += (IsSupplyPin(iterm));
    preset += (IsPresetPin(iterm));
    clear += (IsClearPin(iterm));
    clock += (IsClockPin(iterm));
  }

  // #D = max(q, qn)
  return (std::max(q, qn) >= 2 && GetNumD(tray) == std::max(q, qn)
          && clock + q + qn + scan + supply + preset + clear + std::max(q, qn)
                 == static_cast<int>(tray->getITerms().size()));
}

PortName MBFF::PortType(sta::LibertyPort* lib_port, odb::dbInst* inst)
{
  odb::dbMTerm* mterm = network_->staToDb(lib_port);

  // physical pins
  if (mterm != nullptr) {
    odb::dbITerm* iterm = inst->getITerm(mterm);
    if (iterm != nullptr) {
      if (IsDPin(iterm)) {
        return d;
      }
      if (IsScanIn(iterm)) {
        return si;
      }
      if (IsScanEnable(iterm)) {
        return se;
      }
      if (IsPresetPin(iterm)) {
        return preset;
      }
      if (IsClearPin(iterm)) {
        return clear;
      }
      if (IsQPin(iterm)) {
        return (IsInvertingQPin(iterm) ? qn : q);
      }
      if (IsSupplyPin(iterm)) {
        if (iterm->getSigType() == odb::dbSigType::GROUND) {
          return vss;
        }
        return vdd;
      }
    }
  }

  sta::Cell* cell = network_->dbToSta(inst->getMaster());
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  for (auto seq : lib_cell->sequentials()) {
    // function
    if (sta::LibertyPort::equiv(lib_port, seq->output())) {
      return func;
    }
    // inverting function
    if (sta::LibertyPort::equiv(lib_port, seq->outputInv())) {
      return ifunc;
    }
  }

  log_->error(utl::GPL, 9032, "Could not recognize port {}", lib_port->name());
  return d;
}

// basically a copy-paste of FuncExpr::equiv
bool MBFF::IsSame(sta::FuncExpr* expr1,
                  odb::dbInst* inst1,
                  sta::FuncExpr* expr2,
                  odb::dbInst* inst2)
{
  if (expr1 == nullptr && expr2 == nullptr) {
    return true;
  }
  if (expr1 != nullptr && expr2 != nullptr && expr1->op() == expr2->op()) {
    if (expr1->op() == sta::FuncExpr::op_port) {
      return PortType(expr1->port(), inst1) == PortType(expr2->port(), inst2);
    }
    if (expr1->op() == sta::FuncExpr::op_not) {
      return IsSame(expr1->left(), inst1, expr2->left(), inst2);
    }
    return IsSame(expr1->left(), inst1, expr2->left(), inst2)
           && IsSame(expr1->right(), inst1, expr2->right(), inst2);
  }
  return false;
}

int MBFF::GetMatchingFunc(sta::FuncExpr* expr,
                          odb::dbInst* inst,
                          bool create_new)
{
  for (size_t i = 0; i < funcs_.size(); i++) {
    if (IsSame(funcs_[i].first, funcs_[i].second, expr, inst)) {
      return static_cast<int>(i);
    }
  }
  if (create_new) {
    funcs_.emplace_back(expr, inst);
    return (static_cast<int>(funcs_.size()) - 1);
  }
  return -1;
}

std::vector<int> MBFF::GetArrayMask(odb::dbInst* inst, bool isTray)
{
  std::vector<int> ret(7, 0);
  ret[0] = ClockOn(inst);
  ret[1] = HasClear(inst);
  ret[2] = HasPreset(inst);

  // check the existance of Q / QN pins
  for (auto iterm : inst->getITerms()) {
    if (IsQPin(iterm)) {
      ret[(IsInvertingQPin(iterm) ? 4 : 3)] = 1;
    }
  }

  sta::Cell* cell = network_->dbToSta(inst->getMaster());
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  for (auto seq : lib_cell->sequentials()) {
    ret[5] = GetMatchingFunc(seq->data(), inst, isTray);
    break;
  }

  ret[6] = IsScanCell(inst);

  return ret;
}

MBFF::DataToOutputsMap MBFF::GetPinMapping(odb::dbInst* tray)
{
  sta::Cell* cell = network_->dbToSta(tray->getMaster());
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  sta::LibertyCellPortIterator port_itr(lib_cell);

  std::vector<sta::LibertyPort*> d_pins;
  std::vector<sta::LibertyPort*> q_pins;
  std::vector<sta::LibertyPort*> qn_pins;

  // adds the input (D) pins
  for (auto seq : lib_cell->sequentials()) {
    auto data = seq->data();
    sta::FuncExprPortIterator next_state_itr(data);
    while (next_state_itr.hasNext()) {
      sta::LibertyPort* port = next_state_itr.next();
      if (!port->hasMembers()) {
        odb::dbMTerm* mterm = network_->staToDb(port);
        if (mterm == nullptr) {
          continue;
        }
        odb::dbITerm* iterm = tray->getITerm(mterm);
        if (iterm && IsDPin(iterm)) {
          d_pins.push_back(port);
        }
      }
    }
  }

  // all output pins are Q pins
  while (port_itr.hasNext()) {
    sta::LibertyPort* port = port_itr.next();
    if (port->isBus() || port->isBundle()) {
      continue;
    }
    if (port->isClock()) {
      continue;
    }
    if (port->direction()->isInput()) {
      continue;
    }
    if (port->direction()->isOutput()) {
      odb::dbMTerm* mterm = network_->staToDb(port);
      odb::dbITerm* iterm = tray->getITerm(mterm);
      if (IsInvertingQPin(iterm)) {
        qn_pins.push_back(port);
      } else {
        q_pins.push_back(port);
      }
    }
  }

  DataToOutputsMap ret;
  if (q_pins.size() && qn_pins.size()) {
    for (size_t i = 0; i < d_pins.size(); i++) {
      ret[d_pins[i]] = {q_pins[i], qn_pins[i]};
    }
  } else if (q_pins.size()) {
    for (size_t i = 0; i < d_pins.size(); i++) {
      ret[d_pins[i]] = {q_pins[i], nullptr};
    }
  } else {
    for (size_t i = 0; i < d_pins.size(); i++) {
      ret[d_pins[i]] = {nullptr, qn_pins[i]};
    }
  }

  return ret;
}

void MBFF::ModifyPinConnections(const std::vector<Flop>& flops,
                                const std::vector<Tray>& trays,
                                const std::vector<std::pair<int, int>>& mapping,
                                const std::vector<int> array_mask)
{
  const int num_flops = static_cast<int>(flops.size());
  std::vector<std::pair<int, int>> new_mapping(mapping);

  std::map<int, int> old_to_new_idx;
  std::vector<odb::dbInst*> tray_inst;
  for (int i = 0; i < num_flops; i++) {
    const int tray_idx = new_mapping[i].first;
    if (static_cast<int>(trays[tray_idx].slots.size()) == 1) {
      new_mapping[i].first = std::numeric_limits<int>::max();
      continue;
    }
    if (!old_to_new_idx.count(tray_idx)) {
      old_to_new_idx[tray_idx] = static_cast<int>(tray_inst.size());

      std::string new_name
          = "_tray_size"
            + std::to_string(static_cast<int>(trays[tray_idx].slots.size()))
            + "_" + std::to_string(unused_.back());
      unused_.pop_back();
      const int bit_idx
          = GetBitIdx(static_cast<int>(trays[tray_idx].slots.size()));
      auto new_tray = odb::dbInst::create(
          block_, best_master_[array_mask][bit_idx], new_name.c_str());
      Point tray_center = GetTrayCenter(array_mask, bit_idx);
      new_tray->setLocation(
          static_cast<int>(multiplier_
                           * (trays[tray_idx].pt.x - tray_center.x)),
          static_cast<int>(multiplier_
                           * (trays[tray_idx].pt.y - tray_center.y)));
      new_tray->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      tray_inst.push_back(new_tray);
    }
    new_mapping[i].first = old_to_new_idx[tray_idx];
  }

  odb::dbNet* clk_net = nullptr;
  for (int i = 0; i < num_flops; i++) {
    // single bit flop?
    if (new_mapping[i].first == std::numeric_limits<int>::max()) {
      continue;
    }

    const int tray_idx = new_mapping[i].first;
    const int tray_sz_idx = GetBitIdx(GetNumD(tray_inst[tray_idx]));
    const int slot_idx = new_mapping[i].second;

    // find the new port names
    sta::LibertyPort* d_pin = nullptr;
    sta::LibertyPort* q_pin = nullptr;
    sta::LibertyPort* qn_pin = nullptr;

    int idx = 0;
    for (const auto& pins : pin_mappings_[array_mask][tray_sz_idx]) {
      if (idx == slot_idx) {
        d_pin = pins.first;
        q_pin = pins.second.q;
        qn_pin = pins.second.qn;
        break;
      }
      idx++;
    }

    odb::dbITerm* scan_in = nullptr;
    odb::dbITerm* scan_enable = nullptr;
    odb::dbITerm* preset = nullptr;
    odb::dbITerm* clear = nullptr;
    odb::dbITerm* ground = nullptr;
    odb::dbITerm* power = nullptr;

    for (auto iterm : tray_inst[tray_idx]->getITerms()) {
      if (IsPresetPin(iterm)) {
        preset = iterm;
      }
      if (IsClearPin(iterm)) {
        clear = iterm;
      }
      if (IsScanIn(iterm)) {
        scan_in = iterm;
      }
      if (IsScanEnable(iterm)) {
        scan_enable = iterm;
      }
      if (IsSupplyPin(iterm)) {
        if (iterm->getSigType() == odb::dbSigType::GROUND) {
          ground = iterm;
        } else {
          power = iterm;
        }
      }
    }

    // disconnect / reconnect iterms
    for (auto iterm : insts_[flops[i].idx]->getITerms()) {
      auto net = iterm->getNet();
      while (net) {
        iterm->disconnect();

        // standard pins
        if (IsDPin(iterm)) {
          tray_inst[tray_idx]->findITerm(d_pin->name())->connect(net);
        }
        if (IsQPin(iterm)) {
          if (IsInvertingQPin(iterm)) {
            tray_inst[tray_idx]->findITerm(qn_pin->name())->connect(net);
          } else {
            tray_inst[tray_idx]->findITerm(q_pin->name())->connect(net);
          }
        }
        if (IsSupplyPin(iterm)) {
          if (iterm->getSigType() == odb::dbSigType::GROUND) {
            ground->connect(net);
          } else {
            power->connect(net);
          }
        }
        if (IsClockPin(iterm)) {
          // reconnect pins later
          clk_net = net;
        }

        // scan pins
        if (IsScanIn(iterm)) {
          scan_in->connect(net);
        }
        if (IsScanEnable(iterm)) {
          scan_enable->connect(net);
        }

        // preset/clear pins
        if (IsPresetPin(iterm)) {
          preset->connect(net);
        }
        if (IsClearPin(iterm)) {
          clear->connect(net);
        }

        net = iterm->getNet();
      }
    }
  }

  // all FFs in flops have the same block clock
  std::vector<bool> isConnected(static_cast<int>(tray_inst.size()));
  for (int i = 0; i < num_flops; i++) {
    if (new_mapping[i].first != std::numeric_limits<int>::max()) {
      if (!isConnected[new_mapping[i].first] && clk_net != nullptr) {
        for (auto iterm : tray_inst[new_mapping[i].first]->getITerms()) {
          if (IsClockPin(iterm)) {
            iterm->connect(clk_net);
          }
        }
        isConnected[new_mapping[i].first] = true;
      }
      odb::dbInst::destroy(insts_[flops[i].idx]);
    }
  }
}

float MBFF::RunLP(const std::vector<Flop>& flops,
                  std::vector<Tray>& trays,
                  const std::vector<std::pair<int, int>>& clusters)
{
  const int num_flops = static_cast<int>(flops.size());
  const int num_trays = static_cast<int>(trays.size());

  std::unique_ptr<operations_research::MPSolver> solver(
      operations_research::MPSolver::CreateSolver("GLOP"));
  const float inf = solver->infinity();

  std::vector<operations_research::MPVariable*> tray_x(num_trays);
  std::vector<operations_research::MPVariable*> tray_y(num_trays);

  for (int i = 0; i < num_trays; i++) {
    tray_x[i] = solver->MakeNumVar(0, inf, "");
    tray_y[i] = solver->MakeNumVar(0, inf, "");
  }

  // displacement from flop to tray slot
  std::vector<operations_research::MPVariable*> disp_x(num_flops);
  std::vector<operations_research::MPVariable*> disp_y(num_flops);

  for (int i = 0; i < num_flops; i++) {
    disp_x[i] = solver->MakeNumVar(0, inf, "");
    disp_y[i] = solver->MakeNumVar(0, inf, "");

    const int tray_idx = clusters[i].first;
    const int slot_idx = clusters[i].second;

    const Point& flop = flops[i].pt;
    const Point& tray = trays[tray_idx].pt;
    const Point& slot = trays[tray_idx].slots[slot_idx];

    const float shift_x = slot.x - tray.x;
    const float shift_y = slot.y - tray.y;

    operations_research::MPConstraint* c1
        = solver->MakeRowConstraint(shift_x - flop.x, inf, "");
    c1->SetCoefficient(disp_x[i], 1);
    c1->SetCoefficient(tray_x[tray_idx], -1);

    operations_research::MPConstraint* c2
        = solver->MakeRowConstraint(flop.x - shift_x, inf, "");
    c2->SetCoefficient(disp_x[i], 1);
    c2->SetCoefficient(tray_x[tray_idx], 1);

    operations_research::MPConstraint* c3
        = solver->MakeRowConstraint(shift_y - flop.y, inf, "");
    c3->SetCoefficient(disp_y[i], 1);
    c3->SetCoefficient(tray_y[tray_idx], -1);

    operations_research::MPConstraint* c4
        = solver->MakeRowConstraint(flop.y - shift_y, inf, "");
    c4->SetCoefficient(disp_y[i], 1);
    c4->SetCoefficient(tray_y[tray_idx], 1);
  }

  operations_research::MPObjective* objective = solver->MutableObjective();
  std::vector<int> coeff(num_flops, 1);
  for (int i = 0; i < num_flops; i++) {
    if (path_points_.count(flops[i].idx)) {
      coeff[i] = 1 + occs_[flops[i].idx];
    }
    objective->SetCoefficient(disp_x[i], coeff[i]);
    objective->SetCoefficient(disp_y[i], coeff[i]);
  }
  objective->SetMinimization();
  solver->Solve();

  float tot_disp = 0;
  for (int i = 0; i < num_trays; i++) {
    const float new_x = tray_x[i]->solution_value();
    const float new_y = tray_y[i]->solution_value();

    Tray new_tray;
    new_tray.pt = Point{new_x, new_y};

    tot_disp += GetDist(trays[i].pt, new_tray.pt);
    trays[i].pt = new_tray.pt;
  }

  return tot_disp;
}

double MBFF::RunILP(const std::vector<Flop>& flops,
                    const std::vector<Tray>& trays,
                    std::vector<std::pair<int, int>>& final_flop_to_slot,
                    float alpha,
                    std::vector<int> array_mask)
{
  const int num_flops = static_cast<int>(flops.size());
  const int num_trays = static_cast<int>(trays.size());

  /*
  NOTE: CP-SAT constraints only work with INTEGERS
  so, all coefficients (shift_x and shift_y) are multiplied by 100
  */
  operations_research::sat::CpModelBuilder cp_model;
  int inf = std::numeric_limits<int>::max();

  // cand_tray[i] = tray indices that have a slot which contains flop i
  std::vector<int> cand_tray[num_flops];

  // cand_slot[i] = slot indices that are mapped to flop i
  std::vector<int> cand_slot[num_flops];

  for (int i = 0; i < num_trays; i++) {
    for (int j = 0; j < static_cast<int>(trays[i].slots.size()); j++) {
      if (trays[i].cand[j] >= 0) {
        cand_tray[trays[i].cand[j]].push_back(i);
        cand_slot[trays[i].cand[j]].push_back(j);
      }
    }
  }

  // has a flop been mapped to a slot?
  std::vector<operations_research::sat::BoolVar> mapped[num_flops];

  // displacement from flop to slot
  std::vector<operations_research::sat::IntVar> disp_x[num_flops];
  std::vector<operations_research::sat::IntVar> disp_y[num_flops];

  for (int i = 0; i < num_flops; i++) {
    for (size_t j = 0; j < cand_tray[i].size(); j++) {
      disp_x[i].push_back(
          cp_model.NewIntVar(operations_research::Domain(0, inf)));
      disp_y[i].push_back(
          cp_model.NewIntVar(operations_research::Domain(0, inf)));
      mapped[i].push_back(cp_model.NewBoolVar());
    }
  }

  // add constraints for displacements
  float max_dist = 0;
  for (int i = 0; i < num_flops; i++) {
    for (size_t j = 0; j < cand_tray[i].size(); j++) {
      const int tray_idx = cand_tray[i][j];
      const int slot_idx = cand_slot[i][j];

      const float shift_x = trays[tray_idx].slots[slot_idx].x - flops[i].pt.x;
      const float shift_y = trays[tray_idx].slots[slot_idx].y - flops[i].pt.y;

      max_dist = std::max(
          max_dist, std::max(shift_x, -shift_x) + std::max(shift_y, -shift_y));

      // absolute value constraints for x
      cp_model.AddLessOrEqual(
          0, disp_x[i][j] - int(multiplier_ * shift_x) * mapped[i][j]);
      cp_model.AddLessOrEqual(
          0, disp_x[i][j] + int(multiplier_ * shift_x) * mapped[i][j]);

      // absolute value constraints for y
      cp_model.AddLessOrEqual(
          0, disp_y[i][j] - int(multiplier_ * shift_y) * mapped[i][j]);
      cp_model.AddLessOrEqual(
          0, disp_y[i][j] + int(multiplier_ * shift_y) * mapped[i][j]);
    }
  }

  for (int i = 0; i < num_flops; i++) {
    for (size_t j = 0; j < cand_tray[i].size(); j++) {
      cp_model.AddLessOrEqual(disp_x[i][j] + disp_y[i][j],
                              int(multiplier_ * max_dist));
    }
  }

  std::vector<int> coeff(num_flops, 1);
  for (int i = 0; i < num_flops; i++) {
    if (path_points_.count(flops[i].idx)) {
      coeff[i] = 2;
    }
  }

  // check that each flop is matched to a single slot
  for (int i = 0; i < num_flops; i++) {
    operations_research::sat::LinearExpr mapped_flop;
    for (size_t j = 0; j < cand_tray[i].size(); j++) {
      mapped_flop += mapped[i][j];
    }
    cp_model.AddLessOrEqual(1, mapped_flop);
    cp_model.AddLessOrEqual(mapped_flop, 1);
  }

  std::vector<operations_research::sat::BoolVar> tray_used(num_trays);
  for (int i = 0; i < num_trays; i++) {
    tray_used[i] = cp_model.NewBoolVar();
  }

  for (int i = 0; i < num_trays; i++) {
    operations_research::sat::LinearExpr slots_used;

    std::vector<int> flop_ind;
    for (size_t j = 0; j < trays[i].slots.size(); j++) {
      if (trays[i].cand[j] >= 0) {
        flop_ind.push_back(trays[i].cand[j]);
      }
    }

    for (int index : flop_ind) {
      // TODO: SWITCH TO BINARY SEARCH
      int tray_idx = 0;
      for (size_t k = 0; k < cand_tray[index].size(); k++) {
        if (cand_tray[index][k] == i) {
          tray_idx = k;
        }
      }

      cp_model.AddLessOrEqual(mapped[index][tray_idx], tray_used[i]);
      slots_used += mapped[index][tray_idx];
    }

    // check that tray_used <= slots_used
    cp_model.AddLessOrEqual(tray_used[i], slots_used);
  }

  // calculate the cost of each tray

  std::vector<float> tray_cost(num_trays);
  for (int i = 0; i < num_trays; i++) {
    int bit_idx = 0;
    for (int j = 0; j < num_sizes_; j++) {
      if (best_master_[array_mask][j] != nullptr) {
        if (GetBitCnt(j) == static_cast<int>(trays[i].slots.size())) {
          bit_idx = j;
        }
      }
    }
    if (GetBitCnt(bit_idx) == 1) {
      tray_cost[i] = 1.00;
    } else {
      tray_cost[i] = (GetBitCnt(bit_idx) * norm_power_[bit_idx]);
    }
  }

  /*
    DoubleLinearExpr supports float/double coefficients
    can only be used for the objective function
  */

  operations_research::sat::DoubleLinearExpr obj;

  // add the sum of all distances
  for (int i = 0; i < num_flops; i++) {
    for (size_t j = 0; j < cand_tray[i].size(); j++) {
      obj.AddTerm(disp_x[i][j], coeff[i] * (1 / multiplier_));
      obj.AddTerm(disp_y[i][j], coeff[i] * (1 / multiplier_));
    }
  }

  // add the tray usage constraints
  for (int i = 0; i < num_trays; i++) {
    obj.AddTerm(tray_used[i], alpha * tray_cost[i]);
  }

  cp_model.Minimize(obj);

  operations_research::sat::Model model;
  operations_research::sat::SatParameters parameters;
  model.Add(NewSatParameters(parameters));
  operations_research::sat::CpSolverResponse response
      = operations_research::sat::SolveCpModel(cp_model.Build(), &model);

  if (response.status() == operations_research::sat::CpSolverStatus::FEASIBLE
      || response.status()
             == operations_research::sat::CpSolverStatus::OPTIMAL) {
    double ret = static_cast<double>(response.objective_value());
    std::set<std::pair<int, int>> trays_used;
    // update slot_disp_ vectors
    for (int i = 0; i < num_flops; i++) {
      for (size_t j = 0; j < cand_tray[i].size(); j++) {
        if (operations_research::sat::SolutionBooleanValue(response,
                                                           mapped[i][j])
            == 1) {
          slot_disp_x_[flops[i].idx]
              = trays[cand_tray[i][j]].slots[cand_slot[i][j]].x - flops[i].pt.x;

          slot_disp_y_[flops[i].idx]
              = trays[cand_tray[i][j]].slots[cand_slot[i][j]].y - flops[i].pt.y;

          if (path_points_.count(flops[i].idx)) {
            ret -= (coeff[i] - 1)
                   * std::max(slot_disp_x_[flops[i].idx],
                              -slot_disp_x_[flops[i].idx]);
            ret -= (coeff[i] - 1)
                   * std::max(slot_disp_y_[flops[i].idx],
                              -slot_disp_y_[flops[i].idx]);
          }

          final_flop_to_slot[i] = {cand_tray[i][j], cand_slot[i][j]};
          trays_used.insert(
              {cand_tray[i][j],
               static_cast<int>(trays[cand_tray[i][j]].slots.size())});
        }
      }
    }

    for (const auto& sizes : trays_used) {
      tray_sizes_used_[sizes.second]++;
    }

    return ret;
  }
  return 0;
}

void MBFF::GetSlots(const Point& tray,
                    const int rows,
                    const int cols,
                    std::vector<Point>& slots,
                    const std::vector<int> array_mask)
{
  slots.clear();
  const int idx = GetBitIdx(rows * cols);
  Point center = GetTrayCenter(array_mask, idx);
  for (int i = 0; i < rows * cols; i++) {
    const Point pt
        = Point{tray.x + slot_to_tray_x_[array_mask][idx][i] - center.x,
                tray.y + slot_to_tray_y_[array_mask][idx][i] - center.y};
    slots.push_back(pt);
  }
}

Flop MBFF::GetNewFlop(const std::vector<Flop>& prob_dist, const float tot_dist)
{
  const float rand_num = (float) (std::rand() % 101);
  float cum_sum = 0;
  Flop new_flop;
  for (size_t i = 0; i < prob_dist.size(); i++) {
    cum_sum += prob_dist[i].prob;
    new_flop = prob_dist[i];
    if (cum_sum * 100.0 >= rand_num * tot_dist) {
      break;
    }
  }
  return new_flop;
}

void MBFF::GetStartTrays(std::vector<Flop> flops,
                         const int num_trays,
                         const float AR,
                         std::vector<Tray>& trays)
{
  const int num_flops = static_cast<int>(flops.size());

  /* pick a random flop */
  const int rand_idx = std::rand() % (num_flops);
  Tray tray_zero;
  tray_zero.pt = flops[rand_idx].pt;

  std::set<int> used_flops;
  used_flops.insert(rand_idx);
  trays.push_back(tray_zero);

  float tot_dist = 0;
  for (int i = 0; i < num_flops; i++) {
    const float contr = GetDist(flops[i].pt, tray_zero.pt) / AR;
    flops[i].prob = contr;
    tot_dist += contr;
  }

  while (static_cast<int>(trays.size()) < num_trays) {
    std::vector<Flop> prob_dist;
    for (int i = 0; i < num_flops; i++) {
      if (!used_flops.count(flops[i].idx)) {
        prob_dist.push_back(flops[i]);
      }
    }

    std::sort(prob_dist.begin(), prob_dist.end());

    const Flop new_flop = GetNewFlop(prob_dist, tot_dist);
    used_flops.insert(new_flop.idx);

    Tray new_tray;
    new_tray.pt = new_flop.pt;
    trays.push_back(new_tray);

    for (int i = 0; i < num_flops; i++) {
      const float new_contr = GetDist(flops[i].pt, new_tray.pt) / AR;
      flops[i].prob += new_contr;
      tot_dist += new_contr;
    }
  }
}

Tray MBFF::GetOneBit(const Point& pt)
{
  Tray tray;
  tray.pt = Point{pt.x, pt.y};
  tray.slots.push_back(pt);

  return tray;
}

void MBFF::MinCostFlow(const std::vector<Flop>& flops,
                       std::vector<Tray>& trays,
                       const int sz,
                       std::vector<std::pair<int, int>>& clusters)
{
  const int num_flops = static_cast<int>(flops.size());
  const int num_trays = static_cast<int>(trays.size());

  lemon::ListDigraph graph;
  std::vector<lemon::ListDigraph::Node> nodes;
  std::vector<lemon::ListDigraph::Arc> edges;
  std::vector<int> cur_costs, cur_caps;

  // add edges from source to flop
  lemon::ListDigraph::Node src = graph.addNode(), sink = graph.addNode();
  for (int i = 0; i < num_flops; i++) {
    lemon::ListDigraph::Node flop_node = graph.addNode();
    nodes.push_back(flop_node);

    lemon::ListDigraph::Arc src_to_flop = graph.addArc(src, flop_node);
    edges.push_back(src_to_flop);
    cur_costs.push_back(0), cur_caps.push_back(1);
  }

  std::vector<std::pair<int, int>> slot_to_tray;

  for (int i = 0; i < num_trays; i++) {
    std::vector<Point> tray_slots = trays[i].slots;

    for (size_t j = 0; j < tray_slots.size(); j++) {
      lemon::ListDigraph::Node slot_node = graph.addNode();
      nodes.push_back(slot_node);

      // add edges from flop to slot
      for (int k = 0; k < num_flops; k++) {
        lemon::ListDigraph::Arc flop_to_slot
            = graph.addArc(nodes[k], slot_node);
        edges.push_back(flop_to_slot);

        int edge_cost = (100 * GetDist(flops[k].pt, tray_slots[j]));
        cur_costs.push_back(edge_cost), cur_caps.push_back(1);
      }

      // add edges from slot to sink
      lemon::ListDigraph::Arc slot_to_sink = graph.addArc(slot_node, sink);
      edges.push_back(slot_to_sink);
      cur_costs.push_back(0), cur_caps.push_back(1);

      slot_to_tray.emplace_back(i, j);
    }
  }

  // run min-cost flow
  lemon::ListDigraph::ArcMap<int> costs(graph), caps(graph), flow(graph);
  lemon::ListDigraph::NodeMap<int> labels(graph);
  lemon::NetworkSimplex<lemon::ListDigraph, int, int> new_graph(graph);

  for (size_t i = 0; i < edges.size(); i++) {
    costs[edges[i]] = cur_costs[i], caps[edges[i]] = cur_caps[i];
  }

  labels[src] = -1;
  for (size_t i = 0; i < nodes.size(); i++) {
    labels[nodes[i]] = i;
  }
  labels[sink] = static_cast<int>(nodes.size());

  new_graph.costMap(costs);
  new_graph.upperMap(caps);
  new_graph.stSupply(src, sink, num_flops);
  new_graph.run();
  new_graph.flowMap(flow);

  // get, and save, the clustering solution
  clusters.clear();
  clusters.resize(num_flops);
  for (lemon::ListDigraph::ArcIt itr(graph); itr != lemon::INVALID; ++itr) {
    const int u = labels[graph.source(itr)];
    int v = labels[graph.target(itr)];
    if (flow[itr] != 0 && u < num_flops && v >= num_flops) {
      v -= num_flops;
      const int tray_idx = slot_to_tray[v].first;
      const int slot_idx = slot_to_tray[v].second;
      clusters[u] = {tray_idx, slot_idx};
      trays[tray_idx].cand[slot_idx] = u;
    }
  }
}

float MBFF::GetSilh(const std::vector<Flop>& flops,
                    const std::vector<Tray>& trays,
                    const std::vector<std::pair<int, int>>& clusters)
{
  const int num_flops = static_cast<int>(flops.size());
  const int num_trays = static_cast<int>(trays.size());

  float tot = 0;
  for (int i = 0; i < num_flops; i++) {
    float min_num = std::numeric_limits<float>::max();
    float max_den = GetDist(flops[i].pt,
                            trays[clusters[i].first].slots[clusters[i].second]);
    for (int j = 0; j < num_trays; j++) {
      if (j != clusters[i].first) {
        max_den = std::max(
            max_den, GetDist(flops[i].pt, trays[j].slots[clusters[i].second]));
        for (size_t k = 0; k < trays[j].slots.size(); k++) {
          min_num = std::min(min_num, GetDist(flops[i].pt, trays[j].slots[k]));
        }
      }
    }

    tot += (min_num / max_den);
  }

  return tot;
}

void MBFF::RunCapacitatedKMeans(const std::vector<Flop>& flops,
                                std::vector<Tray>& trays,
                                const int sz,
                                const int iter,
                                std::vector<std::pair<int, int>>& cluster,
                                const std::vector<int> array_mask)
{
  cluster.clear();
  const int num_flops = static_cast<int>(flops.size());
  const int rows = GetRows(sz, array_mask);
  const int cols = sz / rows;
  const int num_trays = (num_flops + (sz - 1)) / sz;

  float delta = 0;
  for (int i = 0; i < iter; i++) {
    MinCostFlow(flops, trays, sz, cluster);
    delta = RunLP(flops, trays, cluster);

    for (int j = 0; j < num_trays; j++) {
      GetSlots(trays[j].pt, rows, cols, trays[j].slots, array_mask);
      for (int k = 0; k < rows * cols; k++) {
        trays[j].cand[k] = -1;
      }
    }

    if (delta < 0.5) {
      break;
    }
  }

  MinCostFlow(flops, trays, sz, cluster);
}

void MBFF::RunMultistart(
    std::vector<std::vector<Tray>>& trays,
    const std::vector<Flop>& flops,
    std::vector<std::vector<std::vector<Tray>>>& start_trays,
    const std::vector<int> array_mask)
{
  const int num_flops = static_cast<int>(flops.size());
  trays.resize(num_sizes_);
  for (int i = 0; i < num_sizes_; i++) {
    const int num_trays = (num_flops + (GetBitCnt(i) - 1)) / GetBitCnt(i);
    trays[i].resize(num_trays);
  }

  // add 1-bit trays
  for (int i = 0; i < num_flops; i++) {
    Tray one_bit = GetOneBit(flops[i].pt);
    one_bit.cand.reserve(1);
    one_bit.cand.emplace_back(i);
    trays[0][i] = one_bit;
  }

  std::vector<float> res[num_sizes_];
  std::vector<std::pair<int, int>> ind;

  for (int i = 1; i < num_sizes_; i++) {
    if (best_master_[array_mask][i] != nullptr) {
      for (int j = 0; j < 5; j++) {
        ind.emplace_back(i, j);
      }
      res[i].resize(5);
    }
  }

  for (int i = 1; i < num_sizes_; i++) {
    if (best_master_[array_mask][i] != nullptr) {
      for (int j = 0; j < 5; j++) {
        const int rows = GetRows(GetBitCnt(i), array_mask);
        const int cols = GetBitCnt(i) / rows;
        const int num_trays = (num_flops + (GetBitCnt(i) - 1)) / GetBitCnt(i);

        for (int k = 0; k < num_trays; k++) {
          GetSlots(start_trays[i][j][k].pt,
                   rows,
                   cols,
                   start_trays[i][j][k].slots,
                   array_mask);
          start_trays[i][j][k].cand.reserve(rows * cols);
          for (int idx = 0; idx < rows * cols; idx++) {
            start_trays[i][j][k].cand.emplace_back(-1);
          }
        }
      }
    }
  }

  for (const auto& [bit_idx, tray_idx] : ind) {
    const int rows = GetRows(GetBitCnt(bit_idx), array_mask);
    const int cols = GetBitCnt(bit_idx) / rows;

    std::vector<std::pair<int, int>> tmp_cluster;

    RunCapacitatedKMeans(flops,
                         start_trays[bit_idx][tray_idx],
                         rows * cols,
                         8,
                         tmp_cluster,
                         array_mask);

    res[bit_idx][tray_idx]
        = GetSilh(flops, start_trays[bit_idx][tray_idx], tmp_cluster);
  }

  for (int i = 1; i < num_sizes_; i++) {
    if (best_master_[array_mask][i] != nullptr) {
      int opt_idx = 0;
      float opt_val = -1;
      for (int j = 0; j < 5; j++) {
        if (res[i][j] > opt_val) {
          opt_val = res[i][j];
          opt_idx = j;
        }
      }
      trays[i] = start_trays[i][opt_idx];
    }
  }
}

// standard K-means++ implementation
void MBFF::KMeans(const std::vector<Flop>& flops,
                  std::vector<std::vector<Flop>>& clusters)
{
  const int num_flops = static_cast<int>(flops.size());

  // choose initial center
  const int seed = std::rand() % num_flops;
  std::set<int> chosen({seed});

  std::vector<Flop> centers;
  centers.push_back(flops[seed]);

  std::vector<float> d(num_flops);
  for (int i = 0; i < num_flops; i++) {
    d[i] = GetDist(flops[i].pt, flops[seed].pt);
  }

  // choose remaining K-1 centers
  while (static_cast<int>(chosen.size()) < knn_) {
    float tot_sum = 0;

    for (int i = 0; i < num_flops; i++) {
      if (!chosen.count(i)) {
        for (int j : chosen) {
          d[i] = std::min(d[i], GetDist(flops[i].pt, flops[j].pt));
        }
        tot_sum += (float(d[i]) * float(d[i]));
      }
    }

    const int rnd = std::rand() % (int(tot_sum * 100));
    const float prob = rnd / 100.0;

    float cum_sum = 0;
    for (int i = 0; i < num_flops; i++) {
      if (!chosen.count(i)) {
        cum_sum += (float(d[i]) * float(d[i]));
        if (cum_sum >= prob) {
          chosen.insert(i);
          centers.push_back(flops[i]);
          break;
        }
      }
    }
  }

  clusters.resize(knn_);
  float prev = -1;
  while (true) {
    for (int i = 0; i < knn_; i++) {
      clusters[i].clear();
    }

    // remap flops to clusters
    for (int i = 0; i < num_flops; i++) {
      float min_cost = std::numeric_limits<float>::max();
      int idx = 0;

      for (int j = 0; j < knn_; j++) {
        if (GetDist(flops[i].pt, centers[j].pt) < min_cost) {
          min_cost = GetDist(flops[i].pt, centers[j].pt);
          idx = j;
        }
      }

      clusters[idx].push_back(flops[i]);
    }

    // find new center locations
    for (int i = 0; i < knn_; i++) {
      const int cur_sz = static_cast<int>(clusters[i].size());
      float cX = 0;
      float cY = 0;

      for (const Flop& flop : clusters[i]) {
        cX += flop.pt.x;
        cY += flop.pt.y;
      }

      const float new_x = cX / float(cur_sz);
      const float new_y = cY / float(cur_sz);
      centers[i].pt = Point{new_x, new_y};
    }

    // get total displacement
    float tot_disp = 0;
    for (int i = 0; i < knn_; i++) {
      for (size_t j = 0; j < clusters[i].size(); j++) {
        tot_disp += GetDist(centers[i].pt, clusters[i][j].pt);
      }
    }

    if (tot_disp == prev) {
      break;
    }
    prev = tot_disp;
  }

  for (int i = 0; i < knn_; i++) {
    clusters[i].push_back(centers[i]);
  }
}

void MBFF::KMeansDecomp(const std::vector<Flop>& flops,
                        const int max_sz,
                        std::vector<std::vector<Flop>>& pointsets)
{
  const int num_flops = static_cast<int>(flops.size());
  if (max_sz == -1 || num_flops <= max_sz) {
    pointsets.push_back(flops);
    return;
  }

  std::vector<std::vector<Flop>> tmp_clusters[multistart_];
  std::vector<float> tmp_costs(multistart_);

  // multistart_ K-means++
  for (int i = 0; i < multistart_; i++) {
    KMeans(flops, tmp_clusters[i]);

    /* cur_cost = sum of distances between flops and its
    matching cluster's center */
    float cur_cost = 0;
    for (int j = 0; j < knn_; j++) {
      for (size_t k = 0; k + 1 < tmp_clusters[i][j].size(); k++) {
        cur_cost
            += GetDist(tmp_clusters[i][j][k].pt, tmp_clusters[i][j].back().pt);
      }
    }
    tmp_costs[i] = cur_cost;
  }

  float best_cost = std::numeric_limits<float>::max();
  std::vector<std::vector<Flop>> k_means_ret;
  for (int i = 0; i < multistart_; i++) {
    if (tmp_costs[i] < best_cost) {
      best_cost = tmp_costs[i];
      k_means_ret = tmp_clusters[i];
    }
  }

  /*
  create edges between all std::pairs of cluster centers
  edge weight = distance between the two centers
  */

  std::vector<std::pair<float, std::pair<int, int>>> cluster_pairs;
  for (int i = 0; i < knn_; i++) {
    for (int j = i + 1; j < knn_; j++) {
      const float dist
          = GetDist(k_means_ret[i].back().pt, k_means_ret[j].back().pt);
      cluster_pairs.emplace_back(dist, std::make_pair(i, j));
    }
  }
  std::sort(cluster_pairs.begin(), cluster_pairs.end());

  for (int i = 0; i < knn_; i++) {
    k_means_ret[i].pop_back();
  }

  // naive implementation of DSU
  std::vector<int> id(knn_);
  std::vector<int> sz(knn_);

  for (int i = 0; i < knn_; i++) {
    id[i] = i;
    sz[i] = static_cast<int>(k_means_ret[i].size());
  }

  for (const auto& cluster_pair : cluster_pairs) {
    int idx1 = cluster_pair.second.first;
    int idx2 = cluster_pair.second.second;

    if (sz[id[idx1]] < sz[id[idx2]]) {
      std::swap(idx1, idx2);
    }
    if (sz[id[idx1]] + sz[id[idx2]] > max_sz) {
      continue;
    }

    sz[id[idx1]] += sz[id[idx2]];

    // merge the two clusters
    const int orig_id = id[idx2];
    for (int j = 0; j < knn_; j++) {
      if (id[j] == orig_id) {
        id[j] = id[idx1];
      }
    }
  }

  std::vector<std::vector<Flop>> nxt_clusters(knn_);
  for (int i = 0; i < knn_; i++) {
    for (const Flop& f : k_means_ret[i]) {
      nxt_clusters[id[i]].push_back(f);
    }
  }

  // recurse on each new cluster
  for (int i = 0; i < knn_; i++) {
    if (static_cast<int>(nxt_clusters[i].size())) {
      std::vector<std::vector<Flop>> R;
      KMeansDecomp(nxt_clusters[i], max_sz, R);
      for (auto& x : R) {
        pointsets.push_back(x);
      }
    }
  }
}

float MBFF::GetPairDisplacements()
{
  float ret = 0;
  for (int i = 0; i < static_cast<int>(flops_.size()); i++) {
    for (const auto& j : paths_[i]) {
      float diff_x = slot_disp_x_[i];
      float diff_y = slot_disp_y_[i];
      diff_x -= slot_disp_x_[j];
      diff_y -= slot_disp_y_[j];
      ret += (std::abs(diff_x) + std::abs(diff_y));
    }
  }
  return ret;
}

float MBFF::RunClustering(const std::vector<Flop>& flops,
                          const int mx_sz,
                          const float alpha,
                          const float beta,
                          const std::vector<int> array_mask)
{
  std::vector<std::vector<Flop>> pointsets;
  KMeansDecomp(flops, mx_sz, pointsets);

  // all_start_trays[t][i][j]: start trays of size 2^i, multistart = j for
  // pointset[t]
  const int num_pointsets = static_cast<int>(pointsets.size());
  std::vector<std::vector<std::vector<Tray>>> all_start_trays[num_pointsets];
  for (int t = 0; t < num_pointsets; t++) {
    all_start_trays[t].resize(num_sizes_);
    for (int i = 1; i < num_sizes_; i++) {
      if (best_master_[array_mask][i] != nullptr) {
        const int rows = GetRows(GetBitCnt(i), array_mask);
        const int cols = GetBitCnt(i) / rows;
        const float AR = (cols * single_bit_width_ * norm_area_[i])
                         / (rows * single_bit_height_);
        const int num_trays
            = (static_cast<int>(pointsets[t].size()) + (GetBitCnt(i) - 1))
              / GetBitCnt(i);
        all_start_trays[t][i].resize(5);
        for (int j = 0; j < 5; j++) {
          // running in parallel ==> not reproducible
          GetStartTrays(pointsets[t], num_trays, AR, all_start_trays[t][i][j]);
        }
      }
    }
  }

  float ans = 0;
  std::vector<std::pair<int, int>> all_mappings[num_pointsets];
  std::vector<Tray> all_final_trays[num_pointsets];

#pragma omp parallel for num_threads(num_threads_)
  for (int t = 0; t < num_pointsets; t++) {
    std::vector<std::vector<Tray>> cur_trays;
    RunMultistart(cur_trays, pointsets[t], all_start_trays[t], array_mask);

    // run capacitated k-means per tray size
    const int num_flops = static_cast<int>(pointsets[t].size());
    for (int i = 1; i < num_sizes_; i++) {
      if (best_master_[array_mask][i] != nullptr) {
        const int rows = GetRows(GetBitCnt(i), array_mask),
                  cols = GetBitCnt(i) / rows;
        const int num_trays = (num_flops + (GetBitCnt(i) - 1)) / GetBitCnt(i);

        for (int j = 0; j < num_trays; j++) {
          GetSlots(cur_trays[i][j].pt,
                   rows,
                   cols,
                   cur_trays[i][j].slots,
                   array_mask);
        }

        std::vector<std::pair<int, int>> cluster;
        RunCapacitatedKMeans(
            pointsets[t], cur_trays[i], GetBitCnt(i), 35, cluster, array_mask);
        MinCostFlow(pointsets[t], cur_trays[i], GetBitCnt(i), cluster);
        for (int j = 0; j < num_trays; j++) {
          GetSlots(cur_trays[i][j].pt,
                   rows,
                   cols,
                   cur_trays[i][j].slots,
                   array_mask);
        }
      }
    }
    for (int i = 0; i < num_sizes_; i++) {
      if (!i || best_master_[array_mask][i] != nullptr) {
        all_final_trays[t].insert(
            all_final_trays[t].end(), cur_trays[i].begin(), cur_trays[i].end());
      }
    }
    std::vector<std::pair<int, int>> mapping(num_flops);
    const float cur_ans
        = RunILP(pointsets[t], all_final_trays[t], mapping, alpha, array_mask);
    all_mappings[t] = mapping;
    ans += cur_ans;
  }

  for (int t = 0; t < num_pointsets; t++) {
    ModifyPinConnections(
        pointsets[t], all_final_trays[t], all_mappings[t], array_mask);
  }

  if (graphics_) {
    Graphics::LineSegs segs;
    for (int t = 0; t < num_pointsets; t++) {
      const int num_flops = pointsets[t].size();
      for (int i = 0; i < num_flops; i++) {
        const int tray_idx = all_mappings[t][i].first;
        if (tray_idx == std::numeric_limits<int>::max()) {
          continue;
        }
        const Point tray_pt = all_final_trays[t][tray_idx].pt;
        const odb::Point tray_pt_dbu(multiplier_ * tray_pt.x,
                                     multiplier_ * tray_pt.y);
        const Point flop_pt = pointsets[t][i].pt;
        const odb::Point flop_pt_dbu(multiplier_ * flop_pt.x,
                                     multiplier_ * flop_pt.y);
        segs.emplace_back(flop_pt_dbu, tray_pt_dbu);
      }
    }

    graphics_->mbff_mapping(segs);
  }

  return ans;
}

void MBFF::SetVars(const std::vector<Flop>& flops)
{
  // get min height and width
  single_bit_height_ = std::numeric_limits<float>::max();
  single_bit_width_ = std::numeric_limits<float>::max();
  single_bit_power_ = std::numeric_limits<float>::max();
  for (const auto& flop : flops) {
    odb::dbMaster* master = insts_[flop.idx]->getMaster();
    single_bit_height_
        = std::min(single_bit_height_, master->getHeight() / multiplier_);
    single_bit_width_
        = std::min(single_bit_width_, master->getWidth() / multiplier_);
    sta::PowerResult ff_power
        = sta_->power(network_->dbToSta(insts_[flop.idx]), corner_);
    single_bit_power_ = std::min(single_bit_power_, ff_power.total());
  }
}

void MBFF::SetRatios(const std::vector<int> array_mask)
{
  norm_area_.clear();
  norm_area_.push_back(1.00);
  norm_power_.clear();
  norm_power_.push_back(1.00);

  for (int i = 1; i < num_sizes_; i++) {
    norm_area_.push_back(std::numeric_limits<float>::max());
    norm_power_.push_back(std::numeric_limits<float>::max());
    if (best_master_[array_mask][i] != nullptr) {
      const int slot_cnt = GetBitCnt(i);
      norm_area_[i] = (tray_area_[array_mask][i]
                       / (single_bit_height_ * single_bit_width_))
                      / slot_cnt;
      norm_power_[i]
          = (tray_power_[array_mask][i] / slot_cnt) / single_bit_power_;
    }
  }
}

void MBFF::SeparateFlops(std::vector<std::vector<Flop>>& ffs)
{
  // group by block clock name
  std::map<odb::dbNet*, std::vector<int>> clk_terms;
  for (size_t i = 0; i < flops_.size(); i++) {
    if (insts_[i]->isDoNotTouch()) {
      continue;
    }
    for (auto iterm : insts_[i]->getITerms()) {
      if (IsClockPin(iterm)) {
        auto net = iterm->getNet();
        if (!net) {
          continue;
        }
        clk_terms[net].push_back(i);
      }
    }
  }

  for (const auto& clks : clk_terms) {
    ArrayMaskVector<Flop> flops_by_mask;
    for (int idx : clks.second) {
      const std::vector<int> vec_mask = GetArrayMask(insts_[idx], false);
      flops_by_mask[vec_mask].push_back(flops_[idx]);
    }

    for (const auto& flop_pair : flops_by_mask) {
      if (!flop_pair.second.empty()) {
        ffs.push_back(flop_pair.second);
      }
    }
  }
}

void MBFF::SetTrayNames()
{
  for (size_t i = 0; i < 2 * flops_.size(); i++) {
    unused_.push_back(i);
  }
}

void MBFF::Run(const int mx_sz, const float alpha, const float beta)
{
  auto start = std::chrono::high_resolution_clock::now();

  std::srand(1);
  omp_set_num_threads(num_threads_);

  ReadFFs();
  ReadPaths();
  ReadLibs();
  SetTrayNames();

  std::vector<std::vector<Flop>> FFs;
  SeparateFlops(FFs);
  const int num_chunks = static_cast<int>(FFs.size());
  float tot_ilp = 0;
  for (int i = 0; i < num_chunks; i++) {
    const std::vector<int> array_mask
        = GetArrayMask(insts_[FFs[i].back().idx], false);
    // do we even have trays to cluster these flops?
    if (!best_master_[array_mask].size()) {
      tot_ilp += (alpha * static_cast<int>(FFs[i].size()));
      tray_sizes_used_[1] += static_cast<int>(FFs[i].size());
      continue;
    }
    SetVars(FFs[i]);
    SetRatios(array_mask);
    tot_ilp += RunClustering(FFs[i], mx_sz, alpha, beta, array_mask);
  }

  float tcp_disp = (beta * GetPairDisplacements());

  int num_paths = 0;
  for (auto& path : paths_) {
    num_paths += static_cast<int>(path.size());
  }

  // calculate average slot-to-flop displacement
  float avg_disp = 0;
  for (size_t i = 0; i < flops_.size(); i++) {
    float dX = (slot_disp_x_[i]);
    float dY = (slot_disp_y_[i]);
    avg_disp += (std::max(dX, -dX) + std::max(dY, -dY));
  }
  avg_disp /= (static_cast<int>(flops_.size()));

  // delete test_trays
  for (int i = 0; i < test_idx_; i++) {
    std::string test_tray_name = "test_tray_" + std::to_string(i);
    odb::dbInst* inst = block_->findInst(test_tray_name.c_str());
    odb::dbInst::destroy(inst);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration
      = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  log_->report("Alpha = {}, Beta = {}, #paths = {}, max size = {}",
               alpha,
               beta,
               num_paths,
               mx_sz);
  log_->report("Total ILP Cost: {}", tot_ilp);
  log_->report("Total Timing Critical Path Displacement: {}", tcp_disp);
  log_->report("Average slot-to-flop displacement: {}", avg_disp);
  log_->report("Final Objective Value: {}", tot_ilp + tcp_disp);
  log_->report("1-bit: {}, 2-bit: {}, 4-bit: {}",
               tray_sizes_used_[1],
               tray_sizes_used_[2],
               tray_sizes_used_[4]);
}

Point MBFF::GetTrayCenter(std::vector<int> array_mask, int idx)
{
  int num_slots = GetBitCnt(idx);
  std::vector<float> all_x;
  std::vector<float> all_y;
  for (int i = 0; i < num_slots; i++) {
    all_x.push_back(slot_to_tray_x_[array_mask][idx][i]);
    all_y.push_back(slot_to_tray_y_[array_mask][idx][i]);
  }
  std::sort(all_x.begin(), all_x.end());
  std::sort(all_y.begin(), all_y.end());
  float tray_center_x = (all_x[0] + all_x[num_slots - 1]) / 2.0;
  float tray_center_y = (all_y[0] + all_y[num_slots - 1]) / 2.0;
  return Point{tray_center_x, tray_center_y};
}

void MBFF::ReadLibs()
{
  test_idx_ = 0;
  for (auto lib : db_->getLibs()) {
    for (auto master : lib->getMasters()) {
      std::string tray_name = "test_tray_" + std::to_string(test_idx_++);
      odb::dbInst* tmp_tray
          = odb::dbInst::create(block_, master, tray_name.c_str());

      if (!IsValidTray(tmp_tray)) {
        continue;
      }

      const int num_slots = GetNumD(tmp_tray);
      const int idx = GetBitIdx(num_slots);
      const std::vector<int> array_mask = GetArrayMask(tmp_tray, true);

      if (!static_cast<int>(best_master_[array_mask].size())) {
        best_master_[array_mask].resize(num_sizes_, nullptr);
        tray_area_[array_mask].resize(num_sizes_,
                                      std::numeric_limits<float>::max());
        tray_power_[array_mask].resize(num_sizes_,
                                       std::numeric_limits<float>::max());
        tray_width_[array_mask].resize(num_sizes_);
        pin_mappings_[array_mask].resize(num_sizes_);

        slot_to_tray_x_[array_mask].resize(num_sizes_);
        slot_to_tray_y_[array_mask].resize(num_sizes_);
      }

      const float cur_area = (master->getHeight() / multiplier_)
                             * (master->getWidth() / multiplier_);
      sta::PowerResult tray_power
          = sta_->power(network_->dbToSta(tmp_tray), corner_);

      if (tray_area_[array_mask][idx] > cur_area) {
        tray_area_[array_mask][idx] = cur_area;
        tray_power_[array_mask][idx] = tray_power.total();
        best_master_[array_mask][idx] = master;
        pin_mappings_[array_mask][idx] = GetPinMapping(tmp_tray);
        tray_width_[array_mask][idx] = master->getWidth() / multiplier_;

        // save slot info
        tmp_tray->setLocation(0, 0);
        tmp_tray->setPlacementStatus(odb::dbPlacementStatus::PLACED);

        slot_to_tray_x_[array_mask][idx].clear();
        slot_to_tray_y_[array_mask][idx].clear();

        std::vector<Point> d;
        std::vector<Point> q;
        std::vector<Point> qn;

        for (const auto& p : pin_mappings_[array_mask][idx]) {
          odb::dbITerm* d_pin = tmp_tray->findITerm(p.first->name());
          odb::dbITerm* q_pin
              = (p.second.q ? tmp_tray->findITerm(p.second.q->name())
                            : nullptr);
          odb::dbITerm* qn_pin
              = (p.second.qn ? tmp_tray->findITerm(p.second.qn->name())
                             : nullptr);

          d.push_back(Point{
              d_pin->getBBox().xCenter() / multiplier_,
              d_pin->getBBox().yCenter() / multiplier_,
          });

          if (q_pin) {
            q.push_back(Point{
                q_pin->getBBox().xCenter() / multiplier_,
                q_pin->getBBox().yCenter() / multiplier_,
            });
          }

          if (qn_pin) {
            qn.push_back(Point{
                qn_pin->getBBox().xCenter() / multiplier_,
                qn_pin->getBBox().yCenter() / multiplier_,
            });
          }
        }

        // slots w.r.t. bottom-left corner
        for (int i = 0; i < num_slots; i++) {
          if (q.size() && qn.size()) {
            slot_to_tray_x_[array_mask][idx].push_back(
                (std::max(d[i].x, std::max(q[i].x, qn[i].x))
                 + std::min(d[i].x, std::min(q[i].x, qn[i].x)))
                / 2.0);
            slot_to_tray_y_[array_mask][idx].push_back(
                (std::max(d[i].y, std::max(q[i].y, qn[i].y))
                 + std::min(d[i].y, std::min(q[i].y, qn[i].y)))
                / 2.0);
          } else if (q.size()) {
            slot_to_tray_x_[array_mask][idx].push_back(
                (std::max(d[i].x, q[i].x) + std::min(d[i].x, q[i].x)) / 2.0);
            slot_to_tray_y_[array_mask][idx].push_back(
                (std::max(d[i].y, q[i].y) + std::min(d[i].y, q[i].y)) / 2.0);
          } else {
            slot_to_tray_x_[array_mask][idx].push_back(
                (std::max(d[i].x, qn[i].x) + std::min(d[i].x, qn[i].x)) / 2.0);
            slot_to_tray_y_[array_mask][idx].push_back(
                (std::max(d[i].y, qn[i].y) + std::min(d[i].y, qn[i].y)) / 2.0);
          }
        }
      }
    }
  }
}

void MBFF::ReadFFs()
{
  int num_flops = 0;
  for (auto inst : block_->getInsts()) {
    if (IsValidFlop(inst)) {
      const odb::Point origin = inst->getOrigin();
      const Point pt{origin.x() / multiplier_, origin.y() / multiplier_};
      flops_.push_back({pt, num_flops, 0.0});
      insts_.push_back(inst);
      name_to_idx_[inst->getName()] = num_flops;
      num_flops++;
    }
  }
  slot_disp_x_.resize(num_flops, 0.0);
  slot_disp_y_.resize(num_flops, 0.0);
}

// read num_paths_ timing-critical path pairs (FF-pair start/end points)
void MBFF::ReadPaths()
{
  paths_.resize(flops_.size());
  occs_.resize(flops_.size());

  sta::ExceptionFrom* e_from = nullptr;
  sta::ExceptionThruSeq* e_thrus = nullptr;
  sta::ExceptionTo* e_to = nullptr;

  sta_->ensureGraph();
  sta_->searchPreamble();
  sta_->ensureLevelized();
  sta::Search* search = sta_->search();
  sta::PathEndSeq path_ends = search->findPathEnds(e_from,
                                                   e_thrus,
                                                   e_to,
                                                   false,
                                                   nullptr,
                                                   sta::MinMaxAll::max(),
                                                   num_paths_,
                                                   num_paths_,
                                                   true,
                                                   -sta::INF,
                                                   sta::INF,
                                                   true,
                                                   nullptr,
                                                   true,
                                                   false,
                                                   false,
                                                   false,
                                                   false,
                                                   false);

  for (auto& path_end : path_ends) {
    sta::Path* path = path_end->path();
    sta::PathExpanded expanded(path, sta_);
    sta::PathRef* pathref_front = expanded.path(expanded.startIndex());
    sta::Vertex* pathvertex_front = pathref_front->vertex(sta_);
    sta::Pin* pathpin_front = pathvertex_front->pin();

    sta::PathRef* pathref_back
        = expanded.path(static_cast<int>(expanded.size()) - 1);
    sta::Vertex* pathvertex_back = pathref_back->vertex(sta_);
    sta::Pin* pathpin_back = pathvertex_back->pin();

    sta::Instance* start_ff = network_->instance(pathpin_front);
    sta::Instance* end_ff = network_->instance(pathpin_back);

    int idx1 = name_to_idx_[network_->pathName(start_ff)];
    int idx2 = name_to_idx_[network_->pathName(end_ff)];
    paths_[idx1].push_back(idx2);
    occs_[idx1]++;
    occs_[idx2]++;
    path_points_.insert(idx1);
    path_points_.insert(idx2);
  }
}

MBFF::MBFF(odb::dbDatabase* db,
           sta::dbSta* sta,
           utl::Logger* log,
           int threads,
           int knn,
           int multistart,
           int num_paths,
           bool debug_graphics)
    : db_(db),
      block_(db_->getChip()->getBlock()),
      sta_(sta),
      network_(sta_->getDbNetwork()),
      corner_(sta_->cmdCorner()),
      log_(log),
      num_threads_(threads),
      knn_(knn),
      multistart_(multistart),
      num_paths_(num_paths),
      multiplier_(static_cast<float>(block_->getDbUnitsPerMicron()))
{
  if (debug_graphics && Graphics::guiActive()) {
    graphics_ = std::make_unique<Graphics>(log_);
  }
}

MBFF::~MBFF() = default;

}  // end namespace gpl
