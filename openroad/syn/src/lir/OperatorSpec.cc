// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/lir/OperatorSpec.h"

#include <cassert>
#include <cstddef>  // IWYU pragma: keep (used in asserts)
#include <cstdint>
#include <iterator>
#include <limits>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "syn/lir/Checksum.h"
#include "syn/lir/ConstPool.h"
#include "syn/lir/Op.h"

namespace syn::lir {

int numInputPorts(const OpSpec& spec)
{
  assert(spec.operands.size()
         <= static_cast<size_t>(std::numeric_limits<int>::max()));
  int ports = 0;
  for (const OpOperand& operand : spec.operands) {
    ports += isPort(operand) ? 1 : 0;
  }
  return ports;
}

absl::Status checkOpSpec(const OpSpec& spec)
{
  if (spec.result_width <= 0) {
    return absl::InvalidArgumentError("operator spec: result width 0");
  }
  if (spec.operands.empty()) {
    return absl::InvalidArgumentError("operator spec: no operands");
  }
  const int64_t total_operands = std::ssize(spec.operands);
  for (int64_t k = 0; k < total_operands; ++k) {
    if (spec.operands[k].width <= 0) {
      return absl::InvalidArgumentError(
          absl::StrCat("operator spec: operand ", k, " has width 0"));
    }
  }
  if (numInputPorts(spec) == 0) {
    return absl::InvalidArgumentError(
        "operator spec: every operand is a constant");
  }
  if (total_operands > std::numeric_limits<int16_t>::max()) {
    return absl::InvalidArgumentError("operator spec: too many operands");
  }
  const int num_operands = static_cast<int>(total_operands);
  switch (spec.kind) {
    case OpSpecKind::kSingleOp:
      if (!spec.terms.empty()) {
        return absl::InvalidArgumentError(
            "operator spec: a single-op spec has no terms");
      }
      return absl::OkStatus();
    case OpSpecKind::kDatapath:
      if (spec.op != Op::kAdd && spec.op != Op::kLt && spec.op != Op::kEq) {
        return absl::InvalidArgumentError(
            absl::StrCat("operator spec: datapath op ",
                         opName(spec.op),
                         " is not add, lt or eq"));
      }
      if (spec.terms.empty()) {
        return absl::InvalidArgumentError(
            "operator spec: a datapath spec needs terms");
      }
      const int64_t num_terms = std::ssize(spec.terms);
      for (int64_t t = 0; t < num_terms; ++t) {
        const DatapathTerm& term = spec.terms[t];
        const bool a_ok = term.a >= 0 && term.a < num_operands;
        const bool b_ok = term.b == DatapathTerm::kNone
                          || (term.b >= 0 && term.b < num_operands);
        if (!a_ok || !b_ok) {
          return absl::InvalidArgumentError(absl::StrCat(
              "operator spec: term ", t, " refers to a missing operand"));
        }
      }
      return absl::OkStatus();
  }
  return absl::InvalidArgumentError("operator spec: unknown kind");
}

uint64_t checksum(const OpSpec& spec, const ConstPool& consts)
{
  uint64_t hash
      = checksumCombine(kChecksumSeed, static_cast<uint64_t>(spec.kind));
  hash = checksumCombine(hash, static_cast<uint64_t>(spec.op));
  hash = checksumCombine(hash, spec.is_signed ? 1 : 0);
  hash = checksumCombine(hash, static_cast<uint32_t>(spec.result_width));
  hash = checksumCombine(hash, spec.operands.size());
  for (const OpOperand& operand : spec.operands) {
    hash = checksumCombine(hash, static_cast<uint32_t>(operand.width));
    hash = checksumCombine(hash, operand.is_signed ? 1 : 0);
    hash = checksumCombine(hash, consts.checksum(operand.constant));
  }
  hash = checksumCombine(hash, spec.terms.size());
  for (const DatapathTerm& term : spec.terms) {
    hash = checksumCombine(hash, static_cast<uint16_t>(term.a));
    hash = checksumCombine(hash, static_cast<uint16_t>(term.b));
    hash = checksumCombine(hash, term.negate ? 1 : 0);
  }
  return hash;
}

}  // namespace syn::lir
