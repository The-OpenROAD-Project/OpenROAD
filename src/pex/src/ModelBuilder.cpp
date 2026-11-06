// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "pex/ModelBuilder.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
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
  list("cw", options.crossing_width_factors);
  list("cf", options.coverages);
  list("dw", options.diagonal_width_factors);
  list("dg", options.diagonal_gap_factors);
  list("ds", options.diagonal_spacing_factors);
  list("df", options.diagonal_far_spacing_factors);
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

void buildCrossing(const fsv::Stack& stack,
                   const Layer layer,
                   const Layer cover,
                   const BuildOptions& options,
                   Model& model)
{
  // Each coverage sets a pitch (width / f) and a gap (width (1 / f - 1)),
  // and the share divides by 1 - f: the fraction has to be strictly inside
  // (0, 1), checked before any solve runs.
  for (const double f : options.coverages) {
    if (!(f > 0 && f < 1)) {
      throw std::invalid_argument("the crossing coverage " + std::to_string(f)
                                  + " is not between 0 and 1");
    }
  }
  const fsv::Stack::Conductor& conductor = requireConductor(stack, layer);
  const fsv::Stack::Conductor& cover_conductor = requireConductor(stack, cover);
  const bool over = cover_conductor.bottom >= conductor.top;
  if (!over && cover_conductor.top > conductor.bottom) {
    throw std::invalid_argument(cover_conductor.name + " is beside "
                                + conductor.name + ", not over or under");
  }
  const CrossingKey key{conductor.name, cover_conductor.name};
  if (model.crossing(key) != nullptr) {
    return;  // built before
  }
  const double w = conductor.min_width;
  // One 3D solve per (crossing width, coverage, length), then the two
  // per-length references. The two lengths are whole pitches and differ
  // by at least one, so their difference is never zero.
  struct Job
  {
    double wc;
    double coverage;
    double length;
    double total = 0;
    double cross = 0;
  };
  std::vector<Job> jobs;
  for (const double wf : options.crossing_width_factors) {
    for (const double f : options.coverages) {
      const double wc = wf * cover_conductor.min_width;
      const double pitch = wc / f;
      const int n_short = std::max(
          1, static_cast<int>(std::lround(options.crossing_length / pitch)));
      const int n_long = std::max(
          n_short + 1,
          static_cast<int>(std::lround(options.crossing_length_long / pitch)));
      jobs.push_back({wc, f, n_short * pitch});
      jobs.push_back({wc, f, n_long * pitch});
    }
  }
  LateralCap open_cap;
  LateralCap plane_cap;
  const int count = static_cast<int>(jobs.size());
  runJobs(options, count + 2, [&](const int i, fsv::Solver& solver) {
    if (i >= count) {
      const bool is_open = i == count;
      LateralCap& cap = is_open ? open_cap : plane_cap;
      // The plane sits where the cover is; the open wire has none.
      const Layer below = !is_open && !over ? cover : Layer();
      const Layer above = !is_open && over ? cover : Layer();
      cap = solveSection(stack,
                         layer,
                         w,
                         -1,
                         -1,
                         below,
                         above,
                         options,
                         solver,
                         options.tol / 3);
      return;
    }
    Job& job = jobs[i];
    CrossingPattern pattern;
    pattern.layer = layer;
    pattern.cover = cover;
    pattern.width = w;
    pattern.crossing_width = job.wc;
    pattern.pitch = job.wc / job.coverage;
    pattern.length = job.length;
    pattern.extent = options.extent;
    const CrossingPattern::Array array = pattern.build(stack);
    job.length = array.length;
    const fsv::Scene scene(array.geometry, stack);
    const fsv::CapRow row
        = solver.solve(scene, array.wire, solveOptions(options.tol));
    job.total = row.total();
    job.cross = row.coupling(array.cross);
  });
  const double plane_total = plane_cap.total();
  const double plane_cross = over ? plane_cap.above : plane_cap.below;
  // The shares are relative to what the plane adds; a plane that adds
  // nothing has no shares to tabulate.
  if (!(plane_total > open_cap.total()) || !(plane_cross > 0)) {
    throw std::runtime_error("a plane on " + cover_conductor.name
                             + " adds nothing to a wire on " + conductor.name);
  }
  // Pairs of lengths give per-length values, then the share over the gap.
  std::vector<std::pair<double, CrossingShare>> points;
  for (int i = 0; i + 1 < count; i += 2) {
    const Job& a = jobs[i];
    const Job& b = jobs[i + 1];
    const double dl = b.length - a.length;
    const double total = (b.total - a.total) / dl;
    const double cross = (b.cross - a.cross) / dl;
    const double f = a.coverage;
    const double g_total
        = (total - open_cap.total()) / (plane_total - open_cap.total());
    const double g_cross = cross / plane_cross;
    const double gap = a.wc * (1 / f - 1);
    points.push_back({gap, {(g_total - f) / (1 - f), (g_cross - f) / (1 - f)}});
  }
  std::sort(points.begin(), points.end(), [](const auto& a, const auto& b) {
    return a.first < b.first;
  });
  // Points at the same gap (within rounding) average into one.
  std::vector<double> gaps;
  std::vector<CrossingShare> values;
  std::vector<int> counts;
  for (const auto& [gap, share] : points) {
    if (!gaps.empty() && gap - gaps.back() <= 1e-9 * std::max(gap, 1.0)) {
      CrossingShare& last = values.back();
      const int n = ++counts.back();
      last.total += (share.total - last.total) / n;
      last.cross += (share.cross - last.cross) / n;
      continue;
    }
    gaps.push_back(gap);
    values.push_back(share);
    counts.push_back(1);
  }
  model.addCrossing(key, CrossingTable(gaps, values));
}

