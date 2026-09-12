// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Placement watermark.
//
// Marks pairs of cells that sit in the same row and have the same width, by
// putting them in a keyed left-to-right order.  Swapping two equally wide cells
// within a row leaves the row legal and the area unchanged, so the mark costs
// nothing structurally; what it can cost is timing, which is why candidates are
// screened on slack and wirelength before being committed and re-checked after.
//
// A pair carries one bit: 0 when the first name sorts left of the second, 1
// otherwise.  The bit is derived from the key and the two names, so an
// observer cannot tell a marked ordering from an arbitrary one without it, and
// a verifier holding the key can derive it again from the names alone.
//
// Which cells get paired is keyed too, and that is the part an observer cannot
// reconstruct.  Candidates are enumerated and screened by rules anyone can
// apply -- same row, same width, close enough, no timing or wirelength cost --
// and the key then orders what survives and takes a greedy non-overlapping
// prefix.  So the eligible set is public and the marked subset is not.
//
// The key orders candidates by their names, never by what they currently look
// like.  An embedder that preferred whichever partner already sat in the keyed
// order would report a perfect extraction rate on any design at all, including
// one it had never touched, and the rate would carry no evidence.  For the same
// reason every pair chosen is claimed, including the ones whose swap did not
// survive legalization: on a design this key did not mark, the rate then sits
// at one half, which is exactly what it should be.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ClaimFile.h"
#include "Marks.h"
#include "Options.h"
#include "Timing.h"
#include "dpl/Opendp.h"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "utl/Logger.h"
#include "wmk/Watermark.h"

