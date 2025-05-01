%{
#include "ord/OpenRoad.hh"
#include "gpl2/DgReplace.h"
#include "odb/db.h"

namespace ord {
OpenRoad*
getOpenRoad();

gpl2::DgReplace*
getDgReplace();

}

using ord::getOpenRoad;
using ord::getDgReplace;
using gpl2::DgReplace;

%}

%include "../../Exception.i"

%inline %{

void 
dg_replace_reset_cmd() 
{
  DgReplace* dg_replace = getDgReplace();  
  dg_replace->reset();
}

void 
dg_replace_initial_place_cmd()
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->doInitialPlace();
}

void 
dg_replace_nesterov_place_cmd()
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->doNesterovPlace();
}

void
dg_set_density_cmd(float density)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setTargetDensity(density);
}

void
dg_set_uniform_target_density_mode_cmd(bool uniform)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setUniformTargetDensityMode(uniform);
}

void
dg_set_initial_place_max_iter_cmd(int iter)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setInitialPlaceMaxIter(iter); 
}

void
dg_set_initial_place_max_fanout_cmd(int fanout)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setInitialPlaceMaxFanout(fanout);
}

void
dg_set_nesv_place_iter_cmd(int iter)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setNesterovPlaceMaxIter(iter);
}

void
dg_set_bin_grid_cnt_cmd(int cnt_x, int cnt_y)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setBinGridCnt(cnt_x, cnt_y);
}

void
dg_set_overflow_cmd(float overflow)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setTargetOverflow(overflow);
}

void
dg_set_min_phi_coef_cmd(float min_phi_coef)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setMinPhiCoef(min_phi_coef);
}

void
dg_set_max_phi_coef_cmd(float max_phi_coef) 
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setMaxPhiCoef(max_phi_coef);
}

void
dg_set_reference_hpwl_cmd(float reference_hpwl)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setReferenceHpwl(reference_hpwl);
}

void
dg_set_init_density_penalty_factor_cmd(float penaltyFactor)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setInitDensityPenalityFactor(penaltyFactor);
}

void
dg_set_init_wirelength_coef_cmd(float coef)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setInitWireLengthCoef(coef);
}

void dg_set_timing_driven_mode(bool timing_driven)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setTimingDrivenMode(timing_driven);
}

void dg_set_datapath_flag_cmd(bool datapath_flag)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setDatapathFlag(datapath_flag);
}

void dg_set_dataflow_flag_cmd(bool dataflow_flag)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setDataflowFlag(dataflow_flag);
}

void dg_set_cluster_constraint_flag_cmd(bool cluster_constraint)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setClusterConstraintFlag(cluster_constraint);
}


void
dg_set_routability_driven_mode(bool routability_driven)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setRoutabilityDrivenMode(routability_driven);
}

void
dg_set_routability_check_overflow_cmd(float overflow) 
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setRoutabilityCheckOverflow(overflow);
}

void
dg_set_routability_max_density_cmd(float density) 
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setRoutabilityMaxDensity(density);
}

void
dg_set_routability_max_bloat_iter_cmd(int iter)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setRoutabilityMaxBloatIter(iter);
}

void
dg_set_routability_max_inflation_iter_cmd(int iter) 
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setRoutabilityMaxInflationIter(iter);
}

void
dg_set_routability_tardg_get_rc_metric_cmd(float rc)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setRoutabilityTargetRcMetric(rc);
}

void
dg_set_routability_inflation_ratio_coef_cmd(float coef)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setRoutabilityInflationRatioCoef(coef);
}

void
dg_set_routability_max_inflation_ratio_cmd(float ratio) 
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setRoutabilityMaxInflationRatio(ratio);
}

void
dg_set_routability_rc_coefficients_cmd(float k1,
                                    float k2,
                                    float k3,
                                    float k4)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setRoutabilityRcCoefficients(k1, k2, k3, k4);
}


void
dg_set_pad_left_cmd(int pad) 
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setPadLeft(pad);
}

void dg_set_halo_width_cmd(float halo_width)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setHaloWidth(halo_width);
}

void dg_set_virtual_iter_cmd(int iter)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setVirtualIter(iter);
}

void dg_set_num_hops_cmd(int num_hops)
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setNumHops(num_hops);
}


void
dg_set_pad_right_cmd(int pad) 
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setPadRight(pad);
}

void
dg_set_skip_io_mode_cmd(bool mode) 
{
  DgReplace* dg_replace = getDgReplace();
  dg_replace->setSkipIoMode(mode);
}

float
dg_get_global_placement_uniform_density_cmd() 
{
  DgReplace* dg_replace = getDgReplace();
  return dg_replace->getUniformTargetDensity();
}

void 
dg_add_timing_net_reweight_overflow_cmd(int overflow)
{
  DgReplace* dg_replace = getDgReplace();
  return dg_replace->addTimingNetWeightOverflow(overflow);
}

void
dg_set_timing_driven_net_weight_max_cmd(float max)
{
  DgReplace* dg_replace = getDgReplace();
  return dg_replace->setTimingNetWeightMax(max);
}


%} // inline
