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
// Ownership requires both a sufficient extraction rate and count-dependent
// evidence. Routing and filling can disturb marks, so an exact match is not
// required. Preserve every checked/held count for the caller's decision.

#include <string>
#include <vector>

#include "Claims.h"
#include "ClockTree.h"
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
  if (!readClaims(claims_file, ClaimStage::kPlacement, rows, error)) {
    logger_->error(utl::WMK, 31, "Placement claims: {}.", error);
    return result;
  }

  for (const ClaimRow& row : rows) {
    if (!claimIsCheckable(row) || claimField(row, "kind") != "pair") {
      continue;
    }
    const std::string a_name = claimField(row, "A_name");
    const std::string b_name = claimField(row, "B_name");
    const int target = claimField(row, "target_bit")[0] - '0';
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
  if (!readClaims(claims_file, ClaimStage::kCts, rows, error)) {
    logger_->error(utl::WMK, 36, "CTS claims: {}.", error);
    return result;
  }

  if (!hasLiberty()) {
    logger_->error(
        utl::WMK, 108, "Read Liberty before verifying a CTS watermark.");
  }

  for (const ClaimRow& row : rows) {
    if (!claimIsCheckable(row)) {
      continue;
    }
    const std::string lcb_name = claimField(row, "target_lcb");
    // The row's own record of how the embedding turned out is deliberately not
    // consulted.  Skipping the claims that record a failure would let a claim
    // file decide its own denominator, and the rate would come out at one for
    // any file that was honest about what it could not set -- on any design,
    // marked or not.  What the design shows is measured against what the key
    // asked for, and nothing else.
    const int target = claimField(row, "target_bit")[0] - '0';
    ++result.checked;

    dbInst* lcb = block->findInst(lcb_name.c_str());
    if (lcb == nullptr) {
      logger_->info(utl::WMK, 37, "CTS claim {}: instance missing.", lcb_name);
      continue;
    }

    const auto fanout = seqFanout(lcb, sta_->getDbNetwork());
    if (!fanout) {
      logger_->info(utl::WMK,
                    109,
                    "CTS claim {}: carrier or fanout cannot be classified "
                    "using Liberty.",
                    lcb_name);
      continue;
    }
    const int observed = *fanout % 2;
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
