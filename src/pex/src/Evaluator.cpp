// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "pex/Evaluator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "fsv/Geometry.h"
#include "fsv/Stack.h"
#include "odb/geom.h"
#include "pex/CoverMap.h"
#include "pex/Model.h"
#include "pex/Patterns.h"

namespace pex {

double NetEstimate::total() const
{
  double sum = ground;
  for (const auto& [net, c] : couplings) {
    sum += c;
  }
  return sum;
}

namespace {

constexpr double kEps = 1e-9;  // for z extents in µm

// A box's extent along axis k (0: x, 1: y), in database units.
int lo(const EvalBox& b, const int k)
{
  return k == 0 ? b.rect.xMin() : b.rect.yMin();
}

int hi(const EvalBox& b, const int k)
{
  return k == 0 ? b.rect.xMax() : b.rect.yMax();
}

// What a wire looks up besides its boxes: the evaluator's tables and
// reaches.
struct Context
{
  const fsv::Stack& stack;
  const Model& model;
  const EvaluatorOptions& options;
  const std::vector<fsv::Stack::Layer>& layers;
  const std::vector<std::string>& names;
  double dbu;  // per µm
  const std::vector<int>& lateral_reach;
  const std::vector<int>& crossing_reach;
  const std::vector<int>& facing_reach;
  const std::vector<std::vector<int>>& diagonal_reach;
};

// The lateral table for a context, falling back to fewer covers. The
// covers come resolved onto tabulated ones (CoverMap), so this only meets
// a pair the builder did not tabulate together.
const LateralKey& fallback(const Model& model, LateralKey& key)
{
  if (model.lateral(key) != nullptr) {
    return key;
  }
  LateralKey below_only{.layer = key.layer, .below = key.below, .above = ""};
  if (!key.above.empty() && model.lateral(below_only) != nullptr) {
    key = std::move(below_only);
    return key;
  }
  LateralKey above_only{.layer = key.layer, .below = "", .above = key.above};
  if (!key.below.empty() && model.lateral(above_only) != nullptr) {
    key = std::move(above_only);
    return key;
  }
  key = {.layer = key.layer, .below = "", .above = ""};
  return key;
}

// A cover's state on one side of a piece: the layer, its share (1 when a
// box covers the piece), and the nets that take the coupling, weighted by
// their part of it.
struct Cover
{
  std::string layer;
  double share = 0;            // of the total
  double cross = 0;            // of the plane's coupling
  std::map<int, double> nets;  // weight by box, summing to 1 when any
  // A box covering the piece: the span along the wire of the layer's runs
  // joined to it. Unbounded for crossings.
  int64_t reach0 = std::numeric_limits<int>::min();
  int64_t reach1 = std::numeric_limits<int>::max();
  // Behind crossings, a farther layer covering the piece: it, not open,
  // is what the rest of the piece sees.
  std::string rest;
  std::map<int, double> rest_nets;
  int64_t rest_reach0 = 0;
  int64_t rest_reach1 = 0;
};

// Along the wire and across it, in database units.
struct Run
{
  int box;
  int a0;  // over the wire; the whole box when past an end
  int a1;
  int e0;  // the whole box
  int e1;
  // How far the box runs past the wire's two sides.
  int past_lo;
  int past_hi;
};

struct Lateral
{
  int box;
  int gap;
  int a0;  // over the wire
  int a1;
  int e0;  // the whole box
  int e1;
};

// The share of a layer's crossings over a piece [x0, x1] that no run
// covers: from the nearest run before and after, through the one-sided
// shares integrated over the piece, each as much as the run reaches past
// the wire's sides keeps (Model::crossingReach).
// The crossing table is `key`'s, the cover of the tabulated layer
// standing in for `layer` when that one has none (CoverMap), at the
// piece's neighbor spacings `left` and `right` in µm (negative: none); the
// reach is `reach_key`'s. Positions are in database units, `dbu` per µm.
Cover partialCover(const Model& model,
                   const std::string& layer,
                   const CrossingKey& key,
                   const CrossingKey& reach_key,
                   const std::vector<Run>& runs,
                   const int x0,
                   const int x1,
                   const double left,
                   const double right,
                   const double dbu)
{
  Cover cover;
  cover.layer = layer;
  if (model.crossing(key) == nullptr) {
    return cover;
  }
  const Run* before = nullptr;
  const Run* after = nullptr;
  for (const Run& r : runs) {
    if (r.a1 <= x0 && (before == nullptr || r.a1 > before->a1)) {
      before = &r;
    }
    if (r.a0 >= x1 && (after == nullptr || r.a0 < after->a0)) {
      after = &r;
    }
  }
  const double len = (x1 - x0) / dbu;
  // The one-sided share integrated from the crossing's edge out to d.
  auto integral = [&](const int64_t d, const bool cross) {
    const CrossingShare k = model.crossingKept(key, d / dbu, left, right);
    return cross ? k.cross : k.total;
  };
  double total = 0;
  double cross = 0;
  auto reach = [&](const Run& r, const int64_t from, const int64_t to) {
    return model.crossingReach(reach_key,
                               from / dbu,
                               to / dbu,
                               r.past_lo / dbu,
                               r.past_hi / dbu,
                               left,
                               right);
  };
  if (before != nullptr) {
    const CrossingShare f = reach(*before, x0 - before->a1, x1 - before->a1);
    const double t = f.total
                     * (integral(x1 - before->a1, false)
                        - integral(x0 - before->a1, false));
    const double c
        = f.cross
          * (integral(x1 - before->a1, true) - integral(x0 - before->a1, true));
    total += t;
    cross += c;
    cover.nets[before->box] += c;
  }
  if (after != nullptr) {
    const CrossingShare f = reach(*after, after->a0 - x1, after->a0 - x0);
    const double t
        = f.total
          * (integral(after->a0 - x0, false) - integral(after->a0 - x1, false));
    const double c
        = f.cross
          * (integral(after->a0 - x0, true) - integral(after->a0 - x1, true));
    total += t;
    cross += c;
    cover.nets[after->box] += c;
  }
  cover.share = total / (2 * len);
  cover.cross = cross / (2 * len);
  double sum = 0;
  for (const auto& [net, wt] : cover.nets) {
    sum += wt;
  }
  if (sum > 0) {
    for (auto& [net, wt] : cover.nets) {
      wt /= sum;
    }
  }
  return cover;
}

// Two partial covers of one layer mixed, `b` by t: the shares, and the
// boxes by their part of the coupling.
Cover blend(const Cover& a, const Cover& b, const double t)
{
  Cover out = a;
  out.share = ((1 - t) * a.share) + (t * b.share);
  out.cross = ((1 - t) * a.cross) + (t * b.cross);
  out.nets.clear();
  for (const auto& [net, wt] : a.nets) {
    out.nets[net] += (1 - t) * a.cross * wt;
  }
  for (const auto& [net, wt] : b.nets) {
    out.nets[net] += t * b.cross * wt;
  }
  double sum = 0;
  for (const auto& [net, wt] : out.nets) {
    sum += wt;
  }
  for (auto& [net, wt] : out.nets) {
    wt = sum > 0 ? wt / sum : 0;
  }
  return out;
}

// One wire of the target, along axis `a`, with what lies around it: the
// boxes `near` (indices into `boxes`, ascending) within its layer's reach.
class Wire
{
 public:
  Wire(const int self,
       const std::vector<EvalBox>& boxes,
       const std::vector<int>& near,
       const Context& context,
       const CoverMap& map,
       std::vector<Contribution>& out,
       EvalCounts& counts)
      : self_(self),
        shape_(boxes[self]),
        net_(shape_.net),
        boxes_(boxes),
        near_(near),
        stack_(context.stack),
        layers_(context.layers),
        names_(context.names),
        model_(context.model),
        options_(context.options),
        map_(map),
        name_(context.names[shape_.layer]),
        dbu_(context.dbu),
        lateral_reach_(context.lateral_reach[shape_.layer]),
        crossing_reach_(context.crossing_reach[shape_.layer]),
        facing_reach_(context.facing_reach[shape_.layer]),
        diagonal_reach_(context.diagonal_reach[shape_.layer]),
        out_(out),
        counts_(counts),
        a_(shape_.rect.dx() >= shape_.rect.dy() ? 0 : 1),
        p_(1 - a_),
        a0_(lo(shape_, a_)),
        a1_(hi(shape_, a_)),
        p0_(lo(shape_, p_)),
        p1_(hi(shape_, p_)),
        w_(p1_ - p0_),
        w_um_(w_ / dbu_)
  {
    collect();
  }

