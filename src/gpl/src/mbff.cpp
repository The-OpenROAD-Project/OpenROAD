// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "mbff.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "AbstractGraphics.h"
#include "absl/container/inlined_vector.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "lemon/core.h"
#include "lemon/list_graph.h"
#include "lemon/network_simplex.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "omp.h"
#include "ortools/linear_solver/linear_solver.h"
#include "ortools/sat/cp_model.h"
#include "ortools/util/sorted_interval_list.h"
#include "rsz/Resizer.hh"
#include "sta/ClkNetwork.hh"
#include "sta/ConcreteLibrary.hh"
#include "sta/ExceptionPath.hh"
#include "sta/FuncExpr.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/InputDrive.hh"
#include "sta/InternalPower.hh"
#include "sta/LeakagePower.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PortDirection.hh"
#include "sta/PowerClass.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/Search.hh"
#include "sta/SearchClass.hh"
#include "sta/Sequential.hh"
#include "sta/StringUtil.hh"
#include "sta/TableModel.hh"
#include "utl/Logger.h"

namespace gpl {

using odb::dbInst;
using odb::dbITerm;
using odb::dbMaster;
using odb::dbModule;
using odb::dbMTerm;
using odb::dbNet;
using utl::GPL;

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

std::string MBFF::Mask::to_string() const
{
  return fmt::format("{}{}{}{}{}{}{}",
                     (int) clock_polarity,
                     (int) has_clear,
                     (int) has_preset,
                     (int) pos_output,
                     (int) inv_output,
                     func_idx,
                     (int) is_scan_cell);
}

bool MBFF::Mask::operator<(const Mask& rhs) const
{
  return std::tie(clock_polarity,
                  has_clear,
                  has_preset,
                  pos_output,
                  inv_output,
                  func_idx,
                  is_scan_cell)
         < std::tie(rhs.clock_polarity,
                    rhs.has_clear,
                    rhs.has_preset,
                    rhs.pos_output,
                    rhs.inv_output,
                    rhs.func_idx,
                    rhs.is_scan_cell);
}

namespace {
// Rebind a tray iterm to the given (flat, mod) pair. The plain
// dbITerm::connect(dbNet*) overload only updates the flat side; if a
// stale modnet was attached from an earlier per-orig-flop iteration
// on a shared role pin, it would form an inconsistent (flat, mod)
// pair. Clear the mod side explicitly when only flat is being
// rebound. dbITerm::connect(dbNet*, dbModNet*) cannot be used with a
// null mod_net because it dereferences the modnet unconditionally.
void reconnectIterm(dbITerm* tray_iterm, dbNet* net, odb::dbModNet* mod_net)
{
  if (net && mod_net) {
    tray_iterm->connect(net, mod_net);
  } else if (net) {
    tray_iterm->disconnectDbModNet();
    tray_iterm->connect(net);
  } else if (mod_net) {
    tray_iterm->connect(mod_net);
  }
}

}  // namespace

float MBFF::GetDist(const Point& a, const Point& b)
{
  return (abs(a.x - b.x) + abs(a.y - b.y));
}

float MBFF::GetDistAR(const Point& a, const Point& b, const float AR)
{
  return (abs(a.x - b.x) / AR + abs(a.y - b.y));
}

int MBFF::GetBitCnt(const int bit_idx)
{
  return (1 << bit_idx);
}

int MBFF::GetBitIdx(const int bit_cnt)
{
  for (int i = 0; i < num_sizes_; i++) {
    if ((1 << i) == bit_cnt) {
      return i;
    }
  }
  log_->error(GPL, 122, "{} is not in 2^[0,{}]", bit_cnt, num_sizes_);
}

MBFF::PortName MBFF::PortType(const sta::LibertyPort* lib_port, dbInst* inst)
{
  dbMTerm* mterm = network_->staToDb(lib_port);

  // physical pins
  if (mterm != nullptr) {
    dbITerm* iterm = inst->getITerm(mterm);
    if (iterm != nullptr) {
      if (network_->isDPin(iterm)) {
        return d;
      }
      if (network_->isScanIn(iterm)) {
        return si;
      }
      if (network_->isScanEnable(iterm)) {
        return se;
      }
      if (network_->isPresetPin(iterm)) {
        return preset;
      }
      if (network_->isClearPin(iterm)) {
        return clear;
      }
      if (network_->isQPin(iterm)) {
        return (network_->isInvertingQPin(iterm) ? qn : q);
      }
      if (network_->isSupplyPin(iterm)) {
        if (iterm->getSigType() == odb::dbSigType::GROUND) {
          return vss;
        }
        return vdd;
      }
    }
  }

  const sta::Cell* cell = network_->dbToSta(inst->getMaster());
  const sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  for (const sta::Sequential& seq : lib_cell->sequentials()) {
    // function
    if (sta::LibertyPort::equiv(lib_port, seq.output())) {
      return func;
    }
    // inverting function
    if (sta::LibertyPort::equiv(lib_port, seq.outputInv())) {
      return ifunc;
    }
  }

  log_->error(GPL, 9032, "Could not recognize port {}", lib_port->name());
  return d;
}

// basically a copy-paste of FuncExpr::equiv
bool MBFF::IsSame(const sta::FuncExpr* expr1,
                  dbInst* inst1,
                  const sta::FuncExpr* expr2,
                  dbInst* inst2)
{
  if (expr1 == nullptr && expr2 == nullptr) {
    return true;
  }
  if (expr1 && expr2 && expr1->op() == expr2->op()) {
    if (expr1->op() == sta::FuncExpr::Op::port) {
      return PortType(expr1->port(), inst1) == PortType(expr2->port(), inst2);
    }
    if (expr1->op() == sta::FuncExpr::Op::not_) {
      return IsSame(expr1->left(), inst1, expr2->left(), inst2);
    }
    return IsSame(expr1->left(), inst1, expr2->left(), inst2)
           && IsSame(expr1->right(), inst1, expr2->right(), inst2);
  }
  return false;
}

int MBFF::GetMatchingFunc(const sta::FuncExpr* expr,
                          dbInst* inst,
                          const bool create_new)
{
  for (size_t i = 0; i < funcs_.size(); i++) {
    if (IsSame(funcs_[i].first, funcs_[i].second, expr, inst)) {
      return i;
    }
  }
  if (create_new) {
    funcs_.emplace_back(expr, inst);
    return funcs_.size() - 1;
  }
  return -1;
}

MBFF::Mask MBFF::GetArrayMask(dbInst* inst, const bool isTray)
{
  Mask ret;
  ret.clock_polarity = network_->clockOn(inst);
  ret.has_clear = network_->hasClear(inst);
  ret.has_preset = network_->hasPreset(inst);

  // check the existance of Q / QN pins
  for (dbITerm* iterm : inst->getITerms()) {
    if (network_->isQPin(iterm)) {
      if (network_->isInvertingQPin(iterm)) {
        ret.inv_output = true;
      } else {
        ret.pos_output = true;
      }
    }
  }

  const sta::Cell* cell = network_->dbToSta(inst->getMaster());
  const sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  const auto& seqs = lib_cell->sequentials();
  if (!seqs.empty()) {
    ret.func_idx = GetMatchingFunc(seqs.front().data(), inst, isTray);
  }

  ret.is_scan_cell = network_->isScanCell(inst);

  return ret;
}

MBFF::DataToOutputsMap MBFF::GetPinMapping(dbInst* tray)
{
  const sta::Cell* cell = network_->dbToSta(tray->getMaster());
  const sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  sta::LibertyCellPortIterator port_itr(lib_cell);

  std::vector<const sta::LibertyPort*> d_pins;
  std::vector<const sta::LibertyPort*> q_pins;
  std::vector<const sta::LibertyPort*> qn_pins;

  // adds the input (D) pins
  for (const sta::Sequential& seq : lib_cell->sequentials()) {
    const sta::FuncExpr* data = seq.data();
    for (auto port : data->ports()) {
      if (!port->hasMembers()) {
        dbMTerm* mterm = network_->staToDb(port);
        if (mterm == nullptr) {
          continue;
        }
        dbITerm* iterm = tray->getITerm(mterm);
        if (iterm && network_->isDPin(iterm)) {
          d_pins.push_back(port);
        }
      }
    }
  }

  // all output pins are Q pins
  while (port_itr.hasNext()) {
    const sta::LibertyPort* port = port_itr.next();
    if (port->isBus() || port->isBundle()) {
      continue;
    }
    if (port->isPwrGnd() || port->isClock()) {
      continue;
    }
    if (port->direction()->isInput()) {
      continue;
    }
    if (port->direction()->isOutput()) {
      dbMTerm* mterm = network_->staToDb(port);
      dbITerm* iterm = tray->getITerm(mterm);
      if (network_->isInvertingQPin(iterm)) {
        qn_pins.push_back(port);
      } else {
        q_pins.push_back(port);
      }
    }
  }

  DataToOutputsMap ret;
  if (!q_pins.empty() && !qn_pins.empty()) {
    for (size_t i = 0; i < d_pins.size(); i++) {
      ret[d_pins[i]] = {q_pins[i], qn_pins[i]};
    }
  } else if (!q_pins.empty()) {
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
                                const Mask& array_mask)
{
  const int num_flops = flops.size();
  std::vector<std::pair<int, int>> new_mapping(mapping);

  std::map<int, int> old_to_new_idx;
  std::vector<dbInst*> tray_inst;
  for (int i = 0; i < num_flops; i++) {
    const int tray_idx = new_mapping[i].first;
    if (trays[tray_idx].slots.size() == 1) {
      new_mapping[i].first = std::numeric_limits<int>::max();
      continue;
    }
    if (!old_to_new_idx.count(tray_idx)) {
      old_to_new_idx[tray_idx] = tray_inst.size();

      std::string new_name = "_tray_size"
                             + std::to_string(trays[tray_idx].slots.size())
                             + "_" + std::to_string(unused_.back());
      unused_.pop_back();
      const int bit_idx = GetBitIdx(trays[tray_idx].slots.size());
      // SeparateFlops partitions by (parent module, mask), so flops in this
      // cluster share one parent module; place the new tray there.
      dbModule* parent = insts_[flops[i].idx]->getModule();
      auto new_tray = dbInst::create(block_,
                                     best_master_[array_mask][bit_idx],
                                     new_name.c_str(),
                                     /*physical_only=*/false,
                                     parent);
      const Point tray_center = GetTrayCenter(array_mask, bit_idx);
      new_tray->setLocation(
          multiplier_ * (trays[tray_idx].pt.x - tray_center.x),
          multiplier_ * (trays[tray_idx].pt.y - tray_center.y));
      new_tray->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      tray_inst.push_back(new_tray);
    }
    new_mapping[i].first = old_to_new_idx[tray_idx];
  }

  dbNet* clk_net = nullptr;
  odb::dbModNet* clk_mod_net = nullptr;
  for (int i = 0; i < num_flops; i++) {
    // single bit flop?
    if (new_mapping[i].first == std::numeric_limits<int>::max()) {
      continue;
    }

    const int tray_idx = new_mapping[i].first;
    const int tray_sz_idx = GetBitIdx(network_->getNumD(tray_inst[tray_idx]));
    const int slot_idx = new_mapping[i].second;

    // find the new port names
    const sta::LibertyPort* d_pin = nullptr;
    const sta::LibertyPort* q_pin = nullptr;
    const sta::LibertyPort* qn_pin = nullptr;

    int idx = 0;
    for (const auto& [d, outputs] : pin_mappings_[array_mask][tray_sz_idx]) {
      if (idx == slot_idx) {
        d_pin = d;
        q_pin = outputs.q;
        qn_pin = outputs.qn;
        break;
      }
      idx++;
    }

    dbITerm* scan_in = nullptr;
    dbITerm* scan_enable = nullptr;
    dbITerm* preset = nullptr;
    dbITerm* clear = nullptr;
    dbITerm* ground = nullptr;
    dbITerm* power = nullptr;

    for (dbITerm* iterm : tray_inst[tray_idx]->getITerms()) {
      if (network_->isPresetPin(iterm)) {
        preset = iterm;
      }
      if (network_->isClearPin(iterm)) {
        clear = iterm;
      }
      if (network_->isScanIn(iterm)) {
        scan_in = iterm;
      }
      if (network_->isScanEnable(iterm)) {
        scan_enable = iterm;
      }
      if (network_->isSupplyPin(iterm)) {
        if (iterm->getSigType() == odb::dbSigType::GROUND) {
          ground = iterm;
        } else {
          power = iterm;
        }
      }
    }

    // Classify each original iterm and record its tray-port mapping
    // *before* disconnecting, then disconnect/reconnect and store the
    // original pin name as a property on the tray instance.
    const std::string orig_inst_name(insts_[flops[i].idx]->getName());
    for (dbITerm* iterm : insts_[flops[i].idx]->getITerms()) {
      // Classify while the iterm is still connected.
      const bool is_d = network_->isDPin(iterm);
      const bool is_q = !is_d && network_->isQPin(iterm);
      const bool is_qn_inv = is_q && network_->isInvertingQPin(iterm);
      const std::string orig_port_name = iterm->getMTerm()->getName();

      // Capture both flat and hierarchical nets of the original iterm
      // before disconnecting, then rebind the matching tray iterm to
      // both. Preserves the hierarchical netlist after clustering.
      dbNet* net = iterm->getNet();
      odb::dbModNet* mod_net = iterm->getModNet();
      auto reconnect = [&](dbITerm* tray_iterm) {
        reconnectIterm(tray_iterm, net, mod_net);
      };
      if (net || mod_net) {
        iterm->disconnect();

        if (is_d) {
          reconnect(tray_inst[tray_idx]->findITerm(d_pin->name().c_str()));
        }
        if (is_q) {
          if (is_qn_inv) {
            reconnect(tray_inst[tray_idx]->findITerm(qn_pin->name().c_str()));
          } else {
            reconnect(tray_inst[tray_idx]->findITerm(q_pin->name().c_str()));
          }
        }
        if (network_->isSupplyPin(iterm)) {
          if (iterm->getSigType() == odb::dbSigType::GROUND) {
            if (ground) {
              reconnect(ground);
            }
          } else {
            if (power) {
              reconnect(power);
            }
          }
        }
        if (network_->isClockPin(iterm)) {
          // reconnect clock pins later (shared across the tray)
          clk_net = net;
          clk_mod_net = mod_net;
        }
        if (network_->isScanIn(iterm)) {
          reconnect(scan_in);
        }
        if (network_->isScanEnable(iterm)) {
          reconnect(scan_enable);
        }
        if (network_->isPresetPin(iterm)) {
          reconnect(preset);
        }
        if (network_->isClearPin(iterm)) {
          reconnect(clear);
        }
      }

      // Store original FF→tray pin mapping as a property on the tray
      // pin (iterm) so the report_path "orig_name" field can display the
      // original pin name.
      dbITerm* tray_iterm = nullptr;
      if (is_d && d_pin) {
        tray_iterm = tray_inst[tray_idx]->findITerm(d_pin->name().c_str());
      } else if (is_q) {
        const sta::LibertyPort* tray_port = is_qn_inv ? qn_pin : q_pin;
        if (tray_port) {
          tray_iterm
              = tray_inst[tray_idx]->findITerm(tray_port->name().c_str());
        }
      }
      if (tray_iterm) {
        const std::string val = orig_inst_name + "/" + orig_port_name;
        odb::dbStringProperty* prop
            = odb::dbStringProperty::find(tray_iterm, kOrigNameProp);
        if (prop) {
          prop->setValue(val.c_str());
        } else {
          odb::dbStringProperty::create(tray_iterm, kOrigNameProp, val.c_str());
        }
      }
    }
  }

  // all FFs in flops have the same block clock
  std::vector<bool> isConnected(tray_inst.size());
  for (int i = 0; i < num_flops; i++) {
    if (new_mapping[i].first != std::numeric_limits<int>::max()) {
      if (!isConnected[new_mapping[i].first]
          && (clk_net != nullptr || clk_mod_net != nullptr)) {
        for (dbITerm* iterm : tray_inst[new_mapping[i].first]->getITerms()) {
          if (network_->isClockPin(iterm)) {
            reconnectIterm(iterm, clk_net, clk_mod_net);
          }
        }
        isConnected[new_mapping[i].first] = true;
      }
      dbInst::destroy(insts_[flops[i].idx]);
    }
  }
}

float MBFF::RunLP(const std::vector<Flop>& flops,
                  std::vector<Tray>& trays,
                  const std::vector<std::pair<int, int>>& clusters)
{
  using operations_research::MPConstraint;
  using operations_research::MPSolver;
  using operations_research::MPVariable;

  const int num_flops = flops.size();
  const int num_trays = trays.size();

  std::unique_ptr<MPSolver> solver(MPSolver::CreateSolver("GLOP"));
  const float inf = solver->infinity();

  std::vector<MPVariable*> tray_x(num_trays);
  std::vector<MPVariable*> tray_y(num_trays);

  for (int i = 0; i < num_trays; i++) {
    tray_x[i] = solver->MakeNumVar(0, inf, "");
    tray_y[i] = solver->MakeNumVar(0, inf, "");
  }

  // displacement from flop to tray slot
  std::vector<MPVariable*> disp_x(num_flops);
  std::vector<MPVariable*> disp_y(num_flops);

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

    MPConstraint* c1 = solver->MakeRowConstraint(shift_x - flop.x, inf, "");
    c1->SetCoefficient(disp_x[i], 1);
    c1->SetCoefficient(tray_x[tray_idx], -1);

    MPConstraint* c2 = solver->MakeRowConstraint(flop.x - shift_x, inf, "");
    c2->SetCoefficient(disp_x[i], 1);
    c2->SetCoefficient(tray_x[tray_idx], 1);

    MPConstraint* c3 = solver->MakeRowConstraint(shift_y - flop.y, inf, "");
    c3->SetCoefficient(disp_y[i], 1);
    c3->SetCoefficient(tray_y[tray_idx], -1);

    MPConstraint* c4 = solver->MakeRowConstraint(flop.y - shift_y, inf, "");
    c4->SetCoefficient(disp_y[i], 1);
    c4->SetCoefficient(tray_y[tray_idx], 1);
  }

  operations_research::MPObjective* objective = solver->MutableObjective();
  for (int i = 0; i < num_flops; i++) {
    objective->SetCoefficient(disp_x[i], 1);
    objective->SetCoefficient(disp_y[i], 1);
  }
  objective->SetMinimization();
  const MPSolver::ResultStatus status = solver->Solve();
  if (status != MPSolver::OPTIMAL && status != MPSolver::FEASIBLE) {
    log_->error(GPL, 153, "ILP solver failed to find an solution solution!");
  }

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
                    const float alpha,
                    const float beta,
                    const Mask& array_mask)
{
  namespace sat = operations_research::sat;
  using operations_research::Domain;

  const int num_flops = flops.size();
  const int num_trays = trays.size();

  const int inf = std::numeric_limits<int>::max();

  // cand_tray[i] = tray indices that have a slot which contains flop i
  std::vector<std::vector<int>> cand_tray(num_flops);

  // cand_slot[i] = slot indices that are mapped to flop i
  std::vector<std::vector<int>> cand_slot(num_flops);

  for (int i = 0; i < num_trays; i++) {
    for (int j = 0; j < trays[i].slots.size(); j++) {
      if (trays[i].cand[j] >= 0) {
        cand_tray[trays[i].cand[j]].push_back(i);
        cand_slot[trays[i].cand[j]].push_back(j);
      }
    }
  }

  // has a flop been mapped to a slot?
  std::vector<std::vector<sat::BoolVar>> mapped(num_flops);

  // displacement from flop to slot
  std::vector<std::vector<sat::IntVar>> disp_x(num_flops);
  std::vector<std::vector<sat::IntVar>> disp_y(num_flops);

  /*
  NOTE: CP-SAT constraints only work with INTEGERS
  so, all coefficients (shift_x and shift_y) are multiplied by 100
  */
  sat::CpModelBuilder cp_model;

  for (int i = 0; i < num_flops; i++) {
    for (size_t j = 0; j < cand_tray[i].size(); j++) {
      disp_x[i].push_back(cp_model.NewIntVar(Domain(0, inf)));
      disp_y[i].push_back(cp_model.NewIntVar(Domain(0, inf)));
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

      const float dist = std::abs(shift_x) + std::abs(shift_y);
      max_dist = std::max(max_dist, dist);

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

  // make sure each timing-critical path contains FFs that can be clustered.
  std::map<int, int> old_to_new_idx;
  for (int i = 0; i < num_flops; i++) {
    old_to_new_idx[flops[i].idx] = i + 1;
  }

  int num_paths = 0;
  for (size_t i = 0; i < (int) flops.size(); i++) {
    for (const int j : paths_[flops[i].idx]) {
      if (!old_to_new_idx[j]) {
        continue;
      }
      num_paths++;
    }
  }

  std::vector<sat::IntVar> disp_path_x;
  std::vector<sat::IntVar> disp_path_y;
  for (int i = 0; i < num_paths; i++) {
    disp_path_x.push_back(cp_model.NewIntVar(Domain(0, inf)));
    disp_path_y.push_back(cp_model.NewIntVar(Domain(0, inf)));
  }

  int cur_path_num = 0;
  for (int i = 0; i < flops.size(); i++) {
    for (const int end_point : paths_[flops[i].idx]) {
      int flop_a_idx = old_to_new_idx[flops[i].idx];
      int flop_b_idx = old_to_new_idx[end_point];
      if (!flop_a_idx || !flop_b_idx) {
        continue;
      }
      flop_a_idx--;
      flop_b_idx--;
      sat::LinearExpr sum_disp_x_flop_a;
      sat::LinearExpr sum_disp_y_flop_a;

      sat::LinearExpr sum_disp_x_flop_b;
      sat::LinearExpr sum_disp_y_flop_b;

      for (int j = 0; j < cand_tray[flop_a_idx].size(); j++) {
        const float shift_x
            = trays[cand_tray[flop_a_idx][j]].slots[cand_slot[flop_a_idx][j]].x
              - flops[flop_a_idx].pt.x;
        const float shift_y
            = trays[cand_tray[flop_a_idx][j]].slots[cand_slot[flop_a_idx][j]].y
              - flops[flop_a_idx].pt.y;
        sum_disp_x_flop_a
            += (int(multiplier_ * shift_x) * mapped[flop_a_idx][j]);
        sum_disp_y_flop_a
            += (int(multiplier_ * shift_y) * mapped[flop_a_idx][j]);
      }

      for (int j = 0; j < cand_tray[flop_b_idx].size(); j++) {
        const float shift_x
            = trays[cand_tray[flop_b_idx][j]].slots[cand_slot[flop_b_idx][j]].x
              - flops[flop_b_idx].pt.x;
        const float shift_y
            = trays[cand_tray[flop_b_idx][j]].slots[cand_slot[flop_b_idx][j]].y
              - flops[flop_b_idx].pt.y;
        sum_disp_x_flop_b
            += (int(multiplier_ * shift_x) * mapped[flop_b_idx][j]);
        sum_disp_y_flop_b
            += (int(multiplier_ * shift_y) * mapped[flop_b_idx][j]);
      }

      cp_model.AddLessOrEqual(
          0, disp_path_x[cur_path_num] + sum_disp_x_flop_a - sum_disp_x_flop_b);
      cp_model.AddLessOrEqual(
          0, disp_path_x[cur_path_num] - sum_disp_x_flop_a + sum_disp_x_flop_b);
      cp_model.AddLessOrEqual(
          0, disp_path_y[cur_path_num] + sum_disp_y_flop_a - sum_disp_y_flop_b);
      cp_model.AddLessOrEqual(
          0,
          disp_path_y[cur_path_num++] - sum_disp_y_flop_a + sum_disp_y_flop_b);
    }
  }
  for (int i = 0; i < num_paths; i++) {
    cp_model.AddLessOrEqual(disp_path_x[i] + disp_path_y[i], max_dist);
  }

  // check that each flop is matched to a single slot
  for (int i = 0; i < num_flops; i++) {
    sat::LinearExpr mapped_flop;
    for (size_t j = 0; j < cand_tray[i].size(); j++) {
      mapped_flop += mapped[i][j];
    }
    cp_model.AddLessOrEqual(1, mapped_flop);
    cp_model.AddLessOrEqual(mapped_flop, 1);
  }

  std::vector<sat::BoolVar> tray_used(num_trays);
  for (int i = 0; i < num_trays; i++) {
    tray_used[i] = cp_model.NewBoolVar();
  }

  for (int i = 0; i < num_trays; i++) {
    std::vector<int> flop_ind;
    for (size_t j = 0; j < trays[i].slots.size(); j++) {
      if (trays[i].cand[j] >= 0) {
        flop_ind.push_back(trays[i].cand[j]);
      }
    }

    sat::LinearExpr slots_used;
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
        if (GetBitCnt(j) == trays[i].slots.size()) {
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

  sat::DoubleLinearExpr obj;

  // add the sum of all distances
  for (int i = 0; i < num_flops; i++) {
    for (size_t j = 0; j < cand_tray[i].size(); j++) {
      obj.AddTerm(disp_x[i][j], (1 / multiplier_));
      obj.AddTerm(disp_y[i][j], (1 / multiplier_));
    }
  }

  for (int i = 0; i < num_paths; i++) {
    obj.AddTerm(disp_path_x[i], (beta / multiplier_));
    obj.AddTerm(disp_path_y[i], (beta / multiplier_));
  }

  // add the tray usage constraints
  for (int i = 0; i < num_trays; i++) {
    obj.AddTerm(tray_used[i], alpha * tray_cost[i]);
  }

  cp_model.Minimize(obj);

  sat::Model model;
  sat::SatParameters parameters;
  model.Add(NewSatParameters(parameters));
  sat::CpSolverResponse response = sat::SolveCpModel(cp_model.Build(), &model);

  if (response.status() == sat::CpSolverStatus::FEASIBLE
      || response.status() == sat::CpSolverStatus::OPTIMAL) {
    const double ret = response.objective_value();
    std::set<std::pair<int, int>> trays_used;
    // update slot_disp_ vectors
    for (int i = 0; i < num_flops; i++) {
      for (size_t j = 0; j < cand_tray[i].size(); j++) {
        if (sat::SolutionBooleanValue(response, mapped[i][j]) == 1) {
          slot_disp_x_[flops[i].idx]
              = trays[cand_tray[i][j]].slots[cand_slot[i][j]].x - flops[i].pt.x;

          slot_disp_y_[flops[i].idx]
              = trays[cand_tray[i][j]].slots[cand_slot[i][j]].y - flops[i].pt.y;

          final_flop_to_slot[i] = {cand_tray[i][j], cand_slot[i][j]};
          trays_used.insert(
              {cand_tray[i][j], trays[cand_tray[i][j]].slots.size()});
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
                    const int bit_cnt,
                    std::vector<Point>& slots,
                    const Mask& array_mask)
{
  slots.clear();
  const int idx = GetBitIdx(bit_cnt);
  const Point center = GetTrayCenter(array_mask, idx);
  for (int i = 0; i < bit_cnt; i++) {
    slots.push_back({tray.x + slot_to_tray_x_[array_mask][idx][i] - center.x,
                     tray.y + slot_to_tray_y_[array_mask][idx][i] - center.y});
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
  const int num_flops = flops.size();

  /* pick a random flop */
  const int rand_idx = std::rand() % (num_flops);
  Tray tray_zero;
  tray_zero.pt = flops[rand_idx].pt;

  std::set<int> used_flops;
  used_flops.insert(flops[rand_idx].idx);
  trays.push_back(tray_zero);

  float tot_dist = 0;
  for (int i = 0; i < num_flops; i++) {
    const float contr = GetDistAR(flops[i].pt, tray_zero.pt, AR);
    flops[i].prob = contr;
    tot_dist += contr;
  }

  while (trays.size() < num_trays) {
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
      const float new_contr = GetDistAR(flops[i].pt, new_tray.pt, AR);
      flops[i].prob += new_contr;
      tot_dist += new_contr;
    }
  }
}

Tray MBFF::GetOneBit(const Point& pt)
{
  Tray tray;
  tray.pt = pt;
  tray.slots.push_back(pt);

  return tray;
}

void MBFF::MinCostFlow(const std::vector<Flop>& flops,
                       std::vector<Tray>& trays,
                       const int sz,
                       std::vector<std::pair<int, int>>& clusters)
{
  const int num_flops = flops.size();
  const int num_trays = trays.size();

  lemon::ListDigraph graph;
  std::vector<lemon::ListDigraph::Node> nodes;
  std::vector<lemon::ListDigraph::Arc> edges;
  std::vector<int> cur_costs, cur_caps;

  // add edges from source to flop
  lemon::ListDigraph::Node src = graph.addNode();
  lemon::ListDigraph::Node sink = graph.addNode();
  for (int i = 0; i < num_flops; i++) {
    lemon::ListDigraph::Node flop_node = graph.addNode();
    nodes.push_back(flop_node);

    lemon::ListDigraph::Arc src_to_flop = graph.addArc(src, flop_node);
    edges.push_back(src_to_flop);
    cur_costs.push_back(0);
    cur_caps.push_back(1);
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
        cur_costs.push_back(edge_cost);
        cur_caps.push_back(1);
      }

      // add edges from slot to sink
      lemon::ListDigraph::Arc slot_to_sink = graph.addArc(slot_node, sink);
      edges.push_back(slot_to_sink);
      cur_costs.push_back(0);
      cur_caps.push_back(1);

      slot_to_tray.emplace_back(i, j);
    }
  }

  // run min-cost flow
  lemon::ListDigraph::ArcMap<int> costs(graph);
  lemon::ListDigraph::ArcMap<int> caps(graph);
  lemon::ListDigraph::ArcMap<int> flow(graph);
  lemon::ListDigraph::NodeMap<int> labels(graph);
  lemon::NetworkSimplex<lemon::ListDigraph, int, int> new_graph(graph);

  for (size_t i = 0; i < edges.size(); i++) {
    costs[edges[i]] = cur_costs[i];
    caps[edges[i]] = cur_caps[i];
  }

  labels[src] = -1;
  for (size_t i = 0; i < nodes.size(); i++) {
    labels[nodes[i]] = i;
  }
  labels[sink] = nodes.size();

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
  const int num_flops = flops.size();
  const int num_trays = trays.size();

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
                                const Mask& array_mask)
{
  cluster.clear();
  const int num_flops = flops.size();
  const int num_trays = (num_flops + (sz - 1)) / sz;

  for (int i = 0; i < iter; i++) {
    MinCostFlow(flops, trays, sz, cluster);
    const float delta = RunLP(flops, trays, cluster);

    for (int j = 0; j < num_trays; j++) {
      GetSlots(trays[j].pt, sz, trays[j].slots, array_mask);
      for (int k = 0; k < sz; k++) {
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
    const Mask& array_mask)
{
  const int num_flops = flops.size();
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
    trays[0][i] = std::move(one_bit);
  }

  std::vector<std::vector<float>> res(num_sizes_);
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
        const int bit_cnt = GetBitCnt(i);
        const int num_trays = (num_flops + (GetBitCnt(i) - 1)) / GetBitCnt(i);

        for (int k = 0; k < num_trays; k++) {
          GetSlots(start_trays[i][j][k].pt,
                   bit_cnt,
                   start_trays[i][j][k].slots,
                   array_mask);
          start_trays[i][j][k].cand.reserve(bit_cnt);
          for (int idx = 0; idx < bit_cnt; idx++) {
            start_trays[i][j][k].cand.emplace_back(-1);
          }
        }
      }
    }
  }

  for (const auto& [bit_idx, tray_idx] : ind) {
    const int bit_cnt = GetBitCnt(bit_idx);

    std::vector<std::pair<int, int>> tmp_cluster;

    RunCapacitatedKMeans(flops,
                         start_trays[bit_idx][tray_idx],
                         bit_cnt,
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
                  const int knn,
                  std::vector<std::vector<Flop>>& clusters,
                  const std::vector<int>& rand_nums)
{
  const int num_flops = flops.size();

  // choose initial center
  int rand_ind = 0;
  const int seed = rand_nums[rand_ind++] % num_flops;
  std::set<int> chosen({seed});

  std::vector<Flop> centers({flops[seed]});

  std::vector<float> d(num_flops);
  for (int i = 0; i < num_flops; i++) {
    d[i] = GetDist(flops[i].pt, flops[seed].pt);
  }

  // choose remaining K-1 centers
  while (chosen.size() < knn) {
    float tot_sum = 0;

    for (int i = 0; i < num_flops; i++) {
      if (!chosen.count(i)) {
        for (int j : chosen) {
          d[i] = std::min(d[i], GetDist(flops[i].pt, flops[j].pt));
        }
        tot_sum += (float(d[i]) * float(d[i]));
      }
    }

    const int rnd = rand_nums[rand_ind++] % (int(tot_sum * 100));
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

  clusters.resize(knn);
  float prev = -1;
  while (true) {
    for (int i = 0; i < knn; i++) {
      clusters[i].clear();
    }

    // remap flops to clusters
    for (int i = 0; i < num_flops; i++) {
      float min_cost = std::numeric_limits<float>::max();
      int idx = 0;

      for (int j = 0; j < knn; j++) {
        const float dist = GetDist(flops[i].pt, centers[j].pt);
        if (dist < min_cost) {
          min_cost = dist;
          idx = j;
        }
      }
      clusters[idx].push_back(flops[i]);
    }

    // find new center locations
    absl::InlinedVector<int, 4> empty_clusters;
    for (int i = 0; i < knn; i++) {
      const int cur_sz = clusters[i].size();
      if (cur_sz == 0) {
        empty_clusters.push_back(i);
        continue;
      }
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

    if (!empty_clusters.empty()) {
      // To revive empty clusters and avoid division by zero, we re-seed their
      // centers with active flops that are currently furthest from their
      // assigned cluster centers. We use an inlined vector to track chosen
      // flops and prevent promoting the same flop to multiple empty clusters.
      absl::InlinedVector<int, 8> used_flops;
      for (int empty_idx : empty_clusters) {
        float max_dist = -1;
        Point best_pt = centers[empty_idx].pt;  // Fallback to previous center
        int best_idx = -1;

        for (int j = 0; j < knn; j++) {
          if (clusters[j].empty()) {
            continue;
          }
          for (const Flop& flop : clusters[j]) {
            if (std::find(used_flops.begin(), used_flops.end(), flop.idx)
                != used_flops.end()) {
              continue;
            }
            // Find the flop that has the worst-fit (largest displacement)
            // to its currently assigned center.
            const float dist = GetDist(flop.pt, centers[j].pt);
            if (dist > max_dist) {
              max_dist = dist;
              best_pt = flop.pt;
              best_idx = flop.idx;
            }
          }
        }

        if (best_idx != -1) {
          // Re-seeding the center directly onto the flop's coordinate.
          // This guarantees that in the next iteration, the distance from this
          // flop to this center is 0.0, forcing it to be assigned to this
          // cluster and keeping it active.
          centers[empty_idx].pt = best_pt;
          used_flops.push_back(best_idx);
        }
      }
    }

    // get total displacement
    float tot_disp = 0;
    for (int i = 0; i < knn; i++) {
      for (size_t j = 0; j < clusters[i].size(); j++) {
        tot_disp += GetDist(centers[i].pt, clusters[i][j].pt);
      }
    }

    if (std::abs(tot_disp - prev) <= 0.01f * prev) {
      break;
    }
    prev = tot_disp;
  }

  for (int i = 0; i < knn; i++) {
    clusters[i].push_back(centers[i]);
  }
}

float MBFF::GetKSilh(const std::vector<std::vector<Flop>>& clusters,
                     const std::vector<Point>& centers)
{
  const int num_centers = centers.size();
  int num_flops = 0;
  float tot = 0;

  for (int i = 0; i < num_centers; i++) {
    const int cur_sz = clusters[i].size();
    num_flops += cur_sz;
    if (cur_sz <= 1) {
      tot += -1;
      continue;
    }

    /* fast silh score */
    std::vector<float> all_x(cur_sz + 1, 0);
    std::vector<float> all_y(cur_sz + 1, 0);
    for (int j = 0; j < cur_sz; j++) {
      all_x[j + 1] = (clusters[i][j].pt.x);
      all_y[j + 1] = (clusters[i][j].pt.y);
    }
    std::sort(all_x.begin(), all_x.end());
    std::sort(all_y.begin(), all_y.end());

    std::vector<float> pref_x(cur_sz + 1, 0);
    std::vector<float> pref_y(cur_sz + 1, 0);
    for (int j = 1; j <= cur_sz; j++) {
      pref_x[j] = pref_x[j - 1] + all_x[j];
      pref_y[j] = pref_y[j - 1] + all_y[j];
    }

    for (int j = 0; j < cur_sz; j++) {
      float a_j = 0;
      float b_j = std::numeric_limits<float>::max();
      for (int k = 0; k < num_centers; k++) {
        if (i != k) {
          b_j = std::min(b_j, GetDist(clusters[i][j].pt, centers[k]));
        }
      }

      /* find first x_loc == current x */
      int lo = 1, hi = cur_sz, ret = std::numeric_limits<int>::max();
      while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (all_x[mid] >= clusters[i][j].pt.x) {
          ret = std::min(ret, mid);
          hi = mid - 1;
        } else {
          lo = mid + 1;
        }
      }

      /* get x contribution */
      float contribution_x = 0;
      if (ret > 1) {
        contribution_x += (clusters[i][j].pt.x * (ret - 1)) - (pref_x[ret - 1]);
      }
      if (ret < cur_sz) {
        contribution_x += (pref_x[cur_sz] - pref_x[ret])
                          - (cur_sz - ret) * (clusters[i][j].pt.x);
      }
      a_j += contribution_x;

      /* find first y_loc == current y */
      lo = 1, hi = cur_sz, ret = std::numeric_limits<int>::max();
      while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (all_y[mid] >= clusters[i][j].pt.y) {
          ret = std::min(ret, mid);
          hi = mid - 1;
        } else {
          lo = mid + 1;
        }
      }

      /* get y contribution */
      float contribution_y = 0;
      if (ret > 1) {
        contribution_y += (clusters[i][j].pt.y * (ret - 1)) - (pref_y[ret - 1]);
      }
      if (ret < cur_sz) {
        contribution_y += (pref_y[cur_sz] - pref_y[ret])
                          - (cur_sz - ret) * (clusters[i][j].pt.y);
      }
      a_j += contribution_y;

      a_j /= (cur_sz - 1);
      tot += ((b_j - a_j) / std::max(a_j, b_j));
    }
  }

  if (num_flops == 0) {
    return 0.0f;
  }
  return tot / num_flops;
}

void MBFF::KMeansDecomp(const std::vector<Flop>& flops,
                        const int max_sz,
                        std::vector<std::vector<Flop>>& pointsets)
{
  const int num_flops = flops.size();
  if (max_sz == -1 || num_flops <= max_sz) {
    pointsets.push_back(flops);
    return;
  }

  int best_k = 4;
  float best_silh = -20.00;
  std::vector<float> all_silhs(9);

  std::vector<std::vector<int>> rand_nums(multistart_ + 7);
  for (int i = 0; i < multistart_ + 7; i++) {
    for (int j = 0; j < 20; j++) {
      rand_nums[i].push_back(std::rand());
    }
  }

#pragma omp parallel for
  for (int k = 2; k <= 8; k++) {
    std::vector<std::vector<Flop>> k_clust;
    KMeans(flops, k, k_clust, rand_nums[k - 2]);
    std::vector<Point> centers;
    for (int i = 0; i < k; i++) {
      centers.push_back(k_clust[i].back().pt);
      k_clust[i].pop_back();
    }
    float cur_silh = GetKSilh(k_clust, centers);
    all_silhs[k] = cur_silh;
  }

  for (int i = 2; i <= 8; i++) {
    if (all_silhs[i] > best_silh) {
      best_silh = all_silhs[i];
      best_k = i;
    }
  }

  std::vector<std::vector<std::vector<Flop>>> tmp_clusters(multistart_);
  std::vector<float> tmp_costs(multistart_);

#pragma omp parallel for
  for (int i = 0; i < multistart_; i++) {
    KMeans(flops, best_k, tmp_clusters[i], rand_nums[i + 7]);

    /* cur_cost = sum of distances between flops and its
    matching cluster's center */
    float cur_cost = 0;
    for (int j = 0; j < best_k; j++) {
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
  for (int i = 0; i < best_k; i++) {
    for (int j = i + 1; j < best_k; j++) {
      const float dist
          = GetDist(k_means_ret[i].back().pt, k_means_ret[j].back().pt);
      cluster_pairs.emplace_back(dist, std::make_pair(i, j));
    }
  }
  std::sort(cluster_pairs.begin(), cluster_pairs.end());

  for (int i = 0; i < best_k; i++) {
    k_means_ret[i].pop_back();
  }

  // naive implementation of Disjoint Set Union
  std::vector<int> id(best_k);
  std::vector<int> sz(best_k);

  for (int i = 0; i < best_k; i++) {
    id[i] = i;
    sz[i] = k_means_ret[i].size();
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
    for (int j = 0; j < best_k; j++) {
      if (id[j] == orig_id) {
        id[j] = id[idx1];
      }
    }
  }

  std::vector<std::vector<Flop>> nxt_clusters(best_k);
  for (int i = 0; i < best_k; i++) {
    for (const Flop& f : k_means_ret[i]) {
      nxt_clusters[id[i]].push_back(f);
    }
  }

  // recurse on each new cluster
  for (int i = 0; i < best_k; i++) {
    if (!nxt_clusters[i].empty()) {
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
  for (int i = 0; i < flops_.size(); i++) {
    for (const int j : paths_[i]) {
      const float diff_x = slot_disp_x_[i] - slot_disp_x_[j];
      const float diff_y = slot_disp_y_[i] - slot_disp_y_[j];
      ret += std::abs(diff_x) + std::abs(diff_y);
    }
  }
  return ret;
}

void MBFF::displayFlopClusters(const char* stage,
                               std::vector<std::vector<Flop>>& clusters)
{
  if (graphics_ && graphics_->enabled()) {
    for (const std::vector<Flop>& cluster : clusters) {
      graphics_->status(fmt::format("{} size: {}", stage, cluster.size()));
      std::vector<odb::dbInst*> inst_cluster;
      inst_cluster.reserve(cluster.size());
      for (const Flop& flop : cluster) {
        inst_cluster.emplace_back(insts_[flop.idx]);
      }
      graphics_->mbffFlopClusters(inst_cluster);
    }
    graphics_->status("");
  }
}

float MBFF::RunClustering(const std::vector<Flop>& flops,
                          const int mx_sz,
                          const float alpha,
                          const float beta,
                          const Mask& array_mask)
{
  std::vector<std::vector<Flop>> pointsets;
  KMeansDecomp(flops, mx_sz, pointsets);

  displayFlopClusters("Point sets", pointsets);

  // all_start_trays[t][i][j]: start trays of size 2^i, multistart = j for
  // pointset[t]
  const int num_pointsets = pointsets.size();
  std::vector<std::vector<std::vector<std::vector<Tray>>>> all_start_trays(
      num_pointsets);
  for (int t = 0; t < num_pointsets; t++) {
    all_start_trays[t].resize(num_sizes_);
    for (int i = 1; i < num_sizes_; i++) {
      odb::dbMaster* master = best_master_[array_mask][i];
      if (master != nullptr) {
        const float aspect_ratio
            = master->getWidth() / static_cast<float>(master->getHeight());
        const int num_trays
            = (pointsets[t].size() + (GetBitCnt(i) - 1)) / GetBitCnt(i);
        all_start_trays[t][i].resize(5);
        for (int j = 0; j < 5; j++) {
          // running in parallel ==> not reproducible
          GetStartTrays(
              pointsets[t], num_trays, aspect_ratio, all_start_trays[t][i][j]);
        }
      }
    }
  }

  float ans = 0;
  std::vector<std::vector<std::pair<int, int>>> all_mappings(num_pointsets);
  std::vector<std::vector<Tray>> all_final_trays(num_pointsets);

#pragma omp parallel for num_threads(num_threads_)
  for (int t = 0; t < num_pointsets; t++) {
    std::vector<std::vector<Tray>> cur_trays;
    RunMultistart(cur_trays, pointsets[t], all_start_trays[t], array_mask);

    // run capacitated k-means per tray size
    const int num_flops = pointsets[t].size();
    for (int i = 1; i < num_sizes_; i++) {
      if (best_master_[array_mask][i] != nullptr) {
        const int bit_cnt = GetBitCnt(i);
        const int num_trays = (num_flops + (GetBitCnt(i) - 1)) / GetBitCnt(i);

        for (int j = 0; j < num_trays; j++) {
          GetSlots(
              cur_trays[i][j].pt, bit_cnt, cur_trays[i][j].slots, array_mask);
        }

        std::vector<std::pair<int, int>> cluster;
        RunCapacitatedKMeans(
            pointsets[t], cur_trays[i], GetBitCnt(i), 35, cluster, array_mask);
        MinCostFlow(pointsets[t], cur_trays[i], GetBitCnt(i), cluster);
        for (int j = 0; j < num_trays; j++) {
          GetSlots(
              cur_trays[i][j].pt, bit_cnt, cur_trays[i][j].slots, array_mask);
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
    const float cur_ans = RunILP(
        pointsets[t], all_final_trays[t], mapping, alpha, beta, array_mask);
    all_mappings[t] = std::move(mapping);
    ans += cur_ans;
  }

  for (int t = 0; t < num_pointsets; t++) {
    ModifyPinConnections(
        pointsets[t], all_final_trays[t], all_mappings[t], array_mask);
  }

  if (graphics_ && graphics_->enabled()) {
    AbstractGraphics::LineSegs segs;
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

    graphics_->mbffMapping(segs);
  }

  return ans;
}

float MBFF::getLeakage(odb::dbMaster* master)
{
  sta::Cell* cell = network_->dbToSta(master);
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  sta::LibertyCell* corner_cell
      = lib_cell->sceneCell(corner_, sta::MinMax::max());
  float cell_leakage;
  bool cell_leakage_exists;
  corner_cell->leakagePower(cell_leakage, cell_leakage_exists);
  if (cell_leakage_exists) {
    return cell_leakage;
  }

  // Look for unconditional power
  cell_leakage = 0;
  for (const sta::LeakagePower& leak : corner_cell->leakagePowers()) {
    if (leak.when()) {
      continue;
    }
    cell_leakage += leak.power();
  }

  // There should be a third method here of looking at conditional
  // power if unconditional isn't present.  However opensta doesn't
  // keep the related_pg_pin for leakage making it impossible to do
  // correctly.  If it did you would sum average power for each
  // pg_pin.

  if (cell_leakage == 0) {
    log_->warn(GPL, 327, "No leakage found for {}", master->getName());
  }

  return cell_leakage;
}

float MBFF::getInternalEnergy(odb::dbInst* inst)
{
  odb::dbMaster* master = inst->getMaster();
  sta::Cell* cell = network_->dbToSta(master);
  sta::LibertyCell* lib_cell = network_->libertyCell(cell);
  sta::LibertyCell* corner_cell
      = lib_cell->sceneCell(corner_, sta::MinMax::max());
  if (!corner_cell) {
    return 0.0;
  }

  // Sum average internal energy across all pins (CK, D, Q, SE, SI, ...).
  // For each pin, when conditions partition the input states; we average
  // across all groups (uniform duty assumption).  This captures the full
  // cell energy profile -- MBFF cells share SE/SI/CK structures across
  // bits, giving substantial savings that clock-pin-only analysis misses.
  float total_energy = 0.0;
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (network_->isSupplyPin(iterm)) {
      continue;
    }
    const sta::Pin* pin = network_->dbToSta(iterm);
    const sta::LibertyPort* port = network_->libertyPort(pin);
    if (!port) {
      continue;
    }
    const sta::LibertyPort* scene_port
        = port->scenePort(corner_, sta::MinMax::max());
    if (!scene_port) {
      continue;
    }
    float port_energy_sum = 0.0;
    int group_count = 0;
    for (const sta::InternalPower* pwr :
         corner_cell->internalPowers(scene_port)) {
      float energy = 0.0;
      int rf_count = 0;
      for (const sta::RiseFall* rf : sta::RiseFall::range()) {
        const sta::InternalPowerModel& model = pwr->model(rf);
        const sta::TableModel* tbl = model.model();
        if (!tbl) {
          continue;
        }
        float v1 = 0, v2 = 0;
        if (tbl->axis1()) {
          v1 = (tbl->axis1()->min() + tbl->axis1()->max()) / 2.0f;
        }
        if (tbl->axis2()) {
          v2 = (tbl->axis2()->min() + tbl->axis2()->max()) / 2.0f;
        }
        energy += tbl->findValue(v1, v2, 0.0f);
        rf_count++;
      }
      if (rf_count > 0) {
        port_energy_sum += energy / rf_count;
        group_count++;
      }
    }
    if (group_count > 0) {
      const float pin_energy = port_energy_sum / group_count;
      total_energy += pin_energy;
      debugPrint(log_,
                 GPL,
                 "mbff",
                 2,
                 "  pin {} groups={} energy={}",
                 port->name(),
                 group_count,
                 pin_energy);
    }
  }
  return total_energy;
}

float MBFF::clockActivity() const
{
  return (clock_period_ > 0) ? (2.0 / clock_period_) : 0.0;
}

float MBFF::getClockPeriod(odb::dbInst* ff_inst)
{
  float period = 0.0;
  for (odb::dbITerm* iterm : ff_inst->getITerms()) {
    if (network_->isClockPin(iterm)) {
      const sta::Pin* sta_pin = network_->dbToSta(iterm);
      for (const sta::Clock* clk : sta_->clocks(sta_pin, corner_->mode())) {
        if (period == 0.0 || clk->period() < period) {
          period = clk->period();
        }
      }
      break;
    }
  }
  return period;
}

std::vector<float> MBFF::precomputeClockPeriods(
    const std::vector<std::vector<Flop>>& FFs)
{
  std::vector<float> clock_periods(FFs.size());
  for (size_t i = 0; i < FFs.size(); i++) {
    dbInst* ff_inst = insts_[FFs[i].back().idx];
    clock_periods[i] = getClockPeriod(ff_inst);
  }
  return clock_periods;
}

void MBFF::SetVars(const std::vector<Flop>& flops)
{
  // get min height and width
  single_bit_height_ = std::numeric_limits<float>::max();
  single_bit_width_ = std::numeric_limits<float>::max();
  single_bit_power_ = std::numeric_limits<float>::max();
  const float activity = clockActivity();
  odb::PtrMap<dbMaster, float> energy_cache;
  for (const Flop& flop : flops) {
    dbMaster* master = insts_[flop.idx]->getMaster();
    single_bit_height_
        = std::min(single_bit_height_, master->getHeight() / multiplier_);
    single_bit_width_
        = std::min(single_bit_width_, master->getWidth() / multiplier_);
    auto [it, inserted] = energy_cache.try_emplace(master, 0.0f);
    if (inserted) {
      it->second = getInternalEnergy(insts_[flop.idx]);
    }
    const float leakage = getLeakage(master);
    const float total_power = leakage + it->second * activity;
    // Select the single-bit cell with lowest total estimated power as
    // the baseline.  Both leakage and internal energy must come from the
    // same cell to avoid an artificially low baseline.
    if (total_power < single_bit_power_) {
      single_bit_power_ = total_power;
      single_bit_master_ = master;
    }
  }
}

void MBFF::SetRatios(const Mask& array_mask)
{
  norm_area_.clear();
  norm_area_.push_back(1.00);
  norm_power_.clear();
  norm_power_.push_back(1.00);

  const float activity = clockActivity();

  debugPrint(log_,
             GPL,
             "mbff",
             1,
             "mask: {} sb_cell: {} sb_power: {} clock_period: {}",
             array_mask.to_string(),
             single_bit_master_ ? single_bit_master_->getName() : "none",
             single_bit_power_,
             clock_period_);

  for (int i = 1; i < num_sizes_; i++) {
    norm_area_.push_back(std::numeric_limits<float>::max());
    norm_power_.push_back(std::numeric_limits<float>::max());
    if (best_master_[array_mask][i]) {
      const int slot_cnt = GetBitCnt(i);
      norm_area_[i] = (tray_area_[array_mask][i]
                       / (single_bit_height_ * single_bit_width_))
                      / slot_cnt;
      if (single_bit_power_ > 0) {
        const float tray_total
            = tray_power_[array_mask][i]
              + tray_internal_energy_[array_mask][i] * activity;
        norm_power_[i] = (tray_total / slot_cnt) / single_bit_power_;
        debugPrint(log_,
                   GPL,
                   "mbff",
                   1,
                   "  {}-bit {}: tray_leakage: {} tray_internal_energy: {} "
                   "tray_total: {} norm_power: {}",
                   slot_cnt,
                   best_master_[array_mask][i]->getName(),
                   tray_power_[array_mask][i],
                   tray_internal_energy_[array_mask][i],
                   tray_total,
                   norm_power_[i]);
      }
    }
  }
}

void MBFF::SeparateFlops(std::vector<std::vector<Flop>>& ffs)
{
  // group by block clock name
  odb::PtrMap<odb::dbNet, std::vector<int>> clk_terms;
  for (size_t i = 0; i < flops_.size(); i++) {
    if (insts_[i]->isDoNotTouch()) {
      continue;
    }
    for (dbITerm* iterm : insts_[i]->getITerms()) {
      if (network_->isClockPin(iterm)) {
        dbNet* net = iterm->getNet();
        if (!net) {
          continue;
        }
        clk_terms[net].push_back(i);
      }
    }
  }

  // Order modules by id, not pointer value — pointer order varies
  // across runs and would make tray naming / ILP seed consumption
  // non-deterministic.
  struct ModMaskLess
  {
    bool operator()(const std::pair<dbModule*, Mask>& a,
                    const std::pair<dbModule*, Mask>& b) const
    {
      if (a.first != b.first) {
        return odb::compare_by_id(a.first, b.first);
      }
      return a.second < b.second;
    }
  };

  for (const auto& [clk_net, indices] : clk_terms) {
    // Partition by (parent module, mask) so flops in different
    // hierarchical modules are never clustered into the same tray.
    std::map<std::pair<dbModule*, Mask>, std::vector<Flop>, ModMaskLess>
        flops_by_mod_mask;
    for (const int idx : indices) {
      const Mask vec_mask = GetArrayMask(insts_[idx], false);
      flops_by_mod_mask[{insts_[idx]->getModule(), vec_mask}].push_back(
          flops_[idx]);
    }

    for (const auto& [key, flops] : flops_by_mod_mask) {
      if (!flops.empty()) {
        ffs.push_back(flops);
        debugPrint(log_,
                   GPL,
                   "mbff",
                   1,
                   "Flop cluster for net {} in module {} with mask {} of "
                   "size {}",
                   clk_net->getName(),
                   key.first ? key.first->getHierarchicalName() : "<top>",
                   key.second.to_string(),
                   flops.size());
      }
    }
  }

  displayFlopClusters("SeparateFlops", ffs);
}

void MBFF::SetTrayNames()
{
  for (size_t i = 0; i < 2 * flops_.size(); i++) {
    unused_.push_back(i);
  }
}

void MBFF::Run(const int mx_sz, const float alpha, const float beta)
{
  std::srand(1);
  omp_set_num_threads(num_threads_);

  ReadFFs();
  ReadPaths();
  ReadLibs();
  SetTrayNames();

  std::vector<std::vector<Flop>> FFs;
  SeparateFlops(FFs);
  const std::vector<float> clock_periods = precomputeClockPeriods(FFs);
  const int num_chunks = FFs.size();
  float tot_ilp = 0;
  bool any_found = false;
  for (int i = 0; i < num_chunks; i++) {
    dbInst* ff_inst = insts_[FFs[i].back().idx];
    const Mask array_mask = GetArrayMask(ff_inst, false);
    // do we even have tray candidates to cluster these flops?
    if (!tray_candidates_.contains(array_mask)) {
      tot_ilp += (alpha * FFs[i].size());
      tray_sizes_used_[1] += FFs[i].size();
      log_->info(GPL,
                 137,
                 "No tray found for group of {} flop instances containing {}",
                 FFs[i].size(),
                 ff_inst->getMaster()->getName());
      continue;
    }
    any_found = true;
    clock_period_ = clock_periods[i];
    SelectBestTrays(array_mask, clockActivity());
    SetVars(FFs[i]);
    SetRatios(array_mask);
    tot_ilp += RunClustering(FFs[i], mx_sz, alpha, beta, array_mask);
  }

  // delete test_trays
  for (int i = 0; i < test_idx_; i++) {
    const std::string test_tray_name = "test_tray_" + std::to_string(i);
    dbInst* inst = block_->findInst(test_tray_name.c_str());
    dbInst::destroy(inst);
  }

  if (!any_found) {
    log_->warn(GPL, 138, "No clusterable flops found");
    return;
  }

  const float tcp_disp = (beta * GetPairDisplacements());

  int num_paths = 0;
  for (const std::vector<int>& path : paths_) {
    num_paths += path.size();
  }

  // calculate average slot-to-flop displacement
  float avg_disp = 0;
  for (size_t i = 0; i < flops_.size(); i++) {
    const float dX = (slot_disp_x_[i]);
    const float dY = (slot_disp_y_[i]);
    avg_disp += (std::max(dX, -dX) + std::max(dY, -dY));
  }
  avg_disp /= flops_.size();

  log_->report("Alpha = {:.1f}, Beta = {:.1f}, #paths = {}, max size = {}",
               alpha,
               beta,
               num_paths,
               mx_sz);
  log_->report("Total ILP Cost: {:.3f}", tot_ilp);
  log_->report("Total Timing Critical Path Displacement: {:.1f}", tcp_disp);
  log_->report("Average slot-to-flop displacement: {:.3f}", avg_disp);
  log_->report("Final Objective Value: {:.3f}", tot_ilp + tcp_disp);
  log_->report("Sizes used");
  for (const auto [tray, count] : tray_sizes_used_) {
    log_->report("  {}-bit: {}", tray, count);
  }
}

Point MBFF::GetTrayCenter(const Mask& array_mask, const int idx)
{
  const int num_slots = GetBitCnt(idx);
  std::vector<float> all_x;
  std::vector<float> all_y;
  for (int i = 0; i < num_slots; i++) {
    all_x.push_back(slot_to_tray_x_[array_mask][idx][i]);
    all_y.push_back(slot_to_tray_y_[array_mask][idx][i]);
  }
  std::sort(all_x.begin(), all_x.end());
  std::sort(all_y.begin(), all_y.end());
  const float tray_center_x = (all_x[0] + all_x[num_slots - 1]) / 2.0;
  const float tray_center_y = (all_y[0] + all_y[num_slots - 1]) / 2.0;
  return Point{tray_center_x, tray_center_y};
}

void MBFF::ReadLibs()
{
  test_idx_ = 0;
  for (odb::dbLib* lib : db_->getLibs()) {
    for (dbMaster* master : lib->getMasters()) {
      const std::string tray_name = "test_tray_" + std::to_string(test_idx_++);
      dbInst* tmp_tray = dbInst::create(block_, master, tray_name.c_str());

      if (!network_->isValidTray(tmp_tray)) {
        dbInst::destroy(tmp_tray);
        --test_idx_;
        continue;
      }

      const int num_slots = network_->getNumD(tmp_tray);
      if ((num_slots & (num_slots - 1)) != 0) {
        continue;  // non-power of 2 not supported
      }
      const int idx = GetBitIdx(num_slots);
      const Mask array_mask = GetArrayMask(tmp_tray, true);

      if (tray_candidates_[array_mask].empty()) {
        tray_candidates_[array_mask].resize(num_sizes_);
      }

      const float cur_area = (master->getHeight() / multiplier_)
                             * (master->getWidth() / multiplier_);
      const float cur_leakage = getLeakage(tmp_tray->getMaster());
      const float cur_internal_energy = getInternalEnergy(tmp_tray);

      debugPrint(log_,
                 GPL,
                 "mbff",
                 1,
                 "Found tray {} mask: {} area: {} leakage: {} "
                 "internal_energy: {}",
                 master->getName(),
                 array_mask.to_string(),
                 cur_area,
                 cur_leakage,
                 cur_internal_energy);

      // Collect slot geometry from the temporary instance.
      tmp_tray->setLocation(0, 0);
      tmp_tray->setPlacementStatus(odb::dbPlacementStatus::PLACED);

      DataToOutputsMap pin_mapping = GetPinMapping(tmp_tray);

      std::vector<Point> d;
      std::vector<Point> q;
      std::vector<Point> qn;

      for (const auto& p : pin_mapping) {
        dbITerm* d_pin = tmp_tray->findITerm(p.first->name().c_str());
        dbITerm* q_pin
            = (p.second.q ? tmp_tray->findITerm(p.second.q->name().c_str())
                          : nullptr);
        dbITerm* qn_pin
            = (p.second.qn ? tmp_tray->findITerm(p.second.qn->name().c_str())
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

      std::vector<float> slot_x;
      std::vector<float> slot_y;
      for (int i = 0; i < num_slots; i++) {
        if (!q.empty() && !qn.empty()) {
          slot_x.push_back((std::max(d[i].x, std::max(q[i].x, qn[i].x))
                            + std::min(d[i].x, std::min(q[i].x, qn[i].x)))
                           / 2.0);
          slot_y.push_back((std::max(d[i].y, std::max(q[i].y, qn[i].y))
                            + std::min(d[i].y, std::min(q[i].y, qn[i].y)))
                           / 2.0);
        } else if (!q.empty()) {
          slot_x.push_back((d[i].x + q[i].x) / 2.0);
          slot_y.push_back((d[i].y + q[i].y) / 2.0);
        } else {
          slot_x.push_back((d[i].x + qn[i].x) / 2.0);
          slot_y.push_back((d[i].y + qn[i].y) / 2.0);
        }
      }

      tray_candidates_[array_mask][idx].push_back(
          TrayCandidate{master,
                        cur_area,
                        cur_leakage,
                        cur_internal_energy,
                        master->getWidth() / multiplier_,
                        std::move(pin_mapping),
                        std::move(slot_x),
                        std::move(slot_y)});
    }
  }
}

void MBFF::SelectBestTrays(const Mask& mask, const float activity)
{
  auto it = tray_candidates_.find(mask);
  if (it == tray_candidates_.end()) {
    return;
  }
  const auto& candidates_per_size = it->second;

  best_master_[mask].assign(num_sizes_, nullptr);
  tray_area_[mask].assign(num_sizes_, std::numeric_limits<float>::max());
  tray_power_[mask].assign(num_sizes_, std::numeric_limits<float>::max());
  tray_internal_energy_[mask].assign(num_sizes_, 0.0);
  tray_width_[mask].assign(num_sizes_, 0.0f);
  pin_mappings_[mask].assign(num_sizes_, DataToOutputsMap{});
  slot_to_tray_x_[mask].assign(num_sizes_, {});
  slot_to_tray_y_[mask].assign(num_sizes_, {});

  for (int idx = 0; idx < num_sizes_; idx++) {
    const TrayCandidate* best = nullptr;
    float best_total_power = std::numeric_limits<float>::max();
    float best_area = std::numeric_limits<float>::max();
    for (const TrayCandidate& cand : candidates_per_size[idx]) {
      const float cur_total_power
          = cand.leakage + cand.internal_energy * activity;
      if (std::tie(best_total_power, best_area)
          > std::tie(cur_total_power, cand.area)) {
        best = &cand;
        best_total_power = cur_total_power;
        best_area = cand.area;
      }
    }
    if (best) {
      best_master_[mask][idx] = best->master;
      tray_area_[mask][idx] = best->area;
      tray_power_[mask][idx] = best->leakage;
      tray_internal_energy_[mask][idx] = best->internal_energy;
      tray_width_[mask][idx] = best->width;
      pin_mappings_[mask][idx] = best->pin_mapping;
      slot_to_tray_x_[mask][idx] = best->slot_x;
      slot_to_tray_y_[mask][idx] = best->slot_y;
    }
  }
}

void MBFF::ReadFFs()
{
  int num_flops = 0;
  for (dbInst* inst : block_->getInsts()) {
    if (network_->isValidFlop(inst)) {
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
  std::vector<std::set<int>> unique(flops_.size());

  sta::ExceptionFrom* e_from = nullptr;
  sta::ExceptionThruSeq* e_thrus = nullptr;
  sta::ExceptionTo* e_to = nullptr;

  sta_->ensureGraph();
  sta_->searchPreamble();
  sta_->ensureLevelized();
  sta::Search* search = sta_->search();
  sta::StringSeq empty_group_names;
  sta::PathEndSeq path_ends = search->findPathEnds(e_from,
                                                   e_thrus,
                                                   e_to,
                                                   false,
                                                   sta_->scenes(),
                                                   sta::MinMaxAll::max(),
                                                   20,
                                                   num_paths_,
                                                   true,
                                                   true,
                                                   -sta::INF,
                                                   sta::INF,
                                                   true,
                                                   empty_group_names,
                                                   true,
                                                   false,
                                                   false,
                                                   false,
                                                   false,
                                                   false);

  for (sta::PathEnd* path_end : path_ends) {
    sta::Path* path = path_end->path();
    sta::PathExpanded expanded(path, sta_);
    const sta::Path* pathref_front = expanded.path(expanded.startIndex());
    sta::Vertex* pathvertex_front = pathref_front->vertex(sta_);
    sta::Pin* pathpin_front = pathvertex_front->pin();

    const sta::Path* pathref_back = expanded.path(expanded.size() - 1);
    sta::Vertex* pathvertex_back = pathref_back->vertex(sta_);
    sta::Pin* pathpin_back = pathvertex_back->pin();

    sta::Instance* start_ff = network_->instance(pathpin_front);
    sta::Instance* end_ff = network_->instance(pathpin_back);

    auto it1 = name_to_idx_.find(network_->pathName(start_ff));
    if (it1 == name_to_idx_.end()) {
      continue;
    }
    const int idx1 = it1->second;
    auto it2 = name_to_idx_.find(network_->pathName(end_ff));
    if (it2 == name_to_idx_.end()) {
      continue;
    }
    const int idx2 = it2->second;

    // ensure that paths are unique and start != end
    if (idx1 == idx2 || unique[idx1].count(idx2)) {
      continue;
    }
    paths_[idx1].push_back(idx2);
    unique[idx1].insert(idx2);
  }
}

MBFF::MBFF(odb::dbDatabase* db,
           sta::dbSta* sta,
           utl::Logger* log,
           rsz::Resizer* resizer,
           const int threads,
           const int multistart,
           const int num_paths,
           const bool debug_graphics,
           std::unique_ptr<AbstractGraphics> graphics)
    : db_(db),
      block_(db_->getChip()->getBlock()),
      sta_(sta),
      network_(sta_->getDbNetwork()),
      corner_(sta_->cmdScene()),
      graphics_(std::move(graphics)),
      log_(log),
      resizer_(resizer),
      num_threads_(threads),
      multistart_(multistart),
      num_paths_(num_paths),
      multiplier_(block_->getDbUnitsPerMicron()),
      single_bit_height_(0.0),
      single_bit_width_(0.0),
      single_bit_power_(0.0),
      clock_period_(0.0),
      single_bit_master_(nullptr),
      test_idx_(-1)
{
  graphics_->setDebugOn(debug_graphics);
}

MBFF::~MBFF() = default;

}  // end namespace gpl
