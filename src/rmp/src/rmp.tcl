############################################################################
##
## Copyright (c) 2019, OpenROAD
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
##
############################################################################

# Restructuring could be done targeting area or timing.
# 
# Argument Description
# liberty_file:    Liberty file with description of cells used in design. This would be passed to ABC.
# target:            "area"|"delay". In area mode focus is area reduction and timing may degrade. In delay mode delay would be reduced but area may increase.
# slack_threshold: specifies slack value below which timing paths need to be analyzed for restructuring
# depth_threshold: specifies the path depth above which a timing path would be considered for restructuring
# locell:          specifies tie cell which can drive constant zero
# loport:          specifies port name of tie low cell
# hicell:          specifies tie cell which can drive constant one
# hiport:          specifies port name of tie high cell
#
# Note that for delay mode slack_threshold and depth_threshold are both considered together.
# Even if slack_threshold is violated, path may not be considered for re-synthesis unless
# depth_threshold is violated as well.

sta::define_cmd_args "restructure" { \
                                      [-slack_threshold slack]\
                                      [-depth_threshold depth]\
                                      [-liberty_file liberty_file]\
                                      [-target area|timing]\
                                      [-tielo_pin tielow_pin]\
                                      [-tiehi_pin tiehigh_pin]
                                    }

proc restructure { args } {
  sta::parse_key_args "restructure" args \
    keys {-slack_threshold -depth_threshold -liberty_file -target -logfile -tielo_pin -tiehi_pin} flags {}

  set slack_threshold_value 0
  set depth_threshold_value 16
  set liberty_file_name ""
  set target "area"

  if { [info exists keys(-slack_threshold)] } {
    set slack_threshold_value $keys(-slack_threshold)
  } 

  if { [info exists keys(-depth_threshold)] } {
    set depth_threshold_value $keys(-depth_threshold)
  }

  if { [info exists keys(-liberty_file)] } {
    set liberty_file_name $keys(-liberty_file)
  } else {
    utl::error RMP 31 "Missing argument -liberty_file"
  }
  
  if { [info exists keys(-target)] } {
    set target $keys(-target)
  }

  if { [info exists keys(-logfile)] } {
    rmp::set_logfile_cmd $keys(-logfile)
  }


  if { [info exists keys(-tielo_pin)] } {
      set lopin $keys(-tielo_pin)
      if { ![sta::is_object $lopin] } {
        set lopin [sta::get_lib_pins $keys(-tielo_pin)]
        if { [llength $lopin] > 1 } {
          # multiple libraries match the lib port arg; use any
          set lopin [lindex $lopin 0]
        }
      }
      if { $lopin != "" } {
        rmp::set_tielo_pin_cmd $lopin
      }
  } else {
      utl::warn RMP 32 "-tielo_pin not specified"
  }

  if { [info exists keys(-tiehi_pin)] } {
      set hipin $keys(-tiehi_pin)
      if { ![sta::is_object $hipin] } {
        set hipin [sta::get_lib_pins $keys(-tiehi_pin)]
        if { [llength $hipin] > 1 } {
          # multiple libraries match the lib port arg; use any
          set hipin [lindex $hipin 0]
        }
      }
      if { $hipin != "" } {
        rmp::set_tiehi_pin_cmd $hipin
      }
  } else {
      utl::warn RMP 33 "-tiehi_pin not specified"
  }

  rmp::restructure_cmd $liberty_file_name $target $slack_threshold_value $depth_threshold_value
}