  // Adds the wire's pieces and line ends.
  void estimate()
  {
    const std::vector<int> edges(cuts_.begin(), cuts_.end());
    for (size_t i = 0; i + 1 < edges.size(); i++) {
      if (insideOther(edges[i], edges[i + 1])) {
        continue;
      }
      counts_.pieces++;
      const auto [below, above] = coversOn(edges[i], edges[i + 1]);
      addPiece(edges[i], edges[i + 1], below, above);
    }
    // An end takes the covers at the end itself, crossings by their share
    // there rather than over the end piece.
    const int d = endSpan();
    const auto [start_below, start_above] = coversOn(a0_, a0_ + d);
    addEnd(a0_, start_below, start_above);
    const auto [end_below, end_above] = coversOn(a1_ - d, a1_);
    addEnd(a1_, end_below, end_above);
  }

  // The corners where another wire of the net meets this one over [x0, x1]
  // along it: `inner` inner and `outer` outer corners, with the covers
  // there. The parts to the planes go to the covering nets as a line
  // end's do.
  void addCorners(const int x0,
                  const int x1,
                  const int inner,
                  const int outer) const
  {
    at_ = (x0 + x1) / 2.0;
    const auto [below, above] = coversOn(x0, x1);
    double total = 0;
    double plane_below = 0;
    double plane_above = 0;
    double rest_below = 0;
    double rest_above = 0;
    forStates(
        below,
        above,
        [&](LateralKey& key, const double weight, const bool b, const bool t) {
          fallback(model_, key);
          if (model_.end(key) == nullptr) {
            key = {.layer = name_, .below = "", .above = ""};
          }
          const Corners c = model_.corners(key, w_um_);
          const double to_total
              = (inner * c.inner.total) + (outer * c.outer.total);
          const double to_below
              = (inner * c.inner.below) + (outer * c.outer.below);
          const double to_above
              = (inner * c.inner.above) + (outer * c.outer.above);
          total += weight * to_total;
          if (b) {
            plane_below += weight / below.share * to_below;
          } else {
            rest_below += weight * to_below;
          }
          if (t) {
            plane_above += weight / above.share * to_above;
          } else {
            rest_above += weight * to_above;
          }
        });
    const double to_below = below.cross * plane_below;
    const double to_above = above.cross * plane_above;
    ground(Ground::kEnd, total - to_below - to_above - rest_below - rest_above);
    for (const auto& [box, share] : below.rest_nets) {
      if (boxes_[box].net != net_) {
        couple(box, Source::kEndRest, rest_below * share);
      }
    }
    for (const auto& [box, share] : above.rest_nets) {
      if (boxes_[box].net != net_) {
        couple(box, Source::kEndRest, rest_above * share);
      }
    }
    for (const auto& [box, share] : below.nets) {
      if (boxes_[box].net != net_) {
        couple(box, Source::kEndCover, to_below * share);
      }
    }
    for (const auto& [box, share] : above.nets) {
      if (boxes_[box].net != net_) {
        couple(box, Source::kEndCover, to_above * share);
      }
    }
  }

 private:
  // The span at an end over which its covers and neighbors are taken: a
  // nanometer, within half the wire's length.
  int endSpan() const
  {
    const int nm = std::max(1, static_cast<int>(std::lround(dbu_ / 1000)));
    return std::max(1, std::min(nm, (a1_ - a0_) / 2));
  }

