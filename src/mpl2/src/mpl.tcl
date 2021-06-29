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
                              }
proc rtl_macro_placer { args } {
    sta::parse_key_args "rtl_macro_placer" args keys { -config_file } flag {  }
    if { [info exists keys(-config_file)] } {
        set config_file $keys(-config_file)
        return [mpl2::rtl_macro_placer_cmd $config_file]
    } else {
        utl::error "MPL" 2 "rtl_macro_placer -config_file config_file"
    }
    
    set block [ord::get_db_block]
    set units [$block getDefUnits]
    set macro_placement_file "./rtl_mp/macro_placement.cfg"

    set ch [open $macro_placement_file]

    while {![eof $ch]} {
        set line [gets $ch]
        if {[llength $line] == 0} {continue}

        set inst_name [lindex $line 0]
        set orientation [lindex $line 1]
        set x [expr round([lindex $line 2] * $units)]
        set y [expr round([lindex $line 3] * $units)]

        if {[set inst [$block findInst $inst_name]] == "NULL"} {
            error "Cannot find instance $inst_name"
        }

        $inst setOrient $orientation
        $inst setOrigin $x $y
        $inst setPlacementStatus FIRM
    }

    close $ch
}
