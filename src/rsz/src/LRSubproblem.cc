// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "LRSubproblem.hh"

#include <algorithm>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/LibertyClass.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "sta/Scene.hh"
#include "sta/Sta.hh"
#include "sta/TimingRole.hh"
#include "sta/Transition.hh"

namespace rsz {

namespace {

// Resizer::area(Cell*) is protected. Compute the same value through the public
// dbuToMeters + db_network->staToDb pair so we don't friend-pierce the Resizer
// class. Matches Resizer::area(dbMaster*) exactly.
double cellAreaSI(const Resizer& resizer,
                  sta::dbNetwork* db_network,
                  sta::LibertyCell* cell)
{
  if (cell == nullptr) {
    return 0.0;
  }
  odb::dbMaster* master = db_network->staToDb(db_network->cell(cell));
  if (master == nullptr || !master->isCoreAutoPlaceable()) {
    return 0.0;
  }
  return resizer.dbuToMeters(master->getWidth())
         * resizer.dbuToMeters(master->getHeight());
}

// File-local output-side DRC helpers. Mirrors the pattern used by
// SizeDownGenerator.cc where similar checks live as file-local statics.
// Polarity: return true when the proposed replacement would introduce a
// violation. Both are pure Liberty/SDC reads and so are safe to call from
// worker threads.

bool checkOutputMaxCap(sta::LibertyPort* output_port,
                       const float output_cap,
                       const sta::MinMax* max_mm)
{
  float max_cap = 0.0f;
  bool cap_limit_exists = false;
  output_port->capacitanceLimit(max_mm, max_cap, cap_limit_exists);
  return cap_limit_exists && max_cap > 0.0f && output_cap > max_cap;
}

bool checkOutputMaxSlew(sta::dbSta* sta,
                        sta::LibertyPort* candidate_port,
                        const float output_slew_factor,
                        const float output_cap,
                        const sta::Scene* scene,
                        const sta::MinMax* max_mm)
{
  const float new_slew
      = output_slew_factor * candidate_port->driveResistance() * output_cap;
  float max_slew = 0.0f;
  bool slew_limit_exists = false;
  sta->findSlewLimit(
      candidate_port, scene, max_mm, max_slew, slew_limit_exists);
  return slew_limit_exists && new_slew > max_slew;
}

}  // namespace

LRSubproblem::LRSubproblem(Resizer* resizer) : resizer_(resizer)
{
}

void LRSubproblem::init()
{
  if (initialized_) {
    return;
  }
  logger_ = resizer_->logger();
  dbStaState::init(resizer_->sta());
  db_network_ = resizer_->dbNetwork();
  computeLeakageScale();
  initialized_ = true;
}

void LRSubproblem::computeLeakageScale()
{
  // Build (leakage, area) pairs for instances whose current cell has both.
  std::vector<float> leakages;
  std::vector<float> areas;
  std::unique_ptr<sta::LeafInstanceIterator> iit(
      network_->leafInstanceIterator());
  while (iit->hasNext()) {
    sta::Instance* inst = iit->next();
    sta::LibertyCell* cell = network_->libertyCell(inst);
    if (cell == nullptr) {
      continue;
    }
    const std::optional<float> leak = resizer_->cellLeakage(cell);
    if (!leak.has_value()) {
      continue;
    }
    const double a = cellAreaSI(*resizer_, db_network_, cell);
    if (a <= 0.0) {
      continue;
    }
    leakages.push_back(*leak);
    areas.push_back(static_cast<float>(a));
  }

  if (leakages.empty()) {
    // Degenerate: no instance exposes leakage. leakageOrArea will return
    // raw area, which is order-preserving within this design.
    area_to_leakage_scale_ = 0.0f;
    return;
  }

  const auto mid = leakages.size() / 2;
  std::nth_element(leakages.begin(), leakages.begin() + mid, leakages.end());
  const float l_med = leakages[mid];
  std::nth_element(areas.begin(), areas.begin() + mid, areas.end());
  const float a_med = areas[mid];

  area_to_leakage_scale_ = (a_med > 0.0f) ? (l_med / a_med) : 0.0f;
}

float LRSubproblem::leakageOrArea(sta::LibertyCell* cell) const
{
  const std::optional<float> leak = resizer_->cellLeakage(cell);
  if (leak.has_value()) {
    return *leak;
  }
  const float a = static_cast<float>(cellAreaSI(*resizer_, db_network_, cell));
  return area_to_leakage_scale_ > 0.0f ? area_to_leakage_scale_ * a : a;
}

bool LRSubproblem::isDataArc(const sta::Edge* edge) const
{
  const sta::TimingRole* role = edge->role();
  if (role->isTimingCheck()) {
    return false;
  }
  if (edge->isDisabledLoop()) {
    return false;
  }
  if (role == sta::TimingRole::latchDtoQ()
      || role == sta::TimingRole::latchEnToQ()) {
    return false;
  }
  return true;
}

float LRSubproblem::portInputCap(sta::LibertyCell* cell,
                                 const char* port_name) const
{
  sta::LibertyPort* port = cell->findLibertyPort(port_name);
  if (port == nullptr) {
    return 0.0f;
  }
  float cap = 0.0f;
  for (auto rf : sta::RiseFall::range()) {
    cap = std::max(cap, port->capacitance(rf, max_));
  }
  return cap;
}

bool LRSubproblem::applyReplacement(sta::Instance* inst,
                                    sta::LibertyCell* replacement)
{
  if (inst == nullptr || replacement == nullptr) {
    return false;
  }
  return resizer_->replaceCell(inst, replacement, /*journal=*/true);
}

bool LRSubproblem::snapshot(sta::Instance* inst,
                            const float* lambda,
                            const int lambda_size,
                            GateSnapshot& snap)
{
  init();

  if (resizer_->dontTouch(inst)) {
    return false;
  }
  sta::LibertyCell* cur_cell = network_->libertyCell(inst);
  if (cur_cell == nullptr) {
    return false;
  }

  const sta::Scene* scene = sta_->cmdScene();
  const sta::MinMax* max_mm = max_;

  snap.inst = inst;
  snap.cur_cell = cur_cell;
  snap.scene = scene;
  snap.outputs.clear();
  snap.upstream.clear();
  snap.inputs.clear();
  snap.candidates.clear();

  std::unique_ptr<sta::InstancePinIterator> pit(network_->pinIterator(inst));
  while (pit->hasNext()) {
    sta::Pin* pin = pit->next();
    const sta::PortDirection* dir = network_->direction(pin);
    if (dir->isOutput()) {
      sta::Vertex* v = graph_->pinDrvrVertex(pin);
      if (v == nullptr) {
        continue;
      }
      const sta::LibertyPort* out_port = network_->libertyPort(pin);
      if (out_port == nullptr) {
        continue;
      }
      float lam_sum = 0.0f;
      sta::VertexInEdgeIterator ieit(v, graph_);
      while (ieit.hasNext()) {
        sta::Edge* e = ieit.next();
        if (!isDataArc(e)) {
          continue;
        }
        // Restrict to gate-internal arcs (from a pin on the same instance).
        const sta::Pin* from_pin = e->from(graph_)->pin();
        if (network_->instance(from_pin) != inst) {
          continue;
        }
        const sta::EdgeId id = graph_->id(e);
        if (static_cast<int>(id) >= lambda_size) {
          continue;
        }
        lam_sum += lambda[id];
      }
      OutputCtx o;
      o.port = out_port;
      o.load_cap = graph_delay_calc_->loadCap(pin, scene, max_mm);
      o.lambda_sum = lam_sum;
      // Freeze the Elmore-slew DRC inputs. slew is the STA graph slew at the
      // output pin's load vertex; it is constant across candidates, so we read
      // it once here on the main thread.
      sta::Vertex* load_v = graph_->pinLoadVertex(pin);
      o.slew = (load_v != nullptr)
                   ? sta::delayAsFloat(sta_->slew(load_v,
                                                  sta::RiseFallBoth::riseFall(),
                                                  sta_->scenes(),
                                                  max_mm))
                   : 0.0f;
      o.drive_res = out_port->driveResistance();
      snap.outputs.push_back(o);
    } else if (dir->isInput()) {
      const sta::LibertyPort* in_port = network_->libertyPort(pin);

      // (a) Input-side max-cap DRC context for every input pin: freeze each
      // fanin driver's current cap-check so workers can replay
      // Resizer::replacementPreservesMaxCap without touching live STA.
      if (in_port != nullptr) {
        sta::PinSet* drivers = network_->drivers(pin);
        if (drivers != nullptr) {
          InputMaxCapCtx in_ctx;
          in_ctx.in_port = in_port;
          in_ctx.old_cap = portInputCap(cur_cell, in_port->name().c_str());
          for (const sta::Pin* driver_pin : *drivers) {
            float cap = 0.0f;
            float max_cap = 0.0f;
            float cap_slack = 0.0f;
            const sta::RiseFall* tr = nullptr;
            const sta::Scene* corner = nullptr;
            sta_->checkCapacitance(driver_pin,
                                   sta_->scenes(),
                                   max_mm,
                                   cap,
                                   max_cap,
                                   cap_slack,
                                   tr,
                                   corner);
            DriverCapCheck dc;
            dc.cap = cap;
            dc.max_cap = max_cap;
            dc.cap_slack = cap_slack;
            dc.corner_ok = (max_cap > 0.0f && corner != nullptr);
            in_ctx.drivers.push_back(dc);
          }
          snap.inputs.push_back(std::move(in_ctx));
        }
      }

      // (b) Upstream-Cin context: only input pins with real upstream pressure.
      sta::Vertex* in_v = graph_->pinLoadVertex(pin);
      if (in_v == nullptr) {
        continue;
      }
      // Locate the driver pin via the wire arc(s) feeding in_v. There's
      // typically exactly one; take the first valid one.
      sta::Pin* drv_pin = nullptr;
      sta::VertexInEdgeIterator wireIt(in_v, graph_);
      while (wireIt.hasNext()) {
        sta::Edge* w = wireIt.next();
        if (w->isDisabledLoop()) {
          continue;
        }
        sta::Pin* candidate_drv = w->from(graph_)->pin();
        if (candidate_drv != nullptr && candidate_drv != pin) {
          drv_pin = candidate_drv;
          break;
        }
      }
      if (drv_pin == nullptr) {
        continue;  // floating / no driver
      }
      sta::Instance* upstream_inst = network_->instance(drv_pin);
      if (upstream_inst == nullptr || upstream_inst == inst) {
        continue;
      }
      sta::LibertyCell* upstream_cell = network_->libertyCell(upstream_inst);
      if (upstream_cell == nullptr) {
        // PI / hierarchical / black box - no Liberty model to evaluate.
        continue;
      }
      sta::LibertyPort* drv_port = network_->libertyPort(drv_pin);
      if (drv_port == nullptr) {
        continue;
      }
      sta::Vertex* drv_v = graph_->pinDrvrVertex(drv_pin);
      if (drv_v == nullptr) {
        continue;
      }
      // Sum λ over U's gate-internal data arcs terminating at drv_pin.
      float lam_U = 0.0f;
      sta::VertexInEdgeIterator drvIt(drv_v, graph_);
      while (drvIt.hasNext()) {
        sta::Edge* e = drvIt.next();
        if (!isDataArc(e)) {
          continue;
        }
        const sta::Pin* from_pin = e->from(graph_)->pin();
        if (network_->instance(from_pin) != upstream_inst) {
          continue;
        }
        const sta::EdgeId id = graph_->id(e);
        if (static_cast<int>(id) >= lambda_size) {
          continue;
        }
        lam_U += lambda[id];
      }
      // Skip pins with no real upstream pressure - saves the per-candidate
      // gateDelay call for arcs whose λ is essentially at floor anyway.
      if (lam_U <= 0.0f) {
        continue;
      }
      if (in_port == nullptr) {
        continue;
      }
      UpstreamCtx u;
      u.orig_in_port = in_port;
      u.drv_port = drv_port;
      u.load_U_cur = graph_delay_calc_->loadCap(drv_pin, scene, max_mm);
      u.c_in_cur = portInputCap(cur_cell, in_port->name().c_str());
      u.lambda_U_drv = lam_U;
      snap.upstream.push_back(u);
    }
  }

  if (snap.outputs.empty()) {
    return false;
  }

  // Precompute leakage-equivalent cost for the current cell and every
  // candidate now, on the main thread - leakageOrArea/getSwappableCells mutate
  // lazy caches and must not be touched from workers.
  snap.cur_leakage = leakageOrArea(cur_cell);
  sta::LibertyCellSeq candidates = resizer_->getSwappableCells(cur_cell);
  snap.candidates.reserve(candidates.size());
  for (sta::LibertyCell* cand : candidates) {
    if (cand == cur_cell) {
      continue;
    }
    Candidate c;
    c.cell = cand;
    c.leakage = leakageOrArea(cand);
    snap.candidates.push_back(c);
  }

  return true;
}

float LRSubproblem::evaluateCellCost(const GateSnapshot& snap,
                                     sta::LibertyCell* cell,
                                     const float cell_leakage,
                                     const float timing_weight,
                                     sta::ArcDelayCalc* arc_delay_calc) const
{
  float cost = cell_leakage;
  const sta::Scene* scene = snap.scene;
  // Output-cone term: arcs that terminate at this instance's output pins.
  for (const OutputCtx& o : snap.outputs) {
    if (o.lambda_sum == 0.0f || o.port == nullptr) {
      continue;  // no timing pressure on this output pin
    }
    sta::LibertyPort* cand_port = cell->findLibertyPort(o.port->name());
    if (cand_port == nullptr) {
      // Candidate cell missing this output port - reject via huge cost.
      return std::numeric_limits<float>::infinity();
    }
    const float d = sta::delayAsFloat(resizer_->gateDelay(
        cand_port, o.load_cap, scene, max_, arc_delay_calc));
    cost += timing_weight * o.lambda_sum * d;
  }
  // Upstream-Cin term: arcs inside each upstream driver U that terminate
  // at the driver pin feeding one of inst's input pins. Their delay
  // depends on the load U drives, which includes inst's input capacitance
  // on that pin. Substituting the candidate's input cap perturbs the
  // upstream's load and shifts its delay.
  for (const UpstreamCtx& u : snap.upstream) {
    if (u.lambda_U_drv == 0.0f || u.drv_port == nullptr
        || u.orig_in_port == nullptr) {
      continue;
    }
    const float c_in_cand = portInputCap(cell, u.orig_in_port->name().c_str());
    if (c_in_cand == 0.0f) {
      // Candidate missing this input port - incompatible.
      return std::numeric_limits<float>::infinity();
    }
    float load_pert = u.load_U_cur - u.c_in_cur + c_in_cand;
    if (load_pert < 0.0f) {
      // Numerical safety: extreme C_in mismatches can push the perturbed
      // load slightly negative. Clamp at zero rather than rejecting; the
      // gateDelay LUT is well-defined at zero load.
      load_pert = 0.0f;
    }
    const float d_U = sta::delayAsFloat(resizer_->gateDelay(
        u.drv_port, load_pert, scene, max_, arc_delay_calc));
    cost += timing_weight * u.lambda_U_drv * d_U;
  }
  return cost;
}

bool LRSubproblem::candidateDrcOkSnapshot(const GateSnapshot& snap,
                                          sta::LibertyCell* replacement) const
{
  // Input-side: reject if a fanin net's max-cap would be violated (or made
  // worse) by the new cell's larger input pin cap. Mirrors
  // Resizer::replacementPreservesMaxCap / checkMaxCapOK against the frozen
  // per-driver cap checks captured in snapshot().
  for (const InputMaxCapCtx& in : snap.inputs) {
    if (in.in_port == nullptr) {
      continue;
    }
    const float new_cap = portInputCap(replacement, in.in_port->name().c_str());
    const float cap_delta = new_cap - in.old_cap;
    if (cap_delta <= 0.0f) {
      continue;
    }
    for (const DriverCapCheck& dc : in.drivers) {
      if (!dc.corner_ok) {
        continue;
      }
      const float ncap = dc.cap + cap_delta;
      if (dc.cap_slack < 0.0f) {
        if (ncap > dc.cap) {
          return false;
        }
      } else if (ncap > dc.max_cap) {
        return false;
      }
    }
  }

  // Output-side: per-output-pin check against the new cell's cap/slew limits.
  for (const OutputCtx& o : snap.outputs) {
    if (o.port == nullptr) {
      continue;
    }
    sta::LibertyPort* cand_port = replacement->findLibertyPort(o.port->name());
    if (cand_port == nullptr) {
      return false;  // candidate missing this output port - reject
    }

    if (checkOutputMaxCap(cand_port, o.load_cap, max_)) {
      return false;
    }

    const float slew_factor = (o.drive_res > 0.0f && o.load_cap > 0.0f)
                                  ? o.slew / (o.drive_res * o.load_cap)
                                  : 0.0f;
    if (checkOutputMaxSlew(
            sta_, cand_port, slew_factor, o.load_cap, snap.scene, max_)) {
      return false;
    }
  }

  return true;
}

LRSubproblem::GateDecision LRSubproblem::evaluateSnapshot(
    const GateSnapshot& snap,
    const float timing_weight,
    sta::ArcDelayCalc* arc_delay_calc) const
{
  GateDecision result;
  result.inst = snap.inst;

  // Baseline cost with the current cell.
  result.baseline_cost = evaluateCellCost(
      snap, snap.cur_cell, snap.cur_leakage, timing_weight, arc_delay_calc);
  result.best_cost = result.baseline_cost;
  float best_leak = snap.cur_leakage;

  for (const Candidate& cand : snap.candidates) {
    // Hard DRC filter: reject any candidate that would introduce a max-cap
    // or max-slew violation.
    if (!candidateDrcOkSnapshot(snap, cand.cell)) {
      continue;
    }
    const float cost = evaluateCellCost(
        snap, cand.cell, cand.leakage, timing_weight, arc_delay_calc);
    if (cost < result.best_cost) {
      result.best_cost = cost;
      result.best_cell = cand.cell;
      best_leak = cand.leakage;
    }
  }

  if (result.best_cell != nullptr) {
    result.best_is_downsize = best_leak < snap.cur_leakage;
  }
  return result;
}

}  // namespace rsz
