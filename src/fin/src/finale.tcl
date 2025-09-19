# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2020-2025, The OpenROAD Authors

proc density_fill_debug { args } {
  fin::set_density_fill_debug_cmd
}

sta::define_cmd_args "density_fill" {[-rules rules_file]\
                                     [-area {lx ly ux uy}]}

proc density_fill { args } {
  sta::parse_key_args "density_fill" args \
    keys {-rules -area} flags {}

  if { [info exists keys(-rules)] } {
    set rules_file $keys(-rules)
  } else {
    utl::error FIN 7 "The -rules argument must be specified."
  }

  if { [info exists keys(-area)] } {
    set area $keys(-area)
    if { [llength $area] != 4 } {
      utl::error FIN 8 "The -area argument must be a list of 4 coordinates."
    }
    lassign $area lx ly ux uy
    set lx [ord::microns_to_dbu $lx]
    set ly [ord::microns_to_dbu $ly]
    set ux [ord::microns_to_dbu $ux]
    set uy [ord::microns_to_dbu $uy]
    set fill_area [odb::Rect x $lx $ly $ux $uy]
  } else {
    set fill_area [ord::get_db_core]
  }

  fin::density_fill_cmd $rules_file $fill_area
}
