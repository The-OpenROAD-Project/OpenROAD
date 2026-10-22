// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <vector>

#include "BoxFace.h"
#include "CubeGreens.h"
#include "CubePanels.h"
#include "LayeredCubeGreens.h"
#include "Philox.h"
#include "fsv/Geometry.h"
#include "gtest/gtest.h"

namespace fsv {
namespace {

using Field = std::function<double(const Vec3&)>;

constexpr int kK = LayeredCubeGreens::kPositions;
// Permittivity ratios eps_above / eps_below to test, including SkyWater
// 130's largest contrasts (TOPNIT 7.5 / PI1 2.94, PI1 2.94 / air 1.0) and
// extreme ones (35, 1 / 35), whose side-face roots crowd the poles of the
// eigenvalue condition.
const std::vector<double> kRatios
    = {1.0, 7.5 / 2.94, 2.94 / 7.5, 1 / 2.94, 10.0, 35.0, 1 / 35.0};

double probability(const LayeredCubeGreens& g,
                   const BoxFace face,
                   const int i,
                   const int j)
{
  if (face.axis != Axis::kZ) {
    return g.sideProbability(i, j);
  }
  return face.high ? g.topProbability(i, j) : g.bottomProbability(i, j);
}

// Sum over the surface of P * f, with f averaged over each panel by 3x3
// Gauss-Legendre; this equals f at the center for fields that satisfy the
// interface conditions.
double average(const LayeredCubeGreens& g, const Field& f)
{
  static const double kNodes[3] = {-std::sqrt(0.6), 0.0, std::sqrt(0.6)};
  static const double kWeights[3] = {5.0 / 18, 8.0 / 18, 5.0 / 18};
  const int n = g.panels();
  const double h = 2.0 / n;
  double sum = 0;
  for (int fi = 0; fi < BoxFace::kCount; fi++) {
    const BoxFace face = BoxFace::fromIndex(fi);
    const auto [t0, t1] = tangentAxes(face.axis);
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        double panel = 0;
        for (int a = 0; a < 3; a++) {
          for (int b = 0; b < 3; b++) {
            Vec3 p;
            p[index(face.axis)] = face.normalSign();
            p[index(t0)] = -1.0 + h * (i + 0.5 + 0.5 * kNodes[a]);
            p[index(t1)] = -1.0 + h * (j + 0.5 + 0.5 * kNodes[b]);
            panel += kWeights[a] * kWeights[b] * f(p);
          }
        }
        sum += probability(g, face, i, j) * panel;
      }
    }
  }
  return sum;
}

double total(const LayeredCubeGreens& g)
{
  return average(g, [](const Vec3&) { return 1.0; });
}

// Piecewise-linear potential with continuous eps * dphi/dz: (z - t) / eps(z).
Field linearField(const double t, const double ratio)
{
  return [=](const Vec3& p) { return (p[2] - t) / (p[2] < t ? 1.0 : ratio); };
}

// cos(alpha x) * (cosh(alpha (z - t)) + sinh(alpha (z - t)) / eps(z)): it
// satisfies Laplace's equation in each layer and the interface conditions.
Field hyperbolicField(const double t, const double ratio)
{
  constexpr double kAlpha = 1.3;
  return [=](const Vec3& p) {
    const double e = p[2] < t ? 1.0 : ratio;
    const double s = kAlpha * (p[2] - t);
    return std::cos(kAlpha * p[0]) * (std::cosh(s) + std::sinh(s) / e);
  };
}

TEST(LayeredCubeGreens, ProbabilitiesSumToOne)
{
  for (const double ratio : kRatios) {
    for (int k = 1 - kK; k < kK; k++) {
      const LayeredCubeGreens g(32, k, ratio);
      EXPECT_NEAR(total(g), 1.0, 1e-10) << "ratio " << ratio << " k " << k;
    }
  }
}

// Equal permittivities give the homogeneous tables at every position.
TEST(LayeredCubeGreens, EqualPermittivitiesMatchHomogeneous)
{
  const CubeGreens homogeneous(32);
  for (int k = 1 - kK; k < kK; k++) {
    const LayeredCubeGreens g(32, k, 1.0);
    for (int i = 0; i < 32; i++) {
      for (int j = 0; j < 32; j++) {
        const double p = homogeneous.panelProbability(i, j);
        ASSERT_NEAR(g.bottomProbability(i, j), p, 1e-13);
        ASSERT_NEAR(g.topProbability(i, j), p, 1e-13);
        ASSERT_NEAR(g.sideProbability(i, j), p, 1e-13);
      }
    }
  }
}

