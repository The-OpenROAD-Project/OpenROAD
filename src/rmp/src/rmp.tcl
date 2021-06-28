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
                                      [-target area|timing]\
                                      [-tielo_port tielow_port]\
                                      [-tiehi_port tiehigh_port]
                                    }

proc restructure { args } {
  sta::parse_key_args "restructure" args \
    keys {-slack_threshold -depth_threshold -target -abc_logfile -tielo_port -tiehi_port} flags {}

  set slack_threshold_value 0
  set depth_threshold_value 16
  set target "area"

  if { [info exists keys(-slack_threshold)] } {
    set slack_threshold_value $keys(-slack_threshold)
  } 

  if { [info exists keys(-depth_threshold)] } {
    set depth_threshold_value $keys(-depth_threshold)
  }

  if { [info exists keys(-target)] } {
    set target $keys(-target)
  }

  if { [info exists keys(-abc_logfile)] } {
    rmp::set_logfile_cmd $keys(-abc_logfile)
  }


  if { [info exists keys(-tielo_port)] } {
      set loport $keys(-tielo_port)
      if { ![sta::is_object $loport] } {
        set loport [sta::get_lib_pins $keys(-tielo_port)]
        if { [llength $loport] > 1 } {
          # multiple libraries match the lib port arg; use any
          set loport [lindex $loport 0]
        }
      }
      if { $loport != "" } {
        rmp::set_tielo_port_cmd $loport
      }
  } else {
      utl::warn RMP 32 "-tielo_port not specified"
  }

  if { [info exists keys(-tiehi_port)] } {
      set hiport $keys(-tiehi_port)
      if { ![sta::is_object $hiport] } {
        set hiport [sta::get_lib_pins $keys(-tiehi_port)]
        if { [llength $hiport] > 1 } {
          # multiple libraries match the lib port arg; use any
          set hiport [lindex $hiport 0]
        }
      }
      if { $hiport != "" } {
        rmp::set_tiehi_port_cmd $hiport
      }
  } else {
      utl::warn RMP 33 "-tiehi_port not specified"
  }

  rmp::restructure_cmd $target $slack_threshold_value $depth_threshold_value
}