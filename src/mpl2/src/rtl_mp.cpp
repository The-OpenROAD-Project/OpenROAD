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

#include "mpl2/rtl_mp.h"

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
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "pin_alignment.h"
#include "shape_engine.h"
#include "util.h"
#include "utl/Logger.h"

using utl::PAR;

namespace mpl {
using block_placement::Block;
using odb::dbDatabase;
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
  logger->info(MPL, 9, "RTL-MP param: {}: {}.", name, param);
}

bool rtl_macro_placer(const char* config_file,
                      Logger* logger,
                      dbDatabase* db,
                      const char* report_directory,
                      const float area_wt,
                      const float wirelength_wt,
                      const float outline_wt,
                      const float boundary_wt,
                      const float macro_blockage_wt,
                      const float location_wt,
                      const float notch_wt,
                      const float dead_space,
                      const float macro_halo,
                      const char* report_file,
                      const char* macro_blockage_file,
                      const char* prefer_location_file)
{
  logger->report("Start RTL-MP");

  string block_file = string(report_directory) + '/' + report_file + ".block";
  string net_file = string(report_directory) + '/' + report_file + ".net";

  //
  //  Default for Parameters
  //
  // These parameters are related to shape engine
  float min_aspect_ratio = 0.29;

  string region_file = string(macro_blockage_file);
  string location_file = string(prefer_location_file);

  // These parameters are related to multi-start in shape engine
  int num_thread = ord::OpenRoad::openRoad()
                       ->getThreadCount();  // set to max threads in OpenROAD
  int num_run = 10;

  unsigned seed = 0;

  // These parameters are related to "Go with the winner"
  float heat_rate = 0.5;
  int num_level = 1;
  int num_worker = 10;

  // These parameters are related to cost function
  float alpha = area_wt;       // weight for area
  float beta = wirelength_wt;  // weight for wirelength auto tuned value
  float gamma = outline_wt;    // weight for outline penalty - auto tuned value
  float boundary_weight = boundary_wt;  // weight for pushing macros to boundary
  float macro_blockage_weight = macro_blockage_wt;  // weight for macro blockage
  float location_weight = location_wt;  // weight for preferred location
  float notch_weight = notch_wt;        // weight for notch
  float halo_width = macro_halo;        // halo width around macros

  float learning_rate
      = 0.00;  // learning rate for dynamic weight in cost function
  float shrink_factor
      = 0.999;  // shrink factor for soft blocks in simulated annealing
  float shrink_freq
      = 0.1;  // shrink frequency for soft blocks in simulated annealing

  // These parameters are related to action probabilities in each step
  float resize_prob = 0.1;
  float pos_swap_prob = 0.3;
  float neg_swap_prob = 0.3;
  float double_swap_prob = 0.3;

  // These parameters are related to fastSA
  float init_prob = 0.95;
  float rej_ratio = 0.95;
  int k = 5000000;
  float c = 1000.0;
  int max_num_step = 4000;
  int perturb_per_step = 400;

  int snap_layer = 4;

  //
  // config_file is not required in the default flow.
  // It is still supported for developer tuning of the parameters
  //
  if (strcmp(config_file, "") != 0) {
    unordered_map<string, string> params = ParseConfigFile(config_file);

    get_param(params, "min_aspect_ratio", min_aspect_ratio, logger);
    get_param(params, "learning_rate", learning_rate, logger);
    get_param(params, "shrink_factor", shrink_factor, logger);
    get_param(params, "shrink_freq", shrink_freq, logger);
    get_param(params, "halo_width", halo_width, logger);
    get_param(params, "region_file", region_file, logger);
    get_param(params, "location_file", location_file, logger);
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
    get_param(params, "location_weight", location_weight, logger);
    get_param(params, "notch_weight", notch_weight, logger);
    get_param(params, "resize_prob", resize_prob, logger);
    get_param(params, "pos_swap_prob", pos_swap_prob, logger);
    get_param(params, "neg_swap_prob", neg_swap_prob, logger);
    get_param(params, "double_swap_prob", double_swap_prob, logger);
    get_param(params, "init_prob", init_prob, logger);
    get_param(params, "rej_ratio", rej_ratio, logger);
    get_param(params, "k", k, logger);
    get_param(params, "c", c, logger);
    get_param(params, "snap_layer", snap_layer, logger);
    get_param(params, "max_num_step", max_num_step, logger);
    get_param(params, "perturb_per_step", perturb_per_step, logger);
    get_param(params, "seed", seed, logger);
  }

  float outline_width = 0.0;
  float outline_height = 0.0;
  float outline_lx = 0.0;
  float outline_ly = 0.0;

  if (num_thread <= 0)
    logger->error(MPL,
                  10,
                  "num_thread shoule be large than 0. (num_thread : {}).",
                  num_thread);

  if (num_run <= 0)
    logger->error(
        MPL, 11, "num_run shoule be large than 0. (num_run : {}).", num_run);

  if (num_level <= 0)
    logger->error(MPL,
                  12,
                  "num_level shoule be large than 0. (num_level : {}).",
                  num_level);

  if (perturb_per_step <= 0)
    logger->error(
        MPL,
        13,
        "perturb_per_step shoule be large than 0. (perturb_per_step : {}).",
        perturb_per_step);

  vector<Cluster*> clusters = shape_engine::ShapeEngine(outline_width,
                                                        outline_height,
                                                        outline_lx,
                                                        outline_ly,
                                                        min_aspect_ratio,
                                                        dead_space,
                                                        halo_width,
                                                        logger,
                                                        report_directory,
                                                        block_file,
                                                        num_thread,
                                                        num_run,
                                                        seed);

  if (clusters.empty()) {
    return true;  // nothing to place
  }

  vector<Block> blocks = block_placement::Floorplan(clusters,
                                                    logger,
                                                    outline_width,
                                                    outline_height,
                                                    net_file,
                                                    region_file.c_str(),
                                                    location_file.c_str(),
                                                    num_level,
                                                    num_worker,
                                                    heat_rate,
                                                    alpha,
                                                    beta,
                                                    gamma,
                                                    boundary_weight,
                                                    macro_blockage_weight,
                                                    location_weight,
                                                    notch_weight,
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

  bool success_flag = pin_alignment::PinAlignment(clusters,
                                                  logger,
                                                  report_directory,
                                                  halo_width,
                                                  num_thread,
                                                  num_run,
                                                  seed);

  if (success_flag == false) {
    logger->report("RTL-MP failed");
    return false;
  }

  // Get Block Placement Grid
  // The Block Placement Grid is based on the pitch of the bottom horizontal and
  // vertical routing layers. The Block Placement Grid is one pitch wide and one
  // pitch tall. TR requires the macro pins to be on grid.
  odb::dbTech* tech = db->getTech();
  const int dbu = tech->getDbUnitsPerMicron();
  float pitch_x
      = static_cast<float>(tech->findRoutingLayer(snap_layer)->getPitchX())
        / dbu;
  float pitch_y
      = static_cast<float>(tech->findRoutingLayer(snap_layer)->getPitchY())
        / dbu;

  string openroad_filename
      = string("./") + string(report_directory) + "/macro_placement.cfg";
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
        float ux = lx + width;
        float uy = ly + height;
        lx = round(lx / pitch_x) * pitch_x;
        ux = round(ux / pitch_x) * pitch_x;
        ly = round(ly / pitch_y) * pitch_y;
        uy = round(uy / pitch_y) * pitch_y;

        if (orientation == string("MX"))
          line += string("  MX  ") + to_string(lx) + string("   ")
                  + to_string(uy);
        else if (orientation == string("MY"))
          line += string("  MY  ") + to_string(ux) + string("   ")
                  + to_string(ly);
        else if (orientation == string("R180"))
          line += string("  R180  ") + to_string(ux) + string("   ")
                  + to_string(uy);
        else
          line += string("  R0 ") + to_string(lx) + string("   ")
                  + to_string(ly);

        file << line << endl;
      }
    }
  }

  file.close();

  string txt_filename
      = string("./") + string(report_directory) + "/macro_placement.txt";
  file.open(txt_filename);
  for (int i = 0; i < clusters.size(); i++) {
    if (clusters[i]->GetNumMacro() > 0) {
      float cluster_lx = clusters[i]->GetX();
      float cluster_ly = clusters[i]->GetY();
      vector<Macro> macros = clusters[i]->GetMacros();
      for (int j = 0; j < macros.size(); j++) {
        string line = macros[j].GetName() + string("   ");
        float lx = outline_lx + cluster_lx + macros[j].GetX() + halo_width;
        float ly = outline_ly + cluster_ly + macros[j].GetY() + halo_width;
        float width = macros[j].GetWidth() - 2 * halo_width;
        float height = macros[j].GetHeight() - 2 * halo_width;
        float ux = lx + width;
        float uy = ly + height;
        lx = round(lx / pitch_x) * pitch_x;
        ux = round(ux / pitch_x) * pitch_x;
        ly = round(ly / pitch_y) * pitch_y;
        uy = round(uy / pitch_y) * pitch_y;

        line += to_string(lx) + string("   ") + to_string(ly) + string("  ");
        line += to_string(ux) + string("  ") + to_string(uy);
        line += string("   ") + macros[j].GetOrientation();
        file << line << endl;
      }
    }
  }

  file.close();

  // just for quick verification
  string floorplan_filename
      = string("./") + string(report_directory) + "/final_floorplan.txt";
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

  // Write back to odb
  auto block = db->getChip()->getBlock();
  bool create_cluster_regions = false; // turned off till validation of flow through gpl and dpl/dpo
  for (const auto cluster : clusters) {
    if (cluster->GetNumMacro() > 0) {
      float cluster_lx = cluster->GetX();
      float cluster_ly = cluster->GetY();
      vector<Macro> macros = cluster->GetMacros();
      for (const auto& macro : macros) {
        float lx = outline_lx + cluster_lx + macro.GetX() + halo_width;
        float ly = outline_ly + cluster_ly + macro.GetY() + halo_width;
        odb::dbOrientType orientation(macro.GetOrientation().c_str());
        lx = round(lx / pitch_x) * pitch_x;
        ly = round(ly / pitch_y) * pitch_y;

        auto inst = block->findInst(macro.GetName().c_str());
        inst->setOrient(orientation);
        inst->setLocation(round(lx * dbu), round(ly * dbu));
        inst->setPlacementStatus(odb::dbPlacementStatus::FIRM);
      }
    } else if (create_cluster_regions) {
      auto region = odb::dbRegion::create(block, cluster->GetName().c_str());
      odb::dbBox::create(region,
                         cluster->GetX() * dbu,
                         cluster->GetY() * dbu,
                         (cluster->GetX() + cluster->GetWidth()) * dbu,
                         (cluster->GetY() + cluster->GetHeight()) * dbu);
      auto group = block->findGroup(cluster->GetName().c_str());
      if (group) {
        region->addGroup(group);
      }
    }
  }

  logger->report("Finish RTL-MP");

  return true;
}

void MacroPlacer2::init(dbDatabase* db, Logger* logger)
{
  db_ = db;
  logger_ = logger;
}

bool MacroPlacer2::place(const char* config_file,
                         const char* report_directory,
                         const float area_wt,
                         const float wirelength_wt,
                         const float outline_wt,
                         const float boundary_wt,
                         const float macro_blockage_wt,
                         const float location_wt,
                         const float notch_wt,
                         const float dead_space,
                         const float macro_halo,
                         const char* report_file,
                         const char* macro_blockage_file,
                         const char* prefer_location_file)
{
  return rtl_macro_placer(config_file,
                          logger_,
                          db_,
                          report_directory,
                          area_wt,
                          wirelength_wt,
                          outline_wt,
                          boundary_wt,
                          macro_blockage_wt,
                          location_wt,
                          notch_wt,
                          dead_space,
                          macro_halo,
                          report_file,
                          macro_blockage_file,
                          prefer_location_file);
}

}  // namespace mpl
