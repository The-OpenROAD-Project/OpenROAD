// BSD 3-Clause License
//
// Copyright (c) 2020, MICL, DD-Lab, University of Michigan
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
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "ant/AntennaChecker.hh"

#include <tcl.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>

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

extern "C" {
extern int Ant_Init(Tcl_Interp* interp);
}

AntennaChecker::AntennaChecker() = default;
AntennaChecker::~AntennaChecker() = default;

void AntennaChecker::init(odb::dbDatabase* db,
                          GlobalRouteSource* global_route_source,
                          utl::Logger* logger)
{
  db_ = db;
  global_route_source_ = global_route_source;
  logger_ = logger;
}

void AntennaChecker::initAntennaRules()
{
  block_ = db_->getChip()->getBlock();
  odb::dbTech* tech = db_->getTech();
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

// DSU functions
void AntennaChecker::initDsu()
{
  dsu_parent_.resize(node_count_);
  dsu_size_.resize(node_count_);
  for (int i = 0; i < node_count_; i++) {
    dsu_size_[i] = 1;
    dsu_parent_[i] = i;
  }
}

int AntennaChecker::findSet(int u)
{
  if (u == dsu_parent_[u]) {
    return u;
  }
  return dsu_parent_[u] = findSet(dsu_parent_[u]);
}

bool AntennaChecker::dsuSame(int u, int v)
{
  return findSet(u) == findSet(v);
}

void AntennaChecker::unionSet(int u, int v)
{
  u = findSet(u);
  v = findSet(v);
  // union the smaller set to bigger set
  if (dsu_size_[u] < dsu_size_[v]) {
    std::swap(u, v);
  }
  dsu_parent_[v] = u;
  dsu_size_[u] += dsu_size_[v];
}

void AntennaChecker::saveGates(odb::dbNet* db_net)
{
  std::unordered_map<PinType, std::vector<int>, PinTypeHash> pin_nbrs;
  std::vector<int> ids;
  // iterate all instance pins
  for (odb::dbITerm* iterm : db_net->getITerms()) {
    odb::dbMTerm* mterm = iterm->getMTerm();
    std::string pin_name = fmt::format("  {}/{} ({})",
                                       iterm->getInst()->getConstName(),
                                       mterm->getConstName(),
                                       mterm->getMaster()->getConstName());
    PinType pin = PinType(pin_name, iterm);
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
        ids = findNodesWithIntersection(node_by_layer_map_[tech_layer],
                                        pin_pol);
        for (const int& index : ids) {
          pin_nbrs[pin].push_back(node_by_layer_map_[tech_layer][index]->id);
        }
        // if has via on upper layer connected to pin
        if (upper_layer) {
          ids = findNodesWithIntersection(node_by_layer_map_[upper_layer],
                                          pin_pol);
          for (const int& index : ids) {
            pin_nbrs[pin].push_back(node_by_layer_map_[upper_layer][index]->id);
          }
        }
        // if has via on lower layer connected to pin
        if (lower_layer) {
          ids = findNodesWithIntersection(node_by_layer_map_[lower_layer],
                                          pin_pol);
          for (const int& index : ids) {
            pin_nbrs[pin].push_back(node_by_layer_map_[lower_layer][index]->id);
          }
        }
      }
    }
  }
  // run DSU from min_layer to max_layer
  initDsu();
  odb::dbTechLayer* iter = min_layer_;
  odb::dbTechLayer* lower_layer;
  while (iter) {
    // iterate each node of this layer to union set
    for (auto& node_it : node_by_layer_map_[iter]) {
      int id_u = node_it->id;
      // if has lower layer
      lower_layer = iter->getLowerLayer();
      if (lower_layer) {
        // get lower neighbors and union
        for (const int& lower_it : node_it->low_adj) {
          int id_v = node_by_layer_map_[lower_layer][lower_it]->id;
          // if they are on different sets then union
          if (!dsuSame(id_u, id_v)) {
            unionSet(id_u, id_v);
          }
        }
      }
    }
    for (auto& node_it : node_by_layer_map_[iter]) {
      int id_u = node_it->id;
      // check gates in same set (first Nodes x gates)
      for (const auto& gate_it : pin_nbrs) {
        for (const int& nbr_id : gate_it.second) {
          if (dsuSame(id_u, nbr_id)) {
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

void AntennaChecker::calculateAreas()
{
  for (const auto& it : node_by_layer_map_) {
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
        info.iterms.push_back(gate.iterm);
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
        if (gate_info_[gate.name].find(it.first)
            != gate_info_[gate.name].end()) {
          gate_info_[gate.name][it.first] += info;
        } else {
          gate_info_[gate.name][it.first] = info;
        }
      }
    }
  }
}

// calculate PAR and PSR of wires and vias
void AntennaChecker::calculatePAR()
{
  for (auto& gate_it : gate_info_) {
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
void AntennaChecker::calculateCAR()
{
  for (auto& gate_it : gate_info_) {
    NodeInfo sumWire, sumVia;
    // iterate from first_layer -> last layer, cumulate sum for wires and vias
    odb::dbTechLayer* iter_layer = min_layer_;
    while (iter_layer) {
      if (gate_it.second.find(iter_layer) != gate_it.second.end()) {
        if (iter_layer->getRoutingLevel() == 0) {
          sumVia += gate_it.second[iter_layer];
          gate_it.second[iter_layer].CAR += sumVia.PAR;
          gate_it.second[iter_layer].CSR += sumVia.PSR;
          gate_it.second[iter_layer].diff_CAR += sumVia.diff_PAR;
          gate_it.second[iter_layer].diff_CSR += sumVia.diff_PSR;
        } else {
          sumWire += gate_it.second[iter_layer];
          gate_it.second[iter_layer].CAR += sumWire.PAR;
          gate_it.second[iter_layer].CSR += sumWire.PSR;
          gate_it.second[iter_layer].diff_CAR += sumWire.diff_PAR;
          gate_it.second[iter_layer].diff_CSR += sumWire.diff_PSR;
        }
      }
      iter_layer = iter_layer->getUpperLayer();
    }
  }
}

std::pair<bool, bool> AntennaChecker::checkPAR(odb::dbTechLayer* tech_layer,
                                               const NodeInfo& info,
                                               bool verbose,
                                               bool report,
                                               std::ofstream& report_file)
{
  // get rules
  const odb::dbTechLayerAntennaRule* antenna_rule
      = tech_layer->getDefaultAntennaRule();
  double PAR_ratio = antenna_rule->getPAR();
  odb::dbTechLayerAntennaRule::pwl_pair diffPAR = antenna_rule->getDiffPAR();
  double diff_PAR_PWL_ratio = getPwlFactor(diffPAR, info.iterm_diff_area, 0.0);
  bool checked = false;
  bool violation = false;

  // apply ratio_margin
  PAR_ratio *= (1.0 - ratio_margin_ / 100.0);
  diff_PAR_PWL_ratio *= (1.0 - ratio_margin_ / 100.0);

  // check PAR or diff_PAR
  if (PAR_ratio != 0) {
    violation = info.PAR > PAR_ratio;
    if (report) {
      std::string par_report = fmt::format(
          "      Partial area ratio: {:7.2f}\n      Required ratio: "
          "{:7.2f} "
          "(Gate area) {}",
          info.PAR,
          PAR_ratio,
          violation ? "(VIOLATED)" : "");
      if (verbose) {
        logger_->report("{}", par_report);
      }
      if (report_file.is_open() && (violation || verbose)) {
        report_file << par_report << "\n";
      }
    }
  } else {
    if (diff_PAR_PWL_ratio != 0) {
      checked = true;
      violation = info.diff_PAR > diff_PAR_PWL_ratio;
    }
    if (report) {
      std::string diff_par_report = fmt::format(
          "      Partial area ratio: {:7.2f}\n      Required ratio: "
          "{:7.2f} "
          "(Gate area) {}",
          info.diff_PAR,
          diff_PAR_PWL_ratio,
          violation ? "(VIOLATED)" : "");
      if (verbose) {
        logger_->report("{}", diff_par_report);
      }
      if (report_file.is_open() && (violation || verbose)) {
        report_file << diff_par_report << "\n";
      }
    }
  }
  return {violation, checked};
}

std::pair<bool, bool> AntennaChecker::checkPSR(odb::dbTechLayer* tech_layer,
                                               const NodeInfo& info,
                                               bool verbose,
                                               bool report,
                                               std::ofstream& report_file)
{
  // get rules
  const odb::dbTechLayerAntennaRule* antenna_rule
      = tech_layer->getDefaultAntennaRule();
  double PSR_ratio = antenna_rule->getPSR();
  const odb::dbTechLayerAntennaRule::pwl_pair diffPSR
      = antenna_rule->getDiffPSR();
  double diff_PSR_PWL_ratio = getPwlFactor(diffPSR, info.iterm_diff_area, 0.0);
  bool checked = false;
  bool violation = false;

  // apply ratio_margin
  PSR_ratio *= (1.0 - ratio_margin_ / 100.0);
  diff_PSR_PWL_ratio *= (1.0 - ratio_margin_ / 100.0);

  // check PSR or diff_PSR
  if (PSR_ratio != 0) {
    violation = info.PSR > PSR_ratio;
    if (report) {
      std::string psr_report = fmt::format(
          "      Partial area ratio: {:7.2f}\n      Required ratio: "
          "{:7.2f} "
          "(Side area) {}",
          info.PSR,
          PSR_ratio,
          violation ? "(VIOLATED)" : "");
      if (verbose) {
        logger_->report("{}", psr_report);
      }
      if (report_file.is_open() && (violation || verbose)) {
        report_file << psr_report << "\n";
      }
    }
  } else {
    if (diff_PSR_PWL_ratio != 0) {
      checked = true;
      violation = info.diff_PSR > diff_PSR_PWL_ratio;
    }
    if (report) {
      std::string diff_psr_report = fmt::format(
          "      Partial area ratio: {:7.2f}\n      Required ratio: "
          "{:7.2f} "
          "(Side area) {}",
          info.diff_PSR,
          diff_PSR_PWL_ratio,
          violation ? "(VIOLATED)" : "");
      if (verbose) {
        logger_->report("{}", diff_psr_report);
      }
      if (report_file.is_open() && (violation || verbose)) {
        report_file << diff_psr_report << "\n";
      }
    }
  }
  return {violation, checked};
}

bool AntennaChecker::checkCAR(odb::dbTechLayer* tech_layer,
                              const NodeInfo& info,
                              bool verbose,
                              bool report,
                              std::ofstream& report_file)
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

  // check CAR or diff_CAR
  if (CAR_ratio != 0) {
    violation = info.CAR > CAR_ratio;
    if (report) {
      std::string car_report = fmt::format(
          "      Cumulative area ratio: {:7.2f}\n      Required ratio: "
          "{:7.2f} "
          "(Cumulative area) {}",
          info.CAR,
          CAR_ratio,
          violation ? "(VIOLATED)" : "");
      if (verbose) {
        logger_->report("{}", car_report);
      }
      if (report_file.is_open() && (violation || verbose)) {
        report_file << car_report << "\n";
      }
    }
  } else {
    if (diff_CAR_PWL_ratio != 0) {
      violation = info.diff_CAR > diff_CAR_PWL_ratio;
    }
    if (report) {
      std::string diff_car_report = fmt::format(
          "      Cumulative area ratio: {:7.2f}\n      Required ratio: "
          "{:7.2f} "
          "(Cumulative area) {}",
          info.diff_CAR,
          diff_CAR_PWL_ratio,
          violation ? "(VIOLATED)" : "");
      if (verbose) {
        logger_->report("{}", diff_car_report);
      }
      if (report_file.is_open() && (violation || verbose)) {
        report_file << diff_car_report << "\n";
      }
    }
  }
  return violation;
}

bool AntennaChecker::checkCSR(odb::dbTechLayer* tech_layer,
                              const NodeInfo& info,
                              bool verbose,
                              bool report,
                              std::ofstream& report_file)
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

  // check CSR or diff_CSR
  if (CSR_ratio != 0) {
    violation = info.CSR > CSR_ratio;
    if (report) {
      std::string csr_report = fmt::format(
          "      Cumulative area ratio: {:7.2f}\n      Required ratio: "
          "{:7.2f} "
          "(Cumulative side area) {}",
          info.CSR,
          CSR_ratio,
          violation ? "(VIOLATED)" : "");
      if (verbose) {
        logger_->report("{}", csr_report);
      }
      if (report_file.is_open() && (violation || verbose)) {
        report_file << csr_report << "\n";
      }
    }
  } else {
    if (diff_CSR_PWL_ratio != 0) {
      violation = info.diff_CSR > diff_CSR_PWL_ratio;
    }
    if (report) {
      std::string diff_csr_report = fmt::format(
          "      Cumulative area ratio: {:7.2f}\n      Required ratio: "
          "{:7.2f} "
          "(Cumulative side area) {}",
          info.diff_CSR,
          diff_CSR_PWL_ratio,
          violation ? "(VIOLATED)" : "");
      if (verbose) {
        logger_->report("{}", diff_csr_report);
      }
      if (report_file.is_open() && (violation || verbose)) {
        report_file << diff_csr_report << "\n";
      }
    }
  }
  return violation;
}

int AntennaChecker::checkGates(odb::dbNet* db_net,
                               bool verbose,
                               bool report_if_no_violation,
                               std::ofstream& report_file,
                               odb::dbMTerm* diode_mterm,
                               float ratio_margin)
{
  ratio_margin_ = ratio_margin;
  int pin_violation_count = 0;

  std::unordered_map<std::string, std::unordered_set<odb::dbTechLayer*>>
      gates_with_violations;

  for (const auto& gate_info : gate_info_) {
    bool pin_has_violation = false;

    for (const auto& layer_info : gate_info.second) {
      bool node_has_violation = false;
      if (layer_info.first->hasDefaultAntennaRule()) {
        // check if node has violation
        if (layer_info.first->getRoutingLevel() != 0) {
          auto par_violation = checkPAR(
              layer_info.first, layer_info.second, false, false, report_file);
          auto psr_violation = checkPSR(
              layer_info.first, layer_info.second, false, false, report_file);
          bool car_violation = checkCAR(
              layer_info.first, layer_info.second, false, false, report_file);
          bool csr_violation = checkCSR(
              layer_info.first, layer_info.second, false, false, report_file);

          if (par_violation.first || psr_violation.first || car_violation
              || csr_violation) {
            node_has_violation = true;
          }
        } else {
          auto par_violation = checkPAR(
              layer_info.first, layer_info.second, false, false, report_file);
          bool car_violation = checkCAR(
              layer_info.first, layer_info.second, false, false, report_file);
          if (par_violation.first || car_violation) {
            node_has_violation = true;
          }
        }
        if (node_has_violation) {
          pin_has_violation = true;
          gates_with_violations[gate_info.first].insert(layer_info.first);
        }
      }
    }
    if (pin_has_violation) {
      pin_violation_count++;
    }
  }

  // iterate the gates with violations to report
  if (((pin_violation_count > 0) || report_if_no_violation)
      && diode_mterm == nullptr) {
    std::string net_name = fmt::format("Net: {}", db_net->getConstName());
    if (verbose) {
      logger_->report("{}", net_name);
    }
    if (report_file.is_open()) {
      report_file << net_name << "\n";
    }
    for (const auto& gate_it : gate_info_) {
      if (!verbose
          && gates_with_violations.find(gate_it.first)
                 == gates_with_violations.end()) {
        continue;
      }
      std::string pin_name = fmt::format("  Pin: {}", gate_it.first);
      if (verbose) {
        logger_->report("{}", pin_name);
      }
      if (report_file.is_open()) {
        report_file << pin_name << "\n";
      }
      for (const auto& layer_info : gate_info_[gate_it.first]) {
        if (!verbose
            && gates_with_violations[gate_it.first].find(layer_info.first)
                   == gates_with_violations[gate_it.first].end()) {
          continue;
        }
        std::string layer_name
            = fmt::format("    Layer: {}", layer_info.first->getConstName());
        if (verbose) {
          logger_->report("{}", layer_name);
        }
        if (report_file.is_open()) {
          report_file << layer_name << "\n";
        }
        // re-check to report violations
        if (layer_info.first->getRoutingLevel() != 0) {
          checkPAR(
              layer_info.first, layer_info.second, verbose, true, report_file);
          checkPSR(
              layer_info.first, layer_info.second, verbose, true, report_file);
          checkCAR(
              layer_info.first, layer_info.second, verbose, true, report_file);
          checkCSR(
              layer_info.first, layer_info.second, verbose, true, report_file);
        } else {
          checkPAR(
              layer_info.first, layer_info.second, verbose, true, report_file);
          checkCAR(
              layer_info.first, layer_info.second, verbose, true, report_file);
        }
        if (verbose) {
          logger_->report("");
        }
        if (report_file.is_open()) {
          report_file << "\n";
        }
      }
      if (verbose) {
        logger_->report("");
      }
      if (report_file.is_open()) {
        report_file << "\n";
      }
    }
    if (verbose) {
      logger_->report("");
    }
    if (report_file.is_open()) {
      report_file << "\n";
    }
  }

  std::unordered_map<odb::dbTechLayer*, std::unordered_set<std::string>>
      pin_added;
  // if checkGates is used by repair antennas
  if (pin_violation_count > 0) {
    for (const auto& gate_iter : gates_with_violations) {
      for (const auto& layer_info : gate_iter.second) {
        // when repair antenna is running, calculate number of diodes
        if (layer_info->getRoutingLevel() != 0
            && pin_added[layer_info].find(gate_iter.first)
                   == pin_added[layer_info].end()) {
          double diode_diff_area = 0.0;
          if (diode_mterm) {
            diode_diff_area = diffArea(diode_mterm);
          }
          NodeInfo violation_info = gate_info_[gate_iter.first][layer_info];
          std::vector<odb::dbITerm*> gates = violation_info.iterms;
          odb::dbTechLayer* violation_layer = layer_info;
          int diode_count_per_gate = 0;
          // check violations only PAR & PSR
          auto par_violation = checkPAR(
              violation_layer, violation_info, false, false, report_file);
          auto psr_violation = checkPSR(
              violation_layer, violation_info, false, false, report_file);
          bool violated = par_violation.first || psr_violation.first;
          // while it has violation, increase iterm_diff_area
          if (diode_mterm) {
            while (par_violation.first || psr_violation.first) {
              // increasing iterm_diff_area and count
              violation_info.iterm_diff_area += diode_diff_area * gates.size();
              diode_count_per_gate++;
              // re-calculate info only PAR & PSR
              calculateWirePar(violation_layer, violation_info);
              // re-check violations only PAR & PSR
              par_violation = checkPAR(
                  violation_layer, violation_info, false, false, report_file);
              psr_violation = checkPSR(
                  violation_layer, violation_info, false, false, report_file);
              if (diode_count_per_gate > max_diode_count_per_gate) {
                logger_->warn(ANT,
                              15,
                              "Net {} requires more than {} diodes per gate to "
                              "repair violations.",
                              db_net->getConstName(),
                              max_diode_count_per_gate);
                break;
              }
            }
          }
          // save the iterms of repaired node
          for (const auto& iterm_iter : gates) {
            pin_added[violation_layer].insert(iterm_iter->getName());
          }
          // save antenna violation
          if (violated) {
            antenna_violations_.push_back(
                {layer_info->getRoutingLevel(), gates, diode_count_per_gate});
          }
        }
      }
    }
  }
  return pin_violation_count;
}

void AntennaChecker::buildLayerMaps(odb::dbNet* db_net)
{
  odb::dbWire* wires = db_net->getWire();

  std::unordered_map<odb::dbTechLayer*, PolygonSet> set_by_layer;

  wiresToPolygonSetMap(wires, set_by_layer);
  avoidPinIntersection(db_net, set_by_layer);

  // init struct (copy polygon set information on struct to save neighbors)
  node_by_layer_map_.clear();
  gate_info_.clear();
  node_count_ = 0;
  odb::dbTech* tech = db_->getTech();
  min_layer_ = tech->findRoutingLayer(1);

  for (const auto& layer_it : set_by_layer) {
    for (const auto& pol_it : layer_it.second) {
      bool isVia = layer_it.first->getRoutingLevel() == 0;
      node_by_layer_map_[layer_it.first].push_back(
          new GraphNode(node_count_, isVia, pol_it));
      node_count_++;
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
            node_by_layer_map_[layer_it.first->getLowerLayer()], via_it);
        upper_index = findNodesWithIntersection(
            node_by_layer_map_[layer_it.first->getUpperLayer()], via_it);

        if (upper_index.size() <= 2) {
          // connect upper -> via
          for (int& up_index : upper_index) {
            node_by_layer_map_[layer_it.first->getUpperLayer()][up_index]
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
            node_by_layer_map_[layer_it.first][via_index]->low_adj.push_back(
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
  saveGates(db_net);
}

void AntennaChecker::checkNet(odb::dbNet* db_net,
                              bool verbose,
                              bool report_if_no_violation,
                              std::ofstream& report_file,
                              odb::dbMTerm* diode_mterm,
                              float ratio_margin,
                              int& net_violation_count,
                              int& pin_violation_count)
{
  odb::dbWire* wire = db_net->getWire();
  if (wire) {
    buildLayerMaps(db_net);

    calculateAreas();

    calculatePAR();
    calculateCAR();

    int pin_violations = checkGates(db_net,
                                    verbose,
                                    report_if_no_violation,
                                    report_file,
                                    diode_mterm,
                                    ratio_margin);

    if (pin_violations > 0) {
      net_violation_count++;
      pin_violation_count += pin_violations;
    }
  }
}

std::vector<Violation> AntennaChecker::getAntennaViolations(
    odb::dbNet* net,
    odb::dbMTerm* diode_mterm,
    float ratio_margin)
{
  antenna_violations_.clear();
  if (net->isSpecial()) {
    return antenna_violations_;
  }

  int net_violation_count, pin_violation_count;
  net_violation_count = 0;
  pin_violation_count = 0;
  std::ofstream report_file;
  checkNet(net,
           false,
           false,
           report_file,
           diode_mterm,
           ratio_margin,
           net_violation_count,
           pin_violation_count);

  return antenna_violations_;
}

int AntennaChecker::checkAntennas(odb::dbNet* net, bool verbose)
{
  initAntennaRules();

  std::ofstream report_file;
  if (!report_file_name_.empty()) {
    report_file.open(report_file_name_, std::ofstream::out);
  }

  bool grt_routes = global_route_source_->haveRoutes();
  bool drt_routes = haveRoutedNets();
  bool use_grt_routes = (grt_routes && !drt_routes);
  if (!grt_routes && !drt_routes) {
    logger_->error(ANT,
                   8,
                   "No detailed or global routing found. Run global_route or "
                   "detailed_route first.");
  }

  if (use_grt_routes) {
    global_route_source_->makeNetWires();
  }

  int net_violation_count = 0;
  int pin_violation_count = 0;

  if (net) {
    if (!net->isSpecial()) {
      checkNet(net,
               verbose,
               true,
               report_file,
               nullptr,
               0,
               net_violation_count,
               pin_violation_count);
    } else {
      logger_->error(
          ANT, 14, "Skipped net {} because it is special.", net->getName());
    }
  } else {
    for (odb::dbNet* net : block_->getNets()) {
      if (!net->isSpecial()) {
        checkNet(net,
                 verbose,
                 false,
                 report_file,
                 nullptr,
                 0,
                 net_violation_count,
                 pin_violation_count);
      }
    }
  }

  logger_->info(ANT, 2, "Found {} net violations.", net_violation_count);
  logger_->metric("antenna__violating__nets", net_violation_count);
  logger_->info(ANT, 1, "Found {} pin violations.", pin_violation_count);
  logger_->metric("antenna__violating__pins", pin_violation_count);

  if (!report_file_name_.empty()) {
    report_file.close();
  }

  if (use_grt_routes) {
    global_route_source_->destroyNetWires();
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

}  // namespace ant
