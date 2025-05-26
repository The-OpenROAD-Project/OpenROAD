# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

# Restructuring could be done targeting area or timing.
#
# Argument Description
# target:            "area"|"timing". In area mode focus is area reduction and
#                    timing may degrade. In timing mode delay would be reduced
#                    but area may increase.
# slack_threshold: specifies slack value below which timing paths need to be
#                  analyzed for restructuring
# depth_threshold: specifies the path depth above which a timing path would be
#                  considered for restructuring
# tielo_port:      specifies port name of tie low cell in format <cell_name>/<port_name>
# tielo_port:      specifies port name of tie high cell in format <cell_name>/<port_name>
# work_dir:        Name of working directory for temporary files.
#                  If not provided run directory would be used
#
# Note that for delay mode slack_threshold and depth_threshold are both considered together.
# Even if slack_threshold is violated, path may not be considered for re-synthesis unless
# depth_threshold is violated as well.

sta::define_cmd_args "restructure" { \
                                      [-slack_threshold slack]\
                                      [-depth_threshold depth]\
                                      [-target area|timing]\
                                      [-abc_logfile logfile]\
                                      [-liberty_file liberty_file]\
                                      [-tielo_port tielow_port]\
                                      [-tiehi_port tiehigh_port]\
                                      [-work_dir workdir_name]
                                    }

proc restructure { args } {
  sta::parse_key_args "restructure" args \
    keys {-slack_threshold -depth_threshold -target -liberty_file -abc_logfile\
          -tielo_port -tiehi_port -work_dir} \
    flags {}

  set slack_threshold_value 0
  set depth_threshold_value 16
  set target "area"
  set workdir_name "."
  set abc_logfile ""

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
    set abc_logfile $keys(-abc_logfile)
  }

  if { [info exists keys(-liberty_file)] } {
    set liberty_file_name $keys(-liberty_file)
  } else {
    utl::error RMP 12 "Missing argument -liberty_file"
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

  if { [info exists keys(-work_dir)] } {
    set workdir_name $keys(-work_dir)
  }

  rmp::restructure_cmd $liberty_file_name $target $slack_threshold_value \
    $depth_threshold_value $workdir_name $abc_logfile
}

sta::define_cmd_args "resynth" {[-corner corner]}

proc resynth { args } {
  sta::parse_key_args "resynth" args \
    keys {-corner} \
    flags {}
  set corner [sta::parse_corner keys]
  rmp::resynth_cmd $corner
}
