// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "ioPinBase.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "nesterovBase.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "placerBase.h"
#include "point.h"
#include "ppl/IOPlacer.h"
#include "utl/Logger.h"

namespace gpl {

using utl::GPL;

IoPinBase::IoPinBase(const std::vector<ppl::SlotPosition>& slots,
                     PlacerBase* pb,
                     NesterovBaseCommon* nbc,
                     utl::Logger* log)
    : pb_(pb), nbc_(nbc), log_(log)
{
  odb::dbBlock* block = pb_->db()->getChip()->getBlock();
  const int dbu_per_micron = block->getDbUnitsPerMicron();

  // Keep fixed ports as anchors, as in the sequential flow.
  int already_placed = 0;
  for (odb::dbBTerm* bterm : block->getBTerms()) {
    const odb::dbPlacementStatus status = bterm->getFirstPinPlacementStatus();
    if (status.isFixed() || bterm->getNet() == nullptr) {
      continue;
    }
    if (status.isPlaced()) {
      ++already_placed;
    }
    bterms_.push_back(bterm);
  }
  if (already_placed > 0) {
    log_->warn(GPL,
               184,
               "Concurrent IO placement moves {} already placed IO pins. Set "
               "them FIXED to keep their positions.",
               already_placed);
  }

  const size_t n = bterms_.size();
  pin_size_.reserve(n);
  for (odb::dbBTerm* bterm : bterms_) {
    const odb::Rect bbox = bterm->getBBox();
    pin_size_.emplace_back(
        bbox.isInverted() ? dbu_per_micron : std::max<int>(bbox.dx(), 1),
        bbox.isInverted() ? dbu_per_micron : std::max<int>(bbox.dy(), 1));
  }
  refreshGPins();

  master_to_follower_.assign(n, kNoMirrorPartner);
  follower_to_master_.assign(n, kNoMirrorPartner);
  last_written_pos_.assign(n, odb::Point(INT_MIN, INT_MIN));
  last_written_layer_.assign(n, nullptr);

  box_constraints_.assign(n, std::nullopt);
  int box_constrained = 0;
  for (size_t i = 0; i < n; ++i) {
    const std::optional<odb::Rect> cr = bterms_[i]->getConstraintRegion();
    if (cr.has_value() && cr->xMin() != cr->xMax()
        && cr->yMin() != cr->yMax()) {
      box_constraints_[i] = cr;
      ++box_constrained;
    }
  }
  if (box_constrained > 0) {
    pickTopLayerGrid();
  }

  initMirrorPairs();

  initSlots(slots);
  initConstraints();

  // The seeds are truncated to the DBU grid, as the GPins hold them.
  cur_.resize(n);
  for (size_t i = 0; i < n; ++i) {
    if (!isMirrorFollower(i)) {
      const FloatPoint p = seed(i);
      cur_[i] = FloatPoint(static_cast<int>(p.x), static_cast<int>(p.y));
    }
  }
  for (const auto& [master_io, follower_io] : mirror_pairs_) {
    const FloatPoint m = mirrorOf(master_io, cur_[master_io]);
    cur_[follower_io]
        = FloatPoint(static_cast<int>(m.x), static_cast<int>(m.y));
  }
  prev_ = cur_;
  next_ = cur_;
  snapshot_ = cur_;
  placeGPins(Iterate::kCur);

  log_->info(GPL,
             171,
             "Concurrent IO placement: {} movable IO pins seeded "
             "({} mirror pairs).",
             n,
             mirror_pairs_.size());
}

void IoPinBase::refreshGPins()
{
  gpins_.resize(bterms_.size());
  for (size_t i = 0; i < bterms_.size(); ++i) {
    gpins_[i] = nbc_->dbToNb(bterms_[i]);
  }
}

void IoPinBase::computeNext()
{
  const size_t n = bterms_.size();
#pragma omp parallel for num_threads(nbc_->getNumThreads())
  for (size_t i = 0; i < n; ++i) {
    // Followers are reflected below.
    next_[i] = isMirrorFollower(i) ? cur_[i] : optimum(i, cur_[i]);
  }
  if (log_->debugCheck(GPL, "placeIos", 1)) {
    optimum_ = next_;
  }

  if (!mirror_pairs_.empty()) {
    separateMirrored(next_);
    applyMirrorConstraints(next_);
  }
  separate(next_);
}

void IoPinBase::commitNext(const int iter)
{
  if (log_->debugCheck(GPL, "placeIos", 1) && optimum_.size() == cur_.size()) {
    reportMoves(iter);
  }
  std::swap(prev_, cur_);
  std::swap(cur_, next_);
}

void IoPinBase::reportMoves(const int iter) const
{
  odb::dbBlock* block = pb_->db()->getChip()->getBlock();
  auto um = [block](const double dbu) { return block->dbuToMicrons(dbu); };
  auto dist = [](const FloatPoint& a, const FloatPoint& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
  };
  int pins = 0;
  int moved = 0;
  double jump_sum = 0, jump_max = 0, spacing_sum = 0, spacing_max = 0;
  for (size_t i = 0; i < bterms_.size(); ++i) {
    // Followers move with their masters.
    if (isMirrorFollower(i)) {
      continue;
    }
    ++pins;
    moved += dist(cur_[i], next_[i]) >= 1;
    const double jump = dist(cur_[i], optimum_[i]);
    const double spacing = dist(optimum_[i], next_[i]);
    jump_sum += jump;
    jump_max = std::max(jump_max, jump);
    spacing_sum += spacing;
    spacing_max = std::max(spacing_max, spacing);
  }
  if (pins == 0) {
    return;
  }
  debugPrint(log_,
             GPL,
             "placeIos",
             1,
             "iter {}: {} of {} pins moved, jump to optimum mean {:.3f} max "
             "{:.3f} um, spacing mean {:.3f} max {:.3f} um",
             iter,
             moved,
             pins,
             um(jump_sum / pins),
             um(jump_max),
             um(spacing_sum / pins),
             um(spacing_max));
}

void IoPinBase::placeGPins(const Iterate iterate)
{
  const std::vector<FloatPoint>& at = iterate == Iterate::kPrev   ? prev_
                                      : iterate == Iterate::kNext ? next_
                                                                  : cur_;
  for (size_t i = 0; i < gpins_.size(); ++i) {
    if (gpins_[i] != nullptr) {
      gpins_[i]->setCenterLocation(static_cast<int>(at[i].x),
                                   static_cast<int>(at[i].y));
    }
  }
}

void IoPinBase::saveSnapshot()
{
  snapshot_ = cur_;
}

void IoPinBase::revertToSnapshot()
{
  cur_ = snapshot_;
}

// A 2D up: region names a position on the define_pin_shape_pattern grid, so
// the pin must be written on that grid's layer with that grid's pin size.
void IoPinBase::pickTopLayerGrid()
{
  odb::dbBlock* block = pb_->db()->getChip()->getBlock();
  const std::optional<odb::dbBlock::dbBTermTopLayerGrid> grid
      = block->getBTermTopLayerGrid();
  if (!grid.has_value() || grid->layer == nullptr) {
    log_->error(GPL,
                183,
                "Concurrent IO placement: IO pins have a top-layer (up:) "
                "constraint region but no pin placement grid exists. Call "
                "define_pin_shape_pattern first, or drop -place_ios.");
  }
  top_layer_ = grid->layer;
  top_pin_width_ = grid->pin_width;
  top_pin_height_ = grid->pin_height;
}

void IoPinBase::initMirrorPairs()
{
  std::unordered_map<odb::dbBTerm*, size_t> bterm_to_io_index;
  bterm_to_io_index.reserve(bterms_.size());
  for (size_t i = 0; i < bterms_.size(); ++i) {
    bterm_to_io_index[bterms_[i]] = i;
  }

  for (size_t i = 0; i < bterms_.size(); ++i) {
    odb::dbBTerm* bterm = bterms_[i];
    if (!bterm->hasMirroredBTerm()) {
      continue;
    }
    auto partner_it = bterm_to_io_index.find(bterm->getMirroredBTerm());
    if (partner_it == bterm_to_io_index.end()) {
      continue;
    }
    const size_t partner = partner_it->second;
    // Mirroring reflects across a die edge, which a box locus does not have.
    if (isBoxConstrained(i) || isBoxConstrained(partner)) {
      continue;
    }
    if (isMirrorMaster(i) || isMirrorFollower(i) || isMirrorMaster(partner)
        || isMirrorFollower(partner)) {
      continue;
    }
    master_to_follower_[i] = partner;
    follower_to_master_[partner] = i;
    mirror_pairs_.emplace_back(static_cast<uint32_t>(i),
                               static_cast<uint32_t>(partner));
  }
}

FloatPoint IoPinBase::seed(const size_t io_index) const
{
  // A position already in the db is a better seed than the net centroid.
  const odb::Rect bbox = bterms_[io_index]->getBBox();
  if (!bbox.isInverted()) {
    return project(io_index, bbox.xCenter(), bbox.yCenter());
  }

  const GPin* own = gpins_[io_index];
  const GNet* gnet = own != nullptr ? own->getGNet() : nullptr;

  int64_t sum_x = 0, sum_y = 0;
  int64_t cnt = 0;
  if (gnet != nullptr) {
    for (GPin* gpin : gnet->getGPins()) {
      const GCell* gc = gpin->getGCell();
      if (gc != nullptr && gc->isInstance()) {
        sum_x += gc->cx();
        sum_y += gc->cy();
        ++cnt;
      }
    }
  }
  const Die& die = pb_->getDie();
  const float cx = (cnt == 0) ? die.dieCx() : static_cast<float>(sum_x) / cnt;
  const float cy = (cnt == 0) ? die.dieCy() : static_cast<float>(sum_y) / cnt;

  return project(io_index, cx, cy);
}

bool IoPinBase::rectToPerimSegment(const odb::Rect& r, PerimSegment& seg) const
{
  const Die& die = pb_->getDie();
  const float lx = die.dieLx();
  const float ux = die.dieUx();
  const float ly = die.dieLy();
  const float uy = die.dieUy();

  // Pick the edge by proximity, not equality, so rounding cannot flip
  // left<->right or bottom<->top.
  if (r.xMin() == r.xMax()) {
    const float x = r.xMin();
    seg.edge = (std::abs(x - lx) <= std::abs(x - ux)) ? DieEdge::kLeft
                                                      : DieEdge::kRight;
    seg.lo = std::min<float>(r.yMin(), r.yMax());
    seg.hi = std::max<float>(r.yMin(), r.yMax());
    return true;
  }
  if (r.yMin() == r.yMax()) {
    const float y = r.yMin();
    seg.edge = (std::abs(y - ly) <= std::abs(y - uy)) ? DieEdge::kBottom
                                                      : DieEdge::kTop;
    seg.lo = std::min<float>(r.xMin(), r.xMax());
    seg.hi = std::max<float>(r.xMin(), r.xMax());
    return true;
  }
  return false;
}

std::vector<IoPinBase::PerimSegment> IoPinBase::mirrorSegments(
    const std::vector<PerimSegment>& segs)
{
  std::vector<PerimSegment> out;
  out.reserve(segs.size());
  for (const PerimSegment& s : segs) {
    switch (s.edge) {
      case DieEdge::kLeft:
        out.push_back({DieEdge::kRight, s.lo, s.hi});
        break;
      case DieEdge::kRight:
        out.push_back({DieEdge::kLeft, s.lo, s.hi});
        break;
      case DieEdge::kBottom:
        out.push_back({DieEdge::kTop, s.lo, s.hi});
        break;
      case DieEdge::kTop:
        out.push_back({DieEdge::kBottom, s.lo, s.hi});
        break;
    }
  }
  return out;
}

std::vector<IoPinBase::PerimSegment> IoPinBase::intersectSegments(
    const std::vector<PerimSegment>& a,
    const std::vector<PerimSegment>& b)
{
  std::vector<PerimSegment> out;
  for (const PerimSegment& sa : a) {
    for (const PerimSegment& sb : b) {
      if (sa.edge != sb.edge) {
        continue;
      }
      const float lo = std::max(sa.lo, sb.lo);
      const float hi = std::min(sa.hi, sb.hi);
      // <= keeps a zero-length overlap: a -region of one point is a legal
      // constraint, and two loci can touch at exactly one legal position.
      // projectOntoSegment() just clamps, so a point segment is harmless.
      if (lo <= hi) {
        out.push_back({sa.edge, lo, hi});
      }
    }
  }
  return out;
}

void IoPinBase::initConstraints()
{
  free_segments_.clear();
  constraint_segments_.assign(bterms_.size(), {});
  if (bterms_.empty()) {
    return;
  }

  odb::dbBlock* block = pb_->db()->getChip()->getBlock();
  const Die& die = pb_->getDie();

  const bool has_blocked = !block->getBlockedRegionsForPins().empty();
  std::vector<std::vector<std::pair<float, float>>> blocked(4);
  for (const odb::Rect& r : block->getBlockedRegionsForPins()) {
    PerimSegment seg;
    if (rectToPerimSegment(r, seg)) {
      blocked[static_cast<int>(seg.edge)].emplace_back(seg.lo, seg.hi);
    }
  }

  auto addFreeEdge = [&](DieEdge edge, float span_lo, float span_hi) {
    if (edge_slots_[static_cast<size_t>(edge)].empty()) {
      return;
    }
    auto& blk = blocked[static_cast<int>(edge)];
    std::sort(blk.begin(), blk.end());
    float cursor = span_lo;
    for (const auto& [b_lo, b_hi] : blk) {
      const float lo = std::max(span_lo, b_lo);
      const float hi = std::min(span_hi, b_hi);
      if (lo >= hi || hi <= cursor) {
        continue;
      }
      if (lo > cursor) {
        free_segments_.push_back({edge, cursor, lo});
      }
      cursor = std::max(cursor, hi);
    }
    if (cursor < span_hi) {
      free_segments_.push_back({edge, cursor, span_hi});
    }
  };
  addFreeEdge(DieEdge::kLeft, die.dieLy(), die.dieUy());
  addFreeEdge(DieEdge::kRight, die.dieLy(), die.dieUy());
  addFreeEdge(DieEdge::kBottom, die.dieLx(), die.dieUx());
  addFreeEdge(DieEdge::kTop, die.dieLx(), die.dieUx());

  bool any_perimeter_pin = false;
  for (size_t i = 0; i < bterms_.size(); ++i) {
    any_perimeter_pin |= !isBoxConstrained(i);
  }
  if (free_segments_.empty() && any_perimeter_pin) {
    log_->error(GPL,
                180,
                "Concurrent IO placement: the excluded IO pin regions cover "
                "the whole die perimeter, so no perimeter IO pin has a legal "
                "position.");
  }

  int constrained = 0;
  for (size_t i = 0; i < bterms_.size(); ++i) {
    // Box-constrained pins are clamped in project, not projected here.
    if (isBoxConstrained(i)) {
      continue;
    }
    const std::optional<odb::Rect> cr = bterms_[i]->getConstraintRegion();
    if (!cr.has_value()) {
      continue;
    }
    PerimSegment seg;
    if (rectToPerimSegment(cr.value(), seg)) {
      constraint_segments_[i] = intersectSegments({seg}, free_segments_);
      if (constraint_segments_[i].empty()) {
        log_->error(GPL,
                    175,
                    "Concurrent IO placement: the constraint region of pin {} "
                    "is entirely excluded from IO pin placement, so the pin "
                    "has no legal position.",
                    bterms_[i]->getConstName());
      }
      ++constrained;
    }
  }

  // ppl treats mirroring as a hard constraint, so restrict the master locus
  // to positions whose reflection is legal for the follower.
  for (const auto& [master_io, follower_io] : mirror_pairs_) {
    const std::vector<PerimSegment> both = intersectSegments(
        locus(master_io), mirrorSegments(locus(follower_io)));
    if (both.empty()) {
      log_->error(
          GPL,
          178,
          "Concurrent IO placement: mirrored pins {} and {} cannot both "
          "be placed legally - no position on {}'s locus reflects onto "
          "{}'s.",
          bterms_[master_io]->getConstName(),
          bterms_[follower_io]->getConstName(),
          bterms_[master_io]->getConstName(),
          bterms_[follower_io]->getConstName());
    }
    constraint_segments_[follower_io] = mirrorSegments(both);
    constraint_segments_[master_io] = both;
  }

  if (constrained > 0 || has_blocked) {
    log_->info(GPL,
               177,
               "Concurrent IO placement: {} pins region-constrained, {} free "
               "perimeter segments after blocked-region removal.",
               constrained,
               free_segments_.size());
  }
}

FloatPoint IoPinBase::projectOntoSegment(const PerimSegment& seg,
                                         float x,
                                         float y) const
{
  const Die& die = pb_->getDie();
  switch (seg.edge) {
    case DieEdge::kLeft:
      return FloatPoint(die.dieLx(), std::min(std::max(y, seg.lo), seg.hi));
    case DieEdge::kRight:
      return FloatPoint(die.dieUx(), std::min(std::max(y, seg.lo), seg.hi));
    case DieEdge::kBottom:
      return FloatPoint(std::min(std::max(x, seg.lo), seg.hi), die.dieLy());
    case DieEdge::kTop:
      return FloatPoint(std::min(std::max(x, seg.lo), seg.hi), die.dieUy());
  }
  return FloatPoint(x, y);
}

const IoPinBase::PerimSegment* IoPinBase::nearestSegment(
    const std::vector<PerimSegment>& segs,
    float x,
    float y,
    FloatPoint* projection) const
{
  const PerimSegment* best = nullptr;
  FloatPoint best_point;
  float best_dist = std::numeric_limits<float>::max();
  for (const PerimSegment& seg : segs) {
    const FloatPoint p = projectOntoSegment(seg, x, y);
    const float dx = p.x - x;
    const float dy = p.y - y;
    const float dist = dx * dx + dy * dy;
    if (dist < best_dist) {
      best_dist = dist;
      best = &seg;
      best_point = p;
    }
  }
  if (best != nullptr && projection != nullptr) {
    *projection = best_point;
  }
  return best;
}

const std::vector<IoPinBase::PerimSegment>& IoPinBase::locus(
    size_t io_index) const
{
  if (!constraint_segments_[io_index].empty()) {
    return constraint_segments_[io_index];
  }
  return free_segments_;
}

FloatPoint IoPinBase::project(size_t io_index, float x, float y) const
{
  if (isBoxConstrained(io_index)) {
    const odb::Rect& box = *box_constraints_[io_index];
    return FloatPoint(
        std::clamp(
            x, static_cast<float>(box.xMin()), static_cast<float>(box.xMax())),
        std::clamp(
            y, static_cast<float>(box.yMin()), static_cast<float>(box.yMax())));
  }
  FloatPoint projection;
  nearestSegment(locus(io_index), x, y, &projection);
  return projection;
}

// A follower moves with its master, so its nets count too, measured at the
// reflected position.
void IoPinBase::netBoxes(const size_t io_index,
                         std::vector<NetBox>& boxes) const
{
  boxes.clear();
  auto collect = [&](const GPin* self, const bool mirrored) {
    if (self == nullptr) {
      return;
    }
    const GNet* net = self->getGNet();
    if (net == nullptr || net->isDontCare()) {
      return;
    }
    NetBox b{std::numeric_limits<float>::max(),
             std::numeric_limits<float>::max(),
             std::numeric_limits<float>::lowest(),
             std::numeric_limits<float>::lowest(),
             net->getTotalWeight(),
             mirrored};
    for (const GPin* p : net->getGPins()) {
      if (p == self) {
        continue;
      }
      b.lx = std::min(b.lx, float(p->cx()));
      b.ly = std::min(b.ly, float(p->cy()));
      b.ux = std::max(b.ux, float(p->cx()));
      b.uy = std::max(b.uy, float(p->cy()));
    }
    if (b.lx <= b.ux) {
      boxes.push_back(b);
    }
  };
  collect(gpins_[io_index], false);
  const size_t follower = master_to_follower_[io_index];
  if (follower != kNoMirrorPartner) {
    collect(gpins_[follower], true);
  }
}

double IoPinBase::cost(const size_t io_index,
                       const std::vector<NetBox>& boxes,
                       const FloatPoint& q) const
{
  const FloatPoint m = isMirrorMaster(io_index) ? mirrorOf(io_index, q) : q;
  double c = 0;
  for (const NetBox& b : boxes) {
    const FloatPoint& p = b.mirrored ? m : q;
    c += b.w
         * (std::max(b.ux, p.x) - std::min(b.lx, p.x) + std::max(b.uy, p.y)
            - std::min(b.ly, p.y));
  }
  return c;
}

FloatPoint IoPinBase::optimum(size_t io_index, const FloatPoint& cur) const
{
  // Reused per thread: this runs for every pin inside the OpenMP loop.
  thread_local std::vector<NetBox> boxes;
  netBoxes(io_index, boxes);
  if (boxes.empty()) {
    return cur;
  }

  FloatPoint best = cur;
  double best_cost = std::numeric_limits<double>::max();
  double best_dist = std::numeric_limits<double>::max();
  auto consider = [&](const FloatPoint& q) {
    const double c = cost(io_index, boxes, q);
    const double d = std::abs(q.x - cur.x) + std::abs(q.y - cur.y);
    const double tol = 1e-6 * std::max(1.0, std::abs(best_cost));
    if (c < best_cost - tol || (c <= best_cost + tol && d < best_dist)) {
      best_cost = c;
      best_dist = d;
      best = q;
    }
  };

  constexpr float kFar = 1e30f;
  thread_local std::vector<float> xs;
  thread_local std::vector<float> ys;
  xs.assign({cur.x, -kFar, kFar});
  ys.assign({cur.y, -kFar, kFar});
  for (const NetBox& b : boxes) {
    xs.insert(xs.end(), {b.lx, b.ux});
    ys.insert(ys.end(), {b.ly, b.uy});
  }
  if (isBoxConstrained(io_index)) {
    for (const float x : xs) {
      for (const float y : ys) {
        consider(project(io_index, x, y));
      }
    }
    return best;
  }
  for (const PerimSegment& seg : locus(io_index)) {
    for (size_t i = 0; i < xs.size(); ++i) {
      consider(projectOntoSegment(seg, xs[i], ys[i]));
    }
  }
  return best;
}

IoPinBase::DieEdge IoPinBase::edgeOnLocus(size_t io_index,
                                          float x,
                                          float y) const
{
  return nearestSegment(locus(io_index), x, y, nullptr)->edge;
}

// Matches ppl Core::getMirroredPosition. The edge comes from the master's own
// locus, not nearest-edge distance: at a corner the two disagree.
FloatPoint IoPinBase::mirrorOf(size_t master_io, const FloatPoint& p) const
{
  const Die& die = pb_->getDie();
  FloatPoint r = p;
  switch (edgeOnLocus(master_io, p.x, p.y)) {
    case DieEdge::kLeft:
      r.x = die.dieUx();
      break;
    case DieEdge::kRight:
      r.x = die.dieLx();
      break;
    case DieEdge::kBottom:
      r.y = die.dieUy();
      break;
    case DieEdge::kTop:
      r.y = die.dieLy();
      break;
  }
  return r;
}

// Serial: masters must already be positioned.
void IoPinBase::applyMirrorConstraints(std::vector<FloatPoint>& coordi) const
{
  for (const auto& [master_io, follower_io] : mirror_pairs_) {
    coordi[follower_io] = mirrorOf(master_io, coordi[master_io]);
  }
}

float slotIndexAt(const std::vector<float>& slots, const float q)
{
  if (q <= slots.front()) {
    return 0;
  }
  if (q >= slots.back()) {
    return slots.size() - 1;
  }
  const size_t hi = std::ranges::upper_bound(slots, q) - slots.begin();
  const size_t lo = hi - 1;
  return lo + (q - slots[lo]) / (slots[hi] - slots[lo]);
}

float slotCoordinateAt(const std::vector<float>& slots, const float g)
{
  const float c = std::clamp(g, 0.0f, static_cast<float>(slots.size() - 1));
  const size_t i = static_cast<size_t>(c);
  if (i + 1 >= slots.size()) {
    return slots.back();
  }
  return slots[i] + (c - i) * (slots[i + 1] - slots[i]);
}

// Cut the ring open in the middle of the widest gap between pins, so pins
// close together never end up at the two ends of the line, and the pins on
// both sides of the gap can spread into it. Pins before the cut move one lap
// on. Returns the slot index the line starts at, which can be negative.
float unrollRingAtWidestGap(std::vector<std::pair<float, size_t>>& pins,
                            const int n)
{
  float widest = n - pins.back().first + pins.front().first;
  size_t cut = 0;
  for (size_t r = 1; r < pins.size(); ++r) {
    const float gap = pins[r].first - pins[r - 1].first;
    if (gap > widest) {
      widest = gap;
      cut = r;
    }
  }
  const float gap_from = cut == 0 ? pins.back().first - n : pins[cut - 1].first;
  const float at = pins[cut].first;
  std::ranges::rotate(pins, pins.begin() + cut);
  for (auto& [slot, io_i] : pins) {
    if (slot < at) {
      slot += n;
    }
  }
  return std::floor((gap_from + at) / 2);
}

// Replace z with the closest non-decreasing sequence within [lo, hi].
void poolAdjacentViolators(std::vector<float>& z,
                           const float lo,
                           const float hi)
{
  std::vector<std::pair<double, int>> blocks;
  blocks.reserve(z.size());
  for (const float v : z) {
    blocks.emplace_back(v, 1);
    while (blocks.size() > 1) {
      auto& last = blocks[blocks.size() - 1];
      auto& prev = blocks[blocks.size() - 2];
      if (prev.first <= last.first) {
        break;
      }
      const int count = prev.second + last.second;
      prev = {(prev.first * prev.second + last.first * last.second) / count,
              count};
      blocks.pop_back();
    }
  }
  size_t r = 0;
  for (const auto& [mean, count] : blocks) {
    const float fit = std::clamp(static_cast<float>(mean), lo, hi);
    for (int k = 0; k < count; ++k) {
      z[r++] = fit;
    }
  }
}

// Move the sorted pins as little as possible so they sit one slot apart. With
// more pins than slots, they share the slots evenly.
std::vector<float> fitSpacing(const std::vector<std::pair<float, size_t>>& pins,
                              const float lo,
                              const int total)
{
  const int n = pins.size();
  const bool fits = n <= total;
  const float step = fits ? 1.0f : (total - 1) / static_cast<float>(n - 1);
  std::vector<float> z(n);
  for (int r = 0; r < n; ++r) {
    z[r] = pins[r].first - r * step;
  }
  poolAdjacentViolators(z, lo, lo + (fits ? total - n : 0));
  for (int r = 0; r < n; ++r) {
    z[r] = (fits ? std::round(z[r]) : z[r]) + r * step;
  }
  return z;
}

void IoPinBase::initSlots(const std::vector<ppl::SlotPosition>& slots)
{
  odb::dbTech* tech = pb_->db()->getTech();
  std::array<std::vector<std::pair<float, odb::dbTechLayer*>>, 4> edge_slots;
  // Maps ppl::Edge, in its order top, bottom, left, right, to DieEdge.
  constexpr std::array<DieEdge, 4> kFromPpl
      = {DieEdge::kTop, DieEdge::kBottom, DieEdge::kLeft, DieEdge::kRight};
  for (const ppl::SlotPosition& slot : slots) {
    // Skip blocked slots, and top-layer and polygon slots, which are not on a
    // die edge.
    if (slot.blocked || slot.edge == ppl::Edge::invalid
        || slot.edge == ppl::Edge::polygonEdge) {
      continue;
    }
    const DieEdge edge = kFromPpl[static_cast<size_t>(slot.edge)];
    edge_slots[static_cast<size_t>(edge)].emplace_back(
        isHorizontalEdge(edge) ? slot.pos.x() : slot.pos.y(),
        tech->findRoutingLayer(slot.layer));
  }

  slot_ring_.clear();
  for (const DieEdge edge : kRingOrder) {
    const size_t e = static_cast<size_t>(edge);
    std::ranges::sort(edge_slots[e]);
    std::vector<float>& edge_slot_pos = edge_slots_[e];
    edge_slot_pos.clear();
    edge_slot_layers_[e].clear();
    for (const auto& [pos, layer] : edge_slots[e]) {
      edge_slot_pos.push_back(pos);
      edge_slot_layers_[e].push_back(layer);
    }
    ring_begin_[e] = slot_ring_.size();
    for (size_t k = 0; k < edge_slot_pos.size(); ++k) {
      slot_ring_.push_back(
          {edge,
           edge_slot_pos[ringAscends(edge) ? k
                                           : edge_slot_pos.size() - 1 - k]});
    }
  }

  // Slots free on both opposite edges, the only ones a mirrored pair can use.
  for (const bool horizontal : {false, true}) {
    const auto& a = edge_slots_[static_cast<size_t>(
        horizontal ? DieEdge::kBottom : DieEdge::kLeft)];
    const auto& b = edge_slots_[static_cast<size_t>(
        horizontal ? DieEdge::kTop : DieEdge::kRight)];
    axis_slots_[horizontal].clear();
    std::ranges::set_intersection(
        a, b, std::back_inserter(axis_slots_[horizontal]));
  }
}

odb::dbTechLayer* IoPinBase::pinLayer(const DieEdge edge,
                                      const float pos,
                                      const int nth) const
{
  const size_t e = static_cast<size_t>(edge);
  const std::vector<float>& slots = edge_slots_[e];
  const float slot = slots[std::lround(slotIndexAt(slots, pos))];
  const auto first = std::ranges::lower_bound(slots, slot) - slots.begin();
  const auto last = std::ranges::upper_bound(slots, slot) - slots.begin() - 1;
  return edge_slot_layers_[e][std::min<ptrdiff_t>(first + nth, last)];
}

float IoPinBase::ringIndexAt(const DieEdge edge, const float pos) const
{
  const size_t e = static_cast<size_t>(edge);
  const std::vector<float>& slots = edge_slots_[e];
  const float k = slotIndexAt(slots, pos);
  return ring_begin_[e] + (ringAscends(edge) ? k : slots.size() - 1 - k);
}

FloatPoint IoPinBase::ringPointAt(const std::vector<RingSlot>& ring,
                                  float r) const
{
  const int n = ring.size();
  // Indices from an unrolled ring can run up to one lap off either end.
  if (r < 0) {
    r += n;
  } else if (r >= n) {
    r -= n;
  }
  const float g = std::clamp(r, 0.0f, static_cast<float>(n - 1));
  int i = static_cast<int>(g);
  float pos = ring[i].pos;
  if (i + 1 < n) {
    if (ring[i + 1].edge == ring[i].edge) {
      pos += (g - i) * (ring[i + 1].pos - pos);
    } else if (g - i >= 0.5f) {
      // Between two edges: take the nearer corner slot.
      pos = ring[++i].pos;
    }
  }
  return projectOntoSegment({ring[i].edge, pos, pos}, pos, pos);
}

// Mirrored pairs are spaced by separateMirrored(), so only the others.
void IoPinBase::separate(std::vector<FloatPoint>& coordi) const
{
  // -1: not on the perimeter.
  std::vector<float> ring_index(bterms_.size(), -1);
  for (size_t i = 0; i < bterms_.size(); ++i) {
    if (isBoxConstrained(i)) {
      continue;
    }
    const FloatPoint& p = coordi[i];
    const DieEdge edge = edgeOnLocus(i, p.x, p.y);
    if (!edge_slots_[static_cast<size_t>(edge)].empty()) {
      ring_index[i] = ringIndexAt(edge, isHorizontalEdge(edge) ? p.x : p.y);
    }
  }

  std::vector<bool> taken(slot_ring_.size(), false);
  for (size_t i = 0; i < bterms_.size(); ++i) {
    if ((isMirrorMaster(i) || isMirrorFollower(i)) && ring_index[i] >= 0) {
      taken[std::lround(ring_index[i])] = true;
    }
  }
  std::vector<float> free_index;
  std::vector<RingSlot> free_ring;
  for (size_t k = 0; k < slot_ring_.size(); ++k) {
    if (!taken[k]) {
      free_index.push_back(k);
      free_ring.push_back(slot_ring_[k]);
    }
  }
  if (free_ring.empty()) {
    return;
  }

  std::vector<std::pair<float, size_t>> pins;
  for (size_t i = 0; i < bterms_.size(); ++i) {
    if (isMirrorMaster(i) || isMirrorFollower(i) || ring_index[i] < 0) {
      continue;
    }
    pins.emplace_back(slotIndexAt(free_index, ring_index[i]), i);
  }
  if (pins.empty()) {
    return;
  }
  const int total = free_ring.size();
  std::ranges::sort(pins);
  const float origin = unrollRingAtWidestGap(pins, total);
  const std::vector<float> fit = fitSpacing(pins, origin, total);
  for (size_t r = 0; r < pins.size(); ++r) {
    const size_t io_i = pins[r].second;
    const FloatPoint p = ringPointAt(free_ring, fit[r]);
    coordi[io_i] = project(io_i, p.x, p.y);
  }
}

void IoPinBase::separateMirrored(std::vector<FloatPoint>& coordi) const
{
  std::array<std::vector<std::pair<float, size_t>>, 2> axis_pins;
  for (size_t i = 0; i < bterms_.size(); ++i) {
    if (!isMirrorMaster(i)) {
      continue;
    }
    const FloatPoint& p = coordi[i];
    const bool horizontal = isHorizontalEdge(edgeOnLocus(i, p.x, p.y));
    const std::vector<float>& slots = axis_slots_[horizontal];
    if (!slots.empty()) {
      axis_pins[horizontal].emplace_back(
          slotIndexAt(slots, horizontal ? p.x : p.y), i);
    }
  }
  for (const bool horizontal : {false, true}) {
    std::vector<std::pair<float, size_t>>& pins = axis_pins[horizontal];
    if (pins.empty()) {
      continue;
    }
    const std::vector<float>& slots = axis_slots_[horizontal];
    std::ranges::sort(pins);
    const std::vector<float> fit = fitSpacing(pins, 0, slots.size());
    for (size_t r = 0; r < pins.size(); ++r) {
      const size_t io_i = pins[r].second;
      const float pos = slotCoordinateAt(slots, fit[r]);
      const FloatPoint& at = coordi[io_i];
      coordi[io_i]
          = project(io_i, horizontal ? pos : at.x, horizontal ? at.y : pos);
    }
  }
}

std::pair<int, int> IoPinBase::halfSize(const size_t io_index) const
{
  if (isBoxConstrained(io_index)) {
    return {top_pin_width_ / 2, top_pin_height_ / 2};
  }
  return {pin_size_[io_index].x() / 2, pin_size_[io_index].y() / 2};
}

std::optional<size_t> IoPinBase::findPin(const odb::dbBTerm* bterm) const
{
  const auto it = std::ranges::find(bterms_, bterm);
  if (it == bterms_.end()) {
    return std::nullopt;
  }
  return it - bterms_.begin();
}

odb::Rect IoPinBase::pinRect(const size_t io_index) const
{
  const int cx = static_cast<int>(std::lround(cur_[io_index].x));
  const int cy = static_cast<int>(std::lround(cur_[io_index].y));
  const auto [half_w, half_h] = halfSize(io_index);
  return {cx - half_w, cy - half_h, cx + half_w, cy + half_h};
}

FloatPoint IoPinBase::target(const size_t io_index) const
{
  if (isMirrorFollower(io_index)) {
    const size_t master = follower_to_master_[io_index];
    return mirrorOf(master, optimum(master, cur_[master]));
  }
  return optimum(io_index, cur_[io_index]);
}

void IoPinBase::report(const size_t io_index) const
{
  odb::dbBlock* block = pb_->db()->getChip()->getBlock();
  auto um = [block](const double dbu) { return block->dbuToMicrons(dbu); };
  auto edgeName = [](const DieEdge edge) {
    switch (edge) {
      case DieEdge::kLeft:
        return "left";
      case DieEdge::kRight:
        return "right";
      case DieEdge::kBottom:
        return "bottom";
      case DieEdge::kTop:
        return "top";
    }
    return "";
  };

  log_->report("Pin: {}", bterms_[io_index]->getName());
  if (isBoxConstrained(io_index)) {
    const odb::Rect& box = *box_constraints_[io_index];
    log_->report("  locus   box ({:.4f}, {:.4f}) - ({:.4f}, {:.4f}) um",
                 um(box.xMin()),
                 um(box.yMin()),
                 um(box.xMax()),
                 um(box.yMax()));
  } else {
    const std::vector<PerimSegment>& segs = locus(io_index);
    log_->report(
        "  locus   {} perimeter segments{}",
        segs.size(),
        constraint_segments_[io_index].empty() ? "" : " (constrained)");
    for (const PerimSegment& seg : segs) {
      log_->report("          {} {:.4f} - {:.4f} um",
                   edgeName(seg.edge),
                   um(seg.lo),
                   um(seg.hi));
    }
  }
  if (isMirrorMaster(io_index)) {
    log_->report("  mirror  master of {}",
                 bterms_[master_to_follower_[io_index]]->getName());
  }
  if (isMirrorFollower(io_index)) {
    log_->report("  mirror  follower of {}, moves with it",
                 bterms_[follower_to_master_[io_index]]->getName());
  }

  std::vector<NetBox> boxes;
  netBoxes(io_index, boxes);
  for (const NetBox& b : boxes) {
    log_->report(
        "  net     box ({:.4f}, {:.4f}) - ({:.4f}, {:.4f}) um  weight {}{}",
        um(b.lx),
        um(b.ly),
        um(b.ux),
        um(b.uy),
        b.w,
        b.mirrored ? "  (follower's, reflected)" : "");
  }

  auto reportAt = [&](const char* label, const FloatPoint& p) {
    log_->report("  {} ({:.4f}, {:.4f}) um  weighted HPWL {:.4f} um",
                 label,
                 um(p.x),
                 um(p.y),
                 um(cost(io_index, boxes, p)));
  };
  reportAt("prev pos", prev_[io_index]);
  reportAt("cur  pos", cur_[io_index]);
  reportAt("optimum ", target(io_index));
}

void IoPinBase::updateDb()
{
  std::map<std::pair<DieEdge, int>, int> pins_at;
  for (size_t i = 0; i < bterms_.size(); ++i) {
    odb::dbBTerm* bterm = bterms_[i];
    // Round, not truncate, so a pin a hair below its slot stays on the track.
    const FloatPoint& at = cur_[i];
    const int cx = static_cast<int>(std::lround(at.x));
    const int cy = static_cast<int>(std::lround(at.y));

    odb::dbTechLayer* layer = top_layer_;
    if (!isBoxConstrained(i)) {
      const DieEdge edge = edgeOnLocus(i, cx, cy);
      const bool horizontal = isHorizontalEdge(edge);
      // Multiple pins can share same slot coordinate with different layers.
      const int nth = pins_at[{edge, horizontal ? cx : cy}]++;
      layer = pinLayer(edge, horizontal ? at.x : at.y, nth);
    }
    auto [half_w, half_h] = halfSize(i);
    if (last_written_pos_[i] == odb::Point(cx, cy)
        && last_written_layer_[i] == layer) {
      continue;
    }
    const int min_half = static_cast<int>(layer->getWidth()) / 2;
    half_w = std::max(half_w, min_half);
    half_h = std::max(half_h, min_half);

    odb::dbSet<odb::dbBPin> bpins = bterm->getBPins();
    for (auto it = bpins.begin(); it != bpins.end();) {
      it = odb::dbBPin::destroy(it);
    }

    odb::dbBPin* bpin = odb::dbBPin::create(bterm);
    odb::dbBox::create(
        bpin, layer, cx - half_w, cy - half_h, cx + half_w, cy + half_h);
    bpin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
    last_written_pos_[i] = odb::Point(cx, cy);
    last_written_layer_[i] = layer;
  }
}

}  // namespace gpl
