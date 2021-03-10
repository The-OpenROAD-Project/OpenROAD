/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

namespace ppl {

class Parameters
{
 public:
  Parameters() = default;
  void setReportHPWL(bool report) { report_hpwl_ = report; }
  bool getReportHPWL() const { return report_hpwl_; }

  void setNumSlots(int num_slots) { num_slots_ = num_slots; }
  int getNumSlots() const { return num_slots_; }

  void setSlotsFactor(float factor) { slots_factor_ = factor; }
  float getSlotsFactor() const { return slots_factor_; }

  void setUsageFactor(float factor) { usage_factor_ = factor; }
  float getUsageFactor() const { return usage_factor_; }

  void setForceSpread(bool force_spread) { force_spread_ = force_spread; }
  bool getForceSpread() const { return force_spread_; }

  void setHorizontalLengthExtend(float length)
  {
    horizontal_length_extend_ = length;
  }
  float getHorizontalLengthExtend() const { return horizontal_length_extend_; }

  void setVerticalLengthExtend(float length)
  {
    vertical_length_extend_ = length;
  }
  float getVerticalLengthExtend() const { return vertical_length_extend_; }

  void setHorizontalLength(float length) { horizontal_length_ = length; }
  float getHorizontalLength() const { return horizontal_length_; }

  void setVerticalLength(float length) { vertical_length_ = length; }
  float getVerticalLength() const { return vertical_length_; }

  void setRandSeed(double seed) { rand_seed_ = seed; }
  double getRandSeed() const { return rand_seed_; }

  void setHorizontalThicknessMultiplier(float length)
  {
    horizontal_thickness_multiplier_ = length;
  }

  float getHorizontalThicknessMultiplier() const
  {
    return horizontal_thickness_multiplier_;
  }

  void setVerticalThicknessMultiplier(float length)
  {
    vertical_thickness_multiplier_ = length;
  }
  float getVerticalThicknessMultiplier() const
  {
    return vertical_thickness_multiplier_;
  }

  void setCornerAvoidance(int length) { corner_avoidance_ = length; }
  int getCornerAvoidance() const { return corner_avoidance_; }

  void setMinDistance(int min_dist) { min_dist_ = min_dist; }
  int getMinDistance() const { return min_dist_; }

 private:
  bool report_hpwl_ = false;
  bool force_spread_ = true;
  int num_slots_ = -1;
  float slots_factor_ = -1;
  float usage_ = -1;
  float usage_factor_ = -1;
  float horizontal_thickness_multiplier_ = 1;
  float vertical_thickness_multiplier_ = 1;
  float horizontal_length_extend_ = -1;
  float vertical_length_extend_ = -1;
  float horizontal_length_ = -1;
  float vertical_length_ = -1;
  double rand_seed_ = 42.0;
  int corner_avoidance_;
  int min_dist_;
};

}  // namespace ppl
