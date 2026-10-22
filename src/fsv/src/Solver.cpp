// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "fsv/Solver.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "BoxFace.h"
#include "CubeGreens.h"
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
  const double offset = std::min(0.5 * gap, offset_frac * min_dim);

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

// The cube one transition uses: its half-size and, when a planar interface
// lies inside it, the two-dielectric tables (whose permittivities are in
// units of eps_below).
struct Cube
{
  double half_size;
  const LayeredCubeGreens* layered = nullptr;
};

// The largest cube allowed at a point, and the interface it may contain.
struct CubeLimit
{
  double half_size;     // at most the distance to the second-nearest interface
  int interface = -1;   // nearest interface, if strictly inside the cube
  double distance = 0;  // to that interface
};

// Heights within this fraction of a cube's size count as on the interface.
constexpr double kOnInterface = 1e-9;

// Sizes cubes against a scene's planar interfaces. A cube may contain at
// most one interface, so its half-size is at most the distance to the
// second-nearest one.
class InterfaceSizer
{
 public:
  InterfaceSizer(
      const Scene& scene,
      const std::vector<
          const std::vector<std::shared_ptr<const LayeredCubeGreens>>*>& tables)
      : interfaces_(scene.interfaces()), tables_(tables)
  {
  }

  CubeLimit limit(const double z, const double max_half_size) const
  {
    const int count = static_cast<int>(interfaces_.size());
    if (count == 0) {
      return {max_half_size};
    }
    // Nearest and second-nearest interfaces, walking out from z.
    int up = static_cast<int>(
        std::ranges::upper_bound(interfaces_, z, {}, &Stack::PlanarInterface::z)
        - interfaces_.begin());
    int down = up - 1;
    double d[2];
    int nearest = -1;
    for (int pass = 0; pass < 2; pass++) {
      const double da = up < count ? interfaces_[up].z - z
                                   : std::numeric_limits<double>::infinity();
      const double db = down >= 0 ? z - interfaces_[down].z
                                  : std::numeric_limits<double>::infinity();
      const int i = da <= db ? up++ : down--;
      d[pass] = std::min(da, db);
      if (pass == 0) {
        nearest = i;
      }
    }
    const double h = std::min(max_half_size, d[1]);
    if (d[0] >= h) {
      return {h};
    }
    return {h, nearest, d[0]};
  }

  // A transition cube. When the nearest interface falls inside, the cube is
  // shrunk so that the interface lands exactly on a tabulated position k /
  // kPositions (snapping), or, if that is impossible, stops at it.
  Cube transition(const double z, const double max_half_size) const
  {
    const CubeLimit lim = limit(z, max_half_size);
    if (lim.interface < 0) {
      return {lim.half_size};
    }
    constexpr int kK = LayeredCubeGreens::kPositions;
    int position = 0;
    double h = lim.half_size;
    if (lim.distance > kOnInterface * lim.half_size) {
      const int k = static_cast<int>(std::ceil(lim.distance * kK / h));
      if (k >= kK) {
        return {lim.distance};
      }
      h = lim.distance * kK / k;
      position = above(lim.interface, z) ? k : -k;
    }
    return {h, table(lim.interface, position)};
  }

  const Stack::PlanarInterface& interface(const int i) const
  {
    return interfaces_[i];
  }
  // The z of interface i is above height z.
  bool above(const int i, const double z) const { return interfaces_[i].z > z; }
  const LayeredCubeGreens* table(const int i, const int position) const
  {
    return (*tables_[i])[position + LayeredCubeGreens::kPositions - 1].get();
  }

