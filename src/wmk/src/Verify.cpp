// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Verification of the placement and CTS watermarks against a loaded design.
//
// Both stages commit their claims to a file at embed time, and verification
// re-observes each claimed object and compares it to the committed value.  The
// key is not needed here: it was consumed at embed time to derive the target
// values, which the claim file records.  The routing stage is different -- it
// is key-recoverable and statistical -- and is handled by selectNetsKeyed and
// reportWatermark.
//
// Ownership is decided by the extraction rate, the fraction of claims that
// still hold, against a threshold.  Routing and filling legitimately disturb a
// few marked objects, so an exact match is not required and not expected.

#include <string>
#include <vector>

#include "Claims.h"
#include "ClockTree.h"
#include "odb/db.h"
#include "utl/Logger.h"
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

// A claimed bit must be exactly "0" or "1".  Anything else means the claim
// file is damaged, and a verifier that read it as zero would quietly measure
// an extraction rate against a target nobody committed to.
bool parseClaimBit(const std::string& text, int& bit)
{
  if (text == "0" || text == "1") {
    bit = text[0] - '0';
    return true;
  }
  return false;
}

}  // namespace

VerifyResult Watermark::verifyPlacement(const std::string& claims_file)
{
  VerifyResult result;
  dbBlock* block = db_->getChip() ? db_->getChip()->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::WMK, 30, "No block loaded; read a design first.");
    return result;
  }

  std::vector<ClaimRow> rows;
  std::string error;
  if (!readClaims(claims_file, rows, error)) {
    logger_->error(utl::WMK, 31, "Placement claims: {}.", error);
    return result;
  }

  for (const ClaimRow& row : rows) {
    if (!claimIsCheckable(row) || claimField(row, "kind") != "pair") {
      continue;
    }
    const std::string a_name = claimField(row, "A_name");
    const std::string b_name = claimField(row, "B_name");
    if (a_name.empty() || b_name.empty()) {
      continue;
    }
    const std::string target_bit = claimField(row, "target_bit");
    int target = 0;
    if (!parseClaimBit(target_bit, target)) {
      logger_->error(utl::WMK,
                     100,
                     "Placement claim {}|{}: target_bit is \"{}\", not 0 or 1.",
                     a_name,
                     b_name,
                     target_bit);
    }
    ++result.checked;

    dbInst* a = block->findInst(a_name.c_str());
    dbInst* b = block->findInst(b_name.c_str());
    if (a == nullptr || b == nullptr) {
      logger_->info(utl::WMK,
                    32,
                    "Placement claim {}|{}: instance missing.",
                    a_name,
                    b_name);
      continue;
    }

    // The bit is which of the pair sits to the left.
    const int observed = instLeftEdge(a) < instLeftEdge(b) ? 0 : 1;
    if (observed == target) {
      ++result.held;
    } else {
      logger_->info(utl::WMK,
                    33,
                    "Placement claim {}|{}: bit={} want={}.",
                    a_name,
                    b_name,
                    observed,
                    target);
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

VerifyResult Watermark::verifyCts(const std::string& claims_file)
{
  VerifyResult result;
  dbBlock* block = db_->getChip() ? db_->getChip()->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::WMK, 35, "No block loaded; read a design first.");
    return result;
  }

  std::vector<ClaimRow> rows;
  std::string error;
  if (!readClaims(claims_file, rows, error)) {
    logger_->error(utl::WMK, 36, "CTS claims: {}.", error);
    return result;
  }

  for (const ClaimRow& row : rows) {
    if (!claimIsCheckable(row)) {
      continue;
    }
    const std::string lcb_name = claimField(row, "target_lcb");
    if (lcb_name.empty()) {
      continue;
    }
    // The row's own record of how the embedding turned out is deliberately not
    // consulted.  Skipping the claims that record a failure would let a claim
    // file decide its own denominator, and the rate would come out at one for
    // any file that was honest about what it could not set -- on any design,
    // marked or not.  What the design shows is measured against what the key
    // asked for, and nothing else.
    const std::string target_bit = claimField(row, "target_bit");
    int target = 0;
    if (!parseClaimBit(target_bit, target)) {
      logger_->error(utl::WMK,
                     101,
                     "CTS claim {}: target_bit is \"{}\", not 0 or 1.",
                     lcb_name,
                     target_bit);
    }
    ++result.checked;

    dbInst* lcb = block->findInst(lcb_name.c_str());
    if (lcb == nullptr) {
      logger_->info(utl::WMK, 37, "CTS claim {}: instance missing.", lcb_name);
      continue;
    }

    const int observed = seqFanout(lcb) % 2;
    if (observed == target) {
      ++result.held;
    } else {
      logger_->info(utl::WMK,
                    38,
                    "CTS claim {}: parity={} want={}.",
                    lcb_name,
                    observed,
                    target);
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
