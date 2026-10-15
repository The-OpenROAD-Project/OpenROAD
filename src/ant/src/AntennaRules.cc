// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "AntennaRules.hh"

#include <cstddef>
#include <cstdint>
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

}  // namespace ant