void buildEnds(const fsv::Stack& stack,
               const Layer layer,
               const BuildOptions& options,
               Model& model)
{
  buildEnds(stack, layer, planCovers(stack, layer, options), options, model);
}

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
  struct Job
  {
    Layer below;
    Layer above;
    int width;
    double excess = 0;
  };
  std::vector<Job> jobs;
  std::vector<LateralKey> keys;
  for (const Layer below : plan.below) {
    for (const Layer above : plan.above) {
      const LateralKey key{
          conductor.name, keyName(stack, below), keyName(stack, above)};
      if (const EndTable* built = model.end(key)) {
        requireGrid("end table of " + key.layer, built->widths(), widths);
        continue;
      }
      keys.push_back(key);
      for (int w = 0; w < static_cast<int>(widths.size()); w++) {
        jobs.push_back({below, above, w});
      }
    }
  }
  runJobs(options, jobs.size(), [&](const int i, fsv::Solver& solver) {
    Job& job = jobs[i];
    const double w = widths[job.width];
    const double per_length = solveSection(stack,
                                           layer,
                                           w,
                                           -1,
                                           -1,
                                           job.below,
                                           job.above,
                                           options,
                                           solver,
                                           options.tol / 3)
                                  .total();
    EndPattern pattern;
    pattern.layer = layer;
    pattern.width = w;
    pattern.length = std::max(options.end_length, 5 * w);
    pattern.below = job.below;
    pattern.above = job.above;
    pattern.extent = options.extent;
    pattern.margin = options.extent;
    const EndPattern::Wire wire = pattern.build(stack);
    const fsv::Scene scene(wire.geometry, stack);
    const fsv::CapRow row
        = solver.solve(scene, wire.wire, solveOptions(options.tol));
    job.excess = (row.total() - per_length * pattern.length) / 2;
  });
  for (size_t k = 0; k < keys.size(); k++) {
    std::vector<double> excess;
    for (size_t w = 0; w < widths.size(); w++) {
      excess.push_back(jobs[k * widths.size() + w].excess);
    }
    model.addEnd(keys[k], EndTable(widths, excess));
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
                   Model& model)
{
  const fsv::Stack::Conductor& conductor = requireConductor(stack, layer);
  const fsv::Stack::Conductor& other_conductor = requireConductor(stack, other);
  const DiagonalKey key{conductor.name, other_conductor.name};
  const double w = conductor.min_width;
  std::vector<double> widths;
  for (const double f : options.diagonal_width_factors) {
    widths.push_back(f * other_conductor.min_width);
  }
  std::vector<double> gaps;
  for (const double f : options.diagonal_gap_factors) {
    gaps.push_back(f * w);
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
          return pattern.build(stack);
        };
  runJobs(options, points + ns * nf, [&](const int i, fsv::Solver& solver) {
    if (i >= points) {
      const int b = i - points;
      const DiagonalPattern::Section d = section(b / nf, b % nf, 0, 0);
      const fsv::Scene2D scene(d.geometry, stack);
      const fsv::CapRow row
          = solver.solve(scene, d.wire, solveOptions(options.tol / 3));
      bases[b]
          = {row.total(), d.right != kNullNetIndex ? row.coupling(d.right) : 0};
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
  for (const std::vector<Layer>* side : {&plan.below, &plan.above}) {
    for (const Layer cover : *side) {
      if (cover.valid()) {
        buildCrossing(stack, layer, cover, options, model);
      }
    }
  }
  for (const Layer other : diagonalLayers(stack, layer, options)) {
    buildDiagonal(stack, layer, other, options, model);
  }
}

}  // namespace pex
