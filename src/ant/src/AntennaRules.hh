// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <vector>

#include "odb/db.h"

namespace utl {
class Logger;
}

namespace ant {

// A layer's antenna rule factors, with LEF defaults filled in for whatever
// the rule leaves unset.
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
  std::vector<double> gate_plus_diff_idx;
  std::vector<double> gate_plus_diff_ratios;
  double diff_metal_reduce_factor;
};

// Builds the model for tech_layer from its default antenna rule.  A routing
// layer with a side area rule but no thickness is an error (ANT-13): its
// side area would be zero, so the side area rules could never fail.
AntennaModel makeAntennaModel(odb::dbTechLayer* tech_layer,
                              utl::Logger* logger);

// Evaluates a piecewise-linear table at ref_value, or returns default_value
// when the table is empty.  A single-entry table is a constant.
double getPwlFactor(const odb::dbTechLayerAntennaRule::pwl_pair& pwl_info,
                    double ref_value,
                    double default_value);

}  // namespace ant
