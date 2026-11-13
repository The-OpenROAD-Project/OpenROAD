// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "pex/ModelBuilder.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "fsv/Geometry.h"
#include "fsv/Scene.h"
#include "fsv/Solver.h"
#include "fsv/Stack.h"
#include "pex/Model.h"
#include "pex/Patterns.h"
#include "utl/ThreadPool.h"

namespace pex {

namespace {

// `grid` extended past its last value to `reach` in steps of at most the
// ratio `step`, evenly in the logarithm.
void extendGrid(std::vector<double>& grid,
                const double reach,
                const double step)
{
  if (!(step > 1) || !std::isfinite(step)) {
    throw std::invalid_argument("a reach step must be finite and above 1");
  }
  if (grid.empty() || grid.back() <= 0 || reach <= grid.back() * 1.05) {
    return;
  }
  const double last = grid.back();
  const int n = static_cast<int>(
      std::ceil(std::log(reach / last) / std::log(step) - 1e-9));
  for (int i = 1; i <= n; i++) {
    grid.push_back(last * std::pow(reach / last, static_cast<double>(i) / n));
  }
}

// A reach option must be finite and not negative; 0 adds no grid points.
void requireReach(const char* name, const double reach)
{
  if (!(reach >= 0) || !std::isfinite(reach)) {
    throw std::invalid_argument(std::string(name)
                                + " must be finite and not negative");
  }
}

// A conductor a wire can be on: the stack's, with a minimum width and
// spacing, and off the ground plane.
const fsv::Stack::Conductor& requireConductor(const fsv::Stack& stack,
                                              const Layer layer)
{
  const fsv::Stack::Conductor& conductor = stack.conductor(layer);
  if (conductor.min_width <= 0 || conductor.min_spacing <= 0) {
    throw std::invalid_argument("conductor " + conductor.name
                                + " needs a minimum width and spacing");
  }
  if (conductor.bottom <= 0) {
    throw std::invalid_argument("conductor " + conductor.name
                                + " sits on the ground plane");
  }
  return conductor;
}

// The options' layers, when given, must all be in the stack: a misspelled
// name would otherwise drop a layer silently.
void requireLayers(const fsv::Stack& stack, const BuildOptions& options)
{
  for (const std::string& name : options.layers) {
    if (!stack.isConductor(stack.layer(name))) {
      throw std::invalid_argument("the stack has no conductor " + name);
    }
  }
}

// Whether a layer takes part: not on the ground plane, and among the
// options' layers when those are given.
bool usable(const fsv::Stack& stack,
            const BuildOptions& options,
            const Layer layer)
{
  const fsv::Stack::Conductor& conductor = stack.conductor(layer);
  if (conductor.bottom <= 0) {
    return false;
  }
  return options.layers.empty()
         || std::find(
                options.layers.begin(), options.layers.end(), conductor.name)
                != options.layers.end();
}

// The covers by depth and far factors.
std::vector<Layer> fixedCovers(const fsv::Stack& stack,
                               const BuildOptions& options,
                               const Layer layer,
                               const std::vector<Layer>& layers,
                               const int depth)
{
  requireLayers(stack, options);
  std::vector<Layer> out{Layer()};
  std::vector<Layer> far;  // usable layers past the depth, nearest first
  for (const Layer cover : layers) {
    if (!usable(stack, options, cover)) {
      continue;
    }
    if (static_cast<int>(out.size()) <= depth) {
      out.push_back(cover);
    } else {
      far.push_back(cover);
    }
  }
  if (far.empty() || options.far_cover_factors.empty()) {
    return out;
  }
  // The far layers nearest in distance to the factors' targets, and the
  // farthest, each once, nearest first.
  const double first = coverDistance(stack, layer, far.front());
  std::vector<Layer> picked{far.back()};
  for (const double factor : options.far_cover_factors) {
    const double target = logCoverDistance(first) + std::log(factor);
    Layer best;
    double best_error = 0;
    for (const Layer cover : far) {
      const double error = std::abs(
          logCoverDistance(coverDistance(stack, layer, cover)) - target);
      if (!best.valid() || error < best_error) {
        best = cover;
        best_error = error;
      }
    }
    if (std::find(picked.begin(), picked.end(), best) == picked.end()) {
      picked.push_back(best);
    }
  }
  for (const Layer cover : far) {
    if (std::find(picked.begin(), picked.end(), cover) != picked.end()) {
      out.push_back(cover);
    }
  }
  return out;
}

// The name a model key holds for a layer: "" for none (the substrate
// below, open above).
std::string keyName(const fsv::Stack& stack, const Layer layer)
{
  return layer.valid() ? stack.name(layer) : std::string();
}

// A table the model holds already is reused by a resumed build, as long
// as it is on the grid the options would build; one on another grid came
// from another build and would make the model inconsistent.
// Grids are compared at the model file's precision (seven significant
// digits), so a table read back from a saved model matches the grid the
// same options compute.
void requireGrid(const std::string& what,
                 const std::vector<double>& have,
                 const std::vector<double>& want)
{
  bool same = have.size() == want.size();
  for (size_t i = 0; same && i < have.size(); i++) {
    same = std::abs(have[i] - want[i])
           <= 1e-6 * std::max(std::abs(have[i]), std::abs(want[i]));
  }
  if (!same) {
    throw std::invalid_argument("the model holds a " + what
                                + " built on another grid; a build resumes "
                                  "only from a model built with the same "
                                  "options");
  }
}

// The reach factors ascend strictly from 0, so the reach grid starts at a
// crossing ending flush with the wire.
void requireReachFactors(const BuildOptions& options)
{
  const std::vector<double>& factors = options.reach_factors;
  if (factors.empty() || factors.front() != 0
      || std::adjacent_find(
             factors.begin(), factors.end(), std::greater_equal<>())
             != factors.end()
      || !std::isfinite(factors.back())) {
    throw std::invalid_argument(
        "the reach factors must ascend from 0 and be finite");
  }
}

fsv::SolveOptions solveOptions(const double tol)
{
  fsv::SolveOptions options;
  options.tol_total = tol;
  options.tol_coupling = tol;
  return options;
}

// A single-threaded solver seeded for one job, so a job's result does not
// depend on which thread runs it.
fsv::Solver jobSolver(const BuildOptions& options, const int job)
{
  fsv::SolverOptions solver_options;
  solver_options.threads = 1;
  solver_options.seed
      = options.seed ^ (0x9E3779B97F4A7C15ull * static_cast<uint64_t>(job + 1));
  return fsv::Solver(solver_options);
}

// Runs `count` jobs on the options' threads, reporting progress.
void runJobs(const BuildOptions& options,
             const int count,
             const std::function<void(int job, fsv::Solver& solver)>& run)
{
  if (count == 0) {
    return;  // a resumed build with nothing left
  }
  int done = 0;
  std::mutex progress_mutex;
  // The caller only waits, so every requested thread is a worker.
  utl::ThreadPool pool(std::max(1, options.threads));
  std::vector<int> jobs(count);
  for (int i = 0; i < count; i++) {
    jobs[i] = i;
  }
  pool.parallelFor(jobs, [&](const int job) {
    fsv::Solver solver = jobSolver(options, job);
    run(job, solver);
    if (options.progress) {
      const std::lock_guard lock(progress_mutex);
      options.progress(++done, count);
    }
  });
}

// The per-unit-length capacitances of the lateral pattern: a wire of the
// layer at a width, its neighbors at spacings (none when negative), and a
// plane on each cover (none when no layer).
LateralCap solveSection(const fsv::Stack& stack,
                        const Layer layer,
                        const double width,
                        const double left,
                        const double right,
                        const Layer below,
                        const Layer above,
                        const BuildOptions& options,
                        fsv::Solver& solver,
                        const double tol)
{
  LateralPattern pattern;
  pattern.layer = layer;
  pattern.width = width;
  pattern.left = left;
  pattern.right = right;
  pattern.below = below;
  pattern.above = above;
  pattern.extent = options.extent;
  const LateralPattern::Section section = pattern.build(stack);
  const fsv::Scene2D scene(section.geometry, stack);
  const fsv::CapRow row = solver.solve(scene, section.wire, solveOptions(tol));
  auto coupling = [&](const NetIndex net) {
    return net != kNullNetIndex ? row.coupling(net) : 0.0;
  };
  LateralCap cap;
  cap.left = coupling(section.left);
  cap.right = coupling(section.right);
  cap.below = coupling(section.below);
  cap.above = coupling(section.above);
  cap.ground = row.total() - cap.left - cap.right - cap.below - cap.above;
  return cap;
}

}  // namespace

std::string buildFingerprint(const fsv::Stack& stack,
                             const BuildOptions& options)
{
  // A canonical text of the inputs, hashed.
  std::ostringstream text;
  text.precision(17);
  text << stack.technology() << '|' << stack.backgroundEps() << '|';
  for (const std::string& name : options.layers) {
    text << 'l' << name << '|';
  }
  for (const fsv::Stack::Dielectric& d : stack.dielectrics()) {
    text << 'd' << d.name << ' ' << d.bottom << ' ' << d.top << ' ' << d.eps
         << '|';
  }
  for (const fsv::Stack::Conductor& c : stack.conductors()) {
    text << 'c' << c.name << ' ' << c.bottom << ' ' << c.top << ' '
         << c.min_width << ' ' << c.min_spacing;
    for (const fsv::Stack::ConformalDielectric& f : c.conformals) {
      text << " [" << f.name << ' ' << f.side_thickness << ' '
           << f.top_thickness << ' ' << f.eps << ']';
    }
    text << '|';
  }
  for (const fsv::Stack::Via& v : stack.vias()) {
    text << 'v' << v.name << ' ' << stack.name(v.upper) << ' '
         << (v.lower.valid() ? stack.name(v.lower) : "SUBSTRATE") << '|';
  }
  auto list = [&](const char* name, const std::vector<double>& values) {
    text << name;
    for (const double v : values) {
      text << ' ' << v;
    }
    text << '|';
  };
  list("w", options.width_factors);
  list("s", options.spacing_factors);
  text << "lr " << options.lateral_reach << ' '
       << options.lateral_reach_one_sided << ' ' << options.lateral_reach_step
       << " dr " << options.diagonal_reach << '|';
  list("rf", options.reach_factors);
  list("cw", options.crossing_width_factors);
  list("cf", options.coverages);
  list("dw", options.diagonal_width_factors);
  list("dg", options.diagonal_gap_factors);
  list("ds", options.diagonal_spacing_factors);
  list("df", options.diagonal_far_spacing_factors);
  list("xs", options.crossing_spacing_factors);
  list("es", options.end_spacing_factors);
  list("ff", options.facing_factors);
  // Only the cover settings in use: the tolerance, or the depths and far
  // factors.
  if (options.cover_tolerance > 0) {
    text << 'c' << options.cover_tolerance << '|';
  } else {
    list("fc", options.far_cover_factors);
    text << 'd' << options.below_depth << ' ' << options.above_depth << '|';
  }
  text << options.diagonal_depth << ' ' << options.crossing_length << ' '
       << options.crossing_length_long << ' ' << options.end_length << ' '
       << options.tol << ' ' << options.seed << ' ' << options.extent;
  // FNV-1a, so the fingerprint is the same on every platform and standard
  // library; std::hash is not.
  uint64_t hash = 0xcbf29ce484222325ull;
  for (const unsigned char c : text.str()) {
    hash = (hash ^ c) * 0x100000001b3ull;
  }
  std::ostringstream hex;
  hex << std::hex << hash;
  return hex.str();
}

std::vector<int> pickCovers(const std::vector<double>& distances,
                            const std::vector<ProbeCurve>& curves,
                            const double tol)
{
  const int n = static_cast<int>(distances.size());
  if (n <= 2) {
    std::vector<int> all(n);
    for (int i = 0; i < n; i++) {
      all[i] = i;
    }
    return all;
  }
  std::vector<double> logs(n);
  for (int i = 0; i < n; i++) {
    logs[i] = logCoverDistance(distances[i]);
  }
  // Whether consecutive picks i and j predict every candidate between.
  auto spans = [&](const int i, const int j) {
    for (int k = i + 1; k < j; k++) {
      const double t
          = logs[j] > logs[i] ? (logs[k] - logs[i]) / (logs[j] - logs[i]) : 0;
      for (const ProbeCurve& c : curves) {
        const double predicted = (1 - t) * c.value[i] + t * c.value[j];
        if (std::abs(predicted - c.value[k]) > tol * c.scale[k]) {
          return false;
        }
      }
    }
    return true;
  };
  // The fewest picks reaching each candidate from the first.
  std::vector<int> count(n, n + 1);
  std::vector<int> before(n, -1);
  count[0] = 1;
  for (int j = 1; j < n; j++) {
    for (int i = 0; i < j; i++) {
      if (count[i] + 1 < count[j] && spans(i, j)) {
        count[j] = count[i] + 1;
        before[j] = i;
      }
    }
  }
  std::vector<int> picks;
  for (int j = n - 1; j >= 0; j = before[j]) {
    picks.push_back(j);
  }
  std::reverse(picks.begin(), picks.end());
  return picks;
}

CoverPlan planCovers(const fsv::Stack& stack,
                     const Layer layer,
                     const BuildOptions& options)
{
  if (!(options.cover_tolerance >= 0)
      || !std::isfinite(options.cover_tolerance)) {
    throw std::invalid_argument(
        "the cover tolerance must be finite and not negative");
  }
  for (const double factor : options.far_cover_factors) {
    if (!(factor > 0) || !std::isfinite(factor)) {
      throw std::invalid_argument("the far cover factor "
                                  + std::to_string(factor)
                                  + " is not positive");
    }
  }
  if (options.cover_tolerance == 0) {
    return {fixedCovers(stack,
                        options,
                        layer,
                        layersBelow(stack, layer),
                        options.below_depth),
            fixedCovers(stack,
                        options,
                        layer,
                        layersAbove(stack, layer),
                        options.above_depth)};
  }
  const fsv::Stack::Conductor& conductor = requireConductor(stack, layer);
  requireLayers(stack, options);
  // Every usable layer on each side, nearest first.
  std::vector<Layer> sides[2];
  for (const int side : {0, 1}) {
    for (const Layer cover :
         side == 0 ? layersBelow(stack, layer) : layersAbove(stack, layer)) {
      if (usable(stack, options, cover)) {
        sides[side].push_back(cover);
      }
    }
  }
  // The probe variants: the grid's narrowest and widest width, neighbors
  // or none, and the other side open or with its nearest cover (when it
  // has one).
  const auto [narrowest, widest] = std::minmax_element(
      options.width_factors.begin(), options.width_factors.end());
  if (narrowest == options.width_factors.end()) {
    throw std::invalid_argument("the width grid is empty");
  }
  struct Variant
  {
    double width;
    double spacing;  // to both neighbors; none when negative
    bool other;      // the other side's nearest cover present
  };
  auto variants = [&](const int side) {
    std::vector<Variant> out;
    std::vector<double> factors{*narrowest};
    if (*widest != *narrowest) {
      factors.push_back(*widest);
    }
    for (const double f : factors) {
      for (const double spacing : {-1.0, conductor.min_spacing}) {
        for (const bool other : {false, true}) {
          if (!other || !sides[1 - side].empty()) {
            out.push_back({f * conductor.min_width, spacing, other});
          }
        }
      }
    }
    return out;
  };
  struct Job
  {
    int side;
    int cover;
    Variant variant;
    int index;  // the variant's
    LateralCap cap;
  };
  std::vector<Job> jobs;
  for (const int side : {0, 1}) {
    if (sides[side].size() <= 2) {
      continue;  // all of them are picked
    }
    const std::vector<Variant> list = variants(side);
    for (int c = 0; c < static_cast<int>(sides[side].size()); c++) {
      for (int v = 0; v < static_cast<int>(list.size()); v++) {
        jobs.push_back({side, c, list[v], v, {}});
      }
    }
  }
  runJobs(options, jobs.size(), [&](const int i, fsv::Solver& solver) {
    Job& job = jobs[i];
    const Layer cover = sides[job.side][job.cover];
    const Layer opposite
        = job.variant.other ? sides[1 - job.side].front() : Layer();
    job.cap = solveSection(stack,
                           layer,
                           job.variant.width,
                           job.variant.spacing,
                           job.variant.spacing,
                           job.side == 0 ? cover : opposite,
                           job.side == 0 ? opposite : cover,
                           options,
                           solver,
                           options.tol / 3);
  });
  CoverPlan plan{{Layer()}, {Layer()}};
  for (const int side : {0, 1}) {
    const std::vector<Layer>& candidates = sides[side];
    std::vector<Layer>& picked = side == 0 ? plan.below : plan.above;
    if (candidates.size() <= 2) {
      // Nothing was probed: the nearest and the farthest are all there is.
      picked.insert(picked.end(), candidates.begin(), candidates.end());
      continue;
    }
    std::vector<double> distances;
    for (const Layer cover : candidates) {
      distances.push_back(coverDistance(stack, layer, cover));
    }
    // Per variant, every part CoverMap interpolates: the total, the plane
    // coupling, the ground and the neighbor coupling, all against the
    // total.
    constexpr int kParts = 4;
    std::vector<ProbeCurve> curves;
    for (const Job& job : jobs) {
      if (job.side != side) {
        continue;
      }
      const size_t first = kParts * job.index;
      if (curves.size() < first + kParts) {
        curves.resize(first + kParts);
        for (ProbeCurve& c : curves) {
          c.value.resize(candidates.size(), 0);
          c.scale.resize(candidates.size(), 0);
        }
      }
      const LateralCap& cap = job.cap;
      const double parts[kParts] = {cap.total(),
                                    side == 0 ? cap.below : cap.above,
                                    cap.ground,
                                    cap.left + cap.right};
      for (int q = 0; q < kParts; q++) {
        curves[first + q].value[job.cover] = parts[q];
        curves[first + q].scale[job.cover] = cap.total();
      }
    }
    for (const int k : pickCovers(distances, curves, options.cover_tolerance)) {
      picked.push_back(candidates[k]);
    }
  }
  return plan;
}

std::vector<Layer> coversBelow(const fsv::Stack& stack,
                               const Layer layer,
                               const BuildOptions& options)
{
  return planCovers(stack, layer, options).below;
}

std::vector<Layer> coversAbove(const fsv::Stack& stack,
                               const Layer layer,
                               const BuildOptions& options)
{
  return planCovers(stack, layer, options).above;
}

void buildLateral(const fsv::Stack& stack,
                  const Layer layer,
                  const BuildOptions& options,
                  Model& model)
{
  buildLateral(stack, layer, planCovers(stack, layer, options), options, model);
}

void buildLateral(const fsv::Stack& stack,
                  const Layer layer,
                  const CoverPlan& plan,
                  const BuildOptions& options,
                  Model& model)
{
  const fsv::Stack::Conductor& conductor = requireConductor(stack, layer);
  struct Job
  {
    Layer below;
    Layer above;
    LateralTable* table;
    int width;
    int left;  // spacing indices; spacings.size() is open
    int right;
  };
  // A point and its mirror share one solve; the table stores both.
  std::vector<Job> jobs;
  for (const Layer below : plan.below) {
    for (const Layer above : plan.above) {
      const LateralKey key{
          conductor.name, keyName(stack, below), keyName(stack, above)};
      std::vector<double> widths;
      for (const double f : options.width_factors) {
        widths.push_back(f * conductor.min_width);
      }
      std::vector<double> spacings;
      for (const double f : options.spacing_factors) {
        spacings.push_back(f * conductor.min_spacing);
      }
      {
        // The distance to the nearest plane: the substrate or a cover.
        double h = below.valid()
                       ? conductor.bottom - requireConductor(stack, below).top
                       : conductor.bottom;
        if (above.valid()) {
          h = std::min(h,
                       requireConductor(stack, above).bottom - conductor.top);
        }
        requireReach("lateral_reach", options.lateral_reach);
        requireReach("lateral_reach_one_sided",
                     options.lateral_reach_one_sided);
        const double reach = below.valid() != above.valid()
                                 ? options.lateral_reach_one_sided
                                 : options.lateral_reach;
        if (reach > 0 && h > 0) {
          extendGrid(spacings, reach * h, options.lateral_reach_step);
        }
      }
      if (const LateralTable* built = model.lateral(key)) {
        requireGrid("lateral table of " + key.layer, built->widths(), widths);
        requireGrid(
            "lateral table of " + key.layer, built->spacings(), spacings);
        continue;
      }
      LateralTable& table
          = model.addLateral(key, LateralTable(widths, spacings));
      const int n = table.spacingCount();
      for (int w = 0; w < table.widthCount(); w++) {
        for (int l = 0; l < n; l++) {
          for (int r = l; r < n; r++) {
            jobs.push_back({below, above, &table, w, l, r});
          }
        }
      }
    }
  }
  runJobs(options, jobs.size(), [&](const int i, fsv::Solver& solver) {
    const Job& job = jobs[i];
    LateralTable& table = *job.table;
    const int open = static_cast<int>(table.spacings().size());
    LateralCap cap
        = solveSection(stack,
                       layer,
                       table.widths()[job.width],
                       job.left == open ? -1 : table.spacings()[job.left],
                       job.right == open ? -1 : table.spacings()[job.right],
                       job.below,
                       job.above,
                       options,
                       solver,
                       options.tol);
    if (job.left == job.right) {
      // One quantity estimated twice.
      cap.left = cap.right = (cap.left + cap.right) / 2;
    }
    // Tables are sized before the jobs start; writes go to distinct points.
    table.set(job.width, job.left, job.right, cap);
  });
}

double keptShare(const double length, const double gap)
{
  if (gap <= 0) {
    return 1;
  }
  return length / gap * -std::expm1(-gap / length);
}

KeptFit fitKept(const std::vector<double>& gaps,
                const std::vector<double>& shares)
{
  if (gaps.empty() || gaps.size() != shares.size()) {
    throw std::invalid_argument("a kept fit needs a share per gap");
  }
  // The squared residual of the kept amounts, gap x share, at a length.
  auto residual = [&](const double length) {
    double sum = 0;
    for (size_t i = 0; i < gaps.size(); i++) {
      const double r = gaps[i] * (shares[i] - keptShare(length, gaps[i]));
      sum += r * r;
    }
    return sum;
  };
  // A scan in the logarithm from a thousandth of the narrowest gap to a
  // thousand times the widest, then golden sections around the best.
  const double lo = std::log(1e-3 * gaps.front());
  const double hi = std::log(1e3 * gaps.back());
  constexpr int kScan = 200;
  int best = 0;
  double best_r = residual(std::exp(lo));
  for (int i = 1; i <= kScan; i++) {
    const double r = residual(std::exp(lo + (hi - lo) * i / kScan));
    if (r < best_r) {
      best = i;
      best_r = r;
    }
  }
  double a = lo + (hi - lo) * std::max(0, best - 1) / kScan;
  double b = lo + (hi - lo) * std::min(kScan, best + 1) / kScan;
  const double phi = (std::sqrt(5.0) - 1) / 2;
  for (int i = 0; i < 60; i++) {
    const double c = b - phi * (b - a);
    const double d = a + phi * (b - a);
    if (residual(std::exp(c)) < residual(std::exp(d))) {
      b = d;
    } else {
      a = c;
    }
  }
  const double length = std::exp((a + b) / 2);
  return {length, std::sqrt(residual(length) / gaps.size())};
}

namespace {

// The reach of each of the layer's crossing tables that has none. Per
// table, the wire running each past factor beyond one crossing running
// each extent factor past it, one running on, and none.
void buildReach(const fsv::Stack& stack,
                const Layer layer,
                const std::vector<Layer>& covers,
                const BuildOptions& options,
                Model& model)
{
  const fsv::Stack::Conductor& conductor = requireConductor(stack, layer);
  requireReachFactors(options);
  const std::vector<double>& factors = options.reach_factors;
  struct Job
  {
    double total = 0;
    double cross = 0;
  };
  struct Table
  {
    Layer cover;
    CrossingKey key;
    double width;  // the crossing's
    std::vector<double> grid;
    double running_on;
    std::vector<Job> jobs;  // [past][extent, running on, none]
  };
  std::vector<Table> tables;
  for (const Layer cover : covers) {
    const fsv::Stack::Conductor& cover_conductor
        = requireConductor(stack, cover);
    const CrossingKey key{conductor.name, cover_conductor.name, ""};
    const CrossingTable* held = model.crossing(key);
    if (held == nullptr) {
      continue;
    }
    const double unit
        = std::max(conductor.min_width, cover_conductor.min_width);
    Table table{cover, key, cover_conductor.min_width, {}, 0, {}};
    for (const double f : factors) {
      table.grid.push_back(f * unit);
    }
    if (held->hasReach()) {
      requireGrid("crossing reach of " + key.layer + " and " + key.cover,
                  held->reach().past,
                  table.grid);
      continue;
    }
    table.running_on = std::max(options.extent, 2 * table.grid.back() + unit);
    table.jobs.resize(table.grid.size() * (table.grid.size() + 2));
    tables.push_back(std::move(table));
  }
  std::vector<std::pair<int, int>> tasks;
  for (int t = 0; t < static_cast<int>(tables.size()); t++) {
    for (int j = 0; j < static_cast<int>(tables[t].jobs.size()); j++) {
      tasks.emplace_back(t, j);
    }
  }
  runJobs(options, tasks.size(), [&](const int i, fsv::Solver& solver) {
    Table& table = tables[tasks[i].first];
    const int j = tasks[i].second;
    const int per_past = static_cast<int>(table.grid.size()) + 2;
    const int e = j % per_past;
    ReachPattern pattern;
    pattern.layer = layer;
    pattern.cover = table.cover;
    pattern.width = conductor.min_width;
    pattern.crossing_width = table.width;
    pattern.past = table.grid[j / per_past];
    pattern.extent = e < per_past - 2    ? table.grid[e]
                     : e == per_past - 2 ? table.running_on
                                         : -1;
    const ReachPattern::Pair pair = pattern.build(stack);
    const fsv::Scene scene(pair.geometry, stack);
    const fsv::CapRow row
        = solver.solve(scene, pair.wire, solveOptions(options.tol / 3));
    table.jobs[j].total = row.total();
    table.jobs[j].cross
        = pair.cross == kNullNetIndex ? 0 : row.coupling(pair.cross);
  });
  for (const Table& table : tables) {
    const int n = static_cast<int>(table.grid.size());
    CrossingReach reach;
    reach.width = table.width;
    reach.past = table.grid;
    reach.extents = table.grid;
    reach.extents.push_back(table.running_on);
    for (int p = 0; p < n; p++) {
      const Job* row = &table.jobs[p * (n + 2)];
      const Job& on = row[n];
      const Job& none = row[n + 1];
      // A total the crossing changes too little against the solver's
      // error takes the coupling's ratio.
      const double effect = on.total - none.total;
      const bool total_usable = std::abs(effect) > 0.05 * none.total;
      for (int e = 0; e <= n; e++) {
        const Job& job = e < n ? row[e] : on;
        const double cross = on.cross > 0 ? job.cross / on.cross : 1;
        const double total
            = total_usable ? (job.total - none.total) / effect : cross;
        reach.values.push_back({total, cross});
      }
    }
    CrossingTable updated = *model.crossing(table.key);
    updated.setReach(std::move(reach));
    model.addCrossing(table.key, std::move(updated));
  }
}

}  // namespace

void buildCrossing(const fsv::Stack& stack,
                   const Layer layer,
                   const Layer cover,
                   const BuildOptions& options,
                   Model& model)
{
  buildCrossings(stack, layer, {cover}, options, model);
}

void buildCrossings(const fsv::Stack& stack,
                    const Layer layer,
                    const std::vector<Layer>& covers,
                    const BuildOptions& options,
                    Model& model,
                    const Layer plane_below,
                    const Layer plane_above)
{
  // Each coverage sets a pitch (width / f) and a gap (width (1 / f - 1)),
  // and the share divides by 1 - f: the fraction has to be strictly inside
  // (0, 1), checked before any solve runs, as are the reach factors.
  requireReachFactors(options);
  for (const double f : options.coverages) {
    if (!(f > 0 && f < 1)) {
      throw std::invalid_argument("the crossing coverage " + std::to_string(f)
                                  + " is not between 0 and 1");
    }
  }
  const fsv::Stack::Conductor& conductor = requireConductor(stack, layer);
  const double w = conductor.min_width;
  std::vector<double> spacings;
  for (const double f : options.crossing_spacing_factors) {
    spacings.push_back(f * conductor.min_spacing);
  }
  const int ns = static_cast<int>(spacings.size()) + 1;
  auto spacingOf
      = [&](const int si) { return si < ns - 1 ? spacings[si] : -1.0; };
  // For each cover and spacing column (no neighbor last): one 3D solve per
  // (crossing width, coverage, length), then the two per-length
  // references. The two lengths are whole pitches and differ by at least
  // one, so their difference is never zero.
  struct Job
  {
    int spacing;
    double wc;
    double coverage;
    double length;
    double total = 0;
    double cross = 0;
  };
  struct Table
  {
    Layer cover;
    Layer opposite;  // the plane on the wire's other side, if any
    CrossingKey key;
    bool over;
    std::vector<Job> jobs;
    std::vector<LateralCap> open_cap;
    std::vector<LateralCap> plane_cap;
  };
  std::vector<Table> tables;
  for (const Layer cover : covers) {
    const fsv::Stack::Conductor& cover_conductor
        = requireConductor(stack, cover);
    const bool over = cover_conductor.bottom >= conductor.top;
    if (!over && cover_conductor.top > conductor.bottom) {
      throw std::invalid_argument(cover_conductor.name + " is beside "
                                  + conductor.name + ", not over or under");
    }
    std::vector<Layer> opposites{Layer()};
    if (const Layer plane = over ? plane_below : plane_above;
        plane.valid() && plane != cover) {
      opposites.push_back(plane);
    }
    for (const Layer opposite : opposites) {
      CrossingKey key{conductor.name, cover_conductor.name, ""};
      if (opposite.valid()) {
        key.opposite = requireConductor(stack, opposite).name;
      }
      if (const CrossingTable* built = model.crossing(key)) {
        requireGrid(
            "crossing table of " + key.layer, built->spacings(), spacings);
        continue;  // built before; the reach follows below
      }
      Table table{cover,
                  opposite,
                  key,
                  over,
                  {},
                  std::vector<LateralCap>(ns),
                  std::vector<LateralCap>(ns)};
      for (int si = 0; si < ns; si++) {
        for (const double wf : options.crossing_width_factors) {
          for (const double f : options.coverages) {
            const double wc = wf * cover_conductor.min_width;
            const double pitch = wc / f;
            const int n_short = std::max(
                1,
                static_cast<int>(std::lround(options.crossing_length / pitch)));
            const int n_long = std::max(
                n_short + 1,
                static_cast<int>(
                    std::lround(options.crossing_length_long / pitch)));
            table.jobs.push_back({si, wc, f, n_short * pitch});
            table.jobs.push_back({si, wc, f, n_long * pitch});
          }
        }
      }
      tables.push_back(std::move(table));
    }
  }
  // All the layer's solves in one pool, the long arrays first so the last
  // to finish are short: (table, job) for an array, (table, -1 - k) for
  // reference k (open and plane per spacing).
  std::vector<std::pair<int, int>> tasks;
  for (int t = 0; t < static_cast<int>(tables.size()); t++) {
    for (int j = 0; j < static_cast<int>(tables[t].jobs.size()); j++) {
      tasks.emplace_back(t, j);
    }
  }
  std::stable_sort(
      tasks.begin(), tasks.end(), [&](const auto& x, const auto& y) {
        return tables[x.first].jobs[x.second].length
               > tables[y.first].jobs[y.second].length;
      });
  for (int t = 0; t < static_cast<int>(tables.size()); t++) {
    for (int k = 0; k < 2 * ns; k++) {
      tasks.emplace_back(t, -1 - k);
    }
  }
  runJobs(options, tasks.size(), [&](const int i, fsv::Solver& solver) {
    Table& table = tables[tasks[i].first];
    const int j = tasks[i].second;
    if (j < 0) {
      const int k = -1 - j;
      const int si = k / 2;
      const bool is_open = k % 2 == 0;
      LateralCap& cap = is_open ? table.open_cap[si] : table.plane_cap[si];
      // The plane sits where the cover is; the open wire has none. The
      // plane on the other side is in both.
      const Layer covered = is_open ? Layer() : table.cover;
      const Layer below = table.over ? table.opposite : covered;
      const Layer above = table.over ? covered : table.opposite;
      const double s = spacingOf(si);
      cap = solveSection(stack,
                         layer,
                         w,
                         s,
                         s,
                         below,
                         above,
                         options,
                         solver,
                         options.tol / 3);
      return;
    }
    Job& job = table.jobs[j];
    CrossingPattern pattern;
    pattern.layer = layer;
    pattern.cover = table.cover;
    pattern.width = w;
    pattern.crossing_width = job.wc;
    pattern.pitch = job.wc / job.coverage;
    pattern.length = job.length;
    pattern.spacing = spacingOf(job.spacing);
    pattern.other = table.opposite;
    pattern.extent = options.extent;
    const CrossingPattern::Array array = pattern.build(stack);
    job.length = array.length;
    const fsv::Scene scene(array.geometry, stack);
    const fsv::CapRow row
        = solver.solve(scene, array.wire, solveOptions(options.tol));
    job.total = row.total();
    job.cross = row.coupling(array.cross);
  });
  for (const Table& table : tables) {
    // Pairs of lengths give per-length values, then the share over the
    // gap, relative to what the plane adds with the same neighbors; a
    // plane that adds nothing has no shares to tabulate.
    std::vector<double> gaps;
    // The raw shares per column, then a fitted kept amount per column for
    // the total and the coupling. A column the neighbors leave the plane
    // nothing to add to, or whose fit fails, takes its length from a wider
    // spacing (the coupling) or from its coupling's, scaled as without
    // neighbors (the total).
    std::vector<std::vector<CrossingShare>> raw(ns);
    std::vector<bool> usable(ns, false);
    const int per_column = static_cast<int>(table.jobs.size()) / ns;
    for (int si = 0; si < ns; si++) {
      const LateralCap& open = table.open_cap[si];
      const LateralCap& plane = table.plane_cap[si];
      const double plane_total = plane.total();
      const double plane_cross = table.over ? plane.above : plane.below;
      usable[si] = plane_total > open.total() && plane_cross > 0;
      if (!usable[si]) {
        continue;
      }
      std::vector<std::pair<double, CrossingShare>> points;
      for (int i = si * per_column; i + 1 < (si + 1) * per_column; i += 2) {
        const Job& a = table.jobs[i];
        const Job& b = table.jobs[i + 1];
        const double dl = b.length - a.length;
        const double total = (b.total - a.total) / dl;
        const double cross = (b.cross - a.cross) / dl;
        const double f = a.coverage;
        const double g_total
            = (total - open.total()) / (plane_total - open.total());
        const double g_cross = cross / plane_cross;
        const double gap = a.wc * (1 / f - 1);
        points.push_back(
            {gap, {(g_total - f) / (1 - f), (g_cross - f) / (1 - f)}});
      }
      std::sort(points.begin(), points.end(), [](const auto& x, const auto& y) {
        return x.first < y.first;
      });
      // Points at the same gap (within rounding) average into one; every
      // column has the same gaps.
      std::vector<double> column_gaps;
      std::vector<CrossingShare> column;
      std::vector<int> counts;
      for (const auto& [gap, share] : points) {
        if (!column_gaps.empty()
            && gap - column_gaps.back() <= 1e-9 * std::max(gap, 1.0)) {
          CrossingShare& last = column.back();
          const int n = ++counts.back();
          last.total += (share.total - last.total) / n;
          last.cross += (share.cross - last.cross) / n;
          continue;
        }
        column_gaps.push_back(gap);
        column.push_back(share);
        counts.push_back(1);
      }
      gaps = column_gaps;
      raw[si] = std::move(column);
    }
    auto fitted = [&](const int si, const bool cross) {
      std::vector<double> shares;
      for (const CrossingShare& v : raw[si]) {
        shares.push_back(cross ? v.cross : v.total);
      }
      const KeptFit fit = fitKept(gaps, shares);
      const bool good = fit.length > 0.01 * w
                        && fit.residual <= kKeptFitTolerance * fit.length;
      return good ? fit.length : -1.0;
    };
    std::vector<double> length_total(ns, -1);
    std::vector<double> length_cross(ns, -1);
    const int none = ns - 1;
    if (usable[none]) {
      length_total[none] = fitted(none, false);
      length_cross[none] = fitted(none, true);
    }
    if (length_cross[none] <= 0 && table.opposite.valid()) {
      // The table without the plane stands in, held under this key too so
      // that a resumed build finds it; its reach stays with that table.
      // The planeless table comes first in `tables` or was held before.
      const CrossingTable* plain
          = model.crossing({table.key.layer, table.key.cover, ""});
      model.addCrossing(
          table.key,
          CrossingTable(plain->gaps(), plain->values(), plain->spacings()));
      continue;
    }
    if (length_cross[none] <= 0) {
      throw std::runtime_error("the crossing shares of " + table.key.layer
                               + " under or over " + table.key.cover
                               + " fit no kept amount; solve them tighter");
    }
    // A far cover changes the total too little for its shares to fit; its
    // coupling's length stands in.
    if (length_total[none] <= 0) {
      length_total[none] = length_cross[none];
    }
    const double ratio = length_total[none] / length_cross[none];
    for (int si = none - 1; si >= 0; si--) {
      if (usable[si]) {
        length_cross[si] = fitted(si, true);
        length_total[si] = fitted(si, false);
      }
      if (length_cross[si] <= 0) {
        length_cross[si] = length_cross[si + 1];
      }
      if (length_total[si] <= 0) {
        length_total[si] = length_cross[si] * ratio;
      }
    }
    std::vector<CrossingShare> values;
    for (int si = 0; si < ns; si++) {
      for (const double gap : gaps) {
        values.push_back({keptShare(length_total[si], gap),
                          keptShare(length_cross[si], gap)});
      }
    }
    model.addCrossing(table.key, CrossingTable(gaps, values, spacings));
  }
  buildReach(stack, layer, covers, options, model);
}

void buildEnds(const fsv::Stack& stack,
               const Layer layer,
               const BuildOptions& options,
               Model& model)
{
  buildEnds(stack, layer, planCovers(stack, layer, options), options, model);
}

namespace {

// The corners of wires of a width meeting, from a straight wire, a cross
// and a bend of the corner pattern and the per-length reference. As the
// reference evaluator adds them, a cross is the two wires with the
// overlap once and four inner corners; a bend is the two wires without
// the ends at the turn, with the overlap once, an inner and an outer
// corner.
Corners solveCorners(const fsv::Stack& stack,
                     const Layer layer,
                     const double width,
                     const Layer below,
                     const Layer above,
                     const BuildOptions& options,
                     fsv::Solver& solver)
{
  const LateralCap unit = solveSection(stack,
                                       layer,
                                       width,
                                       -1,
                                       -1,
                                       below,
                                       above,
                                       options,
                                       solver,
                                       options.tol / 3);
  CornerPattern pattern;
  pattern.layer = layer;
  pattern.width = width;
  pattern.length = std::max(options.end_length, 5 * width);
  pattern.below = below;
  pattern.above = above;
  pattern.extent = options.extent;
  auto solve = [&](const CornerPattern::Shape shape) {
    pattern.shape = shape;
    const CornerPattern::Wire wire = pattern.build(stack);
    const fsv::Scene scene(wire.geometry, stack);
    const fsv::CapRow row
        = solver.solve(scene, wire.wire, solveOptions(options.tol));
    auto coupling = [&](const NetIndex net) {
      return net == kNullNetIndex ? 0.0 : row.coupling(net);
    };
    return CornerValue{row.total(), coupling(wire.below), coupling(wire.above)};
  };
  const CornerValue straight = solve(CornerPattern::Shape::kStraight);
  const CornerValue cross = solve(CornerPattern::Shape::kCross);
  const CornerValue bend = solve(CornerPattern::Shape::kBend);
  const double length = pattern.length;
  auto part
      = [&](const double s, const double c, const double b, const double u) {
          const double end = (s - (u * length)) / 2;
          const double inner = (c - (2 * s) + (width * u)) / 4;
          const double outer = b - ((2 * s) - (2 * end) - (width * u)) - inner;
          return std::pair{inner, outer};
        };
  const auto [inner_total, outer_total]
      = part(straight.total, cross.total, bend.total, unit.total());
  const auto [inner_below, outer_below]
      = part(straight.below, cross.below, bend.below, unit.below);
  const auto [inner_above, outer_above]
      = part(straight.above, cross.above, bend.above, unit.above);
  return {{inner_total, inner_below, inner_above},
          {outer_total, outer_below, outer_above}};
}

}  // namespace

void buildEnds(const fsv::Stack& stack,
               const Layer layer,
               const CoverPlan& plan,
               const BuildOptions& options,
               Model& model)
{
  const fsv::Stack::Conductor& conductor = requireConductor(stack, layer);
  std::vector<double> widths;
  for (const double f : options.width_factors) {
    widths.push_back(f * conductor.min_width);
  }
  std::vector<double> spacings;
  for (const double f : options.end_spacing_factors) {
    spacings.push_back(f * conductor.min_spacing);
  }
  std::vector<double> gaps;
  for (const double f : options.facing_factors) {
    gaps.push_back(f * conductor.min_spacing);
  }
  const int ns = static_cast<int>(spacings.size()) + 1;
  const int ng = static_cast<int>(gaps.size()) + 1;
  // One job per context, width and pair of spacing indices (the last is
  // none): the per-length reference, then the end. A like-sided pair
  // solves the end at each gap (and none), a different-sided one, left
  // first, at none only. And one job per context and width for the
  // corners. A table held without its different-sided ends or corners gets
  // those.
  struct Job
  {
    LateralKey key;
    Layer below;
    Layer above;
    int width;
    int left;
    int right;
    bool corner = false;
    std::vector<EndValue> values;  // like-sided: by gap
    EndSides sides;                // different-sided
    Corners corners;
  };
  std::vector<Job> jobs;
  for (const Layer below : plan.below) {
    for (const Layer above : plan.above) {
      const LateralKey key{
          conductor.name, keyName(stack, below), keyName(stack, above)};
      const EndTable* built = model.end(key);
      if (built != nullptr) {
        const std::string what = "end table of " + key.layer;
        requireGrid(what, built->widths(), widths);
        requireGrid(what, built->spacings(), spacings);
        requireGrid(what, built->gaps(), gaps);
      }
      const bool need_sides = built == nullptr || !built->hasSides();
      const bool need_corners = built == nullptr || !built->hasCorners();
      for (int w = 0; w < static_cast<int>(widths.size()); w++) {
        for (int l = 0; l < ns; l++) {
          for (int r = l; r < ns; r++) {
            if (r == l ? built == nullptr : need_sides) {
              jobs.push_back({key, below, above, w, l, r, false, {}, {}, {}});
            }
          }
        }
        if (need_corners) {
          jobs.push_back({key, below, above, w, 0, 0, true, {}, {}, {}});
        }
      }
    }
  }
  runJobs(options, jobs.size(), [&](const int i, fsv::Solver& solver) {
    Job& job = jobs[i];
    const double w = widths[job.width];
    if (job.corner) {
      job.corners = solveCorners(
          stack, layer, w, job.below, job.above, options, solver);
      return;
    }
    const double sl = job.left < ns - 1 ? spacings[job.left] : -1;
    const double sr = job.right < ns - 1 ? spacings[job.right] : -1;
    const LateralCap unit = solveSection(stack,
                                         layer,
                                         w,
                                         sl,
                                         sr,
                                         job.below,
                                         job.above,
                                         options,
                                         solver,
                                         options.tol / 3);
    EndPattern pattern;
    pattern.layer = layer;
    pattern.width = w;
    pattern.length = std::max(options.end_length, 5 * w);
    pattern.below = job.below;
    pattern.above = job.above;
    pattern.extent = options.extent;
    pattern.margin = options.extent;
    pattern.left_spacing = sl;
    pattern.right_spacing = sr;
    const int solved_gaps = job.left == job.right ? ng : 1;
    for (int k = 0; k < solved_gaps; k++) {
      // The like-sided ends' gaps, ending at none; else none alone.
      const int gi = job.left == job.right ? k : ng - 1;
      pattern.facing_gap = gi < ng - 1 ? gaps[gi] : -1;
      const EndPattern::Wire wire = pattern.build(stack);
      const fsv::Scene scene(wire.geometry, stack);
      const fsv::CapRow row
          = solver.solve(scene, wire.wire, solveOptions(options.tol));
      // The pattern has two like ends: half of what it holds beyond the
      // per-length reference.
      auto excess = [&](const double cap, const double per_length) {
        return (cap - per_length * pattern.length) / 2;
      };
      auto coupling = [&](const NetIndex net) {
        return net == kNullNetIndex ? 0.0 : row.coupling(net);
      };
      const double total = excess(row.total(), unit.total());
      const double to_below = excess(coupling(wire.below), unit.below);
      const double to_above = excess(coupling(wire.above), unit.above);
      const double to_left = excess(coupling(wire.left), unit.left);
      const double to_right = excess(coupling(wire.right), unit.right);
      if (job.left == job.right) {
        job.values.push_back({total,
                              to_below,
                              to_above,
                              to_left + to_right,
                              coupling(wire.facing) / 2});
      } else {
        job.sides = {total, to_below, to_above, to_left, to_right};
      }
    }
  });
  std::map<LateralKey, EndTable> tables;
  for (const Job& job : jobs) {
    auto it = tables.find(job.key);
    if (it == tables.end()) {
      const EndTable* built = model.end(job.key);
      it = tables
               .emplace(
                   job.key,
                   built != nullptr ? *built : EndTable(widths, spacings, gaps))
               .first;
    }
    if (job.corner) {
      it->second.setCorners(job.width, job.corners);
    } else if (job.left == job.right) {
      for (int gi = 0; gi < ng; gi++) {
        it->second.set(job.width, job.left, gi, job.values[gi]);
      }
    } else {
      it->second.setSides(job.width, job.left, job.right, job.sides);
    }
  }
  for (auto& [key, table] : tables) {
    model.addEnd(key, std::move(table));
  }
}

std::vector<Layer> diagonalLayers(const fsv::Stack& stack,
                                  const Layer layer,
                                  const BuildOptions& options)
{
  requireLayers(stack, options);
  std::vector<Layer> out;
  for (const std::vector<Layer>& side :
       {layersBelow(stack, layer), layersAbove(stack, layer)}) {
    int depth = 0;
    for (const Layer other : side) {
      if (depth >= options.diagonal_depth) {
        break;
      }
      if (!usable(stack, options, other)) {
        continue;
      }
      out.push_back(other);
      depth++;
    }
  }
  return out;
}

void buildDiagonal(const fsv::Stack& stack,
                   const Layer layer,
                   const Layer other,
                   const BuildOptions& options,
                   Model& model,
                   const Layer opposite)
{
  const fsv::Stack::Conductor& conductor = requireConductor(stack, layer);
  const fsv::Stack::Conductor& other_conductor = requireConductor(stack, other);
  DiagonalKey key{conductor.name, other_conductor.name, ""};
  if (opposite.valid()) {
    const fsv::Stack::Conductor& o = requireConductor(stack, opposite);
    const bool other_above = other_conductor.bottom >= conductor.top;
    const bool opposite_above = o.bottom >= conductor.top;
    if (other_above == opposite_above
        || (!opposite_above && o.top > conductor.bottom)) {
      throw std::invalid_argument(o.name + " is not on the other side of "
                                  + conductor.name + " from "
                                  + other_conductor.name);
    }
    key.opposite = o.name;
  }
  const double w = conductor.min_width;
  std::vector<double> widths;
  for (const double f : options.diagonal_width_factors) {
    widths.push_back(f * other_conductor.min_width);
  }
  std::vector<double> gaps;
  for (const double f : options.diagonal_gap_factors) {
    gaps.push_back(f * w);
  }
  requireReach("diagonal_reach", options.diagonal_reach);
  if (options.diagonal_reach > 0) {
    const double dz = other_conductor.bottom >= conductor.top
                          ? other_conductor.bottom - conductor.top
                          : conductor.bottom - other_conductor.top;
    extendGrid(gaps, options.diagonal_reach * dz, 2);
  }
  std::vector<double> spacings;
  for (const double f : options.diagonal_spacing_factors) {
    spacings.push_back(f * conductor.min_spacing);
  }
  std::vector<double> far_spacings;
  for (const double f : options.diagonal_far_spacing_factors) {
    far_spacings.push_back(f * conductor.min_spacing);
  }
  if (const DiagonalTable* built = model.diagonal(key)) {
    const std::string what = "diagonal table of " + key.layer;
    requireGrid(what, built->widths(), widths);
    requireGrid(what, built->gaps(), gaps);
    requireGrid(what, built->spacings(), spacings);
    requireGrid(what, built->farSpacings(), far_spacings);
    return;
  }
  DiagonalTable& table = model.addDiagonal(
      key, DiagonalTable(widths, gaps, spacings, far_spacings));
  const int ns = table.spacingCount();
  const int nf = table.farCount();
  const int ng = table.gapCount();
  // The wire with its neighbors, with and without the parallel wire: one
  // base solve per pair of spacings, then one per point.
  struct Base
  {
    double total = 0;
    double right = 0;
    double plane = 0;  // the coupling to the opposite plane, if any
  };
  std::vector<Base> bases(ns * nf);
  const int points = table.widthCount() * ng * ns * nf;
  std::vector<DiagonalCap> raw(points);
  auto section
      = [&](const int s, const int f, const double wd, const double gap) {
          DiagonalPattern pattern;
          pattern.layer = layer;
          pattern.other = other;
          pattern.width = w;
          pattern.other_width = wd;
          pattern.gap = gap;
          pattern.spacing = s == ns - 1 ? -1 : table.spacings()[s];
          pattern.far_spacing = f == nf - 1 ? -1 : table.farSpacings()[f];
          pattern.opposite = opposite;
          pattern.extent = options.extent;
          return pattern.build(stack);
        };
  runJobs(options, points + ns * nf, [&](const int i, fsv::Solver& solver) {
    if (i >= points) {
      const int b = i - points;
      const DiagonalPattern::Section d = section(b / nf, b % nf, 0, 0);
      const fsv::Scene2D scene(d.geometry, stack);
      const fsv::CapRow row
          = solver.solve(scene, d.wire, solveOptions(options.tol / 3));
      bases[b] = {row.total(),
                  d.right != kNullNetIndex ? row.coupling(d.right) : 0,
                  d.opposite != kNullNetIndex ? row.coupling(d.opposite) : 0};
      return;
    }
    const int f = i % nf;
    const int s = (i / nf) % ns;
    const int gi = (i / nf / ns) % ng;
    const int wi = i / nf / ns / ng;
    const DiagonalPattern::Section d
        = section(s, f, table.widths()[wi], table.gaps()[gi]);
    const fsv::Scene2D scene(d.geometry, stack);
    const fsv::CapRow row
        = solver.solve(scene, d.wire, solveOptions(options.tol));
    DiagonalCap& cap = raw[i];
    cap.coupling = row.coupling(d.diagonal);
    cap.total_delta = row.total();
    cap.lateral_delta = d.right != kNullNetIndex ? row.coupling(d.right) : 0;
    cap.plane_delta
        = d.opposite != kNullNetIndex ? row.coupling(d.opposite) : 0;
  });
  // Deltas are against the wire without the parallel one.
  for (int i = 0; i < points; i++) {
    const int f = i % nf;
    const int s = (i / nf) % ns;
    const int gi = (i / nf / ns) % ng;
    const int wi = i / nf / ns / ng;
    DiagonalCap cap = raw[i];
    cap.total_delta -= bases[s * nf + f].total;
    cap.lateral_delta -= bases[s * nf + f].right;
    cap.plane_delta -= bases[s * nf + f].plane;
    table.set(wi, gi, s, f, cap);
  }
}

std::optional<CoverPlan> heldCovers(const fsv::Stack& stack,
                                    const Model& model,
                                    const Layer layer)
{
  std::set<std::string> held[2];
  const std::string& name = stack.conductor(layer).name;
  for (const auto& [key, table] : model.laterals()) {
    if (key.layer == name) {
      held[0].insert(key.below);
      held[1].insert(key.above);
    }
  }
  if (held[0].empty()) {
    return std::nullopt;
  }
  CoverPlan plan;
  for (const int side : {0, 1}) {
    std::vector<Layer>& covers = side == 0 ? plan.below : plan.above;
    covers.push_back(Layer());
    for (const Layer cover :
         side == 0 ? layersBelow(stack, layer) : layersAbove(stack, layer)) {
      if (held[side].count(stack.name(cover))) {
        covers.push_back(cover);
      }
    }
  }
  return plan;
}

namespace {

// The nearest cover of a side of a plan; none when it has none.
Layer nearestCover(const std::vector<Layer>& side)
{
  for (const Layer cover : side) {
    if (cover.valid()) {
      return cover;
    }
  }
  return Layer();
}

}  // namespace

void buildLayer(const fsv::Stack& stack,
                const Layer layer,
                const BuildOptions& options,
                Model& model)
{
  buildLayer(stack, layer, planCovers(stack, layer, options), options, model);
}

void buildLayer(const fsv::Stack& stack,
                const Layer layer,
                const CoverPlan& plan,
                const BuildOptions& options,
                Model& model)
{
  buildLateral(stack, layer, plan, options, model);
  buildEnds(stack, layer, plan, options, model);
  std::vector<Layer> covers;
  for (const std::vector<Layer>* side : {&plan.below, &plan.above}) {
    for (const Layer cover : *side) {
      if (cover.valid()) {
        covers.push_back(cover);
      }
    }
  }
  buildCrossings(stack,
                 layer,
                 covers,
                 options,
                 model,
                 nearestCover(plan.below),
                 nearestCover(plan.above));
  // The nearest cover on each side, the plane opposite a parallel wire on
  // the other.
  const Layer nearest_below = nearestCover(plan.below);
  const Layer nearest_above = nearestCover(plan.above);
  for (const Layer other : diagonalLayers(stack, layer, options)) {
    buildDiagonal(stack, layer, other, options, model);
    const bool above = requireConductor(stack, other).bottom
                       >= requireConductor(stack, layer).top;
    const Layer opposite = above ? nearest_below : nearest_above;
    if (opposite.valid()) {
      buildDiagonal(stack, layer, other, options, model, opposite);
    }
  }
}

}  // namespace pex
