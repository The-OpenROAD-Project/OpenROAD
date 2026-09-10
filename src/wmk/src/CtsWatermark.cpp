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
// Moving a sink changes both buffer loads. Fixed latency-spread and
// endpoint setup/hold budgets bound the timing cost of each accepted move.
//
// Which buffers get paired is keyed as well.  Eligibility is public -- same
// clock logic, close enough that a moved sink stays local -- and the key then
// orders the eligible pairs and takes a greedy prefix, so an observer can list
// the candidates but not say which of them carry marks.
//
// The key orders pairs by their names, never by the parity they currently show,
// and a pair whose parity could not be set is claimed all the same.  Pairing
// with whichever buffer already showed the parity the key wanted, or quietly
// dropping the pairs that did not work out, would report a perfect extraction
// rate on a design this key had never marked -- and a rate like that is no
// evidence of anything.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <optional>
#include <ostream>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "ClaimFile.h"
#include "ClockTree.h"
#include "HmacSha256.h"
#include "Options.h"
#include "Timing.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "utl/Logger.h"
#include "wmk/Watermark.h"

namespace wmk {

using odb::dbBlock;
using odb::dbInst;
using odb::dbITerm;
using odb::dbNet;

namespace {

// A buffer's centre in database units.  Held rather than re-read: the distance
// test below runs once per nearby pair, and getBBox is a database lookup.
struct Centre
{
  std::int64_t x, y;
};

Centre centreOf(dbInst* inst)
{
  odb::dbBox* box = inst->getBBox();
  return {.x = (static_cast<std::int64_t>(box->xMin()) + box->xMax()) / 2,
          .y = (static_cast<std::int64_t>(box->yMin()) + box->yMax()) / 2};
}

std::int64_t manhattan(const Centre& a, const Centre& b)
{
  return std::llabs(a.x - b.x) + std::llabs(a.y - b.y);
}

// The cell a coordinate falls in, flooring rather than truncating: a core that
// starts left of the origin has negative coordinates, and truncation toward
// zero would fold the cells either side of it together.
std::int64_t cellOf(std::int64_t v, std::int64_t cell)
{
  return v >= 0 ? v / cell : -((-v + cell - 1) / cell);
}

// Clock membership must be identical, not just overlapping. Two unknown
// sets cannot establish that reconnecting a sink preserves its clocks.
bool sameClockSet(const ClockIdentities& a, const ClockIdentities& b)
{
  return !a.empty() && a == b;
}

// A sink that can be moved between two buffers without changing what the
// design does: an ordinary sequential clock pin, not a pin the flow has
// pinned down.
dbITerm* movableSink(dbInst* lcb, dbNet* destination, sta::dbNetwork* network)
{
  dbNet* net = singleOutputNet(lcb);
  if (net == nullptr) {
    return nullptr;
  }
  for (dbITerm* iterm : net->getITerms()) {
    if (!isSequentialClockSink(iterm, network)) {
      continue;
    }
    if (!canMoveClockSink(iterm, destination)) {
      continue;
    }
    return iterm;
  }
  return nullptr;
}

void writeCtsClaims(std::ostream& out, const std::vector<CtsClaim>& claims)
{
  out << "pair_idx,pair_key,target_lcb,other_lcb,target_bit,final_bit,"
         "skipped_reason\n";
  int idx = 0;
  for (const CtsClaim& c : claims) {
    out << idx++ << ',' << c.pair_key << ',' << c.target_lcb << ','
        << c.other_lcb << ',' << c.target_bit << ',' << c.final_bit << ",\n";
  }
}

}  // namespace

class Watermark::CtsEmbedding
{
 public:
  CtsEmbedding(Watermark& watermark,
               dbBlock* block,
               const std::vector<dbInst*>& lcbs,
               const std::array<std::uint8_t, 32>& key,
               const CtsOptions& opts)
      : watermark_(watermark),
        block_(block),
        lcbs_(lcbs),
        key_(key),
        opts_(opts),
        network_(watermark.sta_->getDbNetwork()),
        max_dist_(static_cast<std::int64_t>(opts.sibling_dist_um
                                            * block->getDbUnitsPerMicron())),
        claimed_(lcbs.size(), false)
  {
  }

