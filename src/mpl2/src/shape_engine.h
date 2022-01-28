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
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace shape_engine {

// definition of hard macro
class Macro
{
 private:
  float width_ = 0.0;
  float height_ = 0.0;
  float area_ = 0.0;
  float x_ = 0.0;
  float y_ = 0.0;
  float pin_x_ = 0.0;
  float pin_y_ = 0.0;
  odb::dbOrientType orientation_;
  std::string name_;

 public:
  Macro(const std::string& name, float width, float height);

  // overload the comparison operators
  bool operator<(const Macro& macro) const;
  bool operator==(const Macro& macro) const;

  // accessors
  float GetWidth() const { return width_; }
  float GetHeight() const { return height_; }
  float GetX() const { return x_; }
  float GetY() const { return y_; }
  float GetArea() const { return area_; }
  float GetPinX() const { return pin_x_; }
  float GetPinY() const { return pin_y_; }
  std::string GetOrientation() const { return orientation_.getString(); }

  std::string GetName() const { return name_; }

  void SetX(float x) { x_ = x; }
  void SetY(float y) { y_ = y; }
  void SetPinPosition(float pin_x, float pin_y)
  {
    pin_x_ = pin_x;
    pin_y_ = pin_y;
  }

  // operation
  void Flip(bool axis);
};

// definition of clusters:  include std cell clusters and hard macro clusters
class Cluster
{
 private:
  std::string name_;
  float area_ = 0.0;
  std::vector<std::pair<float, float>> aspect_ratio_;
  std::vector<Macro> macros_;
  float x_ = 0.0;
  float y_ = 0.0;
  float width_ = 0.0;
  float height_ = 0.0;

 public:
  Cluster(){};
  Cluster(const std::string& name) { name_ = name; }

  std::string GetName() const { return name_; }
  float GetArea() const { return area_; }
  std::vector<std::pair<float, float>> GetAspectRatio() const
  {
    return aspect_ratio_;
  }
  std::vector<Macro> GetMacros() const { return macros_; }
  int GetNumMacro() const { return macros_.size(); }

  void SetPos(float x, float y)
  {
    x_ = x;
    y_ = y;
  }

  void SetFootprint(float width, float height)
  {
    width_ = width;
    height_ = height;
  }

  float GetX() const { return x_; }
  float GetY() const { return y_; }
  float GetWidth() const { return width_; }
  float GetHeight() const { return height_; }

  void SetArea(float area) { area_ = area; }
  void AddArea(float area) { area_ += area; }
  void SetAspectRatio(const std::vector<std::pair<float, float>>& aspect_ratio)
  {
    aspect_ratio_ = aspect_ratio;
  }

  void AddAspectRatio(std::pair<float, float> ar)
  {
    aspect_ratio_.push_back(ar);
  }

  void AddMacro(const Macro& macro) { macros_.push_back(macro); }
  void SetMacros(const std::vector<Macro>& macros) { macros_ = macros; }

  bool operator==(const Cluster& cluster) const
  {
    if (!(macros_.size() == cluster.macros_.size()))
      return false;

    for (unsigned int i = 0; i < macros_.size(); i++) {
      if (!(macros_[i] == cluster.macros_[i]))
        return false;
    }

    return true;
  }

  void SortMacro() { std::sort(macros_.begin(), macros_.end()); }
};

class SimulatedAnnealingCore
{
 private:
  float init_prob_ = 0.95;
  float rej_ratio_ = 0.95;
  int max_num_step_ = 1000;
  int k_;
  float c_;
  int perturb_per_step_ = 60;
  float alpha_ = 0.5;

  float outline_width_ = 0.0;
  float outline_height_ = 0.0;

  float height_ = 0.0;
  float width_ = 0.0;
  float area_ = 0.0;

  float pre_height_ = 0.0;
  float pre_width_ = 0.0;
  float pre_area_ = 0.0;

  float norm_blockage_ = 0.0;
  float norm_area_ = 0.0;
  float init_T_ = 0.0;

  int action_id_ = -1;

  std::vector<Macro*> macros_;

  std::vector<int> pos_seq_;
  std::vector<int> neg_seq_;
  std::vector<int> pre_pos_seq_;
  std::vector<int> pre_neg_seq_;

  float pos_swap_prob_ = 0.4;
  float neg_swap_prob_ = 0.8;     // actually prob = 0.4
  float double_swap_prob_ = 1.0;  // actually prob = 0.2

  std::mt19937 generator_;
  std::uniform_real_distribution<float> distribution_;

  void PackFloorplan();
  void SingleSwap(bool flag);  // true for pos swap and false for neg swap
  void DoubleSwap();
  void Perturb();
  void Restore();
  void Initialize();
  float CalculateBlockage();
  float NormCost(float area, float blockage_cost);
  void FastSA();

 public:
  // constructor
  SimulatedAnnealingCore(const std::vector<Macro>& macros,
                         float outline_width,
                         float outline_height,
                         float init_prob = 0.95,
                         float rej_ratio = 0.95,
                         int max_num_step = 1000,
                         int k = 100,
                         float c = 100,
                         int perturb_per_step = 60,
                         float alpha = 0.5,
                         float pos_swap_prob = 0.4,
                         float neg_swap_prob = 0.4,
                         float double_swap_prob = 0.2,
                         unsigned seed = 0);

  void Run() { FastSA(); }

  float GetWidth() const { return width_; }
  float GetHeight() const { return height_; }
  float GetArea() const { return area_; }

  void WriteFloorplan(const std::string& file_name) const;
};

// wrapper for run function of SimulatedAnnealingCore
void Run(SimulatedAnnealingCore* sa);

// Macro Tile Engine
std::vector<std::pair<float, float>> TileMacro(const std::string& report_directory,
                                               const std::vector<Macro>& macros,
                                               const std::string& cluster_name,
                                               float& final_area,
                                               float outline_width,
                                               float outline_height,
                                               int num_thread = 10,
                                               int num_run = 20,
                                               unsigned seed = 0);

// Parse Block File
void ParseBlockFile(std::vector<Cluster*>& clusters,
                    const char* file_name,
                    float& outline_width,
                    float& outline_height,
                    float& outline_lx,
                    float& outline_ly,
                    utl::Logger* logger,
                    float dead_space,
                    float halo_width);

// Top level function : Shape Engine
std::vector<Cluster*> ShapeEngine(float& outline_width,
                                  float& outline_height,
                                  float& outline_lx,
                                  float& outline_ly,
                                  float min_aspect_ratio,
                                  float dead_space,
                                  float halo_width,
                                  utl::Logger* logger,
                                  const std::string& report_directory,
                                  const std::string& block_file,
                                  int num_thread = 10,
                                  int num_run = 20,
                                  unsigned seed = 0);

}  // namespace shape_engine