// Flipping the cube over (z -> -z) moves the interface to -t and swaps the
// two permittivities.
TEST(LayeredCubeGreens, MirrorSymmetry)
{
  const double ratio = 7.5 / 2.94;
  for (int k = 1 - kK; k < kK; k++) {
    const LayeredCubeGreens g(32, k, ratio);
    const LayeredCubeGreens flipped(32, -k, 1 / ratio);
    for (int i = 0; i < 32; i++) {
      for (int j = 0; j < 32; j++) {
        ASSERT_NEAR(
            g.topProbability(i, j), flipped.bottomProbability(i, j), 1e-13);
        ASSERT_NEAR(
            g.sideProbability(i, j), flipped.sideProbability(i, 31 - j), 1e-13);
      }
    }
  }
}

// Fields that satisfy the interface conditions average to their center
// value. The only error is panel discretization, second order in the panel
// size. It is larger than in a uniform cube (about 1e-3 against fields of
// order 1-5 at 32 panels) because the fields change slope at the interface.
// The fields grow as 1 / ratio above the interface, and the tolerances with
// them.
TEST(LayeredCubeGreens, ExactFieldsAverageToCenter)
{
  for (const double ratio : {7.5 / 2.94, 1 / 2.94, 35.0, 1 / 35.0}) {
    const double scale = std::max(1.0, 1 / ratio);
    for (const int k : {-5, 0, 3, 7}) {
      const double t = static_cast<double>(k) / kK;
      for (const auto& make : {linearField, hyperbolicField}) {
        const Field f = make(t, ratio);
        const double center = f({0, 0, 0});
        double prev = 0;
        for (const int n : {32, 64}) {
          const double err
              = std::abs(average(LayeredCubeGreens(n, k, ratio), f) - center);
          EXPECT_LT(err, (n == 32 ? 2e-3 : 5e-4) * scale)
              << "ratio " << ratio << " k " << k << " n " << n;
          if (n == 64 && prev > 1e-9) {
            EXPECT_LT(err, prev / 3) << "ratio " << ratio << " k " << k;
          }
          prev = err;
        }
      }
    }
  }
}

// Sampling reproduces the tables: the Monte Carlo mean agrees within 4 sigma.
TEST(LayeredCubeGreens, SamplingMatchesTables)
{
  const double ratio = 1 / 2.94;
  const LayeredCubeGreens g(32, 3, ratio);
  const Field f = hyperbolicField(g.interfaceZ(), ratio);
  Philox rng(11, 0);
  const int samples = 2000000;
  double sum = 0;
  double sum2 = 0;
  for (int s = 0; s < samples; s++) {
    const double v = f(g.sampleTransition(rng));
    sum += v;
    sum2 += v * v;
  }
  const double mean = sum / samples;
  const double sigma = std::sqrt((sum2 / samples - mean * mean) / samples);
  EXPECT_NEAR(mean, average(g, f), 4 * sigma);
}

// Sum over the surface of flux(axis) * f, with f averaged over each panel;
// this equals eps(center) * df/d(axis) at the center for exact fields.
double fluxAverage(const LayeredCubeGreens& g, const Axis axis, const Field& f)
{
  static const double kNodes[3] = {-std::sqrt(0.6), 0.0, std::sqrt(0.6)};
  static const double kWeights[3] = {5.0 / 18, 8.0 / 18, 5.0 / 18};
  const int n = g.panels();
  const double h = 2.0 / n;
  double sum = 0;
  for (int fi = 0; fi < BoxFace::kCount; fi++) {
    const BoxFace face = BoxFace::fromIndex(fi);
    const auto [t0, t1] = tangentAxes(face.axis);
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        double panel = 0;
        for (int a = 0; a < 3; a++) {
          for (int b = 0; b < 3; b++) {
            Vec3 p;
            p[index(face.axis)] = face.normalSign();
            p[index(t0)] = -1.0 + h * (i + 0.5 + 0.5 * kNodes[a]);
            p[index(t1)] = -1.0 + h * (j + 0.5 + 0.5 * kNodes[b]);
            panel += kWeights[a] * kWeights[b] * f(p);
          }
        }
        sum += g.flux(axis, face, i, j) * panel;
      }
    }
  }
  return sum;
}

// sin(alpha x) * (cosh(alpha (z - t)) + sinh(alpha (z - t)) / eps(z)).
Field sineField(const double t, const double ratio)
{
  constexpr double kAlpha = 1.3;
  return [=](const Vec3& p) {
    const double e = p[2] < t ? 1.0 : ratio;
    const double s = kAlpha * (p[2] - t);
    return std::sin(kAlpha * p[0]) * (std::cosh(s) + std::sinh(s) / e);
  };
}

