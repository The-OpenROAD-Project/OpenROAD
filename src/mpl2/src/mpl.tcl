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

sta::define_cmd_args "rtl_macro_placer" { -config_file config_file \
                                         [-report_directory report_file] \
                                         -report_file report_file \
                                         [-macro_blockage_file macro_blockage_file] \
                                         [-prefer_location_file prefer_location_file] \
                              }
proc rtl_macro_placer { args } {
    sta::parse_key_args "rtl_macro_placer" args keys { -config_file
       -report_directory -report_file -macro_blockage_file -prefer_location_file } flag {  }

    if { ![info exists keys(-config_file)] } {
        utl::error MPL 2 "Missing mandatory -config_file file"
    }

    if { ![info exists keys(-report_file)] } {
        utl::error MPL 3 "Missing mandatory argument -report_file file"
    }

    set config_file $keys(-config_file)
    set report_file $keys(-report_file)
    set report_directory "rtl_mp"
    set macro_blockage_file "macro_blockage.txt"
    set prefer_location_file "location.txt"

    if { [info exists keys(-report_directory)] } {
        set report_directory $keys(-report_directory)
    }

    if { [info exists keys(-macro_blockage_file)] } {
        set macro_blockage_file $keys(-macro_blockage_file)
    }

    if { [info exists keys(-prefer_location_file)] } {
        set prefer_location_file $keys(-prefer_location_file)
    }

    if {![mpl2::rtl_macro_placer_cmd $config_file $report_directory $report_file $macro_blockage_file $prefer_location_file]} {
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
