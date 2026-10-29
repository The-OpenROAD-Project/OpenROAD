// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include "fsv/Scene.h"

namespace fsv {

class CubeGreens;
class LayeredCubeGreens;
class LayeredSquareGreens;
class SquareGreens;
class WorkerPool;

// Settings fixed for the life of a Solver. Results depend only on these, the
// scene and the per-solve options; never on the thread count.
struct SolverOptions
{
  int threads = 1;
  uint64_t seed = 1;
  // Panels per cube-face edge in the Green's-function tables: a positive
  // multiple of 16, so that every tabulated interface position lies on a
  // panel boundary.
  int panels = 32;
  // Walks per batch, the batches of the first round, and the most in any
  // round. The stopping test runs after each round; later rounds are sized
  // to the walks it still needs, at least first_round_batches.
  int batch_size = 1024;
  int first_round_batches = 16;
  int round_batches = 256;

  bool operator==(const SolverOptions&) const = default;
};

// Accuracy settings for one solve.
struct SolveOptions
{
  // Stop when the total's standard error is below tol_total times the
  // total, and every coupling's (the ground's included) below tol_coupling
  // times the total; unset, tol_coupling is tol_total.
  double tol_total = 0.01;
  std::optional<double> tol_coupling = std::nullopt;
  int64_t min_walks = 16384;
  int64_t max_walks = 100000000;
  // Gaussian-surface offset as a fraction of the master's smallest dimension,
  // capped at half the gap to the nearest other conductor or the ground.
  double gauss_offset = 0.5;
  // Capture distance as a fraction of the scene's smallest box dimension.
  double capture_rel = 1e-4;
  // A walk exceeding this many hops makes solve() throw: dropping it would
  // bias the result. Must be positive.
  int64_t hop_guard = 1000000;
};

// One row of the Maxwell capacitance matrix, in fF (fF/µm for a Scene2D).
// cap[master] is the total capacitance; cap[j] for j != master is minus the
// coupling to net j. A grounded net's coupling counts toward ground, and its
// own entry is 0.
struct CapRow
{
  int master = -1;
  std::vector<double> cap;
  std::vector<double> sigma;
  double ground = 0;  // minus the coupling to the ground plane
  double ground_sigma = 0;
  // For a geometry with a window: the summed |weight| per walk of the walks
  // whose cubes reached outside it and that then ended on a net rather than
  // the ground plane. It estimates, conservatively, how much the total and
  // the couplings to nets in the window could change if the conductors
  // outside it were present (on parallel wires, 20-70 times the change),
  // and falls about as the square of the window's margin. The ground entry
  // stands in for the conductors outside and is not covered.
  double window_leakage = 0;
  int64_t walks = 0;
  int64_t hops = 0;
  // Around conformal shells a trial may choose a Gaussian surface inside
  // the target's own shells: whether it did, and the walks the trial took
  // (0 when none ran). Trial walks are not in `walks` or `hops`.
  bool inside_surface = false;
  int64_t trial_walks = 0;

  double total() const { return cap[master]; }
  double coupling(int net) const { return -cap[net]; }
};

// Reusable solver: holds the Green's-function tables (shared process-wide by
// panel count, built on first use) and a persistent worker pool. Create one per
// run and call solve() for any number of scenes and masters. Calls to solve()
// on the same Solver are serialized.
class Solver
{
 public:
  explicit Solver(const SolverOptions& options = {});
  ~Solver();

  // Throws std::invalid_argument for a bad master or options, and
  // std::runtime_error if any walk exceeds SolveOptions::hop_guard.
  CapRow solve(const Scene& scene,
               int master,
               const SolveOptions& options = {});
  // A cross section: the row is per unit length, in fF/µm.
  CapRow solve(const Scene2D& scene,
               int master,
               const SolveOptions& options = {});

 private:
  SolverOptions options_;
  // Built by the first solve that needs them.
  std::shared_ptr<const CubeGreens> greens_;
  std::shared_ptr<const SquareGreens> square_greens_;
  // Held so that the tables are reused across solves.
  std::map<double, std::vector<std::shared_ptr<const LayeredCubeGreens>>>
      layered_;
  std::map<double, std::vector<std::shared_ptr<const LayeredSquareGreens>>>
      layered_squares_;
  std::unique_ptr<WorkerPool> pool_;
  std::mutex solve_mutex_;
  // Guards layered_ and layered_squares_ while walks build tables.
  std::mutex tables_mutex_;
};

}  // namespace fsv
