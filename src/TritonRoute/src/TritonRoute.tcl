#############################################################################
#
# BSD 3-Clause License
#
# Copyright (c) 2020, The Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#############################################################################

sta::define_cmd_args "detailed_route" {
    -param filename
}

proc detailed_route { args } {
  sta::parse_key_args "detailed_route" args keys {-param}
  sta::check_argc_eq0 "detailed_route" $args

  if { ![info exists keys(-param)] } {
    sta::cmd_usage_error "detailed_route"
  }
  tr::detailed_route_cmd $keys(-param)
}

proc detailed_route_num_drvs { args } {
  sta::check_argc_eq0 "detailed_route_num_drvs" $args
  return [tr::detailed_route_num_drvs]
}

sta::define_cmd_args "detailed_route_debug" {
    [-pa]
    [-dr]
    [-maze]
    [-net name]
    [-pin name]
    [-gcell x y]
    [-iter iter]
    [-pa_markers]
}

proc detailed_route_debug { args } {
  sta::parse_key_args "detailed_route_debug" args \
      keys {-net -gcell -iter -pin} \
      flags {-dr -maze -pa -pa_markers}

  sta::check_argc_eq0 "detailed_route_debug" $args

  set dr [info exists flags(-dr)]
  set maze [info exists flags(-maze)]
  set pa [info exists flags(-pa)]
  set pa_markers [info exists flags(-pa_markers)]

  if { [info exists keys(-net)] } {
    set net_name $keys(-net)
  } else {
    set net_name ""
  }

  if { [info exists keys(-pin)] } {
    set pin_name $keys(-pin)
  } else {
    set pin_name ""
  }

  set gcell_x -1
  set gcell_y -1
  if [info exists keys(-gcell)] {
    set gcell $keys(-gcell)
    if { [llength $gcell] != 2 } {
      ord::error DRT 118 "-gcell is a list of 2 coordinates."
    }
    lassign $gcell gcell_x gcell_y
    sta::check_positive_integer "-gcell" $gcell_x
    sta::check_positive_integer "-gcell" $gcell_y
  }

  if { [info exists keys(-iter)] } {
    set iter $keys(-iter)
  } else {
    set iter 0
  }

  tr::set_detailed_route_debug_cmd $net_name $pin_name $dr $pa $maze \
      $gcell_x $gcell_y $iter $pa_markers
}
