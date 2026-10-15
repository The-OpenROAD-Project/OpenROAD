// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "ant/AntennaChecker.hh"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "AntennaCheckerImpl.hh"
#include "AntennaReport.hh"
#include "AntennaRules.hh"
#include "PinType.hh"
#include "Polygon.hh"
#include "WireBuilder.hh"
#include "boost/pending/disjoint_sets.hpp"
#include "boost/polygon/polygon.hpp"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/dbWireGraph.h"
#include "odb/geom.h"
#include "utl/Logger.h"
#include "utl/unionFind.h"

namespace ant {

// Abbreviations Index:
//   `PAR`: Partial Area Ratio
//   `CAR`: Cumulative Area Ratio
//   `Area`: Gate Area
//   `S. Area`: Side Diffusion Area
//   `C. Area`: Cumulative Gate Area
//   `C. S. Area`: Cumulative Side (Diffusion) Area

AntennaChecker::AntennaChecker(odb::dbDatabase* db, utl::Logger* logger)
    : impl_(std::make_unique<Impl>(db, logger))
{
}

AntennaChecker::~AntennaChecker() = default;

int AntennaChecker::checkAntennas(odb::dbNet* net,
                                  int num_threads,
                                  bool verbose)
{
  return impl_->checkAntennas(net, num_threads, verbose);
}

int DesignCheck::violatingNets() const
{
  return std::ranges::count_if(nets, &NetCheck::violated);
}

int DesignCheck::violatingPins() const
{
  int count = 0;
  for (const NetCheck& net : nets) {
    count += std::ranges::count_if(net.pins, &PinCheck::violated);
  }
  return count;
}

DesignCheck AntennaChecker::check(odb::dbNet* net, int num_threads)
{
  return impl_->check(net, num_threads);
}

int AntennaChecker::antennaViolationCount() const
{
  return impl_->antennaViolationCount();
}

int AntennaChecker::pinViolationCount() const
{
  return impl_->pinViolationCount();
}

NetCheck AntennaChecker::checkNet(odb::dbNet* net, const float ratio_margin)
{
  return impl_->checkNet(net, ratio_margin);
}

Violations AntennaChecker::getAntennaViolations(odb::dbNet* net,
                                                odb::dbMTerm* diode_mterm,
                                                float ratio_margin)
{
  return impl_->getAntennaViolations(net, diode_mterm, ratio_margin);
}

void AntennaChecker::setReportFileName(const char* file_name)
{
  impl_->setReportFileName(file_name);
}

void AntennaChecker::makeNetWiresFromGuides(
    const std::vector<odb::dbNet*>& nets)
{
  impl_->makeNetWiresFromGuides(nets);
}

void AntennaChecker::initAntennaRules()
{
  impl_->initAntennaRules();
}

//////////////////////////////////////////////////

AntennaChecker::Impl::Impl(odb::dbDatabase* db, utl::Logger* logger)
    : db_(db), logger_(logger)
{
}

void AntennaChecker::Impl::initAntennaRules()
{
  block_ = db_->getChip()->getBlock();
  odb::dbTech* tech = db_->getTech();
  if (!layer_info_.empty()) {
    return;
  }

  // Build the whole table before storing it: makeAntennaModel can raise
  // ANT-13, and a partial table would be taken as complete next time.
  odb::PtrMap<odb::dbTechLayer, AntennaModel> layer_info;
  for (odb::dbTechLayer* tech_layer : tech->getLayers()) {
    layer_info[tech_layer] = makeAntennaModel(tech_layer, logger_);
  }
  layer_info_ = std::move(layer_info);
}

void AntennaChecker::Impl::saveGates(odb::dbNet* db_net,
                                     LayerToGraphNodes& node_by_layer_map,
                                     const int node_count)
{
  std::map<PinType, std::vector<int>, PinTypeCmp> pin_nbrs;
  std::vector<int> ids;
  // struct to save pin polygons
  using LayerAndPin = std::pair<int, PinType>;
  std::vector<LayerAndPin> pin_polys;
  // iterate all instance pins
  for (odb::dbITerm* iterm : db_net->getITerms()) {
    odb::dbMTerm* mterm = iterm->getMTerm();
    std::string pin_name = fmt::format("  {}/{} ({})",
                                       iterm->getInst()->getConstName(),
                                       mterm->getConstName(),
                                       mterm->getMaster()->getConstName());
    PinType pin = PinType(std::move(pin_name), iterm);
    odb::dbInst* inst = iterm->getInst();
    const odb::dbTransform transform = inst->getTransform();
    for (odb::dbMPin* mterm : mterm->getMPins()) {
      for (odb::dbBox* box : mterm->getGeometry()) {
        odb::dbTechLayer* tech_layer = box->getTechLayer();
        if (tech_layer->getType() != odb::dbTechLayerType::ROUTING) {
          continue;
        }
        // get lower and upper layer
        odb::dbTechLayer* upper_layer = tech_layer->getUpperLayer();
        odb::dbTechLayer* lower_layer = tech_layer->getLowerLayer();

        odb::Rect pin_rect = box->getBox();
        transform.apply(pin_rect);
        // convert rect -> polygon
        Polygon pin_pol = rectToPolygon(pin_rect);
        // Save polygon to add on DSU
        pin_polys.emplace_back(tech_layer->getRoutingLevel(), pin);
        // if has wire on same layer connect to pin
        ids = findNodesWithIntersection(node_by_layer_map[tech_layer], pin_pol);
        for (const int& index : ids) {
          pin_nbrs[pin].push_back(node_by_layer_map[tech_layer][index]->id);
        }
        // if has via on upper layer connected to pin
        if (upper_layer) {
          ids = findNodesWithIntersection(node_by_layer_map[upper_layer],
                                          pin_pol);
          for (const int& index : ids) {
            pin_nbrs[pin].push_back(node_by_layer_map[upper_layer][index]->id);
          }
        }
        // if has via on lower layer connected to pin
        if (lower_layer) {
          ids = findNodesWithIntersection(node_by_layer_map[lower_layer],
                                          pin_pol);
          for (const int& index : ids) {
            pin_nbrs[pin].push_back(node_by_layer_map[lower_layer][index]->id);
          }
        }
      }
    }
  }
  // Sort pin polygon based by layer level (greatest first)
  std::sort(pin_polys.begin(),
            pin_polys.end(),
            [](const LayerAndPin& a, const LayerAndPin& b) {
              return a.first > b.first;
            });

  // run DSU from min_layer to max_layer
  std::vector<int> dsu_parent(node_count);
  std::vector<int> dsu_size(node_count);
  for (int i = 0; i < node_count; i++) {
    dsu_size[i] = 1;
    dsu_parent[i] = i;
  }

  boost::disjoint_sets<int*, int*> dsu(dsu_size.data(), dsu_parent.data());

  odb::dbTech* tech = db_->getTech();
  odb::dbTechLayer* iter = tech->findRoutingLayer(1);
  odb::dbTechLayer* lower_layer;
  while (iter) {
    // Get lower layer
    lower_layer = iter->getLowerLayer();
    if (lower_layer) {
      // Check only vias layer to add pin connections
      if (lower_layer->getRoutingLevel() != 0) {
        int layer_level = lower_layer->getRoutingLevel();
        // only include pin on layer below to the current
        while (!pin_polys.empty() && layer_level >= pin_polys.back().first) {
          PinType pin = pin_polys.back().second;
          int last_id = -1;
          pin_polys.pop_back();
          // Set union of all wires connected to pin
          for (const int& nbr_id : pin_nbrs[pin]) {
            if (last_id != -1
                && dsu.find_set(last_id) != dsu.find_set(nbr_id)) {
              dsu.union_set(last_id, nbr_id);
            }
            last_id = nbr_id;
          }
        }
      }
      // iterate each node of this layer to union set
      for (auto& node_it : node_by_layer_map[iter]) {
        int id_u = node_it->id;
        // get lower neighbors and union
        for (const int& lower_it : node_it->low_adj) {
          int id_v = node_by_layer_map[lower_layer][lower_it]->id;
          // if they are on different sets then union
          if (dsu.find_set(id_u) != dsu.find_set(id_v)) {
            dsu.union_set(id_u, id_v);
          }
        }
      }
    }
    for (auto& node_it : node_by_layer_map[iter]) {
      int id_u = node_it->id;
      // check gates in same set (first Nodes x gates)
      for (const auto& gate_it : pin_nbrs) {
        for (const int& nbr_id : gate_it.second) {
          if (dsu.find_set(id_u) == dsu.find_set(nbr_id)) {
            node_it->gates.insert(gate_it.first);
            break;
          }
        }
      }
    }
    iter = iter->getUpperLayer();
  }
}

void AntennaChecker::Impl::calculateWirePar(odb::dbTechLayer* tech_layer,
                                            NodeInfo& info)
{
  // get info from layer map
  const double diff_metal_factor = layer_info_[tech_layer].diff_metal_factor;
  const double diff_side_metal_factor
      = layer_info_[tech_layer].diff_side_metal_factor;
  const double minus_diff_factor = layer_info_[tech_layer].minus_diff_factor;
  const double plus_diff_factor = layer_info_[tech_layer].plus_diff_factor;

  const double metal_factor = layer_info_[tech_layer].metal_factor;
  const double side_metal_factor = layer_info_[tech_layer].side_metal_factor;

  double diff_metal_reduce_factor = 1.0;
  if (tech_layer->hasDefaultAntennaRule()) {
    const odb::dbTechLayerAntennaRule* antenna_rule
        = tech_layer->getDefaultAntennaRule();
    diff_metal_reduce_factor = getPwlFactor(
        antenna_rule->getAreaDiffReduce(), info.iterm_diff_area, 1.0);
  }

  if (info.iterm_diff_area != 0) {
    // Calculate PAR
    info.par = (diff_metal_factor * info.area) / info.iterm_gate_area;
    info.psr = (diff_side_metal_factor * info.side_area) / info.iterm_gate_area;

    double plus_diff_protect = plus_diff_factor * info.iterm_diff_area;
    if (!layer_info_[tech_layer].gate_plus_diff_idx.empty()) {
      odb::dbTechLayerAntennaRule::pwl_pair pwl
          = {.indices = layer_info_[tech_layer].gate_plus_diff_idx,
             .ratios = layer_info_[tech_layer].gate_plus_diff_ratios};
      plus_diff_protect = getPwlFactor(pwl, info.iterm_diff_area, 0.0);
    }

    // Calculate PSR
    info.diff_par = (diff_metal_factor * info.area * diff_metal_reduce_factor
                     - minus_diff_factor * info.iterm_diff_area)
                    / (info.iterm_gate_area + plus_diff_protect);
    info.diff_psr
        = (diff_side_metal_factor * info.side_area * diff_metal_reduce_factor
           - minus_diff_factor * info.iterm_diff_area)
          / (info.iterm_gate_area + plus_diff_protect);
  } else {
    // Calculate PAR
    info.par = (metal_factor * info.area) / info.iterm_gate_area;
    info.psr = (side_metal_factor * info.side_area) / info.iterm_gate_area;

    // Calculate PSR
    info.diff_par = (metal_factor * info.area * diff_metal_reduce_factor)
                    / info.iterm_gate_area;
    info.diff_psr
        = (side_metal_factor * info.side_area * diff_metal_reduce_factor)
          / info.iterm_gate_area;
  }
}

void AntennaChecker::Impl::calculateViaPar(odb::dbTechLayer* tech_layer,
                                           NodeInfo& info)
{
  // get info from layer map
  const double diff_cut_factor = layer_info_[tech_layer].diff_cut_factor;
  const double minus_diff_factor = layer_info_[tech_layer].minus_diff_factor;
  const double plus_diff_factor = layer_info_[tech_layer].plus_diff_factor;
  const double cut_factor = layer_info_[tech_layer].cut_factor;

  double diff_metal_reduce_factor = 1.0;
  if (tech_layer->hasDefaultAntennaRule()) {
    const odb::dbTechLayerAntennaRule* antenna_rule
        = tech_layer->getDefaultAntennaRule();
    diff_metal_reduce_factor = getPwlFactor(
        antenna_rule->getAreaDiffReduce(), info.iterm_diff_area, 1.0);
  }

  if (info.iterm_diff_area != 0) {
    // Calculate PAR
    info.par = (diff_cut_factor * info.area) / info.iterm_gate_area;

    double plus_diff_protect = plus_diff_factor * info.iterm_diff_area;
    if (!layer_info_[tech_layer].gate_plus_diff_idx.empty()) {
      odb::dbTechLayerAntennaRule::pwl_pair pwl
          = {.indices = layer_info_[tech_layer].gate_plus_diff_idx,
             .ratios = layer_info_[tech_layer].gate_plus_diff_ratios};
      plus_diff_protect = getPwlFactor(pwl, info.iterm_diff_area, 0.0);
    }

    // Calculate diff_PAR
    info.diff_par = (diff_cut_factor * info.area * diff_metal_reduce_factor
                     - minus_diff_factor * info.iterm_diff_area)
                    / (info.iterm_gate_area + plus_diff_protect);
  } else {
    // Calculate PAR
    info.par = (cut_factor * info.area) / info.iterm_gate_area;
    // Calculate diff_PAR
    info.diff_par = (cut_factor * info.area * diff_metal_reduce_factor)
                    / info.iterm_gate_area;
  }
}

void AntennaChecker::Impl::calculateAreas(
    const LayerToGraphNodes& node_by_layer_map,
    PinAntennas& pins,
    GateToLayerToNodeInfo& gate_info)
{
  auto getPin = [&](odb::dbMTerm* mterm) -> const PinAntenna& {
    auto [iter, inserted] = pins.try_emplace(mterm);
    if (inserted) {
      iter->second = readPinAntenna(mterm);
    }
    return iter->second;
  };

  for (const auto& it : node_by_layer_map) {
    odb::dbTechLayer* layer = it.first;
    const bool is_routing = layer->getRoutingLevel() != 0;
    double wire_thickness = 0.0;
    if (is_routing) {
      uint32_t wire_thickness_dbu = 0;
      layer->getThickness(wire_thickness_dbu);
      wire_thickness = block_->dbuToMicrons(wire_thickness_dbu);
    }
    // group the layer nodes by the iterms they reach
    const int node_count = it.second.size();
    std::vector<bool> has_gate(node_count, false);
    odb::PtrMap<odb::dbITerm, std::vector<int>> nodes_by_iterm;
    for (int i = 0; i < node_count; i++) {
      for (const auto& gate : it.second[i]->gates) {
        if (!gate.is_iterm) {
          continue;
        }
        nodes_by_iterm[gate.iterm].push_back(i);
        if (getPin(gate.iterm->getMTerm()).is_gate) {
          has_gate[i] = true;
        }
      }
    }
    // nodes sharing an iterm are one physical piece of metal (the pin
    // geometry bridges them, whatever the pin's direction), so merge them
    // transitively
    utl::UnionFind groups(node_count);
    for (const auto& [iterm, node_ids] : nodes_by_iterm) {
      for (const int node_id : node_ids) {
        groups.unite(node_ids.front(), node_id);
      }
    }
    // a group is checked when any of its nodes reaches a valid gate
    std::vector<bool> group_has_gate(node_count, false);
    for (int i = 0; i < node_count; i++) {
      if (has_gate[i]) {
        group_has_gate[groups.find(i)] = true;
      }
    }
    // sum the geometry of each checked group's nodes
    std::vector<NodeInfo> info_by_group(node_count);
    for (int i = 0; i < node_count; i++) {
      const size_t group = groups.find(i);
      if (!group_has_gate[group]) {
        continue;
      }
      NodeInfo& info = info_by_group[group];
      const auto area = static_cast<int64_t>(gtl::area(it.second[i]->pol));
      info.area += block_->dbuAreaToMicrons(area);
      if (is_routing) {
        // Calculate side area of wire
        info.side_area += block_->dbuToMicrons(gtl::perimeter(it.second[i]->pol)
                                               * wire_thickness);
      }
    }
    // each iterm contributes once to every group it touches: its gate and
    // diffusion areas on this layer, and any metal inside its block that
    // LEF reports as partial area
    for (const auto& [iterm, node_ids] : nodes_by_iterm) {
      std::set<size_t> iterm_groups;
      for (const int node_id : node_ids) {
        const size_t group = groups.find(node_id);
        if (group_has_gate[group]) {
          iterm_groups.insert(group);
        }
      }
      const PinAntenna& pin = getPin(iterm->getMTerm());
      for (const size_t group : iterm_groups) {
        NodeInfo& info = info_by_group[group];
        if (pin.is_gate) {
          info.iterms.push_back(iterm);
        }
        info.iterm_gate_area += valueAtOrBelow(pin.gate_area, layer);
        info.iterm_diff_area += valueAtOrBelow(pin.diff_area, layer);
        if (is_routing) {
          info.area += valueOnLayer(pin.partial_metal_area, layer);
          info.side_area += valueOnLayer(pin.partial_metal_side_area, layer);
        } else {
          info.area += valueOnLayer(pin.partial_cut_area, layer);
        }
      }
    }
    // every gate of a group shares the same record
    for (const auto& [iterm, node_ids] : nodes_by_iterm) {
      if (getPin(iterm->getMTerm()).is_gate) {
        gate_info[iterm][layer] = info_by_group[groups.find(node_ids.front())];
      }
    }
  }
}

// calculate PAR and PSR of wires and vias
void AntennaChecker::Impl::calculatePAR(GateToLayerToNodeInfo& gate_info)
{
  for (auto& gate_it : gate_info) {
    for (auto& layer_it : gate_it.second) {
      NodeInfo& gate_info = layer_it.second;
      odb::dbTechLayer* tech_layer = layer_it.first;
      if (tech_layer->getRoutingLevel() == 0) {
        calculateViaPar(tech_layer, gate_info);
      } else {
        calculateWirePar(tech_layer, gate_info);
      }
    }
  }
}

// calculate CAR and CSR of wires and vias
void AntennaChecker::Impl::calculateCAR(const PinAntennas& pins,
                                        GateToLayerToNodeInfo& gate_info)
{
  odb::dbTech* tech = db_->getTech();
  for (auto& [gate, layer_to_node_info] : gate_info) {
    const PinAntenna& pin = pins.at(gate->getMTerm());
    // Variables to store the accumulated values for vias and wires
    NodeInfo sum_wire, sum_via;
    // iterate from first_layer -> last layer, cumulate sum for wires and vias
    for (odb::dbTechLayer* layer = tech->findRoutingLayer(1); layer;
         layer = layer->getUpperLayer()) {
      const bool is_routing = layer->getRoutingLevel() != 0;
      NodeInfo& sum = is_routing ? sum_wire : sum_via;
      // A block pin's MAX*CAR is the cumulative ratio its gates already have
      // from the block's own metal up to this layer.  It carries up through
      // the layers above, like any other layer's damage.
      const double max_car = valueOnLayer(
          is_routing ? pin.max_area_car : pin.max_cut_car, layer);
      const double max_csr
          = is_routing ? valueOnLayer(pin.max_side_area_car, layer) : 0.0;
      sum.par += max_car;
      sum.diff_par += max_car;
      sum.psr += max_csr;
      sum.diff_psr += max_csr;

      auto node_it = layer_to_node_info.find(layer);
      if (node_it == layer_to_node_info.end()) {
        continue;
      }
      NodeInfo& node_info = node_it->second;
      // MAX*CAR already counts the pin's partial area on its layer, so the
      // pin's own gates add only the rest of the node.
      NodeInfo layer_damage = node_info;
      if (max_car != 0.0 || max_csr != 0.0) {
        if (max_car != 0.0) {
          layer_damage.area -= valueOnLayer(
              is_routing ? pin.partial_metal_area : pin.partial_cut_area,
              layer);
        }
        if (max_csr != 0.0) {
          layer_damage.side_area
              -= valueOnLayer(pin.partial_metal_side_area, layer);
        }
        if (is_routing) {
          calculateWirePar(layer, layer_damage);
        } else {
          calculateViaPar(layer, layer_damage);
        }
      }
      sum += layer_damage;
      // Updating the node with the accumulated values
      node_info.car = sum.par;
      node_info.csr = sum.psr;
      node_info.diff_car = sum.diff_par;
      node_info.diff_csr = sum.diff_psr;
    }
  }
}

RatioCheck AntennaChecker::Impl::checkPAR(odb::dbTechLayer* tech_layer,
                                          NodeInfo& info,
                                          const float ratio_margin)
{
  // get rules
  const odb::dbTechLayerAntennaRule* antenna_rule
      = tech_layer->getDefaultAntennaRule();
  double par_ratio = antenna_rule->getPAR();
  odb::dbTechLayerAntennaRule::pwl_pair diff_par = antenna_rule->getDiffPAR();
  double diff_par_pwl_ratio = getPwlFactor(diff_par, info.iterm_diff_area, 0.0);

  // apply ratio_margin
  par_ratio *= (1.0 - ratio_margin / 100.0);
  diff_par_pwl_ratio *= (1.0 - ratio_margin / 100.0);

  RatioCheck check;
  // If node is connected to diffusion area or ANTENNAAREARATIO is not
  // defined, compare with ANTENNADIFFAREARATIO. Otherwise compare with
  // ANTENNAAREARATIO.
  if (info.iterm_diff_area != 0 || par_ratio == 0) {
    if (diff_par_pwl_ratio != 0) {
      check.violated = info.diff_par > diff_par_pwl_ratio;
      info.excess_ratio_par
          = std::max(info.excess_ratio_par, info.diff_par / diff_par_pwl_ratio);
    }
    check.actual = info.diff_par;
    check.required = diff_par_pwl_ratio;
  } else {
    if (par_ratio != 0) {
      check.violated = info.par > par_ratio;
      info.excess_ratio_par
          = std::max(info.excess_ratio_par, info.par / par_ratio);
    }
    check.actual = info.par;
    check.required = par_ratio;
  }
  return check;
}

RatioCheck AntennaChecker::Impl::checkPSR(odb::dbTechLayer* tech_layer,
                                          NodeInfo& info,
                                          const float ratio_margin)
{
  // get rules
  const odb::dbTechLayerAntennaRule* antenna_rule
      = tech_layer->getDefaultAntennaRule();
  double psr_ratio = antenna_rule->getPSR();
  const odb::dbTechLayerAntennaRule::pwl_pair diff_psr
      = antenna_rule->getDiffPSR();
  double diff_psr_pwl_ratio = getPwlFactor(diff_psr, info.iterm_diff_area, 0.0);

  // apply ratio_margin
  psr_ratio *= (1.0 - ratio_margin / 100.0);
  diff_psr_pwl_ratio *= (1.0 - ratio_margin / 100.0);

  RatioCheck check;
  // If node is connected to diffusion area or ANTENNASIDEAREARATIO is not
  // defined, compare with ANTENNADIFFSIDEAREARATIO. Otherwise compare with
  // ANTENNASIDEAREARATIO.
  if (info.iterm_diff_area != 0 || psr_ratio == 0) {
    if (diff_psr_pwl_ratio != 0) {
      check.violated = info.diff_psr > diff_psr_pwl_ratio;
      info.excess_ratio_psr
          = std::max(info.excess_ratio_psr, info.diff_psr / diff_psr_pwl_ratio);
    }
    check.actual = info.diff_psr;
    check.required = diff_psr_pwl_ratio;
  } else {
    if (psr_ratio != 0) {
      check.violated = info.psr > psr_ratio;
      info.excess_ratio_psr
          = std::max(info.excess_ratio_psr, info.psr / psr_ratio);
    }
    check.actual = info.psr;
    check.required = psr_ratio;
  }
  return check;
}

RatioCheck AntennaChecker::Impl::checkCAR(odb::dbTechLayer* tech_layer,
                                          const NodeInfo& info)
{
  // get rules
  const odb::dbTechLayerAntennaRule* antenna_rule
      = tech_layer->getDefaultAntennaRule();
  const double car_ratio = antenna_rule->getCAR();
  const odb::dbTechLayerAntennaRule::pwl_pair diff_car
      = antenna_rule->getDiffCAR();
  const double diff_car_pwl_ratio
      = getPwlFactor(diff_car, info.iterm_diff_area, 0);

  RatioCheck check;
  // If node is connected to diffusion area or ANTENNACUMAREARATIO is not
  // defined, compare with ANTENNACUMDIFFAREARATIO. Otherwise compare with
  // ANTENNACUMAREARATIO.
  if (info.iterm_diff_area != 0 || car_ratio == 0) {
    if (diff_car_pwl_ratio != 0) {
      check.violated = info.diff_car > diff_car_pwl_ratio;
    }
    check.actual = info.diff_car;
    check.required = diff_car_pwl_ratio;
  } else {
    if (car_ratio != 0) {
      check.violated = info.car > car_ratio;
    }
    check.actual = info.car;
    check.required = car_ratio;
  }
  return check;
}

RatioCheck AntennaChecker::Impl::checkCSR(odb::dbTechLayer* tech_layer,
                                          const NodeInfo& info)
{
  // get rules
  const odb::dbTechLayerAntennaRule* antenna_rule
      = tech_layer->getDefaultAntennaRule();
  const double csr_ratio = antenna_rule->getCSR();
  const odb::dbTechLayerAntennaRule::pwl_pair diff_csr
      = antenna_rule->getDiffCSR();
  const double diff_csr_pwl_ratio
      = getPwlFactor(diff_csr, info.iterm_diff_area, 0);

  RatioCheck check;
  // If node is connected to diffusion area or ANTENNACUMSIDEAREARATIO is not
  // defined, compare with ANTENNACUMDIFFSIDEAREARATIO. Otherwise compare with
  // ANTENNACUMSIDEAREARATIO.
  if (info.iterm_diff_area != 0 || csr_ratio == 0) {
    if (diff_csr_pwl_ratio != 0) {
      check.violated = info.diff_csr > diff_csr_pwl_ratio;
    }
    check.actual = info.diff_csr;
    check.required = diff_csr_pwl_ratio;
  } else {
    if (csr_ratio != 0) {
      check.violated = info.csr > csr_ratio;
    }
    check.actual = info.csr;
    check.required = csr_ratio;
  }
  return check;
}

LayerCheck AntennaChecker::Impl::checkRatioViolations(odb::dbTechLayer* layer,
                                                      NodeInfo& node_info,
                                                      const float ratio_margin)
{
  LayerCheck check;
  check.layer = layer;
  check.par = checkPAR(layer, node_info, ratio_margin);
  check.car = checkCAR(layer, node_info);
  check.violated = check.par.violated || check.car.violated;
  if (layer->getRoutingLevel() != 0) {
    check.psr = checkPSR(layer, node_info, ratio_margin);
    check.csr = checkCSR(layer, node_info);
    check.violated
        = check.violated || check.psr->violated || check.csr->violated;
  }
  return check;
}

int AntennaChecker::Impl::checkGates(odb::dbNet* db_net,
                                     odb::dbMTerm* diode_mterm,
                                     float ratio_margin,
                                     GateToLayerToNodeInfo& gate_info,
                                     Violations& antenna_violations,
                                     NetCheck& net_check)
{
  int pin_violation_count = 0;

  GateToViolationLayers gates_with_violations;

  net_check.net = db_net;
  for (auto& [node, layer_to_node] : gate_info) {
    PinCheck pin_check;
    pin_check.gate = node;

    for (auto& [layer, node_info] : layer_to_node) {
      if (layer->hasDefaultAntennaRule()) {
        LayerCheck layer_check
            = checkRatioViolations(layer, node_info, ratio_margin);
        if (layer_check.violated) {
          pin_check.violated = true;
          gates_with_violations[node].insert(layer);
          net_check.violated = true;
        }
        pin_check.layers.push_back(std::move(layer_check));
      }
    }
    if (pin_check.violated) {
      pin_violation_count++;
    }
    net_check.pins.push_back(std::move(pin_check));
  }
  std::unordered_map<odb::dbITerm*, int> num_diodes_added;
  odb::PtrMap<odb::dbTechLayer, odb::PtrSet<odb::dbITerm>> pin_added;
  // if checkGates is used by repair antennas
  if (pin_violation_count > 0) {
    for (const auto& [gate, violation_layers] : gates_with_violations) {
      for (odb::dbTechLayer* layer : violation_layers) {
        // when repair antenna is running, calculate number of diodes
        if (pin_added[layer].find(gate) == pin_added[layer].end()) {
          double diode_diff_area = 0.0;
          if (diode_mterm) {
            LayerValues diode_diff_areas;
            diode_mterm->getDiffArea(diode_diff_areas);
            diode_diff_area = valueAtOrBelow(diode_diff_areas, layer);
          }
          NodeInfo violation_info = gate_info[gate][layer];
          std::vector<odb::dbITerm*> gates = violation_info.iterms;
          odb::dbTechLayer* violation_layer = layer;
          int diode_count_per_gate = 0;
          // check violations only PAR & PSR
          bool par_violation
              = checkPAR(violation_layer, violation_info, ratio_margin)
                    .violated;
          bool psr_violation = false;
          // Only routing layers have side areas
          if (violation_layer->getRoutingLevel() != 0) {
            psr_violation
                = checkPSR(violation_layer, violation_info, ratio_margin)
                      .violated;
          }
          bool violated = par_violation || psr_violation;
          double excess_ratio = 1.0;
          if (violated) {
            excess_ratio = std::max(violation_info.excess_ratio_par,
                                    violation_info.excess_ratio_psr);
          }
          // while it has violation, increase iterm_diff_area
          if (diode_mterm) {
            while (par_violation || psr_violation) {
              // increasing iterm_diff_area and count
              violation_info.iterm_diff_area += diode_diff_area * gates.size();
              diode_count_per_gate++;
              // re-calculate info only PAR & PSR
              if (violation_layer->getRoutingLevel() == 0) {
                calculateViaPar(violation_layer, violation_info);
              } else {
                calculateWirePar(violation_layer, violation_info);
              }
              // re-check violations only PAR & PSR
              par_violation
                  = checkPAR(violation_layer, violation_info, ratio_margin)
                        .violated;
              // Only routing layers have side areas
              if (violation_layer->getRoutingLevel() != 0) {
                psr_violation
                    = checkPSR(violation_layer, violation_info, ratio_margin)
                          .violated;
              }
              if (diode_count_per_gate > kMaxDiodeCountPerGate) {
                debugPrint(logger_,
                           utl::ANT,
                           "check_gates",
                           1,
                           "Net {} requires more than {} diodes per gate to "
                           "repair violations.",
                           db_net->getConstName(),
                           kMaxDiodeCountPerGate);
                break;
              }
            }
          }
          pin_added[violation_layer].insert(gate);
          std::vector<odb::dbITerm*> gates_for_diode_insertion;
          gates_for_diode_insertion.push_back(gate);
          // Reduce the number of added diodes in other layer
          diode_count_per_gate
              = std::max(0, diode_count_per_gate - num_diodes_added[gate]);
          num_diodes_added[gate] += diode_count_per_gate;
          // save antenna violation
          if (violated) {
            antenna_violations.push_back({layer->getRoutingLevel(),
                                          gates_for_diode_insertion,
                                          diode_count_per_gate,
                                          excess_ratio});
          }

          bool car_violation
              = checkCAR(violation_layer, violation_info).violated;
          bool csr_violation
              = checkCSR(violation_layer, violation_info).violated;

          // naive approach for cumulative area violations. here, all the pins
          // of the net are included, and placing one diode per pin is not the
          // best approach. as a first implementation, insert one diode per net.
          // TODO: implement a proper approach for CAR violations
          if (car_violation || csr_violation) {
            gates_for_diode_insertion.clear();
            for (auto gate : gates) {
              odb::dbMaster* gate_master = gate->getMTerm()->getMaster();
              if (gate_master->getType()
                  != odb::dbMasterType::CORE_ANTENNACELL) {
                gates_for_diode_insertion.push_back(gate);
              }
            }
            antenna_violations.push_back({layer->getRoutingLevel(),
                                          std::move(gates_for_diode_insertion),
                                          1,
                                          1.0});
          }
        }
      }
    }
  }
  return pin_violation_count;
}

void AntennaChecker::Impl::buildLayerMaps(odb::dbNet* db_net,
                                          LayerToGraphNodes& node_by_layer_map)
{
  odb::dbWire* wires = db_net->getWire();

  odb::PtrMap<odb::dbTechLayer, PolygonSet> set_by_layer;

  wiresToPolygonSetMap(wires, set_by_layer);
  avoidPinIntersection(db_net, set_by_layer);

  int node_count = 0;
  for (const auto& layer_it : set_by_layer) {
    for (const auto& pol_it : layer_it.second) {
      bool is_via = layer_it.first->getRoutingLevel() == 0;
      node_by_layer_map[layer_it.first].push_back(
          std::make_unique<GraphNode>(node_count, is_via, pol_it));
      node_count++;
    }
  }

  // set connections between Polygons ( wire -> via -> wire)
  std::vector<int> upper_index, lower_index;
  for (const auto& layer_it : set_by_layer) {
    // iterate only via layers
    if (layer_it.first->getRoutingLevel() == 0) {
      int via_index = 0;
      for (const auto& via_it : layer_it.second) {
        lower_index = findNodesWithIntersection(
            node_by_layer_map[layer_it.first->getLowerLayer()], via_it);
        upper_index = findNodesWithIntersection(
            node_by_layer_map[layer_it.first->getUpperLayer()], via_it);

        // connect upper -> via
        for (int& up_index : upper_index) {
          node_by_layer_map[layer_it.first->getUpperLayer()][up_index]
              ->low_adj.push_back(via_index);
        }
        // connect via -> lower
        for (int& low_index : lower_index) {
          node_by_layer_map[layer_it.first][via_index]->low_adj.push_back(
              low_index);
        }
        via_index++;
      }
    }
  }
  saveGates(db_net, node_by_layer_map, node_count);
}

int AntennaChecker::Impl::analyzeNet(odb::dbNet* db_net,
                                     odb::dbMTerm* diode_mterm,
                                     float ratio_margin,
                                     Violations& antenna_violations,
                                     NetCheck& net_check)
{
  net_check.net = db_net;
  odb::dbWire* wire = db_net->getWire();
  int pin_violations = 0;
  if (wire) {
    LayerToGraphNodes node_by_layer_map;
    GateToLayerToNodeInfo gate_info;
    PinAntennas pins;
    buildLayerMaps(db_net, node_by_layer_map);

    calculateAreas(node_by_layer_map, pins, gate_info);

    calculatePAR(gate_info);
    calculateCAR(pins, gate_info);

    pin_violations = checkGates(db_net,
                                diode_mterm,
                                ratio_margin,
                                gate_info,
                                antenna_violations,
                                net_check);
  }
  return pin_violations;
}

Violations AntennaChecker::Impl::getAntennaViolations(odb::dbNet* net,
                                                      odb::dbMTerm* diode_mterm,
                                                      float ratio_margin)
{
  Violations antenna_violations;
  if (net->isSpecial()) {
    return antenna_violations;
  }
  // Repair loads the rules once and then calls this from many threads, so
  // load them here only when missing (a new checker, as with
  // check_net_violation); otherwise this only reads shared state.
  if (block_ == nullptr || layer_info_.empty()) {
    initAntennaRules();
  }

  NetCheck net_check;
  analyzeNet(net, diode_mterm, ratio_margin, antenna_violations, net_check);

  return antenna_violations;
}

NetCheck AntennaChecker::Impl::checkNet(odb::dbNet* net,
                                        const float ratio_margin)
{
  NetCheck net_check;
  net_check.net = net;
  if (net->isSpecial()) {
    return net_check;
  }
  initAntennaRules();
  Violations antenna_violations;
  analyzeNet(net, nullptr, ratio_margin, antenna_violations, net_check);
  return net_check;
}

bool AntennaChecker::Impl::designIsPlaced()
{
  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    if (bterm->getFirstPinPlacementStatus() == odb::dbPlacementStatus::NONE) {
      return false;
    }
  }

