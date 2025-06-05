// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "ant/AntennaChecker.hh"

#include <omp.h>

#include <algorithm>
#include <boost/pending/disjoint_sets.hpp>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Polygon.hh"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace ant {

using utl::ANT;

// Abbreviations Index:
//   `PAR`: Partial Area Ratio
//   `CAR`: Cumulative Area Ratio
//   `Area`: Gate Area
//   `S. Area`: Side Diffusion Area
//   `C. Area`: Cumulative Gate Area
//   `C. S. Area`: Cumulative Side (Diffusion) Area

struct AntennaModel
{
  odb::dbTechLayer* layer;

  double metal_factor;
  double diff_metal_factor;

  double cut_factor;
  double diff_cut_factor;

  double side_metal_factor;
  double diff_side_metal_factor;

  double minus_diff_factor;
  double plus_diff_factor;
  double diff_metal_reduce_factor;
};

AntennaChecker::AntennaChecker() = default;
AntennaChecker::~AntennaChecker() = default;

void AntennaChecker::init(odb::dbDatabase* db, utl::Logger* logger)
{
  db_ = db;
  logger_ = logger;
}

void AntennaChecker::initAntennaRules()
{
  block_ = db_->getChip()->getBlock();
  odb::dbTech* tech = db_->getTech();
  // initialize nets_to_report_ with all nets to avoid issues with
  // multithreading
  if (net_to_report_.empty()) {
    for (odb::dbNet* net : block_->getNets()) {
      if (!net->isSpecial()) {
        net_to_report_[net];
      }
    }
  }

  if (!layer_info_.empty()) {
    return;
  }

  for (odb::dbTechLayer* tech_layer : tech->getLayers()) {
    double metal_factor = 1.0;
    double diff_metal_factor = 1.0;

    double cut_factor = 1.0;
    double diff_cut_factor = 1.0;

    double side_metal_factor = 1.0;
    double diff_side_metal_factor = 1.0;

    double minus_diff_factor = 0.0;
    double plus_diff_factor = 0.0;
    double diff_metal_reduce_factor = 1.0;

    if (tech_layer->hasDefaultAntennaRule()) {
      const odb::dbTechLayerAntennaRule* antenna_rule
          = tech_layer->getDefaultAntennaRule();

      if (antenna_rule->isAreaFactorDiffUseOnly()) {
        diff_metal_factor = antenna_rule->getAreaFactor();

        diff_cut_factor = antenna_rule->getAreaFactor();
      } else {
        metal_factor = antenna_rule->getAreaFactor();
        diff_metal_factor = antenna_rule->getAreaFactor();

        cut_factor = antenna_rule->getAreaFactor();
        diff_cut_factor = antenna_rule->getAreaFactor();
      }
      if (antenna_rule->isSideAreaFactorDiffUseOnly()) {
        diff_side_metal_factor = antenna_rule->getSideAreaFactor();
      } else {
        side_metal_factor = antenna_rule->getSideAreaFactor();
        diff_side_metal_factor = antenna_rule->getSideAreaFactor();
      }

      minus_diff_factor = antenna_rule->getAreaMinusDiffFactor();
      plus_diff_factor = antenna_rule->getGatePlusDiffFactor();

      const double PSR_ratio = antenna_rule->getPSR();
      const odb::dbTechLayerAntennaRule::pwl_pair diffPSR
          = antenna_rule->getDiffPSR();

      uint wire_thickness_dbu = 0;
      tech_layer->getThickness(wire_thickness_dbu);

      const odb::dbTechLayerType layerType = tech_layer->getType();

      // If there is a SIDE area antenna rule, then make sure thickness exists.
      if ((PSR_ratio != 0 || !diffPSR.indices.empty())
          && layerType == odb::dbTechLayerType::ROUTING
          && wire_thickness_dbu == 0) {
        logger_->warn(ANT,
                      13,
                      "No THICKNESS is provided for layer {}.  Checks on this "
                      "layer will not be correct.",
                      tech_layer->getConstName());
      }
    }

    AntennaModel layer_antenna = {tech_layer,
                                  metal_factor,
                                  diff_metal_factor,
                                  cut_factor,
                                  diff_cut_factor,
                                  side_metal_factor,
                                  diff_side_metal_factor,
                                  minus_diff_factor,
                                  plus_diff_factor,
                                  diff_metal_reduce_factor};
    layer_info_[tech_layer] = layer_antenna;
  }
}

double AntennaChecker::gateArea(odb::dbMTerm* mterm)
{
  double max_gate_area = 0;
  if (mterm->hasDefaultAntennaModel()) {
    odb::dbTechAntennaPinModel* pin_model = mterm->getDefaultAntennaModel();
    std::vector<std::pair<double, odb::dbTechLayer*>> gate_areas;
    pin_model->getGateArea(gate_areas);

    for (const auto& [gate_area, layer] : gate_areas) {
      max_gate_area = std::max(max_gate_area, gate_area);
    }
  }
  return max_gate_area;
}

double AntennaChecker::getPwlFactor(
    odb::dbTechLayerAntennaRule::pwl_pair pwl_info,
    double ref_value,
    double default_value)
{
  if (!pwl_info.indices.empty()) {
    if (pwl_info.indices.size() == 1) {
      return pwl_info.ratios[0];
    }
    double pwl_info_index1 = pwl_info.indices[0];
    double pwl_info_ratio1 = pwl_info.ratios[0];
    double slope = 1.0;
    for (int i = 0; i < pwl_info.indices.size(); i++) {
      double pwl_info_index2 = pwl_info.indices[i];
      double pwl_info_ratio2 = pwl_info.ratios[i];
      slope = (pwl_info_ratio2 - pwl_info_ratio1)
              / (pwl_info_index2 - pwl_info_index1);

      if (ref_value >= pwl_info_index1 && ref_value < pwl_info_index2) {
        return pwl_info_ratio1 + (ref_value - pwl_info_index1) * slope;
      }
      pwl_info_index1 = pwl_info_index2;
      pwl_info_ratio1 = pwl_info_ratio2;
    }
    return pwl_info_ratio1 + (ref_value - pwl_info_index1) * slope;
  }
  return default_value;
}

