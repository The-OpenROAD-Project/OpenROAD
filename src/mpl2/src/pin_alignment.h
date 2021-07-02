///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "block_placement.h"
#include "shape_engine.h"
#include "utl/Logger.h"

namespace pin_alignment {

class SimulatedAnnealingCore
{
 private:
  float init_prob_ = 0.95;
  float rej_ratio_ = 0.95;
  int max_num_step_ = 1000;
  int k_;
  float c_;
  int perturb_per_step_ = 60;
  float alpha_ = 0.4;
  float beta_ = 0.3;
  float gamma_ = 0.3;

  float cooling_rate_;

  float outline_width_ = 0.0;
  float outline_height_ = 0.0;

  float height_ = 0.0;
  float width_ = 0.0;
  float area_ = 0.0;
  float wirelength_ = 0.0;
  float outline_penalty_ = 0.0;

  float pre_height_ = 0.0;
  float pre_width_ = 0.0;
  float pre_area_ = 0.0;
  float pre_wirelength_ = 0.0;
  float pre_outline_penalty_ = 0.0;

  float norm_area_ = 0.0;
  float norm_wirelength_ = 0.0;
  float norm_outline_penalty_ = 0.0;
  float init_T_ = 0.0;

  int action_id_ = -1;
  bool flip_flag_ = true;
  std::vector<shape_engine::Macro> macros_;
  std::vector<block_placement::Net*> nets_;
  std::unordered_map<std::string, std::pair<float, float>> terminal_position_;
  std::unordered_map<std::string, int> macro_map_;

  std::vector<int> pos_seq_;
  std::vector<int> neg_seq_;
  std::vector<int> pre_pos_seq_;
  std::vector<int> pre_neg_seq_;

  float flip_prob_ = 0.2;
  float pos_swap_prob_ = 0.5;     // actually prob = 0.3
  float neg_swap_prob_ = 0.8;     // actually prob = 0.3
  float double_swap_prob_ = 1.0;  // actually prob = 0.2

  std::mt19937 generator_;
  std::uniform_real_distribution<float> distribution_;

  void PackFloorplan();
  void SingleFlip();
  void SingleSwap(bool flag);  // true for pos swap and false for neg swap
  void DoubleSwap();
  void Flip();
  void Perturb();
  void Restore();
  void Initialize();
  void CalculateWirelength();
  void CalculateOutlinePenalty();
  float NormCost(float area, float wirelength, float outline_penalty) const;
  void FastSA();

 public:
  // constructor
  SimulatedAnnealingCore(
      const std::vector<shape_engine::Macro>& macros,
      const std::vector<block_placement::Net*>& nets,
      const std::unordered_map<std::string, std::pair<float, float>>&
          terminal_position,
      float cooling_rate,
      float outline_width,
      float outline_height,
      float init_prob = 0.95,
      float rej_ratio = 0.95,
      int max_num_step = 1000,
      int k = 100,
      float c = 100,
      int perturb_per_step = 60,
      float alpha = 0.4,
      float beta = 0.3,
      float gamma = 0.3,
      float flip_prob = 0.2,
      float pos_swap_prob = 0.3,
      float neg_swap_prob = 0.3,
      float double_swap_prob = 0.2,
      unsigned seed = 0);

  void Run();

  float GetWidth() const { return width_; }
  float GetHeight() const { return height_; }
  float GetArea() const { return area_; }
  float GetWirelength() const { return wirelength_; };
  bool IsFeasible() const;

  void WriteFloorplan(const std::string& file_name) const;

  std::vector<shape_engine::Macro> GetMacros() const { return macros_; }
};

// wrapper for run function of SimulatedAnnealingCore
void Run(SimulatedAnnealingCore* sa);

// Parse macro file
void ParseMacroFile(std::vector<shape_engine::Macro>& macros,
                    float halo_width,
                    const std::string& file_name);

bool PinAlignmentSingleCluster(
    const char *report_directory,
    shape_engine::Cluster* cluster,
    const std::unordered_map<std::string, std::pair<float, float>>&
        terminal_position,
    const std::vector<block_placement::Net*>& nets,
    utl::Logger* logger,
    float halo_width,
    int num_thread,
    int num_run,
    unsigned seed);

// Pin Alignment Engine
bool PinAlignment(const std::vector<shape_engine::Cluster*>& clusters,
                  utl::Logger* logger,
                  const char *report_directory,
                  float halo_width,
                  int num_thread,
                  int num_run,
                  unsigned seed = 0);

}  // namespace pin_alignment
