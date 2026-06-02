// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "flow/combinational_mapper_npn.h"

#include <algorithm>
#include <bit>
#include <cassert>
#include <functional>
#include <iterator>

namespace syn {

// cofactor_masks[i] is a bitmask of all rows in a 64-bit truth table where
// variable x_i = 1.
const Truth6 cofactor_masks[6] = {
    0xaaaaaaaaaaaaaaaa,
    0xcccccccccccccccc,
    0xf0f0f0f0f0f0f0f0,
    0xff00ff00ff00ff00,
    0xffff0000ffff0000,
    0xffffffff00000000,
};

Truth6 mask6(const int size)
{
  if (size == 6) {
    return 0xffffffffffffffff;
  }
  return ((Truth6) 1 << (1 << size)) - 1;
}

// Returns a canonical representative of m's NPN-equivalence class — the
// set of functions reachable from m by input permutation, input inversion,
// and output inversion — and fills `npn` with the transform mapping the
// repr back to m (so npn(repr) == m, masked to ninputs).
//
// Canonicalization rule:
//   1. Complement the output if popcount(m) > nbits/2.
//   2. Complement each input whose negative cofactor has more 1s than its
//      positive cofactor.
//   3. Sort inputs by popcount of the (now smaller) negative cofactor.
//   4. Apply the resulting permutation+inversions to m to produce the repr.
//
// "Semi"class reflects that canonicalization is only canonical up to ties
// in the popcount comparisons. Three tie sources arise: a bipolar function
// (popcount == nbits/2) where both output polarities are equally canonical;
// an ambiguous variable whose cofactor popcounts are equal; and tied
// variables with equal negative-cofactor popcounts whose sort order is
// undefined. NPN-equivalent functions that resolve these ties differently
// yield different reprs. For library matching where any such repr could be
// the lookup key, see npn_semiclass_allrepr, which enumerates them all.
Truth6 npn_semiclass(Truth6 m, const int ninputs, NPN& npn)
{
  npn = NPN{};

  if (!ninputs) {
    return 0;
  }

  const int nbits = 1 << ninputs;
  const Truth6 mask = mask6(ninputs);
  m &= mask;
  // Canonicalization rule 1
  if (std::popcount(m) > nbits / 2) {
    npn.output_complement = true;
    m ^= mask;
  }

  bool compls[6] = {};
  int popcount[6] = {};
  int order[6] = {};

  for (int i = 0; i < ninputs; i++) {
    int nfactor = std::popcount(m & ~cofactor_masks[i]);
    int pfactor = std::popcount(m & cofactor_masks[i]);

    // Canonicalization rule 2
    if (nfactor > pfactor) {
      std::swap(pfactor, nfactor);
      compls[i] = true;
    }

    // Canonicalization rule 3
    int j;
    for (j = 0; j < i; j++) {
      if (nfactor < popcount[j]) {
        break;
      }
    }

    for (int k = i - 1; k >= j; k--) {
      popcount[k + 1] = popcount[k];
      order[k + 1] = order[k];
    }

    popcount[j] = nfactor;
    order[j] = i;
  }

  Truth6 sc = 0;

  for (int idx1 = 0; idx1 < nbits; idx1++) {
    if (!(m & (Truth6) 1 << idx1)) {
      continue;
    }

    int idx2 = 0;
    for (int j = 0; j < ninputs; j++) {
      const int pj = order[j];
      if ((!!(idx1 & 1 << pj)) ^ compls[pj]) {
        idx2 |= 1 << j;
      }
    }

    sc |= (Truth6) 1 << idx2;
  }

  for (int i = 0; i < ninputs; i++) {
    npn.input_complement[i] = compls[i];
    npn.permutation[order[i]] = i;
  }

  return sc;
}

// Like npn_semiclass, but invokes cb(repr, npn) once per repr that
// npn_semiclass could produce for any function NPN-equivalent to m. The
// three tie sources are enumerated explicitly:
//
//   * Bipolar output (popcount == nbits/2): both output_complement values.
//     Triggers a re-entry to `repolarize` after the main loop.
//   * Ambiguous variables (popcount(neg) == popcount(pos)): both input
//     polarities are tracked in `ambimask` and walked by the outer
//     `for (int ambi = 0; ...)` loop. The stride
//     `((ambi | ambimask) + 1) & ~ambimask` increments only the ambiguous
//     bits, leaving the unambiguous ones at zero.
//   * Tied variables (equal popcount(neg)): all permutations within each
//     tied run. Run lengths live in `tied[]`; std::next_permutation walks
//     them via the `goto next_round` loop.
//
// Used by buildIndex so a library cell is registered under every repr key
// a cut function might canonicalize to via npn_semiclass.
void npn_semiclass_allrepr(Truth6 m,
                           const int ninputs,
                           const std::function<void(Truth6, NPN&)>& cb)
{
  NPN npn = {};

  if (!ninputs) {
    cb(0, npn);
    return;
  }

  bool bipolar = false;
  const int nbits = 1 << ninputs;
  const Truth6 mask = mask6(ninputs);
  m &= mask;
  if (std::popcount(m) > nbits / 2) {
    npn.output_complement = true;
    m ^= mask;
  } else if (std::popcount(m) == nbits / 2) {
    bipolar = true;
  }

repolarize:
  bool compls[6] = {};
  int popcount[6] = {};
  int order[6] = {};
  int ambimask = nbits - 1;

  for (int i = 0; i < ninputs; i++) {
    int nfactor = std::popcount(m & ~cofactor_masks[i]);
    int pfactor = std::popcount(m & cofactor_masks[i]);

    if (nfactor > pfactor) {
      std::swap(pfactor, nfactor);
      compls[i] = true;
    } else if (nfactor == pfactor) {
      ambimask &= ~(1 << i);
    }

    int j;
    for (j = 0; j < i; j++) {
      if (nfactor < popcount[j]) {
        break;
      }
    }

    for (int k = i - 1; k >= j; k--) {
      popcount[k + 1] = popcount[k];
      order[k + 1] = order[k];
    }

    popcount[j] = nfactor;
    order[j] = i;
  }

  int tied[6] = {};

  for (int i = 0; i < ninputs - 1; i++) {
    const int mark = i;
    for (; popcount[i] == popcount[i + 1] && i < ninputs - 1;) {
      tied[mark]++;
      i++;
    }
  }

  for (int ambi = 0; ambi < nbits; ambi = ((ambi | ambimask) + 1) & ~ambimask) {
  next_round: {
    Truth6 sc = 0;
    for (int idx1 = 0; idx1 < nbits; idx1++) {
      if (!(m & (Truth6) 1 << idx1)) {
        continue;
      }

      int idx2 = 0;
      for (int j = 0; j < ninputs; j++) {
        int pj = order[j];
        if ((!!(idx1 & 1 << pj)) ^ compls[pj] ^ !!(ambi & 1 << pj)) {
          idx2 |= 1 << j;
        }
      }

      sc |= (Truth6) 1 << idx2;
    }

    for (int i = 0; i < ninputs; i++) {
      npn.input_complement[i] = compls[i] ^ !!(ambi & 1 << i);
      npn.permutation[order[i]] = i;
    }

    cb(sc, npn);
  }

    for (int j = ninputs - 2; j >= 0; j--) {
      if (tied[j]) {
        if (std::next_permutation(std::begin(order) + j,
                                  std::begin(order) + j + tied[j] + 1)) {
          goto next_round;
        }
      }
    }
  }
  if (bipolar) {
    bipolar = false;
    m ^= mask;
    npn.output_complement ^= true;
    goto repolarize;
  }
}

}  // namespace syn