  // Lateral candidates with their gap, cover runs by layer, and the cuts
  // where any of them starts or ends.
  void collect()
  {
    cuts_ = {a0_, a1_};
    // On each other layer, the nearest box covering the wire's line past
    // each end: the end pieces and the ends take a share of it.
    // Boxes past the layer's reaches change no lookup and are skipped, so
    // that the result depends only on the boxes within radius().
    std::map<int, int> past_start;
    std::map<int, int> past_end;
    for (const int i : near_) {
      if (i == self_) {
        continue;
      }
      const EvalBox& s = boxes_[i];
      const int o0 = std::max(a0_, lo(s, a_));
      const int o1 = std::min(a1_, hi(s, a_));
      if (o1 <= o0) {
        const int overlap = std::min(p1_, hi(s, p_)) - std::max(p0_, lo(s, p_));
        if (s.layer >= 0 && s.layer != shape_.layer
            && overlap >= options_.cover_overlap * w_) {
          if (hi(s, a_) <= a0_ && a0_ - hi(s, a_) <= crossing_reach_) {
            auto [it, fresh] = past_start.try_emplace(s.layer, i);
            if (!fresh && hi(s, a_) > hi(boxes_[it->second], a_)) {
              it->second = i;
            }
          } else if (lo(s, a_) >= a1_ && lo(s, a_) - a1_ <= crossing_reach_) {
            auto [it, fresh] = past_end.try_emplace(s.layer, i);
            if (!fresh && lo(s, a_) < lo(boxes_[it->second], a_)) {
              it->second = i;
            }
          }
        }
        continue;
      }
      if (s.layer == shape_.layer) {
        if (hi(s, p_) <= p0_) {
          if (p0_ - hi(s, p_) > lateral_reach_) {
            continue;
          }
          left_.push_back({
              .box = i,
              .gap = p0_ - hi(s, p_),
              .a0 = o0,
              .a1 = o1,
              .e0 = lo(s, a_),
              .e1 = hi(s, a_),
          });
        } else if (lo(s, p_) >= p1_) {
          if (lo(s, p_) - p1_ > lateral_reach_) {
            continue;
          }
          right_.push_back({
              .box = i,
              .gap = lo(s, p_) - p1_,
              .a0 = o0,
              .a1 = o1,
              .e0 = lo(s, a_),
              .e1 = hi(s, a_),
          });
        } else if (s.net == shape_.net && lo(s, p_) <= p0_ && hi(s, p_) >= p1_
                   && dominates(i)) {
          // A longer box of the net across the wire's whole width: that
          // box counts the overlap.
          inside_.emplace_back(o0, o1);
        } else {
          continue;  // overlapping on the same layer: a junction
        }
      } else if (s.layer >= 0) {
        const int overlap = std::min(p1_, hi(s, p_)) - std::max(p0_, lo(s, p_));
        if (overlap < options_.cover_overlap * w_) {
          // Beside the wire: a diagonal candidate on a nearby layer, within
          // its table's gaps.
          if (std::abs(s.layer - shape_.layer) > options_.diagonal_depth
              || std::max(lo(s, p_) - p1_, p0_ - hi(s, p_))
                     > diagonal_reach_[s.layer]) {
            continue;
          }
        } else {
          runs_[s.layer].push_back({
              .box = i,
              .a0 = o0,
              .a1 = o1,
              .e0 = lo(s, a_),
              .e1 = hi(s, a_),
              .past_lo = p0_ - lo(s, p_),
              .past_hi = hi(s, p_) - p1_,
          });
        }
      } else {
        continue;
      }
      cuts_.insert(o0);
      cuts_.insert(o1);
    }
    for (const auto* past : {&past_start, &past_end}) {
      for (const auto& [layer, i] : *past) {
        const EvalBox& s = boxes_[i];
        runs_[layer].push_back({
            .box = i,
            .a0 = lo(s, a_),
            .a1 = hi(s, a_),
            .e0 = lo(s, a_),
            .e1 = hi(s, a_),
            .past_lo = p0_ - lo(s, p_),
            .past_hi = hi(s, p_) - p1_,
        });
      }
    }
  }

  // Whether a box of the net counts an overlap with this wire rather than
  // this wire: the longer one, or the first of two as long.
  bool dominates(const int i) const
  {
    const int ls = boxes_[i].rect.maxDXDY();
    const int lw = shape_.rect.maxDXDY();
    return ls > lw || (ls == lw && i < self_);
  }

  bool insideOther(const int x0, const int x1) const
  {
    for (const auto& [o0, o1] : inside_) {
      if (o0 <= x0 && o1 >= x1) {
        return true;
      }
    }
    return false;
  }

  // The nearest neighbor on a side over a piece.
  static const Lateral* nearest(const std::vector<Lateral>& side,
                                const int x0,
                                const int x1)
  {
    const Lateral* best = nullptr;
    for (const Lateral& l : side) {
      if (l.a0 <= x0 && l.a1 >= x1 && (best == nullptr || l.gap < best->gap)) {
        best = &l;
      }
    }
    return best;
  }

  bool coveredOn(const int layer, const int x0, const int x1) const
  {
    const auto it = runs_.find(layer);
    if (it == runs_.end()) {
      return false;
    }
    for (const Run& r : it->second) {
      if (r.a0 <= x0 && r.a1 >= x1) {
        return true;
      }
    }
    return false;
  }

  // The cover on a side: the nearest layer with a run over the piece, or
  // else the layer whose crossings keep the largest share.
  // The covers below and above a span, each side's crossings sharing as
  // they do with the other side's cover.
  std::pair<Cover, Cover> coversOn(const int x0, const int x1) const
  {
    const Cover below = coverOn(true, x0, x1);
    const Cover above = coverOn(false, x0, x1);
    return {coverOn(true, x0, x1, &above), coverOn(false, x0, x1, &below)};
  }

