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

#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "shape_engine.h"
#include "utl/Logger.h"

namespace block_placement {
// definition of blocks:  include semi-soft blocks and soft blocks
class Block
{
 private:
  float width_ = 0.0;
  float height_ = 0.0;
  float area_ = 0.0;
  float x_ = 0.0;  // lx
  float y_ = 0.0;  // ly
  std::string name_;
  bool is_soft_ = true;
  int num_macro_ = 0;

  bool flag_ = false;

  std::vector<std::pair<float, float>> aspect_ratio_;
  std::vector<std::pair<float, float>>
      width_limit_;  // This is in increasing order
  std::vector<std::pair<float, float>>
      height_limit_;  // This is in decreasing order

  std::mt19937* generator_;
  std::uniform_real_distribution<float>* distribution_;

 public:
  Block(const std::string& name,
        float area,
        int num_macro,
        const std::vector<std::pair<float, float>>& aspect_ratio);

  // Accessors
  bool IsSoft() const { return is_soft_; }
  std::string GetName() const { return name_; }
  float GetX() const { return x_; }
  float GetY() const { return y_; }
  float GetWidth() const { return width_; }
  float GetHeight() const { return height_; }
  float GetArea() const { return area_; }
  float GetAspectRatio() const { return height_ / width_; }
  int GetNumMacro() const { return num_macro_; }

  void SetFlag(bool flag) { flag_ = flag; }
  bool GetFlag() { return flag_; }

  void SetX(float x) { x_ = x; }
  void SetY(float y) { y_ = y; }
  void SetAspectRatio(float aspect_ratio);
  void SetRandom(std::mt19937& generator,
                 std::uniform_real_distribution<float>& distribution);

  void ChangeWidth(float width);
  void ChangeHeight(float height);

  bool IsResizeable() const;
  void ResizeHardBlock();

  void ChooseAspectRatioRandom();

  void RemoveSoftBlock();

  void ShrinkSoftBlock(float width_factor, float height_factor);
};

//bool CompareBlockX(const Block& block_a, const Block& block_b)
//{
//    return block_a.GetX() < block_b.GetX();
//}

//bool CompareBlockY(const Block& block_a, const Block& block_b)
//{
//    return block_a.GetY() < block_b.GetY();
//}


struct Net
{
  int weight_ = 0;
  std::vector<std::string> blocks_;
  std::vector<std::string> terminals_;

  Net(int weight,
      const std::vector<std::string>& blocks,
      const std::vector<std::string>& terminals)
  {
    weight_ = weight;
    blocks_ = blocks;
    terminals_ = terminals;
  }
};

struct Region
{
  float lx_ = 0.0;
  float ly_ = 0.0;
  float ux_ = 0.0;
  float uy_ = 0.0;

  Region(float lx, float ly, float ux, float uy)
  {
    lx_ = lx;
    ly_ = ly;
    ux_ = ux;
    uy_ = uy;
  }
};


struct Location
{
  float lx_ = 0.0;
  float ly_ = 0.0;
  float ux_ = 0.0;
  float uy_ = 0.0;
  std::string name_;

  Location(std::string name, float lx, float ly, float ux, float uy) {
    name_ = name;
    lx_ = lx;
    ly_ = ly;
    ux_ = ux;
    uy_ = uy;
  }
};




class SimulatedAnnealingCore
{
 private:
  // These parameters are related to fastSA
  float init_prob_ = 0.95;
  float rej_ratio_ = 0.95;
  int max_num_step_ = 1000;
  int k_;
  float c_;
  int perturb_per_step_ = 60;
  float init_T_;

  float outline_width_;
  float outline_height_;

  float cooling_rate_;

  // learning rate for dynamic weight
  float learning_rate_ = 0.01;

  // shrink_factor for dynamic weight
  float shrink_factor_ = 0.995;
  float shrink_freq_ = 0.01;

  float height_;
  float width_;
  float area_;
  float wirelength_;
  float outline_penalty_;
  float boundary_penalty_;
  float macro_blockage_penalty_;
  float location_penalty_;
   
  float pre_height_;
  float pre_width_;
  float pre_area_;
  float pre_wirelength_;
  float pre_outline_penalty_;
  float pre_boundary_penalty_;
  float pre_macro_blockage_penalty_;
  float pre_location_penalty_;

  float norm_area_;
  float norm_wirelength_;
  float norm_outline_penalty_;
  float norm_boundary_penalty_;
  float norm_macro_blockage_penalty_;
  float norm_location_penalty_;

  // These parameters are related to cost function
  float alpha_;                  // weight for area
  float beta_;                   // weight for wirelength
  float gamma_;                  // weight for outline penalty
  float boundary_weight_;        // weight for boundary penalty
  float macro_blockage_weight_;  // weight for macro blockage penalty
  float location_weight_;        // weight for location penalty


  float alpha_base_;
  float beta_base_;
  float gamma_base_;
  float boundary_weight_base_;
  float macro_blockage_weight_base_;
  float location_weight_base_;
 
  // These parameters are related to action probabilities
  float resize_prob_ = 0.4;
  float pos_swap_prob_ = 0.2;
  float neg_swap_prob_ = 0.2;
  float double_swap_prob_ = 0.2;
  
