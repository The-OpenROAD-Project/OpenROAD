#############################################################################
##
## Copyright (c) 2019, James Cherry, Parallax Software, Inc.
## All rights reserved.
##
## BSD 3-Clause License
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
#############################################################################

sta::define_cmd_args "legalize_placement" {[-verbose]\
					     [-constraints constraints_file]\
					   }

proc legalize_placement { args } {
  sta::parse_key_args "legalize_placement" args \
    keys {-pad_right -pad_left -constraints} flags {-verbose}

  set verbose [info exists flags(-verbose)]
  if { [info exists keys(-constraints)] } {
    set constraints_file $keys(-constraints)
    if { [file readable $constraints_file] } {
      opendp::read_constraints $constraint_file
    } else {
      puts "Warning: cannot read $constraints_file"
    }
  }

  if { [ord::db_has_rows] } {
    opendp::legalize_placement $verbose
  } else {
    puts "Error: no rows defined in design. Use initialize_floorplan to add rows."
  }
}

sta::define_cmd_args "set_padding" { [-global]\
				       [-right site_count]\
				       [-left site_count] \
				     }

proc set_padding { args } {
  sta::parse_key_args "set_padding" args \
    keys {-right -left} flags {-global}

  set left 0
  if { [info exists keys(-left)] } {
    set left $keys(-left)
    sta::check_positive_integer "-left" $left
  }
  set right 0
  if { [info exists keys(-right)] } {
    set right $keys(-right)
    sta::check_positive_integer "-right" $right
  }
  set global [info exists flags(-global)]
  if { $global } {
    opendp::set_padding_global $left $right
  } else {
    sta::sta_error "Only set_padding -global supported."
  }
}
