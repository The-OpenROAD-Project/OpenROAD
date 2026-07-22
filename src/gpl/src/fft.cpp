// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

// FFT — the density-grid context — and CpuFftBackend, the Ooura DCT solver.
// doFFT() delegates to the FftBackend chosen at construction. makeFftBackend()
// makes that choice: GpuFftBackend on an ENABLE_GPU build with gpuEnabled(),
// else the always-compiled CpuFftBackend (Ooura DCT).

#include "fft.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <numbers>
#include <utility>
#include <vector>

#include "backendContext.h"
#include "fftBackend.h"

#ifdef ENABLE_GPU
#include "gpu/gpuFftBackend.h"
#include "gpu/gpuRuntime.h"
#endif

namespace gpl {

namespace {

// CPU FFT backend: the Ooura DCT Poisson solver. Owns the cos/sin and
// wavenumber tables; the solve body is byte-identical to the pre-GPU
// FFT::doFFT().
class CpuFftBackend : public FftBackend
{
 public:
  CpuFftBackend(int bin_cnt_x,
                int bin_cnt_y,
                float bin_size_x,
                float bin_size_y);

  void solve(BinGridSpan density,
             BinGridSpan phi,
             BinGridSpan field_x,
             BinGridSpan field_y) override;

  const char* name() const override { return "CPU (Ooura DCT)"; }

 private:
  int bin_cnt_x_;
  int bin_cnt_y_;

