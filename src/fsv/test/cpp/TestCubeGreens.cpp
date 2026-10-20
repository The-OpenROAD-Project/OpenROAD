// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <cmath>
#include <functional>
#include <stdexcept>

#include "AliasTable.h"
#include "BoxFace.h"
#include "CubeGreens.h"
#include "Philox.h"
#include "fsv/Geometry.h"
#include "gtest/gtest.h"

namespace fsv {
namespace {

using Harmonic = std::function<double(const Vec3&)>;

// Average of f over panel (i, j) of a face, by 3x3 Gauss-Legendre.
double panelAverage(const Harmonic& f,
                    const int n,
                    const BoxFace face,
                    const int i,
                    const int j)
{
  static const double kNodes[3] = {-std::sqrt(0.6), 0.0, std::sqrt(0.6)};
  static const double kWeights[3] = {5.0 / 18, 8.0 / 18, 5.0 / 18};
  const auto [t0, t1] = tangentAxes(face.axis);
  const double h = 2.0 / n;
  double sum = 0;
  for (int a = 0; a < 3; a++) {
    for (int b = 0; b < 3; b++) {
      Vec3 p;
      p[index(face.axis)] = face.normalSign();
      p[index(t0)] = -1.0 + h * (i + 0.5 + 0.5 * kNodes[a]);
      p[index(t1)] = -1.0 + h * (j + 0.5 + 0.5 * kNodes[b]);
      sum += kWeights[a] * kWeights[b] * f(p);
    }
  }
  return sum;
}

// Sum over the surface of P * f, which should equal f at the center.
double averageOverSurface(const CubeGreens& g, const Harmonic& f)
{
  const int n = g.panels();
  double sum = 0;
  for (int face = 0; face < BoxFace::kCount; face++) {
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        sum += g.panelProbability(i, j)
               * panelAverage(f, n, BoxFace::fromIndex(face), i, j);
      }
    }
  }
  return sum;
}

// Sum over the surface of dP/dz * f, which should equal df/dz at the center.
double gradientOverSurface(const CubeGreens& g, const Harmonic& f)
{
  const int n = g.panels();
  double sum = 0;
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      // Faces normal to z.
      sum += g.normalFaceGradient(i, j)
             * panelAverage(f, n, {Axis::kZ, false}, i, j);
      sum -= g.normalFaceGradient(i, j)
             * panelAverage(f, n, {Axis::kZ, true}, i, j);
      // Faces parallel to z (normal to x and y); z is their second tangent.
      for (const Axis axis : {Axis::kX, Axis::kY}) {
        for (const bool high : {false, true}) {
          sum += g.parallelFaceGradient(i, j)
                 * panelAverage(f, n, {axis, high}, i, j);
        }
      }
    }
  }
  return sum;
}

const Vec3 kSource{2.5, 0.3, -0.7};

double inverseDistance(const Vec3& p)
{
  return 1.0
         / std::sqrt(std::pow(p[0] - kSource[0], 2)
                     + std::pow(p[1] - kSource[1], 2)
                     + std::pow(p[2] - kSource[2], 2));
}

TEST(CubeGreens, FacesCarryOneSixth)
{
  const CubeGreens g(32);
  double sum = 0;
  for (int i = 0; i < g.panels(); i++) {
    for (int j = 0; j < g.panels(); j++) {
      sum += g.panelProbability(i, j);
      EXPECT_GT(g.panelProbability(i, j), 0);
      EXPECT_NEAR(g.panelProbability(i, j), g.panelProbability(j, i), 1e-15);
      EXPECT_NEAR(g.panelProbability(i, j),
                  g.panelProbability(g.panels() - 1 - i, j),
                  1e-15);
    }
  }
  EXPECT_NEAR(sum, 1.0 / 6, 1e-12);
}

