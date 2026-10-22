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
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "BoxFace.h"
#include "CubeGreens.h"
#include "Dielectric.h"
#include "LayeredCubeGreens.h"
#include "Philox.h"
#include "SpatialIndex.h"
#include "WorkerPool.h"
#include "fsv/Geometry.h"
#include "fsv/Scene.h"
#include "fsv/Stack.h"

namespace fsv {

namespace {

// Vacuum permittivity in fF/µm.
constexpr double kEps0 = 8.8541878128e-3;

// Box around the master on which walks start; charge is the flux through it.
struct GaussianSurface
{
  Box box;
  std::array<double, BoxFace::kCount> face_area;  // by BoxFace::index()
  double area = 0;
};

// Sums over one batch of walks; one column per net plus the ground plane.
// The all-walk sums feed the control variate (the weights average to zero).
struct Tally
{
  std::vector<double> sum;
  std::vector<double> sum2;
  double sum_all = 0;
  double sum2_all = 0;
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

GaussianSurface makeGaussianSurface(const Geometry& geometry,
                                    const Dielectric& dielectric,
                                    const int master,
                                    const double offset_frac)
{
  const auto& boxes = geometry.boxes();
  Box bbox{};
  bool first = true;
  for (int b = 0; b < static_cast<int>(boxes.size()); b++) {
    if (geometry.boxNet(b) != master) {
      continue;
    }
    if (first) {
      bbox = boxes[b];
      first = false;
      continue;
    }
    for (int k = 0; k < 3; k++) {
      bbox.lo[k] = std::min(bbox.lo[k], boxes[b].lo[k]);
      bbox.hi[k] = std::max(bbox.hi[k], boxes[b].hi[k]);
    }
  }
  if (first) {
    throw std::invalid_argument("fsv: master net has no boxes");
  }
  // Offset: a fraction of the master's smallest dimension, at most half the
  // gap to the nearest other conductor or the ground plane.
  double gap = bbox.lo[2];
  for (int b = 0; b < static_cast<int>(boxes.size()); b++) {
    if (geometry.boxNet(b) != master) {
      gap = std::min(gap, linfGap(bbox, boxes[b]));
    }
  }
  if (gap <= 0) {
    throw std::invalid_argument(
        "fsv: another conductor touches the master's bounding box");
  }
  double min_dim = std::numeric_limits<double>::max();
  for (int k = 0; k < 3; k++) {
    min_dim = std::min(min_dim, bbox.hi[k] - bbox.lo[k]);
  }
  const double limit = 0.5 * gap;
  double offset = std::min(limit, offset_frac * min_dim);
  // Keep the surface clear of every shell face parallel to it nearby, so
  // that first hops near it never meet two interfaces at once (their
  // weight grows as the cube shrinks). Each face is an offset to avoid by a
  // clearance of a quarter of the offset. If the offset is too close to one,
  // move it to the nearest point clear of them all within (0, limit], or
  // into the middle of the widest gap between them.
  std::vector<double> faces;
  for (const Shell& shell : dielectric.shells()) {
    if (linfGap(bbox, shell.outer) >= limit) {
      continue;
    }
    for (int k = 0; k < 3; k++) {
      for (const double c : {shell.outer.lo[k], shell.outer.hi[k]}) {
        for (const double d : {bbox.lo[k] - c, c - bbox.hi[k]}) {
          if (d > 0 && d < limit) {
            faces.push_back(d);
          }
        }
      }
    }
  }
  std::ranges::sort(faces);
  // Moving the offset changes its clearance, so repeat until it holds.
  for (int attempt = 0; attempt < 4; attempt++) {
    const double clearance = 0.25 * offset;
    if (std::ranges::none_of(faces, [&](const double d) {
          return std::abs(d - offset) < clearance;
        })) {
      break;
    }
    // Gaps between consecutive faces, bounded by 0 and the limit, which are
    // not faces.
    double best = offset;
    double best_move = std::numeric_limits<double>::infinity();
    double widest = -1;
    double widest_mid = offset;
    for (size_t i = 0; i <= faces.size(); i++) {
      const double gap_lo = i == 0 ? 0 : faces[i - 1];
      const double gap_hi = i == faces.size() ? limit : faces[i];
      const double lo = i == 0 ? 0 : gap_lo + clearance;
      const double hi = i == faces.size() ? limit : gap_hi - clearance;
      if (gap_hi - gap_lo > widest) {
        widest = gap_hi - gap_lo;
        widest_mid = 0.5 * (gap_lo + gap_hi);
      }
      if (!(hi > lo)) {
        continue;
      }
      const double candidate = std::clamp(offset, lo, hi);
      if (candidate > 0 && std::abs(candidate - offset) < best_move) {
        best_move = std::abs(candidate - offset);
        best = candidate;
      }
    }
    offset = std::isfinite(best_move) ? best : widest_mid;
  }

  GaussianSurface g;
  for (int k = 0; k < 3; k++) {
    g.box.lo[k] = bbox.lo[k] - offset;
    g.box.hi[k] = bbox.hi[k] + offset;
  }
  for (const Axis axis : kAxes) {
    const auto [t0, t1] = tangentAxes(axis);
    const double a = (g.box.hi[index(t0)] - g.box.lo[index(t0)])
                     * (g.box.hi[index(t1)] - g.box.lo[index(t1)]);
    g.face_area[BoxFace{axis, false}.index()] = a;
    g.face_area[BoxFace{axis, true}.index()] = a;
    g.area += 2 * a;
  }
  return g;
}

// Heights within this fraction of a cube's size count as on the interface.
constexpr double kOnInterface = 1e-9;

// The two-dielectric tables put their interface normal to z. A cube whose
// interface is normal to world axis `normal` uses them through a cyclic
// relabelling: world `normal` is table z, the next axis is table x and the
// one after it table y.
Axis toTable(const Axis world, const Axis normal)
{
  const int shift = (index(world) - index(normal) + 3) % 3;
  return shift == 0 ? Axis::kZ : (shift == 1 ? Axis::kX : Axis::kY);
}

Vec3 toWorld(const Vec3& table, const Axis normal)
{
  const int n = index(normal);
  Vec3 world;
  world[n] = table[2];
  world[(n + 1) % 3] = table[0];
  world[(n + 2) % 3] = table[1];
  return world;
}

// The cube one transition uses: its half-size and, when an interface plane
// lies inside it, the two-dielectric table and the plane's normal.
struct Cube
{
  double half_size;
  const LayeredCubeGreens* layered = nullptr;
  Axis normal = Axis::kZ;
};

using Tables = std::vector<std::shared_ptr<const LayeredCubeGreens>>;

// The two-dielectric tables of one solve, for each ordered pair of the
// scene's permittivities, by interface position. A pair's tables are built
// the first time a walk needs them; `build` makes them for one ratio and
// must be called under `mutex`.
class LayeredTableCache
{
 public:
  LayeredTableCache(const Dielectric& dielectric,
                    std::function<const Tables&(double ratio)> build,
                    std::mutex& mutex)
      : build_(std::move(build)), mutex_(mutex)
  {
    eps_values_.push_back(dielectric.baseEps());
    for (const Stack::PlanarInterface& i : dielectric.planar()) {
      eps_values_.push_back(i.eps_below);
      eps_values_.push_back(i.eps_above);
    }
    for (const Shell& shell : dielectric.shells()) {
      eps_values_.push_back(shell.eps);
    }
    std::ranges::sort(eps_values_);
    eps_values_.erase(std::ranges::unique(eps_values_).begin(),
                      eps_values_.end());
    slots_ = std::vector<std::atomic<const Tables*>>(eps_values_.size()
                                                     * eps_values_.size());
  }

