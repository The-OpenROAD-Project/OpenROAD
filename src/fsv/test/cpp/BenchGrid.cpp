// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Throughput on a dense two-layer wire grid, where the nearest-conductor
// query dominates: M1 wires along x crossing M2 wires along y, 0.1 µm wide on
// a 0.2 µm pitch, 8 µm long. The master is the middle M1 wire. Unequal wire
// counts per layer exercise the spatial index's layer split.
//
// Usage: BenchGrid [threads [log2_walks [m1_wires m2_wires]]]  (default 40 40)

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "fsv/Geometry.h"
#include "fsv/Scene.h"
#include "fsv/Solver.h"

int main(int argc, char** argv)
{
  const int threads = argc > 1 ? std::atoi(argv[1]) : 1;
  const int log2_walks = argc > 2 ? std::atoi(argv[2]) : 21;
  const int m1_wires = argc > 4 ? std::atoi(argv[3]) : 40;
  const int m2_wires = argc > 4 ? std::atoi(argv[4]) : 40;

  fsv::Geometry geometry;
  int master = -1;
  for (int i = 0; i < m1_wires; i++) {
    const double y = 8.0 * i / m1_wires;
    const int m1 = geometry.addNet("m1_" + std::to_string(i));
    geometry.addBox(m1, {{0, y, 0.5}, {8, y + 0.1, 0.6}});
    if (i == m1_wires / 2) {
      master = m1;
    }
  }
  for (int i = 0; i < m2_wires; i++) {
    const double x = 8.0 * i / m2_wires;
    const int m2 = geometry.addNet("m2_" + std::to_string(i));
    geometry.addBox(m2, {{x, 0, 0.8}, {x + 0.1, 8, 0.9}});
  }
  const fsv::Scene scene(geometry);

  fsv::SolveOptions options;
  options.min_walks = options.max_walks = int64_t{1} << log2_walks;
  fsv::Solver solver({.threads = threads});
  const auto start = std::chrono::steady_clock::now();
  const fsv::CapRow row = solver.solve(scene, master, options);
  const double secs
      = std::chrono::duration<double>(std::chrono::steady_clock::now() - start)
            .count();
  std::printf(
      "threads %d  %.3f s  %.0f walks/s  %.0f ns/hop  hops/walk %.1f  "
      "C %.5f +- %.5f fF\n",
      threads,
      secs,
      row.walks / secs,
      1e9 * secs * threads / row.hops,
      static_cast<double>(row.hops) / row.walks,
      row.total(),
      row.sigma[master]);
  return 0;
}