namespace wmk {

using odb::dbBlock;
using odb::dbInst;
using odb::dbITerm;
using odb::dbNet;

namespace {

// A cell is eligible only if moving it is both allowed and harmless: it must be
// movable, a single row tall so a swap cannot straddle rows, and not part of
// the clock network, which is placed to meet skew rather than to be shuffled.
bool isEligibleInst(dbInst* inst, int row_height)
{
  if (!inst->isPlaced() || inst->isFixed() || inst->isDoNotTouch()) {
    return false;
  }
  odb::dbMaster* master = inst->getMaster();
  // Only ordinary standard cells carry a mark.  Fillers are inserted and
  // removed by the flow, taps, endcaps and antenna cells are placed by rules
  // rather than by the placer, and tie cells and feedthroughs are not what a
  // placement watermark should rest on: a claim on a cell the flow may
  // legitimately delete or re-create is evidence that evaporates.
  if (master->getType() != odb::dbMasterType::CORE) {
    return false;
  }
  if (std::cmp_not_equal(master->getHeight(), row_height)) {
    return false;
  }
  for (dbITerm* iterm : inst->getITerms()) {
    dbNet* net = iterm->getNet();
    if (net != nullptr && net->getSigType() == odb::dbSigType::CLOCK) {
      return false;
    }
  }
  return true;
}

// Half-perimeter wirelength of one net.  Used to reject swaps that would
// lengthen wires: the watermark should not be visible as a wirelength anomaly,
// and should not cost routability.
std::int64_t netHpwl(dbNet* net)
{
  odb::Rect bbox;
  bbox.mergeInit();
  for (dbITerm* iterm : net->getITerms()) {
    int x, y;
    if (iterm->getAvgXY(&x, &y)) {
      bbox.merge({x, y});
    }
  }
  for (odb::dbBTerm* bterm : net->getBTerms()) {
    int x, y;
    if (bterm->getFirstPinLocation(x, y)) {
      bbox.merge({x, y});
    }
  }
  // mergeInit leaves the rectangle inverted, so an untouched box is one that
  // no pin location could be read from.
  return bbox.isInverted() ? 0 : bbox.dx() + bbox.dy();
}

// Cache of net wirelength for one placement state.
//
// A net is shared by every instance on it, and a cell appears in many
// candidate pairs, so the same net would otherwise be re-measured once per
// pin per candidate.  The cache is only ever consulted while the design is
// untouched -- every swap is applied after enumeration finishes -- so a hit
// describes the placement it was taken from.
using HpwlCache = std::unordered_map<dbNet*, std::int64_t>;

// The nets a swap can change: every signal net on either cell, each once.  A
// net shared by the two cells would otherwise be counted twice, halving the
// wirelength budget for exactly the directly connected pairs that same-row
// neighbours tend to be.
std::vector<dbNet*> swapNets(dbInst* a, dbInst* b)
{
  std::vector<dbNet*> nets;
  for (dbInst* inst : {a, b}) {
    for (dbITerm* iterm : inst->getITerms()) {
      dbNet* net = iterm->getNet();
      if (net == nullptr || net->isSpecial()) {
        continue;
      }
      if (std::ranges::find(nets, net) == nets.end()) {
        nets.push_back(net);
      }
    }
  }
  return nets;
}

// What swapping the two cells would cost in half-perimeter wirelength.  The
// swap is symmetric, so this does not depend on which way round the pair ends
// up, and the key is not consulted.
std::int64_t hpwlDeltaOfSwap(dbInst* a, dbInst* b, HpwlCache& cache)
{
  const std::vector<dbNet*> nets = swapNets(a, b);
  // The design is untouched here, so the cache answers.  After the cells move
  // it cannot, and the second measurement is taken fresh.
  std::int64_t before = 0;
  for (dbNet* net : nets) {
    auto it = cache.find(net);
    if (it == cache.end()) {
      it = cache.emplace(net, netHpwl(net)).first;
    }
    before += it->second;
  }
  const odb::Point a_loc = a->getLocation();
  const odb::Point b_loc = b->getLocation();
  a->setLocation(b_loc.x(), a_loc.y());
  b->setLocation(a_loc.x(), b_loc.y());
  std::int64_t after = 0;
  for (dbNet* net : nets) {
    after += netHpwl(net);
  }
  a->setLocation(a_loc.x(), a_loc.y());
  b->setLocation(b_loc.x(), b_loc.y());
  return std::llabs(after - before);
}

// Detailed placement can move cells outside the selected pairs. Retain the
// complete placement so a failed final timing check can undo those changes too.
struct InstPlacement
{
  dbInst* inst;
  odb::Point location;
  odb::dbOrientType orient;
  odb::dbPlacementStatus status;
};

std::vector<InstPlacement> savePlacement(dbBlock* block)
{
  std::vector<InstPlacement> result;
  result.reserve(block->getInsts().size());
  for (dbInst* inst : block->getInsts()) {
    result.push_back({.inst = inst,
                      .location = inst->getLocation(),
                      .orient = inst->getOrient(),
                      .status = inst->getPlacementStatus()});
  }
  return result;
}

void restorePlacement(const std::vector<InstPlacement>& placement)
{
  for (const InstPlacement& saved : placement) {
    saved.inst->setOrient(saved.orient);
    saved.inst->setLocation(saved.location.x(), saved.location.y());
    saved.inst->setPlacementStatus(saved.status);
  }
}

void legalizePlacement(dpl::Opendp* opendp, dbBlock* block, int max_disp_um)
{
  if (opendp == nullptr) {
    return;
  }
  // The displacement bound is given to the legalizer in sites across and rows
  // down.  A swap leaves every row legal by construction, so this is a bound
  // on what the legalizer may do to everything else.
  odb::dbSite* site = (*block->getRows().begin())->getSite();
  const int max_disp = max_disp_um * block->getDbUnitsPerMicron();
  const int dx = site->getWidth() > 0 ? max_disp / site->getWidth() : 0;
  const int dy = site->getHeight() > 0 ? max_disp / site->getHeight() : 0;
  opendp->detailedPlacement(std::max(1, dx), std::max(1, dy), "");
}

// How far along the row to look for a partner, in candidates.  The distance
// bound already limits the search; this bounds the work when a row is dense.
constexpr size_t kMaxNeighbours = 8;

// How far to look on the second pass, when the first did not find enough to
// mark.  Wider search, and the caller doubles the wirelength budget too.
constexpr size_t kRelaxedNeighbours = 24;

// Write the claim file verify_watermark reads.  The column set is the one
// documented in the module README; a producer is free to add columns.
void writePlacementClaims(std::ostream& out,
                          const std::vector<PlacementClaim>& claims)
{
  out << "kind,id,A_name,B_name,target_bit,skipped_reason\n";
  for (const PlacementClaim& c : claims) {
    out << "pair," << c.a_name << '|' << c.b_name << ',' << c.a_name << ','
        << c.b_name << ',' << c.target_bit << ','
        << (c.already_satisfied ? "already_satisfied" : "") << '\n';
  }
}

}  // namespace

class Watermark::PlacementEmbedding
{
 public:
  PlacementEmbedding(Watermark& watermark,
                     dbBlock* block,
                     const std::array<std::uint8_t, 32>& key,
                     const PlacementOptions& opts)
      : watermark_(watermark),
        block_(block),
        key_(key),
        opts_(opts),
        pair_dist_(
            static_cast<int>(opts.pair_dist_um * block->getDbUnitsPerMicron())),
        hpwl_eps_(static_cast<std::int64_t>(opts.hpwl_eps_um
                                            * block->getDbUnitsPerMicron())),
        screen_slack_(opts.slack_threshold_ns > 0.0
                      && watermark.placementTimingAvailable())
  {
  }

