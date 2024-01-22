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
#include "sta/FuncExpr.hh"
#include "sta/Liberty.hh"
#include "sta/PortDirection.hh"
#include "sta/Sequential.hh"
#include "utl/Logger.h"

namespace gpl {

struct Point
{
  double x;
  double y;
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
  double prob;

  bool operator<(const Flop& a) const
  {
    return std::tie(prob, idx) < std::tie(a.prob, a.idx);
  }
};

struct Path
{
  int start_point;
  int end_point;
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
// check if a flop or single-bit in tray is inverting
bool MBFF::IsInverting(odb::dbInst* inst)
{
  sta::Cell* cell = network_->dbToSta(inst->getMaster());
  if (cell == nullptr) {
    return false;
  }
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  if (lib_cell == nullptr) {
    return false;
  }

  if (non_invert_func_ == nullptr) {
    for (auto iterm : inst->getITerms()) {
      if (IsQPin(iterm)) {
        auto pin = network_->dbToSta(iterm);
        auto port = network_->libertyPort(pin);
        non_invert_func_ = getFunction(port);
      }
    }
    return false;
  }

  for (auto iterm : inst->getITerms()) {
    if (IsQPin(iterm)) {
      auto pin = network_->dbToSta(iterm);
      auto port = network_->libertyPort(pin);
      if (sta::FuncExpr::equiv(non_invert_func_, getFunction(port))) {
        return false;
      }
    }
  }

  return true;
}

bool MBFF::HasSet(odb::dbInst* inst)
{
  sta::Cell* cell = network_->dbToSta(inst->getMaster());
  if (cell == nullptr) {
    return false;
  }
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  if (lib_cell == nullptr) {
    return false;
  }
  for (auto seq : lib_cell->sequentials()) {
    if (seq->preset()) {
      return true;
    }
  }
  return false;
}

bool MBFF::HasReset(odb::dbInst* inst)
{
  sta::Cell* cell = network_->dbToSta(inst->getMaster());
  if (cell == nullptr) {
    return false;
  }
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  if (lib_cell == nullptr) {
    return false;
  }
  for (auto seq : lib_cell->sequentials()) {
    if (seq->clear()) {
      return true;
    }
  }
  return false;
}

bool MBFF::ClockOn(odb::dbInst* inst)
{
  sta::Cell* cell = network_->dbToSta(inst->getMaster());
  if (cell == nullptr) {
    return false;
  }
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  if (lib_cell == nullptr) {
    return false;
  }
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

int MBFF::GetBitMask(odb::dbInst* inst)
{
  const int cnt_d = GetNumD(inst);
  const int cnt_q = GetNumQ(inst);
  int ret = 0;
  // turn 1st bit on
  if (cnt_q - cnt_d > 0) {
    ret |= (1 << 0);
    // check if the instance is inverting
    if (IsInverting(inst)) {
      ret |= (1 << 3);
    }
  }
  // turn 2nd bit on
  if (HasSet(inst)) {
    ret |= (1 << 1);
  }
  // turn 3rd bit on
  if (HasReset(inst)) {
    ret |= (1 << 2);
  }
  // turn 4th bit on
  if (ClockOn(inst)) {
    ret |= (1 << 3);
  }
  return ret;
}

bool MBFF::IsClockPin(odb::dbITerm* iterm)
{
  const bool yes = (iterm->getSigType() == odb::dbSigType::CLOCK);
  const sta::Pin* pin = network_->dbToSta(iterm);
  return yes || sta_->isClock(pin);
}

bool MBFF::IsSupplyPin(odb::dbITerm* iterm)
{
  return iterm->getSigType().isSupply();
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
  auto port = network_->libertyPort(pin);
  if (port == nullptr) {
    return false;
  }

  for (auto seq : lib_cell->sequentials()) {
    if (seq->clear() && sta::FuncExpr::equiv(seq->clear(), getFunction(port))) {
      return false;
    }
    if (seq->preset()
        && sta::FuncExpr::equiv(seq->preset(), getFunction(port))) {
      return false;
    }
  }

  const bool exclude = (IsClockPin(iterm) || IsSupplyPin(iterm));
  const bool yes = (iterm->getIoType() == odb::dbIoType::INPUT);
  return (yes & !exclude);
}

bool MBFF::IsQPin(odb::dbITerm* iterm)
{
  const bool exclude = (IsClockPin(iterm) || IsSupplyPin(iterm));
  const bool yes = (iterm->getIoType() == odb::dbIoType::OUTPUT);
  return (yes & !exclude);
}

int MBFF::GetNumD(odb::dbInst* inst)
{
  int cnt_d = 0;
  sta::Cell* cell = network_->dbToSta(inst->getMaster());
  if (cell == nullptr) {
    return 0;
  }
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  if (lib_cell == nullptr) {
    return 0;
  }
  for (auto seq : lib_cell->sequentials()) {
    auto data = seq->data();
    sta::FuncExprPortIterator port_itr(data);
    while (port_itr.hasNext()) {
      sta::LibertyPort* port = port_itr.next();
      if (port != nullptr) {
        cnt_d++;
      } else {
        break;
      }
    }
  }
  return cnt_d;
}

int MBFF::GetNumQ(odb::dbInst* inst)
{
  int num_q = 0;
  for (auto iterm : inst->getITerms()) {
    num_q += IsQPin(iterm);
  }
  return num_q;
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

int MBFF::GetBitCnt(int bit_idx)
{
  return (1 << bit_idx);
}

int MBFF::GetRows(int slot_cnt, int bitmask)
{
  const int idx = GetBitIdx(slot_cnt);
  const int width = int(multiplier_ * tray_width_[bitmask][idx]);
  const int height = int(
      multiplier_ * (tray_area_[bitmask][idx] / tray_width_[bitmask][idx]));
  return (height / std::gcd(width, height));
}

double MBFF::GetDist(const Point& a, const Point& b)
{
  return (abs(a.x - b.x) + abs(a.y - b.y));
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
  return GetNumD(tray) > 1 && GetNumQ(tray) > 1;
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
    sta::FuncExprPortIterator port_itr(data);
    while (port_itr.hasNext()) {
      sta::LibertyPort* port = port_itr.next();
      d_pins.push_back(port);
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
      if (!q_pins.empty()) {
        if (sta::FuncExpr::equiv(getFunction(q_pins.back()),
                                 getFunction(port))) {
          q_pins.push_back(port);
        } else {
          qn_pins.push_back(port);
        }
      } else {
        q_pins.push_back(port);
      }
    }
  }

  DataToOutputsMap ret;
  for (size_t i = 0; i < d_pins.size(); i++) {
    ret[d_pins[i]] = {q_pins[i], (!qn_pins.empty() ? qn_pins[i] : nullptr)};
  }
  return ret;
}

void MBFF::ModifyPinConnections(const std::vector<Flop>& flops,
                                const std::vector<Tray>& trays,
                                const std::vector<std::pair<int, int>>& mapping,
                                const int bitmask)
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

      // the chance that two trays with the same size get the same number is
      // rare (long long max is 10^18)
      std::string new_name
          = "_tray_size"
            + std::to_string(static_cast<int>(trays[tray_idx].slots.size()))
            + "_" + std::to_string(unused_.back());
      unused_.pop_back();
      const int bit_idx
          = GetBitIdx(static_cast<int>(trays[tray_idx].slots.size()));
      auto new_tray = odb::dbInst::create(
          block_, best_master_[bitmask][bit_idx], new_name.c_str());
      new_tray->setLocation(
          static_cast<int>(multiplier_ * trays[tray_idx].pt.x),
          static_cast<int>(multiplier_ * trays[tray_idx].pt.y));
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
    for (const auto& pins : pin_mappings_[bitmask][tray_sz_idx]) {
      if (idx == slot_idx) {
        d_pin = pins.first;
        q_pin = pins.second.q;
        qn_pin = pins.second.qn;
        break;
      }
      idx++;
    }

    // disconnect / reconnect iterms
    for (auto iterm : insts_[flops[i].idx]->getITerms()) {
      if (IsSupplyPin(iterm)) {
        continue;
      }
      auto net = iterm->getNet();
      if (!net) {
        continue;
      }
      iterm->disconnect();
      if (IsDPin(iterm)) {
        tray_inst[tray_idx]->findITerm(d_pin->name())->connect(net);
      }
      if (IsQPin(iterm)) {
        if (bitmask) {
          sta::Pin* pin = network_->dbToSta(iterm);
          sta::LibertyPort* iterm_port = network_->libertyPort(pin);
          if (sta::FuncExpr::equiv(getFunction(q_pin),
                                   getFunction(iterm_port))) {
            tray_inst[tray_idx]->findITerm(q_pin->name())->connect(net);
          } else {
            tray_inst[tray_idx]->findITerm(qn_pin->name())->connect(net);
          }
        } else {
          tray_inst[tray_idx]->findITerm(q_pin->name())->connect(net);
        }
      }
      if (IsClockPin(iterm)) {
        iterm->disconnect();
        clk_net = net;
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

double MBFF::RunLP(const std::vector<Flop>& flops,
                   std::vector<Tray>& trays,
                   const std::vector<std::pair<int, int>>& clusters)
{
  const int num_flops = static_cast<int>(flops.size());
  const int num_trays = static_cast<int>(trays.size());

  std::unique_ptr<operations_research::MPSolver> solver(
      operations_research::MPSolver::CreateSolver("GLOP"));
  const double inf = solver->infinity();

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

    const double shift_x = slot.x - tray.x;
    const double shift_y = slot.y - tray.y;

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
  for (int i = 0; i < num_flops; i++) {
    objective->SetCoefficient(disp_x[i], 1);
    objective->SetCoefficient(disp_y[i], 1);
  }
  objective->SetMinimization();
  solver->Solve();

  double tot_disp = 0;
  for (int i = 0; i < num_trays; i++) {
    const double new_x = tray_x[i]->solution_value();
    const double new_y = tray_y[i]->solution_value();

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
                    double alpha,
                    int bitmask)
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
  double max_dist = 0;
  for (int i = 0; i < num_flops; i++) {
    for (size_t j = 0; j < cand_tray[i].size(); j++) {
      const int tray_idx = cand_tray[i][j];
      const int slot_idx = cand_slot[i][j];

      const double shift_x = trays[tray_idx].slots[slot_idx].x - flops[i].pt.x;
      const double shift_y = trays[tray_idx].slots[slot_idx].y - flops[i].pt.y;

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

  /*
  remove timing-critical path constraints / objective.
  instead, replace it with a higher coefficient value when it comes to
  minimizing the displacement from flop to slot
  */

  std::vector<int> coeff(num_flops, 1);
  for (int i = 0; i < num_flops; i++) {
    if (flops_in_path_.count(i)) {
      coeff[i] = 1;
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
  std::vector<double> tray_cost(num_trays);
  for (int i = 0; i < num_trays; i++) {
    int bit_idx = 0;
    for (int j = 0; j < num_sizes_; j++) {
      if (best_master_[bitmask][j] != nullptr) {
        if (GetBitCnt(j) == static_cast<int>(trays[i].slots.size())) {
          bit_idx = j;
        }
      }
    }
    if (GetBitCnt(bit_idx) == 1) {
      tray_cost[i] = 1.00;
    } else {
      tray_cost[i] = (GetBitCnt(bit_idx) * ratios_[bit_idx]);
    }
  }

  /*
    DoubleLinearExpr supports double coefficients
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

  operations_research::sat::CpSolverResponse response
      = operations_research::sat::SolveCpModel(cp_model.Build(), &model);

  if (response.status() == operations_research::sat::CpSolverStatus::FEASIBLE
      || response.status()
             == operations_research::sat::CpSolverStatus::OPTIMAL) {
    // update slot_disp_ vectors
    for (int i = 0; i < num_flops; i++) {
      for (size_t j = 0; j < cand_tray[i].size(); j++) {
        if (operations_research::sat::SolutionIntegerValue(response,
                                                           mapped[i][j])
            == 1) {
          slot_disp_x_[flops[i].idx]
              = trays[cand_tray[i][j]].slots[cand_slot[i][j]].x - flops[i].pt.x;
          slot_disp_y_[flops[i].idx]
              = trays[cand_tray[i][j]].slots[cand_slot[i][j]].y - flops[i].pt.y;
          final_flop_to_slot[i] = {cand_tray[i][j], cand_slot[i][j]};
        }
      }
    }
    return static_cast<double>(response.objective_value());
  }
  return 0;
}

void MBFF::GetSlots(const Point& tray,
                    const int rows,
                    const int cols,
                    std::vector<Point>& slots,
                    const int bitmask)
{
  slots.clear();
  const int idx = GetBitIdx(rows * cols);
  for (int i = 0; i < rows * cols; i++) {
    const Point pt = Point{tray.x + slot_to_tray_x_[bitmask][idx][i],
                           tray.y + slot_to_tray_y_[bitmask][idx][i]};
    slots.push_back(pt);
  }
}

Flop MBFF::GetNewFlop(const std::vector<Flop>& prob_dist, const double tot_dist)
{
  const double rand_num = (double) (rand() % 101);
  double cum_sum = 0;
  Flop new_flop;
  for (const Flop& flop : prob_dist) {
    cum_sum += flop.prob;
    new_flop = flop;
    if (cum_sum * 100.0 >= rand_num * tot_dist) {
      break;
    }
  }
  return new_flop;
}

void MBFF::GetStartTrays(std::vector<Flop> flops,
                         const int num_trays,
                         const double AR,
                         std::vector<Tray>& trays)
{
  const int num_flops = static_cast<int>(flops.size());

  /* pick a random flop */
  const int rand_idx = rand() % (num_flops);
  Tray tray_zero;
  tray_zero.pt = flops[rand_idx].pt;

  std::set<int> used_flops;
  used_flops.insert(rand_idx);
  trays.push_back(tray_zero);

  double tot_dist = 0;
  for (int i = 0; i < num_flops; i++) {
    const double contr = GetDist(flops[i].pt, tray_zero.pt) / AR;
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
      const double new_contr = GetDist(flops[i].pt, new_tray.pt) / AR;
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

double MBFF::GetSilh(const std::vector<Flop>& flops,
                     const std::vector<Tray>& trays,
                     const std::vector<std::pair<int, int>>& clusters)
{
  const int num_flops = static_cast<int>(flops.size());
  const int num_trays = static_cast<int>(trays.size());

  double tot = 0;
  for (int i = 0; i < num_flops; i++) {
    double min_num = std::numeric_limits<double>::max();
    double max_den = GetDist(
        flops[i].pt, trays[clusters[i].first].slots[clusters[i].second]);
    for (int j = 0; j < num_trays; j++) {
      if (j != clusters[i].first) {
        max_den = std::max(
            max_den, GetDist(flops[i].pt, trays[j].slots[clusters[i].second]));
        for (const Point& slot : trays[j].slots) {
          min_num = std::min(min_num, GetDist(flops[i].pt, slot));
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
                                const int bitmask)
{
  cluster.clear();
  const int num_flops = static_cast<int>(flops.size());
  const int rows = GetRows(sz, bitmask);
  const int cols = sz / rows;
  const int num_trays = (num_flops + (sz - 1)) / sz;

  double delta = 0;
  for (int i = 0; i < iter; i++) {
    MinCostFlow(flops, trays, sz, cluster);
    delta = RunLP(flops, trays, cluster);

    for (int j = 0; j < num_trays; j++) {
      GetSlots(trays[j].pt, rows, cols, trays[j].slots, bitmask);
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

void MBFF::RunSilh(std::vector<std::vector<Tray>>& trays,
                   const std::vector<Flop>& flops,
                   std::vector<std::vector<std::vector<Tray>>>& start_trays,
                   const int bitmask)
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

  std::vector<double> res[num_sizes_];
  std::vector<std::pair<int, int>> ind;

  for (int i = 1; i < num_sizes_; i++) {
    if (best_master_[bitmask][i] != nullptr) {
      for (int j = 0; j < 5; j++) {
        ind.emplace_back(i, j);
      }
      res[i].resize(5);
    }
  }

  for (int i = 1; i < num_sizes_; i++) {
    if (best_master_[bitmask][i] != nullptr) {
      for (int j = 0; j < 5; j++) {
        const int rows = GetRows(GetBitCnt(i), bitmask);
        const int cols = GetBitCnt(i) / rows;
        const int num_trays = (num_flops + (GetBitCnt(i) - 1)) / GetBitCnt(i);

        for (int k = 0; k < num_trays; k++) {
          GetSlots(start_trays[i][j][k].pt,
                   rows,
                   cols,
                   start_trays[i][j][k].slots,
                   bitmask);
          start_trays[i][j][k].cand.reserve(rows * cols);
          for (int idx = 0; idx < rows * cols; idx++) {
            start_trays[i][j][k].cand.emplace_back(-1);
          }
        }
      }
    }
  }

  // run multistart_ in parallel
  for (const auto& [bit_idx, tray_idx] : ind) {
    const int rows = GetRows(GetBitCnt(bit_idx), bitmask);
    const int cols = GetBitCnt(bit_idx) / rows;

    std::vector<std::pair<int, int>> tmp_cluster;

    RunCapacitatedKMeans(flops,
                         start_trays[bit_idx][tray_idx],
                         rows * cols,
                         8,
                         tmp_cluster,
                         bitmask);

    res[bit_idx][tray_idx]
        = GetSilh(flops, start_trays[bit_idx][tray_idx], tmp_cluster);
  }

  for (int i = 1; i < num_sizes_; i++) {
    if (best_master_[bitmask][i] != nullptr) {
      int opt_idx = 0;
      double opt_val = -1;
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
  const int seed = rand() % num_flops;
  std::set<int> chosen({seed});

  std::vector<Flop> centers;
  centers.push_back(flops[seed]);

  std::vector<double> d(num_flops);
  for (int i = 0; i < num_flops; i++) {
    d[i] = GetDist(flops[i].pt, flops[seed].pt);
  }

  // choose remaining K-1 centers
  while (static_cast<int>(chosen.size()) < knn_) {
    double tot_sum = 0;

    for (int i = 0; i < num_flops; i++) {
      if (!chosen.count(i)) {
        for (int j : chosen) {
          d[i] = std::min(d[i], GetDist(flops[i].pt, flops[j].pt));
        }
        tot_sum += (double(d[i]) * double(d[i]));
      }
    }

    const int rnd = rand() % (int(tot_sum * 100));
    const double prob = rnd / 100.0;

    double cum_sum = 0;
    for (int i = 0; i < num_flops; i++) {
      if (!chosen.count(i)) {
        cum_sum += (double(d[i]) * double(d[i]));
        if (cum_sum >= prob) {
          chosen.insert(i);
          centers.push_back(flops[i]);
          break;
        }
      }
    }
  }

  clusters.resize(knn_);
  double prev = -1;
  while (true) {
    for (int i = 0; i < knn_; i++) {
      clusters[i].clear();
    }

    // remap flops to clusters
    for (int i = 0; i < num_flops; i++) {
      double min_cost = std::numeric_limits<double>::max();
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
      double cX = 0;
      double cY = 0;

      for (const Flop& flop : clusters[i]) {
        cX += flop.pt.x;
        cY += flop.pt.y;
      }

      const double new_x = cX / double(cur_sz);
      const double new_y = cY / double(cur_sz);
      centers[i].pt = Point{new_x, new_y};
    }

    // get total displacement
    double tot_disp = 0;
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
  std::vector<double> tmp_costs(multistart_);

  // multistart_ K-means++
  for (int i = 0; i < multistart_; i++) {
    KMeans(flops, tmp_clusters[i]);

    /* cur_cost = sum of distances between flops and its
    matching cluster's center */
    double cur_cost = 0;
    for (int j = 0; j < knn_; j++) {
      for (size_t k = 0; k + 1 < tmp_clusters[i][j].size(); k++) {
        cur_cost
            += GetDist(tmp_clusters[i][j][k].pt, tmp_clusters[i][j].back().pt);
      }
    }
    tmp_costs[i] = cur_cost;
  }

  double best_cost = std::numeric_limits<double>::max();
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

  std::vector<std::pair<double, std::pair<int, int>>> cluster_pairs;
  for (int i = 0; i < knn_; i++) {
    for (int j = i + 1; j < knn_; j++) {
      const double dist
          = GetDist(k_means_ret[i].back().pt, k_means_ret[j].back().pt);
      cluster_pairs.emplace_back(dist, std::make_pair(i, j));
    }
  }
  std::sort(cluster_pairs.begin(), cluster_pairs.end());

  for (int i = 0; i < knn_; i++) {
    k_means_ret[i].pop_back();
  }

  /*
      lines 890-918: basic implementation of DSU
      keep uniting sets (== mini-pointsets with size <= max_sz) while not
      exceeding max_sz
  */

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

double MBFF::GetTCPDisplacement()
{
  double ret = 0;
  for (const auto& path : paths_) {
    double diff_x = slot_disp_x_[path.start_point];
    double diff_y = slot_disp_y_[path.start_point];
    diff_x -= slot_disp_x_[path.end_point];
    diff_y -= slot_disp_y_[path.end_point];
    ret += (std::abs(diff_x) + std::abs(diff_y));
  }
  return ret;
}

double MBFF::doit(const std::vector<Flop>& flops,
                  const int mx_sz,
                  const double alpha,
                  const double beta,
                  const int bitmask)
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
      if (best_master_[bitmask][i] != nullptr) {
        const int rows = GetRows(GetBitCnt(i), bitmask);
        const int cols = GetBitCnt(i) / rows;
        const double AR = (cols * width_ * ratios_[i]) / (rows * height_);
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

  double ans = 0;
  std::vector<std::pair<int, int>> all_mappings[num_pointsets];
  std::vector<Tray> all_final_trays[num_pointsets];

#pragma omp parallel for num_threads(num_threads_)
  for (int t = 0; t < num_pointsets; t++) {
    std::vector<std::vector<Tray>> cur_trays;
    RunSilh(cur_trays, pointsets[t], all_start_trays[t], bitmask);

    // run capacitated k-means per tray size
    const int num_flops = static_cast<int>(pointsets[t].size());
    for (int i = 1; i < num_sizes_; i++) {
      if (best_master_[bitmask][i] != nullptr) {
        const int rows = GetRows(GetBitCnt(i), bitmask),
                  cols = GetBitCnt(i) / rows;
        const int num_trays = (num_flops + (GetBitCnt(i) - 1)) / GetBitCnt(i);

        for (int j = 0; j < num_trays; j++) {
          GetSlots(
              cur_trays[i][j].pt, rows, cols, cur_trays[i][j].slots, bitmask);
        }

        std::vector<std::pair<int, int>> cluster;
        RunCapacitatedKMeans(
            pointsets[t], cur_trays[i], GetBitCnt(i), 35, cluster, bitmask);
        MinCostFlow(pointsets[t], cur_trays[i], GetBitCnt(i), cluster);
        for (int j = 0; j < num_trays; j++) {
          GetSlots(
              cur_trays[i][j].pt, rows, cols, cur_trays[i][j].slots, bitmask);
        }
      }
    }
    for (int i = 0; i < num_sizes_; i++) {
      if (!i || best_master_[bitmask][i] != nullptr) {
        all_final_trays[t].insert(
            all_final_trays[t].end(), cur_trays[i].begin(), cur_trays[i].end());
      }
    }
    std::vector<std::pair<int, int>> mapping(num_flops);
    const double cur_ans
        = RunILP(pointsets[t], all_final_trays[t], mapping, alpha, bitmask);
    all_mappings[t] = mapping;
    ans += cur_ans;
  }

  for (int t = 0; t < num_pointsets; t++) {
    ModifyPinConnections(
        pointsets[t], all_final_trays[t], all_mappings[t], bitmask);
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
  height_ = std::numeric_limits<double>::max();
  width_ = std::numeric_limits<double>::max();
  for (size_t i = 0; i < flops.size(); i++) {
    odb::dbMaster* master = insts_[flops_[i].idx]->getMaster();
    height_ = std::min(height_, master->getHeight() / multiplier_);
    width_ = std::min(width_, master->getWidth() / multiplier_);
  }
}

void MBFF::SetRatios(int bitmask)
{
  ratios_.clear();
  ratios_.push_back(1.00);
  for (int i = 1; i < num_sizes_; i++) {
    ratios_.push_back(std::numeric_limits<float>::max());
    if (best_master_[bitmask][i] != nullptr) {
      const int slot_cnt = GetBitCnt(i);
      const int rows = GetRows(slot_cnt, bitmask);
      const int cols = slot_cnt / rows;
      ratios_[i]
          = (tray_width_[bitmask][i] / (width_ * (static_cast<float>(cols))));
      log_->info(
          utl::GPL, 1002, "Ratio for tray size {}: {}", slot_cnt, ratios_[i]);
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
    BitMaskVector<Flop> flops_by_mask;
    for (int idx : clks.second) {
      const int bitmask = GetBitMask(insts_[idx]);
      flops_by_mask[bitmask].push_back(flops_[idx]);
    }

    for (const auto& flops : flops_by_mask) {
      if (!flops.empty()) {
        ffs.push_back(flops);
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

void MBFF::Run(const int mx_sz, const double alpha, const double beta)
{
  srand(1);
  omp_set_num_threads(num_threads_);

  ReadFFs();
  ReadPaths();
  ReadLibs();
  SetTrayNames();

  std::vector<std::vector<Flop>> FFs;
  SeparateFlops(FFs);
  const int num_chunks = static_cast<int>(FFs.size());
  double tot_ilp = 0;
  for (int i = 0; i < num_chunks; i++) {
    SetVars(FFs[i]);
    const int bitmask = GetBitMask(insts_[FFs[i].back().idx]);
    SetRatios(bitmask);
    tot_ilp += doit(FFs[i], mx_sz, alpha, beta, bitmask);
  }
  double tcp_disp = (beta * GetTCPDisplacement());

  // round
  tot_ilp = std::ceil(tot_ilp * multiplier_);
  tot_ilp /= multiplier_;

  tcp_disp = std::ceil(tcp_disp * multiplier_);
  tcp_disp /= multiplier_;

  log_->report("Alpha = {}, Beta = {}, max size = {}", alpha, beta, mx_sz);
  log_->report("Total ILP Cost: {}", tot_ilp);
  log_->report("Total Timing Critical Path Displacement: {}", tcp_disp);
  log_->report("Final Objective Value: {}", tot_ilp + tcp_disp);
}

void MBFF::ReadLibs()
{
  for (int i = 0; i < max_bitmask; i++) {
    best_master_[i].resize(num_sizes_, nullptr);
    tray_area_[i].resize(num_sizes_, std::numeric_limits<double>::max());
    tray_width_[i].resize(num_sizes_);
    pin_mappings_[i].resize(num_sizes_);

    slot_to_tray_x_[i].resize(num_sizes_);
    slot_to_tray_y_[i].resize(num_sizes_);
  }

  for (auto lib : db_->getLibs()) {
    for (auto master : lib->getMasters()) {
      odb::dbInst* tmp_tray = odb::dbInst::create(block_, master, "test_tray");
      if (!IsValidTray(tmp_tray)) {
        odb::dbInst::destroy(tmp_tray);
        continue;
      }

      const int num_slots = GetNumD(tmp_tray);
      const int idx = GetBitIdx(num_slots);
      const int bitmask = GetBitMask(tmp_tray);
      const double cur_area = (master->getHeight() / multiplier_)
                              * (master->getWidth() / multiplier_);

      if (tray_area_[bitmask][idx] > cur_area) {
        tray_area_[bitmask][idx] = cur_area;
        best_master_[bitmask][idx] = master;
        pin_mappings_[bitmask][idx] = GetPinMapping(tmp_tray);
        tray_width_[bitmask][idx]
            = static_cast<double>(master->getWidth()) / multiplier_;

        // save slot info
        tmp_tray->setLocation(0, 0);
        tmp_tray->setPlacementStatus(odb::dbPlacementStatus::PLACED);

        slot_to_tray_x_[bitmask][idx].clear();
        slot_to_tray_y_[bitmask][idx].clear();

        std::vector<Point> d(num_slots);
        std::vector<Point> q(num_slots);
        std::vector<Point> qn(num_slots);

        int itr = 0;
        for (const auto& p : pin_mappings_[bitmask][idx]) {
          odb::dbITerm* d_pin = tmp_tray->findITerm(p.first->name());
          odb::dbITerm* q_pin = tmp_tray->findITerm(p.second.q->name());
          odb::dbITerm* qn_pin = nullptr;
          if (bitmask & (1 << 0)) {
            qn_pin = tmp_tray->findITerm(p.second.qn->name());
          }

          d[itr] = Point{
              d_pin->getBBox().xCenter() / multiplier_,
              d_pin->getBBox().yCenter() / multiplier_,
          };
          q[itr] = Point{
              q_pin->getBBox().xCenter() / multiplier_,
              q_pin->getBBox().yCenter() / multiplier_,
          };

          if (bitmask & (1 << 0)) {
            qn[itr] = Point{
                qn_pin->getBBox().xCenter() / multiplier_,
                qn_pin->getBBox().yCenter() / multiplier_,
            };
          }

          itr++;
        }
        for (int i = 0; i < num_slots; i++) {
          if (bitmask & (1 << 0)) {
            slot_to_tray_x_[bitmask][idx].push_back(
                (std::max(d[i].x, std::max(q[i].x, qn[i].x))
                 + std::min(d[i].x, std::min(q[i].x, qn[i].x)))
                / 2.0);
            slot_to_tray_y_[bitmask][idx].push_back(
                (std::max(d[i].y, std::max(q[i].y, qn[i].y))
                 + std::min(d[i].y, std::min(q[i].y, qn[i].y)))
                / 2.0);
          } else {
            slot_to_tray_x_[bitmask][idx].push_back(
                (std::max(d[i].x, q[i].x) + std::min(d[i].x, q[i].x)) / 2.0);
            slot_to_tray_y_[bitmask][idx].push_back(
                (std::max(d[i].y, q[i].y) + std::min(d[i].y, q[i].y)) / 2.0);
          }
        }
      }
      odb::dbInst::destroy(tmp_tray);
    }
  }

  for (int k = 0; k < max_bitmask; k++) {
    for (int i = 1; i < num_sizes_; i++) {
      if (best_master_[k][i] != nullptr) {
        log_->info(utl::GPL,
                   1001,
                   "Name of master with minimum area for size = {}: {}",
                   GetBitCnt(i),
                   best_master_[k][i]->getName());
        for (size_t j = 0; j < slot_to_tray_x_[k][i].size(); j++) {
          log_->info(utl::GPL,
                     1010,
                     "delta x: {}, delta y: {}",
                     slot_to_tray_x_[k][i][j],
                     slot_to_tray_y_[k][i][j]);
        }
      }
    }
  }
}

void MBFF::ReadFFs()
{
  int num_flops = 0;
  for (auto inst : block_->getInsts()) {
    sta::Cell* cell = network_->dbToSta(inst->getMaster());
    sta::LibertyCell* lib_cell = network_->libertyCell(cell);
    if (lib_cell && lib_cell->hasSequentials()) {
      const odb::Point origin = inst->getOrigin();
      const Point pt{origin.x() / multiplier_, origin.y() / multiplier_};
      flops_.push_back({pt, num_flops, 0.0});
      insts_.push_back(inst);
      num_flops++;
    }
  }
  slot_disp_x_.resize(num_flops, 0.0);
  slot_disp_y_.resize(num_flops, 0.0);
}

// how are timing critical paths determined?
void MBFF::ReadPaths()
{
}

MBFF::MBFF(odb::dbDatabase* db,
           sta::dbSta* sta,
           utl::Logger* log,
           int threads,
           int knn,
           int multistart,
           bool debug_graphics)
    : db_(db),
      block_(db_->getChip()->getBlock()),
      sta_(sta),
      network_(sta_->getDbNetwork()),
      log_(log),
      num_threads_(threads),
      multistart_(multistart),
      knn_(knn),
      multiplier_(block_->getDbUnitsPerMicron())
{
  if (debug_graphics && Graphics::guiActive()) {
    graphics_ = std::make_unique<Graphics>(log_);
  }
}

MBFF::~MBFF() = default;

}  // end namespace gpl