  // The table for the cube's plane at a tabulated position.
  const LayeredCubeGreens* table(const Dielectric::Cube& c, const int position)
  {
    return (*tables(c.eps_minus,
                    c.eps_plus))[position + LayeredCubeGreens::kPositions - 1]
        .get();
  }

 private:
  const Tables* tables(const double eps_minus, const double eps_plus)
  {
    std::atomic<const Tables*>& entry
        = slots_[slot(eps_minus) * eps_values_.size() + slot(eps_plus)];
    const Tables* t = entry.load(std::memory_order_acquire);
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

  std::function<const Tables&(double)> build_;
  std::mutex& mutex_;
  std::vector<double> eps_values_;
  std::vector<std::atomic<const Tables*>> slots_;
};

// The walks of one solve.
class Walker
{
 public:
  Walker(const Scene& scene,
         const GaussianSurface& gauss,
         const CubeGreens& greens,
         LayeredTableCache& tables,
         const double capture,
         const int64_t hop_guard,
         const uint64_t key)
      : geometry_(scene.geometry()),
        index_(scene.index()),
        dielectric_(scene.dielectric()),
        gauss_(gauss),
        greens_(greens),
        tables_(tables),
        capture_(capture),
        hop_guard_(hop_guard),
        key_(key),
        charge_scale_(kEps0 * gauss.area)
  {
  }

