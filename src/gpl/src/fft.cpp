// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "fft.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <numbers>
#include <utility>

namespace gpl {

FFT::FFT(int binCntX, int binCntY, float binSizeX, float binSizeY)
    : binCntX_(binCntX),
      binCntY_(binCntY),
      binSizeX_(binSizeX),
      binSizeY_(binSizeY)
{
  binDensity_ = new float*[binCntX_];
  electroPhi_ = new float*[binCntX_];
  electroFieldX_ = new float*[binCntX_];
  electroFieldY_ = new float*[binCntX_];

  for (int i = 0; i < binCntX_; i++) {
    binDensity_[i] = new float[binCntY_];
    electroPhi_[i] = new float[binCntY_];
    electroFieldX_[i] = new float[binCntY_];
    electroFieldY_[i] = new float[binCntY_];

    for (int j = 0; j < binCntY_; j++) {
      binDensity_[i][j] = electroPhi_[i][j] = electroFieldX_[i][j]
          = electroFieldY_[i][j] = 0.0f;
    }
  }

  csTable_.resize(std::max(binCntX_, binCntY_) * 3 / 2, 0);

  wx_.resize(binCntX_, 0);
  wxSquare_.resize(binCntX_, 0);
  wy_.resize(binCntY_, 0);
  wySquare_.resize(binCntY_, 0);

  workArea_.resize(round(sqrt(std::max(binCntX_, binCntY_))) + 2, 0);

  constexpr auto kPi = std::numbers::pi_v<long double>;

  for (int i = 0; i < binCntX_; i++) {
    wx_[i] = kPi * static_cast<float>(i) / static_cast<float>(binCntX_);
    wxSquare_[i] = wx_[i] * wx_[i];
  }

  for (int i = 0; i < binCntY_; i++) {
    wy_[i] = kPi * static_cast<float>(i) / static_cast<float>(binCntY_)
             * binSizeY_ / binSizeX_;
    wySquare_[i] = wy_[i] * wy_[i];
  }
}

FFT::~FFT()
{
  using std::vector;
  for (int i = 0; i < binCntX_; i++) {
    delete[] binDensity_[i];
    delete[] electroPhi_[i];
    delete[] electroFieldX_[i];
    delete[] electroFieldY_[i];
  }
  delete[] binDensity_;
  delete[] electroPhi_;
  delete[] electroFieldX_;
  delete[] electroFieldY_;

  csTable_.clear();
  wx_.clear();
  wxSquare_.clear();
  wy_.clear();
  wySquare_.clear();

  workArea_.clear();
}

void FFT::updateDensity(int x, int y, float density)
{
  binDensity_[x][y] = density;
}

std::pair<float, float> FFT::getElectroField(int x, int y) const
{
  return std::make_pair(electroFieldX_[x][y], electroFieldY_[x][y]);
}

float FFT::getElectroPhi(int x, int y) const
{
  return electroPhi_[x][y];
}

void FFT::doFFT()
{
  ddct2d(binCntX_,
         binCntY_,
         -1,
         binDensity_,
         nullptr,
         workArea_.data(),
         csTable_.data());

  // Normalizations required to perform the inverse operation
  for (int i = 1; i < binCntX_; i++) {
    binDensity_[i][0] *= 0.5;
  }
  for (int i = 1; i < binCntY_; i++) {
    binDensity_[0][i] *= 0.5;
  }
  for (int i = 0; i < binCntX_; i++) {
    for (int j = 0; j < binCntY_; j++) {
      binDensity_[i][j] *= 4.0 / binCntX_ / binCntY_;
    }
  }

  // Solve the PDE in the new basis
  for (int i = 0; i < binCntX_; i++) {
    float wx = wx_[i];
    float wx2 = wxSquare_[i];

    for (int j = 0; j < binCntY_; j++) {
      float wy = wy_[j];
      float wy2 = wySquare_[j];

      float density = binDensity_[i][j];
      float phi = 0;
      float electroX = 0, electroY = 0;

      if (i == 0 && j == 0) {
        // Removes the DC component
        phi = electroX = electroY = 0.0f;
      } else {
        //////////// lutong
        //  denom =
        //  wx2 / 4.0 +
        //  wy2 / 4.0 ;
        // a_phi = a_den / denom ;
        ////b_phi = 0 ; // -1.0 * b / denom ;
        ////a_ex = 0 ; // b_phi * wx ;
        // a_ex = a_phi * wx / 2.0 ;
        ////a_ey = 0 ; // b_phi * wy ;
        // a_ey = a_phi * wy / 2.0 ;
        ///////////
        phi = density / (wx2 + wy2);
        electroX = phi * wx;
        electroY = phi * wy;
      }

      electroPhi_[i][j] = phi;
      electroFieldX_[i][j] = electroX;
      electroFieldY_[i][j] = electroY;
    }
  }

  // Inverse DCT
  ddct2d(binCntX_,
         binCntY_,
         1,
         electroPhi_,
         nullptr,
         workArea_.data(),
         csTable_.data());
  ddsct2d(binCntX_,
          binCntY_,
          1,
          electroFieldX_,
          nullptr,
          workArea_.data(),
          csTable_.data());
  ddcst2d(binCntX_,
          binCntY_,
          1,
          electroFieldY_,
          nullptr,
          workArea_.data(),
          csTable_.data());
}

}  // namespace gpl
