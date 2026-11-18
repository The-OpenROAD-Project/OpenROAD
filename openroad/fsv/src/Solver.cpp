// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "fsv/Solver.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "CubeGreens.h"
#include "Dielectric.h"
#include "GaussianSurface.h"
#include "LayeredCubeGreens.h"
#include "LayeredSquareGreens.h"
#include "Philox.h"
#include "SpatialIndex.h"
#include "SquareGreens.h"
#include "WorkerPool.h"
#include "fsv/Geometry.h"
#include "fsv/Scene.h"
#include "fsv/Stack.h"

namespace fsv {

namespace {

// Vacuum permittivity in fF/µm.
constexpr double kEps0 = 8.8541878128e-3;

// A walk result that is not a column.
constexpr int kHopGuardHit = -1;

// Sums over one batch of walks; one column per net plus the ground plane.
// The all-walk sums feed the control variate (the weights average to zero).
struct Tally
{
  std::vector<double> sum;
  std::vector<double> sum2;
  double sum_all = 0;
  double sum2_all = 0;
  // |weight| of walks that reached outside the window and ended on a net.
  double left_sum = 0;
  int64_t hops = 0;
  int64_t guard_hits = 0;

  // The arrays are padded by a cache line: they are written on every walk,
  // and threads sharing a malloc arena could otherwise share a line.
  static constexpr int kPad = 64 / sizeof(double);

  explicit Tally(const int columns)
      : sum(columns + kPad, 0.0), sum2(columns + kPad, 0.0)
  {
  }

  void add(const Tally& other)
  {
    for (size_t i = 0; i < sum.size(); i++) {
      sum[i] += other.sum[i];
      sum2[i] += other.sum2[i];
    }
    sum_all += other.sum_all;
    sum2_all += other.sum2_all;
    left_sum += other.left_sum;
    hops += other.hops;
    guard_hits += other.guard_hits;
  }
};

// Estimate and standard error of column c. A walk's weight w lands in exactly
// one column, and E[w] = 0 because the first-hop kernel integrates to zero,
// so w * 1{c} - beta * w has the same mean for any beta. The variance is
// smallest at beta = E[w^2 1{c}] / E[w^2].
std::pair<double, double> estimate(const Tally& t, const int c, const double n)
{
  const double beta = t.sum2_all > 0 ? t.sum2[c] / t.sum2_all : 0.0;
  const double mean = (t.sum[c] - beta * t.sum_all) / n;
  const double second
      = (t.sum2[c] * (1 - 2 * beta) + beta * beta * t.sum2_all) / n;
  const double var = std::max(0.0, second - mean * mean);
  return {mean, std::sqrt(var / n)};
}

// Heights within this fraction of a cube's size count as on the interface.
constexpr double kOnInterface = 1e-9;

// The frame of a 3D solve: cubes. The two-dielectric tables put their
// interface normal to z; a cube whose interface is normal to world axis
// `normal` uses them through a cyclic relabelling: world `normal` is table
// z, the next axis is table x and the one after it table y.
struct Space
{
  static constexpr int kD = 3;
  using GeometryType = Geometry;
  using Uniform = CubeGreens;
  using Layered = LayeredCubeGreens;

  static const std::vector<Box>& shapes(const Geometry& g) { return g.boxes(); }
  static int shapeNet(const Geometry& g, const int i) { return g.boxNet(i); }
  static const AlignedBox<2>* window(const Geometry& g)
  {
    return g.window() ? &*g.window() : nullptr;
  }

  // The uniform tables' axis for a world axis.
  static Axis uniformAxis(const int world) { return static_cast<Axis>(world); }

  static Axis toTable(const int world, const int normal)
  {
    const int shift = (world - normal + 3) % 3;
    return shift == 0 ? Axis::kZ : (shift == 1 ? Axis::kX : Axis::kY);
  }
  // Whether a table axis is the interface normal.
  static bool isNormal(const Axis table) { return table == Axis::kZ; }

