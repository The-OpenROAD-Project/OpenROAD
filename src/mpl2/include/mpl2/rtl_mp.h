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

namespace odb {
class dbDatabase;
}

namespace sta {
class dbNetwork;
class dbSta;
}  // namespace sta

namespace utl {
class Logger;
}

namespace par {
class PartitionMgr;
}

namespace mpl2 {

class HierRTLMP;

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

  bool place(const int max_num_macro,
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
             const float macro_blockage_weight,
             const float pin_access_th,
             const float target_util,
             const float target_dead_space,
             const float min_ar,
             const int snap_layer,
             const char* report_directory);

  void setDebug();

 private:
  std::unique_ptr<HierRTLMP> hier_rtlmp_;
};

}  // namespace mpl2
