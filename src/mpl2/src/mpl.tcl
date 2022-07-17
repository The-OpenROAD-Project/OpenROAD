############################################################################
## BSD 3-Clause License
##
## Copyright (c) 2021, The Regents of the University of California
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE
## DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
## FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
## DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
## SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
## CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
## OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
## OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
############################################################################

sta::define_cmd_args "rtl_macro_placer" { -max_num_macro  max_num_macro \
                                          -min_num_macro  min_num_macro \
                                          -max_num_inst   max_num_inst  \
                                          -min_num_inst   min_num_inst  \
                                          -tolerance      tolerance     \
                                          -max_num_level  max_num_level \
                                          -coarsening_ratio coarsening_ratio \
                                          -num_bundled_ios  num_bundled_ios  \
                                          -large_net_threshold large_net_threshold \
                                          -signature_net_threshold signature_net_threshold \
                                          -halo_width halo_width \
                                          -fence_lx   fence_lx \
                                          -fence_ly   fence_ly \
                                          -fence_ux   fence_ux \
                                          -fence_uy   fence_uy \
                                        }
proc rtl_macro_placer { args } {
    sta::parse_key_args "rtl_macro_placer" args keys { 
        -max_num_macro  -min_num_macro -max_num_inst  -min_num_inst  -tolerance   \
        -max_num_level  -coarsening_ratio  -num_bundled_ios  -large_net_threshold \
        -signature_net_threshold -halo_width \
        -fence_lx   -fence_ly  -fence_ux   -fence_uy  \
    } flag {  }
     
    set max_num_macro  $keys(-max_num_macro)  
    set min_num_macro  $keys(-min_num_macro) 
    set max_num_inst   $keys(-max_num_inst)  
    set min_num_inst   $keys(-min_num_inst)  
    set tolerance      $keys(-tolerance)     
    set max_num_level  $keys(-max_num_level) 
    set coarsening_ratio $keys(-coarsening_ratio) 
    set num_bundled_ios  $keys(-num_bundled_ios)  
    set large_net_threshold  $keys(-large_net_threshold) 
    set signature_net_threshold  $keys(-signature_net_threshold) 
    set halo_width  $keys(-halo_width) 
    set fence_lx    $keys(-fence_lx) 
    set fence_ly    $keys(-fence_ly) 
    set fence_ux    $keys(-fence_ux) 
    set fence_uy    $keys(-fence_uy)
        
    if {![mpl2::rtl_macro_placer_cmd  $max_num_macro  \
                                      $min_num_macro  \
                                      $max_num_inst   \
                                      $min_num_inst   \
                                      $tolerance      \
                                      $max_num_level  \
                                      $coarsening_ratio \
                                      $num_bundled_ios  \
                                      $large_net_threshold \
                                      $signature_net_threshold \
                                      $halo_width \
                                      $fence_lx   $fence_ly  $fence_ux  $fence_uy  \
                                      ]} {
        return false
    }

    return true
}