  int run(std::vector<PlacementClaim>& claims,
          std::vector<PlacementEdit>& edits)
  {
    collectBuckets();
    selectFrom(enumerate(kMaxNeighbours, hpwl_eps_, /* counting */ true));
    // Both passes run against the untouched design, so widening the gates
    // cannot be confused by swaps the first pass already made.  Only the strict
    // pass counts rejections: the relaxed pass re-walks the same candidates,
    // and counting them twice would report more rejections than there were
    // pairs.
    const int strict_pairs = static_cast<int>(selected_.size());
    const bool relaxed = strict_pairs < opts_.min_pairs_total;
    // A design that yields only a handful of pairs cannot prove much, so look
    // again with the search widened and the wirelength budget doubled.  The
    // second pass only adds; nothing already chosen is revisited, and the
    // choice is still the key's.
    if (relaxed) {
      selectFrom(
          enumerate(kRelaxedNeighbours, hpwl_eps_ * 2, /* counting */ false));
    }
    apply(claims, edits);
    const int committed = static_cast<int>(selected_.size());
    if (relaxed) {
      watermark_.logger_->info(
          utl::WMK,
          57,
          "Placement watermark: {} pairs after the strict pass, below "
          "the {} wanted, so the search was widened and found {}.",
          strict_pairs,
          opts_.min_pairs_total,
          committed - strict_pairs);
    }

    watermark_.logger_->info(
        utl::WMK,
        52,
        "Placement watermark: {} pairs committed from {} eligible "
        "cells; {} candidate pairs, {} rejected on wirelength, {} cells "
        "rejected on slack.",
        committed,
        n_eligible_,
        n_eligible_pairs_,
        rejected_hpwl_,
        rejected_slack_);
    return committed;
  }

 private:
  struct Bucket
  {
    int tx, ty;
    std::vector<dbInst*> insts;
  };

  struct Candidate
  {
    dbInst* a;
    dbInst* b;
    int tx, ty;
    OrderedPair pair;
    std::array<std::uint8_t, 32> sort_key;
  };

  // Does the cell keep enough slack to be moved at all?  Applied once per
  // cell, before pairing, so the count is of cells and every cell is read
  // once.
  bool passesSlackScreen(dbInst* inst) const
  {
    const auto slack = watermark_.worstSlack(inst);
    return slack && *slack >= opts_.slack_threshold_ns * 1e-9;
  }