  // cos/sin table (prev: w_2d); length max(binCntX, binCntY) * 3 / 2
  std::vector<float> cs_table_;
  // wavenumbers along x (length binCntX) and y (length binCntY)
  std::vector<float> wx_;
  std::vector<float> wx_square_;
  std::vector<float> wy_;
  std::vector<float> wy_square_;
  // work area for bit reversal (prev: ip)
  std::vector<int> work_area_;
};

CpuFftBackend::CpuFftBackend(int bin_cnt_x,
                             int bin_cnt_y,
                             float bin_size_x,
                             float bin_size_y)
    : bin_cnt_x_(bin_cnt_x), bin_cnt_y_(bin_cnt_y)
{
  cs_table_.resize(std::max(bin_cnt_x_, bin_cnt_y_) * 3 / 2, 0);

  wx_.resize(bin_cnt_x_, 0);
  wx_square_.resize(bin_cnt_x_, 0);
  wy_.resize(bin_cnt_y_, 0);
  wy_square_.resize(bin_cnt_y_, 0);

  work_area_.resize(round(sqrt(std::max(bin_cnt_x_, bin_cnt_y_))) + 2, 0);

  constexpr auto kPi = std::numbers::pi_v<long double>;

  for (int i = 0; i < bin_cnt_x_; i++) {
    wx_[i] = kPi * static_cast<float>(i) / static_cast<float>(bin_cnt_x_);
    wx_square_[i] = wx_[i] * wx_[i];
  }

  for (int i = 0; i < bin_cnt_y_; i++) {
    wy_[i] = kPi * static_cast<float>(i) / static_cast<float>(bin_cnt_y_)
             * bin_size_y / bin_size_x;
    wy_square_[i] = wy_[i] * wy_[i];
  }
}

// Build a temporary float** row-pointer table over a flat BinGridSpan so the
// Ooura ddct2d() / ddsct2d() / ddcst2d() API (which expects float**) can be
// called without changing the FFT context's flat storage convention.
namespace {
std::vector<float*> makeRowPtrs(BinGridSpan g)
{
  std::vector<float*> rows(g.bin_cnt_x);
  for (int i = 0; i < g.bin_cnt_x; i++) {
    rows[i] = g.data + static_cast<std::size_t>(i) * g.bin_cnt_y;
  }
  return rows;
}
}  // namespace

void CpuFftBackend::solve(BinGridSpan density,
                          BinGridSpan phi,
                          BinGridSpan field_x,
                          BinGridSpan field_y)
{
  auto density_rows = makeRowPtrs(density);
  auto phi_rows = makeRowPtrs(phi);
  auto field_x_rows = makeRowPtrs(field_x);
  auto field_y_rows = makeRowPtrs(field_y);
  float** density_p = density_rows.data();
  float** phi_p = phi_rows.data();
  float** field_x_p = field_x_rows.data();
  float** field_y_p = field_y_rows.data();

  ddct2d(bin_cnt_x_,
         bin_cnt_y_,
         -1,
         density_p,
         nullptr,
         work_area_.data(),
         cs_table_.data());

  // Normalizations required to perform the inverse operation
  for (int i = 1; i < bin_cnt_x_; i++) {
    density_p[i][0] *= 0.5;
  }
  for (int i = 1; i < bin_cnt_y_; i++) {
    density_p[0][i] *= 0.5;
  }
  for (int i = 0; i < bin_cnt_x_; i++) {
    for (int j = 0; j < bin_cnt_y_; j++) {
      density_p[i][j] *= 4.0 / bin_cnt_x_ / bin_cnt_y_;
    }
  }

  // Solve the PDE in the new basis
  for (int i = 0; i < bin_cnt_x_; i++) {
    float wx = wx_[i];
    float wx2 = wx_square_[i];

    for (int j = 0; j < bin_cnt_y_; j++) {
      float wy = wy_[j];
      float wy2 = wy_square_[j];

      float density_value = density_p[i][j];
      float phi_value = 0;
      float electro_x = 0, electro_y = 0;

      if (i == 0 && j == 0) {
        // Removes the DC component
        phi_value = electro_x = electro_y = 0.0f;
      } else {
        phi_value = density_value / (wx2 + wy2);
        electro_x = phi_value * wx;
        electro_y = phi_value * wy;
      }

      phi_p[i][j] = phi_value;
      field_x_p[i][j] = electro_x;
      field_y_p[i][j] = electro_y;
    }
  }

  // Inverse DCT
  ddct2d(bin_cnt_x_,
         bin_cnt_y_,
         1,
         phi_p,
         nullptr,
         work_area_.data(),
         cs_table_.data());
  ddsct2d(bin_cnt_x_,
          bin_cnt_y_,
          1,
          field_x_p,
          nullptr,
          work_area_.data(),
          cs_table_.data());
  ddcst2d(bin_cnt_x_,
          bin_cnt_y_,
          1,
          field_y_p,
          nullptr,
          work_area_.data(),
          cs_table_.data());
}

}  // namespace

std::unique_ptr<FftBackend> makeFftBackend(const BackendContext& ctx)
{
#ifdef ENABLE_GPU
  if (gpuEnabled()) {
    ensureKokkosInitialized();
    return std::make_unique<GpuFftBackend>(ctx.bin_cnt_x,
                                           ctx.bin_cnt_y,
                                           ctx.bin_size_x,
                                           ctx.bin_size_y,
                                           ctx.region_field);
  }
#endif
  return std::make_unique<CpuFftBackend>(
      ctx.bin_cnt_x, ctx.bin_cnt_y, ctx.bin_size_x, ctx.bin_size_y);
}

namespace {
BackendContext makeFftCtx(int bin_cnt_x,
                          int bin_cnt_y,
                          float bin_size_x,
                          float bin_size_y,
                          RegionDensityField* region_field)
{
  BackendContext ctx;
  ctx.bin_cnt_x = bin_cnt_x;
  ctx.bin_cnt_y = bin_cnt_y;
  ctx.bin_size_x = bin_size_x;
  ctx.bin_size_y = bin_size_y;
  ctx.region_field = region_field;
  return ctx;
}
}  // namespace

FFT::FFT(int bin_cnt_x,
         int bin_cnt_y,
         float bin_size_x,
         float bin_size_y,
         RegionDensityField* region_field)
    : bin_density_(static_cast<std::size_t>(bin_cnt_x) * bin_cnt_y, 0.0f),
      electro_phi_(static_cast<std::size_t>(bin_cnt_x) * bin_cnt_y, 0.0f),
      electro_field_x_(static_cast<std::size_t>(bin_cnt_x) * bin_cnt_y, 0.0f),
      electro_field_y_(static_cast<std::size_t>(bin_cnt_x) * bin_cnt_y, 0.0f),
      bin_cnt_x_(bin_cnt_x),
      bin_cnt_y_(bin_cnt_y),
      backend_(makeFftBackend(makeFftCtx(bin_cnt_x,
                                         bin_cnt_y,
                                         bin_size_x,
                                         bin_size_y,
                                         region_field)))
{
}

FFT::~FFT() = default;

void FFT::updateDensity(int x, int y, float density)
{
  bin_density_[static_cast<std::size_t>(x) * bin_cnt_y_ + y] = density;
}

std::pair<float, float> FFT::getElectroField(int x, int y) const
{
  const std::size_t k = static_cast<std::size_t>(x) * bin_cnt_y_ + y;
  return std::make_pair(electro_field_x_[k], electro_field_y_[k]);
}

float FFT::getElectroPhi(int x, int y) const
{
  return electro_phi_[static_cast<std::size_t>(x) * bin_cnt_y_ + y];
}

void FFT::doFFT()
{
  BinGridSpan density{.data = bin_density_.data(),
                      .bin_cnt_x = bin_cnt_x_,
                      .bin_cnt_y = bin_cnt_y_};
  BinGridSpan phi{.data = electro_phi_.data(),
                  .bin_cnt_x = bin_cnt_x_,
                  .bin_cnt_y = bin_cnt_y_};
  BinGridSpan field_x{.data = electro_field_x_.data(),
                      .bin_cnt_x = bin_cnt_x_,
                      .bin_cnt_y = bin_cnt_y_};
  BinGridSpan field_y{.data = electro_field_y_.data(),
                      .bin_cnt_x = bin_cnt_x_,
                      .bin_cnt_y = bin_cnt_y_};
  backend_->solve(density, phi, field_x, field_y);
}

const char* FFT::getBackendName() const
{
  return backend_->name();
}

}  // namespace gpl