  // A side's crossings keep less where a cover on the other side pulls
  // the field to it: with `other`, the crossing table with a plane on that
  // cover's layer blends in by its share.
  Cover coverOn(const bool below,
                const int x0,
                const int x1,
                const Cover* other = nullptr) const
  {
    // The neighbors over the span, which hold the crossings' reach.
    const Lateral* l = nearest(left_, x0, x1);
    const Lateral* r = nearest(right_, x0, x1);
    const double sl = l == nullptr ? -1 : l->gap / dbu_;
    const double sr = r == nullptr ? -1 : r->gap / dbu_;
    Cover best;
    for (int l = below ? shape_.layer - 1 : shape_.layer + 1;
         l >= 0 && std::cmp_less(l, names_.size());
         l += below ? -1 : 1) {
      const auto it = runs_.find(l);
      if (it == runs_.end()) {
        continue;
      }
      for (const Run& r : it->second) {
        if (r.a0 <= x0 && r.a1 >= x1) {
          Cover full;
          full.layer = names_[l];
          full.share = full.cross = 1;
          full.nets[r.box] = 1;
          full.reach0 = r.e0;
          full.reach1 = r.e1;
          for (bool grew = true; grew;) {
            grew = false;
            for (const Run& q : it->second) {
              if (q.e0 <= full.reach1 && q.e1 >= full.reach0
                  && (q.e0 < full.reach0 || q.e1 > full.reach1)) {
                full.reach0 = std::min<int64_t>(full.reach0, q.e0);
                full.reach1 = std::max<int64_t>(full.reach1, q.e1);
                grew = true;
              }
            }
          }
          // A box crossing the wire keeps as much of a plane's effect as
          // it runs past the wire's sides.
          const CrossingKey key{
              .layer = name_,
              .cover = map_.crossingCover(layers_[l]),
              .opposite = "",
          };
          if (full.reach1 - full.reach0 <= int64_t{r.past_lo} + r.past_hi + w_
              && model_.crossing(key) != nullptr) {
            const CrossingShare f = model_.crossingReach(
                key, 0, 0, r.past_lo / dbu_, r.past_hi / dbu_, sl, sr);
            full.share = std::clamp(f.total, 0.0, 1.0);
            full.cross = std::clamp(f.cross, 0.0, 1.0);
          }
          if (best.share > 0) {
            best.rest = full.layer;
            best.rest_nets = full.nets;
            best.rest_reach0 = full.reach0;
            best.rest_reach1 = full.reach1;
            return best;
          }
          return full;
        }
      }
      const CrossingKey plain{
          .layer = name_,
          .cover = map_.crossingCover(layers_[l]),
          .opposite = "",
      };
      Cover partial = partialCover(
          model_, names_[l], plain, plain, it->second, x0, x1, sl, sr, dbu_);
      if (other != nullptr && other->share > 0 && !other->layer.empty()) {
        const CrossingKey planed{
            .layer = plain.layer,
            .cover = plain.cover,
            .opposite = other->layer,
        };
        if (model_.crossing(planed) != nullptr) {
          // The plain table's reach applies; see ModelBuilder.h.
          const Cover with = partialCover(model_,
                                          names_[l],
                                          planed,
                                          plain,
                                          it->second,
                                          x0,
                                          x1,
                                          sl,
                                          sr,
                                          dbu_);
          partial = blend(partial, with, std::clamp(other->share, 0.0, 1.0));
        }
      }
      if (partial.share > best.share) {
        best = partial;
      }
    }
    return best;
  }

  // The weighted cover states of a piece: (rest, 1 - share) and (layer,
  // share) on each side, where rest is open unless a farther layer covers
  // the piece behind crossings; the layers resolved onto the model's
  // tabulated covers. f gets the state's key, its weight, and whether each
  // side is covered by the nearer layer in it.
  template <class F>
  void forStates(const Cover& below, const Cover& above, F&& f) const
  {
    const std::vector<CoverMap::Part> none{{.key = "", .weight = 1}};
    auto parts = [&](const std::string& layer) {
      return layer.empty() ? none : map_.resolve(stack_.layer(layer));
    };
    const std::vector<CoverMap::Part> covered_below
        = below.share > 0 ? parts(below.layer) : none;
    const std::vector<CoverMap::Part> covered_above
        = above.share > 0 ? parts(above.layer) : none;
    const std::vector<CoverMap::Part> rest_below = parts(below.rest);
    const std::vector<CoverMap::Part> rest_above = parts(above.rest);
    for (const bool b : {false, true}) {
      const double wb = b ? below.share : 1 - below.share;
      if (wb == 0) {
        continue;
      }
      const std::vector<CoverMap::Part>& parts_below
          = b ? covered_below : rest_below;
      for (const bool t : {false, true}) {
        const double wt = t ? above.share : 1 - above.share;
        if (wt == 0) {
          continue;
        }
        const std::vector<CoverMap::Part>& parts_above
            = t ? covered_above : rest_above;
        for (const CoverMap::Part& pb : parts_below) {
          for (const CoverMap::Part& pt : parts_above) {
            LateralKey key{.layer = name_, .below = pb.key, .above = pt.key};
            f(key, wb * pb.weight * wt * pt.weight, b, t);
          }
        }
      }
    }
  }

  // The lateral blend over the cover states, the covers' couplings, and
  // the parallel wires on nearby layers beside the piece.
  void addPiece(const int x0,
                const int x1,
                const Cover& below,
                const Cover& above) const
  {
    at_ = (x0 + x1) / 2.0;
    const double len = (x1 - x0) / dbu_;
    const Lateral* l = nearest(left_, x0, x1);
    const Lateral* r = nearest(right_, x0, x1);
    const double sl = l == nullptr ? -1 : l->gap / dbu_;
    const double sr = r == nullptr ? -1 : r->gap / dbu_;
    double total = 0;
    double cl = 0;  // the lateral couplings; diagonal wires adjust them
    double cr = 0;
    double plane_below = 0;  // the plane's coupling in the covered state
    double plane_above = 0;
    double rest_below = 0;  // to the farther cover in the other state
    double rest_above = 0;
    double ground_below_only = 0;
    double ground_above_only = 0;
    // The plane couplings are per covered state, so they divide out the
    // share of that side.
    forStates(
        below,
        above,
        [&](LateralKey& key, const double weight, const bool b, const bool t) {
          {
            // The piece's ground with one side's cover only.
            LateralKey only_below{
                .layer = key.layer,
                .below = key.below,
                .above = "",
            };
            LateralKey only_above{
                .layer = key.layer,
                .below = "",
                .above = key.above,
            };
            ground_below_only
                += weight
                   * model_.lateral(fallback(model_, only_below), w_um_, sl, sr)
                         .ground;
            ground_above_only
                += weight
                   * model_.lateral(fallback(model_, only_above), w_um_, sl, sr)
                         .ground;
          }
          const LateralCap cap
              = model_.lateral(fallback(model_, key), w_um_, sl, sr);
          total += weight * cap.total();
          cl += weight * cap.left;
          cr += weight * cap.right;
          if (!key.below.empty()) {
            if (b) {
              plane_below += weight / below.share * cap.below;
            } else {
              rest_below += weight * cap.below;
            }
          }
          if (!key.above.empty()) {
            if (t) {
              plane_above += weight / above.share * cap.above;
            } else {
              rest_above += weight * cap.above;
            }
          }
        });
    double to_below = below.cross * plane_below;
    double to_above = above.cross * plane_above;
    const double piece_ground
        = len
          * (total - cl - cr - to_below - to_above - rest_below - rest_above);
    ground(Ground::kPiece, piece_ground);
    for (const auto& [box, share] : below.rest_nets) {
      if (boxes_[box].net != net_) {
        couple(box, Source::kRest, len * rest_below * share);
      }
    }
    for (const auto& [box, share] : above.rest_nets) {
      if (boxes_[box].net != net_) {
        couple(box, Source::kRest, len * rest_above * share);
      }
    }
    addDiagonals(x0,
                 x1,
                 sl,
                 sr,
                 below,
                 above,
                 ground_below_only,
                 ground_above_only,
                 cl,
                 cr,
                 to_below,
                 to_above);
    if (l != nullptr && boxes_[l->box].net != net_) {
      couple(l->box, Source::kLateral, len * cl);
    }
    if (r != nullptr && boxes_[r->box].net != net_) {
      couple(r->box, Source::kLateral, len * cr);
    }
    for (const auto& [box, share] : below.nets) {
      if (boxes_[box].net != net_) {
        couple(box, Source::kCover, len * to_below * share);
      }
    }
    for (const auto& [box, share] : above.nets) {
      if (boxes_[box].net != net_) {
        couple(box, Source::kCover, len * to_above * share);
      }
    }
  }

