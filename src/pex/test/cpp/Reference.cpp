// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "Reference.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "fsv/Geometry.h"
#include "fsv/Stack.h"
#include "pex/CoverMap.h"
#include "pex/Model.h"

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

constexpr double kEps = 1e-9;

// A box on a conductor layer (or a via, layer -1), in the x-y plane.
struct Shape
{
  NetIndex net;
  int layer;  // index into the layers, by height
  double lo[2];
  double hi[2];
};

void couple(NetEstimate& out,
            const NetIndex net,
            const Source source,
            const double value)
{
  out.couplings[net] += value;
  out.sources[net][static_cast<size_t>(source)] += value;
}

// The conductors of the stack ascending by bottom; shapes carry an index
// into them.
struct Layers
{
  std::vector<fsv::Stack::Layer> layers;
  std::vector<std::string> names;  // the layers' names, for the model keys
  int index(const fsv::Stack& stack, const fsv::Box& box) const
  {
    for (int i = 0; i < static_cast<int>(layers.size()); i++) {
      const fsv::Stack::Conductor& c = stack.conductor(layers[i]);
      if (std::abs(c.bottom - box.lo[2]) < kEps
          && std::abs(c.top - box.hi[2]) < kEps) {
        return i;
      }
    }
    return -1;
  }
};

Layers layersByHeight(const fsv::Stack& stack)
{
  Layers layers;
  layers.layers = stack.conductorLayers();
  std::stable_sort(layers.layers.begin(),
                   layers.layers.end(),
                   [&](const fsv::Stack::Layer a, const fsv::Stack::Layer b) {
                     return stack.conductor(a).bottom
                            < stack.conductor(b).bottom;
                   });
  for (const fsv::Stack::Layer layer : layers.layers) {
    layers.names.push_back(stack.name(layer));
  }
  return layers;
}

// The shapes of a geometry, with the target's via boxes counted.
std::vector<Shape> shapesOf(const fsv::Geometry& geometry,
                            const fsv::Stack& stack,
                            const Layers& layers,
                            const NetIndex target,
                            NetEstimate& out)
{
  std::vector<Shape> shapes;
  shapes.reserve(geometry.boxes().size());
  for (int b = 0; b < static_cast<int>(geometry.boxes().size()); b++) {
    const fsv::Box& box = geometry.boxes()[b];
    const int layer = layers.index(stack, box);
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
        throw std::invalid_argument("a box of net "
                                    + geometry.netName(geometry.boxNet(b))
                                    + " lies on no layer of the stack");
      }
      if (geometry.boxNet(b) == target) {
        out.vias++;
      }
    }
    shapes.push_back({geometry.boxNet(b),
                      layer,
                      {box.lo[0], box.lo[1]},
                      {box.hi[0], box.hi[1]}});
  }
  return shapes;
}

// The lateral table for a context, falling back to fewer covers. The
// covers come resolved onto tabulated ones (CoverMap), so this only meets
// a pair the builder did not tabulate together.
const LateralKey& fallback(const Model& model, LateralKey& key)
{
  if (model.lateral(key) != nullptr) {
    return key;
  }
  LateralKey below_only{key.layer, key.below, ""};
  if (!key.above.empty() && model.lateral(below_only) != nullptr) {
    key = below_only;
    return key;
  }
  LateralKey above_only{key.layer, "", key.above};
  if (!key.below.empty() && model.lateral(above_only) != nullptr) {
    key = above_only;
    return key;
  }
  key = {key.layer, "", ""};
  return key;
}

// A cover's state on one side of a piece: the layer, its share (1 when a
// box covers the piece), and the nets that take the coupling, weighted by
// their part of it.
struct Cover
{
  std::string layer;
  double share = 0;                 // of the total
  double cross = 0;                 // of the plane's coupling
  std::map<NetIndex, double> nets;  // weight by net, summing to 1 when any
  // A box covering the piece: the span along the wire of the layer's runs
  // joined to it. Unbounded for crossings.
  double reach0 = -std::numeric_limits<double>::infinity();
  double reach1 = std::numeric_limits<double>::infinity();
  // Behind crossings, a farther layer covering the piece: it, not open,
  // is what the rest of the piece sees.
  std::string rest;
  std::map<NetIndex, double> rest_nets;
  double rest_reach0 = 0;
  double rest_reach1 = 0;
};

