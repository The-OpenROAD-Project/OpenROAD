// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Checks the density Poisson solve (gpl::FFT) against the math rather than
// against recorded output.
//
// The solve expands the bin density in the DCT basis
//   cos(ax * (x + 1/2)) * cos(ay * (y + 1/2)),  ax = pi*i/Nx, ay = pi*j/Ny
// (x, y are bin indices), drops the DC term, and divides each coefficient by
// kx^2 + ky^2 to get the potential.  The field is -grad(phi), so a single
// basis function has a closed-form answer for phi, Ex and Ey.  Every other
// density is a sum of basis functions, which the linearity test covers.

#include "src/gpl/src/fft.h"

#include <cmath>
#include <cstddef>
#include <numbers>
#include <ostream>
#include <random>
#include <vector>

#include "gtest/gtest.h"

namespace gpl {
namespace {

struct Grid
{
  int nx;
  int ny;
  float bin_size_x;
  float bin_size_y;
};

struct Solution
{
  std::vector<float> phi;
  std::vector<float> ex;
  std::vector<float> ey;
};

// Runs the solve on density[x * ny + y].
Solution solve(const Grid& grid, const std::vector<float>& density)
{
  FFT fft(grid.nx, grid.ny, grid.bin_size_x, grid.bin_size_y);
  for (int x = 0; x < grid.nx; x++) {
    for (int y = 0; y < grid.ny; y++) {
      fft.updateDensity(x, y, density[x * grid.ny + y]);
    }
  }
  fft.doFFT();

  Solution s;
  for (int x = 0; x < grid.nx; x++) {
    for (int y = 0; y < grid.ny; y++) {
      const auto [ex, ey] = fft.getElectroField(x, y);
      s.phi.push_back(fft.getElectroPhi(x, y));
      s.ex.push_back(ex);
      s.ey.push_back(ey);
    }
  }
  return s;
}

TEST(FftTest, UniformDensityHasNoField)
{
  const Grid grid{16, 16, 1.0f, 1.0f};
  const Solution s = solve(
      grid,
      std::vector<float>(static_cast<std::size_t>(grid.nx) * grid.ny, 0.7f));

  for (std::size_t k = 0; k < s.phi.size(); k++) {
    EXPECT_NEAR(s.phi[k], 0.0f, 1e-5f);
    EXPECT_NEAR(s.ex[k], 0.0f, 1e-5f);
    EXPECT_NEAR(s.ey[k], 0.0f, 1e-5f);
  }
}

struct ModeCase
{
  Grid grid;
  int i;
  int j;
};

std::ostream& operator<<(std::ostream& os, const ModeCase& c)
{
  return os << c.grid.nx << "x" << c.grid.ny << " bins of " << c.grid.bin_size_x
            << "x" << c.grid.bin_size_y << ", mode (" << c.i << ", " << c.j
            << ")";
}

class CosineModeTest : public ::testing::TestWithParam<ModeCase>
{
};

TEST_P(CosineModeTest, MatchesAnalyticSolution)
{
  const ModeCase& c = GetParam();
  const Grid& grid = c.grid;
  const double pi = std::numbers::pi;
  const double ax = pi * c.i / grid.nx;
  const double ay = pi * c.j / grid.ny;

  // Wavenumbers in units of x-bins.  A mode over Ny bins of height
  // bin_size_y has physical wavenumber ay / bin_size_y, which is
  // ay * bin_size_x / bin_size_y in x-bin units.
  const double kx = ax;
  const double ky = ay * grid.bin_size_x / grid.bin_size_y;
  const double amplitude = 1.0 / (kx * kx + ky * ky);

  std::vector<float> density;
  for (int x = 0; x < grid.nx; x++) {
    for (int y = 0; y < grid.ny; y++) {
      density.push_back(std::cos(ax * (x + 0.5)) * std::cos(ay * (y + 0.5)));
    }
  }
  const Solution s = solve(grid, density);

  const double tol = 1e-4 * amplitude;
  for (int x = 0; x < grid.nx; x++) {
    for (int y = 0; y < grid.ny; y++) {
      const double cx = std::cos(ax * (x + 0.5));
      const double sx = std::sin(ax * (x + 0.5));
      const double cy = std::cos(ay * (y + 0.5));
      const double sy = std::sin(ay * (y + 0.5));
      const int k = x * grid.ny + y;
      EXPECT_NEAR(s.phi[k], amplitude * cx * cy, tol)
          << "bin " << x << "," << y;
      EXPECT_NEAR(s.ex[k], amplitude * kx * sx * cy, tol)
          << "bin " << x << "," << y;
      EXPECT_NEAR(s.ey[k], amplitude * ky * cx * sy, tol)
          << "bin " << x << "," << y;
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    Modes,
    CosineModeTest,
    ::testing::Values(ModeCase{{16, 16, 1.0f, 1.0f}, 1, 0},
                      ModeCase{{16, 16, 1.0f, 1.0f}, 0, 7},
                      ModeCase{{16, 16, 1.0f, 1.0f}, 3, 5},
                      ModeCase{{16, 16, 1.0f, 1.0f}, 15, 15},
                      ModeCase{{16, 8, 1.0f, 1.0f}, 2, 3},
                      ModeCase{{16, 16, 1.0f, 2.0f}, 2, 3},
                      ModeCase{{16, 16, 2.0f, 1.0f}, 2, 3},
                      ModeCase{{8, 8, 3.0f, 1.0f}, 1, 1},
                      ModeCase{{8, 8, 1.0f, 3.0f}, 1, 1},
                      ModeCase{{32, 8, 1.0f, 2.0f}, 4, 1}));

// A charge in the middle of the grid pushes outward, symmetrically.
TEST(FftTest, FieldPointsAwayFromCharge)
{
  const Grid grid{16, 16, 1.0f, 1.0f};
  const int n = grid.nx;
  std::vector<float> density(static_cast<std::size_t>(n) * n, 0.0f);
  for (int x = 7; x <= 8; x++) {
    for (int y = 7; y <= 8; y++) {
      density[x * n + y] = 1.0f;
    }
  }
  const Solution s = solve(grid, density);

  auto at
      = [n](const std::vector<float>& v, int x, int y) { return v[x * n + y]; };
  for (int x = 0; x < n; x++) {
    for (int y = 0; y < n; y++) {
      const int mx = n - 1 - x;
      const int my = n - 1 - y;
      EXPECT_NEAR(at(s.phi, mx, y), at(s.phi, x, y), 1e-5f);
      EXPECT_NEAR(at(s.ex, mx, y), -at(s.ex, x, y), 1e-5f);
      EXPECT_NEAR(at(s.ey, x, my), -at(s.ey, x, y), 1e-5f);
    }
  }
  for (int k = 0; k < n; k++) {
    if (k < 7) {
      EXPECT_LT(at(s.ex, k, 7), 0.0f) << "left of the charge, x=" << k;
      EXPECT_LT(at(s.ey, 7, k), 0.0f) << "below the charge, y=" << k;
    } else if (k > 8) {
      EXPECT_GT(at(s.ex, k, 7), 0.0f) << "right of the charge, x=" << k;
      EXPECT_GT(at(s.ey, 7, k), 0.0f) << "above the charge, y=" << k;
    }
  }
  // The potential peaks at the charge.
  for (int x = 0; x < n; x++) {
    for (int y = 0; y < n; y++) {
      EXPECT_LE(at(s.phi, x, y), at(s.phi, 7, 7) + 1e-5f);
    }
  }
}

TEST(FftTest, SolveIsLinear)
{
  const Grid grid{16, 8, 1.0f, 1.5f};
  const std::size_t size = static_cast<std::size_t>(grid.nx) * grid.ny;
  std::mt19937 rng(1);
  std::uniform_real_distribution<float> dist(0.0f, 1.0f);
  std::vector<float> rho1(size);
  std::vector<float> rho2(size);
  std::vector<float> combined(size);
  constexpr float a = 2.0f;
  constexpr float b = -0.5f;
  for (std::size_t k = 0; k < size; k++) {
    rho1[k] = dist(rng);
    rho2[k] = dist(rng);
    combined[k] = a * rho1[k] + b * rho2[k];
  }

  const Solution s1 = solve(grid, rho1);
  const Solution s2 = solve(grid, rho2);
  const Solution sc = solve(grid, combined);
  for (std::size_t k = 0; k < size; k++) {
    EXPECT_NEAR(sc.phi[k], a * s1.phi[k] + b * s2.phi[k], 1e-4f);
    EXPECT_NEAR(sc.ex[k], a * s1.ex[k] + b * s2.ex[k], 1e-4f);
    EXPECT_NEAR(sc.ey[k], a * s1.ey[k] + b * s2.ey[k], 1e-4f);
  }
}

}  // namespace
}  // namespace gpl
