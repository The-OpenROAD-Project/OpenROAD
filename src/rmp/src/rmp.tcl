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

# Restructuring could be done in various modes targeting area or timing.
# 
# Argument Description
# liberty_file:    Liberty file with description of cells used in design. This would be passed to ABC.
# mode:            "area"|"delay". In area mode focus is area reduction and timing may degrade. In delay mode delay would be reduced but area may increase.
# slack_threshold: specifies slack value below which timing paths need to be analyzed for restructuring
# depth_threshold: specifies the path depth above which a timing path would be considered for restructuring
# locell:          specifies tie cell which can drive constant zero
# loport:          specifies port name of tie low cell
# hicell:          specifies tie cell which can drive constant one
# hiport:          specifies port name of tie high cell

sta::define_cmd_args "restructure" { \
                                      [-slack_threshold slack]\
                                      [-depth_threshold depth]\
                                      [-liberty_file liberty_file]\
                                      [-mode mode_name]\
                                      [-locell tielow_cell]\
                                      [-loport tielow_port]\
                                      [-hiport tiehigh_port]\
                                      [-hicell tiehigh_cell]
                                    }

proc restructure { args } {
  sta::parse_key_args "restructure" args \
    keys {-slack_threshold -depth_threshold -liberty_file -mode -logfile -locell -loport -hicell -hiport} flags {}

  set slack_threshold_value 0
  set depth_threshold_value 16
  set liberty_file_name ""

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

  if { [info exists keys(-mode)] } {
    rmp::set_mode_cmd $keys(-mode)
  }

  if { [info exists keys(-logfile)] } {
    rmp::set_logfile_cmd $keys(-logfile)
  }


  if { [info exists keys(-locell)] } {
    if { [info exists keys(-loport)] } {
      rmp::set_locell_cmd $keys(-locell)
      rmp::set_loport_cmd $keys(-loport)
    } else {
      utl::warn RMP 32 "-loport not specified, skipping locell"
    }
  }

  if { [info exists keys(-hicell)] } {
    if { [info exists keys(-hiport)] } {
      rmp::set_hicell_cmd $keys(-hicell)
      rmp::set_hiport_cmd $keys(-hiport)
    } else {
      utl::warn RMP 33 "-loport not specified, skipping locell"
    }
  }

  rmp::restructure_cmd $liberty_file_name $slack_threshold_value $depth_threshold_value
}