  // The nearest parallel wire on each nearby layer beside the piece, on
  // each side, where no run of that layer covers it, shielded by the
  // piece's neighbor on that side and on the other, and by covers on the
  // layers between. One of the net's own shields like any other but adds
  // no coupling. A cover on the piece's other side from the parallel wire
  // takes part of its coupling: the table with a plane on the nearest
  // cover there blends in by that cover's share, and what the parallel wire
  // takes from that plane comes off the coupling to the cover (`to_below`
  // or `to_above`, per unit length) as much as the cover keeps of the
  // plane's coupling.
  void addDiagonals(const int x0,
                    const int x1,
                    const double sl,
                    const double sr,
                    const Cover& below,
                    const Cover& above,
                    const double ground_below_only,
                    const double ground_above_only,
                    double& cl,
                    double& cr,
                    double& to_below,
                    double& to_above) const
  {
    const double len = (x1 - x0) / dbu_;
    // The diagonal tables have no covers on the parallel wire's side. A
    // parallel wire below takes
    // ground the piece's cover below would otherwise take, and one above
    // ground the cover above would: its part taken from ground scales by
    // the piece's ground with that side's cover alone over the ground with
    // no cover. Its coupling stays, since the wire beside is nearer than
    // the cover.
    LateralKey open_key{.layer = name_, .below = "", .above = ""};
    const double open_ground
        = model_.lateral(fallback(model_, open_key), w_um_, sl, sr).ground;
    auto ratio = [&](const double g) {
      return open_ground > 0 ? std::clamp(g / open_ground, 0.0, 1.0) : 1.0;
    };
    const double covered_below = ratio(ground_below_only);
    const double covered_above = ratio(ground_above_only);
    for (const bool right_side : {false, true}) {
      const double spacing = right_side ? sr : sl;
      double& lateral = right_side ? cr : cl;
      const int first = std::max(0, shape_.layer - options_.diagonal_depth);
      const int last = std::min(static_cast<int>(names_.size()) - 1,
                                shape_.layer + options_.diagonal_depth);
      for (int k = first; k <= last; k++) {
        if (k == shape_.layer || coveredOn(k, x0, x1)) {
          continue;
        }
        const DiagonalKey key{
            .layer = name_,
            .other = names_[k],
            .opposite = "",
        };
        if (model_.diagonal(key) == nullptr) {
          continue;
        }
        // A layer between covering the piece shields the parallel wire: all
        // of it under a box, the crossings' share of it otherwise.
        double open = 1;
        const int step = k < shape_.layer ? -1 : 1;
        for (int j = shape_.layer + step; j != k; j += step) {
          if (coveredOn(j, x0, x1)) {
            open = 0;
            break;
          }
          const auto runs = runs_.find(j);
          if (runs != runs_.end()) {
            open *= 1
                    - partialCover(model_,
                                   names_[j],
                                   {
                                       .layer = name_,
                                       .cover = map_.crossingCover(layers_[j]),
                                       .opposite = "",
                                   },
                                   {
                                       .layer = name_,
                                       .cover = map_.crossingCover(layers_[j]),
                                       .opposite = "",
                                   },
                                   runs->second,
                                   x0,
                                   x1,
                                   sl,
                                   sr,
                                   dbu_)
                          .share;
          }
        }
        if (open <= 0) {
          continue;
        }
        int best = -1;
        int best_gap = 0;
        for (const int i : near_) {
          const EvalBox& s = boxes_[i];
          if (s.layer != k || lo(s, a_) > x0 || hi(s, a_) < x1) {
            continue;
          }
          const int gap = right_side ? lo(s, p_) - p1_ : p0_ - hi(s, p_);
          if (2 * int64_t{gap} < -w_ || gap > diagonal_reach_[k]
              || (best >= 0 && gap >= best_gap)) {
            continue;
          }
          best = i;
          best_gap = gap;
        }
        if (best < 0) {
          continue;
        }
        const double width
            = (hi(boxes_[best], p_) - lo(boxes_[best], p_)) / dbu_;
        const double gap = best_gap / dbu_;
        const double far = right_side ? sl : sr;
        DiagonalCap d = model_.diagonal(key, width, gap, spacing, far);
        const Cover& opposite = k < shape_.layer ? above : below;
        const DiagonalKey opposite_key{
            .layer = name_,
            .other = names_[k],
            .opposite = opposite.layer,
        };
        if (!opposite.layer.empty() && opposite.share > 0
            && model_.diagonal(opposite_key) != nullptr) {
          const double t = std::clamp(opposite.share, 0.0, 1.0);
          const DiagonalCap c
              = model_.diagonal(opposite_key, width, gap, spacing, far);
          d.coupling = ((1 - t) * d.coupling) + (t * c.coupling);
          d.total_delta = ((1 - t) * d.total_delta) + (t * c.total_delta);
          d.lateral_delta = ((1 - t) * d.lateral_delta) + (t * c.lateral_delta);
          d.plane_delta = std::clamp(opposite.cross, 0.0, 1.0) * c.plane_delta;
        }
        d.coupling *= open;
        d.total_delta *= open;
        d.lateral_delta *= open;
        d.plane_delta *= open;
        const double taken
            = d.total_delta - d.coupling - d.lateral_delta - d.plane_delta;
        d.total_delta
            = (taken * (k < shape_.layer ? covered_below : covered_above))
              + d.coupling + d.lateral_delta + d.plane_delta;
        if (boxes_[best].net != net_) {
          couple(best, Source::kDiagonal, len * d.coupling);
        }
        (k < shape_.layer ? to_above : to_below) += d.plane_delta;
        const double diagonal_ground
            = len
              * (d.total_delta - d.coupling - d.lateral_delta - d.plane_delta);
        ground(Ground::kDiagonal, diagonal_ground);
        lateral += d.lateral_delta;
      }
    }
  }

