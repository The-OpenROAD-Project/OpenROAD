// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

// FFT — the density-grid context — and CpuFftBackend, the Ooura DCT solver.
//
// FFT owns the staging grids and the backend-agnostic accessors; doFFT()
// delegates to the FftBackend chosen at construction. CpuFftBackend (always
// compiled) is the Ooura DCT. makeFftBackend() is the single place the runtime
// backend choice is made: on an ENABLE_GPU build with the GPU path selected
// (gpl::gpuEnabled()) it returns the Kokkos GpuFftBackend.

#include "fft.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <utility>
#include <vector>

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

  void solve(float** density,
             float** phi,
             float** field_x,
             float** field_y) override;

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

void CpuFftBackend::solve(float** density,
                          float** phi,
                          float** field_x,
                          float** field_y)
{
  ddct2d(bin_cnt_x_,
         bin_cnt_y_,
         -1,
         density,
         nullptr,
         work_area_.data(),
         cs_table_.data());

  // Normalizations required to perform the inverse operation
  for (int i = 1; i < bin_cnt_x_; i++) {
    density[i][0] *= 0.5;
  }
  for (int i = 1; i < bin_cnt_y_; i++) {
    density[0][i] *= 0.5;
  }
  for (int i = 0; i < bin_cnt_x_; i++) {
    for (int j = 0; j < bin_cnt_y_; j++) {
      density[i][j] *= 4.0 / bin_cnt_x_ / bin_cnt_y_;
    }
  }

  // Solve the PDE in the new basis
  for (int i = 0; i < bin_cnt_x_; i++) {
    float wx = wx_[i];
    float wx2 = wx_square_[i];

    for (int j = 0; j < bin_cnt_y_; j++) {
      float wy = wy_[j];
      float wy2 = wy_square_[j];

      float density_value = density[i][j];
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

      phi[i][j] = phi_value;
      field_x[i][j] = electro_x;
      field_y[i][j] = electro_y;
    }
  }

  // Inverse DCT
  ddct2d(bin_cnt_x_,
         bin_cnt_y_,
         1,
         phi,
         nullptr,
         work_area_.data(),
         cs_table_.data());
  ddsct2d(bin_cnt_x_,
          bin_cnt_y_,
          1,
          field_x,
          nullptr,
          work_area_.data(),
          cs_table_.data());
  ddcst2d(bin_cnt_x_,
          bin_cnt_y_,
          1,
          field_y,
          nullptr,
          work_area_.data(),
          cs_table_.data());
}

}  // namespace

std::unique_ptr<FftBackend> makeFftBackend(int bin_cnt_x,
                                           int bin_cnt_y,
                                           float bin_size_x,
                                           float bin_size_y)
{
#ifdef ENABLE_GPU
  if (gpuEnabled()) {
    ensureKokkosInitialized();
    return std::make_unique<GpuFftBackend>(
        bin_cnt_x, bin_cnt_y, bin_size_x, bin_size_y);
  }
#endif
  return std::make_unique<CpuFftBackend>(
      bin_cnt_x, bin_cnt_y, bin_size_x, bin_size_y);
}

FFT::FFT(int bin_cnt_x, int bin_cnt_y, float bin_size_x, float bin_size_y)
    : bin_cnt_X_(bin_cnt_x),
      bin_cnt_y_(bin_cnt_y),
      backend_(makeFftBackend(bin_cnt_x, bin_cnt_y, bin_size_x, bin_size_y))
{
  bin_density_ = new float*[bin_cnt_X_];
  electro_phi_ = new float*[bin_cnt_X_];
  electro_field_x_ = new float*[bin_cnt_X_];
  electro_field_y_ = new float*[bin_cnt_X_];

  for (int i = 0; i < bin_cnt_X_; i++) {
    bin_density_[i] = new float[bin_cnt_y_];
    electro_phi_[i] = new float[bin_cnt_y_];
    electro_field_x_[i] = new float[bin_cnt_y_];
    electro_field_y_[i] = new float[bin_cnt_y_];

    for (int j = 0; j < bin_cnt_y_; j++) {
      bin_density_[i][j] = electro_phi_[i][j] = electro_field_x_[i][j]
          = electro_field_y_[i][j] = 0.0f;
    }
  }
}

FFT::~FFT()
{
  for (int i = 0; i < bin_cnt_X_; i++) {
    delete[] bin_density_[i];
    delete[] electro_phi_[i];
    delete[] electro_field_x_[i];
    delete[] electro_field_y_[i];
  }
  delete[] bin_density_;
  delete[] electro_phi_;
  delete[] electro_field_x_;
  delete[] electro_field_y_;
}

void FFT::updateDensity(int x, int y, float density)
{
  bin_density_[x][y] = density;
}

std::pair<float, float> FFT::getElectroField(int x, int y) const
{
  return std::make_pair(electro_field_x_[x][y], electro_field_y_[x][y]);
}

float FFT::getElectroPhi(int x, int y) const
{
  return electro_phi_[x][y];
}

void FFT::doFFT()
{
  backend_->solve(
      bin_density_, electro_phi_, electro_field_x_, electro_field_y_);
}

const char* FFT::getBackendName() const
{
  return backend_->name();
}

}  // namespace gpl
