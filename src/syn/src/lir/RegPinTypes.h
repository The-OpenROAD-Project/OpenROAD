// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// L-IR register pin types: the semantic inputs of a register (kRegIn).

#pragma once

#include <cstdint>

namespace syn::lir {

// Input roles of a generic kRegIn; its fanins are the present roles (the
// RegClass mask) in this order. A mapped kRegIn has its Liberty inputs in
// Liberty order. The initial state is in the RegClass (RegInit), not a pin.
// Controls are active-high (inverted means a kNot fanin) and absent ones are
// inactive; flip-flops sample on the rising edge, latches are transparent
// while kClock is 1.
//
// Reset normal form (checkWellFormed): Reset loads 0 (or its value pin), Set
// all ones. A value pin (kConst) is only for a word with mixed reset bits,
// with its Reset. When both are active, RegFlags::set_over_reset picks
// (default: reset), for the sync and the async pair.
//
// Next state, with s = SRST/SSET and rs = ARST/ASET applied as above:
//   g(x) = EN ? x : Q
//   next = SE ? SI : (RegFlags::sync_over_enable ? s(g(D)) : g(s(D)))
//   A(v) = rs(ALOAD ? AD : v)                (async, level-sensitive)
// With S_k the state stored at the start of cycle k:
//   flip-flop: Q_k = A(S_k);  S_k+1 = A(next), with Q = Q_k
//   latch:     Q_k = A(G_k ? D_k : S_k);  S_k+1 = Q_k
// So async controls and transparent latches act in the same cycle. Passes
// keep kScanEnable/kScanIn on the register.
enum class RegPinType : uint8_t
{
  kData,
  kClock,   // edge (flip-flop) or gate (latch)
  kEnable,  // internal feedback mux: next = EN ? DATA : Q
  kSyncReset,
  kSyncSet,
  kSyncResetValue,  // mixed-bit word value loaded by kSyncReset
  kAsyncReset,
  kAsyncSet,
  kAsyncResetValue,  // mixed-bit word value loaded by kAsyncReset
  kAsyncLoad,
  kAsyncData,
  kScanEnable,  // outermost: next = SE ? SI : ... (see above)
  kScanIn,      // the scan data loaded when kScanEnable is 1

  kNumTypes,  // must stay last
};

inline constexpr int kNumRegPinTypes = static_cast<int>(RegPinType::kNumTypes);

using RegPinMask = uint16_t;
static_assert(kNumRegPinTypes <= 16, "RegPinMask is too narrow");

constexpr RegPinMask regPinBit(RegPinType type)
{
  return static_cast<RegPinMask>(1u << static_cast<unsigned>(type));
}

// Returns the lower-case name used by the .lir format ("data", "arst", ...),
// or "?" for out-of-range values.
const char* regPinTypeName(RegPinType type);

// Outputs of a generic register. kQ is the state, carried by the kRegOut
// itself; the others are kFans of the kRegOut with Fan.data = the output.
// A mapped register numbers its outputs in Liberty order instead.
enum class RegOutput : uint8_t
{
  kQ,
  kQN,       // the negated state
  kScanOut,  // the state, on a dedicated scan-out pin

  kNumOutputs,  // must stay last
};

inline constexpr int kNumRegOutputs = static_cast<int>(RegOutput::kNumOutputs);

// Returns "q", "qn" or "so"; "?" for out-of-range values.
const char* regOutputName(RegOutput output);

}  // namespace syn::lir