  void collectBuckets()
  {
    const odb::Rect core = block_->getCoreArea();
    const int row_height = (*block_->getRows().begin())->getSite()->getHeight();
    const int nx = std::max(1, opts_.grid_nx);
    const int ny = std::max(1, opts_.grid_ny);
    const int tile_w = std::max(1, static_cast<int>(core.dx() / nx));
    const int tile_h = std::max(1, static_cast<int>(core.dy() / ny));

    // Group by tile, row and width.  Only cells sharing all three can be
    // swapped without disturbing the row, so these groups are exactly the
    // candidate pools.
    for (dbInst* inst : block_->getInsts()) {
      if (!isEligibleInst(inst, row_height)) {
        continue;
      }
      // Finish name validation before enumeration tries any placement swaps.
      watermark_.checkClaimName(inst);
      ++n_eligible_;
      if (screen_slack_ && !passesSlackScreen(inst)) {
        ++rejected_slack_;
        continue;
      }
      odb::dbBox* bbox = inst->getBBox();
      const int tx = std::min(
          nx - 1, std::max(0, (bbox->xMin() - core.xMin()) / tile_w));
      const int ty = std::min(
          ny - 1, std::max(0, (bbox->yMin() - core.yMin()) / tile_h));
      const int width = bbox->getDX();
      auto& b = buckets_[{tx, ty, bbox->yMin(), width}];
      b.tx = tx;
      b.ty = ty;
      b.insts.push_back(inst);
    }

    for (auto& [bkey, bucket] : buckets_) {
      std::ranges::sort(bucket.insts, [](dbInst* a, dbInst* b) {
        return a->getBBox()->xMin() < b->getBBox()->xMin();
      });
    }
  }

  // Enumerate the candidates, apply the public guards, and only then let the
  // key choose among what is left.  The order matters: the guards depend on the
  // design and the choice depends on the key, so an observer who knows the
  // algorithm still cannot say which of the eligible pairs ended up marked.
  std::vector<Candidate> enumerate(size_t max_neighbours,
                                   std::int64_t hpwl_eps,
                                   bool counting)
  {
    std::vector<Candidate> out;
    for (auto& [bkey, bucket] : buckets_) {
      if (bucket.insts.size() < 2) {
        continue;
      }
      for (size_t i = 0; i + 1 < bucket.insts.size(); ++i) {
        dbInst* a = bucket.insts[i];
        const int ax = a->getBBox()->xMin();
        const size_t last
            = std::min(bucket.insts.size(), i + 1 + max_neighbours);
        for (size_t j = i + 1; j < last; ++j) {
          dbInst* cand = bucket.insts[j];
          if (cand->getBBox()->xMin() - ax > pair_dist_) {
            break;
          }
          if (counting) {
            ++n_eligible_pairs_;
          }
          if (hpwlDeltaOfSwap(a, cand, hpwl_cache_) > hpwl_eps) {
            if (counting) {
              ++rejected_hpwl_;
            }
            continue;
          }
          Candidate c;
          c.a = a;
          c.b = cand;
          c.tx = bucket.tx;
          c.ty = bucket.ty;
          c.pair = orderPair(a->getName(), cand->getName());
          c.sort_key = placementSortKey(key_, bucket.tx, bucket.ty, c.pair);
          out.push_back(std::move(c));
        }
      }
    }
    // The keyed order.  Ties would be a 256-bit collision, so the
    // identifier is only a formality; it keeps the sort total either way.
    std::ranges::sort(out, [](const Candidate& x, const Candidate& y) {
      if (x.sort_key != y.sort_key) {
        return x.sort_key < y.sort_key;
      }
      return std::tie(x.pair.first, x.pair.second)
             < std::tie(y.pair.first, y.pair.second);
    });
    return out;
  }

  // Walk the keyed order and take every candidate that does not overlap one
  // already taken.
  void selectFrom(const std::vector<Candidate>& pool)
  {
    for (const Candidate& c : pool) {
      const std::pair<int, int> tile{c.tx, c.ty};
      if (per_tile_[tile] >= opts_.pairs_per_tile) {
        continue;
      }
      if (used_.contains(c.a) || used_.contains(c.b)) {
        continue;
      }
      selected_.push_back(c);
      ++per_tile_[tile];
      used_.insert(c.a);
      used_.insert(c.b);
    }
  }