TEST(CubeGreens, MeanValueProperty)
{
  const CubeGreens g(32);
  EXPECT_NEAR(
      averageOverSurface(g, [](const Vec3&) { return 1.0; }), 1.0, 1e-12);
  EXPECT_NEAR(averageOverSurface(
                  g, [](const Vec3& p) { return p[0] * p[0] - p[2] * p[2]; }),
              0.0,
              1e-9);
  EXPECT_NEAR(
      averageOverSurface(g, [](const Vec3& p) { return p[0] * p[1] * p[2]; }),
      0.0,
      1e-12);
}

// For a non-polynomial harmonic function the only error is panel
// discretization: below 1e-4 at 32 panels, and second order in panel size.
TEST(CubeGreens, PanelBiasConverges)
{
  const double center = inverseDistance({0, 0, 0});
  const double r = std::sqrt(kSource[0] * kSource[0] + kSource[1] * kSource[1]
                             + kSource[2] * kSource[2]);
  const double slope = kSource[2] / (r * r * r);
  double prev_mean = 0;
  double prev_grad = 0;
  for (const int n : {32, 64}) {
    const CubeGreens g(n);
    const double mean_err
        = std::abs(averageOverSurface(g, inverseDistance) / center - 1);
    const double grad_err
        = std::abs(gradientOverSurface(g, inverseDistance) / slope - 1);
    EXPECT_LT(mean_err, 1e-4);
    EXPECT_LT(grad_err, 1e-4);
    if (n == 64) {
      EXPECT_LT(mean_err, prev_mean / 3);
      EXPECT_LT(grad_err, prev_grad / 3);
    }
    prev_mean = mean_err;
    prev_grad = grad_err;
  }
}

TEST(CubeGreens, GradientProperty)
{
  const CubeGreens g(32);
  EXPECT_NEAR(
      gradientOverSurface(g, [](const Vec3&) { return 1.0; }), 0.0, 1e-12);
  EXPECT_NEAR(
      gradientOverSurface(g, [](const Vec3& p) { return p[2]; }), 1.0, 1e-6);
  EXPECT_NEAR(
      gradientOverSurface(g, [](const Vec3& p) { return p[0]; }), 0.0, 1e-12);
  EXPECT_NEAR(gradientOverSurface(
                  g, [](const Vec3& p) { return p[2] * p[2] - p[0] * p[0]; }),
              0.0,
              1e-9);
}

// Sampling must reproduce the tables: Monte Carlo means agree within 4 sigma.
TEST(CubeGreens, SamplingMatchesTables)
{
  const CubeGreens g(32);
  Philox rng(7, 0);
  const int samples = 2000000;
  double sum = 0;
  double sum2 = 0;
  for (int s = 0; s < samples; s++) {
    const double f = inverseDistance(g.sampleTransition(rng));
    sum += f;
    sum2 += f * f;
  }
  double mean = sum / samples;
  double sigma = std::sqrt((sum2 / samples - mean * mean) / samples);
  EXPECT_NEAR(mean, inverseDistance({0, 0, 0}), 4 * sigma);

  const double r = std::sqrt(kSource[0] * kSource[0] + kSource[1] * kSource[1]
                             + kSource[2] * kSource[2]);
  sum = 0;
  sum2 = 0;
  for (int s = 0; s < samples; s++) {
    const CubeGreens::FirstHop hop = g.sampleFirstHop(Axis::kZ, rng);
    const double f = hop.sign * g.gradientNorm() * inverseDistance(hop.offset);
    sum += f;
    sum2 += f * f;
  }
  mean = sum / samples;
  sigma = std::sqrt((sum2 / samples - mean * mean) / samples);
  EXPECT_NEAR(mean, kSource[2] / (r * r * r), 4 * sigma);
}

TEST(AliasTable, RejectsEmptyOrZeroWeights)
{
  EXPECT_THROW(AliasTable({}), std::invalid_argument);
  EXPECT_THROW(AliasTable({0.0, 0.0}), std::invalid_argument);
  const AliasTable table({0.0, 3.0, 1.0});
  EXPECT_EQ(table.sample(0.0), 1);
}

}  // namespace
}  // namespace fsv