  for (odb::dbNet* net : block_->getNets()) {
    if (net->isSpecial()) {
      continue;
    }
    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      if (!inst->isPlaced()) {
        return false;
      }
    }
  }

  return true;
}

bool AntennaChecker::Impl::haveGuides()
{
  if (!designIsPlaced()) {
    return false;
  }

  for (odb::dbNet* net : block_->getNets()) {
    // check term count due to 1-pin nets in multiple designs.
    if (!net->isSpecial() && net->getGuides().empty() && net->getTermCount() > 1
        && !net->isConnectedByAbutment()) {
      return false;
    }
  }

  return true;
}

namespace {

// Wires built from the global route guides for the duration of a check.
class GuideWires
{
 public:
  GuideWires(odb::dbDatabase* db, utl::Logger* logger, odb::dbBlock* block)
      : block_(block)
  {
    WireBuilder(db, logger).makeNetWiresFromGuides();
  }
  ~GuideWires() { block_->destroyNetWires(); }

  GuideWires(const GuideWires&) = delete;
  GuideWires& operator=(const GuideWires&) = delete;

 private:
  odb::dbBlock* block_;
};

}  // namespace

DesignCheck AntennaChecker::Impl::check(odb::dbNet* net, const int num_threads)
{
  initAntennaRules();

  const bool drt_routes = haveRoutedNets();
  const bool grt_routes = !drt_routes && haveGuides();
  if (!grt_routes && !drt_routes) {
    logger_->error(utl::ANT,
                   8,
                   "No detailed or global routing found. Run global_route or "
                   "detailed_route first.");
  }
  std::optional<GuideWires> guide_wires;
  if (grt_routes) {
    guide_wires.emplace(db_, logger_, block_);
  }

  std::vector<odb::dbNet*> nets;
  if (net) {
    if (net->isSpecial()) {
      logger_->error(utl::ANT,
                     14,
                     "Skipped net {} because it is special.",
                     net->getName());
    }
    nets.push_back(net);
  } else {
    for (odb::dbNet* block_net : block_->getNets()) {
      if (!block_net->isSpecial()) {
        nets.push_back(block_net);
      }
    }
    std::ranges::sort(nets, odb::ODBPtrLess());
  }

  // Each net's result goes to its own slot, so the threads share nothing.
  std::vector<NetCheck> results(nets.size());
  std::vector<char> routed(nets.size(), false);
#pragma omp parallel for num_threads(num_threads) schedule(dynamic)
  for (int i = 0; i < static_cast<int>(nets.size()); i++) {
    routed[i] = nets[i]->getWire() != nullptr;
    Violations antenna_violations;
    analyzeNet(nets[i], nullptr, 0, antenna_violations, results[i]);
  }

  DesignCheck design;
  for (size_t i = 0; i < nets.size(); i++) {
    if (routed[i]) {
      design.nets.push_back(std::move(results[i]));
    }
  }
  return design;
}