  static Vec3 toWorld(const Vec3& table, const int normal)
  {
    Vec3 world;
    world[normal] = table[2];
    world[(normal + 1) % 3] = table[0];
    world[(normal + 2) % 3] = table[1];
    return world;
  }
};

// The frame of a cross section: squares in the x-z plane. The square tables
// put the interface normal to z; an x-normal interface swaps x and z.
struct Plane
{
  static constexpr int kD = 2;
  using GeometryType = Geometry2D;
  using Uniform = SquareGreens;
  using Layered = LayeredSquareGreens;

  static const std::vector<Rect>& shapes(const Geometry2D& g)
  {
    return g.rects();
  }
  static int shapeNet(const Geometry2D& g, const int i) { return g.rectNet(i); }
  static const AlignedBox<2>* window(const Geometry2D&) { return nullptr; }

  static int uniformAxis(const int world) { return world; }

  static int toTable(const int world, const int normal)
  {
    return normal == 1 ? world : 1 - world;
  }
  static bool isNormal(const int table) { return table == 1; }

  static Vec2 toWorld(const Vec2& table, const int normal)
  {
    return normal == 1 ? table : Vec2{table[1], table[0]};
  }
};

// The cube (or square) one transition uses: its half-size and, when an
// interface plane lies inside it, the two-dielectric table and the plane's
// normal.
template <class Layered>
struct Cube
{
  double half_size;
  const Layered* layered = nullptr;
  int normal = 0;  // component index
};

template <class Layered>
using Tables = std::vector<std::shared_ptr<const Layered>>;

// The two-dielectric tables of one solve, for each ordered pair of the
// scene's permittivities, by interface position. A pair's tables are built
// the first time a walk needs them; `build` makes them for one ratio and
// must be called under `mutex`.
template <class Layered, int D>
class LayeredTableCache
{
 public:
  LayeredTableCache(const Dielectric<D>& dielectric,
                    std::function<const Tables<Layered>&(double ratio)> build,
                    std::mutex& mutex)
      : build_(std::move(build)), mutex_(mutex)
  {
    eps_values_.push_back(dielectric.baseEps());
    for (const Stack::PlanarInterface& i : dielectric.planar()) {
      eps_values_.push_back(i.eps_below);
      eps_values_.push_back(i.eps_above);
    }
    for (const Shell<D>& shell : dielectric.shells()) {
      eps_values_.push_back(shell.eps);
    }
    std::ranges::sort(eps_values_);
    eps_values_.erase(std::ranges::unique(eps_values_).begin(),
                      eps_values_.end());
    slots_ = std::vector<std::atomic<const Tables<Layered>*>>(
        eps_values_.size() * eps_values_.size());
  }

  // The table for the cube's plane at a tabulated position.
  const Layered* table(const typename Dielectric<D>::Cube& c,
                       const int position)
  {
    return (*tables(c.eps_minus,
                    c.eps_plus))[position + Layered::kPositions - 1]
        .get();
  }

 private:
  const Tables<Layered>* tables(const double eps_minus, const double eps_plus)
  {
    std::atomic<const Tables<Layered>*>& entry
        = slots_[slot(eps_minus) * eps_values_.size() + slot(eps_plus)];
    const Tables<Layered>* t = entry.load(std::memory_order_acquire);
    if (t == nullptr) {
      const std::lock_guard lock(mutex_);
      t = entry.load(std::memory_order_relaxed);
      if (t == nullptr) {
        t = &build_(eps_plus / eps_minus);
        entry.store(t, std::memory_order_release);
      }
    }
    return t;
  }

  size_t slot(const double eps) const
  {
    return static_cast<size_t>(std::ranges::lower_bound(eps_values_, eps)
                               - eps_values_.begin());
  }

  std::function<const Tables<Layered>&(double)> build_;
  std::mutex& mutex_;
  std::vector<double> eps_values_;
  std::vector<std::atomic<const Tables<Layered>*>> slots_;
};

// The walks of one solve, in a frame (Space or Plane).
template <class Frame>
class Walker
{
  static constexpr int D = Frame::kD;
  using Layered = typename Frame::Layered;
  using Point = Vec<D>;
  using DielectricCube = typename Dielectric<D>::Cube;

