# Make sure instances are inside the core.
proc check_in_core {} {
  set core [ord::get_db_core]
  foreach inst [get_cells *] {
    lassign [[sta::sta_to_db_inst $inst] getLocation] x y
    if { ![$core intersects [odb::new_Point $x $y]] } {
      puts "[get_full_name $inst] outside of core"
    }
  }
}

proc check_ties { tie_cell_name } {
  set tie_insts [get_cells -filter "ref_name == $tie_cell_name"]
  foreach tie_inst $tie_insts {
    foreach tie_pin [get_pins -of $tie_inst -filter "direction == output"] {
      set net [get_nets -of $tie_pin]
      if { $net != "NULL" } {
	set pins [get_pins -of $net]
	set ports [get_ports -of $net]
	if { [expr [llength $pins] + [llength $ports]] != 2 } {
	  puts "Warning: tie inst [get_full_name $tie_inst] has [llength $pins] connections."
	}
      }
    }
  }
}

proc report_ties { tie_cell_name } {
  set tie_insts [get_cells -filter "ref_name == $tie_cell_name"]
  set tie_insts [lsort -command inst_cmp_loc $tie_insts]
  foreach tie_inst $tie_insts {
    set db_inst [sta::sta_to_db_inst $tie_inst]
    lassign [$db_inst getLocation] x y
    puts "[format %.1f [ord::dbu_to_microns $x]] [format %.1f [ord::dbu_to_microns $y]]"
  }
}

proc inst_cmp_loc { inst1 inst2 } {
  set db_inst1 [sta::sta_to_db_inst $inst1]
  set db_inst2 [sta::sta_to_db_inst $inst2]
  lassign [$db_inst1 getLocation] x1 y1
  lassign [$db_inst2 getLocation] x2 y2
  if { $x1 < $x2 \
	 || ($x1 == $x2 && $y1 < $y2) } {
    return -1
  } elseif { $x1 == $x2 && $y1 == $y2 } {
    return 0
  } else {
    return 1
  }
}
