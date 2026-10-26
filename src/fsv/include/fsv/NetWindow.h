// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "fsv/Geometry.h"

namespace odb {
class dbBlock;
class dbNet;
}  // namespace odb

namespace fsv {

// The z extent (bottom, top) in µm of a routing or cut layer, by its name in
// the design; throws for a layer without one.
using LayerExtent
    = std::function<std::pair<double, double>(std::string_view layer)>;

struct NetWindowOptions
{
  // Neighbours whose shapes reach within `halo` µm of the targets' bounding
  // box in x-y are included. Negative: three times the targets' height (the
  // top of their highest box), since the ground plane screens fields over a
  // few heights.
  double halo = -1;
  // Neighbour shapes are clipped `margin` µm beyond the halo, so that the
  // cut ends lie farther out than anything the halo selects. Negative: half
  // the targets' height.
  double margin = -1;
};

// Conductors around some nets of a block, the targets, in µm: every shape
// of the target nets, and the neighbours within the window. Neighbours are the
// other nets' shapes, clipped at the margin, and the cells' obstructions and
// unconnected pins, which have no net; those form one grounded net, trimmed
// clear of the nets' shapes by each layer's minimum spacing. The geometry's
// window is the halo box, inside which nothing is missing, open on every
// side that reaches past the die (and not set at all if every side does).
struct NetWindow
{
  Geometry geometry;
  std::vector<int> targets;  // net indices in `geometry`
  double height = 0;         // of the targets' highest box
  double halo = 0;
  double margin = 0;
  int neighbours = 0;  // nets other than the targets
  int clipped = 0;     // neighbour shapes cut at the margin
  int trimmed = 0;     // obstructions cut clear of the nets' shapes
};

// The name of the grounded net of obstructions and unconnected pins.
inline constexpr std::string_view kObstructionNet = "<obstructions>";

// Reads the conductors of `targets` (a net named twice counts once) and
// their neighbours. Only routing and cut layers are conductors. Throws
// std::invalid_argument for a target without shapes, for shapes of
// different nets that overlap, for non-Manhattan special wires, and for an
// instance or block pin without a placement, which would sit at the
// origin. Every
// net and cell of the block is visited, so a call takes time in proportion
// to the design.
NetWindow readNetWindow(odb::dbBlock* block,
                        const std::vector<odb::dbNet*>& targets,
                        const LayerExtent& layer_extent,
                        const NetWindowOptions& options = {});

}  // namespace fsv
