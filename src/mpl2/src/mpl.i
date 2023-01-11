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

%{
#include "mpl2/rtl_mp.h"

namespace ord {
// Defined in OpenRoad.i
mpl2::MacroPlacer2*
getMacroPlacer2();
}

using ord::getMacroPlacer2;
%}

%include "../../Exception.i"

%inline %{

namespace mpl2 {

bool rtl_macro_placer_cmd(const int max_num_macro,
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
                          const char* report_directory) {

  auto macro_placer = getMacroPlacer2();
  return macro_placer->place(max_num_macro,
                             min_num_macro,
                             max_num_inst,
                             min_num_inst,
                             tolerance,
                             max_num_level,
                             coarsening_ratio,
                             num_bundled_ios,
                             large_net_threshold,
                             signature_net_threshold,
                             halo_width,
                             fence_lx,
                             fence_ly,
                             fence_ux,
                             fence_uy,
                             area_weight,
                             outline_weight,
                             wirelength_weight,
                             guidance_weight,
                             fence_weight,
                             boundary_weight,
                             notch_weight,
                             macro_blockage_weight,
                             pin_access_th,
                             target_util,
                             target_dead_space,
                             min_ar,
                             snap_layer,
                             report_directory);
}

void
set_debug_cmd()
{
  auto macro_placer = getMacroPlacer2();
  macro_placer->setDebug();
}

} // namespace

%} // inline
