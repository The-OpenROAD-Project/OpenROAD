# defaults
set slew_margin 0
set cap_margin 0
set power_corner "default"

proc read_libraries { } {
  global tech_lef std_cell_lef extra_lef
  global liberty_file liberty_files extra_liberty

  read_lef $tech_lef
  read_lef $std_cell_lef
  foreach file $extra_lef { read_lef $file }
  set corners [sta::scenes]
  if { [llength $corners] > 1 } {
    foreach corner $corners {
      set corner_name [$corner name]
      set corner_index [lsearch $liberty_files $corner_name]
      if { $corner_index == -1 } {
        error "No liberty file in \$liberty_files for corner $corner_name."
      } else {
        set liberty_file [lindex $liberty_files [expr $corner_index + 1]]
        read_liberty -corner $corner_name $liberty_file
      }
    }
  } else {
    read_liberty $liberty_file
  }
  foreach file $extra_liberty { read_liberty $file }
}

proc have_macros { } {
  set db [::ord::get_db]
  set block [[$db getChip] getBlock]
  foreach inst [$block getInsts] {
    set inst_master [$inst getMaster]
    # BLOCK means MACRO cells
    if { [string match [$inst_master getType] "BLOCK"] } {
      return 1
    }
  }
  return 0
}

proc derate_layer_wire_rc { layer_name corner derate_factor } {
  set layer [[ord::get_db_tech] findLayer $layer_name]
  lassign [rsz::dblayer_wire_rc $layer] r c
  # ohm/meter -> kohm/micron
  set r_ui [expr $r * 1e-3 * 1e-6]
  # F/meter -> fF/micron
  set c_ui [expr $c * 1e+15 * 1e-6]
  set_layer_rc -layer $layer_name -corner $corner \
    -resistance [expr $r_ui * $derate_factor] \
    -capacitance [expr $c_ui * $derate_factor]
}

proc set_all_input_output_delays { { clk_period_factor .2 } } {
  set clk [lindex [all_clocks] 0]
  set period [get_property $clk period]
  set delay [expr $period * $clk_period_factor]
  set_input_delay $delay -clock $clk [delete_from_list [all_inputs] [all_clocks]]
  set_output_delay $delay -clock $clk [delete_from_list [all_outputs] [all_clocks]]
}
