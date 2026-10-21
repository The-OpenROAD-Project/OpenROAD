// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// What the key decides about a placement mark and a clock tree mark.
//
// Each stage keys three things: the order in which candidates are taken,
// which of a pair is the target, and the value the target is driven to.
// Embedding and verification must agree on all three, so they are defined
// once here, in terms of the object names alone.  A verifier holds the key,
// the names in the claim file and the suspect design, and nothing else from
// embed time, so nothing else may enter a derivation it has to repeat.  A
// target that depended on where a cell stood when it was marked could not be
// re-derived from a layout that has since been touched, and one that had to
// be read back out of the claim file could be chosen by whoever wrote the
// file.

#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace wmk {

using StageKey = std::array<std::uint8_t, 32>;

// Two names in the order the scheme sees them, which is lexicographic.  Every
// keyed value below is a function of the ordered pair, so a claim that names
// the pair the other way round still derives the same target.
struct OrderedPair
{
  std::string first;
  std::string second;
  bool operator==(const OrderedPair&) const = default;
};

OrderedPair orderPair(const std::string& a, const std::string& b);

// Placement: the bit a pair carries.  0 when ``first`` is to sit left of
// ``second``, 1 otherwise.
int placementTargetBit(const StageKey& key, const OrderedPair& pair);

// Placement: where a candidate falls in the keyed order; candidates are taken
// in ascending order of this.  The tile is part of the derivation so that
// each tile's order is drawn afresh.  Verification does not depend on it.
std::array<std::uint8_t, 32> placementSortKey(const StageKey& key,
                                              int tile_x,
                                              int tile_y,
                                              const OrderedPair& pair);

// Clock tree: which of the two buffers carries the mark, and the parity of
// sequential fanout it is driven to.
struct CtsTarget
{
  std::string target;
  std::string other;
  int bit = 0;
};

CtsTarget ctsTarget(const StageKey& key, const OrderedPair& pair);

// Clock tree: the pair's identifier as the claim file records it, and where
// the pair falls in the keyed order.
std::string ctsPairKey(const OrderedPair& pair);
std::array<std::uint8_t, 32> ctsSortKey(const StageKey& key,
                                        const OrderedPair& pair);

}  // namespace wmk