int AntennaChecker::Impl::checkAntennas(odb::dbNet* net,
                                        const int num_threads,
                                        bool verbose)
{
  const DesignCheck design = check(net, num_threads);

  if (verbose) {
    // All nets: only the violating ones.  One net: that net in any case.
    for (const NetCheck& net_check : design.nets) {
      if (net != nullptr || net_check.violated) {
        logger_->report("{}", formatNetCheck(net_check));
      }
    }
  }

  net_violation_count_ = design.violatingNets();
  pin_violation_count_ = design.violatingPins();
  logger_->info(utl::ANT, 2, "Found {} net violations.", net_violation_count_);
  logger_->metric("antenna__violating__nets", net_violation_count_);
  logger_->info(utl::ANT, 1, "Found {} pin violations.", pin_violation_count_);
  logger_->metric("antenna__violating__pins", pin_violation_count_);

  if (!report_file_name_.empty()) {
    std::ofstream report_file(report_file_name_);
    report_file << formatReport(design, verbose);
  }

  return net_violation_count_;
}

int AntennaChecker::Impl::antennaViolationCount() const
{
  return net_violation_count_;
}

int AntennaChecker::Impl::pinViolationCount() const
{
  return pin_violation_count_;
}

bool AntennaChecker::Impl::haveRoutedNets()
{
  for (odb::dbNet* net : block_->getNets()) {
    if (!net->isSpecial() && net->getWireType() == odb::dbWireType::ROUTED
        && net->getWire()) {
      return true;
    }
  }
  return false;
}

void AntennaChecker::Impl::setReportFileName(const char* file_name)
{
  report_file_name_ = file_name;
}

void AntennaChecker::Impl::makeNetWiresFromGuides(
    const std::vector<odb::dbNet*>& nets)
{
  if (block_ == nullptr) {
    block_ = db_->getChip()->getBlock();
  }
  WireBuilder wire_builder(db_, logger_);
  wire_builder.makeNetWiresFromGuides(nets);
}

}  // namespace ant
