// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <vector>

#include "odb/geom.h"

namespace odb {
class dbBTerm;
class dbITerm;
}  // namespace odb

namespace grt {

// For iterm pins this is the edge on a macro/pad cell that the pin lies on.
// For bterm pins this is the edge on block that the pin lies on.
// It is none for standard cells.
enum class PinEdge
{
  north,
  south,
  east,
  west,
  none
};

// Returns the edge of bounds closest to box.
PinEdge findClosestEdge(const odb::Rect& bounds, const odb::Rect& box);

// Returns the edge of bounds that most of the boxes are closest to.
PinEdge findPinEdge(const odb::Rect& bounds,
                    const std::vector<odb::Rect>& boxes);

// Returns the edge of iterm's instance that iterm lies on.
PinEdge findITermEdge(odb::dbITerm* iterm);

// Returns a coordinate of point that increases toward edge.
int coordinateTowardEdge(PinEdge edge, const odb::Point& point);

// Returns a same-net iterm whose metal covers bterm's shapes, or nullptr.
odb::dbITerm* findCoveringITerm(odb::dbBTerm* bterm);

}  // namespace grt
