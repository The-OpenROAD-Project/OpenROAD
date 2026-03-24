// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

%{
#include "ord/OpenRoad.hh"
#include "gpl/Replace.h"
#include "odb/db.h"
#include "utl/options.h"

namespace ord {
OpenRoad*
getOpenRoad();

gpl::Replace*
getReplace();
}

static gpl::PlaceOptions getOptions(
    const std::map<std::string, std::string>& keys,
    const std::map<std::string, std::string>& flags)
{
  using utl::checkFlag;
  using utl::checkKey;

  gpl::PlaceOptions options;
  checkFlag(flags, "-timing_driven", options.timingDrivenMode);
  checkFlag(flags, "-routability_driven", options.routabilityDrivenMode);
  checkFlag(flags, "-routability_use_grt", options.routabilityUseRudy, false);
  checkFlag(
      flags, "-disable_revert_if_diverge", options.disableRevertIfDiverge);
  checkFlag(
      flags, "-disable_pin_density_adjust", options.disablePinDensityAdjust);
  checkFlag(
      flags, "-enable_routing_congestion", options.enable_routing_congestion);
  checkFlag(flags, "-force_center_initial_place", options.forceCenterInitialPlace);
  checkFlag(flags, "-skip_initial_place", [&](bool) {
    options.initialPlaceMaxIter = 0;
  });
  checkKey(keys, "-initial_place_max_iter", options.initialPlaceMaxIter);
  checkKey(keys, "-initial_place_max_fanout", options.initialPlaceMaxFanout);
  checkKey(
      keys, "-routability_check_overflow", options.routabilityCheckOverflow);
  checkKey(
      keys,
      "-routability_snapshot_overflow",
      options.routabilitySnapshotOverflow);
  checkKey(keys, "-routability_max_density", options.routabilityMaxDensity);
  checkKey(
      keys, "-routability_target_rc_metric", options.routabilityTargetRcMetric);
  checkKey(keys,
           "-routability_inflation_ratio_coef",
           options.routabilityInflationRatioCoef);
  checkKey(keys,
           "-routability_max_inflation_ratio",
           options.routabilityMaxInflationRatio);
  checkKey(keys, "-pad_left", options.padLeft);
  checkKey(keys, "-pad_right", options.padRight);
  checkKey(keys,
           "-routability_rc_coefficients",
           std::tie(options.routabilityRcK1,
                    options.routabilityRcK2,
                    options.routabilityRcK3,
                    options.routabilityRcK4));
  checkKey(keys,
           "-timing_driven_net_reweight_overflow",
           options.timingNetWeightOverflows);
  checkKey(keys, "-overflow", options.overflow);
  checkKey(keys, "-timing_driven_net_weight_max", options.timingNetWeightMax);
  checkKey(
      keys, "-keep_resize_below_overflow", options.keepResizeBelowOverflow);
  checkKey(keys, "-min_phi_coef", options.minPhiCoef);
  checkKey(keys, "-max_phi_coef", options.maxPhiCoef);
  checkKey(keys, "-init_density_penalty", options.initDensityPenaltyFactor);
  checkKey(keys, "-init_wirelength_coef", options.initWireLengthCoef);
  checkKey(keys, "-reference_hpwl", options.referenceHpwl);

  if (auto it = keys.find("-density"); it != keys.end()) {
    if (it->second == "uniform") {
      options.uniformTargetDensityMode = true;
    } else {
      options.density = std::stof(it->second);
    }
  }
  if (auto it = keys.find("-bin_grid_count"); it != keys.end()) {
    options.binGridCntX = std::stoi(it->second);
    options.binGridCntY = options.binGridCntX;
  }
  checkFlag(flags, "-skip_io", [&](bool) { options.skipIo(); });
  return options;
}

using ord::getOpenRoad;
using ord::getReplace;
using gpl::Replace;

%}

%include "../../options.i"
%import <std_vector.i>
%import "dbtypes.i"
%import "dbenums.i"
%include "../../Exception.i"

%inline %{

void
placement_cluster_cmd(const std::vector<odb::dbInst*>& cluster)
{
  Replace* replace = getReplace();
  replace->addPlacementCluster(cluster);
}

void 
replace_reset_cmd() 
{
  Replace* replace = getReplace();  
  replace->reset();
}

void 
replace_initial_place_cmd(const std::map<std::string, std::string>& keys,
                          const std::map<std::string, std::string>& flags)
{
  gpl::PlaceOptions options = getOptions(keys, flags);

  Replace* replace = getReplace();
  int threads = ord::OpenRoad::openRoad()->getThreadCount();
  replace->doInitialPlace(threads, options);
}

void 
replace_nesterov_place_cmd(const std::map<std::string, std::string>& keys,
                           const std::map<std::string, std::string>& flags)
{
  gpl::PlaceOptions options = getOptions(keys, flags);
  Replace* replace = getReplace();
  int threads = ord::OpenRoad::openRoad()->getThreadCount();
  replace->doNesterovPlace(threads, options);
}


void
replace_run_mbff_cmd(int max_sz, float alpha, float beta, int num_paths) 
{
  Replace* replace = getReplace();
  int threads = ord::OpenRoad::openRoad()->getThreadCount();
  replace->runMBFF(max_sz, alpha, beta, threads, num_paths);   
}


void
replace_incremental_place_cmd(const std::map<std::string, std::string>& keys,
                              const std::map<std::string, std::string>& flags)
{
  gpl::PlaceOptions options = getOptions(keys, flags);
  Replace* replace = getReplace();
  int threads = ord::OpenRoad::openRoad()->getThreadCount();
  replace->doIncrementalPlace(threads, options);
}


float
get_global_placement_uniform_density_cmd(
  const std::map<std::string, std::string>& keys,
  const std::map<std::string, std::string>& flags)
{
  gpl::PlaceOptions options = getOptions(keys, flags);
  Replace* replace = getReplace();
  int threads = ord::OpenRoad::openRoad()->getThreadCount();
  return replace->getUniformTargetDensity(options, threads);
}

void
set_debug_cmd(int pause_iterations,
              int update_iterations,
              bool draw_bins,
              bool initial,
              const char* inst_name,
              int start_iter,
              int start_rudy,
              int rudy_stride,
              bool generate_images,
              const char* images_path)
{
  Replace* replace = getReplace();
  odb::dbInst* inst = nullptr;
  if (inst_name) {
    auto block = ord::OpenRoad::openRoad()->getDb()->getChip()->getBlock();
    inst = block->findInst(inst_name);
  }

  std::string resolved_path = (images_path && *images_path)
                                  ? images_path
                                  : "REPORTS_DIR";

  replace->setDebug(pause_iterations, update_iterations, draw_bins,
                    initial, inst, start_iter, start_rudy, rudy_stride,
                    generate_images, resolved_path);
}

%} // inline