  std::unordered_map<std::string, int> block_map_;
  std::unordered_map<int, int> location_map_;
  std::unordered_map<std::string, std::pair<float, float>> terminal_position_;
  std::vector<Net*> nets_;
  std::vector<Region*> regions_;
  std::vector<Location*> locations_;

  std::vector<Block> blocks_;
  std::vector<Block> pre_blocks_;

  int action_id_ = -1;
  int block_id_ = -1;

  std::vector<int> pos_seq_;
  std::vector<int> neg_seq_;
  std::vector<int> pre_pos_seq_;
  std::vector<int> pre_neg_seq_;

  std::mt19937 generator_;
  std::uniform_real_distribution<float> distribution_;

  void PackFloorplan();
  void Resize();
  void SingleSwap(bool flag);  // true for pos_seq and false for neg_seq
  void DoubleSwap();
  void Perturb();
  void Restore();
  void CalculateOutlinePenalty();
  void CalculateBoundaryPenalty();
  void CalculateMacroBlockagePenalty();
  void CalculateLocationPenalty();
  void CalculateWirelength();
  bool CalculateOverlap();
  float NormCost(float area,
                 float wirelength,
                 float outline_penalty,
                 float boundary_penalty,
                 float macro_blockage_penalty,
                 float location_penalty) const;

  void UpdateWeight(float avg_area,
                    float avg_wirelength,
                    float avg_outline_penalty,
                    float avg_boundary_penalty,
                    float avg_macro_blockage_penalty,
                    float avg_location_penalty);

 public:
  // Constructor
  SimulatedAnnealingCore(
      float outline_width,
      float outline_height,
      const std::vector<Block>& blocks,
      const std::vector<Net*>& nets,
      const std::vector<Region*>& regions,
      const std::vector<Location*>& locations,
      const std::unordered_map<std::string, std::pair<float, float>>&
          terminal_position,
      float cooling_rate,
      float alpha,
      float beta,
      float gamma,
      float boundary_weight,
      float macro_blockage_weight,
      float location_weight,
      float resize_prob,
      float pos_swap_prob,
      float neg_swap_prob,
      float double_swap_prob,
      float init_prob,
      float rej_ratio,
      int max_num_step,
      int k,
      float c,
      int perturb_per_step,
      float learning_rate,
      float shrink_factor,
      float shrink_freq,
      unsigned seed = 0);

  void FastSA();

  void Initialize();

  void Initialize(float init_T,
                  float norm_area,
                  float norm_wirelength,
                  float norm_outline_penalty,
                  float norm_boundary_penalty,
                  float norm_macro_blockage_penalty,
                  float norm_location_penalty);

  void SetSeq(const std::vector<int>& pos_seq, const std::vector<int>& neg_seq);
  void AlignMacro();
  float GetInitT() const { return init_T_; }
  float GetNormArea() const { return norm_area_; }
  float GetNormWirelength() const { return norm_wirelength_; }
  float GetNormOutlinePenalty() const { return norm_outline_penalty_; }
  float GetNormBoundaryPenalty() const { return norm_boundary_penalty_; }
  float GetNormMacroBlockagePenalty() const
  {
    return norm_macro_blockage_penalty_;
  }
  float GetNormLocationPenalty() const { return norm_location_penalty_; }

  float GetCost() const
  {
    return NormCost(area_,
                    wirelength_,
                    outline_penalty_,
                    boundary_penalty_,
                    macro_blockage_penalty_,
                    location_penalty_);
  }

  float GetWidth() const { return width_; }
  float GetHeight() const { return height_; }
  float GetArea() const { return area_; }
  float GetWirelength() const { return wirelength_; }
  float GetOutlinePenalty() const { return outline_penalty_; }
  float GetBoundaryPenalty() const { return boundary_penalty_; }
  float GetMacroBlockagePenalty() const { return macro_blockage_penalty_; }
  float GetLocationPenalty() const { return location_penalty_; }
  std::vector<Block> GetBlocks() const { return blocks_; }
  std::vector<int> GetPosSeq() const { return pos_seq_; }
  std::vector<int> GetNegSeq() const { return neg_seq_; }

  void ShrinkBlocks();
  bool FitFloorplan();

  bool IsFeasible() const;
};

// wrapper for run function of SimulatedAnnealingCore
void Run(SimulatedAnnealingCore* sa);

void ParseNetFile(
    std::vector<Net*>& nets,
    std::unordered_map<std::string, std::pair<float, float>>& terminal_position,
    const std::string& net_file);

void ParseRegionFile(std::vector<Region*>& regions, const std::string& region_file);
void ParseLocationFile(std::vector<Location*>& locations, const std::string& location_file);

std::vector<Block> Floorplan(
    const std::vector<shape_engine::Cluster*>& clusters,
    utl::Logger* logger,
    float outline_width,
    float outline_height,
    const std::string& net_file,
    const std::string& region_file,
    const std::string& location_file,
    int num_level,
    int num_worker,
    float heat_rate,
    float alpha,
    float beta,
    float gamma,
    float boundary_weight,
    float macro_blockage_weight,
    float location_weight,
    float resize_prob,
    float pos_swap_prob,
    float neg_swap_prob,
    float double_swap_prob,
    float init_prob,
    float rej_ratio,
    int max_num_step,
    int k,
    float c,
    int perturb_per_step,
    float learning_rate,
    float shrink_factor,
    float shrink_freq,
    unsigned seed = 0);

}  // namespace block_placement
