// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

%include "../../options.i"

%{
#include "ord/OpenRoad.hh"
#include "gpl/Replace.h"
#include "odb/db.h"
#include "utl/Logger.h"
#include "utl/cpu_topology.h"

namespace ord {
OpenRoad*
getOpenRoad();

gpl::Replace*
getReplace();
}

// The legacy switches of global_placement, mapped onto PlaceOptions.  Later
// entries override earlier ones, so -skip_io comes last.
static const utl::LegacyArgs kGlobalPlacementArgs = {
    {"-timing_driven", {"timing.enabled"}, "true", {}},
    {"-timing_driven_repair_timing", {"timing.repair_timing"}, "true", {}},
    {"-routability_driven", {"routability.enabled"}, "true", {}},
    {"-virtual_cts", {"virtual_cts.enabled"}, "true", {}},
    {"-routability_use_grt", {"routability.use_rudy"}, "false", {}},
    {"-disable_revert_if_diverge", {"nesterov.revert_if_diverge"}, "false", {}},
    {"-disable_pin_density_adjust", {"pin_density_adjust"}, "false", {}},
    {"-enable_routing_congestion", {"routing_congestion"}, "true", {}},
    {"-force_center_initial_place", {"initial.force_center"}, "true", {}},
    {"-skip_initial_place", {"initial.max_iter"}, "0", {}},
    {"-initial_place_max_iter", {"initial.max_iter"}, "", {}},
    {"-initial_place_max_fanout", {"initial.max_fanout"}, "", {}},
    {"-random_seed", {"initial.perturbation_seed"}, "", {}},
    {"-perturb_dist", {"initial.perturbation_dist"}, "", {}},
    {"-overflow", {"nesterov.target_overflow"}, "", {}},
    {"-min_phi_coef", {"nesterov.min_phi_coef"}, "", {}},
    {"-max_phi_coef", {"nesterov.max_phi_coef"}, "", {}},
    {"-init_density_penalty", {"nesterov.init_density_penalty"}, "", {}},
    {"-init_wirelength_coef", {"nesterov.init_wirelength_coef"}, "", {}},
    {"-reference_hpwl", {"nesterov.reference_hpwl"}, "", {}},
    {"-routability_check_overflow", {"routability.check_overflow"}, "", {}},
    {"-routability_snapshot_overflow", {"routability.snapshot_overflow"}, "", {}},
    {"-routability_max_density", {"routability.max_density"}, "", {}},
    {"-routability_target_rc_metric", {"routability.target_rc_metric"}, "", {}},
    {"-routability_inflation_ratio_coef", {"routability.inflation_ratio_coef"}, "", {}},
    {"-routability_max_inflation_ratio", {"routability.max_inflation_ratio"}, "", {}},
    {"-routability_min_congestion_for_inflation",
     {"routability.min_congestion_for_inflation"}, "", {}},
    {"-routability_max_inflation_total", {"routability.max_inflation_total"}, "", {}},
    {"-routability_net_weight_max", {"routability.net_weight_max"}, "", {}},
    {"-routability_congested_nets_percentage",
     {"routability.congested_nets_percentage"}, "", {}},
    {"-routability_rc_coefficients", {"routability.rc_coefficients"}, "", {}},
    {"-timing_driven_net_reweight_overflow", {"timing.reweight_overflows"}, "", {}},
    {"-timing_driven_net_weight_max", {"timing.net_weight_max"}, "", {}},
    {"-timing_driven_nets_percentage", {"timing.nets_percentage"}, "", {}},
    {"-timing_driven_repair_tns_end_percent", {"timing.repair_tns_end_percent"}, "", {}},
    {"-keep_resize_below_overflow", {"timing.keep_resize_below_overflow"}, "", {}},
    {"-virtual_cts_max_skew_fraction", {"virtual_cts.max_skew_fraction"}, "", {}},
    {"-pad_left", {"pad_left"}, "", {}},
    {"-pad_right", {"pad_right"}, "", {}},
    {"-bin_grid_count", {"bin_grid_count_x", "bin_grid_count_y"}, "", {}},
    {"-place_ios", {"place_ios"}, "true", {}},
    {"-density", {}, "", [](boost::json::object& o, const std::string& value) {
       if (value == "uniform") {
         o["uniform_density"] = true;
       } else {
         o["density"] = value;
       }
     }},
    {"-skip_io", {}, "", [](boost::json::object& o, const std::string&) {
       // PlaceOptions::skipIo()
       o["skip_io"] = true;
       o["initial.max_iter"] = 0;
       o["timing.enabled"] = false;
       o["routability.enabled"] = false;
     }},
};

static gpl::PlaceOptions getOptions(
    Tcl_Interp* interp,
    Tcl_Obj* options,
    const std::map<std::string, std::string>& keys,
    const std::map<std::string, std::string>& flags)
{
  return optionsFromTcl<gpl::PlaceOptions>(
      interp, options, keys, flags, kGlobalPlacementArgs);
}

using ord::getOpenRoad;
using ord::getReplace;
using gpl::Replace;

%}

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

