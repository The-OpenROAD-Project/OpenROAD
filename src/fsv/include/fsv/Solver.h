// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "fsv/Scene.h"

namespace fsv {

class CubeGreens;
class LayeredCubeGreens;
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
  // Walks per batch, and batches in the first and in later rounds; rounds
  // double from the first size up to round_batches. The stopping test runs
  // after each round.
  int batch_size = 1024;
  int first_round_batches = 16;
  int round_batches = 256;

  bool operator==(const SolverOptions&) const = default;
};

// Accuracy settings for one solve.
struct SolveOptions
{
  // Stop when sigma/C_total falls below this.
  double tol_total = 0.01;
  // ... and sigma/|C_ij| falls below this for every coupling at least
  // coupling_threshold * C_total.
  double tol_coupling = 0.05;
  double coupling_threshold = 0.01;
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

// One row of the Maxwell capacitance matrix, in fF. cap[master] is the total
// capacitance; cap[j] for j != master is minus the coupling to net j.
struct CapRow
{
  int master = -1;
  std::vector<double> cap;
  std::vector<double> sigma;
  double ground = 0;  // minus the coupling to the ground plane
  double ground_sigma = 0;
  int64_t walks = 0;
  int64_t hops = 0;

  double total() const { return cap[master]; }
  double coupling(int net) const { return -cap[net]; }
};

// Reusable solver: holds the Green's-function tables (shared process-wide by
// panel count) and a persistent worker pool. Create one per run and call
// solve() for any number of scenes and masters. Calls to solve() on the same
// Solver are serialized.
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

 private:
  // Two-dielectric tables for one permittivity ratio, by interface position.
  const std::vector<std::shared_ptr<const LayeredCubeGreens>>& layeredTables(
      double ratio);

  SolverOptions options_;
  std::shared_ptr<const CubeGreens> greens_;
  // Held so that the tables are reused across solves.
  std::map<double, std::vector<std::shared_ptr<const LayeredCubeGreens>>>
      layered_;
  std::unique_ptr<WorkerPool> pool_;
  std::mutex solve_mutex_;
};

}  // namespace fsv
