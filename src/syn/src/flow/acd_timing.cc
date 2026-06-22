// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "flow/acd.h"
#include "odb/db.h"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/Path.hh"
#include "sta/PortDirection.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingModel.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace syn {
namespace acd {

float& ArrivalSet::atExit(int out, const sta::RiseFall* out_rf)
{
  return v[(size_t) out * 2 + out_rf->index()];
}

float ArrivalSet::atExit(int out, const sta::RiseFall* out_rf) const
{
  return v[(size_t) out * 2 + out_rf->index()];
}

// Elementwise max into this value.
void ArrivalSet::mergeMax(const ArrivalSet& o)
{
  for (int i = 0; i < 4; i++) {
    v[i] = std::max(v[i], o.v[i]);
  }
}

// This value with a scalar delay added to every entry (-inf stays -inf).
ArrivalSet ArrivalSet::plus(float d) const
{
  ArrivalSet r;
  for (int i = 0; i < 4; i++) {
    r.v[i] = v[i] + d;
  }
  return r;
}

float ArrivalSet::maxEntry() const
{
  return std::max(std::max(v[0], v[1]), std::max(v[2], v[3]));
}

ArrivalSet& NodeArrivals::atTransition(const sta::RiseFall* rf)
{
  if (rf == sta::RiseFall::rise()) {
    return rise;
  } else {
    return fall;
  }
}

const ArrivalSet& NodeArrivals::atTransition(const sta::RiseFall* rf) const
{
  if (rf == sta::RiseFall::rise()) {
    return rise;
  } else {
    return fall;
  }
}

float NodeArrivals::maxEntry() const
{
  return std::max(rise.maxEntry(), fall.maxEntry());
}

float NodeArrivals::exitSlack(int idx) const
{
  float ret = std::numeric_limits<float>::infinity();
  for (auto rf : sta::RiseFall::range()) {
    ret = std::min(ret, -atTransition(rf).atExit(idx, rf));
  }
  return ret;
}

NodeArrivals outputArrival(const sta::LibertyPort* out_port,
                           const std::vector<NodeArrivals>& input_pin_arrivals,
                           float load,
                           const sta::Scene* corner,
                           const float fixed_slew[2])
{
  NodeArrivals result;
  const sta::MinMax* min_max = sta::MinMax::max();
  const sta::Pvt* pvt = corner->sdc()->operatingConditions(min_max);
  const sta::LibertyCell* cell = out_port->libertyCell();

  std::vector<sta::LibertyPort*> in_ports;

  {
    // Collect input ports
    sta::LibertyCellPortIterator iterator(cell);
    while (iterator.hasNext()) {
      sta::LibertyPort* p = iterator.next();
      if (p->direction()->isInput() && !p->isPwrGnd()) {
        in_ports.push_back(p);
      }
    }
  }

  for (sta::TimingArcSet* arc_set : cell->timingArcSets()) {
    if (arc_set->to() == out_port && !arc_set->role()->isTimingCheck()) {
      for (sta::TimingArc* arc : arc_set->arcs()) {
        auto* model = dynamic_cast<sta::GateTimingModel*>(arc->model());
        if (model == nullptr) {
          continue;
        }

        auto it = std::find(in_ports.begin(), in_ports.end(), arc->from());
        assert(it != in_ports.end());
        int in_idx = it - in_ports.begin();
        const auto* in_rf = arc->fromEdge()->asRiseFall();
        const auto* out_rf = arc->toEdge()->asRiseFall();

        float gate_delay = 0.0f;
        float drvr_slew = 0.0f;
        model->gateDelay(
            pvt, fixed_slew[in_rf->index()], load, gate_delay, drvr_slew);

        auto& input_rf_arr = input_pin_arrivals[in_idx].atTransition(in_rf);
        result.atTransition(out_rf).mergeMax(input_rf_arr.plus(gate_delay));
      }
    }
  }
  return result;
}

float networkSlack(utl::Logger* logger,
                   const acd::SynthesisProblem& problem,
                   const acd::GateNetwork& net,
                   const sta::Scene* corner,
                   const float fixed_slew[2])
{
  assert(net.outs.size() == problem.outputs.size());

  std::vector<float> load(net.nodes.size(), 0.0f);

  for (size_t i = 0; i < problem.outputs.size(); i++) {
    const auto [is_pi, gate_idx] = net.outs[i];
    if (!is_pi) {
      load[gate_idx] += problem.outputs[i].external_load;
    }
  }

  for (size_t i = 0; i < net.nodes.size(); i++) {
    const acd::GateNode& node = net.nodes[i];

    sta::LibertyCellPortIterator iterator(node.driver_port->libertyCell());
    size_t port_idx = 0;
    while (iterator.hasNext()) {
      sta::LibertyPort* port = iterator.next();
      if (!port->isPwrGnd() && port->direction()->isInput()) {
        const auto [is_pi, idx] = node.fanins[port_idx++];
        if (!is_pi) {
          load[idx] += port->capacitance();
        }
      }
    }
  }

  std::vector<NodeArrivals> node_arrivals(net.nodes.size());
  auto src_arr = [&](std::pair<bool, int> ref) -> const NodeArrivals& {
    if (ref.first) {
      return problem.inputs[ref.second].arrivals;
    } else {
      return node_arrivals[ref.second];
    }
  };

  for (size_t i = 0; i < net.nodes.size(); i++) {
    const acd::GateNode& node = net.nodes[i];

    std::vector<NodeArrivals> input_pin_arrivals(node.fanins.size());
    for (size_t t = 0; t < node.fanins.size(); t++) {
      input_pin_arrivals[t] = src_arr(node.fanins[t]);
    }

    node_arrivals[i] = outputArrival(
        node.driver_port, input_pin_arrivals, load[i], corner, fixed_slew);
  }

  float ws = std::numeric_limits<float>::infinity();
  for (size_t i = 0; i < net.outs.size(); i++) {
    const NodeArrivals& arrivals = src_arr(net.outs[i]);

    for (auto rf : sta::RiseFall::range()) {
      // See comment above `acd::ArrivalSet` to understand the connection
      // between arrivals and slacks by convention inside ACD
      float path_ws = -arrivals.atTransition(rf).atExit(i, rf);
      ws = std::min(ws, path_ws);
    }
  }

  return ws;
}

float avgInputCap(const sta::LibertyCell* cell)
{
  float sum = 0.0f;
  int n = 0;
  sta::LibertyCellPortIterator pit(cell);
  while (pit.hasNext()) {
    sta::LibertyPort* p = pit.next();
    if (p->direction()->isInput() && !p->isPwrGnd()) {
      sum += p->capacitance();
      n++;
    }
  }
  return n > 0 ? sum / (float) n : 0.0f;
}

float findDelayLowerBound(const sta::Scene* corner,
                          const sta::LibertyCell* cell,
                          const float fixed_slew[2])
{
  const sta::MinMax* max = sta::MinMax::max();
  const sta::Pvt* pvt = corner->sdc()->operatingConditions(max);
  float avg_cin = avgInputCap(const_cast<const sta::LibertyCell*>(
      const_cast<sta::LibertyCell*>(cell)->sceneCell(corner, max)));
  float bound = std::numeric_limits<float>::infinity();

  for (sta::TimingArcSet* arc_set : cell->timingArcSets()) {
    if (!arc_set->role()->isTimingCheck()) {
      for (sta::TimingArc* arc : arc_set->arcs()) {
        auto* model = dynamic_cast<sta::GateTimingModel*>(arc->model());
        if (model == nullptr) {
          continue;
        }
        const auto* in_rf = arc->fromEdge()->asRiseFall();
        float gate_delay = 0.0f;
        float drvr_slew = 0.0f;
        model->gateDelay(pvt,
                         fixed_slew[in_rf->index()],
                         2 * avg_cin,
                         gate_delay,
                         drvr_slew);
        bound = std::min(bound, gate_delay);
      }
    }
  }

  return bound;
}

void populateCutTiming(sta::dbNetwork* network,
                       sta::Sta* sta,
                       acd::SynthesisProblem& problem,
                       const std::span<sta::Net*> roots,
                       const std::span<sta::Net*> leaves,
                       const std::unordered_set<sta::Instance*>& cone)
{
  sta::Graph* graph = sta->graph();
  sta::Search* search = sta->search();
  sta::GraphDelayCalc* graph_dcalc = sta->graphDelayCalc();
  const sta::Scene* scene = sta ? sta->cmdScene() : nullptr;
  const sta::MinMax* max = sta::MinMax::max();

  const auto dcalc_ap = scene->dcalcAnalysisPtIndex(max);
  auto poRequired = [&](sta::Vertex* po_v, sta::Path* po_path) -> float {
    const float sta_req = (float) po_path->required();
    const sta::RiseFall* cur_rf = po_path->transition(sta);
    sta::Tag* po_tag = po_path->tag(sta);
    float ext = std::numeric_limits<float>::infinity();
    bool has_internal = false;
    bool has_external = false;
    sta::VertexOutEdgeIterator eit(po_v, graph);
    while (eit.hasNext()) {
      sta::Edge* edge = eit.next();
      sta::Vertex* w = edge->to(graph);
      if (cone.contains(network->instance(w->pin()))) {
        has_internal = true;
        continue;
      }
      for (sta::TimingArc* arc : edge->timingArcSet()->arcs()) {
        if (arc->fromEdge()->asRiseFall() != cur_rf) {
          continue;
        }
        const sta::RiseFall* to_rf = arc->toEdge()->asRiseFall();
        sta::Tag* to_tag = search->thruTag(po_tag, edge, to_rf, nullptr);
        if (!to_tag) {
          continue;
        }
        sta::Path* sp = sta::Path::vertexPath(w, to_tag, sta);
        if (!sp) {
          continue;
        }
        ext = std::min(ext,
                       (float) sp->required()
                           - (float) graph->arcDelay(edge, arc, dcalc_ap));
        has_external = true;
      }
    }
    return (has_internal && has_external) ? ext : sta_req;
  };

  std::unordered_map<sta::Vertex*, int> vertex_po_index;
  assert(roots.size() == problem.outputs.size());
  for (int i = 0; i < problem.outputs.size(); i++) {
    sta::PinSet* pin_set = network->drivers(roots[i]);
    if (pin_set->size() != 1) {
      continue;
    }

    const sta::Pin* pin = *pin_set->begin();
    vertex_po_index[graph->pinDrvrVertex(pin)] = i;

    float load = graph_dcalc->loadCap(pin, scene, max);
    auto pin_iter = std::unique_ptr<sta::NetConnectedPinIterator>(
        network->connectedPinIterator(roots[i]));
    while (pin_iter->hasNext()) {
      sta::Pin* conntected_pin = const_cast<sta::Pin*>(pin_iter->next());
      sta::LibertyPort* port = network->libertyPort(conntected_pin);
      sta::Instance* instance = network->instance(conntected_pin);
      if (instance && cone.contains(instance) && port
          && port->direction()->isInput()) {
        load -= port->scenePort(scene, max)->capacitance();
      }
    }

    problem.outputs[i].external_load = load;
  }

  assert(leaves.size() == problem.inputs.size());
  for (size_t li = 0; li < problem.inputs.size(); li++) {
    sta::PinSet* pin_set = network->drivers(leaves[li]);
    if (pin_set->size() != 1) {
      continue;
    }

    sta::Vertex* leaf = graph->pinDrvrVertex(*pin_set->begin());
    if (!leaf) {
      continue;
    }

    // Walk paths starting at leaf to obtain budget on individual
    // propagation arcs across the cut
    sta::VertexPathIterator pit(leaf, sta);
    while (pit.hasNext()) {
      sta::Path* p0 = pit.next();
      if (p0->minMax(sta) != max) {
        continue;
      }
      const sta::RiseFall* leaf_rf = p0->transition(sta);
      const float a0 = p0->arrival();
      acd::ArrivalSet& slot = problem.inputs[li].arrivals.atTransition(leaf_rf);

      std::vector<std::tuple<sta::Vertex*, sta::Tag*, const sta::RiseFall*>>
          stack;
      std::set<std::pair<sta::Vertex*, sta::Tag*>> visited;
      stack.emplace_back(leaf, p0->tag(sta), leaf_rf);
      visited.insert({leaf, p0->tag(sta)});

      while (!stack.empty()) {
        auto [v, tag, cur_rf] = stack.back();
        stack.pop_back();
        sta::VertexOutEdgeIterator eit(v, graph);
        while (eit.hasNext()) {
          sta::Edge* edge = eit.next();
          sta::Vertex* w = edge->to(graph);
          for (sta::TimingArc* arc : edge->timingArcSet()->arcs()) {
            if (arc->fromEdge()->asRiseFall() != cur_rf) {
              continue;
            }
            const sta::RiseFall* to_rf = arc->toEdge()->asRiseFall();
            sta::Tag* to_tag = search->thruTag(tag, edge, to_rf, nullptr);
            if (!to_tag) {
              continue;
            }
            sta::Path* wp = sta::Path::vertexPath(w, to_tag, sta);
            if (!wp) {
              continue;
            }
            auto po = vertex_po_index.find(w);
            if (po != vertex_po_index.end()) {
              const float m = a0 - poRequired(w, wp);
              float& entry = slot.atExit(po->second, to_rf);
              entry = std::max(entry, m);
            }
            if (cone.contains(network->instance(w->pin()))
                && visited.insert({w, to_tag}).second) {
              stack.emplace_back(w, to_tag, to_rf);
            }
          }
        }
      }
    }
  }
}

};  // namespace acd
};  // namespace syn