  // A line end at x, unless something of the net on this layer touches
  // the end face (a wire continuing, or one meeting it at a corner); its
  // excess takes the end piece's cover states.
  void addEnd(const int x, const Cover& below, const Cover& above) const
  {
    at_ = x;
    for (const int i : near_) {
      const EvalBox& s = boxes_[i];
      if (i == self_ || s.net != net_ || s.layer != shape_.layer) {
        continue;
      }
      const bool overlaps_p
          = std::min(p1_, hi(s, p_)) > std::max(p0_, lo(s, p_));
      const bool reaches = lo(s, a_) <= x && hi(s, a_) >= x;
      if (overlaps_p && reaches) {
        return;
      }
    }
    // The end's last width along the wire.
    const int e0 = x == a0_ ? a0_ : a1_ - std::min(w_, a1_ - a0_);
    const int e1 = x == a0_ ? a0_ + std::min(w_, a1_ - a0_) : a1_;
    // A shape of the net flush against a side there makes the end the
    // outer corner of a bend: its face continues that shape's side wall.
    for (const int i : near_) {
      const EvalBox& s = boxes_[i];
      if (i == self_ || s.net != net_ || s.layer != shape_.layer) {
        continue;
      }
      const bool flush = hi(s, p_) == p0_ || lo(s, p_) == p1_;
      if (flush && std::min(e1, hi(s, a_)) > std::max(e0, lo(s, a_))) {
        counts_.corners++;
        return;
      }
    }
    counts_.ends++;
    // Reached by the net's own shape on another layer or a via.
    for (const int i : near_) {
      const EvalBox& s = boxes_[i];
      if (s.net != net_ || s.layer == shape_.layer) {
        continue;
      }
      if (std::min(p1_, hi(s, p_)) > std::max(p0_, lo(s, p_))
          && std::min(e1, hi(s, a_)) > std::max(e0, lo(s, a_))) {
        counts_.via_ends++;
        break;
      }
    }
    // The nearest shape on the layer beyond the end face, over the part of
    // the face it overlaps; the end table's facing wire is as wide.
    int facing = -1;
    int gap = 0;
    for (const int i : near_) {
      const EvalBox& s = boxes_[i];
      if (i == self_ || s.layer != shape_.layer
          || std::min(p1_, hi(s, p_)) <= std::max(p0_, lo(s, p_))) {
        continue;
      }
      const int g = x == a0_ ? a0_ - hi(s, a_) : lo(s, a_) - a1_;
      if (g > 0 && g <= facing_reach_ && (facing < 0 || g < gap)) {
        facing = i;
        gap = g;
      }
    }
    const double faced
        = facing < 0 ? 0
                     : std::min(1.0,
                                (std::min(p1_, hi(boxes_[facing], p_))
                                 - std::max(p0_, lo(boxes_[facing], p_)))
                                    / static_cast<double>(w_));
    // The neighbor on each side at the end, by how far it runs past the
    // end against its spacing: the tables' neighbors run on.
    const int d = endSpan();
    const int t0 = x == a0_ ? a0_ : a1_ - d;
    const Lateral* l = nearest(left_, t0, t0 + d);
    const Lateral* r = nearest(right_, t0, t0 + d);
    auto runs_on = [&](const Lateral* n) {
      if (n == nullptr) {
        return 0.0;
      }
      const int past = x == a0_ ? x - n->e0 : n->e1 - x;
      return n->gap <= 0
                 ? 1.0
                 : std::clamp(static_cast<double>(past) / n->gap, 0.0, 1.0);
    };
    const double on_left = runs_on(l);
    const double on_right = runs_on(r);
    // The end blended over its neighbors' and facing wire's presence.
    auto blend = [&](const LateralKey& key) {
      EndCap c;
      for (const bool hl : {false, true}) {
        for (const bool hr : {false, true}) {
          for (const bool hf : {false, true}) {
            const double weight = (hl ? on_left : 1 - on_left)
                                  * (hr ? on_right : 1 - on_right)
                                  * (hf ? faced : 1 - faced);
            if (weight == 0) {
              continue;
            }
            const EndCap e = model_.end(key,
                                        w_um_,
                                        hl ? l->gap / dbu_ : -1,
                                        hr ? r->gap / dbu_ : -1,
                                        hf ? gap / dbu_ : -1);
            c.total += weight * e.total;
            c.below += weight * e.below;
            c.above += weight * e.above;
            c.left += weight * e.left;
            c.right += weight * e.right;
            c.facing += weight * e.facing;
          }
        }
      }
      return c;
    };
    // The parts to the planes go to the nets covering the end as the
    // pieces' plane couplings do (per covered state, scaled by the cover's
    // kept coupling), nothing to the net's own.
    double total = 0;
    double to_facing = 0;
    double to_left = 0;
    double to_right = 0;
    double plane_below = 0;
    double plane_above = 0;
    double rest_below = 0;
    double rest_above = 0;
    forStates(
        below,
        above,
        [&](LateralKey& key, const double weight, const bool b, const bool t) {
          fallback(model_, key);
          if (model_.end(key) == nullptr) {
            key = {.layer = name_, .below = "", .above = ""};
          }
          const EndCap c = blend(key);
          total += weight * c.total;
          to_facing += weight * c.facing;
          to_left += weight * c.left;
          to_right += weight * c.right;
          if (!key.below.empty()) {
            if (b) {
              plane_below += weight / below.share * c.below;
            } else {
              rest_below += weight * c.below;
            }
          }
          if (!key.above.empty()) {
            if (t) {
              plane_above += weight / above.share * c.above;
            } else {
              rest_above += weight * c.above;
            }
          }
        });
    if (l != nullptr && boxes_[l->box].net != net_) {
      couple(l->box, Source::kEndLateral, to_left);
    }
    if (r != nullptr && boxes_[r->box].net != net_) {
      couple(r->box, Source::kEndLateral, to_right);
    }
    if (to_facing > 0) {
      counts_.facing_ends++;
      if (boxes_[facing].net != net_) {
        couple(facing, Source::kEndFacing, to_facing);
      }
    }
    // The tables' planes run on past the end. A cover stopping short of
    // that takes a part as far as it reaches past the end face, against
    // its distance from the wire; the rest is ground.
    auto reach = [&](const std::string& layer,
                     const int64_t reach0,
                     const int64_t reach1,
                     const bool is_below) {
      if (layer.empty()) {
        return 0.0;
      }
      const double past = (x == a0_ ? x - reach0 : reach1 - x) / dbu_;
      const fsv::Stack::Conductor& c = stack_.conductor(stack_.layer(layer));
      const fsv::Stack::Conductor& m = stack_.conductor(layers_[shape_.layer]);
      const double gap = is_below ? m.bottom - c.top : c.bottom - m.top;
      return gap <= 0 ? 1.0 : std::clamp(past / gap, 0.0, 1.0);
    };
    const double to_below
        = below.cross * plane_below
          * reach(below.layer, below.reach0, below.reach1, true);
    const double to_above
        = above.cross * plane_above
          * reach(above.layer, above.reach0, above.reach1, false);
    rest_below *= reach(below.rest, below.rest_reach0, below.rest_reach1, true);
    rest_above
        *= reach(above.rest, above.rest_reach0, above.rest_reach1, false);
    const double end_ground = total - to_facing - to_left - to_right - to_below
                              - to_above - rest_below - rest_above;
    ground(Ground::kEnd, end_ground);
    for (const auto& [box, share] : below.rest_nets) {
      if (boxes_[box].net != net_) {
        couple(box, Source::kEndRest, rest_below * share);
      }
    }
    for (const auto& [box, share] : above.rest_nets) {
      if (boxes_[box].net != net_) {
        couple(box, Source::kEndRest, rest_above * share);
      }
    }
    for (const auto& [box, share] : below.nets) {
      if (boxes_[box].net != net_) {
        couple(box, Source::kEndCover, to_below * share);
      }
    }
    for (const auto& [box, share] : above.nets) {
      if (boxes_[box].net != net_) {
        couple(box, Source::kEndCover, to_above * share);
      }
    }
  }

