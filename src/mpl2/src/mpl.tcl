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
                                          -area_weight area_weight \
                                          -outline_weight outline_weight \
                                          -wirelength_weight wirelength_weight \
                                          -guidance_weight guidance_weight \
                                          -fence_weight fence_weight \
                                          -boundary_weight boundary_weight \
                                          -notch_weight notch_weight \
                                          -pin_access_th pin_access_th \
                                          -target_util   target_util \
                                          -target_dead_space target_dead_space \
                                          -min_ar  min_ar \
                                          -snap_layer snap_layer \
                                        }
proc rtl_macro_placer { args } {
<<<<<<< HEAD
    sta::parse_key_args "rtl_macro_placer" args keys { 
        -max_num_macro  -min_num_macro -max_num_inst  -min_num_inst  -tolerance   \
        -max_num_level  -coarsening_ratio  -num_bundled_ios  -large_net_threshold \
        -signature_net_threshold -halo_width \
        -fence_lx   -fence_ly  -fence_ux   -fence_uy  \
        -area_weight  -outline_weight -wirelength_weight -guidance_weight -fence_weight \
        -boundary_weight -notch_weight \
        -pin_access_th -target_util \
        -target_dead_space -min_ar -snap_layer \
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
    set area_weight $keys(-area_weight)
    set wirelength_weight $keys(-wirelength_weight)
    set outline_weight $keys(-outline_weight)
    set guidance_weight $keys(-guidance_weight)
    set fence_weight  $keys(-fence_weight)
    set boundary_weight $keys(-boundary_weight)
    set notch_weight  $keys(-notch_weight)
    set pin_access_th $keys(-pin_access_th)
    set target_util   $keys(-target_util)
    set target_dead_space $keys(-target_dead_space)
    set min_ar $keys(-min_ar)
    set snap_layer $keys(-snap_layer)
        
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
                                      $area_weight $outline_weight $wirelength_weight \
                                      $guidance_weight $fence_weight $boundary_weight \
                                      $notch_weight \
                                      $pin_access_th \
                                      $target_util \
                                      $target_dead_space \
                                      $min_ar \
                                      $snap_layer \
                                      ]} {
=======
    sta::parse_key_args "rtl_macro_placer" args keys { -config_file -report_directory
       -area_weight -wirelength_weight -outline_weight
       -boundary_weight -macro_blockage_weight -location_weight -notch_weight -dead_space
       -macro_halo -report_file -macro_blockage_file -prefer_location_file } flag {  }

    if { ![info exists keys(-report_directory)] } {
        utl::error MPL 2 "Missing mandatory -report_directory for RTLMP"
    }

#
#  Default values for the weights
#
    set area_wt 0.01
    set wirelength_wt 88.7
    set outline_wt 74.71
    set boundary_wt 225.0
    set macro_blockage_wt 50.0
    set location_wt 100.0
    set notch_wt 212.0
    set dead_space 0.05

    set macro_halo 10.0
    set report_directory "rtl_mp"
    set report_file "partition.txt"
    set config_file "" 
    set macro_blockage_file "macro_blockage.txt"
    set prefer_location_file "location.txt"

    if { [info exists keys(-report_file)] } {
        set report_file $keys(-report_file)
    }

    if { [info exists keys(-area_weight)] } {
        set area_wt $keys(-area_weight)
    }

    if { [info exists keys(-wirelength_weight)] } {
        set wirelength_wt $keys(-wirelength_weight)
    }

    if { [info exists keys(-outline_weight)] } {
        set outline_wt $keys(-outline_weight)
    }

    if { [info exists keys(-boundary_weight)] } {
        set boundary_wt $keys(-boundary_weight)
    }

    if { [info exists keys(-macro_blockage_weight)] } {
        set macro_blockage_wt $keys(-macro_blockage_weight)
    }

    if { [info exists keys(-location_weight)] } {
        set location_wt $keys(-location_weight)
    }

    if { [info exists keys(-notch_weight)] } {
        set notch_wt $keys(-notch_weight)
    }

    if { [info exists keys(-dead_space)] } {
        set dead_space $keys(-dead_space)
    }

    if { [info exists keys(-macro_halo)] } {
        set macro_halo $keys(-macro_halo)
    }

    if { [info exists keys(-report_directory)] } {
        set report_directory $keys(-report_directory)
    }

    if { [info exists keys(-config_file)] } {
        set config_file $keys(-config_file)
    }

    if { [info exists keys(-macro_blockage_file)] } {
        set macro_blockage_file $keys(-macro_blockage_file)
    }

    if { [info exists keys(-prefer_location_file)] } {
        set prefer_location_file $keys(-prefer_location_file)
    }

    if {![mpl2::rtl_macro_placer_cmd $config_file $report_directory $area_wt $wirelength_wt \
                    $outline_wt $boundary_wt $macro_blockage_wt $location_wt $notch_wt $dead_space $macro_halo\
                    $report_file $macro_blockage_file $prefer_location_file]} {
>>>>>>> origin/master
        return false
    }

    return true
}