struct Run
{
  NetIndex net;
  double a0;  // over the wire; the whole box when past an end
  double a1;
  double e0;  // the whole box
  double e1;
  // How far the box runs past the wire's two sides.
  double past_lo;
  double past_hi;
};

struct Lateral
{
  NetIndex net;
  double gap;
  double a0;  // over the wire
  double a1;
  double e0;  // the whole box
  double e1;
};

// The share of a layer's crossings over a piece [x0, x1] that no run
// covers: from the nearest run before and after, through the one-sided
// shares integrated over the piece, each as much as the run reaches past
// the wire's sides keeps (Model::crossingReach).
// The crossing table is `key`'s, the cover of the tabulated layer
// standing in for `layer` when that one has none (CoverMap), at the
// piece's neighbor spacings `left` and `right` (negative: none); the
// reach is `reach_key`'s.
Cover partialCover(const Model& model,
                   const std::string& layer,
                   const CrossingKey& key,
                   const CrossingKey& reach_key,
                   const std::vector<Run>& runs,
                   const double x0,
                   const double x1,
                   const double left,
                   const double right)
{
  Cover cover;
  cover.layer = layer;
  if (model.crossing(key) == nullptr) {
    return cover;
  }
  const Run* before = nullptr;
  const Run* after = nullptr;
  for (const Run& r : runs) {
    if (r.a1 <= x0 + kEps && (before == nullptr || r.a1 > before->a1)) {
      before = &r;
    }
    if (r.a0 >= x1 - kEps && (after == nullptr || r.a0 < after->a0)) {
      after = &r;
    }
  }
  const double len = x1 - x0;
  // The one-sided share integrated from the crossing's edge out to d.
  auto integral = [&](const double d, const bool cross) {
    const CrossingShare k = model.crossingKept(key, d, left, right);
    return cross ? k.cross : k.total;
  };
  double total = 0;
  double cross = 0;
  auto reach = [&](const Run& r, const double from, const double to) {
    return model.crossingReach(
        reach_key, from, to, r.past_lo, r.past_hi, left, right);
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
    cover.nets[before->net] += c;
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
    cover.nets[after->net] += c;
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
// nets by their part of the coupling.
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

// One wire of the target, along axis `a`, with what lies around it.
class Wire
{
 public:
  Wire(const Shape& shape,
       const std::vector<Shape>& shapes,
       const fsv::Stack& stack,
       const Layers& layers,
       const Model& model,
       const CoverMap& map,
       const ReferenceOptions& options)
      : shape_(shape),
        shapes_(shapes),
        stack_(stack),
        layers_(layers),
        model_(model),
        options_(options),
        map_(map),
        name_(layers.names[shape.layer]),
        a_(shape.hi[0] - shape.lo[0] >= shape.hi[1] - shape.lo[1] ? 0 : 1),
        p_(1 - a_),
        a0_(shape.lo[a_]),
        a1_(shape.hi[a_]),
        p0_(shape.lo[p_]),
        p1_(shape.hi[p_]),
        w_(p1_ - p0_)
  {
    collect();
  }

  // Adds the wire's pieces and line ends to the estimate.
  void estimate(const NetIndex target, NetEstimate& out)
  {
    const std::vector<double> edges(cuts_.begin(), cuts_.end());
    for (size_t i = 0; i + 1 < edges.size(); i++) {
      if (edges[i + 1] - edges[i] <= kEps) {
        continue;
      }
      if (insideOther(edges[i], edges[i + 1])) {
        continue;
      }
      out.pieces++;
      const auto [below, above] = coversOn(edges[i], edges[i + 1]);
      addPiece(edges[i], edges[i + 1], below, above, target, out);
    }
    // An end takes the covers at the end itself, crossings by their share
    // there rather than over the end piece.
    const double d = std::min(1e-3, (a1_ - a0_) / 2);
    const auto [start_below, start_above] = coversOn(a0_, a0_ + d);
    addEnd(a0_, start_below, start_above, target, out);
    const auto [end_below, end_above] = coversOn(a1_ - d, a1_);
    addEnd(a1_, end_below, end_above, target, out);
  }

  // The corners where another wire of the net meets this one over [x0, x1]
  // along it: `inner` inner and `outer` outer corners, with the covers
  // there. The parts to the planes go to the covering nets as a line
  // end's do.
  void addCorners(const double x0,
                  const double x1,
                  const int inner,
                  const int outer,
                  const NetIndex target,
                  NetEstimate& out) const
  {
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
            key = {name_, "", ""};
          }
          const Corners c = model_.corners(key, w_);
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
    const double ground = total - to_below - to_above - rest_below - rest_above;
    out.ground += ground;
    out.ground_sources[2] += ground;
    for (const auto& [net, share] : below.rest_nets) {
      if (net != target) {
        couple(out, net, Source::kEndRest, rest_below * share);
      }
    }
    for (const auto& [net, share] : above.rest_nets) {
      if (net != target) {
        couple(out, net, Source::kEndRest, rest_above * share);
      }
    }
    for (const auto& [net, share] : below.nets) {
      if (net != target) {
        couple(out, net, Source::kEndCover, to_below * share);
      }
    }
    for (const auto& [net, share] : above.nets) {
      if (net != target) {
        couple(out, net, Source::kEndCover, to_above * share);
      }
    }
  }

 private:
  // Lateral candidates with their gap, cover runs by layer, and the cuts
  // where any of them starts or ends.
  void collect()
  {
    cuts_ = {a0_, a1_};
    // On each other layer, the nearest box covering the wire's line past
    // each end: the end pieces and the ends take a share of it.
    std::map<int, const Shape*> past_start;
    std::map<int, const Shape*> past_end;
    for (const Shape& s : shapes_) {
      if (&s == &shape_) {
        continue;
      }
      const double o0 = std::max(a0_, s.lo[a_]);
      const double o1 = std::min(a1_, s.hi[a_]);
      if (o1 - o0 <= kEps) {
        const double overlap
            = std::min(p1_, s.hi[p_]) - std::max(p0_, s.lo[p_]);
        if (s.layer >= 0 && s.layer != shape_.layer
            && overlap >= options_.cover_overlap * w_) {
          if (s.hi[a_] <= a0_ + kEps) {
            const Shape*& best = past_start[s.layer];
            if (best == nullptr || s.hi[a_] > best->hi[a_]) {
              best = &s;
            }
          } else if (s.lo[a_] >= a1_ - kEps) {
            const Shape*& best = past_end[s.layer];
            if (best == nullptr || s.lo[a_] < best->lo[a_]) {
              best = &s;
            }
          }
        }
        continue;
      }
      if (s.layer == shape_.layer) {
        if (s.hi[p_] <= p0_ + kEps) {
          left_.push_back({s.net, p0_ - s.hi[p_], o0, o1, s.lo[a_], s.hi[a_]});
        } else if (s.lo[p_] >= p1_ - kEps) {
          right_.push_back({s.net, s.lo[p_] - p1_, o0, o1, s.lo[a_], s.hi[a_]});
        } else if (s.net == shape_.net && s.lo[p_] <= p0_ + kEps
                   && s.hi[p_] >= p1_ - kEps && dominates(s)) {
          // A longer box of the net across the wire's whole width: that
          // box counts the overlap.
          inside_.emplace_back(o0, o1);
        } else {
          continue;  // overlapping on the same layer: a junction
        }
      } else if (s.layer >= 0) {
        const double overlap
            = std::min(p1_, s.hi[p_]) - std::max(p0_, s.lo[p_]);
        if (overlap < options_.cover_overlap * w_) {
          // Beside the wire: a diagonal candidate on a nearby layer.
          if (std::abs(s.layer - shape_.layer) > options_.diagonal_depth) {
            continue;
          }
        } else {
          runs_[s.layer].push_back({s.net,
                                    o0,
                                    o1,
                                    s.lo[a_],
                                    s.hi[a_],
                                    p0_ - s.lo[p_],
                                    s.hi[p_] - p1_});
        }
      } else {
        continue;
      }
      cuts_.insert(o0);
      cuts_.insert(o1);
    }
    for (const auto* past : {&past_start, &past_end}) {
      for (const auto& [layer, s] : *past) {
        runs_[layer].push_back({s->net,
                                s->lo[a_],
                                s->hi[a_],
                                s->lo[a_],
                                s->hi[a_],
                                p0_ - s->lo[p_],
                                s->hi[p_] - p1_});
      }
    }
  }

  // Whether a box of the net counts an overlap with this wire rather than
  // this wire: the longer one, or the first of two as long.
  bool dominates(const Shape& s) const
  {
    auto length = [](const Shape& b) {
      return std::max(b.hi[0] - b.lo[0], b.hi[1] - b.lo[1]);
    };
    const double ls = length(s);
    const double lw = length(shape_);
    return ls > lw + kEps || (std::abs(ls - lw) <= kEps && &s < &shape_);
  }

  bool insideOther(const double x0, const double x1) const
  {
    for (const auto& [o0, o1] : inside_) {
      if (o0 <= x0 + kEps && o1 >= x1 - kEps) {
        return true;
      }
    }
    return false;
  }

  // The nearest neighbor on a side over a piece.
  static const Lateral* nearest(const std::vector<Lateral>& side,
                                const double x0,
                                const double x1)
  {
    const Lateral* best = nullptr;
    for (const Lateral& l : side) {
      if (l.a0 <= x0 + kEps && l.a1 >= x1 - kEps
          && (best == nullptr || l.gap < best->gap)) {
        best = &l;
      }
    }
    return best;
  }

  bool coveredOn(const int layer, const double x0, const double x1) const
  {
    const auto it = runs_.find(layer);
    if (it == runs_.end()) {
      return false;
    }
    for (const Run& r : it->second) {
      if (r.a0 <= x0 + kEps && r.a1 >= x1 - kEps) {
        return true;
      }
    }
    return false;
  }

  // The cover on a side: the nearest layer with a run over the piece, or
  // else the layer whose crossings keep the largest share.
  // The covers below and above a span, each side's crossings sharing as
  // they do with the other side's cover.
  std::pair<Cover, Cover> coversOn(const double x0, const double x1) const
  {
    const Cover below = coverOn(true, x0, x1);
    const Cover above = coverOn(false, x0, x1);
    return {coverOn(true, x0, x1, &above), coverOn(false, x0, x1, &below)};
  }

  // A side's crossings keep less where a cover on the other side pulls
  // the field to it: with `other`, the crossing table with a plane on that
  // cover's layer blends in by its share.
  Cover coverOn(const bool below,
                const double x0,
                const double x1,
                const Cover* other = nullptr) const
  {
    // The neighbors over the span, which hold the crossings' reach.
    const Lateral* l = nearest(left_, x0, x1);
    const Lateral* r = nearest(right_, x0, x1);
    const double sl = l == nullptr ? -1 : l->gap;
    const double sr = r == nullptr ? -1 : r->gap;
    Cover best;
    for (int l = below ? shape_.layer - 1 : shape_.layer + 1;
         l >= 0 && l < static_cast<int>(layers_.names.size());
         l += below ? -1 : 1) {
      const auto it = runs_.find(l);
      if (it == runs_.end()) {
        continue;
      }
      for (const Run& r : it->second) {
        if (r.a0 <= x0 + kEps && r.a1 >= x1 - kEps) {
          Cover full;
          full.layer = layers_.names[l];
          full.share = full.cross = 1;
          full.nets[r.net] = 1;
          full.reach0 = r.e0;
          full.reach1 = r.e1;
          for (bool grew = true; grew;) {
            grew = false;
            for (const Run& q : it->second) {
              if (q.e0 <= full.reach1 + kEps && q.e1 >= full.reach0 - kEps
                  && (q.e0 < full.reach0 - kEps || q.e1 > full.reach1 + kEps)) {
                full.reach0 = std::min(full.reach0, q.e0);
                full.reach1 = std::max(full.reach1, q.e1);
                grew = true;
              }
            }
          }
          // A box crossing the wire keeps as much of a plane's effect as
          // it runs past the wire's sides.
          const CrossingKey key{
              name_, map_.crossingCover(layers_.layers[l]), ""};
          if (full.reach1 - full.reach0 <= r.past_lo + r.past_hi + w_
              && model_.crossing(key) != nullptr) {
            const CrossingShare f
                = model_.crossingReach(key, 0, 0, r.past_lo, r.past_hi, sl, sr);
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
      const CrossingKey key{name_, map_.crossingCover(layers_.layers[l]), ""};
      Cover partial = partialCover(
          model_, layers_.names[l], key, key, it->second, x0, x1, sl, sr);
      if (other != nullptr && other->share > 0 && !other->layer.empty()) {
        const CrossingKey planed{key.layer, key.cover, other->layer};
        if (model_.crossing(planed) != nullptr) {
          const Cover with = partialCover(model_,
                                          layers_.names[l],
                                          planed,
                                          key,
                                          it->second,
                                          x0,
                                          x1,
                                          sl,
                                          sr);
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
    const std::vector<CoverMap::Part> none{{"", 1}};
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
            LateralKey key{name_, pb.key, pt.key};
            f(key, wb * pb.weight * wt * pt.weight, b, t);
          }
        }
      }
    }
  }

  // The lateral blend over the cover states, the covers' couplings, and
  // the parallel wires on nearby layers beside the piece.
  void addPiece(const double x0,
                const double x1,
                const Cover& below,
                const Cover& above,
                const NetIndex target,
                NetEstimate& out) const
  {
    const double len = x1 - x0;
    const Lateral* l = nearest(left_, x0, x1);
    const Lateral* r = nearest(right_, x0, x1);
    const double sl = l == nullptr ? -1 : l->gap;
    const double sr = r == nullptr ? -1 : r->gap;
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
            LateralKey only_below{key.layer, key.below, ""};
            LateralKey only_above{key.layer, "", key.above};
            ground_below_only
                += weight
                   * model_.lateral(fallback(model_, only_below), w_, sl, sr)
                         .ground;
            ground_above_only
                += weight
                   * model_.lateral(fallback(model_, only_above), w_, sl, sr)
                         .ground;
          }
          const LateralCap cap
              = model_.lateral(fallback(model_, key), w_, sl, sr);
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
    out.ground += piece_ground;
    out.ground_sources[0] += piece_ground;
    for (const auto& [net, share] : below.rest_nets) {
      if (net != target) {
        couple(out, net, Source::kRest, len * rest_below * share);
      }
    }
    for (const auto& [net, share] : above.rest_nets) {
      if (net != target) {
        couple(out, net, Source::kRest, len * rest_above * share);
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
                 target,
                 cl,
                 cr,
                 to_below,
                 to_above,
                 out);
    if (l != nullptr && l->net != target) {
      couple(out, l->net, Source::kLateral, len * cl);
    }
    if (r != nullptr && r->net != target) {
      couple(out, r->net, Source::kLateral, len * cr);
    }
    for (const auto& [net, share] : below.nets) {
      if (net != target) {
        couple(out, net, Source::kCover, len * to_below * share);
      }
    }
    for (const auto& [net, share] : above.nets) {
      if (net != target) {
        couple(out, net, Source::kCover, len * to_above * share);
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
  void addDiagonals(const double x0,
                    const double x1,
                    const double sl,
                    const double sr,
                    const Cover& below,
                    const Cover& above,
                    const double ground_below_only,
                    const double ground_above_only,
                    const NetIndex target,
                    double& cl,
                    double& cr,
                    double& to_below,
                    double& to_above,
                    NetEstimate& out) const
  {
    const double len = x1 - x0;
    // The diagonal tables have no covers on the parallel wire's side. A
    // parallel wire below takes
    // ground the piece's cover below would otherwise take, and one above
    // ground the cover above would: its part taken from ground scales by
    // the piece's ground with that side's cover alone over the ground with
    // no cover. Its coupling stays, since the wire beside is nearer than
    // the cover.
    LateralKey open_key{name_, "", ""};
    const double open_ground
        = model_.lateral(fallback(model_, open_key), w_, sl, sr).ground;
    auto ratio = [&](const double g) {
      return open_ground > 0 ? std::clamp(g / open_ground, 0.0, 1.0) : 1.0;
    };
    const double covered_below = ratio(ground_below_only);
    const double covered_above = ratio(ground_above_only);
    for (const bool right_side : {false, true}) {
      const double spacing = right_side ? sr : sl;
      double& lateral = right_side ? cr : cl;
      const int lo = std::max(0, shape_.layer - options_.diagonal_depth);
      const int hi = std::min(static_cast<int>(layers_.names.size()) - 1,
                              shape_.layer + options_.diagonal_depth);
      for (int k = lo; k <= hi; k++) {
        if (k == shape_.layer || coveredOn(k, x0, x1)) {
          continue;
        }
        const DiagonalKey key{name_, layers_.names[k], ""};
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
                    - partialCover(
                          model_,
                          layers_.names[j],
                          {name_, map_.crossingCover(layers_.layers[j]), ""},
                          {name_, map_.crossingCover(layers_.layers[j]), ""},
                          runs->second,
                          x0,
                          x1,
                          sl,
                          sr)
                          .share;
          }
        }
        if (open <= 0) {
          continue;
        }
        const Shape* best = nullptr;
        double best_gap = 0;
        for (const Shape& s : shapes_) {
          if (s.layer != k || s.lo[a_] > x0 + kEps || s.hi[a_] < x1 - kEps) {
            continue;
          }
          const double gap = right_side ? s.lo[p_] - p1_ : p0_ - s.hi[p_];
          if (gap < -w_ / 2 || (best != nullptr && gap >= best_gap)) {
            continue;
          }
          best = &s;
          best_gap = gap;
        }
        if (best == nullptr) {
          continue;
        }
        const double width = best->hi[p_] - best->lo[p_];
        const double far = right_side ? sl : sr;
        DiagonalCap d = model_.diagonal(key, width, best_gap, spacing, far);
        const Cover& opposite = k < shape_.layer ? above : below;
        const DiagonalKey opposite_key{name_, layers_.names[k], opposite.layer};
        if (!opposite.layer.empty() && opposite.share > 0
            && model_.diagonal(opposite_key) != nullptr) {
          const double t = std::clamp(opposite.share, 0.0, 1.0);
          const DiagonalCap c
              = model_.diagonal(opposite_key, width, best_gap, spacing, far);
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
        if (best->net != target) {
          couple(out, best->net, Source::kDiagonal, len * d.coupling);
        }
        (k < shape_.layer ? to_above : to_below) += d.plane_delta;
        const double diagonal_ground
            = len
              * (d.total_delta - d.coupling - d.lateral_delta - d.plane_delta);
        out.ground += diagonal_ground;
        out.ground_sources[1] += diagonal_ground;
        lateral += d.lateral_delta;
      }
    }
  }

  // A line end at x, unless something of the net on this layer touches
  // the end face (a wire continuing, or one meeting it at a corner); its
  // excess takes the end piece's cover states.
  void addEnd(const double x,
              const Cover& below,
              const Cover& above,
              const NetIndex target,
              NetEstimate& out) const
  {
    for (const Shape& s : shapes_) {
      if (&s == &shape_ || s.net != target || s.layer != shape_.layer) {
        continue;
      }
      const bool overlaps_p
          = std::min(p1_, s.hi[p_]) - std::max(p0_, s.lo[p_]) > kEps;
      const bool reaches = s.lo[a_] <= x + kEps && s.hi[a_] >= x - kEps;
      if (overlaps_p && reaches) {
        return;
      }
    }
    // The end's last width along the wire.
    const double e0 = x == a0_ ? a0_ : a1_ - std::min(w_, a1_ - a0_);
    const double e1 = x == a0_ ? a0_ + std::min(w_, a1_ - a0_) : a1_;
    // A shape of the net flush against a side there makes the end the
    // outer corner of a bend: its face continues that shape's side wall.
    for (const Shape& s : shapes_) {
      if (&s == &shape_ || s.net != target || s.layer != shape_.layer) {
        continue;
      }
      const bool flush
          = std::abs(s.hi[p_] - p0_) < 1e-6 || std::abs(s.lo[p_] - p1_) < 1e-6;
      if (flush && std::min(e1, s.hi[a_]) - std::max(e0, s.lo[a_]) > kEps) {
        out.corners++;
        return;
      }
    }
    out.ends++;
    // Reached by the net's own shape on another layer or a via.
    for (const Shape& s : shapes_) {
      if (s.net != target || s.layer == shape_.layer) {
        continue;
      }
      if (std::min(p1_, s.hi[p_]) - std::max(p0_, s.lo[p_]) > kEps
          && std::min(e1, s.hi[a_]) - std::max(e0, s.lo[a_]) > kEps) {
        out.via_ends++;
        break;
      }
    }
    // The nearest shape on the layer beyond the end face, over the part of
    // the face it overlaps; the end table's facing wire is as wide.
    const Shape* facing = nullptr;
    double gap = 0;
    for (const Shape& s : shapes_) {
      if (&s == &shape_ || s.layer != shape_.layer
          || std::min(p1_, s.hi[p_]) - std::max(p0_, s.lo[p_]) <= kEps) {
        continue;
      }
      const double g = x == a0_ ? a0_ - s.hi[a_] : s.lo[a_] - a1_;
      if (g > kEps && (facing == nullptr || g < gap)) {
        facing = &s;
        gap = g;
      }
    }
    const double faced = facing == nullptr
                             ? 0
                             : std::min(1.0,
                                        (std::min(p1_, facing->hi[p_])
                                         - std::max(p0_, facing->lo[p_]))
                                            / w_);
    // The neighbor on each side at the end, by how far it runs past the
    // end against its spacing: the tables' neighbors run on.
    const double d = std::min(1e-3, (a1_ - a0_) / 2);
    const double t0 = x == a0_ ? a0_ : a1_ - d;
    const Lateral* l = nearest(left_, t0, t0 + d);
    const Lateral* r = nearest(right_, t0, t0 + d);
    auto runs_on = [&](const Lateral* n) {
      if (n == nullptr) {
        return 0.0;
      }
      const double past = x == a0_ ? x - n->e0 : n->e1 - x;
      return n->gap <= 0 ? 1.0 : std::clamp(past / n->gap, 0.0, 1.0);
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
            const EndCap e = model_.end(
                key, w_, hl ? l->gap : -1, hr ? r->gap : -1, hf ? gap : -1);
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
            key = {name_, "", ""};
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
    if (l != nullptr && l->net != target) {
      couple(out, l->net, Source::kEndLateral, to_left);
    }
    if (r != nullptr && r->net != target) {
      couple(out, r->net, Source::kEndLateral, to_right);
    }
    if (to_facing > 0) {
      out.facing_ends++;
      if (facing->net != target) {
        couple(out, facing->net, Source::kEndFacing, to_facing);
      }
    }
    // The tables' planes run on past the end. A cover stopping short of
    // that takes a part as far as it reaches past the end face, against
    // its distance from the wire; the rest is ground.
    auto reach = [&](const std::string& layer,
                     const double reach0,
                     const double reach1,
                     const bool is_below) {
      if (layer.empty()) {
        return 0.0;
      }
      const double past = x == a0_ ? x - reach0 : reach1 - x;
      const fsv::Stack::Conductor& c = stack_.conductor(stack_.layer(layer));
      const fsv::Stack::Conductor& m
          = stack_.conductor(layers_.layers[shape_.layer]);
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
    out.ground += end_ground;
    out.ground_sources[2] += end_ground;
    for (const auto& [net, share] : below.rest_nets) {
      if (net != target) {
        couple(out, net, Source::kEndRest, rest_below * share);
      }
    }
    for (const auto& [net, share] : above.rest_nets) {
      if (net != target) {
        couple(out, net, Source::kEndRest, rest_above * share);
      }
    }
    for (const auto& [net, share] : below.nets) {
      if (net != target) {
        couple(out, net, Source::kEndCover, to_below * share);
      }
    }
    for (const auto& [net, share] : above.nets) {
      if (net != target) {
        couple(out, net, Source::kEndCover, to_above * share);
      }
    }
  }

  const Shape& shape_;
  const std::vector<Shape>& shapes_;
  const fsv::Stack& stack_;
  const Layers& layers_;
  const Model& model_;
  const ReferenceOptions& options_;
  const CoverMap& map_;
  const std::string& name_;
  const int a_;
  const int p_;
  const double a0_;
  const double a1_;
  const double p0_;
  const double p1_;
  const double w_;
  std::vector<Lateral> left_;
  std::vector<Lateral> right_;
  std::map<int, std::vector<Run>> runs_;  // covers by layer
  // Spans along the wire that a longer box of the net counts.
  std::vector<std::pair<double, double>> inside_;
  std::set<double> cuts_;
};

}  // namespace

NetEstimate estimate(const fsv::Geometry& geometry,
                     const fsv::Stack& stack,
                     const Model& model,
                     const NetIndex target,
                     const ReferenceOptions& options)
{
  const Layers layers = layersByHeight(stack);
  NetEstimate out;
  const std::vector<Shape> shapes
      = shapesOf(geometry, stack, layers, target, out);
  // The junctions: wires (at least twice as long as wide) of the net on one
  // layer crossing or touching each other. A bend has an inner and an
  // outer corner, a T two inner corners and a cross four. Each junction's
  // corners go to the wire along which the two meet over a length.
  struct Junction
  {
    double x0;
    double x1;
    int inner;
    int outer;
  };
  std::map<size_t, std::vector<Junction>> junctions;
  auto axis = [](const Shape& s) {
    return s.hi[0] - s.lo[0] >= s.hi[1] - s.lo[1] ? 0 : 1;
  };
  auto wire = [&](const Shape& s) {
    const int a = axis(s);
    return s.hi[a] - s.lo[a] >= 2 * (s.hi[1 - a] - s.lo[1 - a]);
  };
  for (size_t i = 0; i < shapes.size(); i++) {
    const Shape& x = shapes[i];
    if (x.net != target || x.layer < 0 || !wire(x)) {
      continue;
    }
    for (size_t j = i + 1; j < shapes.size(); j++) {
      const Shape& y = shapes[j];
      if (y.net != target || y.layer != x.layer || !wire(y)
          || axis(x) == axis(y)) {
        continue;
      }
      double o0[2];
      double o1[2];
      bool touch = true;
      bool overlap = false;
      for (int d = 0; d < 2; d++) {
        o0[d] = std::max(x.lo[d], y.lo[d]);
        o1[d] = std::min(x.hi[d], y.hi[d]);
        touch = touch && o1[d] - o0[d] > -kEps;
        overlap = overlap || o1[d] - o0[d] > kEps;
      }
      if (!touch || !overlap) {
        continue;
      }
      auto atEnd = [&](const Shape& s) {
        const int a = axis(s);
        return o0[a] <= s.lo[a] + kEps || o1[a] >= s.hi[a] - kEps;
      };
      const int ends = atEnd(x) + atEnd(y);
      (ends == 2 ? out.bends : ends == 1 ? out.tees : out.crosses)++;
      const int ax = axis(x);
      const bool along_x = o1[ax] - o0[ax] > kEps;
      const int a = along_x ? ax : axis(y);
      junctions[along_x ? i : j].push_back({o0[a],
                                            o1[a],
                                            ends == 2   ? 1
                                            : ends == 1 ? 2
                                                        : 4,
                                            ends == 2 ? 1 : 0});
    }
  }
  // One cover map per layer the net has wires on.
  std::map<int, CoverMap> maps;
  for (size_t i = 0; i < shapes.size(); i++) {
    const Shape& shape = shapes[i];
    if (shape.net != target || shape.layer < 0) {
      continue;
    }
    out.wires++;
    const auto it = maps.try_emplace(
        shape.layer, stack, model, layers.layers[shape.layer]);
    Wire wire(shape, shapes, stack, layers, model, it.first->second, options);
    wire.estimate(target, out);
    const auto found = junctions.find(i);
    if (found != junctions.end()) {
      for (const Junction& j : found->second) {
        wire.addCorners(j.x0, j.x1, j.inner, j.outer, target, out);
      }
    }
  }
  return out;
}

}  // namespace pex
