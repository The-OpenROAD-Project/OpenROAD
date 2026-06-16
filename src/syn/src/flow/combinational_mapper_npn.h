// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cassert>
#include <cstdint>
#include <functional>

namespace syn {

using Truth6 = uint64_t;

// NPN-equivalence transformation between two Boolean functions.
//
// Two functions are NPN-equivalent if one can be obtained from the other by
// some combination of input negation (N), input permutation (P), and output
// negation (N). This struct records such a mapping: input i is optionally
// inverted (input_complement[i]) and routed to position permutation[i], and
// the result is optionally inverted (output_complement).
//
// Apply to a truth table via operator(), compose two transforms with
// operator*, and invert via inv(). The technology mapper uses these to
// describe how the variables of a cut wire up to a library cell's input
// ports, with the inversion bits tracking inverters that fold into the
// match.

struct NPN
{
  bool output_complement = false;
  bool input_complement[6] = {};
  int permutation[6] = {-1, -1, -1, -1, -1, -1};

  static NPN identity(const int ninputs)
  {
    NPN ret;
    for (int i = 0; i < ninputs; i++) {
      ret.permutation[i] = i;
    }
    return ret;
  }

  bool is_identity() const
  {
    if (output_complement) {
      return false;
    }
    for (int i = 0; i < 6 && permutation[i] != -1; i++) {
      if (permutation[i] != i || input_complement[i]) {
        return false;
      }
    }
    return true;
  }

  int ninputs() const
  {
    int n = 0;
    while (n < 6 && permutation[n] != -1) {
      ++n;
    }
    return n;
  }

  Truth6 operator()(const Truth6 m) const
  {
    Truth6 ret = 0;
    for (int idx1 = 0; idx1 < (1 << ninputs()); idx1++) {
      if (!(m & (Truth6) 1 << idx1)) {
        continue;
      }
      int idx2 = 0;
      for (int j = 0; j < ninputs(); j++) {
        assert(permutation[j] >= 0);  // sentinel -1 is excluded by ninputs()
        if ((!!(idx1 & 1 << j)) ^ input_complement[j]) {
          // NOLINTNEXTLINE(clang-analyzer-core.BitwiseShift) — guarded above.
          idx2 |= 1 << permutation[j];
        }
      }
      ret |= (Truth6) 1 << idx2;
    }
    return output_complement ? ~ret : ret;
  }

  NPN operator*(const NPN& other) const
  {
    NPN ret;
    ret.output_complement = output_complement ^ other.output_complement;
    for (int i = 0; i < 6 && permutation[i] != -1; i++) {
      ret.permutation[i] = permutation[other.permutation[i]];
      ret.input_complement[i]
          = input_complement[other.permutation[i]] ^ other.input_complement[i];
    }
    return ret;
  }

  NPN inv() const
  {
    NPN ret;
    ret.output_complement = output_complement;
    for (int i = 0; i < 6 && permutation[i] != -1; i++) {
      ret.permutation[permutation[i]] = i;
      ret.input_complement[permutation[i]] = input_complement[i];
    }
    return ret;
  }

  int c_fingerprint() const
  {
    return output_complement | input_complement[0] << 1
           | input_complement[1] << 2 | input_complement[2] << 3
           | input_complement[3] << 4 | input_complement[4] << 5
           | input_complement[5] << 6;
  }
};

extern const Truth6 cofactor_masks[6];

// Returns the all-ones truth table for a `size`-input function:
// 2^(2^size) - 1, packed into the low bits of a Truth6.
Truth6 mask6(int size);

Truth6 npn_semiclass(Truth6 m, int ninputs, NPN& npn);
void npn_semiclass_allrepr(Truth6 m,
                           int ninputs,
                           const std::function<void(Truth6, NPN&)>& cb);

}  // namespace syn
