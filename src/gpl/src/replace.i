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

// The switches of global_placement are generated from PlaceOptions and
// parsed by optionsFromArgs; the Tcl proc keeps only the switches that are
// about how the command runs (-incremental, -skip_nesterov_place).
void 
replace_initial_place_cmd(Tcl_Obj* args,
                          bool skip_nesterov_place,
                          Tcl_Interp* interp)
{
  gpl::PlaceOptions options
      = optionsFromArgs<gpl::PlaceOptions>(interp, args, "global_placement");
  if (options.place_ios && skip_nesterov_place) {
    getOpenRoad()->getLogger()->error(
        utl::GPL,
        182,
        "-place_ios cannot be used with -skip_nesterov_place placement.");
  }

  Replace* replace = getReplace();
  int threads = placementThreads();
  replace->doInitialPlace(threads, options);
}

void 
replace_nesterov_place_cmd(Tcl_Obj* args, Tcl_Interp* interp)
{
  gpl::PlaceOptions options
      = optionsFromArgs<gpl::PlaceOptions>(interp, args, "global_placement");
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
replace_incremental_place_cmd(Tcl_Obj* args, Tcl_Interp* interp)
{
  gpl::PlaceOptions options
      = optionsFromArgs<gpl::PlaceOptions>(interp, args, "global_placement");
  if (options.place_ios) {
    getOpenRoad()->getLogger()->error(
        utl::GPL, 170, "-place_ios cannot be used with -incremental placement.");
  }
  Replace* replace = getReplace();
  int threads = placementThreads();
  replace->doIncrementalPlace(threads, options);
}


float
get_global_placement_uniform_density_cmd(Tcl_Obj* args, Tcl_Interp* interp)
{
  gpl::PlaceOptions options
      = optionsFromArgs<gpl::PlaceOptions>(interp, args, "global_placement");
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
estimate_target_density_cmd(Tcl_Obj* args, Tcl_Interp* interp)
{
  gpl::PlaceOptions options
      = optionsFromArgs<gpl::PlaceOptions>(interp, args, "global_placement");
  Replace* replace = getReplace();
  int threads = ord::OpenRoad::openRoad()->getThreadCount();

  return replace->estimateTargetDensity(options, threads);
}

%} // inline
