// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "AntennaRules.hh"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace ant {

AntennaModel makeAntennaModel(odb::dbTechLayer* tech_layer, utl::Logger* logger)
{
  double metal_factor = 1.0;
  double diff_metal_factor = 1.0;

  double cut_factor = 1.0;
  double diff_cut_factor = 1.0;

  double side_metal_factor = 1.0;
  double diff_side_metal_factor = 1.0;

  double minus_diff_factor = 0.0;
  double plus_diff_factor = 0.0;
  std::vector<double> gate_plus_diff_idx;
  std::vector<double> gate_plus_diff_ratios;
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
    auto pwl = antenna_rule->getGatePlusDiffPWL();
    gate_plus_diff_idx = pwl.indices;
    gate_plus_diff_ratios = pwl.ratios;

    const double psr_ratio = antenna_rule->getPSR();
    const odb::dbTechLayerAntennaRule::pwl_pair diff_psr
        = antenna_rule->getDiffPSR();

    uint32_t wire_thickness_dbu = 0;
    tech_layer->getThickness(wire_thickness_dbu);

    const odb::dbTechLayerType layer_type = tech_layer->getType();

    // If there is a SIDE area antenna rule, then make sure thickness exists.
    if ((psr_ratio != 0 || !diff_psr.indices.empty())
        && layer_type == odb::dbTechLayerType::ROUTING
        && wire_thickness_dbu == 0) {
      logger->error(utl::ANT,
                    13,
                    "Layer {} has a side area antenna rule but no THICKNESS; "
                    "add THICKNESS to the LEF.",
                    tech_layer->getConstName());
    }
  }

  return {tech_layer,
          metal_factor,
          diff_metal_factor,
          cut_factor,
          diff_cut_factor,
          side_metal_factor,
          diff_side_metal_factor,
          minus_diff_factor,
          plus_diff_factor,
          std::move(gate_plus_diff_idx),
          std::move(gate_plus_diff_ratios),
          diff_metal_reduce_factor};
}

double getPwlFactor(const odb::dbTechLayerAntennaRule::pwl_pair& pwl_info,
                    const double ref_value,
                    const double default_value)
{
  if (!pwl_info.indices.empty()) {
    if (pwl_info.indices.size() == 1) {
      return pwl_info.ratios[0];
    }
    double pwl_info_index1 = pwl_info.indices[0];
    double pwl_info_ratio1 = pwl_info.ratios[0];
    double slope = 1.0;
    for (size_t i = 1; i < pwl_info.indices.size(); i++) {
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

namespace {

int layerNumber(const odb::dbTechLayer* layer)
{
  // An entry without a layer sorts below every layer.
  return layer ? layer->getNumber() : -1;
}

}  // namespace

double valueAtOrBelow(const LayerValues& entries, const odb::dbTechLayer* layer)
{
  const std::pair<double, odb::dbTechLayer*>* at_or_below = nullptr;
  // Later entries win ties, as LEF uses the last of repeated statements.
  for (const auto& entry : entries) {
    const int number = layerNumber(entry.second);
    if (number <= layer->getNumber()
        && (at_or_below == nullptr
            || number >= layerNumber(at_or_below->second))) {
      at_or_below = &entry;
    }
  }
  return at_or_below != nullptr ? at_or_below->first : 0.0;
}

double valueOnLayer(const LayerValues& entries, const odb::dbTechLayer* layer)
{
  std::optional<double> on_layer;
  std::optional<double> any_layer;
  for (const auto& [value, entry_layer] : entries) {
    if (entry_layer == layer) {
      on_layer = value;
    } else if (entry_layer == nullptr) {
      any_layer = value;
    }
  }
  return on_layer.value_or(any_layer.value_or(0.0));
}

PinAntenna readPinAntenna(odb::dbMTerm* mterm)
{
  PinAntenna pin;
  mterm->getDiffArea(pin.diff_area);
  mterm->getPartialMetalArea(pin.partial_metal_area);
  mterm->getPartialMetalSideArea(pin.partial_metal_side_area);
  mterm->getPartialCutArea(pin.partial_cut_area);
  if (mterm->hasDefaultAntennaModel()) {
    odb::dbTechAntennaPinModel* model = mterm->getDefaultAntennaModel();
    model->getGateArea(pin.gate_area);
    model->getMaxAreaCAR(pin.max_area_car);
    model->getMaxSideAreaCAR(pin.max_side_area_car);
    model->getMaxCutCAR(pin.max_cut_car);
  }
  pin.is_input = mterm->getIoType() == odb::dbIoType::INPUT;
  return pin;
}

bool isGateOn(const PinAntenna& pin, const odb::dbTechLayer* layer)
{
  return pin.is_input && valueAtOrBelow(pin.gate_area, layer) > 0.0;
}

}  // namespace ant