  void apply(std::vector<PlacementClaim>& claims,
             std::vector<PlacementEdit>& edits)
  {
    // Apply the marks only once the whole set is settled.  Selected pairs share
    // no cell, so the order the swaps happen in cannot matter, and each pair's
    // observed order is read before anything moves.
    for (const Candidate& c : selected_) {
      const bool a_is_left = c.a->getBBox()->xMin() < c.b->getBBox()->xMin();
      const bool a_is_first = c.a->getName() == c.pair.first;
      const int observed = (a_is_left == a_is_first) ? 0 : 1;
      const int target = placementTargetBit(key_, c.pair);

      PlacementClaim claim;
      claim.a_name = c.pair.first;
      claim.b_name = c.pair.second;
      claim.target_bit = target;
      claim.already_satisfied = observed == target;

      PlacementEdit edit;
      edit.a = c.a;
      edit.b = c.b;
      edit.a_loc = c.a->getLocation();
      edit.b_loc = c.b->getLocation();
      edit.a_slack = watermark_.worstSlack(c.a);
      edit.b_slack = watermark_.worstSlack(c.b);
      edit.moved = observed != target;
      edits.push_back(edit);

      claims.push_back(claim);
    }
    // Take every slack baseline before changing any placement.
    for (const PlacementEdit& edit : edits) {
      if (edit.moved) {
        edit.a->setLocation(edit.b_loc.x(), edit.a_loc.y());
        edit.b->setLocation(edit.a_loc.x(), edit.b_loc.y());
      }
    }
  }

  Watermark& watermark_;
  dbBlock* block_;
  const std::array<std::uint8_t, 32>& key_;
  const PlacementOptions& opts_;
  const int pair_dist_;
  // The bound is given in microns so that it means the same length on
  // every platform; database units do not.
  const std::int64_t hpwl_eps_;
  const bool screen_slack_;
  std::map<std::tuple<int, int, int, int>, Bucket> buckets_;
  // Shared by both passes: they both measure the same untouched placement.
  HpwlCache hpwl_cache_;
  std::map<std::pair<int, int>, int> per_tile_;
  // Membership only, never iterated, so hashing on the pointer cannot make the
  // result depend on where the objects happen to live.
  std::unordered_set<dbInst*> used_;
  std::vector<Candidate> selected_;
  int n_eligible_ = 0;
  int n_eligible_pairs_ = 0;
  int rejected_slack_ = 0;
  int rejected_hpwl_ = 0;
};

class Watermark::PlacementGuard
{
 public:
  PlacementGuard(Watermark& watermark,
                 dbBlock* block,
                 const PlacementOptions& opts)
      : watermark_(watermark),
        block_(block),
        opts_(opts),
        degrade_(static_cast<float>(opts.guard_degrade_ns * 1e-9))
  {
    const bool guard_requested
        = opts_.post_guard && opts_.guard_degrade_ns > 0.0;
    can_estimate_ = watermark_.canEstimateParasitics(/* clock */ false);
    if (guard_requested && can_estimate_) {
      watermark_.estimate_parasitics_->estimateParasitics(
          est::ParasiticsSrc::kPlacement);
    }
    can_measure_ = guard_requested && can_estimate_
                   && watermark_.placementTimingAvailable();
    if (guard_requested && !can_measure_) {
      watermark_.logger_->warn(
          utl::WMK,
          59,
          "Timing cannot be re-evaluated after the swaps, so the marks "
          "will be committed without checking what they cost. Read "
          "liberty, timing constraints and signal wire RC first.");
    }
    timing_before_ = can_measure_ ? endpointSlacks(watermark_.sta_)
                                  : std::vector<EndpointSlack>{};
    placement_before_ = savePlacement(block_);
  }