// With equal permittivities the flux kernels are the homogeneous gradient
// tables, mapped to each face and axis.
TEST(LayeredCubeGreens, EqualPermittivityFluxMatchesHomogeneous)
{
  const CubeGreens h(32);
  const BoxFace x_low{Axis::kX, false};
  const BoxFace y_low{Axis::kY, false};
  const BoxFace z_low{Axis::kZ, false};
  const BoxFace z_high{Axis::kZ, true};
  for (const int k : {-3, 0, 5}) {
    const LayeredCubeGreens g(32, k, 1.0);
    for (int i = 0; i < 32; i++) {
      for (int j = 0; j < 32; j++) {
        const double normal = h.normalFaceGradient(i, j);
        ASSERT_NEAR(g.flux(Axis::kZ, z_low, i, j), normal, 1e-12);
        ASSERT_NEAR(g.flux(Axis::kZ, z_high, i, j), -normal, 1e-12);
        ASSERT_NEAR(
            g.flux(Axis::kZ, x_low, i, j), h.parallelFaceGradient(i, j), 1e-12);
        ASSERT_NEAR(g.flux(Axis::kX, x_low, i, j), normal, 1e-12);
        ASSERT_NEAR(
            g.flux(Axis::kX, y_low, i, j), h.parallelFaceGradient(j, i), 1e-12);
        ASSERT_NEAR(
            g.flux(Axis::kX, z_low, i, j), h.parallelFaceGradient(j, i), 1e-12);
        ASSERT_NEAR(g.flux(Axis::kY, y_low, i, j), normal, 1e-12);
        ASSERT_NEAR(
            g.flux(Axis::kY, x_low, i, j), h.parallelFaceGradient(j, i), 1e-12);
        ASSERT_NEAR(
            g.flux(Axis::kY, z_low, i, j), h.parallelFaceGradient(i, j), 1e-12);
      }
    }
  }
}

// Exact fields reproduce the kernels' center values: eps * df/dz = 1 for
// the piecewise-linear field, and df/dx = alpha * g(0) for the sine field. A
// constant has no flux. The sine field grows as 1 / ratio above the
// interface, and its tolerance with it.
TEST(LayeredCubeGreens, FluxOfExactFields)
{
  for (const double ratio : {7.5 / 2.94, 1 / 2.94, 35.0, 1 / 35.0}) {
    const double scale = std::max(1.0, 1 / ratio);
    for (const int k : {-5, 0, 3, 7}) {
      const LayeredCubeGreens g(32, k, ratio);
      const double t = g.interfaceZ();
      for (const Axis axis : kAxes) {
        EXPECT_NEAR(
            fluxAverage(g, axis, [](const Vec3&) { return 1.0; }), 0.0, 1e-12);
      }
      EXPECT_NEAR(fluxAverage(g, Axis::kZ, linearField(t, ratio)), 1.0, 2e-3)
          << "ratio " << ratio << " k " << k;
      // The sine field's center value depends on the side for t = 0, where
      // it is not defined, so only t != 0 is checked.
      if (k != 0) {
        const double eps = t > 0 ? 1.0 : ratio;
        const double s = -1.3 * t;
        const double g0 = std::cosh(s) + std::sinh(s) / eps;
        EXPECT_NEAR(fluxAverage(g, Axis::kX, sineField(t, ratio)),
                    1.3 * g0,
                    5e-3 * scale)
            << "ratio " << ratio << " k " << k;
      }
    }
  }
}

// First-hop sampling reproduces the flux tables.
TEST(LayeredCubeGreens, FirstHopSamplingMatchesTables)
{
  const double ratio = 7.5 / 2.94;
  const LayeredCubeGreens g(32, -3, ratio);
  const Field f = sineField(g.interfaceZ(), ratio);
  Philox rng(13, 0);
  const int samples = 2000000;
  for (const Axis axis : {Axis::kX, Axis::kZ}) {
    double sum = 0;
    double sum2 = 0;
    for (int s = 0; s < samples; s++) {
      const FirstHop hop = g.sampleFirstHop(axis, rng);
      const double v = hop.sign * g.fluxNorm(axis) * f(hop.offset);
      sum += v;
      sum2 += v * v;
    }
    const double mean = sum / samples;
    const double sigma = std::sqrt((sum2 / samples - mean * mean) / samples);
    EXPECT_NEAR(mean, fluxAverage(g, axis, f), 4 * sigma);
  }
}

TEST(LayeredCubeGreens, RejectsBadArguments)
{
  EXPECT_THROW(LayeredCubeGreens(24, 0, 2.0), std::invalid_argument);
  EXPECT_THROW(LayeredCubeGreens(32, kK, 2.0), std::invalid_argument);
  EXPECT_THROW(LayeredCubeGreens(32, 0, 0.0), std::invalid_argument);
}

}  // namespace
}  // namespace fsv
