%{
#include "ord/OpenRoad.hh"
#include "gpl/Replace.h"
#include "odb/db.h"

namespace ord {
OpenRoad*
getOpenRoad();

gpl::Replace*
getReplace();

}

using ord::getOpenRoad;
using ord::getReplace;
using gpl::Replace;

static gpl::ReplaceOptions options;
%}

%include "../../Exception.i"

%inline %{

void 
replace_reset_cmd() 
{
  Replace* replace = getReplace();  
  replace->reset();
  options = gpl::ReplaceOptions();
}

void
set_density_cmd(float density)
{
  options.setTargetDensity(density);
}

void
set_uniform_target_density_mode_cmd(bool uniform)
{
  options.setUniformTargetDensityMode(uniform);
}

void
set_initial_place_max_iter_cmd(int iter)
{
  options.setInitialPlaceMaxIter(iter); 
}

void
set_initial_place_max_fanout_cmd(int fanout)
{
  options.setInitialPlaceMaxFanout(fanout);
}

void
set_nesv_place_iter_cmd(int iter)
{
  options.setNesterovPlaceMaxIter(iter);
}

void
set_bin_grid_cnt_x_cmd(int cnt_x)
{
  options.setBinGridCntX(cnt_x); 
}

void
set_bin_grid_cnt_y_cmd(int cnt_y)
{
  options.setBinGridCntY(cnt_y);
}

void
set_overflow_cmd(float overflow)
{
  options.setTargetOverflow(overflow);
}

void
set_min_phi_coef_cmd(float min_phi_coef)
{
  options.setMinPhiCoef(min_phi_coef);
}

void
set_max_phi_coef_cmd(float max_phi_coef) 
{
  options.setMaxPhiCoef(max_phi_coef);
}

void
set_reference_hpwl_cmd(float reference_hpwl)
{
  options.setReferenceHpwl(reference_hpwl);
}

void
set_init_density_penalty_factor_cmd(float penaltyFactor)
{
  options.setInitDensityPenalityFactor(penaltyFactor);
}

void
set_init_wirelength_coef_cmd(float coef)
{
  options.setInitWireLengthCoef(coef);
}

void
replace_place_cmd(bool incremental, bool skip_nesterov_place)
{
  options.setIncremental(incremental);
  options.setDoNesterovPlace(!skip_nesterov_place);

  Replace* replace = getReplace();
  replace->place(options);
}


void
set_force_cpu(bool force_cpu)
{
  options.setForceCpu(force_cpu);
}

void set_timing_driven_mode(bool timing_driven)
{
  options.setTimingDrivenMode(timing_driven);
}


void
set_routability_driven_mode(bool routability_driven)
{
  options.setRoutabilityDrivenMode(routability_driven);
}

void
set_routability_check_overflow_cmd(float overflow) 
{
  options.setRoutabilityCheckOverflow(overflow);
}

void
set_routability_max_density_cmd(float density) 
{
  options.setRoutabilityMaxDensity(density);
}

void
set_routability_max_bloat_iter_cmd(int iter)
{
  options.setRoutabilityMaxBloatIter(iter);
}

void
set_routability_max_inflation_iter_cmd(int iter) 
{
  options.setRoutabilityMaxInflationIter(iter);
}

void
set_routability_target_rc_metric_cmd(float rc)
{
  options.setRoutabilityTargetRcMetric(rc);
}

void
set_routability_inflation_ratio_coef_cmd(float coef)
{
  options.setRoutabilityInflationRatioCoef(coef);
}

void
set_routability_max_inflation_ratio_cmd(float ratio) 
{
  options.setRoutabilityMaxInflationRatio(ratio);
}

void
set_routability_rc_coefficients_cmd(float k1,
                                    float k2,
                                    float k3,
                                    float k4)
{
  options.setRoutabilityRcCoefficients(k1, k2, k3, k4);
}


void
set_pad_left_cmd(int pad) 
{
  options.setPadLeft(pad);
}

void
set_pad_right_cmd(int pad) 
{
  options.setPadRight(pad);
}

void
set_skip_io_mode_cmd(bool mode) 
{
  options.setSkipIoMode(mode);
}

float
get_global_placement_uniform_density_cmd() 
{
  Replace* replace = getReplace();
  return replace->getUniformTargetDensity(options);
}

void 
add_timing_net_reweight_overflow_cmd(int overflow)
{
  options.addTimingNetWeightOverflow(overflow);
}

void
set_timing_driven_net_weight_max_cmd(float max)
{
  options.setTimingNetWeightMax(max);
}

void
set_debug_cmd(int pause_iterations,
              int update_iterations,
              bool draw_bins,
              bool initial,
              const char* inst_name)
{
  odb::dbInst* inst = nullptr;
  if (inst_name) {
    auto block = ord::OpenRoad::openRoad()->getDb()->getChip()->getBlock();
    inst = block->findInst(inst_name);
  }
  options.setDebug(pause_iterations, update_iterations, draw_bins,
                   initial, inst);
}

%} // inline
