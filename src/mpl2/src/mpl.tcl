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

sta::define_cmd_args "rtl_macro_placer" { -report_directory report_dir \
                                         [-area_weight area_wt] \
                                         [-wirelength_weight wirelength_wt] \
                                         [-outline_weight outline_wt] \
                                         [-boundary_weight boundary_wt] \
                                         [-macro_blockage_weight macro_blockage_wt] \
                                         [-location_weight location_wt] \
                                         [-notch_weight notch_wt] \
                                         [-macro_halo macro_halo] \
                                         [-report_file report_file] \
                                         [-macro_blockage_file macro_blockage_file] \
                                         [-prefer_location_file prefer_location_file] \
                              }
proc rtl_macro_placer { args } {
    sta::parse_key_args "rtl_macro_placer" args keys { -config_file -report_directory
       -area_weight -wirelength_weight -outline_weight
       -boundary_weight -macro_blockage_weight -location_weight -notch_weight
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
    set boundary_wt 25.0
    set macro_blockage_wt 50.0
    set location_wt 100.0
    set notch_wt 212.5

    set macro_halo 10.0
    set report_directory "rtl_mp"
    set report_file "partition.txt"
    set config_file "" 
    set macro_blockage_file "macro_blockage.txt"
    set prefer_location_file "location.txt"

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
                    $outline_wt $boundary_wt $macro_blockage_wt $location_wt $notch_wt $macro_halo\
                    $report_file $macro_blockage_file $prefer_location_file]} {
        return false
    }

    set block [ord::get_db_block]
    set units [$block getDefUnits]
    set macro_placement_file "./${report_directory}/macro_placement.cfg"

    set ch [open $macro_placement_file]

    while {![eof $ch]} {
        set line [gets $ch]
        if {[llength $line] == 0} {continue}

        set inst_name [lindex $line 0]
        set orientation [lindex $line 1]
        set x [expr round([lindex $line 2] * $units)]
        set y [expr round([lindex $line 3] * $units)]

        if {[set inst [$block findInst $inst_name]] == "NULL"} {
            utl::error MPL 4 "Cannot find instance $inst_name."
        }

        $inst setOrient $orientation
        $inst setOrigin $x $y
        $inst setPlacementStatus FIRM
    }

    close $ch

    return true
}