  int run(ClaimFile& output)
  {
    prepareTiming();
    collectClocks();
    indexBuffers();
    enumerateCandidates();
    edits_.reserve(std::min(candidates_.size(), lcbs_.size()));
    try {
      for (const Candidate& c : candidates_) {
        if (std::cmp_greater_equal(claims_.size(), opts_.num_pairs)) {
          break;
        }
        if (!claimed_[c.i] && !claimed_[c.j]) {
          apply(c);
        }
      }
      output.publish([&](std::ostream& out) { writeCtsClaims(out, claims_); });
    } catch (...) {
      restore();
      throw;
    }
    watermark_.logger_->info(
        utl::WMK,
        73,
        "CTS watermark: {} pairs claimed from {} leaf clock buffers, "
        "{} at the keyed parity ({} sinks moved, {} rejected on skew, "
        "{} with no movable sink, {} on drive strength, "
        "{} with different or unknown clock sets, {} with different or "
        "unproven clock logic, {} on setup/hold timing).",
        static_cast<int>(claims_.size()),
        static_cast<int>(lcbs_.size()),
        held_,
        moved_,
        rejected_skew_,
        rejected_no_sink_,
        rejected_drive_,
        rejected_cross_clock_,
        rejected_clock_logic_,
        rejected_timing_);
    return static_cast<int>(claims_.size());
  }

 private:
  // Enumerate the eligible pairs, then let the key order them.  Eligibility is
  // public -- two leaf buffers close enough together that moving a sink between
  // them stays local -- but which of those pairs carries a mark is not.
  struct Candidate
  {
    size_t i, j;
    std::string pair_key;
    std::array<std::uint8_t, 32> sort_key;
  };

  struct SinkEdit
  {
    dbITerm* sink;
    dbNet* origin;
    dbNet* destination;
  };

  void prepareTiming()
  {
    // Establish fresh clock parasitics before recording a fixed baseline.
    // Each clock and analysis scene has its own budget; an unrelated clock's
    // larger skew must not hide degradation of the clock being edited.
    const bool can_estimate
        = watermark_.canEstimateParasitics(/* clock */ true);
    if (can_estimate) {
      for (dbNet* net : block_->getNets()) {
        if (net->getSigType() == odb::dbSigType::CLOCK) {
          watermark_.reestimateNetParasitics(net, nullptr);
        }
      }
    }
    skew_before_ = can_estimate ? clockSkews(watermark_.sta_) : ClockSkews{};
    // Latency spread alone does not bound setup/hold path degradation. Keep
    // every constrained endpoint's original slack as a second timing guard.
    timing_before_ = endpointSlacks(watermark_.sta_);
  }

  void collectClocks()
  {
    // Which clocks reach each buffer, computed once.  A sink may only move
    // between two buffers with identical clock sets: findLeafClockBuffers
    // returns the leaves of every clock tree in the design, and two trees can
    // run alongside each other, so distance alone would let a move reconnect a
    // flop to a different clock and change what the design does.
    lcb_clocks_.reserve(lcbs_.size());
    int with_a_clock = 0;
    for (dbInst* lcb : lcbs_) {
      lcb_clocks_.push_back(watermark_.clockIdentitiesAt(lcb));
      if (!lcb_clocks_.back().empty()) {
        ++with_a_clock;
      }
    }
    // Rejecting every pair because the question could not be asked is a very
    // different situation from rejecting the few that really do span two trees,
    // and it looks identical in the counts.  A database read back without
    // constraints has a clock tree but no clock, and would otherwise report
    // that every pair spanned two clocks and quietly mark nothing.
    if (with_a_clock == 0) {
      watermark_.logger_->warn(
          utl::WMK,
          106,
          "No clock reaches any of the {} leaf clock buffers, so no "
          "pair can be shown to stay on one clock and none will be "
          "marked.  Read constraints (create_clock) before embedding.",
          static_cast<int>(lcbs_.size()));
    }
  }

  void indexBuffers()
  {
    branches_.reserve(lcbs_.size());
    for (dbInst* lcb : lcbs_) {
      branches_.push_back(clockBranch(lcb, network_));
    }

    centres_.reserve(lcbs_.size());
    for (dbInst* lcb : lcbs_) {
      centres_.push_back(centreOf(lcb));
    }

    // Only buffers within max_dist of each other can be paired, so the search
    // does not have to look at every pair.  Two buffers that close differ by at
    // most max_dist on each axis, so on a grid of cells one max_dist on a side
    // every pair worth considering lies in the same cell or in one of the eight
    // around it.  Testing those nine cells finds exactly the pairs the full
    // sweep found, in time proportional to the number of buffers that really
    // are near one another instead of to the square of the buffer count.  Where
    // a design packs every buffer inside one max_dist the two are the same
    // amount of work, because then every pair genuinely is a candidate.
    const std::int64_t cell = std::max<std::int64_t>(max_dist_, 1);
    for (size_t i = 0; i < lcbs_.size(); ++i) {
      grid_[{cellOf(centres_[i].x, cell), cellOf(centres_[i].y, cell)}]
          .push_back(i);
    }
  }