  // A coupling of the current part to box `other`.
  void couple(const int other, const Source source, const double value) const
  {
    out_.push_back({
        .box = self_,
        .other = other,
        .source = source,
        .ground = Ground::kPiece,
        .value = value,
        .at = at_,
    });
  }

  void ground(const Ground kind, const double value) const
  {
    out_.push_back({
        .box = self_,
        .other = -1,
        .source = Source::kLateral,
        .ground = kind,
        .value = value,
        .at = at_,
    });
  }

  const int self_;
  const EvalBox& shape_;
  const NetIndex net_;
  const std::vector<EvalBox>& boxes_;
  const std::vector<int>& near_;
  const fsv::Stack& stack_;
  const std::vector<fsv::Stack::Layer>& layers_;
  const std::vector<std::string>& names_;
  const Model& model_;
  const EvaluatorOptions& options_;
  const CoverMap& map_;
  const std::string& name_;
  const double dbu_;  // per µm
  const int lateral_reach_;
  const int crossing_reach_;
  const int facing_reach_;
  const std::vector<int>& diagonal_reach_;
  std::vector<Contribution>& out_;
  EvalCounts& counts_;
  // Where along the wire the part being added lies.
  mutable double at_ = 0;
  // The axes along and across the wire, and its extent, in database units.
  const int a_;
  const int p_;
  const int a0_;
  const int a1_;
  const int p0_;
  const int p1_;
  const int w_;
  const double w_um_;
  std::vector<Lateral> left_;
  std::vector<Lateral> right_;
  std::map<int, std::vector<Run>> runs_;  // covers by layer
  // Spans along the wire that a longer box of the net counts.
  std::vector<std::pair<int, int>> inside_;
  std::set<int> cuts_;
};

}  // namespace

Evaluator::Evaluator(const fsv::Stack& stack,
                     const Model& model,
                     const int dbu,
                     const EvaluatorOptions& options)
    : stack_(stack), model_(model), options_(options), dbu_(dbu)
{
  if (dbu < 1) {
    throw std::invalid_argument("pex: database units per µm must be positive");
  }
  layers_ = stack.conductorLayers();
  std::ranges::stable_sort(
      layers_, [&](const fsv::Stack::Layer a, const fsv::Stack::Layer b) {
        return stack.conductor(a).bottom < stack.conductor(b).bottom;
      });
  const int n = layerCount();
  maps_.reserve(n);
  for (const fsv::Stack::Layer layer : layers_) {
    names_.push_back(stack.name(layer));
    maps_.emplace_back(stack, model, layer);
  }
  auto index = [&](const std::string& name) {
    const auto it = std::ranges::find(names_, name);
    if (it == names_.end()) {
      throw std::invalid_argument("pex: the model's layer " + name
                                  + " is not a conductor of the stack");
    }
    return static_cast<int>(it - names_.begin());
  };
  lateral_reach_.assign(n, 0);
  crossing_reach_.assign(n, 0);
  facing_reach_.assign(n, 0);
  diagonal_reach_.assign(n, std::vector<int>(n, 0));
  // A table's last point in database units, to the nearest one.
  auto widen = [&](int& reach, const std::vector<double>& grid) {
    if (!grid.empty()) {
      reach = std::max(reach, static_cast<int>(std::lround(grid.back() * dbu)));
    }
  };
  for (const auto& [key, table] : model.laterals()) {
    widen(lateral_reach_[index(key.layer)], table.spacings());
  }
  for (const auto& [key, table] : model.crossings()) {
    widen(crossing_reach_[index(key.layer)], table.gaps());
  }
  for (const auto& [key, table] : model.ends()) {
    widen(facing_reach_[index(key.layer)], table.gaps());
  }
  for (const auto& [key, table] : model.diagonals()) {
    widen(diagonal_reach_[index(key.layer)][index(key.other)], table.gaps());
  }
  radius_.assign(n, 0);
  for (int i = 0; i < n; i++) {
    radius_[i] = std::max<int>({
        lateral_reach_[i],
        crossing_reach_[i],
        facing_reach_[i],
        *std::ranges::max_element(diagonal_reach_[i]),
    });
  }
}

int Evaluator::layerOf(const fsv::Box& box) const
{
  for (int i = 0; i < layerCount(); i++) {
    const fsv::Stack::Conductor& c = stack_.conductor(layers_[i]);
    if (std::abs(c.bottom - box.lo[2]) < kEps
        && std::abs(c.top - box.hi[2]) < kEps) {
      return i;
    }
  }
  return -1;
}

void Evaluator::evaluate(const std::vector<EvalBox>& boxes,
                         const std::vector<int>& target,
                         const Candidates& candidates,
                         std::vector<Contribution>& out,
                         EvalCounts* counts) const
{
  EvalCounts unused;
  EvalCounts& count = counts != nullptr ? *counts : unused;
  // The junctions: wires (at least twice as long as wide) of the net on one
  // layer crossing or touching each other. A bend has an inner and an
  // outer corner, a T two inner corners and a cross four. Each junction's
  // corners go to the wire along which the two meet over a length.
  struct Junction
  {
    int x0;
    int x1;
    int inner;
    int outer;
  };
  std::map<int, std::vector<Junction>> junctions;
  auto axis
      = [](const EvalBox& s) { return s.rect.dx() >= s.rect.dy() ? 0 : 1; };
  auto wire = [&](const EvalBox& s) {
    return s.rect.maxDXDY() >= 2 * int64_t{s.rect.minDXDY()};
  };
  for (size_t ti = 0; ti < target.size(); ti++) {
    const int i = target[ti];
    const EvalBox& x = boxes[i];
    if (x.layer < 0 || !wire(x)) {
      continue;
    }
    for (size_t tj = ti + 1; tj < target.size(); tj++) {
      const int j = target[tj];
      const EvalBox& y = boxes[j];
      if (y.layer != x.layer || !wire(y) || axis(x) == axis(y)) {
        continue;
      }
      std::array<int, 2> o0{};
      std::array<int, 2> o1{};
      bool touch = true;
      bool overlap = false;
      for (int d = 0; d < 2; d++) {
        o0[d] = std::max(lo(x, d), lo(y, d));
        o1[d] = std::min(hi(x, d), hi(y, d));
        touch = touch && o1[d] >= o0[d];
        overlap = overlap || o1[d] > o0[d];
      }
      if (!touch || !overlap) {
        continue;
      }
      auto atEnd = [&](const EvalBox& s) {
        const int a = axis(s);
        return o0[a] <= lo(s, a) || o1[a] >= hi(s, a);
      };
      const int ends = atEnd(x) + atEnd(y);
      // By the wires' ends at the junction: a cross, a tee, a bend.
      constexpr std::array<int, 3> kInner{4, 2, 1};
      if (ends == 2) {
        count.bends++;
      } else if (ends == 1) {
        count.tees++;
      } else {
        count.crosses++;
      }
      const int ax = axis(x);
      const bool along_x = o1[ax] > o0[ax];
      const int a = along_x ? ax : axis(y);
      junctions[along_x ? i : j].push_back({
          .x0 = o0[a],
          .x1 = o1[a],
          .inner = kInner[ends],
          .outer = ends == 2 ? 1 : 0,
      });
    }
  }
  const Context context{
      .stack = stack_,
      .model = model_,
      .options = options_,
      .layers = layers_,
      .names = names_,
      .dbu = static_cast<double>(dbu_),
      .lateral_reach = lateral_reach_,
      .crossing_reach = crossing_reach_,
      .facing_reach = facing_reach_,
      .diagonal_reach = diagonal_reach_,
  };
  std::vector<int> near;
  for (const int i : target) {
    if (boxes[i].layer < 0) {
      continue;
    }
    count.wires++;
    near.clear();
    candidates(i, near);
    Wire wire(i, boxes, near, context, maps_[boxes[i].layer], out, count);
    wire.estimate();
    const auto found = junctions.find(i);
    if (found != junctions.end()) {
      for (const Junction& j : found->second) {
        wire.addCorners(j.x0, j.x1, j.inner, j.outer);
      }
    }
  }
}

NetEstimate estimate(const fsv::Geometry& geometry,
                     const fsv::Stack& stack,
                     const Model& model,
                     const NetIndex target,
                     const EvaluatorOptions& options,
                     const int dbu)
{
  const Evaluator evaluator(stack, model, dbu, options);
  NetEstimate est;
  std::vector<EvalBox> boxes;
  std::vector<int> mine;
  boxes.reserve(geometry.boxes().size());
  for (int b = 0; std::cmp_less(b, geometry.boxes().size()); b++) {
    const fsv::Box& box = geometry.boxes()[b];
    const NetIndex net = geometry.boxNet(b);
    const int layer = evaluator.layerOf(box);
    if (layer < 0) {
      // A via spans from one conductor's top to another's bottom.
      bool via = false;
      for (const fsv::Stack::Layer v : stack.viaLayers()) {
        if (!stack.via(v).lower.valid()) {
          continue;  // a substrate via would ground the net
        }
        const fsv::Stack::ZExtent z = stack.zExtent(v);
        if (std::abs(z.bottom - box.lo[2]) < kEps
            && std::abs(z.top - box.hi[2]) < kEps) {
          via = true;
        }
      }
      if (!via) {
        throw std::invalid_argument("a box of net " + geometry.netName(net)
                                    + " lies on no layer of the stack");
      }
      if (net == target) {
        est.vias++;
      }
    }
    auto units = [dbu](const double um) {
      const double v = std::round(um * dbu);
      if (std::abs(v) > std::numeric_limits<int>::max()) {
        throw std::invalid_argument("pex: a coordinate is too far out for "
                                    + std::to_string(dbu) + " units per µm");
      }
      return static_cast<int>(v);
    };
    boxes.push_back({
        .net = net,
        .layer = layer,
        .rect = odb::Rect(units(box.lo[0]),
                          units(box.lo[1]),
                          units(box.hi[0]),
                          units(box.hi[1])),
    });
    if (net == target) {
      mine.push_back(b);
    }
  }
  const int n = static_cast<int>(boxes.size());
  std::vector<Contribution> parts;
  evaluator.evaluate(
      boxes,
      mine,
      [n](int, std::vector<int>& out) {
        out.resize(n);
        std::iota(out.begin(), out.end(), 0);
      },
      parts,
      &est);
  for (const Contribution& c : parts) {
    if (c.other < 0) {
      est.ground += c.value;
      est.ground_sources[static_cast<size_t>(c.ground)] += c.value;
    } else {
      const NetIndex net = boxes[c.other].net;
      est.couplings[net] += c.value;
      est.sources[net][static_cast<size_t>(c.source)] += c.value;
    }
  }
  return est;
}

}  // namespace pex
