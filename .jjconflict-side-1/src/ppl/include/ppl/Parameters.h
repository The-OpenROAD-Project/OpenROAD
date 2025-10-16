// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <string>

namespace ppl {

class Parameters
{
 public:
  void setReportHPWL(bool report) { report_hpwl_ = report; }
  bool getReportHPWL() const { return report_hpwl_; }

  void setSlotsPerSection(float slots_per_section)
  {
    slots_per_section_ = slots_per_section;
  }
  float getSlotsPerSection() const { return slots_per_section_; }

  void setHorizontalLengthExtend(int length)
  {
    horizontal_length_extend_ = length;
  }
  int getHorizontalLengthExtend() const { return horizontal_length_extend_; }

  void setVerticalLengthExtend(int length) { vertical_length_extend_ = length; }
  int getVerticalLengthExtend() const { return vertical_length_extend_; }

  void setHorizontalLength(int length) { horizontal_length_ = length; }
  int getHorizontalLength() const { return horizontal_length_; }

  void setVerticalLength(int length) { vertical_length_ = length; }
  int getVerticalLength() const { return vertical_length_; }

  void setRandSeed(unsigned int seed) { rand_seed_ = seed; }
  unsigned int getRandSeed() const { return rand_seed_; }

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

  void setMinDistanceInTracks(bool in_tracks)
  {
    distance_in_tracks_ = in_tracks;
  }
  bool getMinDistanceInTracks() const { return distance_in_tracks_; }

  void setPinPlacementFile(const char* file_name)
  {
    pin_placement_file_ = file_name;
  }
  std::string getPinPlacementFile() const { return pin_placement_file_; }

 private:
  bool report_hpwl_ = false;
  int slots_per_section_ = 200;
  float horizontal_thickness_multiplier_ = 1;
  float vertical_thickness_multiplier_ = 1;
  int horizontal_length_extend_ = -1;
  int vertical_length_extend_ = -1;
  int horizontal_length_ = -1;
  int vertical_length_ = -1;
  unsigned int rand_seed_ = 42;
  int corner_avoidance_ = -1;
  int min_dist_ = 0;
  bool distance_in_tracks_ = false;
  std::string pin_placement_file_;
};

}  // namespace ppl
