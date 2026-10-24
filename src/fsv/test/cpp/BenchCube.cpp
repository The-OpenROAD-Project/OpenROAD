// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Accuracy and thread scaling of the solver on a unit cube high above the
// ground plane, where the capacitance is known analytically.
//
// Usage: BenchCube                       accuracy and scaling report
//        BenchCube threads [batch round [log2_walks]]
//                                        one fixed_size-size solve, for
//                                        profiling

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <numbers>

#include "fsv/Geometry.h"
#include "fsv/Scene.h"
#include "fsv/Solver.h"

namespace {

double secondsSince(const std::chrono::steady_clock::time_point start)
{
  return std::chrono::duration<double>(std::chrono::steady_clock::now() - start)
      .count();
}

}  // namespace

int main(int argc, char** argv)
{
  constexpr double kEps0 = 8.8541878128e-3;  // fF/µm
  constexpr double kUnitCube = 0.6606785;
  const double height = 50;

  fsv::Geometry geometry;
  const int net = geometry.addNet("cube");
  geometry.addBox(net, {{-0.5, -0.5, height - 0.5}, {0.5, 0.5, height + 0.5}});
  const fsv::Scene scene(geometry);
  const double expected = kUnitCube * 4 * std::numbers::pi * kEps0
                          / (1 - kUnitCube / (2 * height));

  fsv::SolveOptions fixed_size;
  fixed_size.min_walks = fixed_size.max_walks = int64_t{1} << 24;

  // With a thread count argument, run only a fixed_size-size solve.
  if (argc > 1) {
    fsv::SolverOptions options{.threads = std::atoi(argv[1])};
    if (argc > 3) {
      options.batch_size = std::atoi(argv[2]);
      options.round_batches = std::atoi(argv[3]);
    }
    if (argc > 4) {
      fixed_size.min_walks = fixed_size.max_walks = int64_t{1}
                                                    << std::atoi(argv[4]);
    }
    fsv::Solver solver(options);
    const auto start = std::chrono::steady_clock::now();
    const fsv::CapRow r = solver.solve(scene, net, fixed_size);
    std::printf("threads %d  %.3f s  C %.6f\n",
                options.threads,
                secondsSince(start),
                r.total());
    return 0;
  }

  // Accuracy at a tight tolerance.
  {
    const auto start = std::chrono::steady_clock::now();
    fsv::Solver solver({.threads = 64});
    std::printf("solver setup %.3f s (tables + 64-thread pool)\n",
                secondsSince(start));
    fsv::SolveOptions tight;
    tight.tol_total = 2e-4;
    const fsv::CapRow row = solver.solve(scene, net, tight);
    std::printf(
        "expected %.6f fF  got %.6f +- %.6f fF  bias %+.4f%% (%.1f sigma)\n",
        expected,
        row.total(),
        row.sigma[net],
        100 * (row.total() / expected - 1),
        (row.total() - expected) / row.sigma[net]);
    std::printf("walks %lld  hops/walk %.1f\n",
                static_cast<long long>(row.walks),
                static_cast<double>(row.hops) / row.walks);
  }

  // Thread scaling at a fixed_size walk count.
  double base = 0;
  for (const int threads : {1, 2, 4, 8, 16, 32, 64}) {
    fsv::Solver solver({.threads = threads});
    const auto start = std::chrono::steady_clock::now();
    const fsv::CapRow r = solver.solve(scene, net, fixed_size);
    const double secs = secondsSince(start);
    if (threads == 1) {
      base = secs;
    }
    std::printf("threads %2d  %7.3f s  speedup %5.1f  %.0f walks/s  C %.6f\n",
                threads,
                secs,
                base / secs,
                r.walks / secs,
                r.total());
  }
  return 0;
}