  // A face of the Gaussian surface: normal to `axis`, at its low or high
  // coordinate.
  struct Face
  {
    int axis;
    bool high;
  };

 public:
  Walker(const typename Frame::GeometryType& geometry,
         const SpatialIndex<D>& index,
         const Dielectric<D>& dielectric,
         const GaussianSurface<D>& gauss,
         const typename Frame::Uniform& greens,
         LayeredTableCache<Layered, D>& tables,
         const double capture,
         const int64_t hop_guard,
         const uint64_t key)
      : geometry_(geometry),
        window_(Frame::window(geometry)),
        index_(index),
        dielectric_(dielectric),
        gauss_(gauss),
        greens_(greens),
        tables_(tables),
        capture_(capture),
        hop_guard_(hop_guard),
        key_(key)
  {
  }

  // One walk: returns the column it ends in (the last is the ground
  // plane, which grounded nets join) or kHopGuardHit. `left` tells whether
  // a cube could have reached outside the geometry's window.
  int walk(const int64_t walk_id,
           double& weight,
           int64_t& hops,
           bool& left) const
  {
    Philox rng(key_, static_cast<uint64_t>(walk_id));
    Face face{};
    Point p;
    left = false;
    const double charge_scale = kEps0 * startPoint(rng, p, face);
    const double first_size = index_.nearest(p).dist;
    left = reaches(p, first_size);
    weight = charge_scale * firstHop(p, first_size, face, rng);
    // Transition hops until the walk is captured by a conductor.
    for (hops = 0; hops < hop_guard_; hops++) {
      const typename SpatialIndex<D>::Nearest n = index_.nearest(p);
      // A conductor outside the window could only matter within n.dist.
      left = left || reaches(p, n.dist);
      if (n.dist <= capture_) {
        if (n.box == SpatialIndex<D>::kGround) {
          return geometry_.netCount();
        }
        const int net = Frame::shapeNet(geometry_, n.box);
        return geometry_.grounded(net) ? geometry_.netCount() : net;
      }
      const Cube<Layered> cube = transition(p, n.dist, rng);
      const Point offset
          = cube.layered ? Frame::toWorld(cube.layered->sampleTransition(rng),
                                          cube.normal)
                         : greens_.sampleTransition(rng);
      for (int k = 0; k < D; k++) {
        p[k] += cube.half_size * offset[k];
      }
    }
    return kHopGuardHit;
  }

 private:
  // Whether the cube of half-size r at p reaches outside the window.
  bool reaches(const Point& p, const double r) const
  {
    return window_ != nullptr
           && (p[0] - r < window_->lo[0] || p[0] + r > window_->hi[0]
               || p[1] - r < window_->lo[1] || p[1] + r > window_->hi[1]);
  }

  // A point on the surface: a piece drawn by its start probability, then a
  // uniform point on it. Returns the piece's scale.
  double startPoint(Philox& rng, Point& p, Face& face) const
  {
    const double u = rng.uniform() * gauss_.cumulative.back();
    const int last = static_cast<int>(gauss_.cumulative.size()) - 1;
    const int i = std::min(
        last,
        static_cast<int>(std::ranges::upper_bound(gauss_.cumulative, u)
                         - gauss_.cumulative.begin()));
    const typename GaussianSurface<D>::Piece& piece = gauss_.pieces[i];
    face = {piece.axis, piece.high};
    const AlignedBox<D>& rect = piece.rect;
    p[face.axis] = rect.lo[face.axis];
    for (int t = 0; t < D; t++) {
      if (t != face.axis) {
        p[t] = rect.lo[t] + rng.uniform() * (rect.hi[t] - rect.lo[t]);
      }
    }
    return piece.scale;
  }

