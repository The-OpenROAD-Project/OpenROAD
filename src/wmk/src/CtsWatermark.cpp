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
// Which buffers get paired is keyed as well.  Eligibility is public -- same
// clock, close enough that a moved sink stays local -- and the key then orders
// the eligible pairs and takes a greedy prefix, so an observer can list the
// candidates but not say which of them carry marks.
//
// The key orders pairs by their names, never by the parity they currently show,
// and a pair whose parity could not be set is claimed all the same.  Pairing
// with whichever buffer already showed the parity the key wanted, or quietly
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
  // Whether skew can be evaluated at all decides whether the guard below has
  // any force.  Reading a design from a database restores no liberty and no
  // constraints, so a caller who has not set timing up would otherwise get the
  // marks committed with the guard silently inert -- and a clock tree damaged
  // without anything having said so.
  const bool skew_known = clockSkewAvailable();
  if (!skew_known) {
    logger_->warn(utl::WMK,
                  74,
                  "No timing is set up, so clock skew cannot be evaluated and "
                  "marks will be committed without the skew guard.  Read "
                  "liberty and constraints first to keep it in force.");
  }
  const float skew_before = worstClockSkew();

  // Enumerate the eligible pairs, then let the key order them.  Eligibility is
  // public -- two leaf buffers close enough together that moving a sink between
  // them stays local -- but which of those pairs carries a mark is not.
  struct Candidate
  {
    size_t i, j;
    std::string pair_key;
    std::array<std::uint8_t, 32> sort_key;
  };
  std::vector<Candidate> candidates;
  for (size_t i = 0; i < lcbs.size(); ++i) {
    for (size_t j = i + 1; j < lcbs.size(); ++j) {
      if (manhattan(lcbs[i], lcbs[j]) > max_dist) {
        continue;
      }
      Candidate c;
      c.i = i;
      c.j = j;
      // findLeafClockBuffers returns them in name order, so i < j already means
      // the identifier is built from the sorted names.
      c.pair_key = lcbs[i]->getName() + "+" + lcbs[j]->getName();
      c.sort_key = hmac_digest(key, {"pair_sort", c.pair_key});
      candidates.push_back(std::move(c));
    }
  }
  std::sort(candidates.begin(),
            candidates.end(),
            [](const Candidate& x, const Candidate& y) {
              if (x.sort_key != y.sort_key) {
                return x.sort_key < y.sort_key;
              }
              return x.pair_key < y.pair_key;
            });

  std::vector<CtsClaim> claims;
  // A buffer that has been claimed is frozen: its fanout is the evidence, so it
  // can be neither the target nor the source of a later move.  A buffer that
  // only lent a sink is still free, which is what the paper's rule amounts to.
  std::vector<bool> claimed(lcbs.size(), false);
  int rejected_skew = 0;
  int rejected_no_sink = 0;
  int rejected_drive = 0;
  int held = 0;
  int moved = 0;

  for (const Candidate& c : candidates) {
    if (static_cast<int>(claims.size()) >= opts.num_pairs) {
      break;
    }
    if (claimed[c.i] || claimed[c.j]) {
      continue;
    }

    dbInst* a = lcbs[c.i];
    dbInst* b = lcbs[c.j];
    const std::string na = a->getName();
    const std::string nb = b->getName();

    // The key picks both which buffer carries the mark and what parity it
    // must show, so neither is guessable from the netlist.
    const std::array<std::uint8_t, 32> d
        = hmac_digest(key, {"pair", c.pair_key, na, nb});
    const int target_bit = d[0] & 1;
    const bool target_is_a = ((d[0] >> 1) & 1) != 0;

    dbInst* target = target_is_a ? a : b;
    dbInst* other = target_is_a ? b : a;

    CtsClaim claim;
    claim.pair_key = c.pair_key;
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
        sink = movableSink(other);
        dest = target_net;
        if (sink == nullptr) {
          sink = movableSink(target);
          dest = other_net;
        }
      }
      if (sink == nullptr) {
        ++rejected_no_sink;
      } else {
        dbNet* origin = sink->getNet();
        sink->disconnect();
        sink->connect(dest);
        const bool skew_ok
            = !skew_known
              || worstClockSkew() <= skew_before + opts.skew_margin_ns * 1e-9f;
        // The buffer that gained a sink now drives more load, so whether it
        // still can is the library's question, not one this code should answer
        // with a number of its own.
        const bool drive_ok
            = driverHeadroomOk(
                  target, opts.slew_headroom_frac, opts.cap_headroom_frac)
              && driverHeadroomOk(
                  other, opts.slew_headroom_frac, opts.cap_headroom_frac);
        if (!skew_ok || !drive_ok) {
          // The mark would cost more than the clock can spare, so put the sink
          // back.
          sink->disconnect();
          sink->connect(origin);
          if (!skew_ok) {
            ++rejected_skew;
          } else {
            ++rejected_drive;
          }
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
    claimed[target_is_a ? c.i : c.j] = true;
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
                "{} with no movable sink, {} on drive strength).",
                static_cast<int>(claims.size()),
                static_cast<int>(lcbs.size()),
                held,
                moved,
                rejected_skew,
                rejected_no_sink,
                rejected_drive);
  return static_cast<int>(claims.size());
}

}  // namespace wmk