 private:
  const std::vector<Stack::PlanarInterface>& interfaces_;
  const std::vector<
      const std::vector<std::shared_ptr<const LayeredCubeGreens>>*>& tables_;
};

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
  if (master < 0 || master >= geometry.netCount()) {
    throw std::invalid_argument("fsv: master net " + std::to_string(master)
                                + " is out of range");
  }
  if (solve_options.hop_guard <= 0) {
    throw std::invalid_argument("fsv: hop_guard must be positive");
  }
  if (solve_options.max_walks <= 0) {
    throw std::invalid_argument("fsv: max_walks must be positive");
  }
  const SpatialIndex& spatial_index = scene.index();
  const double capture = solve_options.capture_rel * scene.minDimension();
  const GaussianSurface gauss
      = makeGaussianSurface(geometry, master, solve_options.gauss_offset);
  const int nets = geometry.netCount();
  const int columns = nets + 1;  // last column is the ground plane
  const double charge_scale = kEps0 * gauss.area;
  // Two-dielectric tables for each planar interface, by position.
  std::vector<const std::vector<std::shared_ptr<const LayeredCubeGreens>>*>
      interface_tables;
  for (const Stack::PlanarInterface& i : scene.interfaces()) {
    interface_tables.push_back(&layeredTables(i.eps_above / i.eps_below));
  }
  const InterfaceSizer sizer(scene, interface_tables);
  const uint64_t key
      = options_.seed
        ^ (0x9E3779B97F4A7C15ull * (static_cast<uint64_t>(master) + 1));

  // One walk: returns the column it ends in, or -1 if it hit the hop guard.
  auto walk = [&](const int64_t walk_id, double& weight, int64_t& hops) {
    Philox rng(key, static_cast<uint64_t>(walk_id));
    // Start point: a face by area, then uniform on the face.
    double u = rng.uniform() * gauss.area;
    int f = 0;
    while (f < BoxFace::kCount - 1 && u >= gauss.face_area[f]) {
      u -= gauss.face_area[f];
      f++;
    }
    const BoxFace face = BoxFace::fromIndex(f);
    const int normal = index(face.axis);
    Vec3 p;
    p[normal] = face.high ? gauss.box.hi[normal] : gauss.box.lo[normal];
    const auto [t0, t1] = tangentAxes(face.axis);
    for (const int t : {index(t0), index(t1)}) {
      p[t] = gauss.box.lo[t]
             + rng.uniform() * (gauss.box.hi[t] - gauss.box.lo[t]);
    }

    // First hop: sample by |eps dP/dn|; Q = -flux of eps grad(phi) through
    // G. The cube is never shrunk for an interface: the weight scales as
    // 1 / size, and shrinking near interfaces gives it infinite variance.
    // Instead, an interface at an untabulated position t is handled by
    // sampling the two nearest tabulated positions with linear weights,
    // which reproduces the kernel interpolated in t.
    const CubeLimit first = sizer.limit(p[2], spatial_index.nearest(p).dist);
    const double eps_here = scene.epsAt(p[2]);
    FirstHop hop;
    double norm;
    const LayeredCubeGreens* layered = nullptr;
    if (first.interface >= 0) {
      constexpr int kK = LayeredCubeGreens::kPositions;
      const double x = first.distance / first.half_size * kK;
      int k = static_cast<int>(x);
      if (rng.uniform() < x - k) {
        k++;
      }
      // Position kK puts the interface on the face: a uniform cube.
      if (k < kK) {
        const bool up = sizer.above(first.interface, p[2]);
        layered = sizer.table(first.interface, up ? k : -k);
      }
    }
    if (layered) {
      hop = layered->sampleFirstHop(face.axis, rng);
      norm = layered->fluxNorm(face.axis)
             * (face.axis == Axis::kZ
                    ? sizer.interface(first.interface).eps_below
                    : eps_here);
    } else {
      hop = greens_->sampleFirstHop(face.axis, rng);
      norm = eps_here * greens_->gradientNorm();
    }
    weight
        = -charge_scale * norm * face.normalSign() * hop.sign / first.half_size;
    for (int k = 0; k < 3; k++) {
      p[k] += first.half_size * hop.offset[k];
    }

    // Transition hops until the walk is captured by a conductor.
    for (hops = 0; hops < solve_options.hop_guard; hops++) {
      const SpatialIndex::Nearest n = spatial_index.nearest(p);
      if (n.dist <= capture) {
        return n.box == SpatialIndex::kGround ? nets : geometry.boxNet(n.box);
      }
      const Cube cube = sizer.transition(p[2], n.dist);
      const Vec3 offset = cube.layered ? cube.layered->sampleTransition(rng)
                                       : greens_->sampleTransition(rng);
      for (int k = 0; k < 3; k++) {
        p[k] += cube.half_size * offset[k];
      }
    }
    return -1;
  };

  // Accumulates locally so threads do not share cache lines while walking.
  auto runBatch = [&](const int64_t batch, Tally& result) {
    Tally tally(columns);
    const int64_t first = batch * options_.batch_size;
    for (int64_t w = first; w < first + options_.batch_size; w++) {
      double weight = 0;
      int64_t hops = 0;
      const int column = walk(w, weight, hops);
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

  CapRow row;
  row.master = master;
  Tally total(columns);

  // The worker pool runs one round of batches at a time. Each batch has its
  // own tally and the tallies are reduced in batch order, so the sums do not
  // depend on the thread count. Rounds start small and double up to
  // round_batches, so loose tolerances stop near their target and long solves
  // still give every thread work.
  std::vector<Tally> tallies(options_.round_batches, Tally(columns));
  int64_t round_first = 0;
  int round_size
      = std::min(options_.first_round_batches, options_.round_batches);
  std::atomic<int> claim{0};
  const std::function<void()> work = [&]() {
    for (int b = claim++; b < round_size; b = claim++) {
      runBatch(round_first + b, tallies[b]);
    }
  };

  const int64_t max_batches
      = (solve_options.max_walks + options_.batch_size - 1)
        / options_.batch_size;
  while (true) {
    // The last round is trimmed so the walk count stops at max_walks.
    round_size = static_cast<int>(
        std::min<int64_t>(round_size, max_batches - round_first));
    std::fill(tallies.begin(), tallies.begin() + round_size, Tally(columns));
    claim = 0;
    pool_->run(work);
    for (int b = 0; b < round_size; b++) {
      total.add(tallies[b]);
    }
    round_first += round_size;
    row.walks = round_first * options_.batch_size;
    round_size = std::min(2 * round_size, options_.round_batches);

    // Mean and standard error per column.
    const double n = static_cast<double>(row.walks);
    row.cap.assign(nets, 0.0);
    row.sigma.assign(nets, 0.0);
    for (int c = 0; c < nets; c++) {
      std::tie(row.cap[c], row.sigma[c]) = estimate(total, c, n);
    }
    std::tie(row.ground, row.ground_sigma) = estimate(total, nets, n);
    row.hops = total.hops;

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
    if (row.walks < solve_options.min_walks) {
      continue;
    }
    const double c_total = row.cap[master];
    bool converged = row.sigma[master] < solve_options.tol_total * c_total;
    for (int c = 0; c < columns && converged; c++) {
      if (c == master) {
        continue;
      }
      const double cap = c < nets ? row.cap[c] : row.ground;
      const double sigma = c < nets ? row.sigma[c] : row.ground_sigma;
      if (std::abs(cap) >= solve_options.coupling_threshold * c_total) {
        converged = sigma < solve_options.tol_coupling * std::abs(cap);
      }
    }
    if (converged) {
      break;
    }
  }
  return row;
}

}  // namespace fsv