  // First hop from p on the surface's face: sample by |eps dP/dn|, since
  // Q = -flux of eps grad(phi) through the surface. Moves p and returns the
  // walk's weight over eps0 and the start piece's scale. The cube is never
  // shrunk for an interface: the weight scales as 1 / size, and shrinking near
  // interfaces gives it infinite variance. Instead, an interface at an
  // untabulated position is handled by the mixture of the two nearest tabulated
  // positions.
  double firstHop(Point& p,
                  const double max_half_size,
                  const Face face,
                  Philox& rng) const
  {
    const DielectricCube first = dielectric_.cube(p, max_half_size);
    const double eps_here = dielectric_.eps(p);
    BasicFirstHop<D> hop;
    double norm;
    const Layered* layered = nullptr;
    if (first.split) {
      layered = mixture(p, first, rng);
    }
    if (layered) {
      const auto axis = Frame::toTable(face.axis, first.axis);
      hop = layered->sampleFirstHop(axis, rng);
      hop.offset = Frame::toWorld(hop.offset, first.axis);
      norm = layered->fluxNorm(axis)
             * (Frame::isNormal(axis) ? first.eps_minus : eps_here);
    } else {
      hop = greens_.sampleFirstHop(Frame::uniformAxis(face.axis), rng);
      norm = eps_here * greens_.gradientNorm();
    }
    for (int k = 0; k < D; k++) {
      p[k] += first.half_size * hop.offset[k];
    }
    const double normal_sign = face.high ? 1.0 : -1.0;
    return -norm * normal_sign * hop.sign / first.half_size;
  }

  // A transition cube. When an interface plane falls inside, the cube is
  // shrunk so that the plane lands exactly on a tabulated position
  // k / kPositions (snapping), or, if that is impossible, stops at it. A
  // floored cube keeps its size and draws the position instead.
  Cube<Layered> transition(const Point& p,
                           const double max_half_size,
                           Philox& rng) const
  {
    const DielectricCube c = dielectric_.cube(p, max_half_size, capture_);
    if (!c.split) {
      return {c.half_size};
    }
    if (c.floored) {
      return {c.half_size, mixture(p, c, rng), c.axis};
    }
    constexpr int kK = Layered::kPositions;
    int position = 0;
    double h = c.half_size;
    if (c.distance > kOnInterface * c.half_size) {
      const int k = static_cast<int>(std::ceil(c.distance * kK / h));
      if (k >= kK) {
        return {c.distance};
      }
      h = c.distance * kK / k;
      position = c.coordinate > p[c.axis] ? k : -k;
    }
    return {h, tables_.table(c, position), c.axis};
  }

  // The table for a plane at an untabulated position inside a cube: one of
  // the two nearest tabulated positions, drawn with linear weights, which
  // reproduces the kernel interpolated in the position. Null when the draw
  // puts the plane on the cube's face, where the uniform table applies.
  const Layered* mixture(const Point& p,
                         const DielectricCube& c,
                         Philox& rng) const
  {
    constexpr int kK = Layered::kPositions;
    const double x = c.distance / c.half_size * kK;
    int k = static_cast<int>(x);
    if (rng.uniform() < x - k) {
      k++;
    }
    if (k >= kK) {
      return nullptr;
    }
    return tables_.table(c, c.coordinate > p[c.axis] ? k : -k);
  }