  void legalize()
  {
    legalizePlacement(watermark_.opendp_, block_, opts_.max_disp_um);
    refresh();
  }

  // Put the whole original placement back.  Returns false if its parasitics
  // could not be re-estimated afterwards, in which case the placement is
  // restored but the timing data is stale.
  bool restore() noexcept
  {
    restorePlacement(placement_before_);
    try {
      refresh();
    } catch (...) {
      return false;
    }
    return true;
  }

  // Undo the marked pairs that cost more than the budget, and report how many.
  // If the legalized design still cannot meet the budget, undo everything.
  int check(const std::vector<PlacementEdit>& edits)
  {
    if (!can_measure_) {
      return 0;
    }
    // Every timing measurement describes one complete, legalized placement.
    // Collect failed edits before restoring any of them, then refresh RC again.
    // A pair that was already in the keyed order was never moved, so there is
    // nothing of it to undo.
    int reverted = 0;
    std::vector<bool> rejected(edits.size());
    for (size_t i = 0; i < edits.size(); ++i) {
      rejected[i] = edits[i].moved && !pairTimingOk(edits[i]);
    }
    for (size_t i = 0; i < edits.size(); ++i) {
      if (rejected[i]) {
        const PlacementEdit& edit = edits[i];
        edit.a->setLocation(edit.a_loc.x(), edit.a_loc.y());
        edit.b->setLocation(edit.b_loc.x(), edit.b_loc.y());
        ++reverted;
      }
    }
    if (reverted > 0) {
      legalize();
    }
    bool final_ok
        = endpointSlacksWithin(watermark_.sta_, timing_before_, degrade_);
    for (size_t i = 0; i < edits.size(); ++i) {
      if (!rejected[i] && edits[i].moved && !pairTimingOk(edits[i])) {
        final_ok = false;
      }
    }
    if (!final_ok) {
      // Global legalization may have changed other cells. Restore the whole
      // original placement instead of returning a state that failed timing.
      if (!restore()) {
        watermark_.logger_->warn(
            utl::WMK,
            141,
            "The original placement was restored but its parasitics could "
            "not be re-estimated; run estimate_parasitics before trusting "
            "timing.");
      }
      restored_all_ = true;
      reverted = static_cast<int>(edits.size());
    }
    return reverted;
  }

  bool restoredAll() const { return restored_all_; }

 private:
  bool pairTimingOk(const PlacementEdit& edit) const
  {
    return cellTimingOk(edit.a, edit.a_slack)
           && cellTimingOk(edit.b, edit.b_slack);
  }

  // A cell that had no constrained slack before the swap has nothing to
  // compare against; the check of every endpoint still covers it.  One that
  // had a slack must still have one, and must not have lost more than the
  // budget.
  bool cellTimingOk(dbInst* inst, const std::optional<float>& before) const
  {
    if (!before) {
      return true;
    }
    const auto now = watermark_.worstSlack(inst);
    return now && *now >= *before - degrade_;
  }

  void refresh()
  {
    if (can_estimate_) {
      watermark_.estimate_parasitics_->estimateParasitics(
          est::ParasiticsSrc::kPlacement);
    }
  }

  Watermark& watermark_;
  dbBlock* block_;
  const PlacementOptions& opts_;
  const float degrade_;
  bool can_estimate_ = false;
  bool can_measure_ = false;
  bool restored_all_ = false;
  std::vector<EndpointSlack> timing_before_;
  std::vector<InstPlacement> placement_before_;
};

int Watermark::embedPlacement(const std::array<std::uint8_t, 32>& key,
                              const PlacementOptions& opts,
                              std::vector<PlacementClaim>& claims)
{
  std::vector<PlacementEdit> edits;
  return embedPlacementEdits(key, opts, claims, edits);
}

int Watermark::embedPlacementEdits(const std::array<std::uint8_t, 32>& key,
                                   const PlacementOptions& opts,
                                   std::vector<PlacementClaim>& claims,
                                   std::vector<PlacementEdit>& edits)
{
  dbBlock* block = db_->getChip() ? db_->getChip()->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::WMK, 50, "No block loaded; read a design first.");
    return 0;
  }
  validateOptions(opts, block->getDbUnitsPerMicron(), logger_);
  claims.clear();
  edits.clear();
  if (block->getRows().empty()) {
    logger_->error(
        utl::WMK, 51, "The design has no rows; run floorplan first.");
    return 0;
  }

  return PlacementEmbedding(*this, block, key, opts).run(claims, edits);
}

