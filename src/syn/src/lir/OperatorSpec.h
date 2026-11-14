// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Records describing operator implementation modules. An implementation
// module replaces one word-level operator. Its record holds the spec (the
// function at the ports, kept by any function-preserving edit) and the
// architecture (which generator built the gates, as the family's enum).
// The generators live in opt/operators.
//
// Ports: input port k is the k-th non-constant operand; output port 0 is the
// result. There are no other ports.

#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "syn/lir/ConstPool.h"
#include "syn/lir/Ids.h"
#include "syn/lir/Op.h"

namespace syn::lir {

enum class OpSpecKind : uint8_t
{
  kSingleOp,  // one L-IR op
  kDatapath,  // a signed sum of products (extractDatapath; not created yet)
};

struct OpOperand
{
  uint32_t width = 0;
  // Sign extension in a datapath spec. A single op uses OpSpec::is_signed.
  bool is_signed = false;
  // Valid: the operand is this constant (same width) and is not a port.
  ConstId constant = ConstId::invalid();
  bool operator==(const OpOperand&) const = default;
  template <typename H>
  friend H AbslHashValue(H h, const OpOperand& o)
  {
    return H::combine(std::move(h), o.width, o.is_signed, o.constant);
  }
};

// True if `operand` is a port of the implementation (not a constant).
inline bool isPort(const OpOperand& operand)
{
  return !operand.constant.isValid();
}

// One term of a datapath spec: +/- operands[a] (* operands[b]). Terms index
// OpSpec::operands (fewer than kNone); an operand may appear in several
// terms, or twice in one (a square). Example: a*b - c is {0, 1}, {2, negate}.
struct DatapathTerm
{
  static constexpr uint16_t kNone = 0xFFFF;

  uint16_t a = kNone;
  uint16_t b = kNone;  // kNone: the term is operands[a] alone
  bool negate = false;

  bool operator==(const DatapathTerm&) const = default;
  template <typename H>
  friend H AbslHashValue(H h, const DatapathTerm& t)
  {
    return H::combine(std::move(h), t.a, t.b, t.negate);
  }
};

struct OpSpec
{
  OpSpecKind kind = OpSpecKind::kSingleOp;
  // kSingleOp: the L-IR op, with its conventions (Op.h).
  // kDatapath: what is done with the sum S of the terms, an exact integer
  // (each operand extended by its is_signed, no overflow): kAdd = S
  // truncated to result_width, kLt = (S < 0), kEq = (S == 0).
  Op op = Op::kAdd;
  bool is_signed = false;  // kSingleOp: the op's signed flag
  uint32_t result_width = 0;
  std::vector<OpOperand> operands;  // kSingleOp: in fanin order
  std::vector<DatapathTerm> terms;  // kDatapath only

  bool operator==(const OpSpec&) const = default;
  template <typename H>
  friend H AbslHashValue(H h, const OpSpec& s)
  {
    return H::combine(std::move(h),
                      s.kind,
                      s.op,
                      s.is_signed,
                      s.result_width,
                      s.operands,
                      s.terms);
  }
};

// Deterministic checksum of `spec`; constant operands are read from
// `consts`.
uint64_t checksum(const OpSpec& spec, const ConstPool& consts);

// Number of input ports of a module implementing `spec`.
uint32_t numInputPorts(const OpSpec& spec);

// Checks the spec's own consistency (widths, operands, terms, datapath op).
// Constant values are checked by checkWellFormed.
absl::Status checkOpSpec(const OpSpec& spec);

// An implementation: a spec and an architecture. Key of OperatorPool's cache.
struct OperatorKey
{
  OpSpecId spec;
  uint16_t arch = 0;               // the family enum's value
  uint16_t generator_version = 0;  // bumped when a generator's output changes
  uint32_t arch_params = 0;        // e.g. Booth radix, tree type

  bool operator==(const OperatorKey&) const = default;
  template <typename H>
  friend H AbslHashValue(H h, const OperatorKey& k)
  {
    return H::combine(
        std::move(h), k.spec, k.arch, k.generator_version, k.arch_params);
  }
};

// The record an implementation module carries (Module::operatorImpl());
// an ordinary module has none.
struct OperatorImpl
{
  OperatorKey key;
  // Module::revision() when the record was set; any later edit changes it.
  uint64_t generated_revision = 0;
  // Set on a copy of an edited module (the copy's revision restarts at 0).
  bool modified = false;
};

}  // namespace syn::lir
