// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// clang-format off
%{

#include <sstream>

#include "ord/OpenRoad.hh"
#include "graphics/Graphics.h"
#include "graphics/DplObserver.h"
#include "dpl/Opendp.h"
#include "utl/Logger.h"

%}

%include "../../Exception.i"

%import <std_vector.i>
%import "dbtypes.i"

%inline %{

namespace dpl {

void
detailed_placement_cmd(int max_displacment_x,
                       int max_displacment_y,
                       const char* report_file_name){
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->detailedPlacement(max_displacment_x, max_displacment_y, std::string(report_file_name));
}

void
report_legalization_stats()
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->reportLegalizationStats();
}

void
check_placement_cmd(bool verbose, const char* report_file_name)
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->checkPlacement(verbose, std::string(report_file_name));
}


void
set_padding_global(int left,
                   int right)
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->setPaddingGlobal(left, right);
}

void
set_padding_master(odb::dbMaster *master,
                   int left,
                   int right)
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->setPadding(master, left, right);
}

void
set_padding_inst(odb::dbInst *inst,
                 int left,
                 int right)
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->setPadding(inst, left, right);
}

void
filler_placement_cmd(const std::vector<odb::dbMaster*>& filler_masters,
                     const char* prefix,
                     bool verbose)
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->fillerPlacement(filler_masters, prefix, verbose);
}

void
remove_fillers_cmd()
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->removeFillers();
}

void
optimize_mirroring_cmd()
{
  dpl::Opendp *opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->optimizeMirroring();
}

void
set_debug_cmd(float min_displacement,
              const odb::dbInst* debug_instance,
              int jump_moves,
              bool iterative_placement)
{
  dpl::Opendp* opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->setJumpMoves(jump_moves);
  opendp->setIterativePlacement(iterative_placement);
  if (dpl::Graphics::guiActive()) {
      std::unique_ptr<DplObserver> graphics = std::make_unique<dpl::Graphics>(
          opendp, min_displacement, debug_instance);
      opendp->setDebug(graphics);
  }
}

void improve_placement_cmd(int seed,
  int max_displacement_x,
  int max_displacement_y)
{
  dpl::Opendp* opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->improvePlacement(seed, max_displacement_x, max_displacement_y);
}

void reset_global_swap_params_cmd()
{
  dpl::Opendp* opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->resetGlobalSwapParams();
}

void configure_global_swap_params_cmd(int passes,
                                      double tolerance,
                                      double tradeoff,
                                      double area_weight,
                                      double pin_weight,
                                      double user_weight,
                                      int sampling_moves,
                                      int normalization_interval,
                                      double profiling_excess,
                                      const char* budget_multipliers_str)
{
  std::vector<double> budget_multipliers;
  if (budget_multipliers_str != nullptr) {
    std::stringstream ss(budget_multipliers_str);
    double value;
    while (ss >> value) {
      budget_multipliers.push_back(value);
    }
  }

  dpl::Opendp* opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->configureGlobalSwapParams(passes,
                                    tolerance,
                                    tradeoff,
                                    area_weight,
                                    pin_weight,
                                    user_weight,
                                    sampling_moves,
                                    normalization_interval,
                                    profiling_excess,
                                    budget_multipliers);
}

void set_extra_dpl_cmd(bool enable)
{
  dpl::Opendp* opendp = ord::OpenRoad::openRoad()->getOpendp();
  opendp->setExtraDplEnabled(enable);
}

} // namespace

%} // inline