  // One walk: returns the column it ends in (the last is the ground
  // plane), or -1 if it hit the hop guard.
  int walk(const int64_t walk_id, double& weight, int64_t& hops) const
  {
    Philox rng(key_, static_cast<uint64_t>(walk_id));
    BoxFace face{};
    Vec3 p = startPoint(rng, face);
    weight = firstHop(p, face, rng);
    // Transition hops until the walk is captured by a conductor.
    for (hops = 0; hops < hop_guard_; hops++) {
      const SpatialIndex::Nearest n = index_.nearest(p);
      if (n.dist <= capture_) {
        return n.box == SpatialIndex::kGround ? geometry_.netCount()
                                              : geometry_.boxNet(n.box);
      }
      const Cube cube = transition(p, n.dist, rng);
      const Vec3 offset
          = cube.layered
                ? toWorld(cube.layered->sampleTransition(rng), cube.normal)
                : greens_.sampleTransition(rng);
      for (int k = 0; k < 3; k++) {
        p[k] += cube.half_size * offset[k];
      }
    }
    return -1;
  }

 private:
  // A point on the Gaussian surface: a face by area, then uniform on it.
  Vec3 startPoint(Philox& rng, BoxFace& face) const
  {
    double u = rng.uniform() * gauss_.area;
    int f = 0;
    while (f < BoxFace::kCount - 1 && u >= gauss_.face_area[f]) {
      u -= gauss_.face_area[f];
      f++;
    }
    face = BoxFace::fromIndex(f);
    const int normal = index(face.axis);
    Vec3 p;
    p[normal] = face.high ? gauss_.box.hi[normal] : gauss_.box.lo[normal];
    const auto [t0, t1] = tangentAxes(face.axis);
    for (const int t : {index(t0), index(t1)}) {
      p[t] = gauss_.box.lo[t]
             + rng.uniform() * (gauss_.box.hi[t] - gauss_.box.lo[t]);
    }
    return p;
  }

  // First hop from p on the surface's face: sample by |eps dP/dn|, since
  // Q = -flux of eps grad(phi) through the surface. Moves p and returns the
  // walk's weight. The cube is never shrunk for an interface: the weight
  // scales as 1 / size, and shrinking near interfaces gives it infinite
  // variance. Instead, an interface at an untabulated position is handled
  // by the mixture of the two nearest tabulated positions.
  double firstHop(Vec3& p, const BoxFace face, Philox& rng) const
  {
    const Dielectric::Cube first = dielectric_.cube(p, index_.nearest(p).dist);
    const double eps_here = dielectric_.eps(p);
    FirstHop hop;
    double norm;
    const LayeredCubeGreens* layered = nullptr;
    if (first.split) {
      layered = mixture(p, first, rng);
    }
    if (layered) {
      const Axis axis = toTable(face.axis, first.axis);
      hop = layered->sampleFirstHop(axis, rng);
      hop.offset = toWorld(hop.offset, first.axis);
      norm = layered->fluxNorm(axis)
             * (axis == Axis::kZ ? first.eps_minus : eps_here);
    } else {
      hop = greens_.sampleFirstHop(face.axis, rng);
      norm = eps_here * greens_.gradientNorm();
    }
    for (int k = 0; k < 3; k++) {
      p[k] += first.half_size * hop.offset[k];
    }
    return -charge_scale_ * norm * face.normalSign() * hop.sign
           / first.half_size;
  }

