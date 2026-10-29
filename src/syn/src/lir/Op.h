// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// L-IR object operations, stored by name in .lir (the enum order is free;
// kNumOps stays last). The set is canonical: importers lower sugar (Xnor,
// Nand/Nor/Xnor reductions, Ne, Gt, Ge, Le, logical ops, unary plus, Bwmux).
//  - Control fanins come first: Mux = [sel, when_one, when_zero]. Concat,
//    packed alternatives and truth tables are LSB-first.
//  - The signed flag is the operation's signedness (isSignSensitive: Cast,
//    Lt, Div/Mod, Mul, Pow, Shr), or the declared signedness on Pi and the
//    state halves. No other op has it (hasSignedFlag).
//  - Operands of bitwise, Add/Sub/Neg, Div/Mod and the muxes have the result
//    width (extension is an explicit Cast); Mul and Pow operands keep their
//    widths; Eq/Lt operands have equal widths and a 1-bit result. Shift
//    amounts are unsigned.
//  - x is unknown, not a don't-care: no pass may rely on its value. The gate
//    generators give the x results below a fixed value.
//  - kInst has width 0 and every output is a kFan; kCell and kRegOut are
//    output 0 (Q) themselves, other outputs kFans. A kFan's fanin 0 is its
//    owner (fanOwner()); its data is the child port, Liberty pin or
//    RegOutput it carries (fanOutput()).
//  - State pairs (kRegOut/kRegIn, kLoopOut/kLoopIn) link through stateIn()
//    and stateOut(), not fanins; the input halves are never read.

#pragma once

#include <cstdint>

namespace syn::lir {

enum class Op : uint8_t
{
  // Structure.
  kPi,         // module input; data = port index
  kPo,         // module output; fanin[0] = driver, data = port index
  kConst,      // data = ConstId (3-state: 0, 1, x; hash-consed)
  kUndriven,   // undriven value; opaque, never optimized through
  kBuf,        // identity; fanin[0]
  kSlice,      // fanin[0][data +: width]
  kConcat,     // LSB-first: fanin[0] supplies the low bits
  kReplicate,  // fanin[0] repeated data times
  kCast,       // resize fanin[0] to width; signed flag selects sign extension
  kInst,       // instance of another module; width 0, data = ModId
  kFan,        // extra output of a multi-output owner; fanin[0] = owner,
               // data = output index / register output

  // State. Each state element is an (Out, In) pair; Out has no fanins,
  // which breaks every cycle.
  kRegOut,  // register output (Q); data = RegClassId
  kRegIn,   // register input; fanins in RegPinType order per its RegClass,
            // data = the kRegOut
  // A loop pair is a stateless wire cut in two to break a combinational
  // cycle. ModuleBuilder cuts every cycle with one.
  kLoopOut,   // data = 0
  kLoopIn,    // fanin[0] = driver (same width), data = the kLoopOut
  kMemOut,    // reserved, not implemented yet
  kMemIn,     // reserved, not implemented yet
  kMemRead,   // reserved, not implemented yet
  kMemWrite,  // reserved, not implemented yet

  // Bitwise and selection.
  kNot,
  kAnd,
  kOr,
  kXor,
  kMux,  // [sel(1), when_one, when_zero]
  // [one_hot_sel(n), packed alternatives(n*W), default(W)]: the selected
  // alternative; an all-zero select gives the default; two or more select
  // bits set give x (gate generators: the OR of the selected alternatives).
  kPmux,
  kBmux,  // [binary_sel(s), packed alternatives((1<<s)*W)]
  // [binary_sel(s), a(W)] -> (W << s) bits, slot i = (sel == i) ? a : 0
  kDemux,

