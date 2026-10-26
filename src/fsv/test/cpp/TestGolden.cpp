// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Compares fsv with reference capacitances on structures with planar
// dielectric stacks. The structures and the references live in
// src/fsv/test/golden:
// - cross sections, with planar and conformal dielectrics, against a 2D
//   finite-volume solution (cross_section.txt, from fv2d_golden.py),
//   accurate to about 1e-3: in the 2D mode, and some as long lines in 3D;
// - 3D structures (a crossing, and line ends and a crossing with conformal
//   dielectrics) against a 3D finite-volume solution (box_3d.txt, from
//   fv3d_golden.py), accurate to about 2e-3.

#include <cmath>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "Structures.h"
#include "fsv/Geometry.h"
#include "fsv/Scene.h"
#include "fsv/Solver.h"
#include "fsv/Stack.h"
#include "gtest/gtest.h"

namespace fsv {
namespace {

using Golden
    = std::map<std::tuple<std::string, std::string, std::string>, double>;

// (structure, master, net) -> Maxwell matrix entry; net "ground" is the
// plane.
Golden readGolden(const std::string& file)
{
  Golden golden;
  std::ifstream in(std::string(kGoldenDir) + file);
  EXPECT_TRUE(in.good()) << file;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    std::istringstream words(line);
    std::string structure;
    std::string master;
    std::string net;
    double value = 0;
    words >> structure >> master >> net >> value;
    golden[{structure, master, net}] = value;
  }
  return golden;
}

// One matrix entry from fsv: value and standard error.
struct Entry
{
  double value;
  double sigma;
};

Entry entry(const CapRow& row, const int net)
{
  if (net == static_cast<int>(row.cap.size())) {
    return {row.ground, row.ground_sigma};
  }
  return {row.cap[net], row.sigma[net]};
}

std::string netName(const Geometry& geometry, const int net)
{
  return net == geometry.netCount() ? "ground" : geometry.netName(net);
}

int netIndex(const Geometry& geometry, const std::string& name)
{
  for (int net = 0; net < geometry.netCount(); net++) {
    if (geometry.netName(net) == name) {
      return net;
    }
  }
  ADD_FAILURE() << "no net " << name;
  return 0;
}

SolveOptions solveOptions(const double tol_total)
{
  SolveOptions options;
  options.tol_total = tol_total;
  options.tol_coupling = tol_total;
  return options;
}

struct CrossSectionCase
{
  std::string structure;
  std::vector<std::string> masters;
};

const auto kCaseName
    = [](const testing::TestParamInfo<CrossSectionCase>& info) {
        return info.param.structure;
      };

class TwoD : public testing::TestWithParam<CrossSectionCase>
{
};

// The 2D mode on each cross section: totals within 0.3% and couplings of at
// least 5% of the total within 0.5%, each plus three standard errors; the
// tables' panel discretization is about 1e-3.
TEST_P(TwoD, MatchesFiniteVolume)
{
  const std::string& name = GetParam().structure;
  const Structure s = readStructure(name);
  const Geometry2D section = crossSection(s.geometry);
  const Geometry& geometry = s.geometry;
  const Golden golden = readGolden("cross_section.txt");
  const Scene2D scene(section, s.stack);
  Solver solver({.threads = 8});
  const SolveOptions options = solveOptions(1e-3);
  for (const std::string& m : GetParam().masters) {
    const int master = netIndex(geometry, m);
    const CapRow row = solver.solve(scene, master, options);
    const double total = golden.at({name, m, m});
    for (int net = 0; net <= geometry.netCount(); net++) {
      const std::string n = netName(geometry, net);
      const double expected = golden.at({name, m, n});
      if (net != master && std::abs(expected) < 0.05 * total) {
        continue;
      }
      const Entry e = entry(row, net);
      const double tol = net == master ? 3e-3 : 5e-3;
      EXPECT_NEAR(e.value, expected, tol * std::abs(expected) + 3 * e.sigma)
          << name << " " << m << " to " << n;
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    Planar,
    TwoD,
    testing::Values(CrossSectionCase{"etch_stop_pair", {"lower", "upper"}},
                    CrossSectionCase{"three_wires", {"left", "middle"}},
                    CrossSectionCase{"conformal_pair", {"a"}},
                    CrossSectionCase{"nested_conformal", {"a"}},
                    CrossSectionCase{"merged_conformal", {"a", "b"}},
                    CrossSectionCase{"sky130_met1_pair", {"a"}}),
    kCaseName);

class LongLine : public testing::TestWithParam<CrossSectionCase>
{
};

// The 3D solver per unit length on the same cross sections: the difference
// of two wire lengths cancels the end effects and leaves the per-unit-length
// matrix, provided each wire's two ends do not interact: at 5 um they still
// do for the upper etch_stop_pair wire (+0.8% on its ground). The same
// tolerances as TwoD. Masters are listed per structure; three_wires
// skips "right", the mirror image of "left".
TEST_P(LongLine, MatchesFiniteVolume)
{
  constexpr double kShort = 10;
  constexpr double kLong = 30;
  const std::string& name = GetParam().structure;
  const Structure short_wires = readStructure(name, kShort);
  const Structure long_wires = readStructure(name, kLong);
  const Geometry& geometry = short_wires.geometry;
  const Golden golden = readGolden("cross_section.txt");
  const Scene short_scene(short_wires.geometry, short_wires.stack);
  const Scene long_scene(long_wires.geometry, long_wires.stack);
  Solver solver({.threads = 8});
  const SolveOptions options = solveOptions(1e-3);
  for (const std::string& m : GetParam().masters) {
    const int master = netIndex(geometry, m);
    const CapRow a = solver.solve(short_scene, master, options);
    const CapRow b = solver.solve(long_scene, master, options);
    const double total = golden.at({name, m, m});
    for (int net = 0; net <= geometry.netCount(); net++) {
      const std::string n = netName(geometry, net);
      const double expected = golden.at({name, m, n});
      if (net != master && std::abs(expected) < 0.05 * total) {
        continue;
      }
      const Entry ea = entry(a, net);
      const Entry eb = entry(b, net);
      const double value = (eb.value - ea.value) / (kLong - kShort);
      const double sigma = std::hypot(ea.sigma, eb.sigma) / (kLong - kShort);
      const double tol = net == master ? 3e-3 : 5e-3;
      EXPECT_NEAR(value, expected, tol * std::abs(expected) + 3 * sigma)
          << name << " " << m << " to " << n;
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    Planar,
    LongLine,
    testing::Values(CrossSectionCase{"etch_stop_pair", {"lower", "upper"}},
                    CrossSectionCase{"three_wires", {"left", "middle"}},
                    CrossSectionCase{"conformal_pair", {"a"}},
                    CrossSectionCase{"sky130_met1_pair", {"a"}}),
    kCaseName);

class ThreeD : public testing::TestWithParam<std::string>
{
};

// 3D structures against a 3D finite-volume solution (box_3d.txt, from
// fv3d_golden.py): a crossing, and line ends and a crossing with conformal
// dielectrics, which cross sections cannot show. Totals within 0.5% and
// couplings of at least 5% of the total within 1%, each plus three standard
// errors; the reference is good to about 0.2%.
TEST_P(ThreeD, MatchesFiniteVolume)
{
  const std::string name = GetParam();
  const Structure s = readStructure(name);
  const Geometry& geometry = s.geometry;
  const Golden golden = readGolden("box_3d.txt");
  const Scene scene(geometry, s.stack);
  Solver solver({.threads = 8});
  const SolveOptions options = solveOptions(2e-3);
  for (int master = 0; master < geometry.netCount(); master++) {
    const std::string& m = geometry.netName(master);
    const CapRow row = solver.solve(scene, master, options);
    const double total = golden.at({name, m, m});
    for (int net = 0; net <= geometry.netCount(); net++) {
      const std::string n = netName(geometry, net);
      const double expected = golden.at({name, m, n});
      if (net != master && std::abs(expected) < 0.05 * total) {
        continue;
      }
      const Entry e = entry(row, net);
      const double tol = net == master ? 5e-3 : 1e-2;
      EXPECT_NEAR(e.value, expected, tol * std::abs(expected) + 3 * e.sigma)
          << name << " " << m << " to " << n;
    }
  }
}

INSTANTIATE_TEST_SUITE_P(Conformal,
                         ThreeD,
                         testing::Values("crossing",
                                         "conformal_line_ends",
                                         "conformal_crossing"));

}  // namespace
}  // namespace fsv