// Global placement runs one thread per core, not per hardware thread. Its
// parallel regions are memory-bound loops over gcells and nets that end in
// an OpenMP barrier; on an SMT machine the sibling spinning in the barrier
// slows the sibling still working, and -threads set to the hardware thread
// count came out slower than the core count on every design measured
// (coralnpu 39 s vs 31 s, bp_quad 334 s vs 293 s), at 2.5x the CPU time.
// A -threads value at or below the core count is honored as given.
static int placementThreads()
{
  auto* openroad = ord::OpenRoad::openRoad();
  const int threads = openroad->getThreadCount();
  const int cores = utl::physicalCoreCount();
  if (threads <= cores) {
    return threads;
  }
  static bool reported = false;
  if (!reported) {
    openroad->getLogger()->info(utl::GPL,
                                190,
                                "Using {} of {} threads for placement: one "
                                "per core, SMT siblings idle.",
                                cores,
                                threads);
    reported = true;
  }
  return cores;
}

void 
replace_initial_place_cmd(Tcl_Obj* options_dict,
                          const std::map<std::string, std::string>& keys,
                          const std::map<std::string, std::string>& flags,
                          Tcl_Interp* interp)
{
  gpl::PlaceOptions options = getOptions(interp, options_dict, keys, flags);

  Replace* replace = getReplace();
  int threads = placementThreads();
  replace->doInitialPlace(threads, options);
}

void 
replace_nesterov_place_cmd(Tcl_Obj* options_dict,
                           const std::map<std::string, std::string>& keys,
                           const std::map<std::string, std::string>& flags,
                           Tcl_Interp* interp)
{
  gpl::PlaceOptions options = getOptions(interp, options_dict, keys, flags);
  Replace* replace = getReplace();
  int threads = placementThreads();
  replace->doNesterovPlace(threads, options);
}


void
replace_run_mbff_cmd(int max_sz, float alpha, float beta, int num_paths,
                     float clock_power_weight)
{
  Replace* replace = getReplace();
  int threads = placementThreads();
  replace->runMBFF(max_sz, alpha, beta, threads, num_paths, clock_power_weight);
}


void
replace_incremental_place_cmd(Tcl_Obj* options_dict,
                              const std::map<std::string, std::string>& keys,
                              const std::map<std::string, std::string>& flags,
                              Tcl_Interp* interp)
{
  gpl::PlaceOptions options = getOptions(interp, options_dict, keys, flags);
  Replace* replace = getReplace();
  int threads = placementThreads();
  replace->doIncrementalPlace(threads, options);
}


float
get_global_placement_uniform_density_cmd(
  const std::map<std::string, std::string>& keys,
  const std::map<std::string, std::string>& flags,
  Tcl_Interp* interp)
{
  gpl::PlaceOptions options = getOptions(interp, nullptr, keys, flags);
  Replace* replace = getReplace();
  int threads = placementThreads();
  return replace->getUniformTargetDensity(options, threads);
}

void
set_debug_cmd(int pause_iterations,
              int update_iterations,
              bool draw_bins,
              bool initial,
              const char* inst_name,
              const char* pin_name,
              int start_iter,
              int start_rudy,
              int rudy_stride,
              bool generate_images,
              const char* images_path)
{
  Replace* replace = getReplace();
  auto block = ord::OpenRoad::openRoad()->getDb()->getChip()->getBlock();
  odb::dbInst* inst = nullptr;
  if (inst_name) {
    inst = block->findInst(inst_name);
  }
  odb::dbBTerm* pin = nullptr;
  if (pin_name) {
    pin = block->findBTerm(pin_name);
  }

  std::string resolved_path = (images_path && *images_path)
                                  ? images_path
                                  : "REPORTS_DIR";

  replace->setDebug(pause_iterations, update_iterations, draw_bins,
                    initial, inst, pin, start_iter, start_rudy, rudy_stride,
                    generate_images, resolved_path);
}

float
estimate_target_density_cmd(
  const std::map<std::string, std::string>& keys,
  const std::map<std::string, std::string>& flags,
  Tcl_Interp* interp)
{
  gpl::PlaceOptions options = getOptions(interp, nullptr, keys, flags);
  Replace* replace = getReplace();
  int threads = ord::OpenRoad::openRoad()->getThreadCount();

  return replace->estimateTargetDensity(options, threads);
}

%} // inline
