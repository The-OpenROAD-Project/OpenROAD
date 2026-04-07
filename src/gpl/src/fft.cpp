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

FFT::FFT(int bin_cnt_x, int bin_cnt_y, float bin_size_x, float bin_size_y)
    : bin_cnt_X_(bin_cnt_x),
      bin_cnt_y_(bin_cnt_y),
      bin_size_x_(bin_size_x),
      bin_size_y_(bin_size_y)
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

  cs_table_.resize(std::max(bin_cnt_X_, bin_cnt_y_) * 3 / 2, 0);

  wx_.resize(bin_cnt_X_, 0);
  wx_square_.resize(bin_cnt_X_, 0);
  wy_.resize(bin_cnt_y_, 0);
  wy_square_.resize(bin_cnt_y_, 0);

  work_area_.resize(round(sqrt(std::max(bin_cnt_X_, bin_cnt_y_))) + 2, 0);

  constexpr auto kPi = std::numbers::pi_v<long double>;

  for (int i = 0; i < bin_cnt_X_; i++) {
    wx_[i] = kPi * static_cast<float>(i) / static_cast<float>(bin_cnt_X_);
    wx_square_[i] = wx_[i] * wx_[i];
  }

  for (int i = 0; i < bin_cnt_y_; i++) {
    wy_[i] = kPi * static_cast<float>(i) / static_cast<float>(bin_cnt_y_)
             * bin_size_y_ / bin_size_x_;
    wy_square_[i] = wy_[i] * wy_[i];
  }
}

FFT::~FFT()
{
  using std::vector;
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

  cs_table_.clear();
  wx_.clear();
  wx_square_.clear();
  wy_.clear();
  wy_square_.clear();

  work_area_.clear();
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
  ddct2d(bin_cnt_X_,
         bin_cnt_y_,
         -1,
         bin_density_,
         nullptr,
         work_area_.data(),
         cs_table_.data());

  // Normalizations required to perform the inverse operation
  for (int i = 1; i < bin_cnt_X_; i++) {
    bin_density_[i][0] *= 0.5;
  }
  for (int i = 1; i < bin_cnt_y_; i++) {
    bin_density_[0][i] *= 0.5;
  }
  for (int i = 0; i < bin_cnt_X_; i++) {
    for (int j = 0; j < bin_cnt_y_; j++) {
      bin_density_[i][j] *= 4.0 / bin_cnt_X_ / bin_cnt_y_;
    }
  }

  // Solve the PDE in the new basis
  for (int i = 0; i < bin_cnt_X_; i++) {
    float wx = wx_[i];
    float wx2 = wx_square_[i];

    for (int j = 0; j < bin_cnt_y_; j++) {
      float wy = wy_[j];
      float wy2 = wy_square_[j];

      float density = bin_density_[i][j];
      float phi = 0;
      float electro_x = 0, electro_y = 0;

      if (i == 0 && j == 0) {
        // Removes the DC component
        phi = electro_x = electro_y = 0.0f;
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
        electro_x = phi * wx;
        electro_y = phi * wy;
      }

      electro_phi_[i][j] = phi;
      electro_field_x_[i][j] = electro_x;
      electro_field_y_[i][j] = electro_y;
    }
  }

  // Inverse DCT
  ddct2d(bin_cnt_X_,
         bin_cnt_y_,
         1,
         electro_phi_,
         nullptr,
         work_area_.data(),
         cs_table_.data());
  ddsct2d(bin_cnt_X_,
          bin_cnt_y_,
          1,
          electro_field_x_,
          nullptr,
          work_area_.data(),
          cs_table_.data());
  ddcst2d(bin_cnt_X_,
          bin_cnt_y_,
          1,
          electro_field_y_,
          nullptr,
          work_area_.data(),
          cs_table_.data());
}

}  // namespace gpl
