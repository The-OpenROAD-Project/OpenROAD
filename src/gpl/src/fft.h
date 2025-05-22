// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <utility>
#include <vector>

namespace gpl {

class FFT
{
 public:
  FFT(int binCntX, int binCntY, float binSizeX, float binSizeY);
  ~FFT();

  // input func
  void updateDensity(int x, int y, float density);

  // do FFT
  void doFFT();

  // returning func
  std::pair<float, float> getElectroForce(int x, int y) const;
  float getElectroPhi(int x, int y) const;

 private:
  // 2D array; width: binCntX_, height: binCntY_;
  // No hope to use Vector at this moment...
  float** binDensity_ = nullptr;
  float** electroPhi_ = nullptr;
  float** electroForceX_ = nullptr;
  float** electroForceY_ = nullptr;

  // cos/sin table (prev: w_2d)
  // length:  max(binCntX, binCntY) * 3 / 2
  std::vector<float> csTable_;

  // wx. length:  binCntX_
  std::vector<float> wx_;
  std::vector<float> wxSquare_;

  // wy. length:  binCntY_
  std::vector<float> wy_;
  std::vector<float> wySquare_;

  // work area for bit reversal (prev: ip)
  // length: round(sqrt( max(binCntX_, binCntY_) )) + 2
  std::vector<int> workArea_;

  int binCntX_ = 0;
  int binCntY_ = 0;
  float binSizeX_ = 0;
  float binSizeY_ = 0;
};

//
// The following FFT library came from
// http://www.kurims.kyoto-u.ac.jp/~ooura/fft.html
//
//
/// 1D FFT ////////////////////////////////////////////////////////////////
void cdft(int n, int isgn, float* a, int* ip, float* w);
void ddct(int n, int isgn, float* a, int* ip, float* w);
void ddst(int n, int isgn, float* a, int* ip, float* w);

/// 2D FFT ////////////////////////////////////////////////////////////////
void cdft2d(int, int, int, float**, float*, int*, float*);
void rdft2d(int, int, int, float**, float*, int*, float*);
void ddct2d(int, int, int, float**, float*, int*, float*);
void ddst2d(int, int, int, float**, float*, int*, float*);
void ddsct2d(int n1, int n2, int isgn, float** a, float* t, int* ip, float* w);
void ddcst2d(int n1, int n2, int isgn, float** a, float* t, int* ip, float* w);

/// 3D FFT ////////////////////////////////////////////////////////////////
void cdft3d(int, int, int, int, float***, float*, int*, float*);
void rdft3d(int, int, int, int, float***, float*, int*, float*);
void ddct3d(int, int, int, int, float***, float*, int*, float*);
void ddst3d(int, int, int, int, float***, float*, int*, float*);
void ddscct3d(int, int, int, int isgn, float***, float*, int*, float*);
void ddcsct3d(int, int, int, int isgn, float***, float*, int*, float*);
void ddccst3d(int, int, int, int isgn, float***, float*, int*, float*);

}  // namespace gpl
