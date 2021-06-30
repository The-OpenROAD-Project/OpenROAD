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

#include <algorithm>
#include <cmath>
#include <fstream>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "shape_engine.h"
#include "util.h"
#include "utl/Logger.h"

namespace shape_engine {
using std::abs;
using std::cout;
using std::endl;
using std::exp;
using std::fstream;
using std::getline;
using std::ios;
using std::log;
using std::pair;
using std::pow;
using std::sort;
using std::stof;
using std::string;
using std::swap;
using std::thread;
using std::to_string;
using std::unordered_map;
using std::vector;
using utl::Logger;
using utl::MPL;

Macro::Macro(const std::string& name, float width, float height)
{
  name_ = name;
  width_ = width;
  height_ = height;
  area_ = width_ * height_;
}

bool Macro::operator<(const Macro& macro) const
{
  if (width_ != macro.width_)
    return width_ < macro.width_;

  return height_ < macro.height_;
}

bool Macro::operator==(const Macro& macro) const
{
  if (width_ == macro.width_ && height_ == macro.height_)
    return true;
  else
    return false;
}

void Macro::Flip(bool axis)
{
  if (axis == true) {
    orientation_ = orientation_.flipY();
    pin_x_ = width_ - pin_x_;
  } else {
    orientation_ = orientation_.flipX();
    pin_y_ = height_ - pin_y_;
  }
}

SimulatedAnnealingCore::SimulatedAnnealingCore(const std::vector<Macro>& macros,
                                               float outline_width,
                                               float outline_height,
                                               float init_prob,
                                               float rej_ratio,
                                               int max_num_step,
                                               int k,
                                               float c,
                                               int perturb_per_step,
                                               float alpha,
                                               float pos_swap_prob,
                                               float neg_swap_prob,
                                               float double_swap_prob,
                                               unsigned seed)
{
  outline_width_ = outline_width;
  outline_height_ = outline_height;

  init_prob_ = init_prob;
  rej_ratio_ = rej_ratio;
  max_num_step_ = max_num_step;
  k_ = k;
  c_ = c;
  perturb_per_step_ = perturb_per_step;
  alpha_ = alpha;

  pos_swap_prob_ = pos_swap_prob;
  neg_swap_prob_ = pos_swap_prob_ + neg_swap_prob;
  double_swap_prob_ = neg_swap_prob_ + double_swap_prob;

  for (unsigned int i = 0; i < macros.size(); i++) {
    pos_seq_.push_back(i);
    neg_seq_.push_back(i);

    pre_pos_seq_.push_back(i);
    pre_neg_seq_.push_back(i);

    Macro* macro = new Macro(macros[i]);
    macros_.push_back(macro);
  }

  std::mt19937 randGen(seed);
  generator_ = randGen;
  std::uniform_real_distribution<float> distribution(0.0, 1.0);
  distribution_ = distribution;

  // Initialize init_T_, norm_blockage_, norm_area_
  Initialize();
}

void SimulatedAnnealingCore::PackFloorplan()
{
  for (int i = 0; i < macros_.size(); i++) {
    macros_[i]->SetX(0.0);
    macros_[i]->SetY(0.0);
  }

  // calculate X position
  vector<pair<int, int>> match(macros_.size());
  for (int i = 0; i < pos_seq_.size(); i++) {
    match[pos_seq_[i]].first = i;
    match[neg_seq_[i]].second = i;
  }

  vector<float> length(macros_.size());
  for (int i = 0; i < macros_.size(); i++)
    length[i] = 0.0;

  for (int i = 0; i < pos_seq_.size(); i++) {
    int b = pos_seq_[i];
    int p = match[b].second;
    macros_[b]->SetX(length[p]);
    float t = macros_[b]->GetX() + macros_[b]->GetWidth();
    for (int j = p; j < neg_seq_.size(); j++)
      if (t > length[j])
        length[j] = t;
      else
        break;
  }

  width_ = length[macros_.size() - 1];

  // calulate Y position
  vector<int> pos_seq(pos_seq_.size());
  int num_blocks = pos_seq_.size();
  for (int i = 0; i < num_blocks; i++)
    pos_seq[i] = pos_seq_[num_blocks - 1 - i];

  for (int i = 0; i < num_blocks; i++) {
    match[pos_seq[i]].first = i;
    match[neg_seq_[i]].second = i;
  }

  for (int i = 0; i < num_blocks; i++)
    length[i] = 0.0;

  for (int i = 0; i < num_blocks; i++) {
    int b = pos_seq[i];
    int p = match[b].second;
    macros_[b]->SetY(length[p]);
    float t = macros_[b]->GetY() + macros_[b]->GetHeight();
    for (int j = p; j < num_blocks; j++)
      if (t > length[j])
        length[j] = t;
      else
        break;
  }

  height_ = length[num_blocks - 1];
  area_ = width_ * height_;
}

void SimulatedAnnealingCore::SingleSwap(bool flag)
{
  int index1 = (int) (floor((distribution_) (generator_) *macros_.size()));
  int index2 = (int) (floor((distribution_) (generator_) *macros_.size()));
  while (index1 == index2) {
    index2 = (int) (floor((distribution_) (generator_) *macros_.size()));
  }

  if (flag)
    swap(pos_seq_[index1], pos_seq_[index2]);
  else
    swap(neg_seq_[index1], neg_seq_[index2]);
}

void SimulatedAnnealingCore::DoubleSwap()
{
  int index1 = (int) (floor((distribution_) (generator_) *macros_.size()));
  int index2 = (int) (floor((distribution_) (generator_) *macros_.size()));
  while (index1 == index2) {
    index2 = (int) (floor((distribution_) (generator_) *macros_.size()));
  }

  swap(pos_seq_[index1], pos_seq_[index2]);
  swap(neg_seq_[index1], neg_seq_[index2]);
}

void SimulatedAnnealingCore::Perturb()
{
  if (macros_.size() == 1)
    return;

  pre_pos_seq_ = pos_seq_;
  pre_neg_seq_ = neg_seq_;
  pre_width_ = width_;
  pre_height_ = height_;
  pre_area_ = area_;

  float op = (distribution_) (generator_);
  if (op <= pos_swap_prob_) {
    action_id_ = 0;
    SingleSwap(true);
  } else if (op <= neg_swap_prob_) {
    action_id_ = 1;
    SingleSwap(false);
  } else {
    action_id_ = 2;
    DoubleSwap();
  }

  PackFloorplan();
}

void SimulatedAnnealingCore::Restore()
{
  if (action_id_ == 0)
    pos_seq_ = pre_pos_seq_;
  else if (action_id_ == 1)
    neg_seq_ = pre_neg_seq_;
  else {
    pos_seq_ = pre_pos_seq_;
    neg_seq_ = pre_neg_seq_;
  }

  width_ = pre_width_;
  height_ = pre_height_;
  area_ = pre_area_;
}

float SimulatedAnnealingCore::CalculateBlockage()
{
  float blockage_cost = 0.0;

  if (width_ > outline_width_ && height_ > outline_height_)
    blockage_cost = width_ * height_ - outline_width_ * outline_height_;
  else if (width_ > outline_width_ && height_ <= outline_height_)
    blockage_cost = (width_ - outline_width_) * outline_height_;
  else if (width_ <= outline_width_ && height_ > outline_height_)
    blockage_cost = outline_width_ * (height_ - outline_height_);
  else
    blockage_cost = 0.0;

  return blockage_cost;
}

float SimulatedAnnealingCore::NormCost(float area, float blockage_cost)
{
  float cost = alpha_ * area / norm_area_;
  if (norm_blockage_ > 0.0)
    cost += (1 - alpha_) * blockage_cost / norm_blockage_;

  return cost;
}

void SimulatedAnnealingCore::Initialize()
{
  vector<float> area_list;
  vector<float> blockage_list;
  vector<float> norm_cost_list;

  float avg_area = 0.0;
  float avg_blockage = 0.0;

  float area = 0.0;
  float blockage = 0.0;
  float norm_cost = 0.0;

  for (int i = 0; i < perturb_per_step_; i++) {
    Perturb();
    area = area_;
    blockage = CalculateBlockage();
    avg_area += area;
    avg_blockage += blockage;
    area_list.push_back(area);
    blockage_list.push_back(blockage);
  }

  norm_area_ = avg_area / perturb_per_step_;
  norm_blockage_ = avg_blockage / perturb_per_step_;

  for (int i = 0; i < area_list.size(); i++) {
    norm_cost = NormCost(area_list[i], blockage_list[i]);
    norm_cost_list.push_back(norm_cost);
  }

  float delta_cost = 0.0;
  for (int i = 1; i < norm_cost_list.size(); i++)
    delta_cost += abs(norm_cost_list[i] - norm_cost_list[i - 1]);

  delta_cost = delta_cost / (norm_cost_list.size() - 1);
  init_T_ = (-1) * delta_cost / log(init_prob_);
}

void SimulatedAnnealingCore::WriteFloorplan(const std::string& file_name) const
{
  std::ofstream file;
  file.open(file_name);
  for (unsigned int i = 0; i < macros_.size(); i++) {
    file << macros_[i]->GetX() << "   ";
    file << macros_[i]->GetY() << "   ";
    file << macros_[i]->GetX() + macros_[i]->GetWidth() << "   ";
    file << macros_[i]->GetY() + macros_[i]->GetHeight() << "   ";
    file << std::endl;
  }

  file.close();
}

void SimulatedAnnealingCore::FastSA()
{
  int step = 1;

  float area = area_;
  float blockage = CalculateBlockage();
  float pre_cost = NormCost(area, blockage);
  float cost = pre_cost;
  float delta_cost = 0.0;
  float best_cost = cost;
  // vector<int> best_pos_seq = pos_seq_;
  // vector<int> best_neg_seq = neg_seq_;

  float rej_num = 0.0;
  float T = init_T_;
  float rej_threshold = rej_ratio_ * perturb_per_step_;

  // while(step <= max_num_step_ && rej_num <= rej_threshold) {
  while (step <= max_num_step_ && rej_num <= rej_threshold) {
    rej_num = 0.0;
    for (int i = 0; i < perturb_per_step_; i++) {
      Perturb();
      area = area_;
      blockage = CalculateBlockage();
      cost = NormCost(area, blockage);

      delta_cost = cost - pre_cost;
      float num = distribution_(generator_);
      float prob = (delta_cost > 0.0) ? exp((-1) * delta_cost / T) : 1;

      if (delta_cost <= 0 || num <= prob) {
        pre_cost = cost;
        if (best_cost > cost) {
          best_cost = cost;
          // best_pos_seq = pos_seq_;
          // best_neg_seq = neg_seq_;
        }
      } else {
        cost = pre_cost;
        rej_num += 1.0;
        Restore();
      }
    }
    step++;
    // if(step <= k_)
    //    T = init_T_ / (step * c_);
    // else
    //    T = init_T_ / step;
    T = T * 0.995;
  }

  // pos_seq_ = best_pos_seq;
  // neg_seq_ = best_neg_seq;
  PackFloorplan();
}

void Run(SimulatedAnnealingCore* sa)
{
  sa->Run();
}

// Macro Tile Engine
vector<pair<float, float>> TileMacro(const vector<Macro>& macros,
                                     const string& cluster_name,
                                     float& final_area,
                                     float outline_width,
                                     float outline_height,
                                     int num_thread,
                                     int num_run,
                                     unsigned seed)
{
  vector<pair<float, float>> aspect_ratio;

  if (macros.size() == 1) {
    float width = macros[0].GetWidth();
    float height = macros[0].GetHeight();
    float ar = height / width;
    final_area = height * width;
    aspect_ratio.push_back(pair<float, float>(ar, ar));
    return aspect_ratio;
  }

  // parameterse related to fastSA
  float init_prob = 0.6;
  float rej_ratio = 1.0;
  int max_num_step = 3000;
  int k = 5000;
  float c = 1.0;
  int perturb_per_step = 2 * macros.size();
  float alpha = 0.5;
  float pos_swap_prob = 0.45;
  float neg_swap_prob = 0.45;
  float double_swap_prob = 0.1;

  std::mt19937 rand_generator(seed);
  vector<int> seed_list;
  for (int i = 0; i < num_thread; i++)
    seed_list.push_back((unsigned) rand_generator());

  int remaining_run = num_run;
  int run_thread = num_thread;
  int sa_id = 0;

  vector<float> factor_list(num_thread);
  if (num_thread == 1)
    factor_list[0] = 1.0;

  if (num_thread > 1) {
    // float step = 1.0 / (num_thread - 1);
    // for (int i = 0; i < num_thread; i++)
    //   factor_list[i] = pow(10.0, 1.0 - step * i);
    for (int i = 0; i < num_thread / 2; i++)
      factor_list[i] = 1.0 / (i + 1);

    for (int i = num_thread / 2; i < num_thread; i++)
      factor_list[i] = 1.0 * (i + 1 - num_thread / 2);
  }

  vector<SimulatedAnnealingCore*> sa_vector;
  while (remaining_run > 0) {
    run_thread = num_thread;
    if (remaining_run < num_thread)
      run_thread = remaining_run;

    for (int i = 0; i < run_thread; i++) {
      float factor = factor_list[i];
      cout << "factor:   " << factor << endl;
      float temp_outline_width = outline_width * factor;
      float temp_outline_height
          = outline_width * outline_height / temp_outline_width;
      // float temp_outline_height = outline_height;
      SimulatedAnnealingCore* sa
          = new SimulatedAnnealingCore(macros,
                                       temp_outline_width,
                                       temp_outline_height,
                                       init_prob,
                                       rej_ratio,
                                       max_num_step,
                                       k,
                                       c,
                                       perturb_per_step,
                                       alpha,
                                       pos_swap_prob,
                                       neg_swap_prob,
                                       double_swap_prob,
                                       seed_list[i]);
      sa_vector.push_back(sa);
    }

    vector<thread> threads;
    for (int i = 0; i < run_thread; i++)
      threads.push_back(thread(Run, sa_vector[sa_id++]));

    for (auto& th : threads)
      th.join();

    remaining_run = remaining_run - run_thread;
  }

  vector<pair<int, float>> rank;
  for (int i = 0; i < sa_vector.size(); i++)
    rank.push_back(pair<int, float>(i, sa_vector[i]->GetArea()));

  string cluster_file_name(cluster_name);
  for (int i = 0; i < cluster_file_name.size(); i++) {
    if (cluster_file_name[i] == '/') {
      cluster_file_name[i] = '*';
    }
  }

  for (int i = 0; i < sa_vector.size(); i++) {
    string file_name = string("./rtl_mp/") + cluster_file_name + string("_")
                       + to_string(i) + string(".txt");
    sa_vector[i]->WriteFloorplan(file_name);
  }

  sort(rank.begin(), rank.end(), SortBySecond);

  vector<pair<float, float>> footprints;
  for (int i = 0; i < sa_vector.size(); i++) {
    int index = rank[i].first;
    float width = sa_vector[index]->GetWidth();
    float height = sa_vector[index]->GetHeight();
    if (width <= outline_width && height <= outline_height)
      footprints.push_back(pair<float, float>(width, height));
  }

  for (int i = 0; i < sa_vector.size(); i++)
    delete sa_vector[i];

  sa_vector.clear();

  if (footprints.size() == 0)
    throw std::invalid_argument(std::string(
        "Invalid Floorplan.  Please increase the size of floorplan"));

  float base_area = footprints[0].first * footprints[0].second;
  final_area = base_area;
  // vector<pair<float, float> > aspect_ratio;
  vector<float> ar_list;
  for (int i = 0; i < footprints.size(); i++) {
    if (footprints[i].first * footprints[i].second <= base_area * 1.01) {
      float ar = footprints[i].second / footprints[i].first;
      ar_list.push_back(ar);
    }
  }

  sort(ar_list.begin(), ar_list.end());

  vector<float> temp_ar_list;
  temp_ar_list.push_back(ar_list[0]);
  float temp_ar = ar_list[0];
  for (int i = 1; i < ar_list.size(); i++) {
    if (ar_list[i] > temp_ar) {
      temp_ar = ar_list[i];
      temp_ar_list.push_back(temp_ar);
    }
  }

  for (int i = 0; i < temp_ar_list.size(); i++) {
    aspect_ratio.push_back(
        pair<float, float>(temp_ar_list[i], temp_ar_list[i]));
  }

  return aspect_ratio;
}

void ParseBlockFile(vector<Cluster*>& clusters,
                    const string& block_file,
                    float& outline_width,
                    float& outline_height,
                    float& outline_lx,
                    float& outline_ly,
                    Logger* logger,
                    float dead_space,
                    float halo_width)
{
  // Read block file
  fstream f;
  string line;
  vector<string> content;
  f.open(block_file, ios::in);
  while (getline(f, line))
    content.push_back(line);

  f.close();
  // get outline information
  vector<string> words = Split(content[1]);
  outline_width = stof(words[words.size() - 1]);

  words = Split(content[2]);
  outline_height = stof(words[words.size() - 1]);

  words = Split(content[3]);
  outline_lx = stof(words[words.size() - 1]);

  words = Split(content[4]);
  outline_ly = stof(words[words.size() - 1]);

  float macro_area = 0.0;
  float std_cell_area = 0.0;

  int i = 0;
  while (i < content.size()) {
    words = Split(content[i]);
    if (words.size() == 2 && words[0] == string("cluster:")) {
      string name = words[words.size() - 1];
      Cluster* cluster = new Cluster(name);
      clusters.push_back(cluster);
      i++;
      vector<string> temp_words = Split(content[i]);
      float area = stof(temp_words[temp_words.size() - 1]);
      float block_area = 0.0;
      i++;
      while (i < content.size() & (Split(content[i])).size() != 0) {
        temp_words = Split(content[i]);
        string macro_name = temp_words[0];
        float macro_width = stof(temp_words[1]);
        float macro_height = stof(temp_words[2]);
        macro_width += 2 * halo_width;
        macro_height += 2 * halo_width;
        block_area += macro_width * macro_height;
        cluster->AddMacro(Macro(macro_name, macro_width, macro_height));
        i++;
      }

      if (block_area > 0.0) {
        area = block_area;
        macro_area += block_area;
      } else {
        area = area;
        std_cell_area += area;
      }

      cluster->SetArea(area);
      cluster->SortMacro();
    } else {
      i++;
    }
  }

  float chip_area = outline_width * outline_height;
  float std_cell_util
      = std_cell_area / ((chip_area - macro_area) * (1 - dead_space));

  logger->info(MPL, 1001, "Shape_Engine Outline width:  {}", outline_width);
  logger->info(MPL, 1002, "Shape_Engine Outline height: {}", outline_height);
  logger->info(MPL, 1003, "Shape_Engine Chip area: {}", chip_area);
  logger->info(MPL, 1004, "Shape_Engine Macro area:  {}", macro_area);
  logger->info(MPL, 1005, "Shape_Engine Std cell area: {}", std_cell_area);
  logger->info(MPL, 1006, "Shape_Engine Std cell util: {}", std_cell_util);

  for (int j = 0; j < clusters.size(); j++) {
    if (clusters[j]->GetNumMacro() == 0) {
      float area = clusters[j]->GetArea() / std_cell_util;
      clusters[j]->SetArea(area);
    }
  }
}

vector<Cluster*> ShapeEngine(float& outline_width,
                             float& outline_height,
                             float& outline_lx,
                             float& outline_ly,
                             float min_aspect_ratio,
                             float dead_space,
                             float halo_width,
                             Logger* logger,
                             const string& block_file,
                             int num_thread,
                             int num_run,
                             unsigned seed)
{
  // we assume min_aspect_ratio * max_aspect_ratio = 1.0;
  if (min_aspect_ratio >= 1.0)
    min_aspect_ratio = 1.0;

  float max_aspect_ratio = 1.0 / min_aspect_ratio;
  vector<Cluster*> clusters;

  ParseBlockFile(clusters,
                 block_file,
                 outline_width,
                 outline_height,
                 outline_lx,
                 outline_ly,
                 logger,
                 dead_space,
                 halo_width);

  // Classify all the clusters into different types
  vector<int> class_list(clusters.size());
  for (int i = 0; i < clusters.size(); i++)
    class_list[i] = -1;

  for (int i = 0; i < clusters.size(); i++) {
    if (class_list[i] == -1) {
      class_list[i] = i;
      for (int j = i + 1; j < clusters.size(); j++) {
        if ((*clusters[i]) == (*clusters[j])) {
          class_list[j] = i;
        }
      }
    }
  }

  // Generate Aspect Ratio based on cluster types
  for (int i = 0; i < clusters.size(); i++) {
    if (clusters[i]->GetNumMacro() != 0 && class_list[i] == i) {
      float final_area = 0.0;
      vector<pair<float, float>> aspect_ratio
          = TileMacro(clusters[i]->GetMacros(),
                      clusters[i]->GetName(),
                      final_area,
                      outline_width,
                      outline_height,
                      num_thread,
                      num_run,
                      seed);
      clusters[i]->SetAspectRatio(aspect_ratio);
      clusters[i]->SetArea(final_area);
    }
  }

  for (int i = 0; i < clusters.size(); i++) {
    if (clusters[i]->GetNumMacro() == 0) {
      clusters[i]->AddAspectRatio(
          pair<float, float>(min_aspect_ratio, max_aspect_ratio));
    } else if (class_list[i] != i) {
      float area = clusters[class_list[i]]->GetArea();
      vector<pair<float, float>> aspect_ratio
          = clusters[class_list[i]]->GetAspectRatio();
      clusters[i]->SetAspectRatio(aspect_ratio);
      clusters[i]->SetArea(area);
    } else {
      ;
    }
  }

  // Verify the results
  for (int i = 0; i < clusters.size(); i++) {
    string output_info = "Cluster: ";
    output_info += clusters[i]->GetName() + ", ";
    output_info += "area: ";
    output_info += to_string(clusters[i]->GetArea()) + ", ";
    output_info += "aspect_ratio: ";
    vector<pair<float, float>> aspect_ratio = clusters[i]->GetAspectRatio();
    for (int j = 0; j < aspect_ratio.size(); j++) {
      output_info += "(";
      output_info += to_string(aspect_ratio[j].first);
      output_info += ",";
      output_info += to_string(aspect_ratio[j].second);
      output_info += ") ";
    }

    logger->info(MPL,  1007, "Shape_Engine {}", output_info);
  }

  return clusters;
}

}  // namespace shape_engine