int Watermark::placementWatermark(const std::array<std::uint8_t, 32>& key,
                                  const PlacementOptions& opts,
                                  const std::string& claims_file)
{
  dbBlock* block = db_->getChip() ? db_->getChip()->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::WMK, 53, "No block loaded; read a design first.");
    return 0;
  }

  validateOptions(opts, block->getDbUnitsPerMicron(), logger_);

  ClaimFile output(claims_file);

  PlacementGuard guard(*this, block, opts);

  // Whatever went wrong, the design goes back to how it was.  If even that
  // cannot be completed, say so rather than let the original error stand
  // alone as if nothing else had happened.
  const auto abandon = [&](const char* what) {
    if (!guard.restore()) {
      logger_->error(utl::WMK,
                     126,
                     "Placement watermark failed ({}); the original placement "
                     "was restored but its parasitics could not be "
                     "re-estimated. Run estimate_parasitics before trusting "
                     "timing.",
                     what);
    }
  };

  std::vector<PlacementClaim> claims;
  std::vector<PlacementEdit> edits;
  int displaced = 0;
  try {
    const int committed = embedPlacementEdits(key, opts, claims, edits);
    if (committed == 0) {
      logger_->warn(utl::WMK,
                    54,
                    "No placement pairs were committed; the design may be too "
                    "small or the gates too strict.");
    }

    int reverted = 0;
    if (committed > 0) {
      guard.legalize();
      reverted = guard.check(edits);
    }
    if (guard.restoredAll()) {
      // The design carries none of the mark, so there is nothing to claim.  A
      // claim file for it would verify at chance and prove nothing, and
      // publishing one would read as a partial success.
      logger_->warn(utl::WMK,
                    110,
                    "Placement watermark: the legalized placement exceeds "
                    "the timing budget even with every marked pair put back; "
                    "restored the original placement and claimed nothing.");
      claims.clear();
    } else if (reverted > 0) {
      logger_->info(utl::WMK,
                    58,
                    "Placement watermark: {} pairs restored to keep timing "
                    "within {:.4g} ps of the original placement.",
                    reverted,
                    opts.guard_degrade_ns * 1000.0);
    }
    // Timing-rejected edits remain claims. Their survival can depend on whether
    // the keyed bit needed a swap, so removing them would bias extraction.

    // Legalization can move a cell back out of the order we just set.  Every
    // claim is still written: dropping the ones that no longer hold would let
    // the embedder pick its evidence after the fact, and an extraction rate
    // chosen that way would be one on any design.  What the count below reports
    // is how much of the mark actually survived, which is the number the owner
    // needs to see before shipping.
    for (const PlacementClaim& claim : claims) {
      dbInst* a = block->findInst(claim.a_name.c_str());
      dbInst* b = block->findInst(claim.b_name.c_str());
      if (a == nullptr || b == nullptr
          || (a->getBBox()->xMin() < b->getBBox()->xMin() ? 0 : 1)
                 != claim.target_bit) {
        ++displaced;
      }
    }

    output.publish(
        [&](std::ostream& out) { writePlacementClaims(out, claims); });
  } catch (const std::exception& error) {
    abandon(error.what());
    throw;
  } catch (...) {
    abandon("unknown error");
    throw;
  }

  logger_->info(utl::WMK,
                56,
                "Placement watermark: {} pairs written to {} ({} no longer "
                "hold after legalization).",
                static_cast<int>(claims.size()),
                claims_file,
                displaced);
  return static_cast<int>(claims.size());
}

}  // namespace wmk
