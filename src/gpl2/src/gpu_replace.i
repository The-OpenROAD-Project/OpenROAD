%{
#include "ord/OpenRoad.hh"
#include "gpl2/GpuReplace.h"
#include "odb/db.h"

namespace ord {
OpenRoad*
getOpenRoad();

gpl2::GpuReplace*
getGpuReplace();

}

using ord::getOpenRoad;
using ord::getGpuReplace;
using gpl2::GpuReplace;

%}

%include "../../Exception.i"

%inline %{

void 
gpu_replace_reset_cmd() 
{
  GpuReplace* gpu_replace = getGpuReplace();  
  gpu_replace->reset();
}

void 
gpu_replace_initial_place_cmd()
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->doInitialPlace();
}

void 
gpu_replace_nesterov_place_cmd()
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->doNesterovPlace();
}

void
gpu_set_density_cmd(float density)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setTargetDensity(density);
}

void
gpu_set_uniform_target_density_mode_cmd(bool uniform)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setUniformTargetDensityMode(uniform);
}

void
gpu_set_initial_place_max_iter_cmd(int iter)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setInitialPlaceMaxIter(iter); 
}

void
gpu_set_initial_place_max_fanout_cmd(int fanout)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setInitialPlaceMaxFanout(fanout);
}

void
gpu_set_nesv_place_iter_cmd(int iter)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setNesterovPlaceMaxIter(iter);
}

void
gpu_set_bin_grid_cnt_cmd(int cnt_x, int cnt_y)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setBinGridCnt(cnt_x, cnt_y);
}

void
gpu_set_overflow_cmd(float overflow)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setTargetOverflow(overflow);
}

void
gpu_set_min_phi_coef_cmd(float min_phi_coef)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setMinPhiCoef(min_phi_coef);
}

void
gpu_set_max_phi_coef_cmd(float max_phi_coef) 
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setMaxPhiCoef(max_phi_coef);
}

void
gpu_set_reference_hpwl_cmd(float reference_hpwl)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setReferenceHpwl(reference_hpwl);
}

void
gpu_set_init_density_penalty_factor_cmd(float penaltyFactor)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setInitDensityPenalityFactor(penaltyFactor);
}

void
gpu_set_init_wirelength_coef_cmd(float coef)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setInitWireLengthCoef(coef);
}

void
gpu_replace_incremental_place_cmd()
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->doIncrementalPlace();
}

void gpu_set_timing_driven_mode(bool timing_driven)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setTimingDrivenMode(timing_driven);
}

void gpu_set_datapath_flag_cmd(bool datapath_flag)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setDatapathFlag(datapath_flag);
}

void gpu_set_dataflow_flag_cmd(bool dataflow_flag)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setDataflowFlag(dataflow_flag);
}

void gpu_set_dreamplace_flag_cmd(bool flag) 
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setDREAMPlaceFlag(flag);
}

void gpu_set_cluster_constraint_flag_cmd(bool cluster_constraint)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setClusterConstraintFlag(cluster_constraint);
}


void
gpu_set_routability_driven_mode(bool routability_driven)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setRoutabilityDrivenMode(routability_driven);
}

void
gpu_set_routability_check_overflow_cmd(float overflow) 
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setRoutabilityCheckOverflow(overflow);
}

void
gpu_set_routability_max_density_cmd(float density) 
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setRoutabilityMaxDensity(density);
}

void
gpu_set_routability_max_bloat_iter_cmd(int iter)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setRoutabilityMaxBloatIter(iter);
}

void
gpu_set_routability_max_inflation_iter_cmd(int iter) 
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setRoutabilityMaxInflationIter(iter);
}

void
gpu_set_routability_targpu_get_rc_metric_cmd(float rc)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setRoutabilityTargetRcMetric(rc);
}

void
gpu_set_routability_inflation_ratio_coef_cmd(float coef)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setRoutabilityInflationRatioCoef(coef);
}

void
gpu_set_routability_max_inflation_ratio_cmd(float ratio) 
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setRoutabilityMaxInflationRatio(ratio);
}

void
gpu_set_routability_rc_coefficients_cmd(float k1,
                                    float k2,
                                    float k3,
                                    float k4)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setRoutabilityRcCoefficients(k1, k2, k3, k4);
}


void
gpu_set_pad_left_cmd(int pad) 
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setPadLeft(pad);
}

void gpu_set_halo_width_cmd(float halo_width)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setHaloWidth(halo_width);
}

void gpu_set_virtual_iter_cmd(int iter)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setVirtualIter(iter);
}

void gpu_set_num_hops_cmd(int num_hops)
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setNumHops(num_hops);
}


void
gpu_set_pad_right_cmd(int pad) 
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setPadRight(pad);
}

void
gpu_set_skip_io_mode_cmd(bool mode) 
{
  GpuReplace* gpu_replace = getGpuReplace();
  gpu_replace->setSkipIoMode(mode);
}

float
gpu_get_global_placement_uniform_density_cmd() 
{
  GpuReplace* gpu_replace = getGpuReplace();
  return gpu_replace->getUniformTargetDensity();
}

void 
gpu_add_timing_net_reweight_overflow_cmd(int overflow)
{
  GpuReplace* gpu_replace = getGpuReplace();
  return gpu_replace->addTimingNetWeightOverflow(overflow);
}

void
gpu_set_timing_driven_net_weight_max_cmd(float max)
{
  GpuReplace* gpu_replace = getGpuReplace();
  return gpu_replace->setTimingNetWeightMax(max);
}


%} // inline