  void enumerateCandidates()
  {
    const std::int64_t cell = std::max<std::int64_t>(max_dist_, 1);
    for (size_t i = 0; i < lcbs_.size(); ++i) {
      const std::int64_t cx = cellOf(centres_[i].x, cell);
      const std::int64_t cy = cellOf(centres_[i].y, cell);
      for (std::int64_t dx = -1; dx <= 1; ++dx) {
        for (std::int64_t dy = -1; dy <= 1; ++dy) {
          const auto cell_it = grid_.find({cx + dx, cy + dy});
          if (cell_it == grid_.end()) {
            continue;
          }
          // Indices went into each cell in increasing order, so taking only
          // those above i visits each pair once and keeps i < j.
          for (size_t j : cell_it->second) {
            if (j <= i) {
              continue;
            }
            if (manhattan(centres_[i], centres_[j]) > max_dist_) {
              continue;
            }
            // Both sets are sorted. Empty or unequal sets cannot establish
            // that the sink will remain on exactly the same clocks.
            if (!sameClockSet(lcb_clocks_[i], lcb_clocks_[j])) {
              ++rejected_cross_clock_;
              continue;
            }
            // Named clocks do not describe enables or inversion. Only move
            // between branches with the same unconditional clock function.
            if (!branches_[i] || !branches_[j]
                || branches_[i] != branches_[j]) {
              ++rejected_clock_logic_;
              continue;
            }
            Candidate c;
            c.i = i;
            c.j = j;
            // findLeafClockBuffers returns them in name order, so i < j already
            // means the identifier is built from the sorted names.
            c.pair_key = lcbs_[i]->getName() + "+" + lcbs_[j]->getName();
            c.sort_key = hmac_digest(key_, {"pair_sort", c.pair_key});
            candidates_.push_back(std::move(c));
          }
        }
      }
    }
    std::ranges::sort(candidates_, [](const Candidate& x, const Candidate& y) {
      if (x.sort_key != y.sort_key) {
        return x.sort_key < y.sort_key;
      }
      return x.pair_key < y.pair_key;
    });
  }

  int fanoutParity(dbInst* buffer) const
  {
    // Enumeration established this fanout and sink moves preserve its
    // definition. Report a broken invariant before emitting invalid claims.
    const auto fanout = seqFanout(buffer, network_);
    if (!fanout) {
      watermark_.logger_->error(
          utl::WMK,
          118,
          "Cannot determine sequential fanout of clock buffer {}.",
          buffer->getName());
    }
    return *fanout % 2;
  }

  void apply(const Candidate& c)
  {
    dbInst* a = lcbs_[c.i];
    dbInst* b = lcbs_[c.j];
    const std::string na = a->getName();
    const std::string nb = b->getName();

    // The key picks both which buffer carries the mark and what parity it
    // must show, so neither is guessable from the netlist.
    const std::array<std::uint8_t, 32> d
        = hmac_digest(key_, {"pair", c.pair_key, na, nb});
    const int target_bit = d[0] & 1;
    const bool target_is_a = ((d[0] >> 1) & 1) != 0;

    dbInst* target = target_is_a ? a : b;
    dbInst* other = target_is_a ? b : a;

    CtsClaim claim;
    claim.pair_key = c.pair_key;
    claim.target_lcb = target->getName();
    claim.other_lcb = other->getName();
    claim.target_bit = target_bit;
    claim.final_bit = fanoutParity(target);

    if (claim.final_bit != target_bit) {
      setParity(claim, target, other, lcb_clocks_[c.i]);
    }
    if (claim.final_bit == target_bit) {
      ++held_;
    }
    claims_.push_back(claim);
    claimed_[target_is_a ? c.i : c.j] = true;
  }

  void setParity(CtsClaim& claim,
                 dbInst* target,
                 dbInst* other,
                 const ClockIdentities& clocks)
  {
    // Move one sink across the boundary to flip the parity.  Taking it from
    // the target lowers that count by one; taking it from the peer raises
    // it.  Either direction changes the parity, so use whichever buffer has
    // a sink free to move.
    dbNet* target_net = singleOutputNet(target);
    dbNet* other_net = singleOutputNet(other);
    dbITerm* sink = nullptr;
    dbNet* dest = nullptr;
    if (target_net != nullptr && other_net != nullptr) {
      sink = movableSink(other, target_net, network_);
      dest = target_net;
      if (sink == nullptr) {
        sink = movableSink(target, other_net, network_);
        dest = other_net;
      }
    }
    if (sink == nullptr) {
      ++rejected_no_sink_;
    } else if (!haveClockSkews(clocks, skew_before_)) {
      ++rejected_skew_;
      if (!warned_skew_) {
        watermark_.logger_->warn(
            utl::WMK,
            74,
            "Clock skew cannot be evaluated for a selected pair; "
            "its sinks will not be moved. Read liberty, constraints "
            "and clock wire RC, and propagate the clocks first.");
        warned_skew_ = true;
      }
    } else {
      moveSink(claim, target, other, sink, dest);
    }
  }

