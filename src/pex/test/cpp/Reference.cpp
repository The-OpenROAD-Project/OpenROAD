// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "Reference.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "fsv/Geometry.h"
#include "fsv/Stack.h"
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

// The lateral table for a context, falling back to fewer covers.
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
};

struct Run
{
  NetIndex net;
  double a0;
  double a1;
};

struct Lateral
{
  NetIndex net;
  double gap;
  double a0;
  double a1;
};

// The share of a layer's crossings over a piece [x0, x1] that no run
// covers: from the nearest run before and after, through the one-sided
// shares integrated over the piece.
Cover partialCover(const Model& model,
                   const CrossingKey& key,
                   const std::vector<Run>& runs,
                   const double x0,
                   const double x1)
{
  Cover cover;
  cover.layer = key.cover;
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
    const CrossingShare k = model.crossingKept(key, d);
    return cross ? k.cross : k.total;
  };
  double total = 0;
  double cross = 0;
  if (before != nullptr) {
    const double t
        = integral(x1 - before->a1, false) - integral(x0 - before->a1, false);
    const double c
        = integral(x1 - before->a1, true) - integral(x0 - before->a1, true);
    total += t;
    cross += c;
    cover.nets[before->net] += c;
  }
  if (after != nullptr) {
    const double t
        = integral(after->a0 - x0, false) - integral(after->a0 - x1, false);
    const double c
        = integral(after->a0 - x0, true) - integral(after->a0 - x1, true);
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

// One wire of the target, along axis `a`, with what lies around it.
class Wire
{
 public:
  Wire(const Shape& shape,
       const std::vector<Shape>& shapes,
       const Layers& layers,
       const Model& model,
       const ReferenceOptions& options)
      : shape_(shape),
        shapes_(shapes),
        layers_(layers),
        model_(model),
        options_(options),
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
    bool first = true;
    Cover first_below;
    Cover first_above;
    Cover last_below;
    Cover last_above;
    for (size_t i = 0; i + 1 < edges.size(); i++) {
      if (edges[i + 1] - edges[i] <= kEps) {
        continue;
      }
      out.pieces++;
      const Cover below = coverOn(true, edges[i], edges[i + 1]);
      const Cover above = coverOn(false, edges[i], edges[i + 1]);
      if (first) {
        first_below = below;
        first_above = above;
        first = false;
      }
      last_below = below;
      last_above = above;
      addPiece(edges[i], edges[i + 1], below, above, target, out);
    }
    addEnd(a0_, first_below, first_above, target, out);
    addEnd(a1_, last_below, last_above, target, out);
  }

 private:
  // Lateral candidates with their gap, cover runs by layer, and the cuts
  // where any of them starts or ends.
  void collect()
  {
    cuts_ = {a0_, a1_};
    for (const Shape& s : shapes_) {
      if (&s == &shape_) {
        continue;
      }
      const double o0 = std::max(a0_, s.lo[a_]);
      const double o1 = std::min(a1_, s.hi[a_]);
      if (o1 - o0 <= kEps) {
        continue;
      }
      if (s.layer == shape_.layer) {
        if (s.hi[p_] <= p0_ + kEps) {
          left_.push_back({s.net, p0_ - s.hi[p_], o0, o1});
        } else if (s.lo[p_] >= p1_ - kEps) {
          right_.push_back({s.net, s.lo[p_] - p1_, o0, o1});
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
          runs_[s.layer].push_back({s.net, o0, o1});
        }
      } else {
        continue;
      }
      cuts_.insert(o0);
      cuts_.insert(o1);
    }
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
  Cover coverOn(const bool below, const double x0, const double x1) const
  {
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
          return full;
        }
      }
      const Cover partial
          = partialCover(model_, {name_, layers_.names[l]}, it->second, x0, x1);
      if (partial.share > best.share) {
        best = partial;
      }
    }
    return best;
  }

  // The weighted cover states of a piece: ("", 1 - share) and (layer,
  // share) on each side.
  template <class F>
  void forStates(const Cover& below, const Cover& above, F&& f) const
  {
    for (const bool b : {false, true}) {
      const double wb = b ? below.share : 1 - below.share;
      if (wb == 0) {
        continue;
      }
      for (const bool t : {false, true}) {
        const double wt = t ? above.share : 1 - above.share;
        if (wt == 0) {
          continue;
        }
        LateralKey key{name_, b ? below.layer : "", t ? above.layer : ""};
        f(key, wb, wt, b, t);
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
    forStates(below,
              above,
              [&](LateralKey& key,
                  const double wb,
                  const double wt,
                  const bool b,
                  const bool t) {
                const LateralCap cap
                    = model_.lateral(fallback(model_, key), w_, sl, sr);
                total += wb * wt * cap.total();
                cl += wb * wt * cap.left;
                cr += wb * wt * cap.right;
                if (b && !key.below.empty()) {
                  plane_below += wt * cap.below;
                }
                if (t && !key.above.empty()) {
                  plane_above += wb * cap.above;
                }
              });
    const double to_below = below.cross * plane_below;
    const double to_above = above.cross * plane_above;
    out.ground += len * (total - cl - cr - to_below - to_above);
    addDiagonals(x0, x1, sl, sr, target, cl, cr, out);
    if (l != nullptr && l->net != target) {
      out.couplings[l->net] += len * cl;
    }
    if (r != nullptr && r->net != target) {
      out.couplings[r->net] += len * cr;
    }
    for (const auto& [net, share] : below.nets) {
      if (net != target) {
        out.couplings[net] += len * to_below * share;
      }
    }
    for (const auto& [net, share] : above.nets) {
      if (net != target) {
        out.couplings[net] += len * to_above * share;
      }
    }
  }

  // The nearest parallel wire on each nearby layer beside the piece, on
  // each side, where no run of that layer covers it, shielded by the
  // piece's neighbor on that side and on the other. One of the net's own
  // shields like any other but adds no coupling.
  void addDiagonals(const double x0,
                    const double x1,
                    const double sl,
                    const double sr,
                    const NetIndex target,
                    double& cl,
                    double& cr,
                    NetEstimate& out) const
  {
    const double len = x1 - x0;
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
        const DiagonalKey key{name_, layers_.names[k]};
        if (model_.diagonal(key) == nullptr) {
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
        const DiagonalCap d = model_.diagonal(key,
                                              best->hi[p_] - best->lo[p_],
                                              best_gap,
                                              spacing,
                                              right_side ? sl : sr);
        if (best->net != target) {
          out.couplings[best->net] += len * d.coupling;
        }
        out.ground += len * (d.total_delta - d.coupling - d.lateral_delta);
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
    out.ends++;
    forStates(
        below,
        above,
        [&](LateralKey& key, const double wb, const double wt, bool, bool) {
          fallback(model_, key);
          if (model_.end(key) == nullptr) {
            key = {name_, "", ""};
          }
          out.ground += wb * wt * model_.endExcess(key, w_);
        });
  }

  const Shape& shape_;
  const std::vector<Shape>& shapes_;
  const Layers& layers_;
  const Model& model_;
  const ReferenceOptions& options_;
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
  for (const Shape& shape : shapes) {
    if (shape.net != target || shape.layer < 0) {
      continue;
    }
    out.wires++;
    Wire(shape, shapes, layers, model, options).estimate(target, out);
  }
  return out;
}

}  // namespace pex
