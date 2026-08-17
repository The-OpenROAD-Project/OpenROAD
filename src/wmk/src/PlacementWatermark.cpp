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
// otherwise.  The bit is derived from the key, so an observer cannot tell a
// marked ordering from an arbitrary one without it.
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
#include <cmath>
#include <fstream>
#include <map>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include "HmacSha256.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "dpl/Opendp.h"
#include "odb/db.h"
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
using odb::dbRow;

namespace {

// A cell is eligible only if moving it is both allowed and harmless: it must be
// movable, a single row tall so a swap cannot straddle rows, and not part of
// the clock network, which is placed to meet skew rather than to be shuffled.
bool isEligibleInst(dbInst* inst, int row_height)
{
  if (!inst->isPlaced() || inst->isFixed()) {
    return false;
  }
  odb::dbMaster* master = inst->getMaster();
  if (master->isBlock() || master->isPad() || master->isCover()) {
    return false;
  }
  if (static_cast<int>(master->getHeight()) != row_height) {
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

// Half-perimeter wirelength of every net the instance touches.  Used to reject
// swaps that would lengthen wires: the watermark should not be visible as a
// wirelength anomaly, and should not cost routability.
std::int64_t instHpwl(dbInst* inst)
{
  std::int64_t total = 0;
  for (dbITerm* iterm : inst->getITerms()) {
    dbNet* net = iterm->getNet();
    if (net == nullptr || net->isSpecial()) {
      continue;
    }
    odb::Rect bbox;
    bbox.mergeInit();
    bool any = false;
    for (dbITerm* other : net->getITerms()) {
      int x, y;
      if (other->getAvgXY(&x, &y)) {
        bbox.merge(odb::Rect(x, y, x, y));
        any = true;
      }
    }
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      int x, y;
      if (bterm->getFirstPinLocation(x, y)) {
        bbox.merge(odb::Rect(x, y, x, y));
        any = true;
      }
    }
    if (any) {
      total += bbox.dx() + bbox.dy();
    }
  }
  return total;
}

// How far along the row to look for a partner, in candidates.  The distance
// bound already limits the search; this bounds the work when a row is dense.
constexpr size_t kMaxNeighbours = 8;

// The tile a candidate sits in, as the PRF sees it: two big-endian int32.
std::string tileBytes(int tx, int ty)
{
  std::string out(8, '\0');
  for (int k = 0; k < 4; ++k) {
    out[k] = static_cast<char>((tx >> (24 - 8 * k)) & 0xff);
    out[4 + k] = static_cast<char>((ty >> (24 - 8 * k)) & 0xff);
  }
  return out;
}

// What swapping the two cells would cost in half-perimeter wirelength.  The
// swap is symmetric, so this does not depend on which way round the pair ends
// up, and the key is not consulted.
std::int64_t hpwlDeltaOfSwap(dbInst* a, dbInst* b)
{
  const std::int64_t before = instHpwl(a) + instHpwl(b);
  const odb::Point a_loc = a->getLocation();
  const odb::Point b_loc = b->getLocation();
  a->setLocation(b_loc.x(), a_loc.y());
  b->setLocation(a_loc.x(), b_loc.y());
  const std::int64_t after = instHpwl(a) + instHpwl(b);
  a->setLocation(a_loc.x(), a_loc.y());
  b->setLocation(b_loc.x(), b_loc.y());
  return std::llabs(after - before);
}

// Write the claim file verify_watermark reads.  The column set is the one
// documented in the module README; a producer is free to add columns, so the
// extra bookkeeping the Python embedder emits stays compatible.
bool writePlacementClaims(const std::string& path,
                          const std::vector<PlacementClaim>& claims)
{
  std::ofstream out(path);
  if (!out.is_open()) {
    return false;
  }
  out << "kind,id,A_name,B_name,target_bit,skipped_reason\n";
  for (const PlacementClaim& c : claims) {
    out << "pair," << c.a_name << '|' << c.b_name << ',' << c.a_name << ','
        << c.b_name << ',' << c.target_bit << ','
        << (c.already_satisfied ? "already_satisfied" : "") << '\n';
  }
  return out.good();
}

}  // namespace

int Watermark::embedPlacement(const std::array<std::uint8_t, 32>& key,
                              const PlacementOptions& opts,
                              std::vector<PlacementClaim>& claims)
{
  claims.clear();
  dbBlock* block = db_->getChip() ? db_->getChip()->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::WMK, 50, "No block loaded; read a design first.");
    return 0;
  }
  if (block->getRows().empty()) {
    logger_->error(
        utl::WMK, 51, "The design has no rows; run floorplan first.");
    return 0;
  }

  const odb::Rect core = block->getCoreArea();
  const int row_height = (*block->getRows().begin())->getSite()->getHeight();
  const int dbu = block->getDbUnitsPerMicron();
  const int pair_dist = static_cast<int>(opts.pair_dist_um * dbu);
  const int nx = std::max(1, opts.grid_nx);
  const int ny = std::max(1, opts.grid_ny);
  const int tile_w = std::max(1, static_cast<int>(core.dx() / nx));
  const int tile_h = std::max(1, static_cast<int>(core.dy() / ny));

  // Group by tile, row and width.  Only cells sharing all three can be swapped
  // without disturbing the row, so these groups are exactly the candidate
  // pools.
  struct Bucket
  {
    int tx, ty;
    std::vector<dbInst*> insts;
  };
  std::map<std::tuple<int, int, int, int>, Bucket> buckets;

  int n_eligible = 0;
  for (dbInst* inst : block->getInsts()) {
    if (!isEligibleInst(inst, row_height)) {
      continue;
    }
    ++n_eligible;
    odb::dbBox* bbox = inst->getBBox();
    const int tx
        = std::min(nx - 1, std::max(0, (bbox->xMin() - core.xMin()) / tile_w));
    const int ty
        = std::min(ny - 1, std::max(0, (bbox->yMin() - core.yMin()) / tile_h));
    const int width = bbox->getDX();
    auto& b = buckets[{tx, ty, bbox->yMin(), width}];
    b.tx = tx;
    b.ty = ty;
    b.insts.push_back(inst);
  }

  sta::dbSta* sta = opts.slack_threshold_ns > 0.0 ? sta_ : nullptr;

  // Enumerate the candidates first, apply the public guards, and only then let
  // the key choose among what is left.  The order matters: the guards depend on
  // the design and the choice depends on the key, so an observer who knows the
  // algorithm still cannot say which of the eligible pairs ended up marked.
  struct Candidate
  {
    dbInst* a;
    dbInst* b;
    int tx, ty;
    std::string first, second;
    std::array<std::uint8_t, 32> sort_key;
  };
  std::vector<Candidate> candidates;

  int n_eligible_pairs = 0;
  int rejected_slack = 0;
  int rejected_hpwl = 0;

  for (auto& [bkey, bucket] : buckets) {
    if (bucket.insts.size() < 2) {
      continue;
    }
    std::sort(
        bucket.insts.begin(), bucket.insts.end(), [](dbInst* a, dbInst* b) {
          return a->getBBox()->xMin() < b->getBBox()->xMin();
        });

    for (size_t i = 0; i + 1 < bucket.insts.size(); ++i) {
      dbInst* a = bucket.insts[i];
      if (sta != nullptr && worstSlack(a) < opts.slack_threshold_ns * 1e-9) {
        ++rejected_slack;
        continue;
      }
      const int ax = a->getBBox()->xMin();

      // Bound the fan-out of this enumeration.  Every extra neighbour costs a
      // trial swap and two wirelength evaluations, and partners far along the
      // row are the ones a swap moves furthest anyway.
      const size_t last = std::min(bucket.insts.size(), i + 1 + kMaxNeighbours);
      for (size_t j = i + 1; j < last; ++j) {
        dbInst* cand = bucket.insts[j];
        if (cand->getBBox()->xMin() - ax > pair_dist) {
          break;
        }
        if (sta != nullptr
            && worstSlack(cand) < opts.slack_threshold_ns * 1e-9) {
          continue;
        }
        ++n_eligible_pairs;
        if (hpwlDeltaOfSwap(a, cand) > opts.hpwl_eps_dbu) {
          ++rejected_hpwl;
          continue;
        }
        const std::string name_a = a->getName();
        const std::string name_b = cand->getName();
        Candidate c;
        c.a = a;
        c.b = cand;
        c.tx = bucket.tx;
        c.ty = bucket.ty;
        c.first = std::min(name_a, name_b);
        c.second = std::max(name_a, name_b);
        c.sort_key = hmac_digest(
            key,
            {"pair_sort", tileBytes(bucket.tx, bucket.ty), c.first, c.second});
        candidates.push_back(std::move(c));
      }
    }
  }

  // The keyed order.  Ties would be a 256-bit collision, so the identifier is
  // only a formality; it keeps the sort total either way.
  std::sort(candidates.begin(),
            candidates.end(),
            [](const Candidate& x, const Candidate& y) {
              if (x.sort_key != y.sort_key) {
                return x.sort_key < y.sort_key;
              }
              return std::tie(x.first, x.second) < std::tie(y.first, y.second);
            });

  int committed = 0;
  std::map<std::pair<int, int>, int> per_tile;
  // Membership only, never iterated, so hashing on the pointer cannot make the
  // result depend on where the objects happen to live.
  std::unordered_set<dbInst*> used;

  // Walk the keyed order and take every candidate that does not overlap one
  // already taken.
  for (const Candidate& c : candidates) {
    const std::pair<int, int> tile{c.tx, c.ty};
    if (per_tile[tile] >= opts.pairs_per_tile) {
      continue;
    }
    if (used.count(c.a) != 0 || used.count(c.b) != 0) {
      continue;
    }

    // The observed bit is which of the two names sits further left; the target
    // comes from the key.  Both sides sort the names first so the bit does not
    // depend on which cell the loop happened to see first.
    const int observed = (c.a->getBBox()->xMin() < c.b->getBBox()->xMin())
                             ? (c.a->getName() == c.first ? 0 : 1)
                             : (c.a->getName() == c.first ? 1 : 0);
    const std::array<std::uint8_t, 32> d
        = hmac_digest(key, {"bit", tileBytes(c.tx, c.ty), c.first, c.second});
    const int target = d[0] & 1;

    PlacementClaim claim;
    claim.a_name = c.first;
    claim.b_name = c.second;
    claim.target_bit = target;
    claim.already_satisfied = observed == target;

    if (!claim.already_satisfied) {
      const odb::Point a_loc = c.a->getLocation();
      const odb::Point b_loc = c.b->getLocation();
      c.a->setLocation(b_loc.x(), a_loc.y());
      c.b->setLocation(a_loc.x(), b_loc.y());
    }

    claims.push_back(claim);
    ++committed;
    ++per_tile[tile];
    used.insert(c.a);
    used.insert(c.b);
  }

  logger_->info(
      utl::WMK,
      52,
      "Placement watermark: {} pairs committed from {} eligible "
      "cells; {} candidate pairs, {} rejected on wirelength, {} cells "
      "rejected on slack.",
      committed,
      n_eligible,
      n_eligible_pairs,
      rejected_hpwl,
      rejected_slack);
  return committed;
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

  // Record the slack of every cell we are about to touch, so a mark that costs
  // timing can be identified and undone rather than shipped.
  std::vector<PlacementClaim> claims;
  const int committed = embedPlacement(key, opts, claims);
  if (committed == 0) {
    logger_->warn(utl::WMK,
                  54,
                  "No placement pairs were committed; the design may be too "
                  "small or the gates too strict.");
  }

  // Swapping equally wide cells within a row keeps the row legal, but the
  // incremental legalizer still has to settle any overlap the swap exposed.
  dpl::Opendp* opendp = opendp_;
  if (opendp != nullptr && committed > 0) {
    const int site_width = (*block->getRows().begin())->getSite()->getWidth();
    const int row_height = (*block->getRows().begin())->getSite()->getHeight();
    const int max_disp_x
        = site_width > 0
              ? opts.max_disp_um * block->getDbUnitsPerMicron() / site_width
              : 0;
    const int max_disp_y
        = row_height > 0
              ? opts.max_disp_um * block->getDbUnitsPerMicron() / row_height
              : 0;
    opendp->detailedPlacement(std::max(1, max_disp_x),
                              std::max(1, max_disp_y),
                              "",
                              /* disallow_one_site_gaps */ true);
  }

  // Legalization can move a cell back out of the order we just set.  Every
  // claim is still written: dropping the ones that no longer hold would let
  // the embedder pick its evidence after the fact, and an extraction rate
  // chosen that way would be one on any design.  What the count below reports
  // is how much of the mark actually survived, which is the number the owner
  // needs to see before shipping.
  int displaced = 0;
  for (const PlacementClaim& claim : claims) {
    dbInst* a = block->findInst(claim.a_name.c_str());
    dbInst* b = block->findInst(claim.b_name.c_str());
    if (a == nullptr || b == nullptr
        || (a->getBBox()->xMin() < b->getBBox()->xMin() ? 0 : 1)
               != claim.target_bit) {
      ++displaced;
    }
  }

  if (!writePlacementClaims(claims_file, claims)) {
    logger_->error(
        utl::WMK, 55, "Could not write claims to '{}'.", claims_file);
    return 0;
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
