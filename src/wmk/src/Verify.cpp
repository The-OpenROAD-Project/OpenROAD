// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Verification of the placement and CTS watermarks against a loaded design.
//
// Both stages commit their claims to a file at embed time, and verification
// re-observes each claimed object and compares it to the value the key calls
// for.  The claim file supplies the names of the marked objects and nothing
// more: which of a pair is the target and the value it carries are derived
// from the key again here, exactly as the embedder derived them, and a file
// that recorded anything else is refused.  That is what makes the file
// evidence.  A list of names whose target values could be read from the file
// itself could be written by anyone, from any layout, to match it perfectly.
// The routing stage is different -- it is key-recoverable and statistical --
// and is handled by selectNetsKeyed and verifyRouting.
//
// Ownership requires both a sufficient extraction rate and count-dependent
// evidence. Routing and filling can disturb marks, so an exact match is not
// required. Preserve every checked/held count for the caller's decision.

#include <array>
#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Claims.h"
#include "ClockTree.h"
#include "Marks.h"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "utl/Logger.h"
#include "wmk/VerifyResult.h"
#include "wmk/Watermark.h"

namespace wmk {

using odb::dbBlock;
using odb::dbInst;

namespace {

// The x coordinate a placement claim is expressed in terms of.  The embedder
// uses the instance bounding box, not the origin, so the two must agree.
int instLeftEdge(dbInst* inst)
{
  return inst->getBBox()->xMin();
}

// What counts as a leaf clock buffer's sequential fanout -- and so what the
// parity the watermark carries means -- comes from ClockTree.h, which the
// embedder calls too.  A second definition here that drifted from that one
// would turn a valid watermark into a failed verification.

int recordedBit(const ClaimRow& row)
{
  return claimField(row, "target_bit")[0] - '0';
}

}  // namespace

VerifyResult Watermark::verifyPlacement(const std::array<std::uint8_t, 32>& key,
                                        const std::string& claims_file)
{
  VerifyResult result;
  dbBlock* block = db_->getChip() ? db_->getChip()->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::WMK, 30, "No block loaded; read a design first.");
    return result;
  }

  std::vector<ClaimRow> rows;
  std::string error;
  if (!readClaims(claims_file, ClaimStage::kPlacement, rows, error)) {
    logger_->error(utl::WMK, 31, "Placement claims: {}.", error);
    return result;
  }

  // Every scored claim must record the bit this key derives for its pair.
  // Check them all before scoring any: a file that disagrees anywhere was not
  // written by an embedder holding this key, and no rate measured from it
  // would mean anything.
  struct Scored
  {
    OrderedPair pair;
    int target;
  };
  std::vector<Scored> scored;
  int mismatched = 0;
  std::string example;
  for (const ClaimRow& row : rows) {
    if (!claimIsCheckable(row) || claimField(row, "kind") != "pair") {
      continue;
    }
    const auto [a_name, b_name] = claimNames(row, ClaimStage::kPlacement);
    const OrderedPair pair = orderPair(a_name, b_name);
    // The row records its bit relative to its own A and B.
    const int recorded
        = a_name == pair.first ? recordedBit(row) : 1 - recordedBit(row);
    const int target = placementTargetBit(key, pair);
    if (recorded != target) {
      ++mismatched;
      if (example.empty()) {
        example = a_name + "|" + b_name;
      }
      continue;
    }
    scored.push_back({.pair = pair, .target = target});
  }
  if (mismatched > 0) {
    logger_->error(utl::WMK,
                   121,
                   "{} of {} placement claims record a target bit that this "
                   "key does not derive (for example {}); the claim file was "
                   "not produced with this key.",
                   mismatched,
                   mismatched + static_cast<int>(scored.size()),
                   example);
  }

  for (const Scored& claim : scored) {
    ++result.checked;
    dbInst* a = block->findInst(claim.pair.first.c_str());
    dbInst* b = block->findInst(claim.pair.second.c_str());
    if (a == nullptr || b == nullptr) {
      logger_->info(utl::WMK,
                    32,
                    "Placement claim {}|{}: instance missing.",
                    claim.pair.first,
                    claim.pair.second);
      continue;
    }

    // The bit is which of the pair sits to the left.
    const int observed = instLeftEdge(a) < instLeftEdge(b) ? 0 : 1;
    if (observed == claim.target) {
      ++result.held;
    } else {
      logger_->info(utl::WMK,
                    33,
                    "Placement claim {}|{}: bit={} want={}.",
                    claim.pair.first,
                    claim.pair.second,
                    observed,
                    claim.target);
    }
  }

  logger_->info(utl::WMK,
                34,
                "Placement watermark: {} / {} claims hold (r_P={:.4f}).",
                result.held,
                result.checked,
                result.rate());
  return result;
}

