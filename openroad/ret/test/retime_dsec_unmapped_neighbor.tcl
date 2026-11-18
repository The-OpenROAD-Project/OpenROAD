# Registers retimed forward across gates the unmapper built for another
# register.  The result is proven sequentially equivalent with ABC dsec when
# EQUIVALENCE_CHECK is set.
source "helpers.tcl"
source "ret_helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_dsec_unmapped_neighbor.v
link_design top
create_clock -period 5.0 clk

proc unmap_gates_reading { net_name } {
  set gates {}
  set net [[ord::get_db_block] findNet $net_name]
  foreach iterm [$net getITerms] {
    set gate [[$iterm getInst] getName]
    if { [$iterm getIoType] == "INPUT" && [string match "unmap_*" $gate] } {
      lappend gates $gate
    }
  }
  return $gates
}

write_verilog_for_dsec retime_dsec_unmapped_neighbor before
report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4

# A register retimed across a gate leaves that gate reading the register's D
# net: rA crosses the inverter and the D & DE gate, where it merges with rD.
set ra_gates [unmap_gates_reading da]
set rd_gates [unmap_gates_reading dd]
set merged [expr { [lindex $rd_gates 0] in $ra_gates }]
if { [llength $ra_gates] != 2 || [llength $rd_gates] != 1 || !$merged } {
  error "rA and rD were not retimed across the gates unmapped from rB"
}

run_dsec_test retime_dsec_unmapped_neighbor \
  -liberty_files sky130hd/sky130hd_tt.lib -clock clk -resetn rst_n

set v_file [make_result_file retime_dsec_unmapped_neighbor.v]
write_verilog $v_file
diff_files retime_dsec_unmapped_neighbor.vok $v_file
