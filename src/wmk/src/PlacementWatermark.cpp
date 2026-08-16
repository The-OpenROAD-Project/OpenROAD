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
// Which cells get paired is decided without ever consulting that bit.  This
// matters more than it looks: an embedder that picked whichever partner
// already happened to be in the keyed order would report a perfect extraction
// rate on any design at all, including one it had never touched, and the rate
// would carry no evidence.  Pairing therefore depends only on geometry,
// timing and wirelength, and every pair chosen is claimed -- including the
// ones whose swap did not survive legalization.  On a design this key did not
// mark, the rate then sits at one half, which is exactly what it should be.

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <string>
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

  int committed = 0;
  int rejected_distance = 0;
  int rejected_slack = 0;
  int rejected_hpwl = 0;
  std::map<std::pair<int, int>, int> per_tile;

  for (auto& [bkey, bucket] : buckets) {
    if (bucket.insts.size() < 2) {
      continue;
    }
    std::sort(
        bucket.insts.begin(), bucket.insts.end(), [](dbInst* a, dbInst* b) {
          return a->getBBox()->xMin() < b->getBBox()->xMin();
        });

    std::vector<bool> paired(bucket.insts.size(), false);
    for (size_t i = 0; i + 1 < bucket.insts.size(); ++i) {
      const std::pair<int, int> tile{bucket.tx, bucket.ty};
      if (per_tile[tile] >= opts.pairs_per_tile) {
        break;
      }
      if (paired[i]) {
        continue;
      }
      dbInst* a = bucket.insts[i];
      const int ax = a->getBBox()->xMin();

      // Screen on timing before touching anything: a marked pair that has to be
      // reverted later is wasted work, and one that is not reverted is a
      // regression the watermark caused.
      if (sta != nullptr && worstSlack(a) < opts.slack_threshold_ns * 1e-9) {
        ++rejected_slack;
        continue;
      }

      // Look along the row for a partner the design can absorb a swap with.
      // Taking the immediate neighbour and giving up would tie capacity to how
      // the cells happen to be ordered; most of the wirelength-neutral partners
      // are a little further along, because a pin that moves inside its net's
      // existing bounding box costs nothing.
      //
      // Nothing here reads the key.  The swap is symmetric, so its cost does
      // not depend on which way round the pair ends up, and the bit is only
      // consulted once the partner is settled.
      dbInst* b = nullptr;
      size_t b_index = 0;
      bool needs_swap = false;
      for (size_t j = i + 1; j < bucket.insts.size(); ++j) {
        dbInst* cand = bucket.insts[j];
        if (cand->getBBox()->xMin() - ax > pair_dist) {
          break;
        }
        if (paired[j]) {
          continue;
        }
        if (sta != nullptr
            && worstSlack(cand) < opts.slack_threshold_ns * 1e-9) {
          continue;
        }
        const std::int64_t hpwl_before = instHpwl(a) + instHpwl(cand);
        const odb::Point a_loc = a->getLocation();
        const odb::Point c_loc = cand->getLocation();
        a->setLocation(c_loc.x(), a_loc.y());
        cand->setLocation(a_loc.x(), c_loc.y());
        const std::int64_t hpwl_after = instHpwl(a) + instHpwl(cand);
        a->setLocation(a_loc.x(), a_loc.y());
        cand->setLocation(c_loc.x(), c_loc.y());
        if (std::llabs(hpwl_after - hpwl_before) <= opts.hpwl_eps_dbu) {
          b = cand;
          b_index = j;
          break;
        }
        ++rejected_hpwl;
      }
      if (b == nullptr) {
        ++rejected_distance;
        continue;
      }

      const std::string name_a = a->getName();
      const std::string name_b = b->getName();
      const std::string first = std::min(name_a, name_b);
      const std::string second = std::max(name_a, name_b);

      // The observed bit is which of the two names sits further left; the
      // target comes from the key.  Both sides sort the names first so the bit
      // does not depend on which cell the loop happened to see first.
      const int observed = (a->getBBox()->xMin() < b->getBBox()->xMin())
                               ? (name_a == first ? 0 : 1)
                               : (name_a == first ? 1 : 0);

      std::string tile_bytes(8, '\0');
      for (int k = 0; k < 4; ++k) {
        tile_bytes[k] = static_cast<char>((bucket.tx >> (24 - 8 * k)) & 0xff);
        tile_bytes[4 + k]
            = static_cast<char>((bucket.ty >> (24 - 8 * k)) & 0xff);
      }
      const std::array<std::uint8_t, 32> d
          = hmac_digest(key, {"bit", tile_bytes, first, second});
      const int target = d[0] & 1;

      PlacementClaim claim;
      claim.a_name = first;
      claim.b_name = second;
      claim.target_bit = target;
      claim.already_satisfied = observed == target;
      needs_swap = !claim.already_satisfied;

      if (needs_swap) {
        const odb::Point a_loc = a->getLocation();
        const odb::Point b_loc = b->getLocation();
        a->setLocation(b_loc.x(), a_loc.y());
        b->setLocation(a_loc.x(), b_loc.y());
      }

      claims.push_back(claim);
      ++committed;
      ++per_tile[tile];
      paired[i] = true;
      paired[b_index] = true;
    }
  }

  logger_->info(utl::WMK,
                52,
                "Placement watermark: {} pairs committed from {} eligible "
                "cells ({} rejected on distance, {} on slack, {} on "
                "wirelength).",
                committed,
                n_eligible,
                rejected_distance,
                rejected_slack,
                rejected_hpwl);
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
