############################################################################
##
## Copyright (c) 2022, The Regents of the University of Minnesota
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

sta::define_cmd_args "analyze_power_grid" {
  [-vsrc vsrc_file ]
  [-outfile out_file]
  [-error_file err_file]
  [-enable_em]
  [-em_outfile em_out_file]
  [-net net_name]
  [-dx bump_pitch_x]
  [-dy bump_pitch_y]
  [-node_density val_node_density]
  [-node_density_factor val_node_density_factor]
  [-corner corner]
  }

proc analyze_power_grid { args } {
  sta::parse_key_args "analyze_power_grid" args \
    keys {-vsrc -outfile -error_file -em_outfile -net -dx -dy -node_density -node_density_factor \
      -corner} \
    flags {-enable_em}
  if { [info exists keys(-vsrc)] } {
    psm::import_vsrc_cfg_cmd $keys(-vsrc)
  }
  if { [info exists keys(-net)] } {
    psm::set_power_net [psm::find_net $keys(-net)]
  } else {
    utl::error PSM 54 "Argument -net not specified."
  }
  if { [info exists keys(-dx)] } {
    set bump_pitch_x $keys(-dx)
    psm::set_bump_pitch_x $bump_pitch_x
  }
  if { [info exists keys(-dy)] } {
    set bump_pitch_y $keys(-dy)
    psm::set_bump_pitch_y $bump_pitch_y
  }

  psm::set_corner [sta::parse_corner_or_default keys]

  if { [info exists keys(-node_density)] && [info exists keys(-node_density_factor)] } {
    utl::error PSM 77 "Cannot use both node_density and node_density_factor together.\
      Use any one argument"
  }

  if { [info exists keys(-node_density)] } {
    set val_node_density $keys(-node_density)
    psm::set_node_density $val_node_density
  }
  if { [info exists keys(-node_density_factor)] } {
    set val_node_density $keys(-node_density_factor)
    psm::set_node_density_factor $val_node_density
  }
  set voltage_file ""
  if { [info exists keys(-outfile)] } {
    set voltage_file $keys(-outfile)
  }
  set error_file ""
  if { [info exists keys(-error_file)] } {
    set error_file $keys(-error_file)
  }
  set enable_em [info exists flags(-enable_em)]
  set em_file ""
  if { [info exists keys(-em_outfile)]} {
    set em_file $keys(-em_outfile)
    if { !$enable_em } {
      utl::error PSM 55 "EM outfile defined without EM enable flag. Add -enable_em."
    }
  }
  if { [ord::db_has_rows] } {
    psm::analyze_power_grid_cmd $voltage_file $enable_em $em_file $error_file
  } else {
    utl::error PSM 56 "No rows defined in design. Floorplan not defined.\
      Use initialize_floorplan to add rows."
  }
}

sta::define_cmd_args "check_power_grid" {
  [-error_file error_file]
  [-net power_net]}

proc check_power_grid { args } {
  sta::parse_key_args "check_power_grid" args \
    keys {-net -error_file} flags {}
  if { [info exists keys(-net)] } {
    psm::set_power_net [psm::find_net $keys(-net)]
  } else {
    utl::error PSM 57 "Argument -net not specified."
  }
  set error_file ""
  if { [info exists keys(-error_file)] } {
    set error_file $keys(-error_file)
  }
  if { [ord::db_has_rows] } {
    if { ![psm::check_connectivity_cmd $error_file] } {
      utl::error PSM 69 "Check connectivity failed."
    }
    return true
  } else {
    utl::error PSM 58 "No rows defined in design. Use initialize_floorplan to add rows."
  }
}

sta::define_cmd_args "write_pg_spice" {
  [-vsrc vsrc_file ]
  [-outfile out_file]
  [-net net_name]
  [-dx bump_pitch_x]
  [-dy bump_pitch_y]
  [-corner corner]
  }

proc write_pg_spice { args } {
  sta::parse_key_args "write_pg_spice" args \
    keys {-vsrc -outfile -net -dx -dy -corner} flags {}
  if { [info exists keys(-vsrc)] } {
    set vsrc_file $keys(-vsrc)
    if { [file readable $vsrc_file] } {
      psm::import_vsrc_cfg_cmd $vsrc_file
    } else {
      utl::error PSM 59 "Cannot read $vsrc_file."
    }
  }
  if { ![info exists keys(-outfile)] } {
    utl::error PSM 85 "Argument -outfile not specified."
  }
  if { [info exists keys(-net)] } {
    psm::set_power_net [psm::find_net $keys(-net)]
  } else {
    utl::error PSM 60 "Argument -net not specified."
  }
  if { [info exists keys(-dx)] } {
    set bump_pitch_x $keys(-dx)
    psm::set_bump_pitch_x $bump_pitch_x
  }
  if { [info exists keys(-dy)] } {
    set bump_pitch_y $keys(-dy)
    psm::set_bump_pitch_y $bump_pitch_y
  }

  psm::set_corner [sta::parse_corner_or_default keys]

  if { [ord::db_has_rows] } {
    psm::write_pg_spice_cmd $keys(-outfile)
  } else {
    utl::error PSM 61 "No rows defined in design.\
      Use initialize_floorplan to add rows and construct PDN."
  }
}

sta::define_cmd_args "set_pdnsim_net_voltage" {
  [-net net_name]
  [-voltage volt]}

proc set_pdnsim_net_voltage { args } {
  sta::parse_key_args "set_pdnsim_net_voltage" args \
    keys {-net -voltage} flags {}
  if { [info exists keys(-net)] && [info exists keys(-voltage)] } {
    set net [psm::find_net $keys(-net)]
    set voltage $keys(-voltage)
    psm::set_net_voltage_cmd $net $voltage
  } else {
    utl::error PSM 62 "Argument -net or -voltage not specified.\
      Please specify both -net and -voltage arguments."
  }
}

namespace eval psm {
proc find_net {net_name} {
  set net [[ord::get_db_block] findNet $net_name]
  if { $net == "NULL" } {
    utl::error PSM 28 "Cannot find net $net_name in the design."
  }
  return $net
}
}
