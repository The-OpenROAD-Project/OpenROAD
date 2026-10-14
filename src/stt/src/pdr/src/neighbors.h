// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <vector>

#include "odb/geom.h"

namespace pdr {

// This is the method in "Prim-Dijkstra Revisited" section 4.
// The key idea is: "We say that vi is a neighbor of vj if the smallest
// bounding box containing vi and vj contains no other nodes."
//
// The bounding box is closed so a node on its boundary also excludes
// the pair.  Routing through such a node w costs no more than the
// direct edge: d(u,w) + d(w,v) = d(u,v) and the PD cost changes by
// (alpha - 1) * d(u,w) <= 0.  This keeps the neighbor count small when
// many nodes share an x or y, as with pins on rows and sites.

using Neighbors = std::vector<int>;

// Returns the neighbors of each point, sorted by index.  Coincident
// points are only neighbors of the lowest indexed point at their
// location.
std::vector<Neighbors> getNearestNeighbors(const std::vector<odb::Point>& pts);

}  // namespace pdr
