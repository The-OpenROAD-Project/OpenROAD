// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <utility>
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

// A pin's antenna values by layer, as LEF gives them ("ANTENNAGATEAREA 0.5
// LAYER M1").  An entry without a layer applies to every layer.
using LayerValues = std::vector<std::pair<double, odb::dbTechLayer*>>;

// The value that holds when checking layer: the entry for the nearest layer
// at or below it, so a gate area given on M1 still applies on V1 and M2.
// Entries only above layer give zero: in a block, a device reached only
// through upper metal is not connected while the lower layers are built.
double valueAtOrBelow(const LayerValues& entries,
                      const odb::dbTechLayer* layer);

// The value of the entry for exactly layer, else of an entry without a
// layer, else zero.  For values that describe one layer's own geometry or
// ratio, such as ANTENNAPARTIALMETALAREA and ANTENNAMAXAREACAR.
double valueOnLayer(const LayerValues& entries, const odb::dbTechLayer* layer);

// The antenna values LEF gives on a macro pin.  The PARTIAL areas and the
// MAX*CAR ratios describe metal inside a hierarchical block that connects
// to the pin, which the checker cannot see.  The gate area and MAX*CAR
// values come from the default (OXIDE1) antenna model.
struct PinAntenna
{
  bool is_input{false};
  LayerValues gate_area;                // ANTENNAGATEAREA
  LayerValues diff_area;                // ANTENNADIFFAREA
  LayerValues partial_metal_area;       // ANTENNAPARTIALMETALAREA
  LayerValues partial_metal_side_area;  // ANTENNAPARTIALMETALSIDEAREA
  LayerValues partial_cut_area;         // ANTENNAPARTIALCUTAREA
  LayerValues max_area_car;             // ANTENNAMAXAREACAR
  LayerValues max_side_area_car;        // ANTENNAMAXSIDEAREACAR
  LayerValues max_cut_car;              // ANTENNAMAXCUTCAR
};

PinAntenna readPinAntenna(odb::dbMTerm* mterm);

// Whether the pin is a gate the checker protects on layer: an input pin
// with gate area there.
bool isGateOn(const PinAntenna& pin, const odb::dbTechLayer* layer);

}  // namespace ant