  void moveSink(CtsClaim& claim,
                dbInst* target,
                dbInst* other,
                dbITerm* sink,
                dbNet* dest)
  {
    dbNet* origin = sink->getNet();
    bool skew_ok = false;
    bool timing_ok = false;
    bool drive_ok = false;
    edits_.push_back({.sink = sink, .origin = origin, .destination = dest});
    const bool accepted = tryClockSinkMove(
        sink,
        dest,
        [&] { watermark_.reestimateNetParasitics(origin, dest); },
        [&] {
          skew_ok = clockSkewsWithin(skew_before_,
                                     clockSkews(watermark_.sta_),
                                     opts_.skew_margin_ns * 1e-9f);
          timing_ok = endpointSlacksWithin(
              watermark_.sta_, timing_before_, opts_.skew_margin_ns * 1e-9f);
          // Moving a sink changes both loads; use Liberty limits for both
          // drivers after refreshing the affected parasitics.
          drive_ok
              = watermark_.driverHeadroomOk(
                    target, opts_.slew_headroom_frac, opts_.cap_headroom_frac)
                && watermark_.driverHeadroomOk(
                    other, opts_.slew_headroom_frac, opts_.cap_headroom_frac);
          return skew_ok && timing_ok && drive_ok;
        });
    if (!accepted) {
      edits_.pop_back();
      if (!skew_ok) {
        ++rejected_skew_;
      } else if (!timing_ok) {
        ++rejected_timing_;
      } else {
        ++rejected_drive_;
      }
    } else {
      ++moved_;
      claim.final_bit = fanoutParity(target);
    }
  }

  void restore()
  {
    for (const SinkEdit& edit : std::views::reverse(edits_)) {
      if (edit.sink->getNet() != edit.origin) {
        edit.sink->connect(edit.origin);
      }
    }
    for (const SinkEdit& edit : edits_) {
      watermark_.reestimateNetParasitics(edit.origin, edit.destination);
    }
  }

  Watermark& watermark_;
  dbBlock* block_;
  const std::vector<dbInst*>& lcbs_;
  const std::array<std::uint8_t, 32>& key_;
  const CtsOptions& opts_;
  sta::dbNetwork* network_;
  const std::int64_t max_dist_;
  ClockSkews skew_before_;
  std::vector<EndpointSlack> timing_before_;
  std::vector<ClockIdentities> lcb_clocks_;
  std::vector<std::optional<ClockBranch>> branches_;
  std::vector<Centre> centres_;
  std::map<std::pair<std::int64_t, std::int64_t>, std::vector<size_t>> grid_;
  std::vector<Candidate> candidates_;
  std::vector<CtsClaim> claims_;
  // A buffer that has been claimed is frozen: its fanout is the evidence, so it
  // can be neither the target nor the source of a later move.  A buffer that
  // only lent a sink is still free, which is what the paper's rule amounts to.
  std::vector<bool> claimed_;
  std::vector<SinkEdit> edits_;
  bool warned_skew_ = false;
  int rejected_cross_clock_ = 0;
  int rejected_clock_logic_ = 0;
  int rejected_skew_ = 0;
  int rejected_no_sink_ = 0;
  int rejected_drive_ = 0;
  int rejected_timing_ = 0;
  int held_ = 0;
  int moved_ = 0;
};

int Watermark::ctsWatermark(const std::array<std::uint8_t, 32>& key,
                            const CtsOptions& opts,
                            const std::string& claims_file)
{
  dbBlock* block = db_->getChip() ? db_->getChip()->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::WMK, 70, "No block loaded; read a design first.");
    return 0;
  }

  validateOptions(opts, block->getDbUnitsPerMicron(), logger_);

  if (!hasLiberty()) {
    logger_->error(
        utl::WMK, 107, "Read Liberty before embedding a CTS watermark.");
  }
  // Flat reconnection cannot preserve module ports and nets. Until edits and
  // rollback support hierarchy, reject before changing connectivity or RC.
  if (db_->hasHierarchy()) {
    logger_->error(utl::WMK,
                   111,
                   "CTS watermark embedding requires a flat design. "
                   "Link the design without -hier before embedding.");
  }
  sta::dbNetwork* network = sta_->getDbNetwork();
  const std::vector<dbInst*> lcbs = findLeafClockBuffers(block, network);
  for (dbInst* lcb : lcbs) {
    checkClaimName(lcb);
  }
  ClaimFile output(claims_file);
  if (lcbs.size() < 2) {
    // A successful empty embedding must replace claims from an earlier run.
    output.publish([](std::ostream& out) { writeCtsClaims(out, {}); });
    logger_->warn(utl::WMK,
                  71,
                  "Found {} leaf clock buffer(s); at least two are needed to "
                  "move a sink between them.",
                  static_cast<int>(lcbs.size()));
    return 0;
  }

  return CtsEmbedding(*this, block, lcbs, key, opts).run(output);
}

}  // namespace wmk
