// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// One definition of what the routing stage measures.
//
// Selection, reporting and verification have to agree on two things: which
// nets are eligible, and what a net's wrong-way fraction is.  They each used
// to carry their own copy of both, and the copies drifted: one walked vias
// correctly and the other did not, so the fraction the report printed and the
// fraction the verifier tested were different numbers.  All three call these.

#pragma once

#include <cstdint>

#include "odb/db.h"

namespace wmk {

// A signal net the router is free to route, and so a net the watermark may
// select.  Special, supply and clock nets are not.
bool isRoutableSignalNet(odb::dbNet* net);

// Canonical wrong-way and total planar wirelength for one net, in DBU.
//
// Canonical matters because a router may split one straight run across several
// records, or overlap them, and a naive sum would then depend on how the route
// happened to be written rather than on where the metal is.  Segments are
// bucketed by layer, orientation and the line they sit on, and each bucket's
// union is measured once.
//
// A via contributes no length -- it has no direction and occupies one point --
// but it does change the layer the wire continues on, and that layer is what
// decides whether the following run is wrong-way.
//
// diagonal_segments is incremented rather than reset, so a caller can total it
// over a whole block.
void canonicalWirelength(odb::dbNet* net,
                         std::int64_t& l_ww,
                         std::int64_t& l_tot,
                         int& diagonal_segments);

}  // namespace wmk