void AntennaChecker::saveGates(odb::dbNet* db_net,
                               LayerToGraphNodes& node_by_layer_map,
                               const int node_count)
{
  std::map<PinType, std::vector<int>, PinTypeCmp> pin_nbrs;
  std::vector<int> ids;
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
  // run DSU from min_layer to max_layer
  std::vector<int> dsu_parent(node_count);
  std::vector<int> dsu_size(node_count);
  for (int i = 0; i < node_count; i++) {
    dsu_size[i] = 1;
    dsu_parent[i] = i;
  }

  boost::disjoint_sets<int*, int*> dsu(&dsu_size[0], &dsu_parent[0]);

  odb::dbTech* tech = db_->getTech();
  odb::dbTechLayer* iter = tech->findRoutingLayer(1);
  odb::dbTechLayer* lower_layer;
  while (iter) {
    // iterate each node of this layer to union set
    for (auto& node_it : node_by_layer_map[iter]) {
      int id_u = node_it->id;
      // if has lower layer
      lower_layer = iter->getLowerLayer();
      if (lower_layer) {
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

bool AntennaChecker::isValidGate(odb::dbMTerm* mterm)
{
  return mterm->getIoType() == odb::dbIoType::INPUT && gateArea(mterm) > 0.0;
}

void AntennaChecker::calculateWirePar(odb::dbTechLayer* tech_layer,
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
    info.PAR = (diff_metal_factor * info.area) / info.iterm_gate_area;
    info.PSR = (diff_side_metal_factor * info.side_area) / info.iterm_gate_area;

    // Calculate PSR
    info.diff_PAR
        = (diff_metal_factor * info.area * diff_metal_reduce_factor
           - minus_diff_factor * info.iterm_diff_area)
          / (info.iterm_gate_area + plus_diff_factor * info.iterm_diff_area);
    info.diff_PSR
        = (diff_side_metal_factor * info.side_area * diff_metal_reduce_factor
           - minus_diff_factor * info.iterm_diff_area)
          / (info.iterm_gate_area + plus_diff_factor * info.iterm_diff_area);
  } else {
    // Calculate PAR
    info.PAR = (metal_factor * info.area) / info.iterm_gate_area;
    info.PSR = (side_metal_factor * info.side_area) / info.iterm_gate_area;

    // Calculate PSR
    info.diff_PAR = (metal_factor * info.area * diff_metal_reduce_factor)
                    / info.iterm_gate_area;
    info.diff_PSR
        = (side_metal_factor * info.side_area * diff_metal_reduce_factor)
          / info.iterm_gate_area;
  }
}

void AntennaChecker::calculateViaPar(odb::dbTechLayer* tech_layer,
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
    info.PAR = (diff_cut_factor * info.area) / info.iterm_gate_area;
    // Calculate diff_PAR
    info.diff_PAR
        = (diff_cut_factor * info.area * diff_metal_reduce_factor
           - minus_diff_factor * info.iterm_diff_area)
          / (info.iterm_gate_area + plus_diff_factor * info.iterm_diff_area);
  } else {
    // Calculate PAR
    info.PAR = (cut_factor * info.area) / info.iterm_gate_area;
    // Calculate diff_PAR
    info.diff_PAR = (cut_factor * info.area * diff_metal_reduce_factor)
                    / info.iterm_gate_area;
  }
}

void AntennaChecker::calculateAreas(const LayerToGraphNodes& node_by_layer_map,
                                    GateToLayerToNodeInfo& gate_info)
{
  for (const auto& it : node_by_layer_map) {
    for (const auto& node_it : it.second) {
      NodeInfo info;
      double area = gtl::area(node_it->pol);
      // convert from dbu^2 to microns^2
      area = block_->dbuToMicrons(area);
      area = block_->dbuToMicrons(area);
      info.area = area;
      int gates_count = 0;
      std::vector<odb::dbITerm*> iterms;
      for (const auto& gate : node_it->gates) {
        if (!gate.isITerm) {
          continue;
        }

        if (isValidGate(gate.iterm->getMTerm())) {
          info.iterms.push_back(gate.iterm);
        }
        info.iterm_gate_area += gateArea(gate.iterm->getMTerm());
        info.iterm_diff_area += diffArea(gate.iterm->getMTerm());
        gates_count++;
      }
      if (gates_count == 0) {
        continue;
      }

      if (it.first->getRoutingLevel() != 0) {
        // Calculate side area of wire
        uint wire_thickness_dbu = 0;
        it.first->getThickness(wire_thickness_dbu);
        double wire_thickness = block_->dbuToMicrons(wire_thickness_dbu);
        info.side_area = block_->dbuToMicrons(gtl::perimeter(node_it->pol)
                                              * wire_thickness);
      }
      // put values on struct
      for (const auto& gate : node_it->gates) {
        if (!gate.isITerm) {
          continue;
        }
        if (!isValidGate(gate.iterm->getMTerm())) {
          continue;
        }
        // check if has another node with gate in the layer, then merge area
        if (gate_info[gate.iterm].find(it.first)
            != gate_info[gate.iterm].end()) {
          gate_info[gate.iterm][it.first] += info;
        } else {
          gate_info[gate.iterm][it.first] = info;
        }
      }
    }
  }
}

// calculate PAR and PSR of wires and vias
void AntennaChecker::calculatePAR(GateToLayerToNodeInfo& gate_info)
{
  for (auto& gate_it : gate_info) {
    for (auto& layer_it : gate_it.second) {
      NodeInfo& gate_info = layer_it.second;
      odb::dbTechLayer* tech_layer = layer_it.first;
      NodeInfo info;
      if (tech_layer->getRoutingLevel() == 0) {
        calculateViaPar(tech_layer, gate_info);
      } else {
        calculateWirePar(tech_layer, gate_info);
      }
    }
  }
}

// calculate CAR and CSR of wires and vias
void AntennaChecker::calculateCAR(GateToLayerToNodeInfo& gate_info)
{
  for (auto& [gate, layer_to_node_info] : gate_info) {
    // Variables to store the accumulated values for vias and wires
    NodeInfo sumWire, sumVia;
    // iterate from first_layer -> last layer, cumulate sum for wires and vias
    odb::dbTech* tech = db_->getTech();
    odb::dbTechLayer* iter_layer = tech->findRoutingLayer(1);
    while (iter_layer) {
      if (layer_to_node_info.find(iter_layer) != layer_to_node_info.end()) {
        NodeInfo& node_info = layer_to_node_info[iter_layer];
        if (iter_layer->getRoutingLevel() == 0) {
          // Accumulating the PAR of vias in sumVia
          sumVia += node_info;
          // Updating the node with the accumulated values
          node_info.CAR += sumVia.PAR;
          node_info.CSR += sumVia.PSR;
          node_info.diff_CAR += sumVia.diff_PAR;
          node_info.diff_CSR += sumVia.diff_PSR;
        } else {
          // Accumulating the PAR of wires in sumWire
          sumWire += node_info;
          // Updating the node with the accumulated values
          node_info.CAR += sumWire.PAR;
          node_info.CSR += sumWire.PSR;
          node_info.diff_CAR += sumWire.diff_PAR;
          node_info.diff_CSR += sumWire.diff_PSR;
        }
      }
      iter_layer = iter_layer->getUpperLayer();
    }
  }
}

bool AntennaChecker::checkPAR(odb::dbNet* db_net,
                              odb::dbTechLayer* tech_layer,
                              NodeInfo& info,
                              const float ratio_margin,
                              bool verbose,
                              bool report,
                              ViolationReport& net_report)
{
  // get rules
  const odb::dbTechLayerAntennaRule* antenna_rule
      = tech_layer->getDefaultAntennaRule();
  double PAR_ratio = antenna_rule->getPAR();
  odb::dbTechLayerAntennaRule::pwl_pair diffPAR = antenna_rule->getDiffPAR();
  double diff_PAR_PWL_ratio = getPwlFactor(diffPAR, info.iterm_diff_area, 0.0);

  // apply ratio_margin
  PAR_ratio *= (1.0 - ratio_margin / 100.0);
  diff_PAR_PWL_ratio *= (1.0 - ratio_margin / 100.0);

  bool violation = false;
  double calculated_value = 0.0;
  double required_value = 0.0;
  // If node is connected to diffusion area or ANTENNAAREARATIO is not
  // defined, compare with ANTENNADIFFAREARATIO. Otherwise compare with
  // ANTENNAAREARATIO.
  if (info.iterm_diff_area != 0 || PAR_ratio == 0) {
    if (diff_PAR_PWL_ratio != 0) {
      violation = info.diff_PAR > diff_PAR_PWL_ratio;
      info.excess_ratio_PAR
          = std::max(info.excess_ratio_PAR, info.diff_PAR / diff_PAR_PWL_ratio);
    }
    calculated_value = info.diff_PAR;
    required_value = diff_PAR_PWL_ratio;
  } else {
    if (PAR_ratio != 0) {
      violation = info.PAR > PAR_ratio;
      info.excess_ratio_PAR
          = std::max(info.excess_ratio_PAR, info.PAR / PAR_ratio);
    }
    calculated_value = info.PAR;
    required_value = PAR_ratio;
  }

  if (report) {
    std::string par_report = fmt::format(
        "      Partial area ratio: {:7.2f}\n      Required ratio: "
        "{:7.2f} "
        "(Gate area) {}",
        calculated_value,
        required_value,
        violation ? "(VIOLATED)" : "");
    net_report.report += par_report + "\n";
  }

  return violation;
}

bool AntennaChecker::checkPSR(odb::dbNet* db_net,
                              odb::dbTechLayer* tech_layer,
                              NodeInfo& info,
                              const float ratio_margin,
                              bool verbose,
                              bool report,
                              ViolationReport& net_report)
{
  // get rules
  const odb::dbTechLayerAntennaRule* antenna_rule
      = tech_layer->getDefaultAntennaRule();
  double PSR_ratio = antenna_rule->getPSR();
  const odb::dbTechLayerAntennaRule::pwl_pair diffPSR
      = antenna_rule->getDiffPSR();
  double diff_PSR_PWL_ratio = getPwlFactor(diffPSR, info.iterm_diff_area, 0.0);

  // apply ratio_margin
  PSR_ratio *= (1.0 - ratio_margin / 100.0);
  diff_PSR_PWL_ratio *= (1.0 - ratio_margin / 100.0);

  bool violation = false;
  double calculated_value = 0.0;
  double required_value = 0.0;
  // If node is connected to diffusion area or ANTENNASIDEAREARATIO is not
  // defined, compare with ANTENNADIFFSIDEAREARATIO. Otherwise compare with
  // ANTENNASIDEAREARATIO.
  if (info.iterm_diff_area != 0 || PSR_ratio == 0) {
    if (diff_PSR_PWL_ratio != 0) {
      violation = info.diff_PSR > diff_PSR_PWL_ratio;
      info.excess_ratio_PSR
          = std::max(info.excess_ratio_PSR, info.diff_PSR / diff_PSR_PWL_ratio);
    }
    calculated_value = info.diff_PSR;
    required_value = diff_PSR_PWL_ratio;
  } else {
    if (PSR_ratio != 0) {
      violation = info.PSR > PSR_ratio;
      info.excess_ratio_PSR
          = std::max(info.excess_ratio_PSR, info.PSR / PSR_ratio);
    }
    calculated_value = info.PSR;
    required_value = PSR_ratio;
  }

  if (report) {
    std::string psr_report = fmt::format(
        "      Partial area ratio: {:7.2f}\n      Required ratio: "
        "{:7.2f} "
        "(Side area) {}",
        calculated_value,
        required_value,
        violation ? "(VIOLATED)" : "");
    net_report.report += psr_report + "\n";
  }
  return violation;
}

bool AntennaChecker::checkCAR(odb::dbNet* db_net,
                              odb::dbTechLayer* tech_layer,
                              const NodeInfo& info,
                              bool verbose,
                              bool report,
                              ViolationReport& net_report)
{
  // get rules
  const odb::dbTechLayerAntennaRule* antenna_rule
      = tech_layer->getDefaultAntennaRule();
  const double CAR_ratio = antenna_rule->getCAR();
  const odb::dbTechLayerAntennaRule::pwl_pair diffCAR
      = antenna_rule->getDiffCAR();
  const double diff_CAR_PWL_ratio
      = getPwlFactor(diffCAR, info.iterm_diff_area, 0);

  bool violation = false;
  double calculated_value = 0.0;
  double required_value = 0.0;
  // If node is connected to diffusion area or ANTENNACUMAREARATIO is not
  // defined, compare with ANTENNACUMDIFFAREARATIO. Otherwise compare with
  // ANTENNACUMAREARATIO.
  if (info.iterm_diff_area != 0 || CAR_ratio == 0) {
    if (diff_CAR_PWL_ratio != 0) {
      violation = info.diff_CAR > diff_CAR_PWL_ratio;
    }
    calculated_value = info.diff_CAR;
    required_value = diff_CAR_PWL_ratio;
  } else {
    if (CAR_ratio != 0) {
      violation = info.CAR > CAR_ratio;
    }
    calculated_value = info.CAR;
    required_value = CAR_ratio;
  }

  if (report) {
    std::string car_report = fmt::format(
        "      Cumulative area ratio: {:7.2f}\n      Required ratio: "
        "{:7.2f} "
        "(Cumulative area) {}",
        calculated_value,
        required_value,
        violation ? "(VIOLATED)" : "");
    net_report.report += car_report + "\n";
  }
  return violation;
}

bool AntennaChecker::checkCSR(odb::dbNet* db_net,
                              odb::dbTechLayer* tech_layer,
                              const NodeInfo& info,
                              bool verbose,
                              bool report,
                              ViolationReport& net_report)
{
  // get rules
  const odb::dbTechLayerAntennaRule* antenna_rule
      = tech_layer->getDefaultAntennaRule();
  const double CSR_ratio = antenna_rule->getCSR();
  const odb::dbTechLayerAntennaRule::pwl_pair diffCSR
      = antenna_rule->getDiffCSR();
  const double diff_CSR_PWL_ratio
      = getPwlFactor(diffCSR, info.iterm_diff_area, 0);

  bool violation = false;
  double calculated_value = 0.0;
  double required_value = 0.0;
  // If node is connected to diffusion area or ANTENNACUMSIDEAREARATIO is not
  // defined, compare with ANTENNACUMDIFFSIDEAREARATIO. Otherwise compare with
  // ANTENNACUMSIDEAREARATIO.
  if (info.iterm_diff_area != 0 || CSR_ratio == 0) {
    if (diff_CSR_PWL_ratio != 0) {
      violation = info.diff_CSR > diff_CSR_PWL_ratio;
    }
    calculated_value = info.diff_CSR;
    required_value = diff_CSR_PWL_ratio;
  } else {
    if (CSR_ratio != 0) {
      violation = info.CSR > CSR_ratio;
    }
    calculated_value = info.CSR;
    required_value = CSR_ratio;
  }

  if (report) {
    std::string csr_report = fmt::format(
        "      Cumulative area ratio: {:7.2f}\n      Required ratio: "
        "{:7.2f} "
        "(Cumulative side area) {}",
        calculated_value,
        required_value,
        violation ? "(VIOLATED)" : "");
    net_report.report += csr_report + "\n";
  }
  return violation;
}

bool AntennaChecker::checkRatioViolations(odb::dbNet* db_net,
                                          odb::dbTechLayer* layer,
                                          NodeInfo& node_info,
                                          const float ratio_margin,
                                          bool verbose,
                                          bool report,
                                          ViolationReport& net_report)
{
  bool node_has_violation
      = checkPAR(
            db_net, layer, node_info, ratio_margin, verbose, report, net_report)
        || checkCAR(db_net, layer, node_info, verbose, report, net_report);
  if (layer->getRoutingLevel() != 0) {
    bool psr_violation = checkPSR(
        db_net, layer, node_info, ratio_margin, verbose, report, net_report);
    bool csr_violation
        = checkCSR(db_net, layer, node_info, verbose, report, net_report);
    node_has_violation = node_has_violation || psr_violation || csr_violation;
  }

  return node_has_violation;
}

void AntennaChecker::writeReport(std::ofstream& report_file, bool verbose)
{
  std::lock_guard<std::mutex> lock(map_mutex_);
  for (const auto& [net, violation_report] : net_to_report_) {
    if (verbose || violation_report.violated) {
      report_file << violation_report.report;
    }
  }
}

void AntennaChecker::printReport(odb::dbNet* db_net)
{
  if (db_net) {
    logger_->report("{}", net_to_report_[db_net].report);
  } else {
    std::lock_guard<std::mutex> lock(map_mutex_);
    for (const auto& [net, violation_report] : net_to_report_) {
      if (violation_report.violated) {
        logger_->report("{}", violation_report.report);
      }
    }
  }
}

int AntennaChecker::checkGates(odb::dbNet* db_net,
                               bool verbose,
                               bool save_report,
                               odb::dbMTerm* diode_mterm,
                               float ratio_margin,
                               GateToLayerToNodeInfo& gate_info,
                               Violations& antenna_violations)
{
  int pin_violation_count = 0;

  GateToViolationLayers gates_with_violations;

  ViolationReport net_report;
  std::string net_name = fmt::format("Net: {}", db_net->getConstName());
  net_report.report += net_name + "\n";

  for (auto& [node, layer_to_node] : gate_info) {
    bool pin_has_violation = false;

    odb::dbMTerm* mterm = node->getMTerm();
    std::string pin_name = fmt::format("  Pin:   {}/{} ({})",
                                       node->getInst()->getConstName(),
                                       mterm->getConstName(),
                                       mterm->getMaster()->getConstName());
    net_report.report += pin_name + "\n";

    for (auto& [layer, node_info] : layer_to_node) {
      if (layer->hasDefaultAntennaRule()) {
        std::string layer_name
            = fmt::format("    Layer: {}", layer->getConstName());
        net_report.report += layer_name + "\n";

        bool node_has_violation = checkRatioViolations(
            db_net, layer, node_info, ratio_margin, verbose, true, net_report);

        net_report.report += "\n";
        if (node_has_violation) {
          pin_has_violation = true;
          gates_with_violations[node].insert(layer);
          net_report.violated = true;
        }
      }
    }
    if (pin_has_violation) {
      pin_violation_count++;
    }
    net_report.report += "\n";
  }
  // Write report on map
  if (save_report) {
    std::lock_guard<std::mutex> lock(map_mutex_);
    net_to_report_.at(db_net) = net_report;
  }

  std::unordered_map<odb::dbITerm*, int> num_diodes_added;
  std::map<odb::dbTechLayer*, std::set<odb::dbITerm*>> pin_added;
  // if checkGates is used by repair antennas
  if (pin_violation_count > 0) {
    for (const auto& [gate, violation_layers] : gates_with_violations) {
      for (odb::dbTechLayer* layer : violation_layers) {
        // when repair antenna is running, calculate number of diodes
        if (pin_added[layer].find(gate) == pin_added[layer].end()) {
          double diode_diff_area = 0.0;
          if (diode_mterm) {
            diode_diff_area = diffArea(diode_mterm);
          }
          NodeInfo violation_info = gate_info[gate][layer];
          std::vector<odb::dbITerm*> gates = violation_info.iterms;
          odb::dbTechLayer* violation_layer = layer;
          int diode_count_per_gate = 0;
          // check violations only PAR & PSR
          bool par_violation = checkPAR(db_net,
                                        violation_layer,
                                        violation_info,
                                        ratio_margin,
                                        false,
                                        false,
                                        net_report);
          bool psr_violation = false;
          // Only routing layers have side areas
          if (violation_layer->getRoutingLevel() != 0) {
            psr_violation = checkPSR(db_net,
                                     violation_layer,
                                     violation_info,
                                     ratio_margin,
                                     false,
                                     false,
                                     net_report);
          }
          bool violated = par_violation || psr_violation;
          double excess_ratio = 1.0;
          if (violated) {
            excess_ratio = std::max(violation_info.excess_ratio_PAR,
                                    violation_info.excess_ratio_PSR);
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
              par_violation = checkPAR(db_net,
                                       violation_layer,
                                       violation_info,
                                       ratio_margin,
                                       false,
                                       false,
                                       net_report);
              // Only routing layers have side areas
              if (violation_layer->getRoutingLevel() != 0) {
                psr_violation = checkPSR(db_net,
                                         violation_layer,
                                         violation_info,
                                         ratio_margin,
                                         false,
                                         false,
                                         net_report);
              }
              if (diode_count_per_gate > max_diode_count_per_gate) {
                debugPrint(logger_,
                           ANT,
                           "check_gates",
                           1,
                           "Net {} requires more than {} diodes per gate to "
                           "repair violations.",
                           db_net->getConstName(),
                           max_diode_count_per_gate);
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

          bool car_violation = checkCAR(db_net,
                                        violation_layer,
                                        violation_info,
                                        false,
                                        false,
                                        net_report);
          bool csr_violation = checkCSR(db_net,
                                        violation_layer,
                                        violation_info,
                                        false,
                                        false,
                                        net_report);

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

void AntennaChecker::buildLayerMaps(odb::dbNet* db_net,
                                    LayerToGraphNodes& node_by_layer_map)
{
  odb::dbWire* wires = db_net->getWire();

  std::map<odb::dbTechLayer*, PolygonSet> set_by_layer;

  wiresToPolygonSetMap(wires, set_by_layer);
  avoidPinIntersection(db_net, set_by_layer);

  int node_count = 0;
  for (const auto& layer_it : set_by_layer) {
    for (const auto& pol_it : layer_it.second) {
      bool isVia = layer_it.first->getRoutingLevel() == 0;
      node_by_layer_map[layer_it.first].push_back(
          std::make_unique<GraphNode>(node_count, isVia, pol_it));
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

        if (upper_index.size() <= 2) {
          // connect upper -> via
          for (int& up_index : upper_index) {
            node_by_layer_map[layer_it.first->getUpperLayer()][up_index]
                ->low_adj.push_back(via_index);
          }
        } else if (upper_index.size() > 2) {
          std::string log_error = fmt::format(
              "ERROR: net {} has via on {} conect with multiple wires on layer "
              "{} \n",
              db_net->getConstName(),
              layer_it.first->getName(),
              layer_it.first->getUpperLayer()->getName());
          logger_->report("{}", log_error);
        }
        if (lower_index.size() == 1) {
          // connect via -> lower
          for (int& low_index : lower_index) {
            node_by_layer_map[layer_it.first][via_index]->low_adj.push_back(
                low_index);
          }
        } else if (lower_index.size() > 2) {
          std::string log_error = fmt::format(
              "ERROR: net {} has via on {} conect with multiple wires on layer "
              "{} \n",
              db_net->getConstName(),
              layer_it.first->getName(),
              layer_it.first->getLowerLayer()->getName());
          logger_->report("{}", log_error);
        }
        via_index++;
      }
    }
  }
  saveGates(db_net, node_by_layer_map, node_count);
}

void AntennaChecker::makeNetWiresFromGuides()
{
  logger_->report("Making net wires from guides");
  std::map<odb::dbTechLayer*, odb::dbTechVia*> default_vias
      = block_->getDefaultVias();
  const int guide_dimension = computeGuideDimension();
  for (odb::dbNet* db_net : block_->getNets()) {
    const bool is_detailed_routed
        = db_net->getWireType() == odb::dbWireType::ROUTED && db_net->getWire();

    if (!db_net->isSpecial() && !db_net->isConnectedByAbutment()
        && !is_detailed_routed) {
      makeNetWire(db_net, default_vias, guide_dimension);
    }
  }
}

void AntennaChecker::makeNetWire(
    odb::dbNet* db_net,
    std::map<odb::dbTechLayer*, odb::dbTechVia*> default_vias,
    const int guide_dimension)
{
  odb::dbWire* wire = odb::dbWire::create(db_net);
  if (wire) {
    odb::dbTech* tech = db_->getTech();
    odb::dbWireEncoder wire_encoder;
    wire_encoder.begin(wire);
    std::vector<GuideSegment> route
        = makeWireFromGuides(db_net, guide_dimension);
    GuidePtPinsMap route_pt_pins;  //= findGuidePointPins(net);
    std::unordered_set<GuideSegment, GuideSegmentHash> wire_segments;
    int prev_conn_layer = -1;
    for (GuideSegment& seg : route) {
      int l1 = seg.pt1.layer->getRoutingLevel();
      int l2 = seg.pt2.layer->getRoutingLevel();
      odb::dbTechLayer* bottom_tech_layer
          = seg.pt1.layer->getRoutingLevel() < seg.pt2.layer->getRoutingLevel()
                ? seg.pt1.layer
                : seg.pt2.layer;
      odb::dbTechLayer* top_tech_layer
          = seg.pt1.layer->getRoutingLevel() > seg.pt2.layer->getRoutingLevel()
                ? seg.pt1.layer
                : seg.pt2.layer;

      if (std::abs(l1 - l2) > 1) {
        debugPrint(logger_,
                   ANT,
                   "make_net_wire",
                   1,
                   "invalid seg: ({}, {})um to ({}, {})um",
                   block_->dbuToMicrons(seg.pt1.pos.getX()),
                   block_->dbuToMicrons(seg.pt1.pos.getY()),
                   block_->dbuToMicrons(seg.pt2.pos.getX()),
                   block_->dbuToMicrons(seg.pt2.pos.getY()));

        logger_->error(ANT,
                       15,
                       "Global route segment for net {} not "
                       "valid. The layers {} and {} "
                       "are not adjacent.",
                       db_net->getName(),
                       bottom_tech_layer->getName(),
                       top_tech_layer->getName());
      }
      if (wire_segments.find(seg) == wire_segments.end()) {
        int x1 = seg.pt1.pos.getX();
        int y1 = seg.pt1.pos.getY();
        if (seg.isVia()) {
          if (bottom_tech_layer->getRoutingLevel()
              >= block_->getMinRoutingLayer()) {
            if (bottom_tech_layer->getRoutingLevel() == prev_conn_layer) {
              wire_encoder.newPath(bottom_tech_layer, odb::dbWireType::ROUTED);
              prev_conn_layer = std::max(l1, l2);
            } else if (top_tech_layer->getRoutingLevel() == prev_conn_layer) {
              wire_encoder.newPath(top_tech_layer, odb::dbWireType::ROUTED);
              prev_conn_layer = std::min(l1, l2);
            } else {
              // if a via is the first object added to the wire_encoder, or the
              // via starts a new path and is not connected to previous
              // wires create a new path using the bottom layer and do not
              // update the prev_conn_layer. this way, this process is repeated
              // until the first wire is added and properly update the
              // prev_conn_layer
              wire_encoder.newPath(bottom_tech_layer, odb::dbWireType::ROUTED);
            }

            wire_encoder.addPoint(x1, y1);
            wire_encoder.addTechVia(default_vias[bottom_tech_layer]);
            addWireTerms(db_net,
                         route,
                         x1,
                         y1,
                         bottom_tech_layer,
                         route_pt_pins,
                         wire_encoder,
                         default_vias,
                         false);
            wire_segments.insert(seg);
          }
        } else {
          // Add wire
          int x2 = seg.pt2.pos.getX();
          int y2 = seg.pt2.pos.getY();
          if (x1 != x2 || y1 != y2) {
            odb::dbTechLayer* tech_layer = tech->findRoutingLayer(l1);
            addWireTerms(db_net,
                         route,
                         x1,
                         y1,
                         tech_layer,
                         route_pt_pins,
                         wire_encoder,
                         default_vias,
                         true);
            wire_encoder.newPath(tech_layer, odb::dbWireType::ROUTED);
            wire_encoder.addPoint(x1, y1);
            wire_encoder.addPoint(x2, y2);
            addWireTerms(db_net,
                         route,
                         x2,
                         y2,
                         tech_layer,
                         route_pt_pins,
                         wire_encoder,
                         default_vias,
                         true);
            wire_segments.insert(seg);
            prev_conn_layer = l1;
          }
        }
      }
    }
    wire_encoder.end();
  } else {
    logger_->error(
        ANT, 16, "Cannot create wire for net {}.", db_net->getConstName());
  }
}

void AntennaChecker::addWireTerms(
    odb::dbNet* db_net,
    std::vector<GuideSegment>& route,
    int grid_x,
    int grid_y,
    odb::dbTechLayer* tech_layer,
    GuidePtPinsMap& route_pt_pins,
    odb::dbWireEncoder& wire_encoder,
    std::map<odb::dbTechLayer*, odb::dbTechVia*>& default_vias,
    bool connect_to_segment)
{
  std::vector<int> layers;
  int layer = tech_layer->getRoutingLevel();
  layers.push_back(layer);
  if (layer == block_->getMinRoutingLayer()) {
    layer--;
    layers.push_back(layer);
  }

  for (int l : layers) {
    odb::dbTech* tech = db_->getTech();
    GuidePoint guide_pt;
    guide_pt.pos = odb::Point(grid_x, grid_y);
    guide_pt.layer = tech->findRoutingLayer(l);
    ;
    auto itr = route_pt_pins.find(guide_pt);
    if (itr != route_pt_pins.end() && !itr->second.connected) {
      for (odb::dbBTerm* bterm : itr->second.bterms) {
        itr->second.connected = true;
        odb::dbTechLayer* conn_layer;      // = bterm->getBPins();
        std::vector<odb::Rect> pin_boxes;  // = pin->getBoxes().at(conn_layer);
        odb::Point grid_pt;                // = pin->getOnGridPosition();
        odb::Point pin_pt;                 // = grid_pt;
        // create the local connection with the pin center only when the global
        // segment doesn't overlap the pin
        if (!pinOverlapsGSegment(grid_pt, conn_layer, pin_boxes, route)) {
          int min_dist = std::numeric_limits<int>::max();
          for (const odb::Rect& pin_box : pin_boxes) {
            odb::Point pos = pin_box.center();
            int dist = odb::Point::manhattanDistance(pos, pin_pt);
            if (dist < min_dist) {
              min_dist = dist;
              pin_pt = pos;
            }
          }
        }

        if (conn_layer->getRoutingLevel() >= block_->getMinRoutingLayer()) {
          wire_encoder.newPath(tech_layer, odb::dbWireType::ROUTED);
          wire_encoder.addPoint(grid_pt.x(), grid_pt.y());
          wire_encoder.addPoint(pin_pt.x(), grid_pt.y());
          wire_encoder.addPoint(pin_pt.x(), pin_pt.y());
        } else {
          odb::dbTechLayer* min_layer
              = tech->findRoutingLayer(block_->getMinRoutingLayer());

          if (connect_to_segment && tech_layer != min_layer) {
            // create vias to connect the guide segment to the min routing
            // layer. the min routing layer will be used to connect to the pin.
            wire_encoder.newPath(tech_layer, odb::dbWireType::ROUTED);
            wire_encoder.addPoint(grid_pt.x(), grid_pt.y());
            for (int i = min_layer->getRoutingLevel();
                 i < tech_layer->getRoutingLevel();
                 i++) {
              odb::dbTechLayer* l = tech->findRoutingLayer(i);
              wire_encoder.addTechVia(default_vias[l]);
            }
          }

          if (min_layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
            makeWire(wire_encoder,
                     min_layer,
                     grid_pt,
                     odb::Point(grid_pt.x(), pin_pt.y()));

            wire_encoder.addTechVia(default_vias[min_layer]);
            makeWire(wire_encoder,
                     min_layer,
                     odb::Point(grid_pt.x(), pin_pt.y()),
                     pin_pt);
          } else {
            makeWire(wire_encoder,
                     min_layer,
                     grid_pt,
                     odb::Point(pin_pt.x(), grid_pt.y()));
            wire_encoder.addTechVia(default_vias[min_layer]);
            makeWire(wire_encoder,
                     min_layer,
                     odb::Point(pin_pt.x(), grid_pt.y()),
                     pin_pt);
          }

          // create vias to reach the pin
          for (int i = min_layer->getRoutingLevel() - 1;
               i >= conn_layer->getRoutingLevel();
               i--) {
            odb::dbTechLayer* l = tech->findRoutingLayer(i);
            wire_encoder.addTechVia(default_vias[l]);
          }
        }
      }
    }
  }
}

void AntennaChecker::makeWire(odb::dbWireEncoder& wire_encoder,
                              odb::dbTechLayer* layer,
                              const odb::Point& start,
                              const odb::Point& end)
{
  wire_encoder.newPath(layer, odb::dbWireType::ROUTED);
  wire_encoder.addPoint(start.x(), start.y());
  wire_encoder.addPoint(end.x(), end.y());
}

bool AntennaChecker::pinOverlapsGSegment(
    const odb::Point& pin_position,
    const odb::dbTechLayer* pin_layer,
    const std::vector<odb::Rect>& pin_boxes,
    const std::vector<GuideSegment>& route)
{
  // check if pin position on grid overlaps with the pin shape
  for (const odb::Rect& box : pin_boxes) {
    if (box.overlaps(pin_position)) {
      return true;
    }
  }

  // check if pin position on grid overlaps with at least one GSegment
  for (const odb::Rect& box : pin_boxes) {
    for (const GuideSegment& seg : route) {
      if (seg.pt1.layer == seg.pt2.layer &&  // ignore vias
          seg.pt1.layer == pin_layer) {
        auto [x0, x1] = std::minmax(seg.pt1.pos.getX(), seg.pt2.pos.getX());
        auto [y0, y1] = std::minmax(seg.pt1.pos.getY(), seg.pt2.pos.getY());
        odb::Rect seg_rect(x0, y0, x1, y1);

        if (box.intersects(seg_rect)) {
          return true;
        }
      }
    }
  }

  return false;
}

std::vector<GuideSegment> AntennaChecker::makeWireFromGuides(
    odb::dbNet* db_net,
    const int guide_dimension)
{
  std::vector<GuideSegment> route;
  for (odb::dbGuide* guide : db_net->getGuides()) {
    odb::dbTechLayer* layer = guide->getLayer();
    odb::dbTechLayer* via_layer = guide->getViaLayer();
    boxToGuideSegment(
        guide->getBox(), layer, via_layer, route, guide_dimension);
  }

  return route;
}

int AntennaChecker::computeGuideDimension()
{
  for (odb::dbNet* db_net : block_->getNets()) {
    for (odb::dbGuide* guide : db_net->getGuides()) {
      if (guide->getBox().dx() == guide->getBox().dy()) {
        return guide->getBox().dx();
      }
    }
  }

  return 0;
}

void AntennaChecker::boxToGuideSegment(const odb::Rect& guide_box,
                                       odb::dbTechLayer* layer,
                                       odb::dbTechLayer* via_layer,
                                       std::vector<GuideSegment>& route,
                                       const int guide_dimension)
{
  int x0 = (guide_dimension * (guide_box.xMin() / guide_dimension))
           + (guide_dimension / 2);
  int y0 = (guide_dimension * (guide_box.yMin() / guide_dimension))
           + (guide_dimension / 2);

  const int x1 = (guide_dimension * (guide_box.xMax() / guide_dimension))
                 - (guide_dimension / 2);
  const int y1 = (guide_dimension * (guide_box.yMax() / guide_dimension))
                 - (guide_dimension / 2);

  if (x0 == x1 && y0 == y1) {
    const GuideSegment seg
        = GuideSegment{GuidePoint{odb::Point(x0, y0), layer},
                       GuidePoint{odb::Point(x1, y1), via_layer}};
    route.push_back(seg);
  }

  while (y0 == y1 && (x0 + guide_dimension) <= x1) {
    const GuideSegment seg
        = GuideSegment{GuidePoint{odb::Point(x0, y0), layer},
                       GuidePoint{odb::Point(x0 + guide_dimension, y0), layer}};
    route.push_back(seg);
    x0 += guide_dimension;
  }

  while (x0 == x1 && (y0 + guide_dimension) <= y1) {
    const GuideSegment seg
        = GuideSegment{GuidePoint{odb::Point(x0, y0), layer},
                       GuidePoint{odb::Point(x0, y0 + guide_dimension), layer}};
    route.push_back(seg);
    y0 += guide_dimension;
  }
}

int AntennaChecker::checkNet(odb::dbNet* db_net,
                             bool verbose,
                             bool save_report,
                             odb::dbMTerm* diode_mterm,
                             float ratio_margin,
                             Violations& antenna_violations)
{
  odb::dbWire* wire = db_net->getWire();
  int pin_violations = 0;
  if (wire) {
    LayerToGraphNodes node_by_layer_map;
    GateToLayerToNodeInfo gate_info;
    buildLayerMaps(db_net, node_by_layer_map);

    calculateAreas(node_by_layer_map, gate_info);

    calculatePAR(gate_info);
    calculateCAR(gate_info);

    pin_violations = checkGates(db_net,
                                verbose,
                                save_report,
                                diode_mterm,
                                ratio_margin,
                                gate_info,
                                antenna_violations);
  }
  return pin_violations;
}

Violations AntennaChecker::getAntennaViolations(odb::dbNet* net,
                                                odb::dbMTerm* diode_mterm,
                                                float ratio_margin)
{
  Violations antenna_violations;
  if (net->isSpecial()) {
    return antenna_violations;
  }

  checkNet(net, false, false, diode_mterm, ratio_margin, antenna_violations);

  return antenna_violations;
}

bool AntennaChecker::designIsPlaced()
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

bool AntennaChecker::haveGuides()
{
  if (!designIsPlaced()) {
    return false;
  }

  for (odb::dbNet* net : block_->getNets()) {
    if (!net->isSpecial() && net->getGuides().empty()) {
      return false;
    }
  }

  return true;
}

int AntennaChecker::checkAntennas(odb::dbNet* net,
                                  const int num_threads,
                                  bool verbose)
{
  {
    std::lock_guard<std::mutex> lock(map_mutex_);
    net_to_report_.clear();
  }
  initAntennaRules();

  std::ofstream report_file;
  if (!report_file_name_.empty()) {
    report_file.open(report_file_name_, std::ofstream::out);
  }

  bool drt_routes = haveRoutedNets();
  bool grt_routes = false;
  if (!drt_routes) {
    grt_routes = haveGuides();
  }
  bool use_grt_routes = (grt_routes && !drt_routes);
  if (!grt_routes && !drt_routes) {
    logger_->error(ANT,
                   8,
                   "No detailed or global routing found. Run global_route or "
                   "detailed_route first.");
  }

  if (use_grt_routes) {
    makeNetWiresFromGuides();
  }

  int net_violation_count = 0;
  int pin_violation_count = 0;

  if (net) {
    Violations antenna_violations;
    if (!net->isSpecial()) {
      pin_violation_count
          += checkNet(net, verbose, true, nullptr, 0, antenna_violations);
      if (pin_violation_count > 0) {
        net_violation_count++;
      }
    } else {
      logger_->error(
          ANT, 14, "Skipped net {} because it is special.", net->getName());
    }
  } else {
    nets_.clear();
    for (odb::dbNet* net : block_->getNets()) {
      if (!net->isSpecial()) {
        nets_.push_back(net);
      }
    }
    omp_set_num_threads(num_threads);
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < nets_.size(); i++) {
      odb::dbNet* net = nets_[i];
      Violations antenna_violations;
      int pin_viol_count
          = checkNet(net, verbose, true, nullptr, 0, antenna_violations);
      if (pin_viol_count > 0) {
        std::lock_guard<std::mutex> lock(map_mutex_);
        net_violation_count++;
        pin_violation_count += pin_viol_count;
      }
    }
  }

  if (verbose) {
    printReport(net);
  }

  logger_->info(ANT, 2, "Found {} net violations.", net_violation_count);
  logger_->metric("antenna__violating__nets", net_violation_count);
  logger_->info(ANT, 1, "Found {} pin violations.", pin_violation_count);
  logger_->metric("antenna__violating__pins", pin_violation_count);

  if (!report_file_name_.empty()) {
    writeReport(report_file, verbose);
    report_file.close();
  }

  if (use_grt_routes) {
    block_->destroyNetWires();
  }

  net_violation_count_ = net_violation_count;
  return net_violation_count;
}

int AntennaChecker::antennaViolationCount() const
{
  return net_violation_count_;
}

bool AntennaChecker::haveRoutedNets()
{
  for (odb::dbNet* net : block_->getNets()) {
    if (!net->isSpecial() && net->getWireType() == odb::dbWireType::ROUTED
        && net->getWire()) {
      return true;
    }
  }
  return false;
}

double AntennaChecker::diffArea(odb::dbMTerm* mterm)
{
  double max_diff_area = 0.0;
  std::vector<std::pair<double, odb::dbTechLayer*>> diff_areas;
  mterm->getDiffArea(diff_areas);
  for (const auto& [area, layer] : diff_areas) {
    max_diff_area = std::max(max_diff_area, area);
  }
  return max_diff_area;
}

void AntennaChecker::setReportFileName(const char* file_name)
{
  report_file_name_ = file_name;
}

bool operator<(const GuidePoint& pt1, const GuidePoint& pt2)
{
  return (pt1.pos.getX() < pt2.pos.getX())
         || (pt1.pos.getX() == pt2.pos.getX()
             && pt1.pos.getY() < pt2.pos.getY())
         || (pt1.pos.getX() == pt2.pos.getX()
             && pt1.pos.getY() == pt2.pos.getY()
             && pt1.layer->getRoutingLevel() < pt2.layer->getRoutingLevel());
}

bool GuideSegment::operator==(const GuideSegment& segment) const
{
  return pt1.layer == segment.pt1.layer && pt2.layer == segment.pt2.layer
         && pt1.pos == segment.pt1.pos && pt2.pos == segment.pt2.pos;
}

std::size_t GuideSegmentHash::operator()(const GuideSegment& seg) const
{
  return boost::hash<std::tuple<int, int, int, int, int, int>>()(
      {seg.pt1.pos.getX(),
       seg.pt1.pos.getY(),
       seg.pt1.layer->getRoutingLevel(),
       seg.pt2.pos.getX(),
       seg.pt2.pos.getY(),
       seg.pt2.layer->getRoutingLevel()});
}

}  // namespace ant
