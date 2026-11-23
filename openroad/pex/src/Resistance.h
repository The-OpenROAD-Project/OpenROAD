// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Resistance of a block's wires and vias from the process stack's ITF
// data, at the stack's global temperature.

#pragma once

#include <string>
#include <unordered_map>

#include "fsv/Stack.h"
#include "odb/geom.h"

namespace odb {
class dbShape;
class dbTech;
class dbTechLayer;
}  // namespace odb

namespace pex {

class Resistance
{
 public:
  // Maps each routing layer of `tech` to the stack's conductor of the same
  // name, and each cut layer to the stack's via between the conductors of
  // the routing layers below and above it.
  Resistance(const fsv::Stack& stack, odb::dbTech* tech);

  // A wire of a drawn width and centerline length, in database units, on a
  // routing layer; ohms. Throws std::invalid_argument for a layer the
  // stack has no conductor for, or whose resistance the stack gives in a
  // form not supported yet: a sheet resistance or resistivity against
  // width and spacing, or an etch table that applies to resistance.
  double wire(odb::dbTechLayer* layer, int width, int length) const;
  // A via shape: its cuts in parallel; ohms. Throws std::invalid_argument
  // for a via with no cut or a cut layer the stack has no via for.
  double via(const odb::dbShape& via) const;
  // One cut; ohms.
  double cut(odb::dbTechLayer* layer, const odb::Rect& cut) const;

 private:
  const fsv::Stack::Conductor& conductor(odb::dbTechLayer* layer) const;

  const fsv::Stack& stack_;
  double dbu_;    // per µm
  double scale_;  // the stack's half-node scale factor
  std::unordered_map<odb::dbTechLayer*, fsv::Stack::Layer> conductors_;
  std::unordered_map<odb::dbTechLayer*, fsv::Stack::Layer>
      vias_;  // by cut layer
  // Why a routing layer has no conductor here.
  std::unordered_map<odb::dbTechLayer*, std::string> unusable_;
};

}  // namespace pex
