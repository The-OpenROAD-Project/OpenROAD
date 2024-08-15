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

#pragma once

#include <memory>

#include "utl/Logger.h"

namespace odb {
class dbDatabase;
class dbInst;
class dbOrientType;
}  // namespace odb

namespace sta {
class dbNetwork;
class dbSta;
}  // namespace sta

namespace par {
class PartitionMgr;
}

namespace mpl2 {

class HierRTLMP;
class Mpl2Observer;

class MacroPlacer2
{
 public:
  MacroPlacer2();
  ~MacroPlacer2();

  void init(sta::dbNetwork* network,
            odb::dbDatabase* db,
            sta::dbSta* sta,
            utl::Logger* logger,
            par::PartitionMgr* tritonpart);

  bool place(int num_threads,
             int max_num_macro,
             int min_num_macro,
             int max_num_inst,
             int min_num_inst,
             float tolerance,
             int max_num_level,
             float coarsening_ratio,
             int num_bundled_ios,
             int large_net_threshold,
             int signature_net_threshold,
             float halo_width,
             float halo_height,
             float fence_lx,
             float fence_ly,
             float fence_ux,
             float fence_uy,
             float area_weight,
             float outline_weight,
             float wirelength_weight,
             float guidance_weight,
             float fence_weight,
             float boundary_weight,
             float notch_weight,
             float macro_blockage_weight,
             float pin_access_th,
             float target_util,
             float target_dead_space,
             float min_ar,
             int snap_layer,
             bool bus_planning_flag,
             const char* report_directory);

  void placeMacro(odb::dbInst* inst,
                  const float& x_origin,
                  const float& y_origin,
                  const odb::dbOrientType& orientation);

  void setMacroPlacementFile(const std::string& file_name);

  void setDebug(std::unique_ptr<Mpl2Observer>& graphics);
  void setDebugShowBundledNets(bool show_bundled_nets);
  void setDebugSkipSteps(bool skip_steps);
  void setDebugOnlyFinalResult(bool only_final_result);

 private:
  std::unique_ptr<HierRTLMP> hier_rtlmp_;

  utl::Logger* logger_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
};

}  // namespace mpl2
