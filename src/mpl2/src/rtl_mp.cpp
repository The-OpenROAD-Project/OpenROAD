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
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "block_placement.h"
#include "mpl2/rtl_mp.h"
#include "pin_alignment.h"
#include "shape_engine.h"
#include "util.h"
#include "utl/Logger.h"

using utl::PAR;

namespace mpl {
using block_placement::Block;
using shape_engine::Cluster;
using shape_engine::Macro;
using std::cout;
using std::endl;
using std::ofstream;
using std::string;
using std::to_string;
using std::unordered_map;
using std::vector;
using utl::Logger;
using utl::MPL;

template <class T>
static void get_param(const unordered_map<string, string>& params,
                      const char* name,
                      T& param,
                      Logger* logger)
{
  auto iter = params.find(name);
  if (iter != params.end()) {
    std::istringstream s(iter->second);
    s >> param;
  }
  logger->info(MPL, 9, "RTL_MP  Param: {}: {}", name, param);
}

bool rtl_macro_placer(const char* config_file,
                      Logger* logger,
                      const char* report_directory,
                      const char* report_file)
{
  logger->report("*** In RTLMP ***");

  string block_file = string(report_directory) + '/' + report_file + ".block";
  string net_file = string(report_directory) + '/' + report_file + ".net";

  // parameters defined in config_file
  // These parameters are related to shape engine
  float min_aspect_ratio = 0.33;
  float dead_space = 0.1;
  float halo_width = 2.0;

  string region_file = string("macro_blockage.txt");

  // These parameters are related to multi-start in shape engine
  int num_thread = 5;
  int num_run = 5;

  unsigned seed = 0;

  // These parameters are related to "Go with the winner"
  float heat_rate = 0.5;
  int num_level = 5;
  int num_worker = 10;

  // These parameters are related to cost function
  float alpha = 0.4;                   // weight for area
  float beta = 0.3;                    // weight for wirelength
  float gamma = 0.3;                   // weight for outline penalty
  float boundary_weight = 0.06;        // weight for pushing macros to boundary
  float macro_blockage_weight = 0.08;  // weight for macro blockage

  float learning_rate
      = 0.01;  // learning rate for dynamic weight in cost function
  float shrink_factor
      = 0.995;  // shrink factor for soft blocks in simulated annealing
  float shrink_freq
      = 0.01;  // shrink frequency for soft blocks in simulated annealing

  // These parameters are related to action probabilities in each step
  float resize_prob = 0.4;
  float pos_swap_prob = 0.2;
  float neg_swap_prob = 0.2;
  float double_swap_prob = 0.2;

  // These parameters are related to fastSA
  float init_prob = 0.95;
  float rej_ratio = 0.95;
  int k = 50;
  float c = 100;
  int max_num_step = 300;
  int perturb_per_step = 3000;

  unordered_map<string, string> params = ParseConfigFile(config_file);

  get_param(params, "min_aspect_ratio", min_aspect_ratio, logger);
  get_param(params, "dead_space", dead_space, logger);
  get_param(params, "learning_rate", learning_rate, logger);
  get_param(params, "shrink_factor", shrink_factor, logger);
  get_param(params, "shrink_freq", shrink_freq, logger);
  get_param(params, "halo_width", halo_width, logger);
  get_param(params, "region_file", region_file, logger);
  get_param(params, "num_thread", num_thread, logger);
  get_param(params, "num_run", num_run, logger);
  get_param(params, "heat_rate", heat_rate, logger);
  get_param(params, "num_level", num_level, logger);
  get_param(params, "num_worker", num_worker, logger);
  get_param(params, "alpha", alpha, logger);
  get_param(params, "beta", beta, logger);
  get_param(params, "gamma", gamma, logger);
  get_param(params, "boundary_weight", boundary_weight, logger);
  get_param(params, "macro_blockage_weight", macro_blockage_weight, logger);
  get_param(params, "resize_prob", resize_prob, logger);
  get_param(params, "pos_swap_prob", pos_swap_prob, logger);
  get_param(params, "neg_swap_prob", neg_swap_prob, logger);
  get_param(params, "double_swap_prob", double_swap_prob, logger);
  get_param(params, "init_prob", init_prob, logger);
  get_param(params, "rej_ratio", rej_ratio, logger);
  get_param(params, "k", k, logger);
  get_param(params, "c", c, logger);
  get_param(params, "max_num_step", max_num_step, logger);
  get_param(params, "perturb_per_step", perturb_per_step, logger);
  get_param(params, "seed", seed, logger);

  float outline_width = 0.0;
  float outline_height = 0.0;
  float outline_lx = 0.0;
  float outline_ly = 0.0;

  vector<Cluster*> clusters = shape_engine::ShapeEngine(outline_width,
                                                        outline_height,
                                                        outline_lx,
                                                        outline_ly,
                                                        min_aspect_ratio,
                                                        dead_space,
                                                        halo_width,
                                                        logger,
                                                        block_file,
                                                        num_thread,
                                                        num_run,
                                                        seed);

  vector<Block> blocks = block_placement::Floorplan(clusters,
                                                    logger,
                                                    outline_width,
                                                    outline_height,
                                                    net_file,
                                                    region_file.c_str(),
                                                    num_level,
                                                    num_worker,
                                                    heat_rate,
                                                    alpha,
                                                    beta,
                                                    gamma,
                                                    boundary_weight,
                                                    macro_blockage_weight,
                                                    resize_prob,
                                                    pos_swap_prob,
                                                    neg_swap_prob,
                                                    double_swap_prob,
                                                    init_prob,
                                                    rej_ratio,
                                                    max_num_step,
                                                    k,
                                                    c,
                                                    perturb_per_step,
                                                    learning_rate,
                                                    shrink_factor,
                                                    shrink_freq,
                                                    seed);

  unordered_map<string, int> block_map;

  for (int i = 0; i < clusters.size(); i++)
    block_map[blocks[i].GetName()] = i;

  for (int i = 0; i < clusters.size(); i++) {
    float x = blocks[block_map[clusters[i]->GetName()]].GetX();
    float y = blocks[block_map[clusters[i]->GetName()]].GetY();
    float width = blocks[block_map[clusters[i]->GetName()]].GetWidth();
    float height = blocks[block_map[clusters[i]->GetName()]].GetHeight();
    clusters[i]->SetPos(x, y);
    clusters[i]->SetFootprint(width, height);
  }

  bool success_flag = pin_alignment::PinAlignment(
      clusters, logger, halo_width, num_thread, num_run, seed);

  if (success_flag == false) {
    logger->report("*** RTLMP Failed ***");
    return false;
  }

  const char* openroad_filename = "./rtl_mp/macro_placement.cfg";
  ofstream file;
  file.open(openroad_filename);
  for (int i = 0; i < clusters.size(); i++) {
    if (clusters[i]->GetNumMacro() > 0) {
      float cluster_lx = clusters[i]->GetX();
      float cluster_ly = clusters[i]->GetY();
      vector<Macro> macros = clusters[i]->GetMacros();
      for (int j = 0; j < macros.size(); j++) {
        string line = macros[j].GetName();
        float lx = outline_lx + cluster_lx + macros[j].GetX() + halo_width;
        float ly = outline_ly + cluster_ly + macros[j].GetY() + halo_width;
        float width = macros[j].GetWidth() - 2 * halo_width;
        float height = macros[j].GetHeight() - 2 * halo_width;
        string orientation = macros[j].GetOrientation();

        if (orientation == string("MX"))
          line += string("  MX  ") + to_string(lx) + string("   ")
                  + to_string(ly + height);
        else if (orientation == string("MY"))
          line += string("  MY  ") + to_string(lx + width) + string("   ")
                  + to_string(ly);
        else if (orientation == string("R180"))
          line += string("  R180  ") + to_string(lx + width) + string("   ")
                  + to_string(ly + height);
        else
          line += string("  R0 ") + to_string(lx) + string("   ")
                  + to_string(ly);

        file << line << endl;
      }
    }
  }

  file.close();

  const char* invs_filename = "./rtl_mp/macro_placement.tcl";
  file.open(invs_filename);
  for (int i = 0; i < clusters.size(); i++) {
    if (clusters[i]->GetNumMacro() > 0) {
      float cluster_lx = clusters[i]->GetX();
      float cluster_ly = clusters[i]->GetY();
      vector<Macro> macros = clusters[i]->GetMacros();
      for (int j = 0; j < macros.size(); j++) {
        string line = "setObjFPlanBox Instance ";
        line += macros[j].GetName() + string("   ");
        float lx = outline_lx + cluster_lx + macros[j].GetX() + halo_width;
        float ly = outline_ly + cluster_ly + macros[j].GetY() + halo_width;
        float width = macros[j].GetWidth() - 2 * halo_width;
        float height = macros[j].GetHeight() - 2 * halo_width;
        line += to_string(lx) + string("   ") + to_string(ly) + string("  ");
        line += to_string(lx + width) + string("  ") + to_string(ly + height);
        file << line << endl;

        line = string("dbSet [dbGet top.insts.name -p ");
        line += macros[j].GetName() + string(" ].orient ");
        string orientation = macros[j].GetOrientation();
        line += orientation;
        file << line << endl;
      }
    }
  }

  file.close();

  // just for quick verification
  const char* floorplan_filename = "./rtl_mp/final_floorplan.txt";
  file.open(floorplan_filename);
  file << "outline_width:  " << outline_width << endl;
  file << "outline_height:  " << outline_height << endl;
  for (int i = 0; i < clusters.size(); i++) {
    float cluster_lx = clusters[i]->GetX();
    float cluster_ly = clusters[i]->GetY();
    float cluster_ux = cluster_lx + clusters[i]->GetWidth();
    float cluster_uy = cluster_ly + clusters[i]->GetHeight();
    string cluster_name = clusters[i]->GetName();
    file << cluster_name << "   ";
    file << cluster_lx << "    ";
    file << cluster_ly << "    ";
    file << cluster_ux << "    ";
    file << cluster_uy << "    ";
    file << endl;
  }
  file << endl;

  for (int i = 0; i < clusters.size(); i++) {
    if (clusters[i]->GetNumMacro() > 0) {
      float cluster_lx = clusters[i]->GetX();
      float cluster_ly = clusters[i]->GetY();
      vector<Macro> macros = clusters[i]->GetMacros();
      for (int j = 0; j < macros.size(); j++) {
        string name = macros[j].GetName();
        float lx = cluster_lx + macros[j].GetX() + halo_width;
        float ly = cluster_ly + macros[j].GetY() + halo_width;
        float width = macros[j].GetWidth() - 2 * halo_width;
        float height = macros[j].GetHeight() - 2 * halo_width;
        file << name << "    ";
        file << lx << "   ";
        file << ly << "   ";
        file << lx + width << "   ";
        file << ly + height << "   ";
        file << endl;
      }
    }
  }

  file.close();

  logger->report("*** Exit RTLMP ***");

  return true;
}

void MacroPlacer2::init(Logger* logger)
{
  logger_ = logger;
}

bool MacroPlacer2::place(const char* config_file,
                         const char* report_directory,
                         const char* report_file)
{
  return rtl_macro_placer(config_file, logger_, report_directory, report_file);
}

}  // namespace mpl
