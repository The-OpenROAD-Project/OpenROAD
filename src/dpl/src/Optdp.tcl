# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021-2025, The OpenROAD Authors

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

  if { [info exists flags(-disallow_one_site_gaps)] } {
    utl::warn DPO 3 "-disallow_one_site_gaps is deprecated"
  }
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
  dpo::improve_placement_cmd $seed $max_displacement_x $max_displacement_y
}

namespace eval dpo {

}
