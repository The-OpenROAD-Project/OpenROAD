// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/lir/OperatorSpec.h"

#include <cstddef>
#include <cstdint>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "syn/lir/Checksum.h"
#include "syn/lir/ConstPool.h"
#include "syn/lir/Op.h"

namespace syn::lir {

uint32_t numInputPorts(const OpSpec& spec)
{
  uint32_t ports = 0;
  for (const OpOperand& operand : spec.operands) {
    ports += isPort(operand) ? 1 : 0;
  }
  return ports;
}

absl::Status checkOpSpec(const OpSpec& spec)
{
  if (spec.result_width == 0) {
    return absl::InvalidArgumentError("operator spec: result width 0");
  }
  if (spec.operands.empty()) {
    return absl::InvalidArgumentError("operator spec: no operands");
  }
  for (size_t k = 0; k < spec.operands.size(); ++k) {
    if (spec.operands[k].width == 0) {
      return absl::InvalidArgumentError(
          absl::StrCat("operator spec: operand ", k, " has width 0"));
    }
  }
  if (numInputPorts(spec) == 0) {
    return absl::InvalidArgumentError(
        "operator spec: every operand is a constant");
  }
  if (spec.operands.size() >= DatapathTerm::kNone) {
    return absl::InvalidArgumentError("operator spec: too many operands");
  }
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
      for (size_t t = 0; t < spec.terms.size(); ++t) {
        const DatapathTerm& term = spec.terms[t];
        const bool a_ok = term.a < spec.operands.size();
        const bool b_ok
            = term.b == DatapathTerm::kNone || term.b < spec.operands.size();
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
  hash = checksumCombine(hash, spec.result_width);
  hash = checksumCombine(hash, spec.operands.size());
  for (const OpOperand& operand : spec.operands) {
    hash = checksumCombine(hash, operand.width);
    hash = checksumCombine(hash, operand.is_signed ? 1 : 0);
    hash = checksumCombine(hash, consts.checksum(operand.constant));
  }
  hash = checksumCombine(hash, spec.terms.size());
  for (const DatapathTerm& term : spec.terms) {
    hash = checksumCombine(hash, term.a);
    hash = checksumCombine(hash, term.b);
    hash = checksumCombine(hash, term.negate ? 1 : 0);
  }
  return hash;
}

}  // namespace syn::lir
