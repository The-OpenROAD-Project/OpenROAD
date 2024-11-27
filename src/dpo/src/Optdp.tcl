###############################################################################
## BSD 3-Clause License
##
## Copyright (c) 2021, Andrew Kennings
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
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.

sta::define_cmd_args "improve_placement" {\
    [-random_seed seed]\
    [-max_displacement disp|{disp_x disp_y}]\
    [-disallow_one_site_gaps]\
}

proc improve_placement { args } {
  sta::parse_key_args "improve_placement" args \
    keys {-random_seed -max_displacement} flags {-disallow_one_site_gaps}

  if { [ord::get_db_block] == "NULL" } {
    utl::error DPO 2 "No design block found."
  }

  set disallow_one_site_gaps [info exists flags(-disallow_one_site_gaps)]
  set seed 1
  if { [info exists keys(-random_seed)] } {
    set seed $keys(-random_seed)
  }
  if { [info exists keys(-max_displacement)] } {
    set max_displacement $keys(-max_displacement)
    if { [llength $max_displacement] == 1 } {
      sta::check_positive_integer "-max_displacement" $max_displacement
      set max_displacement_x $max_displacement
      set max_displacement_y $max_displacement
    } elseif { [llength $max_displacement] == 2 } {
      lassign $max_displacement max_displacement_x max_displacement_y
      sta::check_positive_integer "-max_displacement" $max_displacement_x
      sta::check_positive_integer "-max_displacement" $max_displacement_y
    } else {
      sta::error DPO 31 "-max_displacement disp|{disp_x disp_y}"
    }
  } else {
    # use default displacement
    set max_displacement_x 0
    set max_displacement_y 0
  }

  sta::check_argc_eq0 "improve_placement" $args
  dpo::improve_placement_cmd $seed $max_displacement_x $max_displacement_y $disallow_one_site_gaps
}

namespace eval dpo {

}