  const typename Frame::GeometryType& geometry_;
  const AlignedBox<2>* window_;  // x-y; null when there is none
  const SpatialIndex<D>& index_;
  const Dielectric<D>& dielectric_;
  const GaussianSurface<D>& gauss_;
  const typename Frame::Uniform& greens_;
  LayeredTableCache<Layered, D>& tables_;
  const double capture_;
  const int64_t hop_guard_;
  const uint64_t key_;
};

// Runs batches [first, first + count) of `batch_size` walks on the pool,
// one tally per batch. A worker cannot throw: the first failure (from
// building a table, say) is rethrown here after the round.
template <class W>
void runRound(WorkerPool& pool,
              const W& walker,
              const int batch_size,
              const int64_t first,
              const int count,
              const int columns,
              std::vector<Tally>& tallies)
{
  // Accumulates locally so threads do not share cache lines while walking.
  auto runBatch = [&](const int64_t batch, Tally& result) {
    Tally tally(columns);
    const int64_t first_walk = batch * batch_size;
    for (int64_t w = first_walk; w < first_walk + batch_size; w++) {
      double weight = 0;
      int64_t hops = 0;
      bool left = false;
      const int column = walker.walk(w, weight, hops, left);
      tally.hops += hops;
      if (column == kHopGuardHit) {
        tally.guard_hits++;
        continue;
      }
      // Walks that reached outside the window and came back to a conductor;
      // those that went on to the ground plane would at most end on a
      // conductor outside instead, which ground stands in for.
      if (left && column != columns - 1) {
        tally.left_sum += std::abs(weight);
      }
      tally.sum[column] += weight;
      tally.sum2[column] += weight * weight;
      tally.sum_all += weight;
      tally.sum2_all += weight * weight;
    }
    result = std::move(tally);
  };
  std::atomic<int> claim{0};
  std::exception_ptr failure;
  std::mutex failure_mutex;
  pool.run([&]() {
    try {
      for (int b = claim++; b < count; b = claim++) {
        runBatch(first + b, tallies[b]);
      }
    } catch (...) {
      const std::lock_guard lock(failure_mutex);
      if (!failure) {
        failure = std::current_exception();
      }
    }
  });
  if (failure) {
    std::rethrow_exception(failure);
  }
}

template <class GeometryType>
void checkSolveArguments(const GeometryType& geometry,
                         const int master,
                         const SolveOptions& options)
{
  if (master < 0 || master >= geometry.netCount()) {
    throw std::invalid_argument("fsv: master net " + std::to_string(master)
                                + " is out of range");
  }
  if (geometry.grounded(master)) {
    throw std::invalid_argument("fsv: master net " + geometry.netName(master)
                                + " is grounded");
  }
  if (options.hop_guard <= 0) {
    throw std::invalid_argument("fsv: hop_guard must be positive");
  }
  if (options.max_walks <= 0) {
    throw std::invalid_argument("fsv: max_walks must be positive");
  }
  if (!(options.gauss_offset > 0) || !std::isfinite(options.gauss_offset)) {
    throw std::invalid_argument("fsv: gauss_offset must be positive");
  }
  // The stopping test divides by the tolerances.
  if (!(options.tol_total > 0) || !std::isfinite(options.tol_total)) {
    throw std::invalid_argument("fsv: tol_total must be positive");
  }
  if (options.tol_coupling
      && (!(*options.tol_coupling > 0)
          || !std::isfinite(*options.tol_coupling))) {
    throw std::invalid_argument("fsv: tol_coupling must be positive");
  }
}

// The row's means and standard errors from the summed tallies.
void estimateRow(const Tally& total, const int nets, CapRow& row)
{
  const double n = static_cast<double>(row.walks);
  row.cap.assign(nets, 0.0);
  row.sigma.assign(nets, 0.0);
  for (int c = 0; c < nets; c++) {
    std::tie(row.cap[c], row.sigma[c]) = estimate(total, c, n);
  }
  std::tie(row.ground, row.ground_sigma) = estimate(total, nets, n);
  row.hops = total.hops;
  row.window_leakage = total.left_sum / n;
}

// The largest ratio of a standard error to its target: the total's to
// tol_total of the total, and each coupling's, the ground's included, to
// tol_coupling of the total. The row has converged when it is below 1.
double worstErrorRatio(const CapRow& row, const SolveOptions& options)
{
  const int nets = static_cast<int>(row.cap.size());
  const double c_total = row.cap[row.master];
  if (!(c_total > 0)) {
    return std::numeric_limits<double>::infinity();
  }
  double worst = row.sigma[row.master] / (options.tol_total * c_total);
  const double coupling_target
      = options.tol_coupling.value_or(options.tol_total) * c_total;
  for (int c = 0; c < nets; c++) {
    if (c != row.master) {
      worst = std::max(worst, row.sigma[c] / coupling_target);
    }
  }
  return std::max(worst, row.ground_sigma / coupling_target);
}

// The walks the row needs to meet its tolerances, or min_walks if more:
// standard errors fall as 1 / sqrt(walks).
int64_t walksNeeded(const CapRow& row, const SolveOptions& options)
{
  const double ratio = worstErrorRatio(row, options);
  const double walks
      = std::ceil(ratio * ratio * static_cast<double>(row.walks));
  // An infinite or NaN ratio (a non-positive total, or NaN weights) asks for
  // as many walks as max_walks allows.
  constexpr double kMost = 9.0e18;
  return std::max(options.min_walks,
                  static_cast<int64_t>(walks < kMost ? walks : kMost));
}

// Runs rounds of walks until the row meets its tolerances or max_walks.
// The worker pool runs one round of batches at a time.
template <class W, class GeometryType>
CapRow runRounds(WorkerPool& pool,
                 const W& walker,
                 const SolverOptions& options,
                 const SolveOptions& solve_options,
                 const GeometryType& geometry,
                 const int master)
{
  // The worker pool runs one round of batches at a time. Each batch has its
  // own tally and the tallies are reduced in batch order, so the sums do not
  // depend on the thread count. After a small first round, each round is
  // sized to the walks still needed (see walksNeeded) plus a margin, within
  // [first_round_batches, round_batches], so solves stop near their target
  // and long ones still give every thread work.
  const int nets = geometry.netCount();
  const int columns = nets + 1;  // last column is the ground plane
  CapRow row;
  row.master = master;
  Tally total(columns);
  std::vector<Tally> tallies(options.round_batches, Tally(columns));
  int64_t round_first = 0;
  int round_size = std::min(options.first_round_batches, options.round_batches);
  const int64_t max_batches
      = (solve_options.max_walks + options.batch_size - 1) / options.batch_size;
  while (true) {
    // The last round is trimmed so the walk count stops at max_walks.
    round_size = static_cast<int>(
        std::min<int64_t>(round_size, max_batches - round_first));
    std::fill(tallies.begin(), tallies.begin() + round_size, Tally(columns));
    runRound(pool,
             walker,
             options.batch_size,
             round_first,
             round_size,
             columns,
             tallies);
    for (int b = 0; b < round_size; b++) {
      total.add(tallies[b]);
    }
    round_first += round_size;
    row.walks = round_first * options.batch_size;
    estimateRow(total, nets, row);

    // A dropped walk would bias every column without showing up in sigma.
    if (!std::isfinite(row.cap[master])) {
      throw std::runtime_error(
          "fsv: walks of non-finite weight while solving net "
          + geometry.netName(master));
    }
    if (total.guard_hits > 0) {
      throw std::runtime_error(
          "fsv: " + std::to_string(total.guard_hits) + " walk(s) exceeded "
          + std::to_string(solve_options.hop_guard) + " hops while solving net "
          + geometry.netName(master)
          + "; raise SolveOptions::hop_guard or check the geometry");
    }
    if (row.walks >= solve_options.max_walks) {
      break;
    }
    if (row.walks >= solve_options.min_walks
        && worstErrorRatio(row, solve_options) < 1) {
      break;
    }
    // The estimate of the walks needed is itself noisy; the margin makes
    // falling just short, which costs another round, less likely.
    constexpr double kMargin = 1.05;
    const double missing
        = kMargin * static_cast<double>(walksNeeded(row, solve_options))
          - static_cast<double>(row.walks);
    const double batches = std::ceil(missing / options.batch_size);
    round_size = static_cast<int>(
        std::clamp(batches,
                   static_cast<double>(std::min(options.first_round_batches,
                                                options.round_batches)),
                   static_cast<double>(options.round_batches)));
  }
  return row;
}

// Each trial round in choosing a Gaussian surface: 8192 walks, in batches
// small enough to occupy many threads.
constexpr int kTrialBatches = 32;
constexpr int kTrialBatchSize = 256;
// The inside surface must cost at most this much of the other in its trial:
// a close call is not worth the noise in either cost.
constexpr double kInsideAdvantage = 0.8;
// Below this first-cube shrink on the clear surface (see GaussianSurface),
// the inside one has nothing to gain and is not tried.
constexpr double kShrinkToTry = 1.1;
// A solve allowed fewer than this many times the trial's walks (both
// surfaces) skips the trial, which would be a large part of its budget.
constexpr int64_t kTrialBudget = 8;
// These constants were tuned on the benchmark; see test/README.md, Surface
// placement.

// The hops a walker's solve would take, from one trial round of walks on
// stream `key`: the walks needed to meet the tolerances (see
// worstErrorRatio), held within [min_walks, max_walks] as the solve's are,
// times the hops per walk. With min_walks = max_walks only the hops per
// walk differ. A walk past the hop guard, or a surface with no finite
// estimate, makes it infinite.
template <class W, class GeometryType>
double trialCost(WorkerPool& pool,
                 const W& walker,
                 const SolveOptions& solve_options,
                 const GeometryType& geometry,
                 const int master)
{
  const int nets = geometry.netCount();
  const int columns = nets + 1;
  std::vector<Tally> tallies(kTrialBatches, Tally(columns));
  runRound(pool, walker, kTrialBatchSize, 0, kTrialBatches, columns, tallies);
  Tally total(columns);
  for (const Tally& tally : tallies) {
    total.add(tally);
  }
  if (total.guard_hits > 0) {
    return std::numeric_limits<double>::infinity();
  }
  CapRow row;
  row.master = master;
  row.walks = static_cast<int64_t>(kTrialBatches) * kTrialBatchSize;
  estimateRow(total, nets, row);
  const double ratio = worstErrorRatio(row, solve_options);
  if (!std::isfinite(ratio)) {
    // No estimate (a non-positive or non-finite total): never the cheaper.
    return std::numeric_limits<double>::infinity();
  }
  const double lo = static_cast<double>(solve_options.min_walks);
  const double hi = static_cast<double>(solve_options.max_walks);
  double walks = ratio * ratio * static_cast<double>(row.walks);
  // An infinite or NaN ratio runs to max_walks.
  walks = walks < hi ? std::max(walks, lo) : hi;
  return walks * static_cast<double>(row.hops) / static_cast<double>(row.walks);
}

// The two-dielectric tables for one permittivity ratio, by interface
// position, from a solver's cache. Cached only once every position is
// built, so a failure leaves no partial entry.
template <class Layered>
const Tables<Layered>& cachedTables(std::map<double, Tables<Layered>>& cache,
                                    const int panels,
                                    const double ratio)
{
  const auto cached = cache.find(ratio);
  if (cached != cache.end()) {
    return cached->second;
  }
  constexpr int kK = Layered::kPositions;
  Tables<Layered> tables;
  for (int position = 1 - kK; position < kK; position++) {
    tables.push_back(Layered::shared(panels, position, ratio));
  }
  return cache.emplace(ratio, std::move(tables)).first->second;
}

// One solve in a frame: the Gaussian surface, the walks and the rounds.
template <class Frame, class SceneType>
CapRow solveScene(const SceneType& scene,
                  const int master,
                  const SolverOptions& options,
                  const SolveOptions& solve_options,
                  const typename Frame::Uniform& greens,
                  std::map<double, Tables<typename Frame::Layered>>& cache,
                  std::mutex& tables_mutex,
                  WorkerPool& pool)
{
  constexpr int D = Frame::kD;
  using Layered = typename Frame::Layered;
  const auto& geometry = scene.geometry();
  checkSolveArguments(geometry, master, solve_options);
  auto surface = [&](const SurfacePlacement placement) {
    return makeGaussianSurface<D>(
        Frame::shapes(geometry),
        [&](const int b) { return Frame::shapeNet(geometry, b); },
        scene.index(),
        scene.dielectric(),
        master,
        solve_options.gauss_offset,
        placement);
  };
  const GaussianSurface<D> clear = surface(SurfacePlacement::kClearOfShells);
  LayeredTableCache<Layered, D> tables(
      scene.dielectric(),
      [&](const double ratio) -> const Tables<Layered>& {
        return cachedTables(cache, options.panels, ratio);
      },
      tables_mutex);
  const uint64_t key
      = options.seed
        ^ (0x9E3779B97F4A7C15ull * (static_cast<uint64_t>(master) + 1));
  auto walker = [&](const GaussianSurface<D>& gauss, const uint64_t k) {
    return Walker<Frame>(geometry,
                         scene.index(),
                         scene.dielectric(),
                         gauss,
                         greens,
                         tables,
                         solve_options.capture_rel * scene.minDimension(),
                         solve_options.hop_guard,
                         k);
  };
  // Inside the shells the first cubes meet fewer interfaces but are
  // smaller, which wins on some structures and loses on others; one trial
  // round on each surface picks the cheaper. The trial walks use their own
  // streams and are discarded, so the choice does not bias the solve.
  const GaussianSurface<D>* gauss = &clear;
  std::optional<GaussianSurface<D>> inside;
  constexpr int64_t kTrialWalks
      = 2 * static_cast<int64_t>(kTrialBatches) * kTrialBatchSize;
  if (clear.first_cube_shrink > kShrinkToTry
      && solve_options.max_walks >= kTrialBudget * kTrialWalks) {
    inside = surface(SurfacePlacement::kInsideShells);
  }
  int64_t trial_walks = 0;
  if (inside && inside->inside_shells) {
    const uint64_t trial_key = key ^ 0xD1B54A32D192ED03ull;
    const double clear_cost = trialCost(
        pool, walker(clear, trial_key), solve_options, geometry, master);
    const double inside_cost = trialCost(
        pool, walker(*inside, ~trial_key), solve_options, geometry, master);
    trial_walks = kTrialWalks;
    if (inside_cost < kInsideAdvantage * clear_cost) {
      gauss = &*inside;
    }
  }
  CapRow row = runRounds(
      pool, walker(*gauss, key), options, solve_options, geometry, master);
  row.inside_surface = gauss != &clear;
  row.trial_walks = trial_walks;
  return row;
}

// The panel count, checked before any table is built: every tabulated
// interface position must lie on a panel boundary.
int checkedPanels(const int panels)
{
  constexpr int kMultiple = 2 * LayeredCubeGreens::kPositions;
  if (panels <= 0 || panels % kMultiple != 0) {
    throw std::invalid_argument("fsv: panels must be a positive multiple of "
                                + std::to_string(kMultiple));
  }
  return panels;
}

}  // namespace

Solver::Solver(const SolverOptions& options)
    : options_(options), pool_(std::make_unique<WorkerPool>(options.threads))
{
  checkedPanels(options.panels);
}

Solver::~Solver() = default;

CapRow Solver::solve(const Scene& scene,
                     const int master,
                     const SolveOptions& solve_options)
{
  const std::lock_guard lock(solve_mutex_);
  if (!greens_) {
    greens_ = CubeGreens::shared(options_.panels);
  }
  return solveScene<Space>(scene,
                           master,
                           options_,
                           solve_options,
                           *greens_,
                           layered_,
                           tables_mutex_,
                           *pool_);
}

CapRow Solver::solve(const Scene2D& scene,
                     const int master,
                     const SolveOptions& solve_options)
{
  const std::lock_guard lock(solve_mutex_);
  if (!square_greens_) {
    square_greens_ = SquareGreens::shared(options_.panels);
  }
  return solveScene<Plane>(scene,
                           master,
                           options_,
                           solve_options,
                           *square_greens_,
                           layered_squares_,
                           tables_mutex_,
                           *pool_);
}

}  // namespace fsv
