// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "fftBackend.h"

namespace gpl {

class DeviceState;

// FFT — the density-grid context for the Poisson solve. It owns the staging
// grids and the backend-agnostic accessors; the solve itself is delegated to
// an FftBackend (the CPU Ooura DCT or the GPU Kokkos solver) selected at
// construction by makeFftBackend(). Callers see one concrete class regardless
// of backend.
class FFT
{
 public:
  FFT(int bin_cnt_x,
      int bin_cnt_y,
      float bin_size_x,
      float bin_size_y,
      DeviceState* device_state = nullptr);
  ~FFT();

  // input func
  void updateDensity(int x, int y, float density);

  // do FFT
  void doFFT();

  // returning func
  std::pair<float, float> getElectroField(int x, int y) const;
  float getElectroPhi(int x, int y) const;

  // Diagnostic label of the backend chosen at construction (e.g. "CPU").
  const char* getBackendName() const;

 private:
  // Row-major flat buffers, layout [x * bin_cnt_y_ + y]. The backend takes a
  // BinGridSpan over each; the CPU Ooura backend re-wraps as float** locally
  // because ddct2d() takes that legacy shape.
  std::vector<float> bin_density_;
  std::vector<float> electro_phi_;
  std::vector<float> electro_field_x_;
  std::vector<float> electro_field_y_;

  int bin_cnt_x_ = 0;
  int bin_cnt_y_ = 0;

  // The Poisson solve backend (CPU Ooura or GPU Kokkos), selected at run time
  // in the constructor. doFFT() delegates to it.
  std::unique_ptr<FftBackend> backend_;
};

//
// The following FFT library came from
// http://www.kurims.kyoto-u.ac.jp/~ooura/fft.html
//
//
/// 1D FFT ////////////////////////////////////////////////////////////////
void cdft(int n, int isgn, float* a, int* ip, float* w);
void rdft(int n, int isgn, float* a, int* ip, float* w);
void ddct(int n, int isgn, float* a, int* ip, float* w);
void ddst(int n, int isgn, float* a, int* ip, float* w);
void dfct(int n, float* a, float* t, int* ip, float* w);
void dfst(int n, float* a, float* t, int* ip, float* w);

/// 2D FFT ////////////////////////////////////////////////////////////////
void cdft2d(int, int, int, float**, float*, int*, float*);
void rdft2d(int, int, int, float**, float*, int*, float*);
void rdft2dsort(int n1, int n2, int isgn, float** a);
void ddct2d(int, int, int, float**, float*, int*, float*);
void ddst2d(int, int, int, float**, float*, int*, float*);
void ddsct2d(int n1, int n2, int isgn, float** a, float* t, int* ip, float* w);
void ddcst2d(int n1, int n2, int isgn, float** a, float* t, int* ip, float* w);

/// Utils /////////////////////////////////////////////////////////////////
void makect(int nc, int* ip, float* c);
void makewt(int nw, int* ip, float* w);

}  // namespace gpl