  // A transition cube. When an interface plane falls inside, the cube is
  // shrunk so that the plane lands exactly on a tabulated position
  // k / kPositions (snapping), or, if that is impossible, stops at it. A
  // floored cube keeps its size and draws the position instead.
  Cube transition(const Vec3& p, const double max_half_size, Philox& rng) const
  {
    const Dielectric::Cube c = dielectric_.cube(p, max_half_size, capture_);
    if (!c.split) {
      return {c.half_size};
    }
    if (c.floored) {
      return {c.half_size, mixture(p, c, rng), c.axis};
    }
    constexpr int kK = LayeredCubeGreens::kPositions;
    int position = 0;
    double h = c.half_size;
    if (c.distance > kOnInterface * c.half_size) {
      const int k = static_cast<int>(std::ceil(c.distance * kK / h));
      if (k >= kK) {
        return {c.distance};
      }
      h = c.distance * kK / k;
      position = c.coordinate > p[index(c.axis)] ? k : -k;
    }
    return {h, tables_.table(c, position), c.axis};
  }

  // The table for a plane at an untabulated position inside a cube: one of
  // the two nearest tabulated positions, drawn with linear weights, which
  // reproduces the kernel interpolated in the position. Null when the draw
  // puts the plane on the cube's face, where the uniform table applies.
  const LayeredCubeGreens* mixture(const Vec3& p,
                                   const Dielectric::Cube& c,
                                   Philox& rng) const
  {
    constexpr int kK = LayeredCubeGreens::kPositions;
    const double x = c.distance / c.half_size * kK;
    int k = static_cast<int>(x);
    if (rng.uniform() < x - k) {
      k++;
    }
    if (k >= kK) {
      return nullptr;
    }
    return tables_.table(c, c.coordinate > p[index(c.axis)] ? k : -k);
  }

