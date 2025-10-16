source "helpers.tcl"
read_lef example1.lef
read_liberty example1_typ.lib
read_verilog hier_deep.v
link_design top -hier

proc test_get_cells { pattern } {
  set cells [get_cells $pattern]
  puts "cmd: get_cells $pattern"
  puts "count: [llength $cells]"
  foreach cell $cells {
    puts [get_full_name $cell]
  }
  puts "----------------------"
}

test_get_cells "level1_inst"
test_get_cells "level1_inst/level2_inst"
test_get_cells "level1_inst/level2_inst/leaf_inst"
test_get_cells "level1_inst/level2_inst/leaf_inst/inverter"

test_get_cells "level1_*"
test_get_cells "*level2*"
test_get_cells "*leaf*"
test_get_cells "*inv*"
test_get_cells "*/level2*inv*"
