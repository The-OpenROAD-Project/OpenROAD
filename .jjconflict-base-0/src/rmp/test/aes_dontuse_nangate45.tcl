source "helpers.tcl"

read_liberty ./Nangate45/Nangate45_typ.lib
read_lef ./Nangate45/Nangate45.lef
read_lef ./Nangate45/Nangate45_stdcell.lef
read_verilog ./aes_nangate45.v
link_design aes_cipher_top
read_sdc ./aes_nangate45.sdc

# Set dont use for all X1 cells
set_dont_use *_X1

# Unset dont use for tie cells
unset_dont_use *LOGIC*

puts "-- Before --\n"
report_cell_usage
report_timing_histogram
report_checks
report_wns
report_tns

puts "-- After --\n"

resynth
report_timing_histogram
report_cell_usage
report_checks
report_wns
report_tns

proc list_rmp_cells { } {
  set insts [odb::dbBlock_getInsts [ord::get_db_block]]
  set used_rmp_cells {}
  foreach inst $insts {
    set name [odb::dbInst_getName $inst]
    if { [string match "*cut*" $name] } {
      set master [odb::dbInst_getMaster $inst]
      set master_name [odb::dbMaster_getName $master]
      lappend used_rmp_cells $master_name
    }
  }
  set used_rmp_cells [lsort -unique $used_rmp_cells]
  puts "Used RMP cells:"
  foreach cell $used_rmp_cells {
    puts "  $cell"
  }
}

# Check that no X1 cell was used by RMP on first pass
list_rmp_cells

# Reset dont use
reset_dont_use
unset_dont_use *LOGIC*

resynth
report_timing_histogram
report_cell_usage
report_checks
report_wns
report_tns

# Check that X1 cells are used again
list_rmp_cells