  const Geometry& geometry_;
  const SpatialIndex& index_;
  const Dielectric& dielectric_;
  const GaussianSurface& gauss_;
  const CubeGreens& greens_;
  LayeredTableCache& tables_;
  const double capture_;
  const int64_t hop_guard_;
  const uint64_t key_;
  const double charge_scale_;
};

// Runs batches [first, first + count) of `batch_size` walks on the pool,
// one tally per batch. A worker cannot throw: the first failure (from
// building a table, say) is rethrown here after the round.
void runRound(WorkerPool& pool,
              const Walker& walker,
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
      const int column = walker.walk(w, weight, hops);
      tally.hops += hops;
      if (column < 0) {
        tally.guard_hits++;
        continue;
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

void checkSolveArguments(const Geometry& geometry,
                         const int master,
                         const SolveOptions& options)
{
  if (master < 0 || master >= geometry.netCount()) {
    throw std::invalid_argument("fsv: master net " + std::to_string(master)
                                + " is out of range");
  }
  if (options.hop_guard <= 0) {
    throw std::invalid_argument("fsv: hop_guard must be positive");
  }
  if (options.max_walks <= 0) {
    throw std::invalid_argument("fsv: max_walks must be positive");
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
}

// Whether the total and every coupling of at least coupling_threshold of
// the total meet their tolerances.
bool converged(const CapRow& row, const SolveOptions& options)
{
  const int nets = static_cast<int>(row.cap.size());
  const double c_total = row.cap[row.master];
  bool ok = row.sigma[row.master] < options.tol_total * c_total;
  for (int c = 0; c <= nets && ok; c++) {
    if (c == row.master) {
      continue;
    }
    const double cap = c < nets ? row.cap[c] : row.ground;
    const double sigma = c < nets ? row.sigma[c] : row.ground_sigma;
    if (std::abs(cap) >= options.coupling_threshold * c_total) {
      ok = sigma < options.tol_coupling * std::abs(cap);
    }
  }
  return ok;
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
    : options_(options),
      greens_(CubeGreens::shared(checkedPanels(options.panels))),
      pool_(std::make_unique<WorkerPool>(options.threads))
{
}

Solver::~Solver() = default;

const std::vector<std::shared_ptr<const LayeredCubeGreens>>&
Solver::layeredTables(const double ratio)
{
  const auto cached = layered_.find(ratio);
  if (cached != layered_.end()) {
    return cached->second;
  }
  // Cached only once every position is built, so a failure leaves no
  // partial entry.
  constexpr int kK = LayeredCubeGreens::kPositions;
  std::vector<std::shared_ptr<const LayeredCubeGreens>> tables;
  for (int position = 1 - kK; position < kK; position++) {
    tables.push_back(
        LayeredCubeGreens::shared(options_.panels, position, ratio));
  }
  return layered_.emplace(ratio, std::move(tables)).first->second;
}

CapRow Solver::solve(const Scene& scene,
                     const int master,
                     const SolveOptions& solve_options)
{
  const std::lock_guard lock(solve_mutex_);
  const Geometry& geometry = scene.geometry();
  checkSolveArguments(geometry, master, solve_options);
  const GaussianSurface gauss = makeGaussianSurface(
      geometry, scene.dielectric(), master, solve_options.gauss_offset);
  LayeredTableCache tables(
      scene.dielectric(),
      [this](const double ratio) -> const Tables& {
        return layeredTables(ratio);
      },
      tables_mutex_);
  const uint64_t key
      = options_.seed
        ^ (0x9E3779B97F4A7C15ull * (static_cast<uint64_t>(master) + 1));
  const Walker walker(scene,
                      gauss,
                      *greens_,
                      tables,
                      solve_options.capture_rel * scene.minDimension(),
                      solve_options.hop_guard,
                      key);

  // The worker pool runs one round of batches at a time. Each batch has its
  // own tally and the tallies are reduced in batch order, so the sums do not
  // depend on the thread count. Rounds start small and double up to
  // round_batches, so loose tolerances stop near their target and long solves
  // still give every thread work.
  const int nets = geometry.netCount();
  const int columns = nets + 1;  // last column is the ground plane
  CapRow row;
  row.master = master;
  Tally total(columns);
  std::vector<Tally> tallies(options_.round_batches, Tally(columns));
  int64_t round_first = 0;
  int round_size
      = std::min(options_.first_round_batches, options_.round_batches);
  const int64_t max_batches
      = (solve_options.max_walks + options_.batch_size - 1)
        / options_.batch_size;
  while (true) {
    // The last round is trimmed so the walk count stops at max_walks.
    round_size = static_cast<int>(
        std::min<int64_t>(round_size, max_batches - round_first));
    std::fill(tallies.begin(), tallies.begin() + round_size, Tally(columns));
    runRound(*pool_,
             walker,
             options_.batch_size,
             round_first,
             round_size,
             columns,
             tallies);
    for (int b = 0; b < round_size; b++) {
      total.add(tallies[b]);
    }
    round_first += round_size;
    row.walks = round_first * options_.batch_size;
    round_size = std::min(2 * round_size, options_.round_batches);
    estimateRow(total, nets, row);

    // A dropped walk would bias every column without showing up in sigma.
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
    if (row.walks >= solve_options.min_walks && converged(row, solve_options)) {
      break;
    }
  }
  return row;
}

}  // namespace fsv
