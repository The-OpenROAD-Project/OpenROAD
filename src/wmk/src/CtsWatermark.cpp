// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Clock tree watermark.
//
// Marks a leaf clock buffer by setting the parity of how many sequential cells
// it drives.  The parity is changed by moving one flip-flop's clock pin to a
// nearby buffer, which leaves the flop clocked and the tree connected but
// shifts one sink across the boundary between two buffers.
//
// Parity is a good carrier because it survives anything that does not add or
// remove a sink: routing, filling and metal fixes all preserve it.  It is also
// cheap to observe, which is what makes verification a simple count.
//
// The cost is skew.  Moving a sink changes the load on both buffers, so the
// move is undone if the clock's worst skew gets worse.
//
// Which buffers get paired is decided before the key is consulted, and a pair
// whose parity could not be set is claimed all the same.  Pairing with
// whichever buffer already showed the parity the key wanted, or quietly
// dropping the pairs that did not work out, would report a perfect extraction
// rate on a design this key had never marked -- and a rate like that is no
// evidence of anything.

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

#include "ClockTree.h"
#include "HmacSha256.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Scene.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"
#include "wmk/Watermark.h"

namespace wmk {

using odb::dbBlock;
using odb::dbInst;
using odb::dbITerm;
using odb::dbNet;

namespace {

std::int64_t manhattan(dbInst* a, dbInst* b)
{
  odb::dbBox* ba = a->getBBox();
  odb::dbBox* bb = b->getBBox();
  const std::int64_t ax = (ba->xMin() + ba->xMax()) / 2;
  const std::int64_t ay = (ba->yMin() + ba->yMax()) / 2;
  const std::int64_t bx = (bb->xMin() + bb->xMax()) / 2;
  const std::int64_t by = (bb->yMin() + bb->yMax()) / 2;
  return std::llabs(ax - bx) + std::llabs(ay - by);
}

// A sink that can be moved between two buffers without changing what the
// design does: an ordinary sequential clock pin, not a pin the flow has
// pinned down.
dbITerm* movableSink(dbInst* lcb)
{
  dbNet* net = singleOutputNet(lcb);
  if (net == nullptr) {
    return nullptr;
  }
  for (dbITerm* iterm : net->getITerms()) {
    if (!isSequentialClockSink(iterm)) {
      continue;
    }
    dbInst* inst = iterm->getInst();
    if (inst == nullptr || inst->isDoNotTouch() || inst->isFixed()) {
      continue;
    }
    return iterm;
  }
  return nullptr;
}

bool writeCtsClaims(const std::string& path,
                    const std::vector<CtsClaim>& claims)
{
  std::ofstream out(path);
  if (!out.is_open()) {
    return false;
  }
  out << "pair_idx,pair_key,target_lcb,other_lcb,target_bit,final_bit,"
         "skipped_reason\n";
  int idx = 0;
  for (const CtsClaim& c : claims) {
    out << idx++ << ',' << c.pair_key << ',' << c.target_lcb << ','
        << c.other_lcb << ',' << c.target_bit << ',' << c.final_bit << ",\n";
  }
  return out.good();
}

}  // namespace

int Watermark::ctsWatermark(const std::array<std::uint8_t, 32>& key,
                            const CtsOptions& opts,
                            const std::string& claims_file)
{
  dbBlock* block = db_->getChip() ? db_->getChip()->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::WMK, 70, "No block loaded; read a design first.");
    return 0;
  }

  const std::vector<dbInst*> lcbs = findLeafClockBuffers(block);
  if (lcbs.size() < 2) {
    logger_->warn(utl::WMK,
                  71,
                  "Found {} leaf clock buffer(s); at least two are needed to "
                  "move a sink between them.",
                  static_cast<int>(lcbs.size()));
    return 0;
  }

  const int dbu = block->getDbUnitsPerMicron();
  const std::int64_t max_dist
      = static_cast<std::int64_t>(opts.sibling_dist_um) * dbu;
  const float skew_before = worstClockSkew();

  std::vector<CtsClaim> claims;
  std::vector<bool> used(lcbs.size(), false);
  int rejected_skew = 0;
  int rejected_no_sink = 0;
  int held = 0;
  int moved = 0;

  for (size_t i = 0;
       i < lcbs.size() && static_cast<int>(claims.size()) < opts.num_pairs;
       ++i) {
    if (used[i]) {
      continue;
    }

    // Pair with the nearest free buffer in range.  The choice is geometric and
    // never looks at the key: pairing with whichever buffer already showed the
    // parity the key asked for would report a perfect extraction rate on a
    // design this key had never marked, so the rate would prove nothing.  Once
    // the pair is fixed it is claimed either way, and a parity the embedder
    // could not set simply shows up as a claim that does not hold.
    size_t best = lcbs.size();
    std::int64_t best_dist = 0;
    for (size_t j = i + 1; j < lcbs.size(); ++j) {
      if (used[j]) {
        continue;
      }
      const std::int64_t dist = manhattan(lcbs[i], lcbs[j]);
      if (dist > max_dist) {
        continue;
      }
      if (best == lcbs.size() || dist < best_dist) {
        best = j;
        best_dist = dist;
      }
    }
    if (best == lcbs.size()) {
      continue;
    }

    dbInst* a = lcbs[i];
    dbInst* b = lcbs[best];
    used[i] = used[best] = true;
    const std::string na = a->getName();
    const std::string nb = b->getName();
    const std::string pair_key = na + "+" + nb;

    // The key picks both which buffer carries the mark and what parity it
    // must show, so neither is guessable from the netlist.
    const std::array<std::uint8_t, 32> d
        = hmac_digest(key, {"pair", pair_key, na, nb});
    const int target_bit = d[0] & 1;
    const bool target_is_a = ((d[0] >> 1) & 1) != 0;

    dbInst* target = target_is_a ? a : b;
    dbInst* other = target_is_a ? b : a;

    CtsClaim claim;
    claim.pair_key = pair_key;
    claim.target_lcb = target->getName();
    claim.other_lcb = other->getName();
    claim.target_bit = target_bit;
    claim.final_bit = seqFanout(target) % 2;

    if (claim.final_bit != target_bit) {
      // Move one sink across the boundary to flip the parity.  Taking it from
      // the target lowers that count by one; taking it from the peer raises
      // it.  Either direction changes the parity, so use whichever buffer has
      // a sink free to move.
      dbNet* target_net = singleOutputNet(target);
      dbNet* other_net = singleOutputNet(other);
      dbITerm* sink = nullptr;
      dbNet* dest = nullptr;
      if (target_net != nullptr && other_net != nullptr) {
        sink = movableSink(target);
        dest = other_net;
        if (sink == nullptr) {
          sink = movableSink(other);
          dest = target_net;
        }
      }
      if (sink == nullptr) {
        ++rejected_no_sink;
      } else {
        dbNet* origin = sink->getNet();
        sink->disconnect();
        sink->connect(dest);
        if (worstClockSkew() > skew_before + opts.skew_margin_ns * 1e-9f) {
          // The mark would cost clock skew, so put the sink back.
          sink->disconnect();
          sink->connect(origin);
          ++rejected_skew;
        } else {
          ++moved;
          claim.final_bit = seqFanout(target) % 2;
        }
      }
    }

    if (claim.final_bit == target_bit) {
      ++held;
    }
    claims.push_back(claim);
  }

  if (!writeCtsClaims(claims_file, claims)) {
    logger_->error(
        utl::WMK, 72, "Could not write claims to '{}'.", claims_file);
    return 0;
  }

  logger_->info(utl::WMK,
                73,
                "CTS watermark: {} pairs claimed from {} leaf clock buffers, "
                "{} at the keyed parity ({} sinks moved, {} rejected on skew, "
                "{} with no movable sink).",
                static_cast<int>(claims.size()),
                static_cast<int>(lcbs.size()),
                held,
                moved,
                rejected_skew,
                rejected_no_sink);
  return static_cast<int>(claims.size());
}

}  // namespace wmk
