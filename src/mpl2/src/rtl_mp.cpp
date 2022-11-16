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

#include "hier_rtlmp.h"
#include "object.h"
#include "utl/Logger.h"

namespace mpl {
using odb::dbDatabase;
using std::string;
using std::unordered_map;
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

void MacroPlacer2::init(sta::dbNetwork* network,
                        odb::dbDatabase* db,
                        sta::dbSta* sta,
                        utl::Logger* logger)
{
  network_ = network;
  db_ = db;
  sta_ = sta;
  logger_ = logger;
}

bool MacroPlacer2::place(const int max_num_macro,
                         const int min_num_macro,
                         const int max_num_inst,
                         const int min_num_inst,
                         const float tolerance,
                         const int max_num_level,
                         const float coarsening_ratio,
                         const int num_bundled_ios,
                         const int large_net_threshold,
                         const int signature_net_threshold,
                         const float halo_width,
                         const float fence_lx,
                         const float fence_ly,
                         const float fence_ux,
                         const float fence_uy,
                         const float area_weight,
                         const float outline_weight,
                         const float wirelength_weight,
                         const float guidance_weight,
                         const float fence_weight,
                         const float boundary_weight,
                         const float notch_weight,
                         const float pin_access_th,
                         const float target_util,
                         const float target_dead_space,
                         const float min_ar,
                         const int snap_layer,
                         const char* report_directory)
{
  HierRTLMP* rtlmp_engine_ = new HierRTLMP(network_, db_, sta_, logger_);

  logger_->report("Hier_RTLMP report dir: {}", report_directory);

  rtlmp_engine_->setTopLevelClusterSize(
      max_num_macro, min_num_macro, max_num_inst, min_num_inst);
  rtlmp_engine_->setClusterSizeTolerance(tolerance);
  rtlmp_engine_->setMaxNumLevel(max_num_level);
  rtlmp_engine_->setClusterSizeRatioPerLevel(coarsening_ratio);
  rtlmp_engine_->setNumBundledIOsPerBoundary(num_bundled_ios);
  rtlmp_engine_->setLargeNetThreshold(large_net_threshold);
  rtlmp_engine_->setSignatureNetThreshold(signature_net_threshold);
  rtlmp_engine_->setHaloWidth(halo_width);
  rtlmp_engine_->setGlobalFence(fence_lx, fence_ly, fence_ux, fence_uy);
  rtlmp_engine_->setAreaWeight(area_weight);
  rtlmp_engine_->setOutlineWeight(outline_weight);
  rtlmp_engine_->setWirelengthWeight(wirelength_weight);
  rtlmp_engine_->setGuidanceWeight(guidance_weight);
  rtlmp_engine_->setFenceWeight(fence_weight);
  rtlmp_engine_->setBoundaryWeight(boundary_weight);
  rtlmp_engine_->setNotchWeight(notch_weight);
  rtlmp_engine_->setPinAccessThreshold(pin_access_th);
  rtlmp_engine_->setTargetUtil(target_util);
  rtlmp_engine_->setTargetDeadSpace(target_dead_space);
  rtlmp_engine_->setMinAR(min_ar);
  rtlmp_engine_->setSnapLayer(snap_layer);
  rtlmp_engine_->setReportDirectory(report_directory);

  rtlmp_engine_->hierRTLMacroPlacer();

  return true;
}

}  // namespace mpl