VerifyResult Watermark::verifyCts(const std::array<std::uint8_t, 32>& key,
                                  const std::string& claims_file)
{
  VerifyResult result;
  dbBlock* block = db_->getChip() ? db_->getChip()->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::WMK, 35, "No block loaded; read a design first.");
    return result;
  }

  std::vector<ClaimRow> rows;
  std::string error;
  if (!readClaims(claims_file, ClaimStage::kCts, rows, error)) {
    logger_->error(utl::WMK, 36, "CTS claims: {}.", error);
    return result;
  }

  if (!hasLiberty()) {
    logger_->error(
        utl::WMK, 108, "Read Liberty before verifying a CTS watermark.");
  }

  // As for placement: the key says which buffer of each pair is the target
  // and what parity it carries, and every row must agree before any is
  // scored.  The embedder never marks a buffer twice, so a file in which two
  // pairs derive the same target was not written by it either.
  std::vector<CtsTarget> scored;
  std::set<std::string> targets;
  int mismatched = 0;
  std::string example;
  for (const ClaimRow& row : rows) {
    if (!claimIsCheckable(row)) {
      continue;
    }
    const auto [target_name, other_name] = claimNames(row, ClaimStage::kCts);
    const CtsTarget derived
        = ctsTarget(key, orderPair(target_name, other_name));
    if (derived.target != target_name || derived.bit != recordedBit(row)) {
      ++mismatched;
      if (example.empty()) {
        example = target_name;
      }
      continue;
    }
    if (!targets.insert(derived.target).second) {
      logger_->error(utl::WMK,
                     123,
                     "CTS claims name {} as the target of more than one "
                     "pair; the claim file was not produced by the embedder.",
                     derived.target);
    }
    scored.push_back(derived);
  }
  if (mismatched > 0) {
    logger_->error(utl::WMK,
                   122,
                   "{} of {} CTS claims record a target buffer or parity that "
                   "this key does not derive (for example {}); the claim file "
                   "was not produced with this key.",
                   mismatched,
                   mismatched + static_cast<int>(scored.size()),
                   example);
  }

  for (const CtsTarget& claim : scored) {
    ++result.checked;
    dbInst* lcb = block->findInst(claim.target.c_str());
    if (lcb == nullptr) {
      logger_->info(
          utl::WMK, 37, "CTS claim {}: instance missing.", claim.target);
      continue;
    }

    const auto fanout = seqFanout(lcb, sta_->getDbNetwork());
    if (!fanout) {
      logger_->info(utl::WMK,
                    109,
                    "CTS claim {}: carrier or fanout cannot be classified "
                    "using Liberty.",
                    claim.target);
      continue;
    }
    const int observed = *fanout % 2;
    if (observed == claim.bit) {
      ++result.held;
    } else {
      logger_->info(utl::WMK,
                    38,
                    "CTS claim {}: parity={} want={}.",
                    claim.target,
                    observed,
                    claim.bit);
    }
  }

  logger_->info(utl::WMK,
                39,
                "CTS watermark: {} / {} claims hold (r_C={:.4f}).",
                result.held,
                result.checked,
                result.rate());
  return result;
}

}  // namespace wmk