  // Arithmetic.
  kNeg,
  kAdd,
  kSub,
  kMul,
  // Truncating; divide by zero gives x (gate generators: all ones).
  kDiv,
  // Sign of the dividend; modulo by zero gives x (gate generators: the
  // dividend).
  kMod,
  // [base(Wb), exp(We)] -> the low W bits of base ** exp (two's complement
  // when signed). exp == 0 gives 1; a negative exp gives 0 for |base| > 1,
  // 1 for base 1, +-1 by the parity of exp for base -1, and x for base 0
  // (gate generators: 0).
  kPow,

  // Comparison.
  kEq,
  kLt,

  // Shifts: [a, amount]; the amount is unsigned.
  kShl,
  kShr,  // logical, or arithmetic when signed
  // Right shift padding with x (variable part-select): bit i is a[i + s] if
  // i + s < W, else x. Any x bit in the amount gives all x. The gate
  // generators give 0 for the x bits.
  kShiftx,

  // Reductions (1-bit result).
  kReduceAnd,
  kReduceOr,
  kReduceXor,

  // Mapped.
  kLut,   // 1-bit; fanin[0] = I0; data = ConstId of the truth table
  kCell,  // Liberty cell; data = LibCellId, output 0 on the owner

  // Tri-state.
  kTribuf,   // [en(1), a(W)] -> a when en = 1, z when en = 0, x when en = x
  kTriJoin,  // resolved bus [d0(W), d1(W), ...], n >= 2; per bit: z drivers
             // are ignored, agreeing drivers give their value, 0 vs 1 or
             // any x gives x, all z gives z

  kNumOps,  // must stay last
};

inline constexpr int kNumOps = static_cast<int>(Op::kNumOps);

// Returns the lower-case name used by the .lir format ("and", "reg_out",
// ...), or "?" for out-of-range values.
const char* opName(Op op);

// State halves: the output starts a value and the input ends it, which cuts
// combinational paths. A loop pair is a state pair but holds no value: use
// isStorageOut() for flopped values and loopPairDriver() (Traversal.h) to read
// through a loop pair.
constexpr bool isStateOut(Op op)
{
  return op == Op::kRegOut || op == Op::kLoopOut || op == Op::kMemOut;
}

constexpr bool isStateIn(Op op)
{
  return op == Op::kRegIn || op == Op::kLoopIn || op == Op::kMemIn;
}

// State outputs whose value is stored: registers and memories.
constexpr bool isStorageOut(Op op)
{
  return op == Op::kRegOut || op == Op::kMemOut;
}

constexpr bool isLoopPair(Op op)
{
  return op == Op::kLoopOut || op == Op::kLoopIn;
}

// Ops that only move bits: every result bit is a bit of a fanin or a
// constant 0 (zero-extending Cast). See Wiring.h.
constexpr bool isWiring(Op op)
{
  return op == Op::kBuf || op == Op::kSlice || op == Op::kConcat
         || op == Op::kReplicate || op == Op::kCast;
}

// Kinds whose function depends on the signed flag.
constexpr bool isSignSensitive(Op op)
{
  return op == Op::kCast || op == Op::kLt || op == Op::kDiv || op == Op::kMod
         || op == Op::kMul || op == Op::kShr || op == Op::kPow;
}

// Kinds that may carry the signed flag: the sign-sensitive ones, plus Pi
// and the state halves, where it records the declared signedness (for
// export only).
constexpr bool hasSignedFlag(Op op)
{
  return isSignSensitive(op) || op == Op::kPi || isStateOut(op)
         || isStateIn(op);
}

// The signed flag as a builder argument, so call sites name it.
enum class Signedness : uint8_t
{
  kUnsigned,
  kSigned,
};

// Kinds whose enum value is reserved but which no code may create yet.
constexpr bool isReserved(Op op)
{
  return op == Op::kMemOut || op == Op::kMemIn || op == Op::kMemRead
         || op == Op::kMemWrite;
}

// Kinds that can produce z. Two-state engines (ABC blasting, simulation)
// treat them as cut points or reject them.
constexpr bool isTriState(Op op)
{
  return op == Op::kTribuf || op == Op::kTriJoin;
}

}  // namespace syn::lir
