// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <vector>

#include "CubePanels.h"
#include "LayeredSquareGreens.h"
#include "Philox.h"
#include "SquareGreens.h"
#include "fsv/Geometry.h"
#include "gtest/gtest.h"

namespace fsv {
namespace {

using Field = std::function<double(const Vec2&)>;

constexpr int kK = LayeredSquareGreens::kPositions;

// Average of f over panel i of an edge, by 3-point Gauss-Legendre.
double panelAverage(const Field& f,
                    const int n,
                    const SquareEdge edge,
                    const int i)
{
  static const double kNodes[3] = {-std::sqrt(0.6), 0.0, std::sqrt(0.6)};
  static const double kWeights[3] = {5.0 / 18, 8.0 / 18, 5.0 / 18};
  double sum = 0;
  for (int a = 0; a < 3; a++) {
    sum += kWeights[a] * f(squareEdgePoint(edge, n, i, 0.5 + 0.5 * kNodes[a]));
  }
  return sum;
}

// Sum over the boundary of `weight(edge, i)` times f on the panel.
double boundarySum(const int n,
                   const std::function<double(SquareEdge, int)>& weight,
                   const Field& f)
{
  double sum = 0;
  for (int e = 0; e < kSquareEdges; e++) {
    const SquareEdge edge = squareEdge(e);
    for (int i = 0; i < n; i++) {
      sum += weight(edge, i) * panelAverage(f, n, edge, i);
    }
  }
  return sum;
}

// Harmonic in the x-z plane.
double harmonic(const Vec2& p)
{
  return std::exp(1.1 * p[0]) * std::cos(1.1 * p[1]) + p[0] * p[1]
         + 0.3 * (p[0] * p[0] - p[1] * p[1]);
}

TEST(SquareGreens, ProbabilitiesSumToOne)
{
  const SquareGreens g(32);
  double edge = 0;
  for (int i = 0; i < g.panels(); i++) {
    edge += g.panelProbability(i);
    EXPECT_NEAR(
        g.panelProbability(i), g.panelProbability(g.panels() - 1 - i), 1e-15);
  }
  EXPECT_NEAR(edge, 0.25, 1e-12);
}

// A harmonic function averages to its center value, and the gradient
// kernels give its gradient there. The error is second order in the panel
// size.
TEST(SquareGreens, HarmonicFields)
{
  for (const int n : {32, 64}) {
    const SquareGreens g(n);
    const double center = boundarySum(
        n,
        [&](SquareEdge, const int i) { return g.panelProbability(i); },
        harmonic);
    EXPECT_NEAR(center, harmonic({0, 0}), n == 32 ? 2e-4 : 5e-5);
    // d/dz at the center: edges normal to z use the normal-edge table
    // (negated at z = +1), edges parallel to it the parallel-edge table.
    const double dz = boundarySum(
        n,
        [&](const SquareEdge edge, const int i) {
          if (edge.axis == 1) {
            return edge.high ? -g.normalEdgeGradient(i)
                             : g.normalEdgeGradient(i);
          }
          return g.parallelEdgeGradient(i);
        },
        harmonic);
    // Every term of harmonic() has df/dz = 0 at the center.
    EXPECT_NEAR(dz, 0.0, n == 32 ? 2e-3 : 5e-4);
    const double dz_linear = boundarySum(
        n,
        [&](const SquareEdge edge, const int i) {
          if (edge.axis == 1) {
            return edge.high ? -g.normalEdgeGradient(i)
                             : g.normalEdgeGradient(i);
          }
          return g.parallelEdgeGradient(i);
        },
        [](const Vec2& p) { return p[1] + 0.5 * p[0]; });
    EXPECT_NEAR(dz_linear, 1.0, n == 32 ? 2e-3 : 5e-4);
  }
}

TEST(SquareGreens, SamplingStaysOnTheBoundary)
{
  const SquareGreens g(32);
  Philox rng(3, 0);
  double sum = 0;
  const int samples = 400000;
  for (int s = 0; s < samples; s++) {
    const Vec2 p = g.sampleTransition(rng);
    ASSERT_NEAR(std::max(std::abs(p[0]), std::abs(p[1])), 1.0, 1e-15);
    sum += harmonic(p);
  }
  EXPECT_NEAR(sum / samples, harmonic({0, 0}), 0.01);
  for (const int axis : {0, 1}) {
    const FirstHop2D hop = g.sampleFirstHop(axis, rng);
    EXPECT_TRUE(hop.sign == 1.0 || hop.sign == -1.0);
  }
}

double probability(const LayeredSquareGreens& g, const SquareEdge edge, int i)
{
  if (edge.axis == 0) {
    return g.sideProbability(i);
  }
  return edge.high ? g.topProbability(i) : g.bottomProbability(i);
}

double average(const LayeredSquareGreens& g, const Field& f)
{
  return boundarySum(
      g.panels(),
      [&](const SquareEdge edge, const int i) {
        return probability(g, edge, i);
      },
      f);
}

double fluxAverage(const LayeredSquareGreens& g, const int axis, const Field& f)
{
  return boundarySum(
      g.panels(),
      [&](const SquareEdge edge, const int i) { return g.flux(axis, edge, i); },
      f);
}

// Piecewise-linear potential with continuous eps * dphi/dz.
Field linearField(const double t, const double ratio)
{
  return [=](const Vec2& p) { return (p[1] - t) / (p[1] < t ? 1.0 : ratio); };
}

// cos(alpha x) * (cosh(alpha (z - t)) + sinh(alpha (z - t)) / eps(z)): it
// satisfies Laplace's equation in each layer and the interface conditions.
Field hyperbolicField(const double t, const double ratio)
{
  constexpr double kAlpha = 1.3;
  return [=](const Vec2& p) {
    const double e = p[1] < t ? 1.0 : ratio;
    const double s = kAlpha * (p[1] - t);
    return std::cos(kAlpha * p[0]) * (std::cosh(s) + std::sinh(s) / e);
  };
}

// The same with sin(alpha x), for the x flux.
Field sineField(const double t, const double ratio)
{
  constexpr double kAlpha = 1.3;
  return [=](const Vec2& p) {
    const double e = p[1] < t ? 1.0 : ratio;
    const double s = kAlpha * (p[1] - t);
    return std::sin(kAlpha * p[0]) * (std::cosh(s) + std::sinh(s) / e);
  };
}

const std::vector<double> kRatios
    = {1.0, 7.5 / 2.94, 2.94 / 7.5, 1 / 2.94, 10.0, 35.0, 1 / 35.0};

TEST(LayeredSquareGreens, ProbabilitiesSumToOne)
{
  for (const double ratio : kRatios) {
    for (int k = 1 - kK; k < kK; k++) {
      const LayeredSquareGreens g(32, k, ratio);
      EXPECT_NEAR(average(g, [](const Vec2&) { return 1.0; }), 1.0, 1e-10)
          << "ratio " << ratio << " k " << k;
    }
  }
}

TEST(LayeredSquareGreens, EqualPermittivitiesMatchHomogeneous)
{
  const SquareGreens homogeneous(32);
  for (int k = 1 - kK; k < kK; k++) {
    const LayeredSquareGreens g(32, k, 1.0);
    for (int i = 0; i < 32; i++) {
      const double p = homogeneous.panelProbability(i);
      ASSERT_NEAR(g.bottomProbability(i), p, 1e-13);
      ASSERT_NEAR(g.topProbability(i), p, 1e-13);
      ASSERT_NEAR(g.sideProbability(i), p, 1e-13);
      ASSERT_NEAR(g.flux(1, squareEdge(2), i),
                  homogeneous.normalEdgeGradient(i),
                  1e-13);
      ASSERT_NEAR(g.flux(1, squareEdge(0), i),
                  homogeneous.parallelEdgeGradient(i),
                  1e-13);
    }
  }
}

// Flipping the square over (z -> -z) moves the interface to -t and swaps
// the two permittivities.
TEST(LayeredSquareGreens, MirrorSymmetry)
{
  const double ratio = 7.5 / 2.94;
  for (int k = 1 - kK; k < kK; k++) {
    const LayeredSquareGreens g(32, k, ratio);
    const LayeredSquareGreens flipped(32, -k, 1 / ratio);
    for (int i = 0; i < 32; i++) {
      ASSERT_NEAR(g.topProbability(i), flipped.bottomProbability(i), 1e-13);
      ASSERT_NEAR(g.sideProbability(i), flipped.sideProbability(31 - i), 1e-13);
    }
  }
}

// Fields that satisfy the interface conditions average to their center
// value; the error is panel discretization, second order in the panel
// size, and grows with the field as 1 / ratio above the interface.
TEST(LayeredSquareGreens, ExactFieldsAverageToCenter)
{
  for (const double ratio : {7.5 / 2.94, 1 / 2.94, 35.0, 1 / 35.0}) {
    const double scale = std::max(1.0, 1 / ratio);
    for (const int k : {-5, 0, 3, 7}) {
      const double t = static_cast<double>(k) / kK;
      for (const auto& make : {linearField, hyperbolicField}) {
        const Field f = make(t, ratio);
        const double center = f({0, 0});
        double prev = 0;
        for (const int n : {32, 64}) {
          const double err
              = std::abs(average(LayeredSquareGreens(n, k, ratio), f) - center);
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

// Exact fields reproduce the kernels' center values: eps * df/dz = 1 for
// the piecewise-linear field, df/dx = alpha * g(0) for the sine field; a
// constant has no flux.
TEST(LayeredSquareGreens, FluxOfExactFields)
{
  for (const double ratio : {7.5 / 2.94, 1 / 2.94, 35.0, 1 / 35.0}) {
    const double scale = std::max(1.0, 1 / ratio);
    for (const int k : {-5, 0, 3, 7}) {
      const LayeredSquareGreens g(32, k, ratio);
      const double t = g.interfaceZ();
      for (const int axis : {0, 1}) {
        EXPECT_NEAR(
            fluxAverage(g, axis, [](const Vec2&) { return 1.0; }), 0.0, 1e-12);
      }
      EXPECT_NEAR(fluxAverage(g, 1, linearField(t, ratio)), 1.0, 2e-3)
          << "ratio " << ratio << " k " << k;
      if (k != 0) {
        const double eps = t > 0 ? 1.0 : ratio;
        const double s = -1.3 * t;
        const double g0 = std::cosh(s) + std::sinh(s) / eps;
        EXPECT_NEAR(
            fluxAverage(g, 0, sineField(t, ratio)), 1.3 * g0, 5e-3 * scale)
            << "ratio " << ratio << " k " << k;
      }
    }
  }
}

// Sampling reproduces the tables: the Monte Carlo means agree within 4
// sigma.
TEST(LayeredSquareGreens, SamplingMatchesTables)
{
  const double ratio = 1 / 2.94;
  const LayeredSquareGreens g(32, 3, ratio);
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

  const Field s = sineField(g.interfaceZ(), ratio);
  for (const int axis : {0, 1}) {
    double hop_sum = 0;
    double hop_sum2 = 0;
    for (int i = 0; i < samples; i++) {
      const FirstHop2D hop = g.sampleFirstHop(axis, rng);
      const double v = hop.sign * s(hop.offset) * g.fluxNorm(axis);
      hop_sum += v;
      hop_sum2 += v * v;
    }
    const double hop_mean = hop_sum / samples;
    const double hop_sigma
        = std::sqrt((hop_sum2 / samples - hop_mean * hop_mean) / samples);
    EXPECT_NEAR(hop_mean, fluxAverage(g, axis, s), 4 * hop_sigma);
  }
}

TEST(LayeredSquareGreens, RejectsBadArguments)
{
  EXPECT_THROW(LayeredSquareGreens(24, 0, 2.0), std::invalid_argument);
  EXPECT_THROW(LayeredSquareGreens(32, kK, 2.0), std::invalid_argument);
  EXPECT_THROW(LayeredSquareGreens(32, 0, 0.0), std::invalid_argument);
}

}  // namespace
}  // namespace fsv
